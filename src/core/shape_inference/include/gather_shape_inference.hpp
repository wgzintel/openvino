// Copyright (C) 2018-2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//
#pragma once
#include <openvino/op/util/gather_base.hpp>

#include "utils.hpp"

namespace ov {
namespace op {
namespace util {
template <class T>
void shape_infer(const GatherBase* op,
                 const std::vector<T>& input_shapes,
                 std::vector<T>& output_shapes,
                 const std::map<size_t, std::shared_ptr<ngraph::runtime::HostTensor>>& constant_data = {}) {
    NODE_VALIDATION_CHECK(op, input_shapes.size() == 3 && output_shapes.size() == 1);
    const auto& data_pshape = input_shapes[0];
    const auto& indices_pshape = input_shapes[1];
    const auto& axis_pshape = input_shapes[2];
    auto data_rank = data_pshape.rank();
    auto indices_rank = indices_pshape.rank();
    auto axis_rank = axis_pshape.rank();
    auto& output_pshape = output_shapes[0];

    if (axis_rank.is_static() && axis_pshape.is_static()) {
        const auto axis_is_scalar = axis_rank.get_length() == 0;
        const auto axis_has_one_elem = axis_rank.get_length() == 1 && axis_pshape[0].get_length() == 1;
        NODE_VALIDATION_CHECK(op,
                              axis_is_scalar || axis_has_one_elem,
                              "Axis input must be scalar or have 1 element. But instead got axis_shape = ",
                              axis_pshape);
    }

    int64_t batch_dims = op->get_batch_dims();
    if (batch_dims < 0 && indices_rank.is_static()) {
        batch_dims += indices_rank.get_length();
    }

    std::vector<int64_t> axes_val;
    bool axis_is_set = get_data_as_int64<T>(2, op, axes_val, constant_data);
    int64_t axis = 0;

    if (axis_is_set) {
        axis = axes_val[0];

        if (data_rank.is_static()) {
            OPENVINO_SUPPRESS_DEPRECATED_START
            axis = ov::normalize_axis(op, axis, data_rank);
            OPENVINO_SUPPRESS_DEPRECATED_END
        }
        // batch_dims, axis both can be positive by default or after normalization if data_rank &
        // indices_rank are static.
        // If at least one of them is negative we cannot check their consistency.
        NODE_VALIDATION_CHECK(op,
                              batch_dims <= axis || batch_dims < 0 || axis < 0,
                              "After normalization batch_dims must be <= axis. But instead got: batch_dims = ",
                              batch_dims,
                              ", axis = ",
                              axis);
    }

    if (indices_rank.is_static() && batch_dims >= 0) {
        NODE_VALIDATION_CHECK(op,
                              batch_dims <= indices_rank.get_length(),
                              "The batch_dims must be <= indices_rank. But instead got: batch_dims = ",
                              batch_dims,
                              ", indices_rank = ",
                              indices_rank.get_length());
    }

    if (data_rank.is_static() && indices_rank.is_static()) {
        auto out_rank = data_rank.get_length() + indices_rank.get_length() - 1 - batch_dims;
        // scalar has one
        output_pshape.resize(out_rank);

        // implementation of out_shape formula
        // data.shape[:batch_dims] + data.shape[batch_dims:axis] + indices.shape[batch_dims:] +
        // data.shape[axis + 1:]
        int i = 0;
        for (; i < batch_dims; i++) {
            NODE_VALIDATION_CHECK(op,
                                  data_pshape[i].compatible(indices_pshape[i]),
                                  "Shapes ",
                                  data_pshape,
                                  " and ",
                                  indices_pshape,
                                  " are not consistent. data and indices must have equal or "
                                  "intersecting sizes until batch_dims");

            output_pshape[i] = data_pshape[i] & indices_pshape[i];
        }

        if (axis_is_set) {
            for (; i < axis; i++) {
                output_pshape[i] = data_pshape[i];
            }
            for (; i < axis + indices_rank.get_length() - batch_dims; i++) {
                output_pshape[i] = indices_pshape[batch_dims - axis + i];
            }
            for (; i < out_rank; i++) {
                output_pshape[i] = data_pshape[batch_dims + 1 - indices_rank.get_length() + i];
            }
        }
    } else {
        auto out_rank = data_rank + indices_rank - 1 - batch_dims;
        if (batch_dims < 0)
            out_rank = out_rank - indices_rank.get_max_length();
        output_pshape = PartialShape::dynamic(out_rank);
    }
}
}  // namespace util
}  // namespace op
}  // namespace ov
