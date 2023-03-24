// Copyright (C) 2018-2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

///////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "multi_schedule.hpp"

#ifdef  MULTIUNITTEST
#define MOCKTESTMACRO virtual
#define MultiDevicePlugin MockMultiDevicePlugin
#else
#define MOCKTESTMACRO
#endif
namespace MultiDevicePlugin {
struct AutoLoadContext {
    std::atomic<bool> isEnabled = {false};
    std::atomic<bool> isAlready = {false};
    std::atomic<bool> isLoadSuccess = {false};
    std::atomic<bool> isReloadSuccess = {false};
    std::future<void> future;
    std::promise<void> promise;
    SoExecNetwork executableNetwork;
    DeviceInformation  deviceInfo;
    std::vector<DeviceInformation> metaDevices;
    std::string networkPrecision;
    std::string errMessage;
    IE::Task task;
    //ACTUALDEVICE's workName is same as it's deviceName,
    //CPU_HELP's workName is "CPU_HELP", and  deviceName is "CPU"
    //add workName is because of ACTUALDEVICE and CPU maybe all CPU,
    //they can't use the same workerQueue
    std::string workName = "";
};

enum AutoLoadContextIndex {
    CPU = 0,
    ACTUALDEVICE = 1,
    FALLBACKDEVICE = 2,
    CONTEXTNUM = 3
};
class AutoSchedule : public MultiSchedule {
public:
    using Ptr = std::shared_ptr<AutoSchedule>;
    void init(const ScheduleContext::Ptr& sContext) override;
    IInferPtr CreateInferRequest() override;
    void WaitActualNetworkReady() const;
    virtual ~AutoSchedule();

public:
    AutoLoadContext                           _loadContext[CONTEXTNUM];
    std::unique_ptr<AutoLoadContext[]>        _pCTPUTLoadContext = nullptr;

protected:
    void GenerateWorkers(const std::string& device, const SoExecNetwork& executableNetwork) override;
    bool ScheduleToWorkerInferRequest(IE::Task, DeviceName preferred_device = "") override;
    static bool RunPipelineTask(IE::Task& inferPipelineTask, NotBusyPriorityWorkerRequests& idleWorkerRequests,
                                const DeviceName& preferred_device);
    DeviceMap<NotBusyPriorityWorkerRequests> _idleWorkerRequests;
    AutoScheduleContext::Ptr                 _autoSContext;

private:
    void WaitFirstNetworkReady();
    void TryToLoadNetWork(AutoLoadContext& context, const std::string& modelPath, const IE::CNNNetwork& network, bool isCumulative);
    bool selectOtherDevice(const std::string& currentDeviceName);
    IE::Task releaseActualdeviceTask;

private:
    IE::IStreamsExecutor::Ptr                _executor;
    mutable std::once_flag                   _oc;
    std::once_flag                           _firstLoadOC;
    std::future<void>                        _firstLoadFuture;
    std::promise<void>                       _firstLoadPromise;
    bool                                     _exitFlag = {false};
    size_t                                   _cpuHelpInferCount = 0;
};

}  // namespace MultiDevicePlugin
