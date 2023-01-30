// Copyright (C) 2018-2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#include <string>
#include <sstream>
#include <vector>

#include <openvino/core/partial_shape.hpp>
#include "ngraph_functions/builders.hpp"
#include "shared_test_classes/base/layer_test_utils.hpp"
#include "shared_test_classes/base/ov_subgraph.hpp"
#include "test_utils/cpu_test_utils.hpp"

using namespace InferenceEngine;
using namespace CPUTestUtils;
using namespace ov::test;

namespace CPULayerTestsDefinitions {

typedef std::tuple<
    std::vector<float>,  // widths
    std::vector<float>,  // heights
    bool,                // clip
    float,               // step_width
    float,               // step_height
    float,               // step
    float,               // offset
    std::vector<float>> priorBoxClusteredSpecificParams;

typedef std::tuple<
    priorBoxClusteredSpecificParams,
    ov::test::ElementType,        // net precision
    ov::test::ElementType,        // Input precision
    ov::test::ElementType,        // Output precision
    InferenceEngine::Layout,      // Input layout
    InferenceEngine::Layout,      // Output layout
    ov::test::InputShape,         // input shape
    ov::test::InputShape,         // image shape
    std::string> priorBoxClusteredLayerParams;

class PriorBoxClusteredLayerCPUTest : public testing::WithParamInterface<priorBoxClusteredLayerParams>,
        virtual public SubgraphBaseTest, public CPUTestsBase {
public:
    static std::string getTestCaseName(const testing::TestParamInfo<priorBoxClusteredLayerParams>& obj) {
        ov::test::ElementType netPrecision;
        ov::test::ElementType inPrc, outPrc;
        InferenceEngine::Layout inLayout, outLayout;
        ov::test::InputShape inputShapes, imageShapes;
        std::string targetDevice;
        priorBoxClusteredSpecificParams specParams;
        std::tie(specParams,
                 netPrecision,
                 inPrc, outPrc, inLayout, outLayout,
                 inputShapes,
                 imageShapes,
                 targetDevice) = obj.param;

        ngraph::op::PriorBoxClusteredAttrs attributes;
        std::tie(
            attributes.widths,
            attributes.heights,
            attributes.clip,
            attributes.step_widths,
            attributes.step_heights,
            attributes.step,
            attributes.offset,
            attributes.variances) = specParams;

        std::ostringstream result;
        const char separator = '_';

        result << "IS="      << inputShapes << separator;
        result << "imageS="  << imageShapes << separator;
        result << "netPRC="  << netPrecision << separator;
        result << "inPRC="   << inPrc << separator;
        result << "outPRC="  << outPrc << separator;
        result << "inL="     << inLayout << separator;
        result << "outL="    << outLayout << separator;
        result << "widths="  << CommonTestUtils::vec2str(attributes.widths)  << separator;
        result << "heights=" << CommonTestUtils::vec2str(attributes.heights) << separator;
        result << "variances=";
        if (attributes.variances.empty())
            result << "()" << separator;
        else
            result << CommonTestUtils::vec2str(attributes.variances) << separator;
        result << "stepWidth="  << attributes.step_widths  << separator;
        result << "stepHeight=" << attributes.step_heights << separator;
        result << "step="       << attributes.step << separator;
        result << "offset="     << attributes.offset << separator;
        result << "clip="       << std::boolalpha << attributes.clip << separator;
        result << "trgDev="     << targetDevice;
        return result.str();
    }

protected:
    void SetUp() override {
        priorBoxClusteredSpecificParams specParams;

        InferenceEngine::Layout inLayout;
        InferenceEngine::Layout outLayout;
        ov::test::ElementType netPrecision;
        ov::test::ElementType inPrc;
        ov::test::ElementType outPrc;
        ov::test::InputShape inputShapes;
        ov::test::InputShape imageShapes;
        std::tie(specParams, netPrecision,
                 inPrc, outPrc, inLayout, outLayout,
                 inputShapes, imageShapes, targetDevice) = GetParam();

        selectedType = makeSelectedTypeStr("ref", inPrc);
        targetDevice = CommonTestUtils::DEVICE_CPU;

        init_input_shapes({ inputShapes, imageShapes });

        ngraph::op::PriorBoxClusteredAttrs attributes;
        std::tie(
            attributes.widths,
            attributes.heights,
            attributes.clip,
            attributes.step_widths,
            attributes.step_heights,
            attributes.step,
            attributes.offset,
            attributes.variances) = specParams;

        auto params = ngraph::builder::makeDynamicParams(netPrecision, { inputShapes.first, imageShapes.first });

        auto shape_of_1 = std::make_shared<ngraph::opset3::ShapeOf>(params[0]);
        auto shape_of_2 = std::make_shared<ngraph::opset3::ShapeOf>(params[1]);
        auto priorBoxClustered = std::make_shared<ngraph::op::PriorBoxClustered>(
                shape_of_1,
                shape_of_2,
                attributes);

        ngraph::ResultVector results{ std::make_shared<ngraph::opset1::Result>(priorBoxClustered) };
        function = std::make_shared<ngraph::Function>(results, params, "priorBoxClustered");
    }
};

TEST_P(PriorBoxClusteredLayerCPUTest, CompareWithRefs) {
    run();
    CheckPluginRelatedResults(compiledModel, "PriorBoxClustered");
}

namespace {
// Common params
const std::vector<ov::test::ElementType> netPrecisions = {
        ov::test::ElementType::f32,
        ov::test::ElementType::f16
};

const std::vector<std::vector<float>> widths = {
    { 5.12f, 14.6f, 13.5f },
    { 7.0f, 8.2f, 33.39f }
};

const std::vector<std::vector<float>> heights = {
    { 15.12f, 15.6f, 23.5f },
    { 10.0f, 16.2f, 36.2f }
};

const std::vector<float> step_widths = {
    0.0f, 2.0f
};

const std::vector<float> step_heights = {
    0.0f, 1.5f
};

const std::vector<float> step = {
    0.0f, 1.0f, 1.5f
};

const std::vector<float> offsets = {
    0.5f
};

const std::vector<std::vector<float>> variances = {
    {0.1f, 0.1f, 0.2f, 0.2f},
    {0.2f},
    {}
};

const std::vector<bool> clips = {
    true, false
};

const auto layerSpeficParams = ::testing::Combine(
    ::testing::ValuesIn(widths),
    ::testing::ValuesIn(heights),
    ::testing::ValuesIn(clips),
    ::testing::ValuesIn(step_widths),
    ::testing::ValuesIn(step_heights),
    ::testing::ValuesIn(step),
    ::testing::ValuesIn(offsets),
    ::testing::ValuesIn(variances)
);

const std::vector<ov::test::InputShape> inputShapes = {
        {{4, 4}, {{4, 4}}},
        {{ov::Dimension::dynamic(), ov::Dimension::dynamic()}, {{4, 4}, {8, 8}}},
        {{{4, 8}, {4, 8}}, {{4, 4}, {8, 8}}}
};

const std::vector<ov::test::InputShape> imageShapes = {
        {{50, 50}, {{50, 50}}},
        {{ov::Dimension::dynamic(), ov::Dimension::dynamic()}, {{50, 50}, {100, 100}}},
        {{{50, 100}, {50, 100}}, {{50, 50}, {100, 100}}}
};

INSTANTIATE_TEST_SUITE_P(smoke_PriorBoxClustered, PriorBoxClusteredLayerCPUTest,
    ::testing::Combine(
        layerSpeficParams,
        ::testing::ValuesIn(netPrecisions),
        ::testing::Values(ov::test::ElementType::undefined),
        ::testing::Values(ov::test::ElementType::undefined),
        ::testing::Values(InferenceEngine::Layout::ANY),
        ::testing::Values(InferenceEngine::Layout::ANY),
        ::testing::ValuesIn(inputShapes),
        ::testing::ValuesIn(imageShapes),
        ::testing::Values(CommonTestUtils::DEVICE_CPU)),
    PriorBoxClusteredLayerCPUTest::getTestCaseName
);

}  // namespace
}  // namespace CPULayerTestsDefinitions
