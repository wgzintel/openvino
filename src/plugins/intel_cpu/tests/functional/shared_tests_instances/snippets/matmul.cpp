// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#include "snippets/matmul.hpp"
#include "common_test_utils/test_constants.hpp"
#include "ie_system_conf.h"

namespace ov {
namespace test {
namespace snippets {


namespace {
std::vector<std::vector<ov::PartialShape>> input_shapes{
        {{2, 1, 3, 5}, {1, 3, 5, 3}},
        {{3, 1, 32, 14}, {1, 2, 14, 32}},
        {{1, 2, 37, 23}, {2, 1, 23, 37}},
        {{1, 1, 37, 23}, {1, 2, 23, 33}},
        {{1, 16, 384, 64}, {1, 16, 64, 384}}
};
static inline std::vector<std::vector<element::Type>> precisions(bool only_fp32 = true) {
    std::vector<std::vector<element::Type>> prc = {
            {element::f32, element::f32},
    };
    if (!only_fp32) {
        // In Snippets MatMul INT8 is supported only on VNNI/AMX platforms
        if (InferenceEngine::with_cpu_x86_avx512_core_vnni() || InferenceEngine::with_cpu_x86_avx512_core_amx_int8()) {
            prc.emplace_back(std::vector<element::Type>{element::i8, element::i8});
            prc.emplace_back(std::vector<element::Type>{element::u8, element::i8});
        }
        // In Snippets MatMul BF16 is supported only on bf16/AMX platforms
        if (InferenceEngine::with_cpu_x86_bfloat16() || InferenceEngine::with_cpu_x86_avx512_core_amx_bf16()) {
            prc.emplace_back(std::vector<element::Type>{element::bf16, element::bf16});
        }
    }
    return prc;
}
INSTANTIATE_TEST_SUITE_P(smoke_Snippets_MatMult, MatMul,
                         ::testing::Combine(
                             ::testing::ValuesIn(input_shapes),
                             ::testing::ValuesIn(precisions(false)),
                             ::testing::Values(1), // MatMul
                             ::testing::Values(1), // Tokenized MatMul
                             ::testing::Values(CommonTestUtils::DEVICE_CPU)),
                         MatMul::getTestCaseName);

INSTANTIATE_TEST_SUITE_P(smoke_Snippets_MatMulFQ, MatMulFQ,
                         ::testing::Combine(
                                 ::testing::ValuesIn(input_shapes),
                                 ::testing::ValuesIn(precisions()),
                                 ::testing::Values(1), // MatMul;
                                 ::testing::Values(1), // Tokenized MatMul
                                 ::testing::Values(CommonTestUtils::DEVICE_CPU)),
                         MatMul::getTestCaseName);

INSTANTIATE_TEST_SUITE_P(smoke_Snippets_MatMulBias, MatMulBias,
                         ::testing::Combine(
                                 ::testing::Values(std::vector<ov::PartialShape>{{1, 2, 69, 43}, {2, 1, 43, 49}, {1, 1, 69, 49}}),
                                 ::testing::ValuesIn(precisions(false)),
                                 ::testing::Values(1), // Subgraph;
                                 ::testing::Values(1), // Tokenized MatMul+Bias
                                 ::testing::Values(CommonTestUtils::DEVICE_CPU)),
                         MatMul::getTestCaseName);

}  // namespace
} // namespace snippets
} // namespace test
} // namespace ov