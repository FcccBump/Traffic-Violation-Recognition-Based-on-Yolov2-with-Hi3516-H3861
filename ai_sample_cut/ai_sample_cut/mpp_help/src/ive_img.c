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
#include <errno.h>
#include <unistd.h>

#include "hi_comm_vb.h"
#include "mpi_sys.h"
#include "ive_img.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#define HALF_THE_HEIGHT 2 // Half the height

HI_U32 HiAlign16(HI_U32 num)
{
    return (((num) + 16 - 1) / 16 * 16); // 16: align 16
}

HI_U32 HiAlign32(HI_U32 num)
{
    return (((num) + 32 - 1) / 32 * 32); // 32: align 32
}

/*
 * VIDEO_FRAME_INFO_S格式转换成IVE_IMAGE_S格式
 * 复制数据指针，不复制数据
 *
 * video frame to ive image.
 * Copy the data pointer, do not copy the data.
 */
int FrmToOrigImg(const VIDEO_FRAME_INFO_S* frm, IVE_IMAGE_S *img)
{
    static const int chnNum = 2; // Currently only supports YUV420/422, so only the addresses of 2 channels are copied
    PIXEL_FORMAT_E pixelFormat = frm->stVFrame.enPixelFormat;// enPixelFormat 是视频像素格式，取决于摄像头输入的配置

    if (memset_s(img, sizeof(*img), 0, sizeof(*img)) != EOK) {
        HI_ASSERT(0);
    }

    img->u32Width = frm->stVFrame.u32Width;
    img->u32Height = frm->stVFrame.u32Height;

    int i=0;
    if (pixelFormat == PIXEL_FORMAT_YVU_SEMIPLANAR_420) {
        img->enType = IVE_IMAGE_TYPE_YUV420SP;
        // for(i=0; i<5 ; i++){
        //     printf("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa \n");
        //     printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ \n");
        //     printf("bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb \n");
        // }
    } else if (pixelFormat == PIXEL_FORMAT_YVU_SEMIPLANAR_422) {
        img->enType = IVE_IMAGE_TYPE_YUV422SP;
        // for(i=0; i<5 ; i++){
        //     printf("ccccccccccccccccccccccccccccccccccccccccccccccccccccccc \n");
        //     printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ \n");
        //     printf("ddddddddddddddddddddddddddddddddddddddddddddddddddddddd \n");
        // }

    } else {
        HI_ASSERT(0);
        return -1;
    }

    for (int i = 0; i < chnNum; i++) {
        img->au64PhyAddr[i] = frm->stVFrame.u64PhyAddr[i];
        img->au64VirAddr[i] = frm->stVFrame.u64VirAddr[i];
        img->au32Stride[i] = frm->stVFrame.u32Stride[i];
    }
    return 0;
}

/*
 * 计算通道的步长
 * Calculate the stride of a channel
 */
static uint32_t IveCalStride(IVE_IMAGE_TYPE_E enType, uint32_t width, AlignType align)
{
    uint32_t size = 1;

    switch (enType) {
        case IVE_IMAGE_TYPE_U8C1:
        case IVE_IMAGE_TYPE_S8C1:
        case IVE_IMAGE_TYPE_S8C2_PACKAGE:
        case IVE_IMAGE_TYPE_S8C2_PLANAR:
        case IVE_IMAGE_TYPE_U8C3_PACKAGE:
        case IVE_IMAGE_TYPE_U8C3_PLANAR:
            size = sizeof(HI_U8);
            break;
        case IVE_IMAGE_TYPE_S16C1:
        case IVE_IMAGE_TYPE_U16C1:
            size = sizeof(HI_U16);
            break;
        case IVE_IMAGE_TYPE_S32C1:
        case IVE_IMAGE_TYPE_U32C1:
            size = sizeof(uint32_t);
            break;
        case IVE_IMAGE_TYPE_S64C1:
        case IVE_IMAGE_TYPE_U64C1:
            size = sizeof(uint64_t);
            break;
        default:
            break;
    }

    if (align == ALIGN_TYPE_16) {
        return HiAlign16(width * size);
    } else if (align == ALIGN_TYPE_32) {
        return HiAlign32(width * size);
    } else {
        HI_ASSERT(0);
        return 0;
    }
}

/*
 * 根据类型和大小创建缓存
 * Create ive image buffer based on type and size
 */
int IveImgCreate(IVE_IMAGE_S* img,
    IVE_IMAGE_TYPE_E enType, uint32_t width, uint32_t height)
{
    HI_ASSERT(img);
    uint32_t oneChnSize;
    uint32_t size;
    int ret;

    if (memset_s(img, sizeof(*img), 0, sizeof(*img)) != EOK) {
        HI_ASSERT(0);
    }
    img->enType = enType;
    img->u32Width = width;
    img->u32Height = height;
    img->au32Stride[0] = IveCalStride(img->enType, img->u32Width, ALIGN_TYPE_16);

    switch (enType) {
        case IVE_IMAGE_TYPE_U8C1:
        case IVE_IMAGE_TYPE_S8C1: // Only 1 channel
            size = img->au32Stride[0] * img->u32Height;
            ret = HI_MPI_SYS_MmzAlloc(&img->au64PhyAddr[0], (void**)&img->au64VirAddr[0], NULL, NULL, size);
            SAMPLE_CHECK_EXPR_RET(HI_SUCCESS != ret, ret, "Error(%#x), HI_MPI_SYS_MmzAlloc!\n", ret);
            break;
        /*
         * 大小相当于像素的1.5倍(3/2), 相当于2个通道
         * The size is equivalent to 1.5 times (3/2) of the pixel, which is equivalent to 2 channels
         */
        case IVE_IMAGE_TYPE_YUV420SP:
        /*
         * 大小相当于像素的2倍，相当于2个通道
         * The size is equivalent to 2 times the pixel, which is equivalent to 2 channels
         */
        case IVE_IMAGE_TYPE_YUV422SP:
            if (enType == IVE_IMAGE_TYPE_YUV420SP) {
                size = img->au32Stride[0] * img->u32Height * THREE_TIMES / TWO_TIMES;
            } else {
                size = img->au32Stride[0] * img->u32Height * TWO_TIMES;
            }
            ret = HI_MPI_SYS_MmzAlloc(&img->au64PhyAddr[0], (void**)&img->au64VirAddr[0], NULL, NULL, size);
            SAMPLE_CHECK_EXPR_RET(HI_SUCCESS != ret, ret, "Error(%#x), HI_MPI_SYS_MmzAlloc!\n", ret);

            /*
             * 设置通道1地址的步长，两者都需要通道1
             * Set the stride of the address of channel 1, both of which require channel 1
             */
            img->au32Stride[1] = img->au32Stride[0];
            img->au64PhyAddr[1] = img->au64PhyAddr[0] + img->au32Stride[0] * (uint64_t)img->u32Height;
            img->au64VirAddr[1] = img->au64VirAddr[0] + img->au32Stride[0] * (uint64_t)img->u32Height;
            break;

        case IVE_IMAGE_TYPE_U8C3_PLANAR: // 3 channels, often used for RGB
            oneChnSize = img->au32Stride[0] * img->u32Height;
            size = oneChnSize * 3; // 3 channels have the same size
            ret = HI_MPI_SYS_MmzAlloc(&img->au64PhyAddr[0], (void**)&img->au64VirAddr[0], NULL, NULL, size);
            SAMPLE_CHECK_EXPR_RET(HI_SUCCESS != ret, ret, "Error(%#x), HI_MPI_SYS_MmzAlloc!\n", ret);

            /*
             * 设置通道1和通道2的地址和步长
             * Set the address and stride of channel 1 and channel 2
             */
            img->au64VirAddr[1] = img->au64VirAddr[0] + oneChnSize;
            img->au64PhyAddr[1] = img->au64PhyAddr[0] + oneChnSize;
            img->au32Stride[1] = img->au32Stride[0];
            img->au64VirAddr[2] = img->au64VirAddr[1] + oneChnSize; // 2: au64VirAddr array subscript, not out of bounds
            img->au64PhyAddr[2] = img->au64PhyAddr[1] + oneChnSize; // 2: au64VirAddr array subscript, not out of bounds
            img->au32Stride[2] = img->au32Stride[0]; // 2: au64VirAddr array subscript, not out of bounds
            break;

        /*
         * 目前如下格式不支持，主要为YVC420P, YUV422P, S8C2_PACKAGE, S8C2_PLANAR,
         * S32C1, U32C1, S64C1, U64C1, S16C1, U16C1, U8C3_PACKAGE等
         *
         * Types not currently supported: YVC420P, YUV422P, S8C2_PACKAGE, S8C2_PLANAR,
         * S32C1, U32C1, S64C1, U64C1, S16C1, U16C1, U8C3_PACKAGE,etc.
         */
        default:
            HI_ASSERT(0);
            break;
    }
    return HI_SUCCESS;
}

int ImgYuvCrop(const IVE_IMAGE_S *src, IVE_IMAGE_S *dst, const RectBox* origBox)
{
    RectBox box = *origBox;
    int boxWidth = box.xmax - box.xmin;
    int boxHeight = box.ymax - box.ymin;
    int ret;

    HI_ASSERT(boxWidth > 0 && boxWidth <= src->u32Width);
    HI_ASSERT(boxHeight > 0 && boxHeight <= src->u32Height);
    HI_ASSERT(src->au64VirAddr[0]);
    HI_ASSERT(src->au32Stride[0] >= src->u32Width);

    /*
     * 将框的宽度/高度调整为 2 的倍数
     * Adjust the width/height of the box to a multiple of 2
     */
    if (boxWidth == 1 || boxHeight == 1) {
        SAMPLE_PRT("box dstWidth=1 && dstHeight=1\n");
        return -1;
    }
    if (boxWidth % HI_OVEN_BASE) {
        box.xmax--;
        boxWidth--;
    }
    if (boxHeight % HI_OVEN_BASE) {
        box.ymax--;
        boxHeight--;
    }

    ret = IveImgCreate(dst, src->enType, boxWidth, boxHeight);
    HI_ASSERT(!ret);

    /*
     * 将框从源地址复制到目的地址
     * copy box from src to dst
     */
    /*
     * Y分量
     * Y component
     */
    int srcStrideY = src->au32Stride[0];
    int dstStrideY = dst->au32Stride[0];
    uint8_t *srcBufY = (uint8_t*)((uintptr_t)src->au64VirAddr[0]);
    uint8_t *dstBufY = (uint8_t*)((uintptr_t)dst->au64VirAddr[0]);
    uint8_t *srcPtrY = &srcBufY[box.ymin * srcStrideY];
    uint8_t *dstPtrY = dstBufY;
    for (int h = 0; h < boxHeight; h++, srcPtrY += srcStrideY, dstPtrY += dstStrideY) {
        if (memcpy_s(dstPtrY, boxWidth, srcPtrY + box.xmin, boxWidth) != EOK) {
            HI_ASSERT(0);
        }
    }
    HI_ASSERT(dstPtrY - dstBufY == boxHeight * dstStrideY);

    /*
     * UV分量
     * UV component
     */
    int srcStrideUV = src->au32Stride[1];
    int dstStrideUV = dst->au32Stride[1];
    uint8_t *srcBufUV = (uint8_t*)((uintptr_t)src->au64VirAddr[1]);
    uint8_t *dstBufUV = (uint8_t*)((uintptr_t)dst->au64VirAddr[1]);
    uint8_t *srcPtrUV = &srcBufUV[(box.ymin / HALF_THE_HEIGHT) * srcStrideUV];
    uint8_t *dstPtrUV = dstBufUV;
    for (int h = 0; h < (boxHeight / HALF_THE_HEIGHT);
        h++, srcPtrUV += srcStrideUV, dstPtrUV += dstStrideUV) {
        if (memcpy_s(dstPtrUV, boxWidth, srcPtrUV + box.xmin, boxWidth) != EOK) {
            HI_ASSERT(0);
        }
    }
    HI_ASSERT(dstPtrUV - dstBufUV == (boxHeight / HALF_THE_HEIGHT) * dstStrideUV);

    return ret;
}

/*
 * 销毁ive image
 * Destory ive image
 */
void IveImgDestroy(IVE_IMAGE_S* img)
{
    for (int i = 0; i < IMG_FULL_CHN; i++) {
        if (img->au64PhyAddr[0] && img->au64VirAddr[0]) {
            HI_MPI_SYS_MmzFree(img->au64PhyAddr[i], (void*)((uintptr_t)img->au64VirAddr[i]));
            img->au64PhyAddr[i] = 0;
            img->au64VirAddr[i] = 0;
        }
    }
    if (memset_s(img, sizeof(*img), 0, sizeof(*img)) != EOK) {
        HI_ASSERT(0);
    }
}

int OrigImgToFrm(const IVE_IMAGE_S *img, VIDEO_FRAME_INFO_S* frm)
{
    static const int chnNum = 2;
    IVE_IMAGE_TYPE_E enType = img->enType;
    if (memset_s(frm, sizeof(*frm), 0, sizeof(*frm)) != EOK) {
        HI_ASSERT(0);
    }

    frm->stVFrame.u32Width = img->u32Width;
    frm->stVFrame.u32Height = img->u32Height;

    if (enType == IVE_IMAGE_TYPE_YUV420SP) {
        frm->stVFrame.enPixelFormat = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    } else if (enType == IVE_IMAGE_TYPE_YUV422SP) {
        frm->stVFrame.enPixelFormat = PIXEL_FORMAT_YVU_SEMIPLANAR_422;
    } else {
        HI_ASSERT(0);
        return -1;
    }

    for (int i = 0; i < chnNum; i++) {
        frm->stVFrame.u64PhyAddr[i] = img->au64PhyAddr[i];
        frm->stVFrame.u64VirAddr[i] = img->au64VirAddr[i];
        frm->stVFrame.u32Stride[i] = img->au32Stride[i];
    }
    return 0;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
