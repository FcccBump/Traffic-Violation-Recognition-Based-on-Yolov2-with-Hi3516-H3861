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

#include <string.h>
#include <sys/prctl.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include "mpi_vi.h"
#include "mpi_vpss.h"
#include "mpi_venc.h"
#include "mpi_vo.h"
#include "mpi_region.h"
#include "sample_comm_ive.h"
#include "sample_media_ai.h"
#include "vgs_img.h"
#include "osd_img.h"

/* OSD font library */
static const HI_U8 G_FONT_LIB[] __attribute__((aligned(4))) = {
#include "simsunb_16x32.txt"
};

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#define OSD_FONT_WIDTH_DEF      40 // Default font width
#define OSD_FONT_HEIGHT_DEF     40 // The default font height
#define NOASCII_CHARACTER_BYTES  (2) /* Number of bytes occupied by each Chinese character */
#define BYTE_BITS (8)

#define OSD_FONT_ASC
#define OSD_FONT_MOD_W          16
#define OSD_FONT_MOD_H          32
#define X_COORDINATE            100
#define Y_COORDINATE            100
#define BASE_YAER               1900
#define MULTIPLE_NUM            100

/*
 * 全局对象
 * 多个OsdSet实例将从全局池中分配id
 *
 * Global object.
 * Multiple OsdSet instances will allocate ids from the global pool.
 */
static void* g_osdHndPool[HI_OSD_MAX_CNT]; // Used to identify whether the index handle is used
static pthread_mutex_t g_osdMutex; // pool access lock

/*
 * OSD参数数组
 * OSD Parameter Array、
 */
static OSD_PARAM_S s_stOSDParam[HI_OSD_MAX_CNT];

/*
 * OSD 模块初始化标志
 * 只能被HI_PDT_OSD_Init/HI_PDT_OSD_DeInit修改
 *
 * OSD Module Init Flag
 * Only modified by HI_PDT_OSD_Init/HI_PDT_OSD_DeInit
 */
static HI_BOOL s_bOSDInitFlg = HI_FALSE;

/*
 * OSD 字体库，由 HI_PDT_OSD_Init 初始化
 * OSD Fonts Lib, inited by HI_PDT_OSD_Init
 */
static HI_OSD_FONTS_S s_stOSDFonts;

/*
 * OSD 时间更新运行标志
 * 只能被HI_PDT_OSD_Init/HI_PDT_OSD_DeInit修改
 *
 * OSD Time Update Runing Flag
 * Only modified by HI_PDT_OSD_Init/HI_PDT_OSD_DeInit
 */
static HI_BOOL s_bOSDTimeRun = HI_FALSE;

/*
 * 时间OSD更新任务线程ID，由HI_PDT_OSD_Init创建，由HI_OSD_DeInit销毁
 * Time OSD Update Task Thread ID, created by HI_PDT_OSD_Init, destroyed by HI_OSD_DeInit
 */
static pthread_t s_OSDTimeTskId = 0;

static HI_OSD_TEXTBITMAP_S s_stOSDTextBitMap;

static HI_U8* FontMod = NULL;
static HI_S32 FontModLen = 0;

/*
 * 位图行/列索引
 * Bitmap Row/Col Index
 */
static HI_S32 s32BmRow;
static HI_S32 s32BmCol;
struct tm stTime = {0};

/*
 * OSD Font Step In Lib，以字节为单位
 * OSD Font Step In Lib, in bytes
 */
#define OSD_LIB_FONT_W (s_stOSDFonts.u32FontWidth)
#define OSD_LIB_FONT_H (s_stOSDFonts.u32FontHeight)
#define OSD_LIB_FONT_STEP (OSD_LIB_FONT_W * OSD_LIB_FONT_H / BYTE_BITS)

/*
 * 数据对齐
 * Value Align
 */
HI_S32 HiAppcommAlign(HI_S32 value, HI_S32 base)
{
    return (((value) + (base)-1) / (base) * (base));
}

HI_U32 max(HI_U32 a, HI_U32 b)
{
    return (((a) < (b)) ? (b) : (a));
}

HI_U32 min(HI_U32 a, HI_U32 b)
{
    return (((a) > (b)) ? (b) : (a));
}

HI_S32 IsAscii(HI_S32 a)
{
    return (((a) >= 0x00 && (a) <= 0x7F) ? 1 : 0);
}

static HI_S32 OSD_GetNonASCNum(HI_CHAR* string, HI_S32 len)
{
    HI_S32 i;
    HI_S32 n = 0;
    for (i = 0; i < len; i++) {
        if (string[i] == '\0') {
            break;
        }
        if (!IsAscii(string[i])) {
            i++;
            n++;
        }
    }
    return n;
}

/*
 * 创建OsdSet
 * Creat OsdSet
 */
OsdSet* OsdsCreate(HI_OSD_BIND_MOD_E bindMod, HI_U32 modHnd, HI_U32 chnHnd)
{
    OsdSet *self = NULL;

    self = (OsdSet*)malloc(sizeof(*self));
    if (!self) {
        HI_ASSERT(0);
    }
    if (memset_s(self, sizeof(*self), 0, sizeof(*self)) != EOK) {
        HI_ASSERT(0);
    }

    self->bindMod = bindMod;
    self->modHnd = modHnd;
    self->chnHnd = chnHnd;
    return self;
}

/*
 * @brief 获取给定格式的时间字符串
 * @param[in]pstTime : 时间结构，如果为空则获取当前系统时间
 * @param[out]pazStr : 时间字符串缓冲区
 * @param[in]s32Len : 时间字符串缓冲区长度
 *
 * @brief get time string with given format
 * @param[in]pstTime : time struct, get current system time if null
 * @param[out]pazStr : time string buffer
 * @param[in]s32Len : time string buffer length
 */
static HI_VOID OSD_GetTimeStr(struct tm* pstTime, HI_CHAR* pazStr, HI_S32 s32Len)
{
    /*
     * 获取时间
     * Get Time
     */
    time_t nowTime;

    if (!pstTime) {
        time(&nowTime);
        localtime_r(&nowTime, pstTime);
    }

    /*
     * 生成时间字符串
     * Generate Time String
     */
    if (snprintf_s(pazStr, s32Len, s32Len - 1, "%04d-%02d-%02d %02d:%02d:%02d",
        pstTime->tm_year + BASE_YAER, pstTime->tm_mon + 1, pstTime->tm_mday,
        pstTime->tm_hour, pstTime->tm_min, pstTime->tm_sec) < 0) {
        HI_ASSERT(0);
    }

    return;
}

static HI_S32 OSD_RGNDetach(RGN_HANDLE RgnHdl, const HI_OSD_DISP_ATTR_S* pstDispAttr)
{
    HI_S32 s32Ret = HI_SUCCESS;
    MPP_CHN_S stChn;

    stChn.s32DevId = pstDispAttr->ModHdl;
    stChn.s32ChnId = pstDispAttr->ChnHdl;
    switch (pstDispAttr->enBindedMod) {
        case HI_OSD_BINDMOD_VI:
            stChn.enModId = HI_ID_VI;
            break;
        case HI_OSD_BINDMOD_VPSS:
            stChn.enModId = HI_ID_VPSS;
            break;
        case HI_OSD_BINDMOD_AVS:
            stChn.enModId = HI_ID_AVS;
            break;
        case HI_OSD_BINDMOD_VENC:
            stChn.s32DevId = 0;
            stChn.enModId = HI_ID_VENC;
            break;
        case HI_OSD_BINDMOD_VO:
            stChn.enModId = HI_ID_VO;
            break;
        default:
            SAMPLE_PRT("RgnHdl[%d] invalide bind mode [%d]\n", RgnHdl, pstDispAttr->enBindedMod);
            return HI_EINVAL;
    }

    s32Ret = HI_MPI_RGN_DetachFromChn(RgnHdl, &stChn);
    if (s32Ret != HI_SUCCESS) {
        SAMPLE_PRT("HI_MPI_RGN_DetachFromChn fail,RgnHdl[%d] stChn[%d,%d,%d] Error Code: [0x%08X]\n",
            RgnHdl, stChn.enModId, stChn.s32DevId, stChn.s32ChnId, s32Ret);
        return s32Ret;
    }

    return HI_SUCCESS;
}

static HI_S32 OSD_DestroyRGN(RGN_HANDLE RgnHdl, const HI_OSD_ATTR_S* pstAttr)
{
    HI_S32 s32Ret = HI_SUCCESS;
    HI_S32 s32DispIdx = 0;

    for (s32DispIdx = 0; s32DispIdx < pstAttr->u32DispNum ; ++s32DispIdx) {
        if (pstAttr->astDispAttr[s32DispIdx].bShow) {
            OSD_RGNDetach(RgnHdl, &pstAttr->astDispAttr[s32DispIdx]);
        }
    }

    s32Ret = HI_MPI_RGN_Destroy(RgnHdl);
    if (s32Ret != HI_SUCCESS) {
        SAMPLE_PRT("HI_MPI_RGN_Destroy fail, RgnHdl[%d] Error Code: [0x%08X]\n", RgnHdl, s32Ret);
        return s32Ret;
    }

    return HI_SUCCESS;
}

/*
 * 在OsdSet中创建区域
 * Create a region in OsdSet
 */
int OsdsCreateRgn(OsdSet* self)
{
    HI_ASSERT(self);
    int ret = -1;

    MutexLock(&g_osdMutex);
    for (int i = 0; i < HI_OSD_MAX_CNT; i++) {
        if (!g_osdHndPool[i]) {
            g_osdHndPool[i] = self;
            ret = i;
            break;
        }
    }
    MutexUnlock(&g_osdMutex);
    return ret;
}

static HI_S32 OSD_Stop(HI_S32 s32OsdIdx)
{
    HI_S32 s32Ret = HI_SUCCESS;
    OSD_PARAM_S* pstOsdParam = &s_stOSDParam[s32OsdIdx];
	
    if (!pstOsdParam->bOn) {
        return HI_SUCCESS;
    }
	
    s32Ret = OSD_DestroyRGN(s32OsdIdx, &pstOsdParam->stAttr);
    if (s32Ret != HI_SUCCESS) {
        SAMPLE_PRT("OSD_DestroyRGN s32OsdIdx[%d] failed:[0x%08X]\n", s32OsdIdx, s32Ret);
        return s32Ret;
    }

    pstOsdParam->bOn = HI_FALSE;
    return HI_SUCCESS;
}

/*
 * @brief 按索引停止osd
 * @param[in] s32OsdIdx:osd 索引，范围[0,HI_OSD_MAX_CNT)
 * @return 0 成功，非零错误码
 *
 * @brief    stop osd by index.
 * @param[in] s32OsdIdx:osd index, range[0,HI_OSD_MAX_CNT)
 * @return 0 success,non-zero error code.
 */
HI_S32 HI_OSD_Stop(HI_S32 s32OsdIdx)
{
    /*
     * 检查模块初始化与否
     * Check Module Init or not
     */
    HI_ASSERT(HI_TRUE == s_bOSDInitFlg);
    /*
     * 检查输入参数
     * Check Input Param
     */
    HI_ASSERT(s32OsdIdx >= 0);
    HI_ASSERT(HI_OSD_MAX_CNT > s32OsdIdx);

    HI_S32 s32Ret = HI_SUCCESS;
    OSD_PARAM_S* pstOsdParam = &s_stOSDParam[s32OsdIdx];

    pthread_mutex_lock(&pstOsdParam->mutexLock);

    /*
     * 检查OSD Attrbute是否初始化
     * Check OSD Attrbute init or not
     */
    if (!pstOsdParam->bInit) {
        pthread_mutex_unlock(&pstOsdParam->mutexLock);
        return HI_SUCCESS;
    }

    /*
     * 检查OSD是否停止
     * Check OSD stop or not
     */
    if (!pstOsdParam->bOn) {
        pthread_mutex_unlock(&pstOsdParam->mutexLock);
        return HI_SUCCESS;
    }

    s32Ret = OSD_Stop(s32OsdIdx);
    pstOsdParam->stMaxSize.u32Width = 0;
    pstOsdParam->stMaxSize.u32Height= 0;
    pthread_mutex_unlock(&pstOsdParam->mutexLock);
    return s32Ret;
}

/*
 * 销毁OsdSet中指定的区域
 * Destroy the region specified in the OsdSet
 */
void OsdsDestroyRgn(OsdSet* self, int rgnHnd)
{
    HI_ASSERT(self);
    HI_ASSERT(rgnHnd >= 0 && rgnHnd < HI_OSD_MAX_CNT);

    MutexLock(&g_osdMutex);
    HI_ASSERT(g_osdHndPool[rgnHnd] && g_osdHndPool[rgnHnd] == (void*)self);
    g_osdHndPool[rgnHnd] = NULL;
    MutexUnlock(&g_osdMutex);

    HI_OSD_Stop(rgnHnd);
}

/*
 * 销毁OsdSet中的所有区域
 * Destroy all regions in OsdSet
 */
void OsdsClear(OsdSet* self)
{
    MutexLock(&g_osdMutex);
    for (int i = 0; i < HI_OSD_MAX_CNT; i++) {
        if (g_osdHndPool[i] && g_osdHndPool[i] == (void*)self) {
            OsdsDestroyRgn(self, i);
        }
    }
    MutexUnlock(&g_osdMutex);
}

/*
 * 销毁OsdSet
 * Destory OsdSet
 */
void OsdsDestroy(OsdSet* self)
{
    HI_ASSERT(self);
    OsdsClear(self);
    free(self);
}

/*
 * 设置文本区域的属性值
 * Set the attribute value of the text region
 */
int TxtRgnInit(HI_OSD_ATTR_S* rgnAttr, const char* str, uint32_t begX, uint32_t begY, uint32_t color)
{
    HI_ASSERT(rgnAttr);
    if (!str) {
        HI_ASSERT(0);
    }
    // 64：[0,128], the transparency of the text background block, the larger the background block color,
    // the darker the background block color, 0 means no background block, that is, completely transparent
    static const uint32_t bgAlpha = 64;
    // 128:[0,128], the brightness of the text, the larger the value, the brighter the text
    static const uint32_t fgAlpha = 128;

    if (memset_s(rgnAttr, sizeof(*rgnAttr), 0, sizeof(*rgnAttr)) != EOK) {
        HI_ASSERT(0);
    }
    rgnAttr->u32DispNum = 1;
    rgnAttr->astDispAttr[0].bShow = (str && *str) ? HI_TRUE : HI_FALSE;
    rgnAttr->astDispAttr[0].enBindedMod = HI_OSD_BINDMOD_BUTT;
    rgnAttr->astDispAttr[0].ChnHdl = UINT32_MAX;
    rgnAttr->astDispAttr[0].u32BgAlpha = bgAlpha;
    rgnAttr->astDispAttr[0].u32FgAlpha = fgAlpha;
    rgnAttr->astDispAttr[0].enCoordinate = HI_OSD_COORDINATE_ABS_COOR;
    rgnAttr->astDispAttr[0].stStartPos.s32X = begX;
    rgnAttr->astDispAttr[0].stStartPos.s32Y = begY;
    rgnAttr->astDispAttr[0].enAttachDest = ATTACH_JPEG_MAIN;
    rgnAttr->stContent.enType = HI_OSD_TYPE_STRING;
    rgnAttr->stContent.u32Color = color; // ARGB #FFFF0000 Red
    HiStrxfrm(rgnAttr->stContent.szStr, str, sizeof(rgnAttr->stContent.szStr));
    rgnAttr->stContent.stFontSize.u32Width = OSD_FONT_WIDTH_DEF;
    rgnAttr->stContent.stFontSize.u32Height = OSD_FONT_HEIGHT_DEF;
    return 0;
}

static HI_S32 OSD_Ratio2Absolute(MPP_CHN_S stChn, const POINT_S* pstRatioCoor, POINT_S* pstAbsCoor)
{
    HI_S32 s32Ret = HI_SUCCESS;
    SIZE_S stImageSize;

    if (pstRatioCoor->s32X < 0 || pstRatioCoor->s32X > X_COORDINATE ||
        pstRatioCoor->s32Y < 0 || pstRatioCoor->s32Y > Y_COORDINATE) {
        SAMPLE_PRT("invalide Ratio coordinate(%d,%d)\n", pstRatioCoor->s32X, pstRatioCoor->s32Y);
        return HI_EINVAL;
    }
    switch (stChn.enModId) {
        case HI_ID_VI: {
            VI_CHN_ATTR_S stChnAttr;
            s32Ret = HI_MPI_VI_GetChnAttr(stChn.s32DevId, stChn.s32ChnId, &stChnAttr);
            if (s32Ret != HI_SUCCESS) {
                SAMPLE_PRT("HI_MPI_VI_GetChnAttr(%d,%d) fail,Error Code: [0x%08X]\n",
                    stChn.s32DevId, stChn.s32ChnId, s32Ret);
                return s32Ret;
            }
            stImageSize = stChnAttr.stSize;
            break;
        }
        case HI_ID_VPSS: {
            VPSS_CHN_ATTR_S stChnAttr;
            s32Ret = HI_MPI_VPSS_GetChnAttr(stChn.s32DevId, stChn.s32ChnId, &stChnAttr);
            if (s32Ret != HI_SUCCESS) {
                SAMPLE_PRT("HI_MPI_VPSS_GetChnAttr(%d,%d) fail, Error Code: [0x%08X]\n",
                    stChn.s32DevId, stChn.s32ChnId, s32Ret);
                return s32Ret;
            }
            stImageSize.u32Width = stChnAttr.u32Width;
            stImageSize.u32Height = stChnAttr.u32Height;
            break;
        }
        case HI_ID_VO: {
            VO_CHN_ATTR_S stChnAttr;
            s32Ret = HI_MPI_VO_GetChnAttr(stChn.s32DevId, stChn.s32ChnId, &stChnAttr);
            if (s32Ret != HI_SUCCESS) {
                SAMPLE_PRT("HI_MPI_VO_GetChnAttr(%d,%d) fail,Error Code: [0x%08X]\n",
                    stChn.s32DevId, stChn.s32ChnId, s32Ret);
                return s32Ret;
            }
            stImageSize.u32Width = stChnAttr.stRect.u32Width;
            stImageSize.u32Height = stChnAttr.stRect.u32Height;
            break;
        }
        default:
            SAMPLE_PRT("invalide mode id [%d]\n", stChn.enModId);
            return HI_EINVAL;
    }

    // 2: HiAppcommAlign api base param
    pstAbsCoor->s32X = HiAppcommAlign(stImageSize.u32Width * pstRatioCoor->s32X / MULTIPLE_NUM, 2);
    // 2: HiAppcommAlign api base param
    pstAbsCoor->s32Y = HiAppcommAlign(stImageSize.u32Height * pstRatioCoor->s32Y / MULTIPLE_NUM, 2);
    return HI_SUCCESS;
}

static HI_S32 OSD_Update(RGN_HANDLE RgnHdl, const HI_OSD_ATTR_S* pstAttr)
{
    HI_S32 s32Ret = HI_SUCCESS;
    HI_S32 s32DispIdx = 0;
    RGN_CHN_ATTR_S stRgnChnAttr;
    MPP_CHN_S stChn;

    for (s32DispIdx = 0; s32DispIdx < pstAttr->u32DispNum ; ++s32DispIdx) {
        if (!pstAttr->astDispAttr[s32DispIdx].bShow) {
            continue;
        }

        stChn.s32DevId = pstAttr->astDispAttr[s32DispIdx].ModHdl;
        stChn.s32ChnId = pstAttr->astDispAttr[s32DispIdx].ChnHdl;
        switch (pstAttr->astDispAttr[s32DispIdx].enBindedMod) {
            case HI_OSD_BINDMOD_VI:
                stChn.enModId = HI_ID_VI;
                break;
            case HI_OSD_BINDMOD_VPSS:
                stChn.enModId = HI_ID_VPSS;
                break;
            case HI_OSD_BINDMOD_VO:
                stChn.enModId = HI_ID_VO;
                break;
            default:
                SAMPLE_PRT("invalide bind mode [%d]\n", pstAttr->astDispAttr[s32DispIdx].enBindedMod);
                return HI_EINVAL;
        }
        /*
         * 获取区域的通道显示属性
         * Get the channel display properties of the zone
         */
        s32Ret = HI_MPI_RGN_GetDisplayAttr(RgnHdl, &stChn, &stRgnChnAttr);
        SAMPLE_CHECK_EXPR_RET(s32Ret != HI_SUCCESS, s32Ret, "GetDisplayAttr fail, s32Ret:[0x%08X]\n", s32Ret);

        stRgnChnAttr.bShow = pstAttr->astDispAttr[s32DispIdx].bShow;
        POINT_S stStartPos;
        if (pstAttr->astDispAttr[s32DispIdx].enCoordinate == HI_OSD_COORDINATE_RATIO_COOR) {
            s32Ret = OSD_Ratio2Absolute(stChn, &pstAttr->astDispAttr[s32DispIdx].stStartPos, &stStartPos);
            SAMPLE_CHECK_EXPR_RET(s32Ret != HI_SUCCESS, s32Ret, "Ratio2Absolute fail, s32Ret:[0x%08X]\n", s32Ret);
        } else {
            stStartPos = pstAttr->astDispAttr[s32DispIdx].stStartPos;
        }

        if (stRgnChnAttr.enType == OVERLAYEX_RGN) {
            stRgnChnAttr.unChnAttr.stOverlayExChn.stPoint.s32X = stStartPos.s32X;
            stRgnChnAttr.unChnAttr.stOverlayExChn.stPoint.s32Y = stStartPos.s32Y;
            stRgnChnAttr.unChnAttr.stOverlayExChn.u32BgAlpha = pstAttr->astDispAttr[s32DispIdx].u32BgAlpha;
            stRgnChnAttr.unChnAttr.stOverlayExChn.u32FgAlpha = pstAttr->astDispAttr[s32DispIdx].u32FgAlpha;
        } else {
            stRgnChnAttr.unChnAttr.stOverlayChn.stPoint.s32X = stStartPos.s32X;
            stRgnChnAttr.unChnAttr.stOverlayChn.stPoint.s32Y = stStartPos.s32Y;
            stRgnChnAttr.unChnAttr.stOverlayChn.u32BgAlpha = pstAttr->astDispAttr[s32DispIdx].u32BgAlpha;
            stRgnChnAttr.unChnAttr.stOverlayChn.u32FgAlpha = pstAttr->astDispAttr[s32DispIdx].u32FgAlpha;
        }
        /*
         * 设置区域的通道显示属性
         * Sets the channel display properties for the zone
         */
        s32Ret = HI_MPI_RGN_SetDisplayAttr(RgnHdl, &stChn, &stRgnChnAttr);
        SAMPLE_CHECK_EXPR_RET(s32Ret != HI_SUCCESS, s32Ret, "SetDisplayAttr fail, s32Ret:[0x%08X]\n", s32Ret);
    }

    return HI_SUCCESS;
}

HI_VOID OSD_PuBmData_Cal(HI_OSD_CONTENT_S* pstContent, HI_U16* puBmData, HI_S32 s32HexOffset,
    HI_S32 s32BmDataIdx, HI_S32 s32BitOffset)
{
    HI_U8 temp = FontMod[s32HexOffset];
    if ((temp >> ((BYTE_BITS - 1) - s32BitOffset)) & 0x1) {
        puBmData[s32BmDataIdx] = (HI_U16)pstContent->u32Color;
    } else {
        puBmData[s32BmDataIdx] = (HI_U16)pstContent->u32BgColor;
    }

    return;
}

HI_S32 OSD_Bitmap_Cal(HI_OSD_CONTENT_S* pstContent, HI_S32 NonASCNum, HI_U16* puBmData)
{
    HI_S32 NonASCShow = 0;
    for (s32BmCol = 0; s32BmCol < pstContent->stBitmap.u32Width; ++s32BmCol) {
        /*
         * 点的位图数据偏移
         * Bitmap Data Offset for the point
         */
        HI_S32 s32BmDataIdx = s32BmRow * s_stOSDTextBitMap.stCanvasInfo.u32Stride / 2 + s32BmCol;
        /*
         * 文本字符串中的字符索引
         * Character Index in Text String
         */
        HI_S32 s32CharIdx = s32BmCol / pstContent->stFontSize.u32Width;
        HI_S32 s32StringIdx = s32CharIdx+NonASCShow * (NOASCII_CHARACTER_BYTES - 1);
        if (NonASCNum > 0 && s32CharIdx > 0) {
            NonASCShow = OSD_GetNonASCNum(pstContent->szStr, s32StringIdx);
            s32StringIdx = s32CharIdx+NonASCShow * (NOASCII_CHARACTER_BYTES - 1);
        }
        /*
         * 字符中的点行/列索引
         * Point Row/Col Index in Character
         */
        HI_S32 s32CharCol = (s32BmCol - (pstContent->stFontSize.u32Width * s32CharIdx)) *
            OSD_LIB_FONT_W / pstContent->stFontSize.u32Width;
        HI_S32 s32CharRow = s32BmRow * OSD_LIB_FONT_H / pstContent->stFontSize.u32Height;
        HI_S32 s32HexOffset = s32CharRow * OSD_LIB_FONT_W / BYTE_BITS + s32CharCol / BYTE_BITS;
        HI_S32 s32BitOffset = s32CharCol % BYTE_BITS;

        if (s_stOSDFonts.pfnGetFontMod(&pstContent->szStr[s32StringIdx], &FontMod, &FontModLen)
            == HI_SUCCESS) {
            if (FontMod != NULL && s32HexOffset < FontModLen) {
                OSD_PuBmData_Cal(pstContent, puBmData, s32HexOffset, s32BmDataIdx, s32BitOffset);
                continue;
            }
        }
        SAMPLE_PRT("GetFontMod Fail\n");
        return HI_FAILURE;
    }
}

HI_S32 OSD_Generate_Bitmap(RGN_HANDLE RgnHdl, HI_OSD_CONTENT_S* pstContent)
{
    HI_S32 s32Ret;
    HI_S32 s32StrLen = strnlen(pstContent->szStr, HI_OSD_MAX_STR_LEN);
    HI_S32 NonASCNum = OSD_GetNonASCNum(pstContent->szStr, s32StrLen);

    s32Ret = HI_MPI_RGN_GetCanvasInfo(RgnHdl, &s_stOSDTextBitMap.stCanvasInfo);
    SAMPLE_CHECK_EXPR_RET(s32Ret != HI_SUCCESS, s32Ret, "RGN_GetCanvasInfo FAIL, s32Ret=%x\n", s32Ret);

    /*
     * 生成位图
     * Generate Bitmap
     */
    pstContent->stBitmap.u32Width = pstContent->stFontSize.u32Width *
        (s32StrLen - NonASCNum * (NOASCII_CHARACTER_BYTES - 1));
    pstContent->stBitmap.u32Height = pstContent->stFontSize.u32Height;
    HI_U16* puBmData = (HI_U16*)(HI_UL)s_stOSDTextBitMap.stCanvasInfo.u64VirtAddr;

    for (s32BmRow = 0; s32BmRow < pstContent->stBitmap.u32Height; ++s32BmRow) {
        OSD_Bitmap_Cal(pstContent, NonASCNum, puBmData);
        for (s32BmCol = pstContent->stBitmap.u32Width;
            s32BmCol < s_stOSDParam[RgnHdl].stMaxSize.u32Width; ++s32BmCol) {
            HI_S32 s32BmDataIdx = s32BmRow * s_stOSDTextBitMap.stCanvasInfo.u32Stride / 2 + s32BmCol;
            puBmData[s32BmDataIdx] = 0;
        }
    }

    for (s32BmRow = pstContent->stBitmap.u32Height;
        s32BmRow < s_stOSDParam[RgnHdl].stMaxSize.u32Height; ++s32BmRow) {
        for (s32BmCol = 0; s32BmCol < s_stOSDParam[RgnHdl].stMaxSize.u32Width; ++s32BmCol) {
            HI_S32 s32BmDataIdx = s32BmRow * s_stOSDTextBitMap.stCanvasInfo.u32Stride / 2 + s32BmCol;
            puBmData[s32BmDataIdx] = 0;
        }
    }

    return s32Ret;
}

static HI_S32 OSD_UpdateTextBitmap(RGN_HANDLE RgnHdl, HI_OSD_CONTENT_S* pstContent)
{
    HI_S32 s32Ret = HI_SUCCESS;

    OSD_Generate_Bitmap(RgnHdl, pstContent);
    s_stOSDTextBitMap.stCanvasInfo.enPixelFmt = PIXEL_FORMAT_ARGB_1555;
    s_stOSDTextBitMap.stCanvasInfo.stSize.u32Width = pstContent->stBitmap.u32Width;
    s_stOSDTextBitMap.stCanvasInfo.stSize.u32Height = pstContent->stBitmap.u32Height;

    s32Ret = HI_MPI_RGN_UpdateCanvas(RgnHdl);
    SAMPLE_CHECK_EXPR_RET(s32Ret != HI_SUCCESS, s32Ret, "RGN_UpdateCanvas FAIL, s32Ret=%x\n", s32Ret);

    return s32Ret;
}

static HI_S32 OSD_RGNAttach(RGN_HANDLE RgnHdl, const HI_OSD_DISP_ATTR_S* pstDispAttr)
{
    HI_S32 s32Ret = HI_SUCCESS;
    RGN_CHN_ATTR_S stRgnChnAttr;
    MPP_CHN_S stChn;

    stChn.s32DevId = pstDispAttr->ModHdl;
    stChn.s32ChnId = pstDispAttr->ChnHdl;
    memset_s(&stRgnChnAttr, sizeof(RGN_CHN_ATTR_S), 0x0, sizeof(RGN_CHN_ATTR_S));
    stRgnChnAttr.bShow = pstDispAttr->bShow;
    stRgnChnAttr.enType = OVERLAYEX_RGN;
    switch (pstDispAttr->enBindedMod) {
        case HI_OSD_BINDMOD_VI:
            stChn.enModId = HI_ID_VI;
            break;
        case HI_OSD_BINDMOD_VPSS:
            stChn.enModId = HI_ID_VPSS;
            break;
        case HI_OSD_BINDMOD_VO:
            stChn.enModId = HI_ID_VO;
            break;
        default:
            SAMPLE_PRT("RgnHdl[%d] invalide bind mode [%d]\n", RgnHdl, pstDispAttr->enBindedMod);
            return HI_EINVAL;
    }

    POINT_S stStartPos;
    if (pstDispAttr->enCoordinate == HI_OSD_COORDINATE_RATIO_COOR) {
        s32Ret = OSD_Ratio2Absolute(stChn, &pstDispAttr->stStartPos, &stStartPos);
        SAMPLE_CHECK_EXPR_RET(s32Ret != HI_SUCCESS, s32Ret, "Ratio2Absolute FAIL, s32Ret=%x\n", s32Ret);
    } else {
        stStartPos = pstDispAttr->stStartPos;
    }

    if (stRgnChnAttr.enType == OVERLAYEX_RGN) {
        stRgnChnAttr.unChnAttr.stOverlayExChn.stPoint.s32X = stStartPos.s32X;
        stRgnChnAttr.unChnAttr.stOverlayExChn.stPoint.s32Y = stStartPos.s32Y;
        stRgnChnAttr.unChnAttr.stOverlayExChn.u32BgAlpha = pstDispAttr->u32BgAlpha;
        stRgnChnAttr.unChnAttr.stOverlayExChn.u32FgAlpha = pstDispAttr->u32FgAlpha;
        stRgnChnAttr.unChnAttr.stOverlayExChn.u32Layer = 0;
    } else {
        stRgnChnAttr.unChnAttr.stOverlayChn.stPoint.s32X = stStartPos.s32X;
        stRgnChnAttr.unChnAttr.stOverlayChn.stPoint.s32Y = stStartPos.s32Y;
        stRgnChnAttr.unChnAttr.stOverlayChn.u32BgAlpha = pstDispAttr->u32BgAlpha;
        stRgnChnAttr.unChnAttr.stOverlayChn.u32FgAlpha = pstDispAttr->u32FgAlpha;
        stRgnChnAttr.unChnAttr.stOverlayChn.u32Layer = 0;
    }

    s32Ret = HI_MPI_RGN_AttachToChn(RgnHdl, &stChn, &stRgnChnAttr);
    SAMPLE_CHECK_EXPR_RET(s32Ret != HI_SUCCESS, s32Ret, "RGN_AttachToChn FAIL, s32Ret=%x\n", s32Ret);

    return HI_SUCCESS;
}

static HI_S32 OSD_CreateRGN(RGN_HANDLE RgnHdl, const HI_OSD_ATTR_S* pstAttr)
{
    HI_S32 s32Ret = HI_SUCCESS;
    HI_S32 s32DispIdx = 0;
    RGN_ATTR_S stRgnAttr;
    stRgnAttr.enType = OVERLAY_RGN;

    for (s32DispIdx = 0; s32DispIdx < pstAttr->u32DispNum ; ++s32DispIdx) {
        if (pstAttr->astDispAttr[s32DispIdx].enBindedMod != HI_OSD_BINDMOD_VENC) {
            stRgnAttr.enType = OVERLAYEX_RGN;
            break;
        }
    }

    if (stRgnAttr.enType == OVERLAYEX_RGN) {
        stRgnAttr.unAttr.stOverlayEx.enPixelFmt = PIXEL_FORMAT_ARGB_1555;
        stRgnAttr.unAttr.stOverlayEx.u32BgColor = pstAttr->stContent.u32Color;
        stRgnAttr.unAttr.stOverlayEx.stSize.u32Width = pstAttr->stContent.stBitmap.u32Width;
        stRgnAttr.unAttr.stOverlayEx.stSize.u32Height = pstAttr->stContent.stBitmap.u32Height;
        stRgnAttr.unAttr.stOverlayEx.u32CanvasNum =
            (HI_OSD_TYPE_BITMAP == pstAttr->stContent.enType) ? 1 : 2; // 2: u32CanvasNum
    } else {
        stRgnAttr.unAttr.stOverlay.enPixelFmt = PIXEL_FORMAT_ARGB_1555;
        stRgnAttr.unAttr.stOverlay.u32BgColor = pstAttr->stContent.u32Color;
        stRgnAttr.unAttr.stOverlay.stSize.u32Width = pstAttr->stContent.stBitmap.u32Width;
        stRgnAttr.unAttr.stOverlay.stSize.u32Height = pstAttr->stContent.stBitmap.u32Height;
        stRgnAttr.unAttr.stOverlay.u32CanvasNum =
            (HI_OSD_TYPE_BITMAP == pstAttr->stContent.enType) ? 1 : 2; // 2: u32CanvasNum
    }

    s32Ret = HI_MPI_RGN_Create(RgnHdl, &stRgnAttr);
    SAMPLE_CHECK_EXPR_RET(s32Ret != HI_SUCCESS, s32Ret,
        "HI_MPI_RGN_Create fail,RgnHdl[%d] Error Code: [0x%08X]\n", RgnHdl, s32Ret);

    if (pstAttr->stContent.enType == HI_OSD_TYPE_BITMAP) {
        BITMAP_S stBitmap;
        stBitmap.enPixelFormat = PIXEL_FORMAT_ARGB_1555;
        stBitmap.u32Width = pstAttr->stContent.stBitmap.u32Width;
        stBitmap.u32Height = pstAttr->stContent.stBitmap.u32Height;
        stBitmap.pData = pstAttr->stContent.stBitmap.pvData;
        s32Ret = HI_MPI_RGN_SetBitMap(RgnHdl, &stBitmap);
        SAMPLE_CHECK_EXPR_RET(s32Ret != HI_SUCCESS, s32Ret,
            "HI_MPI_RGN_SetBitMap fail,RgnHdl[%d] Error Code: [0x%08X]\n", RgnHdl, s32Ret);
    } else {
        s32Ret = OSD_UpdateTextBitmap(RgnHdl, (HI_OSD_CONTENT_S*)&pstAttr->stContent);
        SAMPLE_CHECK_EXPR_RET(s32Ret != HI_SUCCESS, s32Ret, "UpdateTextBitmap fail, ret=%x\n", s32Ret);
    }

    for (s32DispIdx = 0; s32DispIdx < pstAttr->u32DispNum; ++s32DispIdx) {
        if (pstAttr->astDispAttr[s32DispIdx].bShow) {
            OSD_RGNAttach(RgnHdl, &pstAttr->astDispAttr[s32DispIdx]);
        }
    }

    return HI_SUCCESS;
}

HI_VOID OSD_Update_Relate_Info(HI_S32 s32OsdIdx)
{
    HI_S32 s32Ret = 0;

    for (s32OsdIdx = 0; s32OsdIdx < HI_OSD_MAX_CNT; ++s32OsdIdx) {
        pthread_mutex_lock(&s_stOSDParam[s32OsdIdx].mutexLock);
        if (s_stOSDParam[s32OsdIdx].stAttr.stContent.enType ==
            HI_OSD_TYPE_TIME && s_stOSDParam[s32OsdIdx].bOn) {
            /*
             * 更新OSD时间字符串
             * Update OSD Time String
             */
            OSD_GetTimeStr(&stTime,
                s_stOSDParam[s32OsdIdx].stAttr.stContent.szStr, HI_OSD_MAX_STR_LEN);
            /*
             * 更新OSD文本位图
             * Update OSD Text Bitmap
             */
            s32Ret = OSD_UpdateTextBitmap(s32OsdIdx, &s_stOSDParam[s32OsdIdx].stAttr.stContent);
            if (HI_SUCCESS != s32Ret) {
                pthread_mutex_unlock(&s_stOSDParam[s32OsdIdx].mutexLock);
                SAMPLE_PRT("Update Text Bitmap failed\n");
                continue;
            }
            /*
             * 更新OSD属性
             * Update OSD Attribute
             */
            s32Ret = OSD_Update(s32OsdIdx, &s_stOSDParam[s32OsdIdx].stAttr);
            if (HI_SUCCESS != s32Ret) {
                SAMPLE_PRT("Update Attribute failed\n");
            }
        }
        pthread_mutex_unlock(&s_stOSDParam[s32OsdIdx].mutexLock);
    }

    return;
}

/*
 * @brief 时间 osd 更新任务
 * @param[in]pvParam : 不使用
 * @return 0 成功，非零错误码
 *
 * @brief   time osd update task
 * @param[in]pvParam : nonuse
 * @return 0 success, non-zero error code.
 */
static HI_VOID* OSD_TimeUpdate(HI_VOID* pvParam)
{
    HI_S32 s32Ret = 0;
    HI_S32 s32OsdIdx = 0;
    time_t nowTime = 0;
    time_t lastTime = 0;
    prctl(PR_SET_NAME, __FUNCTION__, 0, 0, 0);

    while (s_bOSDTimeRun) {
        nowTime = time(NULL); // also means time(&nowTime)
        if (nowTime == lastTime) {
            usleep(10000); // 10000:usleep time
            continue;
        } else {
            localtime_r(&nowTime, &stTime);
            OSD_Update_Relate_Info(s32OsdIdx);
            lastTime = nowTime; /* update time */
        }
        usleep(500000); // 500000: usleep time
    }

    return NULL;
}

/*
 * @brief osd模块初始化，例如：创建时间OSD更新任务
 * @param[in] pstFonts:osd 字体库
 * @return 0 成功，非零表示错误码
 *
 * @brief    osd module initialization, eg. create time osd update task.
 * @param[in] pstFonts:osd fonts lib
 * @return 0 success, non-zero error code.
 */
HI_S32 HI_OSD_Init(const HI_OSD_FONTS_S* pstFonts)
{
    if (!(HI_TRUE != s_bOSDInitFlg)) {
        return HI_EINITIALIZED;
    }
    if (pstFonts != NULL) {
        if (!pstFonts->pfnGetFontMod) {
            return HI_EINVAL;
        }
        if (pstFonts->u32FontWidth % BYTE_BITS) {
            SAMPLE_PRT("FontWidth must be a multiple of %d.", BYTE_BITS);
            return HI_EINVAL;
        }
        memcpy_s(&s_stOSDFonts, sizeof(HI_OSD_FONTS_S), pstFonts, sizeof(HI_OSD_FONTS_S));
    } else {
        memset_s(&s_stOSDFonts, sizeof(HI_OSD_FONTS_S), 0, sizeof(HI_OSD_FONTS_S));
    }
    HI_S32 s32Idx = 0;
    HI_S32 s32Ret = HI_SUCCESS;

    /*
     * 初始化OSD参数
     * Init OSD Param
     */
    for (s32Idx = 0; s32Idx < HI_OSD_MAX_CNT; ++s32Idx) {
        pthread_mutex_init(&s_stOSDParam[s32Idx].mutexLock, NULL);
        pthread_mutex_lock(&s_stOSDParam[s32Idx].mutexLock);
        memset_s(&s_stOSDParam[s32Idx], sizeof(OSD_PARAM_S), 0, sizeof(OSD_PARAM_S));
        pthread_mutex_unlock(&s_stOSDParam[s32Idx].mutexLock);
    }

    if (pstFonts != NULL) {
        /*
         * 创建时间OSD更新线程
         * Create Time OSD Update Thread
         */
        s_bOSDTimeRun = HI_TRUE;
        s32Ret = pthread_create(&s_OSDTimeTskId, NULL, OSD_TimeUpdate, NULL);
        if (HI_SUCCESS != s32Ret) {
            SAMPLE_PRT("create OSD_TimeUpdate failed:%s\n", strerror(errno));
            return HI_ENORES;
        }
    }

    s_bOSDInitFlg = HI_TRUE;
    return HI_SUCCESS;
}

/*
 * 初始化OSD字体
 * Initialize OSD font
 */
static int OsdInitFont(HI_CHAR *character, HI_U8 **fontMod, HI_S32 *fontModLen)
{
    static const HI_CHAR baseChar = 0x20;

    /*
     * 获取ASCII字体库中的字体模组
     * Get Font Mod in ASCII Fontlib
     */
    if (!character || !fontMod || !fontModLen) {
        return HI_FAILURE;
    }
    /*
     * 如果参数是ASCII字符则返回true，否则返回NULL(0)
     * Return true if the parameter is an ASCII character, otherwise NULL (0)
     */
    if (!IsAscii(character[0])) {
        return HI_FAILURE;
    }
    HI_U32 offset = (character[0] - baseChar) * (OSD_FONT_MOD_H * OSD_FONT_MOD_W / HI_BYTE_BITS);
    *fontMod = (HI_U8 *)G_FONT_LIB + offset;
    *fontModLen = OSD_FONT_MOD_H * OSD_FONT_MOD_W / HI_BYTE_BITS;
    return HI_SUCCESS;
}

/*
 * 初始化OsdSet库
 * Initialize OsdSet lib
 */
int OsdLibInit(void)
{
    RecurMutexInit(&g_osdMutex);

#   if defined(OSD_FONT_HZK) || defined(OSD_FONT_ASC)
    SAMPLE_PRT("==================success=================\n");
    HI_OSD_FONTS_S stOsdFonts;
    stOsdFonts.u32FontWidth = OSD_FONT_MOD_W;
    stOsdFonts.u32FontHeight = OSD_FONT_MOD_H;
    stOsdFonts.pfnGetFontMod = OsdInitFont;
    return HI_OSD_Init(&stOsdFonts);
#   else
    SAMPLE_PRT("##################success##################\n");
    return HI_OSD_Init(NULL);
#   endif
}

HI_VOID Osd_Param_Config(const HI_OSD_ATTR_S* pstAttr, OSD_PARAM_S* pstOsdParam)
{
    /*
     * 更新属性
     * Update Attribute
     */
    pstOsdParam->stAttr.stContent.u32Color = pstAttr->stContent.u32Color;
    pstOsdParam->stAttr.stContent.u32BgColor = pstAttr->stContent.u32BgColor;

    if (HI_OSD_TYPE_BITMAP != pstAttr->stContent.enType) {
        if (HI_OSD_TYPE_TIME == pstAttr->stContent.enType) {
            /*
             * 时间类型：更新时间字符串
             * Time Type: Update time string
             */
            pstOsdParam->stAttr.stContent.enTimeFmt = pstAttr->stContent.enTimeFmt;
            OSD_GetTimeStr(NULL, ((HI_OSD_ATTR_S*)pstAttr)->stContent.szStr, HI_OSD_MAX_STR_LEN);
        }

        /*
         * 更新字符串
         * Update string
         */
        if (snprintf_s(pstOsdParam->stAttr.stContent.szStr, HI_OSD_MAX_STR_LEN,
            HI_OSD_MAX_STR_LEN - 1, "%s", pstAttr->stContent.szStr) < 0) {
            HI_ASSERT(0);
        }
        
        pstOsdParam->stAttr.stContent.stBitmap.enPixelFormat = PIXEL_FORMAT_ARGB_1555;
        ((HI_OSD_ATTR_S*)pstAttr)->stContent.stBitmap.u32Width =
            pstAttr->stContent.stFontSize.u32Width * strnlen(pstOsdParam->stAttr.stContent.szStr, HI_OSD_MAX_STR_LEN);
        ((HI_OSD_ATTR_S*)pstAttr)->stContent.stBitmap.u32Height= pstAttr->stContent.stFontSize.u32Height;
        pstOsdParam->stAttr.stContent.stFontSize = pstAttr->stContent.stFontSize;
    } else {
        pstOsdParam->stAttr.stContent.stBitmap.enPixelFormat = pstAttr->stContent.stBitmap.enPixelFormat;
        pstOsdParam->stAttr.stContent.stBitmap.u64PhyAddr = pstAttr->stContent.stBitmap.u64PhyAddr;
        pstOsdParam->stAttr.stContent.stBitmap.pvData = pstAttr->stContent.stBitmap.pvData;
    }
    pstOsdParam->stAttr.stContent.stBitmap.u32Width = pstAttr->stContent.stBitmap.u32Width;
    pstOsdParam->stAttr.stContent.stBitmap.u32Height = pstAttr->stContent.stBitmap.u32Height;
    pstOsdParam->stAttr.stContent.enType = pstAttr->stContent.enType;

    return;
}

static HI_S32 OSD_Update_RGN_Content(const HI_OSD_ATTR_S* pstAttr, OSD_PARAM_S* pstOsdParam, HI_S32 s32OsdIdx)
{
    HI_S32 s32Ret = HI_SUCCESS;
    /*
     * 更新RGN内容
     * Update RGN Content
     */
    if (pstAttr->stContent.enType == HI_OSD_TYPE_BITMAP) {
        BITMAP_S stBitmap;
        stBitmap.enPixelFormat = PIXEL_FORMAT_ARGB_1555;
        stBitmap.u32Width = pstAttr->stContent.stBitmap.u32Width;
        stBitmap.u32Height = pstAttr->stContent.stBitmap.u32Height;
        stBitmap.pData = pstAttr->stContent.stBitmap.pvData;
        /*
         * 设置区域位图，即对区域进行位图填充
         * Set the area bitmap, that is, fill the area with a bitmap
         */
        s32Ret = HI_MPI_RGN_SetBitMap(s32OsdIdx, &stBitmap);
        SAMPLE_CHECK_EXPR_GOTO(s32Ret != HI_SUCCESS, FAIL, "HI_MPI_RGN_SetBitMap. s32Ret: 0x%x\n", s32Ret);
    } else {
        /*
         * 时间/字符串类型：更新文本位图
         * Time/String Type: Update text bitmap
         */
        s32Ret = OSD_UpdateTextBitmap(s32OsdIdx, (HI_OSD_CONTENT_S*)&pstOsdParam->stAttr.stContent);
        SAMPLE_CHECK_EXPR_GOTO(s32Ret != HI_SUCCESS, FAIL, "OSD_UpdateTextBitmap fail, err(%#x)\n", s32Ret);
    }

    HI_S32 s32DispIdx = 0;
    for (s32DispIdx = 0; s32DispIdx < pstAttr->u32DispNum ; ++s32DispIdx) {
        if (HI_TRUE == pstAttr->astDispAttr[s32DispIdx].bShow && HI_FALSE ==
            pstOsdParam->stAttr.astDispAttr[s32DispIdx].bShow) {
            OSD_RGNAttach(s32OsdIdx, &pstAttr->astDispAttr[s32DispIdx]);
        } else if (HI_FALSE == pstAttr->astDispAttr[s32DispIdx].bShow && HI_TRUE ==
            pstOsdParam->stAttr.astDispAttr[s32DispIdx].bShow) {
            OSD_RGNDetach(s32OsdIdx, &pstOsdParam->stAttr.astDispAttr[s32DispIdx]);
        }
    }
    s32Ret = OSD_Update(s32OsdIdx, pstAttr);
    SAMPLE_CHECK_EXPR_GOTO(s32Ret != HI_SUCCESS, FAIL, "OSD_Update fail, err(%#x)\n", s32Ret);

    return s32Ret;

FAIL:
    pthread_mutex_unlock(&pstOsdParam->mutexLock);
    return s32Ret;
}

/*
 * @brief 设置osd属性
 * @param[in] s32OsdIdx:osd索引，范围[0,HI_OSD_MAX_CNT)
 * @param[in] pstAttr:osd配置属性
 * @return 0 成功，非零错误码
 *
 * @brief    set osd attribute.
 * @param[in] s32OsdIdx:osd index, range[0,HI_OSD_MAX_CNT)
 * @param[in] pstAttr:osd configure attribute
 * @return 0 success,non-zero error code.
 */
HI_S32 HI_OSD_SetAttr(HI_S32 s32OsdIdx, const HI_OSD_ATTR_S* pstAttr)
{
    /*
     * 检查模块是否初始化成功
     * Check Module Init or not
     */
    HI_ASSERT(HI_TRUE == s_bOSDInitFlg);
    /*
     * 检查输入的参数
     * Check Input Param
     */
    HI_ASSERT(s32OsdIdx >= 0);
    HI_ASSERT(HI_OSD_MAX_CNT > s32OsdIdx);
    HI_ASSERT(pstAttr);
    HI_ASSERT(HI_OSD_MAX_DISP_CNT >= pstAttr->u32DispNum);

    if (HI_OSD_TYPE_BITMAP == pstAttr->stContent.enType) {
        if (!pstAttr->stContent.stBitmap.pvData) {
            return HI_FAILURE;
        }
    } else {
        if (s_stOSDFonts.pfnGetFontMod == NULL) {
            SAMPLE_PRT("The font library is not registered,only support bitmaps OSD\n");
            return HI_FAILURE;
        }
    }

    HI_S32 s32Ret = HI_SUCCESS;
    OSD_PARAM_S* pstOsdParam = &s_stOSDParam[s32OsdIdx];
    pthread_mutex_lock(&pstOsdParam->mutexLock);
    HI_BOOL bOn = pstOsdParam->bOn;

    /*
     * 更新属性
     * Update Attribute
     */
    Osd_Param_Config(pstAttr, pstOsdParam);
    if (bOn) {
        if (pstOsdParam->stMaxSize.u32Width < pstOsdParam->stAttr.stContent.stBitmap.u32Width
            || pstOsdParam->stMaxSize.u32Height < pstOsdParam->stAttr.stContent.stBitmap.u32Height) {
            SAMPLE_PRT("RGN(%d) size increase[%d,%d->%d,%d], rebuild\n", s32OsdIdx,
                pstOsdParam->stMaxSize.u32Width, pstOsdParam->stMaxSize.u32Height,
                pstAttr->stContent.stBitmap.u32Width, pstAttr->stContent.stBitmap.u32Height);
            /*
             * 销毁区域
             * Destory region
             */
            s32Ret = OSD_DestroyRGN(s32OsdIdx, &pstOsdParam->stAttr);
            if (s32Ret != HI_SUCCESS) {
                pthread_mutex_unlock(&pstOsdParam->mutexLock);
                return s32Ret;
            }

            s32Ret = OSD_CreateRGN(s32OsdIdx, pstAttr);
            if (s32Ret != HI_SUCCESS) {
                pthread_mutex_unlock(&pstOsdParam->mutexLock);
                return s32Ret;
            }
        } else {
            OSD_Update_RGN_Content(pstAttr, pstOsdParam, s32OsdIdx);
        }
    }

    memcpy_s(pstOsdParam->stAttr.astDispAttr, sizeof(HI_OSD_DISP_ATTR_S) * HI_OSD_MAX_DISP_CNT,
        pstAttr->astDispAttr, sizeof(HI_OSD_DISP_ATTR_S) * HI_OSD_MAX_DISP_CNT);
    pstOsdParam->stAttr.u32DispNum = pstAttr->u32DispNum;
    pstOsdParam->stMaxSize.u32Width =
        max(pstOsdParam->stMaxSize.u32Width, pstOsdParam->stAttr.stContent.stBitmap.u32Width);
    pstOsdParam->stMaxSize.u32Height=
        max(pstOsdParam->stMaxSize.u32Height, pstOsdParam->stAttr.stContent.stBitmap.u32Height);
    pstOsdParam->bInit = HI_TRUE;
    pthread_mutex_unlock(&pstOsdParam->mutexLock);

    return HI_SUCCESS;
}

static HI_S32 OSD_Start(HI_S32 s32OsdIdx)
{
    HI_S32 s32Ret = HI_SUCCESS;
    OSD_PARAM_S* pstOsdParam = &s_stOSDParam[s32OsdIdx];

    /*
     * 时间OSD：更新时间字符串和位图
     * Time OSD: Update time string and bitmap
     */
    if (HI_OSD_TYPE_TIME == pstOsdParam->stAttr.stContent.enType) {
        OSD_GetTimeStr(NULL, pstOsdParam->stAttr.stContent.szStr, HI_OSD_MAX_STR_LEN);
    }

    s32Ret = OSD_CreateRGN(s32OsdIdx, &pstOsdParam->stAttr);
    if (HI_SUCCESS != s32Ret) {
        SAMPLE_PRT("OSD_CreateRGN s32OsdIdx[%d] failed:[0x%08X]\n", s32OsdIdx, s32Ret);
        return s32Ret;
    }
    pstOsdParam->bOn = HI_TRUE;
    return HI_SUCCESS;
}

/*
 * @brief 按索引启动 osd
 * @param[in] s32OsdIdx:osd 索引，范围[0,HI_OSD_MAX_CNT)
 * @return 0 成功，非零错误码
 *
 * @brief    start osd by index.
 * @param[in] s32OsdIdx:osd index, range[0,HI_OSD_MAX_CNT)
 * @return 0 success,non-zero error code.
 */
HI_S32 HI_OSD_Start(HI_S32 s32OsdIdx)
{
    /*
     * 检查模块是否初始化成功
     * Check Module Init or not
     */
    HI_ASSERT(HI_TRUE == s_bOSDInitFlg);
    /*
     * 检查输入的参数
     * Check Input Param
     */
    HI_ASSERT(s32OsdIdx >= 0);
    HI_ASSERT(HI_OSD_MAX_CNT > s32OsdIdx);

    HI_S32 s32Ret = HI_SUCCESS;
    OSD_PARAM_S* pstOsdParam = &s_stOSDParam[s32OsdIdx];

    pthread_mutex_lock(&pstOsdParam->mutexLock);

    /*
     * 检查OSD属性是否初始化
     * Check OSD Attrbute init or not
     */
    if (!pstOsdParam->bInit) {
        pthread_mutex_unlock(&pstOsdParam->mutexLock);
        SAMPLE_PRT("OSD[%d] not init yet!\n", s32OsdIdx);
        return HI_EINVAL;
    }

    /*
     * 检查OSD是否停止
     * Check OSD stop or not
     */
    if (pstOsdParam->bOn) {
        pthread_mutex_unlock(&pstOsdParam->mutexLock);
        SAMPLE_PRT("OSD[%d] has already started!\n", s32OsdIdx);
        return HI_SUCCESS;
    }

    s32Ret = OSD_Start(s32OsdIdx);
    pthread_mutex_unlock(&pstOsdParam->mutexLock);
    return s32Ret;
}

/*
 * 在OsdSet中设置指定区域的属性
 * Set attributes for the specified region in OsdSet
 */
int OsdsSetRgn(OsdSet* self, int rgnHnd, const HI_OSD_ATTR_S* rgnAttr)
{
    HI_ASSERT(self);
    HI_ASSERT(rgnHnd >= 0 && rgnHnd < HI_OSD_MAX_CNT);
    HI_ASSERT(g_osdHndPool[rgnHnd] && g_osdHndPool[rgnHnd] == (void*)self); // not need lock
    HI_OSD_ATTR_S attr;
    int ret;

    if (!rgnAttr) { // hidden the region
        if (memset_s(&attr, sizeof(attr), 0, sizeof(attr)) != EOK) {
            HI_ASSERT(0);
        }
        attr.u32DispNum = 1;
        attr.astDispAttr[0].bShow = HI_FALSE;
    } else {
        attr = *rgnAttr;
    }

    attr.astDispAttr[0].enBindedMod = self->bindMod;
    attr.astDispAttr[0].ModHdl = self->modHnd;
    attr.astDispAttr[0].ChnHdl = self->chnHnd;

    ret = HI_OSD_SetAttr(rgnHnd, &attr);
    SAMPLE_CHECK_EXPR_RET(ret, "HI_OSD_SetAttr FAIL, ret=%d\n", ret);

    ret = HI_OSD_Start(rgnHnd);
    SAMPLE_CHECK_EXPR_RET(ret, "HI_OSD_Start FAIL, ret=%d\n", ret);
    return 0;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */