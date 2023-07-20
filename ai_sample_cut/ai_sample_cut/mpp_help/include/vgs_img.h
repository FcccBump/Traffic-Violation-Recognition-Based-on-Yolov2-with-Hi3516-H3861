/*
 * Copyright (c) 2022 HiSilicon (Shanghai) Technologies CO., LIMITED.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef VGS_IMG_H
#define VGS_IMG_H

#include <stdbool.h>
#include <stdint.h>

#include "hi_common.h"
#include "hi_comm_video.h"
#include "hi_comm_venc.h"
#include "ai_infer_process.h"

#if __cplusplus
extern "C" {
#endif

/*
 * 常用数字单位
 * Commonly used numerical units
 */
#define HI_KB               1024
#define HI_MB               (1024 * 1024)
#define HI_MS_OF_SEC        1000 // 1s in milliseconds
#define HI_NS_OF_MS         1000000 // Nanoseconds in 1ms
#define HI_BYTE_BITS        8 // Number of bits in 1 byte
#define HI_INT8_BITS        8 // 8-bit integer number of bits
#define HI_INT16_BITS       16 // 16-bit integer number of bits
#define HI_INT32_BITS       32 // 32-bit integer number of bits
#define HI_INT64_BITS       64 // The number of bits of a 64-bit integer

/*
 * 帧缩放
 * 多次调用vgs_resize以实现任意比例的缩放
 * 为简化实现，约定每次缩放最大14倍，此时宽、高仅需2像素对齐
 * 当两个方向缩放方向不同时，例如一向(如X)放大，另一向缩小倍，无需特别处理
 * 此时某个方向或两个方向缩放比例均超标，也不需要特别处理
 *
 * resize frame.
 * Call vgs_resize multiple times to achieve arbitrary scaling.
 * In order to simplify the implementation, it is agreed that each zoom is up to 14 times,
 * and at this time, the width and height only need to be aligned by 2 pixels.
 * When the zoom directions are different in the two directions, for example,
 * zooming in one direction (such as X) and zooming out in the other direction,
 * no special processing is required. At this time, the zoom ratio in one direction or
 * both directions exceeds the standard, and no special treatment is required.
 */
int MppFrmResize(
    const VIDEO_FRAME_INFO_S* src,
    VIDEO_FRAME_INFO_S* dst,
    uint32_t dstWidth, uint32_t dstHeight);

/*
 * 销毁帧
 * Destory frame
 */
void MppFrmDestroy(VIDEO_FRAME_INFO_S* frm);

/*
 * 在框架中叠加一个或多个矩形框
 * Superimpose one or more rectangular boxes in the frame
 */
int MppFrmDrawRects(VIDEO_FRAME_INFO_S *frm,
    const RectBox *boxes, int boxesNum, uint32_t color, int thick);

#ifdef __cplusplus
}
#endif
#endif
