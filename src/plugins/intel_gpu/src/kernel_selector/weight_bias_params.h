// Copyright (C) 2018-2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "kernel_selector_params.h"

namespace kernel_selector {

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// weight_bias_params
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct weight_bias_params : public base_params {
    explicit weight_bias_params(KernelType kt) : base_params(kt) {}

    WeightsTensor weights;
    MultiDataTensor bias;

    ParamsKey GetParamsKey() const override;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// weight_bias_params
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct weight_bias_zero_point_params : public weight_bias_params {
    using weight_bias_params::weight_bias_params;

    MultiDataTensor weights_zero_points;
    MultiDataTensor activations_zero_points;
    MultiDataTensor compensation;

    bool HasCompensation() const { return !compensation.empty(); }
    std::string to_cache_string_v2() const override;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// weight_bias_optional_params
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct weight_bias_optional_params : optional_params {
protected:
    explicit weight_bias_optional_params(KernelType kt) : optional_params(kt) {}
};

}  // namespace kernel_selector
