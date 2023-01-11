// Copyright (C) 2018-2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#include "openvino/op/util/framework_node.hpp"

#include "common_test_utils/graph_comparator.hpp"
#include "gtest/gtest.h"
#include "openvino/op/util/attr_types.hpp"
#include "openvino/opsets/opset10.hpp"
#include "util/visitor.hpp"

using namespace std;
using namespace ov;
using namespace ov::opset10;
using ngraph::test::NodeBuilder;
using ngraph::test::ValueMap;

TEST(attributes, framework_node_op) {
    NodeBuilder::get_ops().register_factory<op::util::FrameworkNode>();
    auto X = make_shared<Parameter>(element::f32, Shape{1, 2, 2});
    auto Y = make_shared<Parameter>(element::f32, Shape{1, 2, 2});
    auto cond = make_shared<Constant>(element::boolean, Shape{1}, true);
    auto cond2 = make_shared<Constant>(element::boolean, Shape{1}, false);
    auto Xt = make_shared<Parameter>(element::f32, PartialShape::dynamic());
    auto Yt = make_shared<Parameter>(element::f32, PartialShape::dynamic());
    auto Xe = make_shared<Parameter>(element::f32, PartialShape::dynamic());
    auto Ye = make_shared<Parameter>(element::f32, PartialShape::dynamic());
    auto then_op = make_shared<Multiply>(Xt, Yt);
    auto res0 = make_shared<Result>(then_op);
    auto res1 = make_shared<Result>(Xe);
    auto body1 = make_shared<Model>(OutputVector{res0}, ParameterVector{Xt, Yt});
    auto body2 = make_shared<Model>(OutputVector{res1}, ParameterVector{Xe});
    auto fn_op = make_shared<op::util::FrameworkNode>(OutputVector{cond}, 0, 2);

    // Add attributes
    auto attrs = op::util::FrameworkNodeAttrs();
    attrs.set_type_name("some_type");
    fn_op->set_attrs(attrs);

    fn_op->set_function(0, body1);
    fn_op->set_function(1, body2);
    fn_op->set_invariant_inputs(X, {Xt, Xe});
    fn_op->set_invariant_inputs(Y, {Yt, nullptr});
    auto out = fn_op->set_body_outputs({res0, res1});
    fn_op->validate_and_infer_types();
    EXPECT_EQ(fn_op->inputs().size(), 3);
    EXPECT_EQ(fn_op->outputs().size(), 1);

    NodeBuilder builder(fn_op);
    auto g_fn = ov::as_type_ptr<op::util::FrameworkNode>(builder.create());
    EXPECT_EQ(g_fn->get_attrs(), fn_op->get_attrs());
    EXPECT_EQ(g_fn->get_input_descriptions(0), fn_op->get_input_descriptions(0));
    EXPECT_EQ(g_fn->get_input_descriptions(1), fn_op->get_input_descriptions(1));
    EXPECT_EQ(g_fn->get_output_descriptions(0), fn_op->get_output_descriptions(0));
    EXPECT_EQ(g_fn->get_output_descriptions(1), fn_op->get_output_descriptions(1));

    auto comparator = FunctionsComparator::with_default();
    ASSERT_TRUE(g_fn->get_function(0));
    auto res = comparator.compare(g_fn->get_function(0), fn_op->get_function(0));
    EXPECT_TRUE(res.valid) << res.message;

    ASSERT_TRUE(g_fn->get_function(1));
    res = comparator.compare(g_fn->get_function(1), fn_op->get_function(1));
    EXPECT_TRUE(res.valid) << res.message;
}
