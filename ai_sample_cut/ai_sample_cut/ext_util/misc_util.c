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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include "sample_media_ai.h"
#include "ai_infer_process.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

/*
 * 将整数放大到给定的倍数范围
 * Amplify the integer to the given multiple range
 */
int IntZoomTo(int n, double rate, double rateMin, double rateMax)
{
    HI_ASSERT(rateMin < 1 && rateMax > 1);
    int ret;

    if (!rateMin) {
        HI_ASSERT(rateMin);
        return n;
    } else {
        if (rate > rateMax) {
            ret = n * (int)rateMax;
        } else if (rate < rateMin) {
            ret = n / (int)(1 / rateMin);
        } else {
            ret = (int)(n * rate);
        }
        return ret < 1 ? 1 : ret;
    }
}

/*
 * 按比例转换坐标
 * Convert coordinates proportionally
 */
void RectBoxTran(RectBox* box, int srcWidth, int srcHeight, int dstWidth, int dstHeight)
{
    if (!srcWidth || !srcHeight) {
        HI_ASSERT(srcWidth && srcHeight);
    } else {
        if (srcWidth != 0 && srcHeight != 0) {
            box->xmin = box->xmin * dstWidth / srcWidth * HI_OVEN_BASE / HI_OVEN_BASE;
            box->xmax = box->xmax * dstWidth / srcWidth * HI_OVEN_BASE / HI_OVEN_BASE;
            box->ymin = box->ymin * dstHeight / srcHeight * HI_OVEN_BASE / HI_OVEN_BASE;
            box->ymax = box->ymax * dstHeight / srcHeight * HI_OVEN_BASE / HI_OVEN_BASE;
        }
    }
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
