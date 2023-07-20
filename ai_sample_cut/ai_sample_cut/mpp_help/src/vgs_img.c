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

#include "hi_comm_vb.h"
#include "hi_comm_vgs.h"
#include "hi_comm_region.h"
#include "mpi_sys.h"
#include "mpi_vgs.h"
#include "hi_buffer.h"

#include "misc_util.h"
#include "ai_infer_process.h"
#include "vgs_img.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#define VGS_MAX_LINE            100 // The maximum number of lines for a superimposed graphics operation
#define RECT_LINES              4 // The number of lines in a rectangle

#define RECT_LINE1              1
#define RECT_LINE2              2
#define RECT_LINE3              3

/*
 * 设置帧的buffer
 * Set the buf of the frame
 */
static void MppFrmSetBuf(VIDEO_FRAME_INFO_S* frm,
    const VB_CAL_CONFIG_S *vbCfg, HI_U64 phyAddr, uint8_t *virAddr)
{
    /*
     * 目前只支持SP422/SP420，不支持SP444
     * Currently only SP422/SP420 is supported, SP444 is not supported
     */
    frm->stVFrame.u32HeaderStride[0] = vbCfg->u32HeadStride;
    frm->stVFrame.u32HeaderStride[1] = vbCfg->u32HeadStride;
    frm->stVFrame.u32HeaderStride[2] = vbCfg->u32HeadStride; // 2: Array subscript, not out of bounds
    frm->stVFrame.u64HeaderPhyAddr[0] = phyAddr;
    frm->stVFrame.u64HeaderPhyAddr[1] = frm->stVFrame.u64HeaderPhyAddr[0] + vbCfg->u32HeadYSize;
    frm->stVFrame.u64HeaderPhyAddr[2] = frm->stVFrame.u64HeaderPhyAddr[1]; // 2: Array subscript, not out of bounds
    frm->stVFrame.u64HeaderVirAddr[0] = (HI_U64)(HI_UL)virAddr;
    frm->stVFrame.u64HeaderVirAddr[1] = frm->stVFrame.u64HeaderVirAddr[0] + vbCfg->u32HeadYSize;
    frm->stVFrame.u64HeaderVirAddr[2] = frm->stVFrame.u64HeaderVirAddr[1]; // 2: Array subscript, not out of bounds

    frm->stVFrame.u32Stride[0] = vbCfg->u32MainStride;
    frm->stVFrame.u32Stride[1] = vbCfg->u32MainStride;
    frm->stVFrame.u32Stride[2] = vbCfg->u32MainStride; // 2: Array subscript, not out of bounds
    frm->stVFrame.u64PhyAddr[0] = frm->stVFrame.u64HeaderPhyAddr[0] + vbCfg->u32HeadSize;
    frm->stVFrame.u64PhyAddr[1] = frm->stVFrame.u64PhyAddr[0] + vbCfg->u32MainYSize;
    frm->stVFrame.u64PhyAddr[2] = frm->stVFrame.u64PhyAddr[1]; // 2: Array subscript, not out of bounds
    frm->stVFrame.u64VirAddr[0] = frm->stVFrame.u64HeaderVirAddr[0] + vbCfg->u32HeadSize;
    frm->stVFrame.u64VirAddr[1] = frm->stVFrame.u64VirAddr[0] + vbCfg->u32MainYSize;
    frm->stVFrame.u64VirAddr[2] = frm->stVFrame.u64VirAddr[1]; // 2: Array subscript, not out of bounds
}

/*
 * 创建一个空的frame buf.
 * 它根据参数的规格计算frame所需空间，为frame分配内存.
 * @param frm[in|out]: 若成功，创建的frame信息会复制到frm.
 * @param width[in]: frame width.
 * @param height[in]: frame height.
 * @param pixelFormat[in]: 置为-1表示取默认值PIXEL_FORMAT_YVU_SEMIPLANAR_420.
 * @param bitWidth[in]: 置为-1表示取默认值DATA_BITWIDTH_8.
 * @param compressMode[in]: 置为-1表示取默认值COMPRESS_MODE_NONE.
 * @param align[in]: 对齐的字节长度. 置为-1表示取默认值'自动对齐'.
 *
 * Create an empty frame buf.
 * It calculates the space required by the frame according to the parameter specification,
 * and allocates memory for the frame.
 * @param frm[in|out]: If successful, the created frame information will be copied to frm.
 * @param width[in]: frame width.
 * @param height[in]: frame height.
 * @param pixelFormat[in]: Set to -1 means to take the default value PIXEL_FORMAT_YVU_SEMIPLANAR_420.
 * @param bitWidth[in]: Set to -1 means to take the default value DATA_BITWIDTH_8.
 * @param compressMode[in]: Set to -1 means to take the default value COMPRESS_MODE_NONE.
 * @param align[in]: The byte length of the alignment. Set to -1 to take the default value 'automatic alignment'.
 */
int MppFrmCreate(
    VIDEO_FRAME_INFO_S* frm,
    int width, int height,
    PIXEL_FORMAT_E pixelFormat,
    DATA_BITWIDTH_E bitWidth,
    COMPRESS_MODE_E compressMode,
    int align)
{
    HI_ASSERT(frm);
    VB_CAL_CONFIG_S vbCfg;

    if (memset_s(frm, sizeof(*frm), 0, sizeof(*frm)) != EOK) {
        HI_ASSERT(0);
    }

    HI_ASSERT(width > 0 && height > 0);
    if ((int)pixelFormat < 0) {
        pixelFormat = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    }
    if ((int)bitWidth < 0) {
        bitWidth = DATA_BITWIDTH_8;
    }
    if ((int)compressMode < 0) {
        compressMode = COMPRESS_MODE_NONE;
    }
    if (align < 0) {
        HI_ASSERT(0);
    }

    COMMON_GetPicBufferConfig(width, height, pixelFormat, bitWidth, compressMode, align, &vbCfg);

    VB_BLK vbHnd = HI_MPI_VB_GetBlock(VB_INVALID_POOLID, vbCfg.u32VBSize, NULL);
    if (vbHnd == VB_INVALID_HANDLE) {
        SAMPLE_PRT("HI_MPI_VB_GetBlock FAIL\n");
        return HI_FAILURE;
    }

    HI_U64 phyAddr = HI_MPI_VB_Handle2PhysAddr(vbHnd);
    HI_ASSERT(phyAddr);
    uint8_t* virAddr = (uint8_t*)HI_MPI_SYS_Mmap(phyAddr, vbCfg.u32VBSize);
    HI_ASSERT(virAddr);

    /*
     * u64PrivateData用来存储映射的内存区长度和vbHnd，在destroy时会用到
     *
     * u64PrivateData is used to store the length of the mapped memory area and vbHnd,
     * which will be used when destroying
     */
    frm->stVFrame.u64PrivateData = ((uint64_t)(uint32_t)vbHnd) << HI_INT32_BITS;
    frm->stVFrame.u64PrivateData |= (uint64_t)vbCfg.u32VBSize;

    frm->enModId = HI_ID_VGS;
    frm->u32PoolId = HI_MPI_VB_Handle2PoolId(vbHnd);

    frm->stVFrame.u32Width = width;
    frm->stVFrame.u32Height = height;
    frm->stVFrame.enField = VIDEO_FIELD_FRAME;
    frm->stVFrame.enPixelFormat = pixelFormat;
    frm->stVFrame.enVideoFormat = VIDEO_FORMAT_LINEAR;
    frm->stVFrame.enCompressMode = compressMode;
    frm->stVFrame.enDynamicRange = DYNAMIC_RANGE_SDR8;
    frm->stVFrame.enColorGamut = COLOR_GAMUT_BT601;

    MppFrmSetBuf(frm, &vbCfg, phyAddr, virAddr);
    return HI_SUCCESS;
}

/*
 * 判断frame是否可用， 即是否分配了内存
 * Determine whether the frame is available, That is, whether the memory is allocated
 */
bool MppFrmValid(const VIDEO_FRAME_INFO_S* frm)
{
    /*
     * VPSS输出的frame默认没有映射虚地址
     * The frame output by VPSS does not map virtual addresses by default
     */
    return frm->stVFrame.u64PhyAddr[0];
}

/*
 * 销毁frame
 * Destory frame
 */
void MppFrmDestroy(VIDEO_FRAME_INFO_S* frm)
{
    if (!MppFrmValid(frm)) {
        return;
    }

    /*
     * u64PrivateData被用来存储映射的内存区长度和vbHnd，在创建时设置
     * u64PrivateData is used to store the length of the mapped memory area and vbHnd, which is set at create
     */
    uint32_t memSize = (uint32_t)(frm->stVFrame.u64PrivateData);
    uint32_t vbHnd = (uint32_t)(frm->stVFrame.u64PrivateData >> HI_INT32_BITS);
    HI_S32 ret;

    ret = HI_MPI_SYS_Munmap((void*)(uintptr_t)frm->stVFrame.u64VirAddr[0], memSize);
    HI_ASSERT(ret == HI_SUCCESS);
    ret = HI_MPI_VB_ReleaseBlock(vbHnd);
    HI_ASSERT(ret == HI_SUCCESS);
    if (memset_s(frm, sizeof(*frm), 0, sizeof(*frm)) != EOK) {
        HI_ASSERT(0);
    }
}

/*
 * 执行一次VGS缩放
 * 每一次VGS resize的缩放倍数是有限制的. VGS支持对一幅图像进行缩放，最大支持图像
 * 宽高均放大16倍，缩小30 倍。支持单分量（Y）缩放
 *
 * Perform a VGS resize.
 * The zoom factor of each VGS resize is limited. VGS supports zooming of an image.
 * The width and height are both enlarged by 16 times and reduced by 30 times.Support single-component (Y) scaling.
 */
static int VgsResizeOnce(const VIDEO_FRAME_INFO_S* src, VIDEO_FRAME_INFO_S* dst, uint32_t dstWidth, uint32_t dstHeight)
{
    HI_ASSERT(src && dst);
    HI_ASSERT(dstWidth > 0 && dstHeight > 0);
    VGS_HANDLE jobHnd = -1;
    VGS_TASK_ATTR_S task;
    int ret;

    ret = MppFrmCreate(dst, dstWidth, dstHeight, src->stVFrame.enPixelFormat, DATA_BITWIDTH_8,
        src->stVFrame.enCompressMode, 0);
    if (ret != 0) {
        SAMPLE_PRT("frm resize FAIL, for create dstFrm FAIL\n");
        return ret;
    }

    if (memset_s(&task, sizeof(task), 0, sizeof(task)) != EOK) {
        HI_ASSERT(0);
    }
    task.stImgIn = *src;
    task.stImgOut = *dst;

    ret = HI_MPI_VGS_BeginJob(&jobHnd);
    if (ret != 0) {
        SAMPLE_PRT("HI_MPI_VGS_BeginJob FAIL, ret=%08X\n", ret);
        if (jobHnd >= 0 && HI_MPI_VGS_CancelJob(jobHnd) != HI_SUCCESS) {
            HI_ASSERT(0);
        }
        MppFrmDestroy(dst);
        return ret;
    }
    HI_ASSERT(jobHnd >= 0);

    ret = HI_MPI_VGS_AddScaleTask(jobHnd, &task, VGS_SCLCOEF_NORMAL);
    if (ret != 0) {
        SAMPLE_PRT("HI_MPI_VGS_AddScaleTask FAIL, ret=%08X\n", ret);
        if (jobHnd >= 0 && HI_MPI_VGS_CancelJob(jobHnd) != HI_SUCCESS) {
            HI_ASSERT(0);
        }
        MppFrmDestroy(dst);
        return ret;
    }

    ret = HI_MPI_VGS_EndJob(jobHnd);
    if (ret != 0) {
        SAMPLE_PRT("HI_MPI_VGS_EndJob FAIL, ret=%08X\n", ret);
        if (jobHnd >= 0 && HI_MPI_VGS_CancelJob(jobHnd) != HI_SUCCESS) {
            HI_ASSERT(0);
        }
        MppFrmDestroy(dst);
        return ret;
    }
    return 0;
}

/*
 * 对一帧实现缩放
 * 多次调用vgs_resize以实现任意比例的缩放
 * 为简化实现，约定每次缩放最大14倍，此时宽、高仅需2像素对齐
 * 当两个方向缩放方向不同时，例如一向(如X)放大，另一向缩小倍，无需特别处理
 * 此时某个方向或两个方向缩放比例均超标，也不需要特别处理
 *
 * resize frame.
 * Call vgs_resize multiple times to achieve arbitrary scaling.
 * In order to simplify the implementation, it is agreed that each zoom is up to 14 times,
 * and at this time, the width and height only need to be aligned by 2 pixels.
 * When the zooming direction of the two directions is different, for example,
 * zooming in one direction (such as X) and zooming out in the other direction, no special processing is required
 * At this time, the zoom ratio in one direction or both directions exceeds the standard,
 * and no special treatment is required.
 */
int MppFrmResize(
    const VIDEO_FRAME_INFO_S* src,
    VIDEO_FRAME_INFO_S* dst,
    uint32_t dstWidth, uint32_t dstHeight)
{
    static const double rateMax = 14.0; // Maximum magnification
    static const double rateMin = 1.0 / rateMax; // The smallest magnification, that is, the largest reduction

    uint32_t srcWidth = src->stVFrame.u32Width;
    uint32_t srcHeight = src->stVFrame.u32Height;
    HI_ASSERT(srcWidth > 0 && srcHeight > 0);
    HI_ASSERT(!(srcWidth % HI_OVEN_BASE) && !(srcHeight % HI_OVEN_BASE));
    HI_ASSERT(dstWidth > 0 && dstHeight > 0);
    HI_ASSERT(!(dstWidth % HI_OVEN_BASE) && !(dstHeight % HI_OVEN_BASE));
    int ret;

    /*
     * 放大倍数
     * magnification
     */
    double widthRate = ((double)dstWidth) / (double)srcWidth; // >1 means zoom in, <1 means zoom out
    double heightRate = ((double)dstHeight) / (double)srcHeight; // >1 means zoom in, <1 means zoom out

    /*
     * 根据缩放倍数分别处理
     * Separate processing according to zoom factor
     */
    if (widthRate > rateMax || widthRate < rateMin ||
        heightRate > rateMax || heightRate < rateMin) {
        /*
         * 缩放倍数超过一次VGS的最大值时，递归处理 ...
         * When the zoom factor exceeds the maximum value of one VGS, recursive processing...
         */
        uint32_t midWidth = (uint32_t)IntZoomTo((int)srcWidth, widthRate, rateMin, rateMax);
        uint32_t midHeight = (uint32_t)IntZoomTo((int)srcHeight, heightRate, rateMin, rateMax);
        /*
         * 确保为偶数。为奇数时，放大则减一，否则加一
         *
         * Make sure it is an even number. When it is an odd number,
         * the zoom is reduced by one, otherwise it is increased by one
         */
        if (midWidth % HI_OVEN_BASE) {
            midWidth += widthRate > 1 ? -1 : 1;
        }
        if (midHeight % HI_OVEN_BASE) {
            midHeight += heightRate > 1 ? -1 : 1;
        }

        SAMPLE_PRT("@@@ multi-lev vgs resize, src={%u, %u}, mid={%u, %u}, dst={%u, %u}, rate={%.4f, %.4f}\n",
            srcWidth, srcHeight, midWidth, midHeight, dstWidth, dstHeight, widthRate, heightRate);

        /*
         * 缩放一次
         * Zoom once
         */
        VIDEO_FRAME_INFO_S midFrm;
        ret = VgsResizeOnce(src, &midFrm, midWidth, midHeight);
        if (ret != 0) {
            SAMPLE_PRT("VgsResizeOnce(dw=%u, dh=%u) FAIL\n", midWidth, midHeight);
            return ret;
        }

        /*
         * 以midFrm为src递归调用
         * Recursively call with midFrm as src
         */
        ret = MppFrmResize(&midFrm, dst, dstWidth, dstHeight);
        MppFrmDestroy(&midFrm);
        if (ret != 0) {
            SAMPLE_PRT("sub call MppFrmResize(dw=%u, dh=%u) FAIL\n", dstWidth, dstHeight);
            return ret;
        }
    } else {
        /*
         * 缩放倍数未超过一次VGS的最大值，直接完成
         * The zoom factor does not exceed the maximum value of VGS once, and it is done directly
         */
        ret = VgsResizeOnce(src, dst, dstWidth, dstHeight);
        if (ret != 0) {
            SAMPLE_PRT("VgsResizeOnce(dw=%u, dh=%u) FAIL\n", dstWidth, dstHeight);
            return ret;
        }
    }
    return ret;
}

/*
 * 将整数向上修整为偶数
 * Round up integers to even numbers
 */
static inline int IntToOven(int x)
{
    if (x % HI_OVEN_BASE == 0) {
        return x;
    } else {
        return x + 1;
    }
}

/*
 * 创建并执行VGS画线任务
 * Create and execute VGS draw lines job
 */
static HI_S32 VgsDrawLines(VIDEO_FRAME_INFO_S *frm, const VGS_DRAW_LINE_S lines[], int lineNum)
{
    VGS_HANDLE jobHnd = -1;
    VGS_TASK_ATTR_S task;
    int ret;

    if (memset_s(&task, sizeof(task), 0, sizeof(task)) != EOK) {
        HI_ASSERT(0);
    }
    task.stImgIn = *frm;
    task.stImgOut = *frm;

    /*
     * 启动一个job
     * Start a job
     */
    ret = HI_MPI_VGS_BeginJob(&jobHnd);
    if (ret != 0) {
        SAMPLE_PRT("HI_MPI_VGS_BeginJob FAIL, ret=%08X\n", ret);
        if (jobHnd >= 0 && HI_MPI_VGS_CancelJob(jobHnd) != HI_SUCCESS) {
            HI_ASSERT(0);
        }
        return ret;
    }
    HI_ASSERT(jobHnd >= 0);

    /*
     * 往一个已经启动的job里添加批量画线task
     * Add a batch line drawing task to an already started job
     */
    ret = HI_MPI_VGS_AddDrawLineTaskArray(jobHnd, &task, lines, lineNum);
    if (ret != 0) {
        SAMPLE_PRT("HI_MPI_VGS_AddDrawLineTaskArray FAIL, ret=%08X\n", ret);
        if (jobHnd >= 0 && HI_MPI_VGS_CancelJob(jobHnd) != HI_SUCCESS) {
            HI_ASSERT(0);
        }
        return ret;
    }

    /*
     * 提交一个job
     * Submit a job
     */
    ret = HI_MPI_VGS_EndJob(jobHnd);
    if (ret != 0) {
        SAMPLE_PRT("HI_MPI_VGS_EndJob FAIL, ret=%08X\n", ret);
        if (jobHnd >= 0 && HI_MPI_VGS_CancelJob(jobHnd) != HI_SUCCESS) {
            HI_ASSERT(0);
        }
        return ret;
    }
    return 0;
}

/*
 * 在frame中叠加一个或多个矩形框
 * Superimpose one or more rectangular boxes in the frame
 */
int MppFrmDrawRects(VIDEO_FRAME_INFO_S *frm,
    const RectBox *boxes, int boxesNum, uint32_t color, int thick)
{
    VGS_DRAW_LINE_S lines[VGS_MAX_LINE];
    int i;

    if (thick <= 0) {
        HI_ASSERT(0);
    }

    /*
     * 将各矩形四边平面化为lines
     * Planarize the four sides of each rectangle into lines
     */
    for (i = 0; i < boxesNum; i++) {
        lines[RECT_LINES * i].stStartPoint.s32X = IntToOven(boxes[i].xmin);
        lines[RECT_LINES * i].stStartPoint.s32Y = IntToOven(boxes[i].ymin);
        lines[RECT_LINES * i].stEndPoint.s32X = IntToOven(boxes[i].xmax);
        lines[RECT_LINES * i].stEndPoint.s32Y = IntToOven(boxes[i].ymin);
        lines[RECT_LINES * i].u32Color = color;
        lines[RECT_LINES * i].u32Thick = thick;
        lines[RECT_LINES * i + RECT_LINE1].stStartPoint.s32X = IntToOven(boxes[i].xmax);
        lines[RECT_LINES * i + RECT_LINE1].stStartPoint.s32Y = IntToOven(boxes[i].ymin);
        lines[RECT_LINES * i + RECT_LINE1].stEndPoint.s32X = IntToOven(boxes[i].xmax);
        lines[RECT_LINES * i + RECT_LINE1].stEndPoint.s32Y = IntToOven(boxes[i].ymax);
        lines[RECT_LINES * i + RECT_LINE1].u32Color = color;
        lines[RECT_LINES * i + RECT_LINE1].u32Thick = thick;
        lines[RECT_LINES * i + RECT_LINE2].stStartPoint.s32X = IntToOven(boxes[i].xmax);
        lines[RECT_LINES * i + RECT_LINE2].stStartPoint.s32Y = IntToOven(boxes[i].ymax);
        lines[RECT_LINES * i + RECT_LINE2].stEndPoint.s32X = IntToOven(boxes[i].xmin);
        lines[RECT_LINES * i + RECT_LINE2].stEndPoint.s32Y = IntToOven(boxes[i].ymax);
        lines[RECT_LINES * i + RECT_LINE2].u32Color = color;
        lines[RECT_LINES * i + RECT_LINE2].u32Thick = thick;
        lines[RECT_LINES * i + RECT_LINE3].stStartPoint.s32X = IntToOven(boxes[i].xmin);
        lines[RECT_LINES * i + RECT_LINE3].stStartPoint.s32Y = IntToOven(boxes[i].ymax);
        lines[RECT_LINES * i + RECT_LINE3].stEndPoint.s32X = IntToOven(boxes[i].xmin);
        lines[RECT_LINES * i + RECT_LINE3].stEndPoint.s32Y = IntToOven(boxes[i].ymin);
        lines[RECT_LINES * i + RECT_LINE3].u32Color = color;
        lines[RECT_LINES * i + RECT_LINE3].u32Thick = thick;
    }
    return VgsDrawLines(frm, lines, i * RECT_LINES);
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
