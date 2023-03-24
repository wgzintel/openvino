// Copyright (C) 2018-2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

///////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "auto_schedule.hpp"

#ifdef  MULTIUNITTEST
#define MOCKTESTMACRO virtual
#define MultiDevicePlugin MockMultiDevicePlugin
#else
#define MOCKTESTMACRO
#endif

namespace MultiDevicePlugin {
class BinderMultiSchedule : public AutoSchedule {
public:
    using Ptr = std::shared_ptr<BinderMultiSchedule>;
    IInferPtr CreateInferRequestImpl(IE::InputsDataMap networkInputs, IE::OutputsDataMap networkOutputs) override;
    IE::IInferRequestInternal::Ptr CreateInferRequestImpl(const std::vector<std::shared_ptr<const ov::Node>>& inputs,
                                                          const std::vector<std::shared_ptr<const ov::Node>>& outputs) override;
    void init(const ScheduleContext::Ptr& sContext) override;
    Pipeline GetPipeline(const IInferPtr& syncRequestImpl, WorkerInferRequest** WorkerInferRequest) override;
    virtual ~BinderMultiSchedule();
};
}  // namespace MultiDevicePlugin
