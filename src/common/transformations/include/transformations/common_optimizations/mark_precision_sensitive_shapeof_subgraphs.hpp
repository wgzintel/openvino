// Copyright (C) 2018-2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "openvino/pass/pass.hpp"
#include "transformations_visibility.hpp"

namespace ov {
namespace pass {

class TRANSFORMATIONS_API MarkPrecisionSensitiveShapeOfSubgraphs;
class TRANSFORMATIONS_API MarkPrecisionSensitiveConstants;
class TRANSFORMATIONS_API MarkDividesInShapeSubgraphs;

}  // namespace pass
}  // namespace ov

/**
 * @ingroup ie_transformation_common_api
 * @brief MarkPrecisionSensitiveShapeOfSubgraphs marks entirely
 * all shape subgraphs starting from precision-sensitive inputs and ending at
 * the ShapeOf node as disabled for FP16 compression.
 */
class ov::pass::MarkPrecisionSensitiveShapeOfSubgraphs : public ModelPass {
public:
    OPENVINO_RTTI("MarkPrecisionSensitiveShapeOfSubgraphs", "0");
    MarkPrecisionSensitiveShapeOfSubgraphs();
    bool run_on_model(const std::shared_ptr<ov::Model>& f) override;

protected:
    std::function<void(ov::Node*)> m_markup_func;
};

/**
 * @ingroup ie_transformation_common_api
 * @brief MarkPrecisionSensitiveConstants marks the constants
 * inside of all shape subgraphs starting from precision-sensitive inputs and ending at
 * the ShapeOf node as disabled for FP16 compression.
 */
class ov::pass::MarkPrecisionSensitiveConstants : public MarkPrecisionSensitiveShapeOfSubgraphs {
public:
    OPENVINO_RTTI("MarkPrecisionSensitiveConstants", "0");
    MarkPrecisionSensitiveConstants();
};

/**
 * @ingroup ie_transformation_common_api
 * @brief MarkDividesInShapeSubgraphs marks the Divide layers
 * inside of all shape subgraphs starting from precision-sensitive input and ending at
 * the ShapeOf node as disabled for ConvertDivide transformation.
 */
class ov::pass::MarkDividesInShapeSubgraphs : public MarkPrecisionSensitiveShapeOfSubgraphs {
public:
    OPENVINO_RTTI("MarkDividesInShapeSubgraphs", "0");
    MarkDividesInShapeSubgraphs();
};
