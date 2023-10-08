// Copyright (C) 2018-2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#include <thread>

#include "auto_func_test.hpp"
#include "openvino/runtime/exception.hpp"

using namespace ov::auto_plugin::tests;

TEST_F(AutoFuncTests, can_infer_and_wait_for_result) {
    ov::CompiledModel compiled_model;
    ASSERT_NO_THROW(compiled_model = core.compile_model(
                        model_can_batch,
                        "AUTO",
                        {ov::device::priorities("MOCK_GPU", "MOCK_CPU"),
                         ov::hint::performance_mode(ov::hint::PerformanceMode::CUMULATIVE_THROUGHPUT)}));
    auto req = compiled_model.create_infer_request();
    ov::Tensor tensor;
    auto input = compiled_model.input();
    auto output = compiled_model.output();
    ASSERT_NO_THROW(tensor = req.get_tensor(input));
    ASSERT_NO_THROW(req.infer());
    ASSERT_NO_THROW(req.start_async());
    ASSERT_NO_THROW(req.wait());
    ASSERT_NO_THROW(tensor = req.get_tensor(output));
}

TEST_F(AutoFuncTests, can_wait_without_startasync) {
    ov::CompiledModel compiled_model;
    ASSERT_NO_THROW(compiled_model = core.compile_model(
                        model_can_batch,
                        "AUTO",
                        {ov::device::priorities("MOCK_GPU", "MOCK_CPU"),
                         ov::hint::performance_mode(ov::hint::PerformanceMode::CUMULATIVE_THROUGHPUT)}));
    auto req = compiled_model.create_infer_request();
    ASSERT_NO_THROW(req.wait());
    ASSERT_NO_THROW(req.wait_for({}));
    ASSERT_NO_THROW(req.wait_for(std::chrono::milliseconds{1}));
}

TEST_F(AutoFuncTests, can_throw_if_request_busy) {
    ov::CompiledModel compiled_model;
    ASSERT_NO_THROW(compiled_model = core.compile_model(
                        model_can_batch,
                        "AUTO",
                        {ov::device::priorities("MOCK_GPU", "MOCK_CPU"),
                         ov::hint::performance_mode(ov::hint::PerformanceMode::CUMULATIVE_THROUGHPUT)}));
    auto req = compiled_model.create_infer_request();
    auto input = compiled_model.input();
    auto output = compiled_model.output();
    auto output_tensor = req.get_tensor(input);
    ASSERT_NO_THROW(req.wait_for({}));
    ASSERT_NO_THROW(req.start_async());
    ASSERT_NO_THROW(try { req.set_tensor(input, output_tensor); } catch (const ov::Busy&){});
    ASSERT_NO_THROW(req.wait_for({}));
    ASSERT_NO_THROW(req.wait());
}

TEST_F(AutoFuncTests, can_throw_on_get_tensor_if_request_busy) {
    ov::CompiledModel compiled_model;
    ASSERT_NO_THROW(compiled_model = core.compile_model(
                        model_can_batch,
                        "AUTO",
                        {ov::device::priorities("MOCK_GPU", "MOCK_CPU"),
                         ov::hint::performance_mode(ov::hint::PerformanceMode::CUMULATIVE_THROUGHPUT)}));
    auto req = compiled_model.create_infer_request();
    auto input = compiled_model.input();
    ASSERT_NO_THROW(req.start_async());
    ASSERT_NO_THROW(try { req.get_tensor(input); } catch (const ov::Busy&){});
    ASSERT_NO_THROW(req.wait());
}