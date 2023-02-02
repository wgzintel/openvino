// Copyright (C) 2018-2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#include "openvino/frontend/pytorch/node_context.hpp"
#include "openvino/op/grid_sample.hpp"

namespace ov {
namespace frontend {
namespace pytorch {
namespace op {

OutputVector translate_grid_sampler(NodeContext& context) {
    auto x = context.get_input(0);
    auto grid = context.get_input(1);
    ov::op::v9::GridSample::Attributes attrs{};
    const std::unordered_map<int64_t, ov::op::v9::GridSample::InterpolationMode> grid_sample_mode_map{
        {0, ov::op::v9::GridSample::InterpolationMode::BILINEAR},
        {1, ov::op::v9::GridSample::InterpolationMode::NEAREST},
        {2, ov::op::v9::GridSample::InterpolationMode::BICUBIC},
    };
    const std::unordered_map<int64_t, ov::op::v9::GridSample::PaddingMode> grid_sample_padding_mode_map{
        {0, ov::op::v9::GridSample::PaddingMode::ZEROS},
        {1, ov::op::v9::GridSample::PaddingMode::BORDER},
        {2, ov::op::v9::GridSample::PaddingMode::REFLECTION}};
    auto mode = context.const_input<int64_t>(2);
    FRONT_END_OP_CONVERSION_CHECK(grid_sample_mode_map.count(mode), "Unknown interpolation mode: ", mode);
    attrs.mode = grid_sample_mode_map.at(mode);
    auto padding_mode = context.const_input<int64_t>(3);
    FRONT_END_OP_CONVERSION_CHECK(grid_sample_padding_mode_map.count(padding_mode),
                                  "Unknown padding mode: ",
                                  padding_mode);
    attrs.padding_mode = grid_sample_padding_mode_map.at(padding_mode);
    bool align_corners = false;
    if (!context.input_is_none(4)) {
        align_corners = context.const_input<int64_t>(4);
    }
    attrs.align_corners = align_corners;

    return {context.mark_node(std::make_shared<ov::op::v9::GridSample>(x, grid, attrs))};
};

}  // namespace op
}  // namespace pytorch
}  // namespace frontend
}  // namespace ov