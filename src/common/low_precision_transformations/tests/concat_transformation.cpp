// Copyright (C) 2018-2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#include <gtest/gtest.h>

#include <low_precision/concat.hpp>
#include <memory>
#include <sstream>
#include <vector>

#include "common_test_utils/ngraph_test_utils.hpp"
#include "layer_transformation.hpp"
#include "lpt_ngraph_functions/common/builders.hpp"
#include "lpt_ngraph_functions/concat_function.hpp"
#include "simple_low_precision_transformer.hpp"

using namespace testing;
using namespace ov;
using namespace ov::pass;
using namespace ngraph::builder::subgraph;

namespace {
class ConcatTransformationTestValues {
public:
public:
    class Actual {
    public:
        ov::element::Type inputPrecision;
        std::vector<DequantizationOperations> dequantizations;
    };

    class Expected {
    public:
        ov::element::Type precisionBefore;
        std::vector<DequantizationOperations> dequantizationsBefore;
        ov::element::Type precisionAfter;
        DequantizationOperations dequantizationAfter;
    };

    std::vector<ov::PartialShape> inputShapes;
    std::int64_t concatAxis;
    TestTransformationParams params;
    Actual actual;
    Expected expected;
};

typedef std::tuple<ov::element::Type, ConcatTransformationTestValues> ConcatTransformationParams;

class ConcatTransformation : public LayerTransformation,
                             public testing::WithParamInterface<ConcatTransformationParams> {
public:
    void SetUp() override {
        const ov::element::Type deqOutPrecision = std::get<0>(GetParam());
        ConcatTransformationTestValues testValues = std::get<1>(GetParam());

        actualFunction = ConcatFunction::get(testValues.actual.inputPrecision,
                                             deqOutPrecision,
                                             testValues.inputShapes,
                                             testValues.actual.dequantizations,
                                             testValues.concatAxis);

        SimpleLowPrecisionTransformer transformer;
        transformer.add<ngraph::pass::low_precision::ConcatTransformation, ngraph::opset1::Concat>(testValues.params);
        transformer.transform(actualFunction);

        referenceFunction = ConcatFunction::get(testValues.expected.precisionBefore,
                                                deqOutPrecision,
                                                testValues.inputShapes,
                                                testValues.expected.dequantizationsBefore,
                                                testValues.concatAxis,
                                                testValues.expected.precisionAfter,
                                                testValues.expected.dequantizationAfter);
    }

    static std::string getTestCaseName(testing::TestParamInfo<ConcatTransformationParams> obj) {
        ov::element::Type deqOutPrecision = std::get<0>(obj.param);
        ConcatTransformationTestValues testValues = std::get<1>(obj.param);

        std::ostringstream result;
        result << deqOutPrecision << "_" << toString(testValues.params) << "_iShapes_" << testValues.inputShapes
               << "_actual_" << testValues.actual.inputPrecision << "_" << testValues.actual.dequantizations
               << "_expected_" << testValues.expected.precisionBefore << testValues.expected.dequantizationsBefore
               << testValues.expected.precisionAfter << testValues.expected.dequantizationAfter << "_axis_"
               << testValues.concatAxis;
        return result.str();
    }
};

TEST_P(ConcatTransformation, CompareFunctions) {
    actualFunction->validate_nodes_and_infer_types();
    auto res = compare_functions(actualFunction, referenceFunction, true, true, true, true);
    ASSERT_TRUE(res.first) << res.second;
    ASSERT_TRUE(LayerTransformation::allNamesAreUnique(actualFunction)) << "Not all names are unique";
}

const ov::element::TypeVector deqOutPrecisions = {ov::element::f32, ov::element::f16};
const std::vector<ConcatTransformationTestValues> testValues = {
    // the same per-tensor values
    {{{1, 3, 4, 4}, {1, 3, 4, 4}},
     std::int64_t{1},
     LayerTransformation::createParamsU8I8(),
     {ov::element::u8, {{ov::element::f32, {128.f}, {0.1f}}, {ov::element::f32, {128.f}, {0.1f}}}},
     {ov::element::u8, {{}, {}}, ov::element::u8, {ov::element::f32, {128.f}, {0.1f}}}},
    {{{1, 3, 4, 4}, {1, 3, 4, 4}},
     std::int64_t{1},
     LayerTransformation::createParamsU8I8(),
     {ov::element::u8, {{ov::element::f32, {127.f}, {0.1f}}, {ov::element::f32, {128.f}, {0.2f}}}},
     {ov::element::u8,
      {{}, {}},
      ov::element::u8,
      {ov::element::f32, {{127.f, 127.f, 127.f, 128.f, 128.f, 128.f}}, {{0.1f, 0.1f, 0.1f, 0.2f, 0.2f, 0.2f}}}}},
    // 1D concat, 3 inputs
    {{{2}, {1}, {3}},
     std::int64_t{0},
     LayerTransformation::createParamsU8I8(),
     {ov::element::u8,
      {
          {ov::element::f32, {126.f}, {0.1f}},
          {ov::element::f32, {127.f}, {0.2f}},
          {ov::element::f32, {128.f}, {0.3f}},
      }},
     {ov::element::u8,
      {{}, {}, {}},
      ov::element::u8,
      {ov::element::f32, {{126.f, 126.f, 127.f, 128.f, 128.f, 128.f}}, {{0.1f, 0.1f, 0.2f, 0.3f, 0.3f, 0.3f}}}}},
    // per-channel by axis = 2
    {{{1, 3, 4, 4}, {1, 3, 4, 4}},
     std::int64_t{2},
     LayerTransformation::createParamsU8I8(),
     {ov::element::u8,
      {{ov::element::f32, {}, {{0.1f, 0.2f, 0.3f, 0.4f}, ngraph::element::f32, {1, 1, 4, 1}}},
       {ov::element::f32, {}, {{0.1f, 0.2f, 0.3f, 0.4f}, ngraph::element::f32, {1, 1, 4, 1}}}}},
     {ov::element::u8,
      {{}, {}},
      ov::element::u8,
      {ov::element::f32, {}, {{0.1f, 0.2f, 0.3f, 0.4f, 0.1f, 0.2f, 0.3f, 0.4f}, ngraph::element::f32, {1, 1, 8, 1}}}}},
    // 2D concat, 4 inputs
    {{{2, 4}, {1, 4}, {1, 4}, {1, 4}},
     std::int64_t{0},
     LayerTransformation::createParamsU8I8(),
     {ov::element::u8,
      {
          {ov::element::f32,
           {{124.f, 125.f}, ngraph::element::f32, {2, 1}},
           {{0.1f, 0.2f}, ngraph::element::f32, {2, 1}}},
          {ov::element::f32, {126.f}, {0.3f}},
          {ov::element::f32, {127.f}, {0.4f}},
          {ov::element::f32, {128.f}, {0.5f}},
      }},
     {ov::element::u8,
      {{}, {}, {}, {}},
      ov::element::u8,
      {ov::element::f32,
       {{124.f, 125.f, 126.f, 127.f, 128.f}, ngraph::element::f32, {5, 1}},
       {{0.1f, 0.2f, 0.3f, 0.4f, 0.5f}, ngraph::element::f32, {5, 1}}}}},
    // Dequantization not by concat axis
    {{{1, 3, 4, 4}, {1, 3, 4, 4}},
     std::int64_t{0},
     LayerTransformation::createParamsU8I8(),
     {ov::element::u8,
      {{ov::element::f32, {}, {{0.1f}, ngraph::element::f32, {1, 3, 1, 1}}},
       {ov::element::f32, {}, {{0.2f}, ngraph::element::f32, {1, 3, 1, 1}}}}},
     {ov::element::u8,
      {{ov::element::f32, {}, {{0.1f}, ngraph::element::f32, {1, 3, 1, 1}}},
       {ov::element::f32, {}, {{0.2f}, ngraph::element::f32, {1, 3, 1, 1}}}},
      ov::element::f32,
      {}}},
};

INSTANTIATE_TEST_SUITE_P(smoke_LPT,
                         ConcatTransformation,
                         ::testing::Combine(::testing::ValuesIn(deqOutPrecisions), ::testing::ValuesIn(testValues)),
                         ConcatTransformation::getTestCaseName);
}  // namespace
