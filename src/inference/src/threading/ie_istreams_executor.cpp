// Copyright (C) 2018-2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#include "threading/ie_istreams_executor.hpp"

#include <algorithm>
#include <openvino/util/log.hpp>
#include <string>
#include <thread>
#include <vector>

#include "cpp_interfaces/interface/ie_internal_plugin_config.hpp"
#include "ie_parallel.hpp"
#include "ie_parallel_custom_arena.hpp"
#include "ie_parameter.hpp"
#include "ie_plugin_config.hpp"
#include "ie_system_conf.h"
#include "openvino/runtime/properties.hpp"
#include "openvino/util/common_util.hpp"

namespace InferenceEngine {
IStreamsExecutor::~IStreamsExecutor() {}

std::vector<std::string> IStreamsExecutor::Config::SupportedKeys() const {
    return {
        CONFIG_KEY(CPU_THROUGHPUT_STREAMS),
        CONFIG_KEY(CPU_BIND_THREAD),
        CONFIG_KEY(CPU_THREADS_NUM),
        CONFIG_KEY_INTERNAL(CPU_THREADS_PER_STREAM),
        CONFIG_KEY_INTERNAL(BIG_CORE_STREAMS),
        CONFIG_KEY_INTERNAL(SMALL_CORE_STREAMS),
        CONFIG_KEY_INTERNAL(THREADS_PER_STREAM_BIG),
        CONFIG_KEY_INTERNAL(THREADS_PER_STREAM_SMALL),
        CONFIG_KEY_INTERNAL(SMALL_CORE_OFFSET),
        CONFIG_KEY_INTERNAL(ENABLE_HYPER_THREAD),
        ov::num_streams.name(),
        ov::inference_num_threads.name(),
        ov::affinity.name(),
    };
}
int IStreamsExecutor::Config::GetDefaultNumStreams(const bool enable_hyper_thread) {
    const int sockets = static_cast<int>(getAvailableNUMANodes().size());
    // bare minimum of streams (that evenly divides available number of core)
    const int num_cores = sockets == 1 ? (enable_hyper_thread ? parallel_get_max_threads() : getNumberOfCPUCores())
                                       : getNumberOfCPUCores();
    if (0 == num_cores % 4)
        return std::max(4, num_cores / 4);
    else if (0 == num_cores % 5)
        return std::max(5, num_cores / 5);
    else if (0 == num_cores % 3)
        return std::max(3, num_cores / 3);
    else  // if user disables some cores say in BIOS, so we got weird #cores which is not easy to divide
        return 1;
}

int IStreamsExecutor::Config::GetHybridNumStreams(std::map<std::string, std::string>& config, const int stream_mode) {
    const int num_cores = parallel_get_max_threads();
    const int num_cores_phy = getNumberOfCPUCores();
    const int num_big_cores_phy = getNumberOfCPUCores(true);
    const int num_small_cores = num_cores_phy - num_big_cores_phy;
    const int num_big_cores = num_cores > num_cores_phy ? num_big_cores_phy * 2 : num_big_cores_phy;
    int big_core_streams = 0;
    int small_core_streams = 0;
    int threads_per_stream_big = 0;
    int threads_per_stream_small = 0;

    if (stream_mode == DEFAULT) {
        // bare minimum of streams (that evenly divides available number of core)
        if (0 == num_big_cores_phy % 4) {
            threads_per_stream_big = 4;
        } else if (0 == num_big_cores_phy % 5) {
            threads_per_stream_big = 5;
        } else if (0 == num_big_cores_phy % 3) {
            threads_per_stream_big = 3;
        } else {  // if user disables some cores say in BIOS, so we got weird #cores which is not easy to divide
            threads_per_stream_big = num_big_cores_phy;
        }

        big_core_streams = num_big_cores / threads_per_stream_big;
        threads_per_stream_small = threads_per_stream_big;
        if (num_small_cores == 0) {
            threads_per_stream_small = 0;
        } else if (num_small_cores < threads_per_stream_small) {
            small_core_streams = 1;
            threads_per_stream_small = num_small_cores;
            threads_per_stream_big = threads_per_stream_small;
            // Balance the computation of physical core and logical core, the number of threads on the physical core and
            // logical core should be equal
            big_core_streams = num_big_cores_phy / threads_per_stream_big * 2;
        } else {
            small_core_streams = num_small_cores / threads_per_stream_small;
        }
    } else if (stream_mode == AGGRESSIVE) {
        big_core_streams = num_big_cores;
        small_core_streams = num_small_cores;
        threads_per_stream_big = num_big_cores / big_core_streams;
        threads_per_stream_small = num_small_cores == 0 ? 0 : num_small_cores / small_core_streams;
    } else if (stream_mode == LESSAGGRESSIVE) {
        big_core_streams = num_big_cores / 2;
        small_core_streams = num_small_cores / 2;
        threads_per_stream_big = num_big_cores / big_core_streams;
        threads_per_stream_small = num_small_cores == 0 ? 0 : num_small_cores / small_core_streams;
    } else {
        IE_THROW() << "Wrong stream mode to get num of streams: " << stream_mode;
    }
    config[CONFIG_KEY_INTERNAL(BIG_CORE_STREAMS)] = std::to_string(big_core_streams);
    config[CONFIG_KEY_INTERNAL(SMALL_CORE_STREAMS)] = std::to_string(small_core_streams);
    config[CONFIG_KEY_INTERNAL(THREADS_PER_STREAM_BIG)] = std::to_string(threads_per_stream_big);
    config[CONFIG_KEY_INTERNAL(THREADS_PER_STREAM_SMALL)] = std::to_string(threads_per_stream_small);
    // This is default setting for specific CPU which Pcore is in front and Ecore is in the back.
    config[CONFIG_KEY_INTERNAL(SMALL_CORE_OFFSET)] = std::to_string(num_small_cores == 0 ? 0 : num_big_cores);
    return big_core_streams + small_core_streams;
}

void IStreamsExecutor::Config::SetConfig(const std::string& key, const std::string& value) {
    if (key == CONFIG_KEY(CPU_BIND_THREAD)) {
        if (value == CONFIG_VALUE(YES) || value == CONFIG_VALUE(NUMA)) {
#if (defined(__APPLE__) || defined(_WIN32))
            _threadBindingType = IStreamsExecutor::ThreadBindingType::NUMA;
#else
            _threadBindingType = (value == CONFIG_VALUE(YES)) ? IStreamsExecutor::ThreadBindingType::CORES
                                                              : IStreamsExecutor::ThreadBindingType::NUMA;
#endif
        } else if (value == CONFIG_VALUE(HYBRID_AWARE)) {
            _threadBindingType = IStreamsExecutor::ThreadBindingType::HYBRID_AWARE;
        } else if (value == CONFIG_VALUE(NO)) {
            _threadBindingType = IStreamsExecutor::ThreadBindingType::NONE;
        } else {
            IE_THROW() << "Wrong value for property key " << CONFIG_KEY(CPU_BIND_THREAD)
                       << ". Expected only YES(binds to cores) / NO(no binding) / NUMA(binds to NUMA nodes) / "
                          "HYBRID_AWARE (let the runtime recognize and use the hybrid cores)";
        }
    } else if (key == ov::affinity) {
        ov::Affinity affinity;
        std::stringstream{value} >> affinity;
        switch (affinity) {
        case ov::Affinity::NONE:
            _threadBindingType = ThreadBindingType::NONE;
            break;
        case ov::Affinity::CORE: {
#if (defined(__APPLE__) || defined(_WIN32))
            _threadBindingType = ThreadBindingType::NUMA;
#else
            _threadBindingType = ThreadBindingType::CORES;
#endif
        } break;
        case ov::Affinity::NUMA:
            _threadBindingType = ThreadBindingType::NUMA;
            break;
        case ov::Affinity::HYBRID_AWARE:
            _threadBindingType = ThreadBindingType::HYBRID_AWARE;
            break;
        default:
            OPENVINO_UNREACHABLE("Unsupported affinity type");
        }
    } else if (key == CONFIG_KEY(CPU_THROUGHPUT_STREAMS)) {
        if (value == CONFIG_VALUE(CPU_THROUGHPUT_NUMA)) {
            _streams = static_cast<int>(getAvailableNUMANodes().size());
        } else if (value == CONFIG_VALUE(CPU_THROUGHPUT_AUTO)) {
            // bare minimum of streams (that evenly divides available number of cores)
            _streams = GetDefaultNumStreams();
        } else {
            int val_i;
            try {
                val_i = std::stoi(value);
            } catch (const std::exception&) {
                IE_THROW() << "Wrong value for property key " << CONFIG_KEY(CPU_THROUGHPUT_STREAMS)
                           << ". Expected only positive numbers (#streams) or "
                           << "PluginConfigParams::CPU_THROUGHPUT_NUMA/CPU_THROUGHPUT_AUTO";
            }
            if (val_i < 0) {
                IE_THROW() << "Wrong value for property key " << CONFIG_KEY(CPU_THROUGHPUT_STREAMS)
                           << ". Expected only positive numbers (#streams)";
            }
            _streams = val_i;
        }
    } else if (key == ov::num_streams) {
        auto streams = ov::util::from_string(value, ov::streams::num);
        if (streams == ov::streams::NUMA) {
            _streams = static_cast<int32_t>(getAvailableNUMANodes().size());
        } else if (streams == ov::streams::AUTO) {
            // bare minimum of streams (that evenly divides available number of cores)
            _streams = GetDefaultNumStreams();
        } else if (streams.num >= 0) {
            _streams = streams.num;
        } else {
            OPENVINO_UNREACHABLE("Wrong value for property key ",
                                 ov::num_streams.name(),
                                 ". Expected non negative numbers (#streams) or ",
                                 "ov::streams::NUMA|ov::streams::AUTO, Got: ",
                                 streams);
        }
    } else if (key == CONFIG_KEY(CPU_THREADS_NUM) || key == ov::inference_num_threads) {
        int val_i;
        try {
            val_i = std::stoi(value);
        } catch (const std::exception&) {
            IE_THROW() << "Wrong value for property key " << CONFIG_KEY(CPU_THREADS_NUM)
                       << ". Expected only positive numbers (#threads)";
        }
        if (val_i < 0) {
            IE_THROW() << "Wrong value for property key " << CONFIG_KEY(CPU_THREADS_NUM)
                       << ". Expected only positive numbers (#threads)";
        }
        _threads = val_i;
    } else if (key == CONFIG_KEY_INTERNAL(CPU_THREADS_PER_STREAM)) {
        int val_i;
        try {
            val_i = std::stoi(value);
        } catch (const std::exception&) {
            IE_THROW() << "Wrong value for property key " << CONFIG_KEY_INTERNAL(CPU_THREADS_PER_STREAM)
                       << ". Expected only non negative numbers (#threads)";
        }
        if (val_i < 0) {
            IE_THROW() << "Wrong value for property key " << CONFIG_KEY_INTERNAL(CPU_THREADS_PER_STREAM)
                       << ". Expected only non negative numbers (#threads)";
        }
        _threadsPerStream = val_i;
    } else if (key == CONFIG_KEY_INTERNAL(BIG_CORE_STREAMS)) {
        int val_i;
        try {
            val_i = std::stoi(value);
        } catch (const std::exception&) {
            IE_THROW() << "Wrong value for HYBRID_AWARE key " << CONFIG_KEY_INTERNAL(BIG_CORE_STREAMS)
                       << ". Expected only non negative numbers (#streams)";
        }
        if (val_i < 0) {
            IE_THROW() << "Wrong value for HYBRID_AWARE key " << CONFIG_KEY_INTERNAL(BIG_CORE_STREAMS)
                       << ". Expected only non negative numbers (#streams)";
        }
        _big_core_streams = val_i;
    } else if (key == CONFIG_KEY_INTERNAL(SMALL_CORE_STREAMS)) {
        int val_i;
        try {
            val_i = std::stoi(value);
        } catch (const std::exception&) {
            IE_THROW() << "Wrong value for HYBRID_AWARE key " << CONFIG_KEY_INTERNAL(SMALL_CORE_STREAMS)
                       << ". Expected only non negative numbers (#streams)";
        }
        if (val_i < 0) {
            IE_THROW() << "Wrong value for HYBRID_AWARE key " << CONFIG_KEY_INTERNAL(SMALL_CORE_STREAMS)
                       << ". Expected only non negative numbers (#streams)";
        }
        _small_core_streams = val_i;
    } else if (key == CONFIG_KEY_INTERNAL(THREADS_PER_STREAM_BIG)) {
        int val_i;
        try {
            val_i = std::stoi(value);
        } catch (const std::exception&) {
            IE_THROW() << "Wrong value for HYBRID_AWARE key " << CONFIG_KEY_INTERNAL(THREADS_PER_STREAM_BIG)
                       << ". Expected only non negative numbers (#threads)";
        }
        if (val_i < 0) {
            IE_THROW() << "Wrong value for HYBRID_AWARE key " << CONFIG_KEY_INTERNAL(THREADS_PER_STREAM_BIG)
                       << ". Expected only non negative numbers (#threads)";
        }
        _threads_per_stream_big = val_i;
    } else if (key == CONFIG_KEY_INTERNAL(THREADS_PER_STREAM_SMALL)) {
        int val_i;
        try {
            val_i = std::stoi(value);
        } catch (const std::exception&) {
            IE_THROW() << "Wrong value for HYBRID_AWARE key " << CONFIG_KEY_INTERNAL(THREADS_PER_STREAM_SMALL)
                       << ". Expected only non negative numbers (#threads)";
        }
        if (val_i < 0) {
            IE_THROW() << "Wrong value for HYBRID_AWARE key " << CONFIG_KEY_INTERNAL(THREADS_PER_STREAM_SMALL)
                       << ". Expected only non negative numbers (#threads)";
        }
        _threads_per_stream_small = val_i;
    } else if (key == CONFIG_KEY_INTERNAL(SMALL_CORE_OFFSET)) {
        int val_i;
        try {
            val_i = std::stoi(value);
        } catch (const std::exception&) {
            IE_THROW() << "Wrong value for HYBRID_AWARE key " << CONFIG_KEY_INTERNAL(SMALL_CORE_OFFSET)
                       << ". Expected only non negative numbers";
        }
        if (val_i < 0) {
            IE_THROW() << "Wrong value for HYBRID_AWARE key " << CONFIG_KEY_INTERNAL(SMALL_CORE_OFFSET)
                       << ". Expected only non negative numbers";
        }
        _small_core_offset = val_i;
    } else if (key == CONFIG_KEY_INTERNAL(ENABLE_HYPER_THREAD)) {
        int val_i;
        if (value == CONFIG_VALUE_INTERNAL(YES)) {
            val_i = true;
        } else if (value == CONFIG_VALUE_INTERNAL(NO)) {
            val_i = false;
        } else {
            OPENVINO_UNREACHABLE("Unsupported enable hyper thread type");
        }
        _enable_hyper_thread = val_i;
    } else {
        IE_THROW() << "Wrong value for property key " << key;
    }
}

Parameter IStreamsExecutor::Config::GetConfig(const std::string& key) const {
    if (key == ov::affinity) {
        switch (_threadBindingType) {
        case IStreamsExecutor::ThreadBindingType::NONE:
            return ov::Affinity::NONE;
        case IStreamsExecutor::ThreadBindingType::CORES:
            return ov::Affinity::CORE;
        case IStreamsExecutor::ThreadBindingType::NUMA:
            return ov::Affinity::NUMA;
        case IStreamsExecutor::ThreadBindingType::HYBRID_AWARE:
            return ov::Affinity::HYBRID_AWARE;
        }
    } else if (key == CONFIG_KEY(CPU_BIND_THREAD)) {
        switch (_threadBindingType) {
        case IStreamsExecutor::ThreadBindingType::NONE:
            return {CONFIG_VALUE(NO)};
        case IStreamsExecutor::ThreadBindingType::CORES:
            return {CONFIG_VALUE(YES)};
        case IStreamsExecutor::ThreadBindingType::NUMA:
            return {CONFIG_VALUE(NUMA)};
        case IStreamsExecutor::ThreadBindingType::HYBRID_AWARE:
            return {CONFIG_VALUE(HYBRID_AWARE)};
        }
    } else if (key == CONFIG_KEY(CPU_THROUGHPUT_STREAMS)) {
        return {std::to_string(_streams)};
    } else if (key == ov::num_streams) {
        return decltype(ov::num_streams)::value_type{_streams};
    } else if (key == CONFIG_KEY(CPU_THREADS_NUM)) {
        return {std::to_string(_threads)};
    } else if (key == ov::inference_num_threads) {
        return decltype(ov::inference_num_threads)::value_type{_threads};
    } else if (key == CONFIG_KEY_INTERNAL(CPU_THREADS_PER_STREAM)) {
        return {std::to_string(_threadsPerStream)};
    } else if (key == CONFIG_KEY_INTERNAL(BIG_CORE_STREAMS)) {
        return {std::to_string(_big_core_streams)};
    } else if (key == CONFIG_KEY_INTERNAL(SMALL_CORE_STREAMS)) {
        return {std::to_string(_small_core_streams)};
    } else if (key == CONFIG_KEY_INTERNAL(THREADS_PER_STREAM_BIG)) {
        return {std::to_string(_threads_per_stream_big)};
    } else if (key == CONFIG_KEY_INTERNAL(THREADS_PER_STREAM_SMALL)) {
        return {std::to_string(_threads_per_stream_small)};
    } else if (key == CONFIG_KEY_INTERNAL(SMALL_CORE_OFFSET)) {
        return {std::to_string(_small_core_offset)};
    } else if (key == CONFIG_KEY_INTERNAL(ENABLE_HYPER_THREAD)) {
        return {_enable_hyper_thread ? CONFIG_VALUE_INTERNAL(YES) : CONFIG_VALUE_INTERNAL(NO)};
    } else {
        IE_THROW() << "Wrong value for property key " << key;
    }
    return {};
}

void IStreamsExecutor::Config::UpdateHybridCustomThreads(Config& config) {
    const auto num_cores = parallel_get_max_threads();
    const auto num_cores_phys = getNumberOfCPUCores();
    const auto num_big_cores_phys = getNumberOfCPUCores(true);
    const auto num_big_cores = num_cores > num_cores_phys ? num_big_cores_phys * 2 : num_big_cores_phys;
    const auto num_small_cores_phys = num_cores_phys - num_big_cores_phys;
    const auto threads = config._threads ? config._threads : num_cores;
    const auto streams = config._streams > 0 ? config._streams : 1;

    config._small_core_offset = num_big_cores;
    const int threads_per_stream = std::max(1, threads / streams);
    const int threads_per_stream_big = std::min(num_big_cores_phys, threads_per_stream);
    const int threads_per_stream_small = std::min(num_small_cores_phys, threads_per_stream);
    const int base_big_streams = num_cores > num_cores_phys
                                     ? (num_big_cores_phys + threads_per_stream_big - 1) / threads_per_stream_big * 2
                                     : (num_big_cores_phys + threads_per_stream_big - 1) / threads_per_stream_big;
    const int base_small_streams = num_small_cores_phys > 0 ? num_small_cores_phys / threads_per_stream_small : 0;
    const int base_streams = base_big_streams + base_small_streams;
    // big_streams = all_streams * base_big_streams / base_streams
    config._big_core_streams = (streams * base_big_streams + base_streams - 1) / base_streams;
    config._small_core_streams = config._streams - config._big_core_streams;
    // _big_core_streams > 2, num_big_cores_phys must be divisible by threads_per_stream_big
    config._threads_per_stream_big = (config._big_core_streams > 2 && num_big_cores_phys % threads_per_stream_big != 0)
                                         ? std::min(num_big_cores_phys, num_big_cores / base_big_streams)
                                         : threads_per_stream_big;
    config._threads_per_stream_small = config._small_core_streams > 0 ? threads_per_stream_small : 0;
}

IStreamsExecutor::Config IStreamsExecutor::Config::MakeDefaultMultiThreaded(const IStreamsExecutor::Config& initial,
                                                                            const bool fp_intesive) {
    const auto envThreads = parallel_get_env_threads();
    const auto& numaNodes = getAvailableNUMANodes();
    const int numaNodesNum = static_cast<int>(numaNodes.size());
    auto streamExecutorConfig = initial;
    const bool bLatencyCase = streamExecutorConfig._streams <= numaNodesNum;

    // by default, do not use the hyper-threading (to minimize threads synch overheads)
    int num_cores_default = getNumberOfCPUCores();
#if (IE_THREAD == IE_THREAD_TBB || IE_THREAD == IE_THREAD_TBB_AUTO)
    // additional latency-case logic for hybrid processors:
    if (ThreadBindingType::HYBRID_AWARE == streamExecutorConfig._threadBindingType) {
        const auto core_types = custom::info::core_types();
        const auto num_little_cores =
            custom::info::default_concurrency(custom::task_arena::constraints{}.set_core_type(core_types.front()));
        const auto num_big_cores_phys = getNumberOfCPUCores(true);
        const int int8_threshold = 4;  // ~relative efficiency of the VNNI-intensive code for Big vs Little cores;
        const int fp32_threshold = 2;  // ~relative efficiency of the AVX2 fp32 code for Big vs Little cores;
        // by default the latency case uses (faster) Big cores only, depending on the compute ratio
        const bool bLatencyCaseBigOnly =
            num_big_cores_phys > (num_little_cores / (fp_intesive ? fp32_threshold : int8_threshold));
        // selecting the preferred core type
        streamExecutorConfig._threadPreferredCoreType =
            bLatencyCase ? (bLatencyCaseBigOnly ? IStreamsExecutor::Config::PreferredCoreType::BIG
                                                : IStreamsExecutor::Config::PreferredCoreType::ANY)
                         : IStreamsExecutor::Config::PreferredCoreType::ROUND_ROBIN;
        // additionally selecting the #cores to use in the "Big-only" case
        if (bLatencyCaseBigOnly) {
            const int hyper_threading_threshold =
                2;  // min #cores, for which the hyper-threading becomes useful for the latency case
            const auto num_big_cores =
                custom::info::default_concurrency(custom::task_arena::constraints{}.set_core_type(core_types.back()));
            num_cores_default = (num_big_cores_phys <= hyper_threading_threshold) ? num_big_cores : num_big_cores_phys;
        }
        // if nstreams or nthreads are set, need to calculate the Hybrid aware parameters here
        if (streamExecutorConfig._big_core_streams == 0 || streamExecutorConfig._threads) {
            UpdateHybridCustomThreads(streamExecutorConfig);
        }
        OPENVINO_DEBUG << "[ p_e_core_info ] streams (threads): " << streamExecutorConfig._streams << "("
                       << streamExecutorConfig._threads_per_stream_big * streamExecutorConfig._big_core_streams +
                              streamExecutorConfig._threads_per_stream_small * streamExecutorConfig._small_core_streams
                       << ") -- PCore: " << streamExecutorConfig._big_core_streams << "("
                       << streamExecutorConfig._threads_per_stream_big
                       << ")  ECore: " << streamExecutorConfig._small_core_streams << "("
                       << streamExecutorConfig._threads_per_stream_small << ")";
    }
#endif
    const auto hwCores =
        !bLatencyCase && numaNodesNum == 1
            // throughput case on a single-NUMA node machine uses all available cores
            ? (!streamExecutorConfig._enable_hyper_thread ? num_cores_default : parallel_get_max_threads())
            // in the rest of cases:
            //    multi-node machine
            //    or
            //    latency case, single-node yet hybrid case that uses
            //      all core types
            //      or
            //      big-cores only, but the #cores is "enough" (pls see the logic above)
            // it is usually beneficial not to use the hyper-threading (which is default)
            : num_cores_default;
    const auto threads =
        streamExecutorConfig._threads ? streamExecutorConfig._threads : (envThreads ? envThreads : hwCores);
    streamExecutorConfig._threadsPerStream =
        streamExecutorConfig._streams ? std::max(1, threads / streamExecutorConfig._streams) : threads;
    streamExecutorConfig._threads =
        ThreadBindingType::HYBRID_AWARE == streamExecutorConfig._threadBindingType
            ? streamExecutorConfig._big_core_streams * streamExecutorConfig._threads_per_stream_big +
                  streamExecutorConfig._small_core_streams * streamExecutorConfig._threads_per_stream_small
            : streamExecutorConfig._threadsPerStream * streamExecutorConfig._streams;
    return streamExecutorConfig;
}

}  //  namespace InferenceEngine
