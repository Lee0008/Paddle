/* Copyright (c) 2016 PaddlePaddle Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License. */

#include "paddle/fluid/framework/op_registry.h"

namespace paddle {
namespace operators {

class AccuracyOp : public framework::OperatorWithKernel {
 public:
  using framework::OperatorWithKernel::OperatorWithKernel;

  void InferShape(framework::InferShapeContext *ctx) const override {
    PADDLE_ENFORCE_EQ(
        ctx->HasInput("Out"), true,
        platform::errors::NotFound("Input (Out) of AccuracyOp is not found."));
    PADDLE_ENFORCE_EQ(ctx->HasInput("Indices"), true,
                      platform::errors::NotFound(
                          "Input (Indices) of AccuracyOp is not found."));
    PADDLE_ENFORCE_EQ(ctx->HasInput("Label"), true,
                      platform::errors::NotFound(
                          "Input (Label) of AccuracyOp is not found."));
    PADDLE_ENFORCE_EQ(ctx->HasOutput("Accuracy"), true,
                      platform::errors::NotFound(
                          "Output (Accuracy) of AccuracyOp is not found."));
    PADDLE_ENFORCE_EQ(ctx->HasOutput("Correct"), true,
                      platform::errors::NotFound(
                          "Output (Correct) of AccuracyOp is not found."));
    PADDLE_ENFORCE_EQ(ctx->HasOutput("Total"), true,
                      platform::errors::NotFound(
                          "Output (Total) of AccuracyOp is not found."));

    OP_INOUT_CHECK(ctx->HasInput("Out"), "Input", "Out", "Accuracy");
    OP_INOUT_CHECK(ctx->HasInput("Indices"), "Input", "Indices", "Accuracy");
    OP_INOUT_CHECK(ctx->HasInput("Label"), "Input", "Label", "Accuracy");
    OP_INOUT_CHECK(ctx->HasOutput("Accuracy"), "Output", "Accuracy",
                   "Accuracy");
    OP_INOUT_CHECK(ctx->HasOutput("Correct"), "Output", "Correct", "Accuracy");
    OP_INOUT_CHECK(ctx->HasOutput("Total"), "Output", "Total", "Accuracy");

    auto inference_dim = ctx->GetInputDim("Out");
    auto label_dim = ctx->GetInputDim("Label");
    // Assume indices has same shape as inference, because
    // it's the output of topk.

    PADDLE_ENFORCE_EQ(
        label_dim.size(), 2,
        platform::errors::InvalidArgument(
            "ShapeError: label's dimensions of AccuracyOp must be 2. "
            "But received label's dimensions = %d, label's shape = [%s]",
            label_dim.size(), label_dim));
    if (ctx->IsRuntime()) {
      PADDLE_ENFORCE_EQ(label_dim[1], 1,
                        platform::errors::InvalidArgument(
                            "ShapeError: label's second dimension of "
                            "AccuracyOp must be 1. But received label's "
                            "second dimension is = %d, label's shape = [%s]",
                            label_dim[1], label_dim));
      PADDLE_ENFORCE_EQ(
          inference_dim[0], label_dim[0],
          platform::errors::InvalidArgument(
              "ShapeError: the output's num_rows of AccuracyOp must be"
              " the same as label's num_rows. But received output's "
              "shape = [%s], label's shape = [%s], output's num_rows = %d, "
              "label's "
              "num_rows = %d",
              inference_dim, label_dim, inference_dim[0], label_dim[0]));
    }

    ctx->SetOutputDim("Accuracy", {1});
    ctx->SetOutputDim("Correct", {1});
    ctx->SetOutputDim("Total", {1});
    ctx->ShareLoD("Out", /*->*/ "Accuracy");
  }

 protected:
  framework::OpKernelType GetExpectedKernelType(
      const framework::ExecutionContext &ctx) const override {
    return framework::OpKernelType(
        OperatorWithKernel::IndicateVarDataType(ctx, "Out"), ctx.GetPlace());
  }
};

class AccuracyOpMaker : public framework::OpProtoAndCheckerMaker {
 public:
  void Make() override {
    // TODO(typhoonzero): support both inference value and indices.
    AddInput("Out", "The network output of topk (inferences)");
    AddInput("Indices", "The the network output of topk (indices)");
    AddInput("Label", "Label of the training data");
    // TODO(typhoonzero): AddInput("Weight", ...
    AddOutput("Accuracy", "The accuracy of current batch");
    AddOutput("Correct", "The correct samples count of current batch");
    AddOutput("Total", "The samples count of current batch");

    AddComment(R"DOC(
Accuracy Operator. 

It will print accuracy rate for classification.
The accuracy is calculated as follows:

$$accuracy = \frac{NumOfCorrectPredicts}{NumOfAllSamples}$$

Both the input Out and Label can carry the LoD (Level of Details)
information, or not. But the output only shares the LoD information 
with the input Out(Inference).

)DOC");
  }
};

}  // namespace operators
}  // namespace paddle

// FIXME(typhoonzero): types of T is for infernece data.
// label data is always int.
namespace ops = paddle::operators;
REGISTER_OPERATOR(
    accuracy, ops::AccuracyOp, ops::AccuracyOpMaker,
    paddle::framework::EmptyGradOpMaker<paddle::framework::OpDesc>,
    paddle::framework::EmptyGradOpMaker<paddle::imperative::OpBase>);
