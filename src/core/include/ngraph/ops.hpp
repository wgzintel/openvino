// Copyright (C) 2018-2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

// All op headers

#pragma once

#include "ngraph/op/abs.hpp"
#include "ngraph/op/acos.hpp"
#include "ngraph/op/acosh.hpp"
#include "ngraph/op/adaptive_avg_pool.hpp"
#include "ngraph/op/adaptive_max_pool.hpp"
#include "ngraph/op/add.hpp"
#include "ngraph/op/and.hpp"
#include "ngraph/op/asin.hpp"
#include "ngraph/op/asinh.hpp"
#include "ngraph/op/assign.hpp"
#include "ngraph/op/atan.hpp"
#include "ngraph/op/atanh.hpp"
#include "ngraph/op/avg_pool.hpp"
#include "ngraph/op/batch_norm.hpp"
#include "ngraph/op/batch_to_space.hpp"
#include "ngraph/op/binary_convolution.hpp"
#include "ngraph/op/broadcast.hpp"
#include "ngraph/op/bucketize.hpp"
#include "ngraph/op/ceiling.hpp"
#include "ngraph/op/clamp.hpp"
#include "ngraph/op/concat.hpp"
#include "ngraph/op/constant.hpp"
#include "ngraph/op/convert.hpp"
#include "ngraph/op/convert_like.hpp"
#include "ngraph/op/convolution.hpp"
#include "ngraph/op/cos.hpp"
#include "ngraph/op/cosh.hpp"
#include "ngraph/op/ctc_greedy_decoder.hpp"
#include "ngraph/op/ctc_greedy_decoder_seq_len.hpp"
#include "ngraph/op/ctc_loss.hpp"
#include "ngraph/op/cum_sum.hpp"
#include "ngraph/op/deformable_convolution.hpp"
#include "ngraph/op/deformable_psroi_pooling.hpp"
#include "ngraph/op/depth_to_space.hpp"
#include "ngraph/op/detection_output.hpp"
#include "ngraph/op/dft.hpp"
#include "ngraph/op/divide.hpp"
#include "ngraph/op/einsum.hpp"
#include "ngraph/op/elu.hpp"
#include "ngraph/op/embedding_segments_sum.hpp"
#include "ngraph/op/embeddingbag_offsets_sum.hpp"
#include "ngraph/op/embeddingbag_packedsum.hpp"
#include "ngraph/op/equal.hpp"
#include "ngraph/op/erf.hpp"
#include "ngraph/op/exp.hpp"
#include "ngraph/op/experimental_detectron_detection_output.hpp"
#include "ngraph/op/experimental_detectron_generate_proposals.hpp"
#include "ngraph/op/experimental_detectron_prior_grid_generator.hpp"
#include "ngraph/op/experimental_detectron_roi_feature.hpp"
#include "ngraph/op/experimental_detectron_topkrois.hpp"
#include "ngraph/op/extractimagepatches.hpp"
#include "ngraph/op/eye.hpp"
#include "ngraph/op/fake_quantize.hpp"
#include "ngraph/op/floor.hpp"
#include "ngraph/op/floor_mod.hpp"
#include "ngraph/op/gather.hpp"
#include "ngraph/op/gather_elements.hpp"
#include "ngraph/op/gather_nd.hpp"
#include "ngraph/op/gather_tree.hpp"
#include "ngraph/op/gelu.hpp"
#include "ngraph/op/greater.hpp"
#include "ngraph/op/greater_eq.hpp"
#include "ngraph/op/grn.hpp"
#include "ngraph/op/group_conv.hpp"
#include "ngraph/op/gru_cell.hpp"
#include "ngraph/op/gru_sequence.hpp"
#include "ngraph/op/hard_sigmoid.hpp"
#include "ngraph/op/hsigmoid.hpp"
#include "ngraph/op/hswish.hpp"
#include "ngraph/op/i420_to_bgr.hpp"
#include "ngraph/op/i420_to_rgb.hpp"
#include "ngraph/op/idft.hpp"
#include "ngraph/op/if.hpp"
#include "ngraph/op/interpolate.hpp"
#include "ngraph/op/irdft.hpp"
#include "ngraph/op/less.hpp"
#include "ngraph/op/less_eq.hpp"
#include "ngraph/op/log.hpp"
#include "ngraph/op/log_softmax.hpp"
#include "ngraph/op/loop.hpp"
#include "ngraph/op/lrn.hpp"
#include "ngraph/op/lstm_cell.hpp"
#include "ngraph/op/lstm_sequence.hpp"
#include "ngraph/op/matmul.hpp"
#include "ngraph/op/matrix_nms.hpp"
#include "ngraph/op/max.hpp"
#include "ngraph/op/max_pool.hpp"
#include "ngraph/op/maximum.hpp"
#include "ngraph/op/min.hpp"
#include "ngraph/op/minimum.hpp"
#include "ngraph/op/mish.hpp"
#include "ngraph/op/mod.hpp"
#include "ngraph/op/multiclass_nms.hpp"
#include "ngraph/op/multiply.hpp"
#include "ngraph/op/mvn.hpp"
#include "ngraph/op/negative.hpp"
#include "ngraph/op/non_max_suppression.hpp"
#include "ngraph/op/non_zero.hpp"
#include "ngraph/op/normalize_l2.hpp"
#include "ngraph/op/not.hpp"
#include "ngraph/op/not_equal.hpp"
#include "ngraph/op/nv12_to_bgr.hpp"
#include "ngraph/op/nv12_to_rgb.hpp"
#include "ngraph/op/one_hot.hpp"
#include "ngraph/op/or.hpp"
#include "ngraph/op/pad.hpp"
#include "ngraph/op/parameter.hpp"
#include "ngraph/op/power.hpp"
#include "ngraph/op/prelu.hpp"
#include "ngraph/op/prior_box.hpp"
#include "ngraph/op/prior_box_clustered.hpp"
#include "ngraph/op/proposal.hpp"
#include "ngraph/op/psroi_pooling.hpp"
#include "ngraph/op/random_uniform.hpp"
#include "ngraph/op/range.hpp"
#include "ngraph/op/rdft.hpp"
#include "ngraph/op/read_value.hpp"
#include "ngraph/op/reduce_l1.hpp"
#include "ngraph/op/reduce_l2.hpp"
#include "ngraph/op/reduce_logical_and.hpp"
#include "ngraph/op/reduce_logical_or.hpp"
#include "ngraph/op/reduce_mean.hpp"
#include "ngraph/op/reduce_prod.hpp"
#include "ngraph/op/reduce_sum.hpp"
#include "ngraph/op/region_yolo.hpp"
#include "ngraph/op/relu.hpp"
#include "ngraph/op/reorg_yolo.hpp"
#include "ngraph/op/reshape.hpp"
#include "ngraph/op/result.hpp"
#include "ngraph/op/reverse.hpp"
#include "ngraph/op/reverse_sequence.hpp"
#include "ngraph/op/rnn_cell.hpp"
#include "ngraph/op/rnn_sequence.hpp"
#include "ngraph/op/roi_align.hpp"
#include "ngraph/op/roi_pooling.hpp"
#include "ngraph/op/roll.hpp"
#include "ngraph/op/round.hpp"
#include "ngraph/op/scatter_elements_update.hpp"
#include "ngraph/op/scatter_nd_update.hpp"
#include "ngraph/op/scatter_update.hpp"
#include "ngraph/op/select.hpp"
#include "ngraph/op/selu.hpp"
#include "ngraph/op/shape_of.hpp"
#include "ngraph/op/shuffle_channels.hpp"
#include "ngraph/op/sigmoid.hpp"
#include "ngraph/op/sign.hpp"
#include "ngraph/op/sin.hpp"
#include "ngraph/op/sinh.hpp"
#include "ngraph/op/slice.hpp"
#include "ngraph/op/softmax.hpp"
#include "ngraph/op/softplus.hpp"
#include "ngraph/op/softsign.hpp"
#include "ngraph/op/space_to_batch.hpp"
#include "ngraph/op/space_to_depth.hpp"
#include "ngraph/op/split.hpp"
#include "ngraph/op/sqrt.hpp"
#include "ngraph/op/squared_difference.hpp"
#include "ngraph/op/squeeze.hpp"
#include "ngraph/op/strided_slice.hpp"
#include "ngraph/op/subtract.hpp"
#include "ngraph/op/swish.hpp"
#include "ngraph/op/tan.hpp"
#include "ngraph/op/tanh.hpp"
#include "ngraph/op/tensor_iterator.hpp"
#include "ngraph/op/tile.hpp"
#include "ngraph/op/topk.hpp"
#include "ngraph/op/transpose.hpp"
#include "ngraph/op/unsqueeze.hpp"
#include "ngraph/op/util/attr_types.hpp"
#include "ngraph/op/util/op_types.hpp"
#include "ngraph/op/variadic_split.hpp"
#include "ngraph/op/xor.hpp"
