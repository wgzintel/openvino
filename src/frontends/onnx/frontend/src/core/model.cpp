// Copyright (C) 2018-2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#include "core/model.hpp"

#include <onnx/onnx_pb.h>

#include "ngraph/log.hpp"
#include "onnx_framework_node.hpp"
#include "ops_bridge.hpp"

namespace ngraph {
namespace onnx_import {
std::string get_node_domain(const ONNX_NAMESPACE::NodeProto& node_proto) {
    return node_proto.has_domain() ? node_proto.domain() : "";
}

std::int64_t get_opset_version(const ONNX_NAMESPACE::ModelProto& model_proto, const std::string& domain) {
    // copy the opsets and sort them (descending order)
    // then return the version from the first occurence of a given domain
    auto opset_imports = model_proto.opset_import();
    std::sort(std::begin(opset_imports),
              std::end(opset_imports),
              [](const ONNX_NAMESPACE::OperatorSetIdProto& lhs, const ONNX_NAMESPACE::OperatorSetIdProto& rhs) {
                  return lhs.version() > rhs.version();
              });

    for (const auto& opset_import : opset_imports) {
        if (domain == opset_import.domain()) {
            return opset_import.version();
        }
    }

    throw ov::Exception("Couldn't find operator set's version for domain: " + domain + ".");
}

Model::Model(std::shared_ptr<ONNX_NAMESPACE::ModelProto> model_proto, ModelOpSet&& model_opset)
    : m_model_proto{std::move(model_proto)},
      m_opset{std::move(model_opset)} {}

const Operator& Model::get_operator(const std::string& name, const std::string& domain) const {
    const auto dm = m_opset.find(domain);
    if (dm == std::end(m_opset)) {
        throw error::UnknownDomain{domain};
    }
    const auto op = dm->second.find(name);
    if (op == std::end(dm->second)) {
        throw error::UnknownOperator{name, domain};
    }
    return op->second;
}

bool Model::is_operator_available(const ONNX_NAMESPACE::NodeProto& node_proto) const {
    const auto dm = m_opset.find(get_node_domain(node_proto));
    if (dm == std::end(m_opset)) {
        return false;
    }
    const auto op = dm->second.find(node_proto.op_type());
    return (op != std::end(dm->second));
}

void Model::enable_opset_domain(const std::string& domain, const OperatorsBridge& ops_bridge) {
    // There is no need to 'update' already enabled domain.
    // Since this function may be called only during model import,
    // (maybe multiple times) the registered domain opset won't differ
    // between subsequent calls.
    if (m_opset.find(domain) == std::end(m_opset)) {
        const auto opset = ops_bridge.get_operator_set(domain);
        if (opset.empty()) {
            NGRAPH_WARN << "Couldn't enable domain: " << domain << " since it does not have any registered operators.";
            return;
        }
        m_opset.emplace(domain, opset);
    }
}

}  // namespace onnx_import

}  // namespace ngraph
