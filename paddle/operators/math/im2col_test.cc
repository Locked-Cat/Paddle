/* Copyright (c) 2016 PaddlePaddle Authors. All Rights Reserve.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License. */

#include "paddle/operators/math/im2col.h"
#include <gtest/gtest.h>
#include <iostream>

template <typename Place>
void testIm2col() {
  paddle::framework::Tensor input_tmp;
  paddle::framework::Tensor input;
  paddle::framework::Tensor output_cfo;
  paddle::framework::Tensor output_ocf;
  paddle::framework::Tensor output_tmp;

  /**
   * input = [0, 1, 2,
   *          3, 4, 5]
   *
   * output_cfo = [0, 1
   *               1, 2
   *               3, 4
   *               4, 5]
   *
   * output_ocf = [0, 1, 3, 4
   *               1, 2, 4, 5]
   */
  int input_height = 2;
  int input_width = 3;
  int filter_size = 2;
  int stride = 1;
  int padding = 0;
  int output_height = (input_height - filter_size + 2 * padding) / stride + 1;
  int output_width = (input_width - filter_size + 2 * padding) / stride + 1;
  float* input_ptr = input_tmp.mutable_data<float>(
      {1, input_height, input_width}, paddle::platform::CPUPlace());
  float arr[6] = {0, 1, 2, 3, 4, 5};
  memcpy(input_ptr, arr, 6 * sizeof(float));

  auto* place = new Place();
  if (paddle::platform::is_cpu_place(*place)) {
    input = input_tmp;
  } else {
    input.CopyFrom<float>(input_tmp, *place);
  }
  output_cfo.mutable_data<float>(
      {1, filter_size, filter_size, output_height, output_width}, *place);
  output_ocf.mutable_data<float>(
      {output_height, output_width, 1, filter_size, filter_size}, *place);

  paddle::operators::math::Im2ColFunctor<
      paddle::operators::math::ColFormat::kCFO, Place, float>
      im2col;
  paddle::operators::math::Im2ColFunctor<
      paddle::operators::math::ColFormat::kOCF, Place, float>
      im2col_ocf;

  paddle::platform::DeviceContext* context;
  if (paddle::platform::is_cpu_place(*place)) {
    context =
        new paddle::platform::CPUDeviceContext(paddle::platform::CPUPlace());
  } else {
#ifdef PADDLE_WITH_CUDA
    context =
        new paddle::platform::CUDADeviceContext(paddle::platform::GPUPlace());
#else
    PADDLE_THROW("no GPU support");
#endif  // PADDLE_ONLY_CPU
  }
  im2col(*context, input, output_cfo, stride, stride, padding, padding);
  im2col_ocf(*context, input, output_ocf, stride, stride, padding, padding);

  float* out_cfo_ptr;
  if (paddle::platform::is_cpu_place(*place)) {
    out_cfo_ptr = output_cfo.data<float>();
  } else {
    output_tmp.CopyFrom<float>(output_cfo, paddle::platform::CPUPlace());
    out_cfo_ptr = output_tmp.data<float>();
  }
  EXPECT_EQ(out_cfo_ptr[0], 0);
  EXPECT_EQ(out_cfo_ptr[1], 1);
  EXPECT_EQ(out_cfo_ptr[2], 1);
  EXPECT_EQ(out_cfo_ptr[3], 2);
  EXPECT_EQ(out_cfo_ptr[4], 3);
  EXPECT_EQ(out_cfo_ptr[5], 4);
  EXPECT_EQ(out_cfo_ptr[6], 4);
  EXPECT_EQ(out_cfo_ptr[7], 5);

  float* out_ocf_ptr;
  if (paddle::platform::is_cpu_place(*place)) {
    out_ocf_ptr = output_ocf.data<float>();
  } else {
    output_tmp.CopyFrom<float>(output_ocf, paddle::platform::CPUPlace());
    out_ocf_ptr = output_tmp.data<float>();
  }
  EXPECT_EQ(out_ocf_ptr[0], 0);
  EXPECT_EQ(out_ocf_ptr[1], 1);
  EXPECT_EQ(out_ocf_ptr[2], 3);
  EXPECT_EQ(out_ocf_ptr[3], 4);
  EXPECT_EQ(out_ocf_ptr[4], 1);
  EXPECT_EQ(out_ocf_ptr[5], 2);
  EXPECT_EQ(out_ocf_ptr[6], 4);
  EXPECT_EQ(out_ocf_ptr[7], 5);
}

TEST(math, im2col) {
  testIm2col<paddle::platform::CPUPlace>();
#ifdef PADDLE_WITH_CUDA
  testIm2col<paddle::platform::GPUPlace>();
#endif
}
