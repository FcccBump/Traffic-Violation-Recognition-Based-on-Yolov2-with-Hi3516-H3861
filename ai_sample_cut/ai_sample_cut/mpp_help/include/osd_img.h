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

#ifndef OSD_IMG_H
#define OSD_IMG_H

#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>
#include "hi_comm_region.h"
#include "hi_common.h"

#if __cplusplus
extern "C" {
#endif

#define HI_OSD_MAX_CNT  (16) // Maximum OSD Count
#define HI_OSD_MAX_DISP_CNT (2) // Maximum Display Count for Each OSD
#define HI_OSD_MAX_STR_LEN  (64) // String OSD Maximum Length

typedef HI_S32 HI_ERRNO;

/*
 * 常见的错误码
 * Common error code
 */
#define HI_ERRNO_COMMON_BASE  0
#define HI_ERRNO_COMMON_COUNT 256

#define HI_EUNKNOWN            (HI_ERRNO)(HI_ERRNO_COMMON_BASE + 1)
#define HI_EOTHER              (HI_ERRNO)(HI_ERRNO_COMMON_BASE + 2)
#define HI_EINTER              (HI_ERRNO)(HI_ERRNO_COMMON_BASE + 3)
#define HI_EVERSION            (HI_ERRNO)(HI_ERRNO_COMMON_BASE + 4)
#define HI_EPAERM              (HI_ERRNO)(HI_ERRNO_COMMON_BASE + 5)
#define HI_EINVAL              (HI_ERRNO)(HI_ERRNO_COMMON_BASE + 6)
#define HI_ENOINIT             (HI_ERRNO)(HI_ERRNO_COMMON_BASE + 7)
#define HI_ENOTREADY           (HI_ERRNO)(HI_ERRNO_COMMON_BASE + 8)
#define HI_ENORES              (HI_ERRNO)(HI_ERRNO_COMMON_BASE + 9)
#define HI_EEXIST              (HI_ERRNO)(HI_ERRNO_COMMON_BASE + 10)
#define HI_ELOST               (HI_ERRNO)(HI_ERRNO_COMMON_BASE + 11)
#define HI_ENOOP               (HI_ERRNO)(HI_ERRNO_COMMON_BASE + 12)
#define HI_EBUSY               (HI_ERRNO)(HI_ERRNO_COMMON_BASE + 13)
#define HI_EIDLE               (HI_ERRNO)(HI_ERRNO_COMMON_BASE + 14)
#define HI_EFULL               (HI_ERRNO)(HI_ERRNO_COMMON_BASE + 15)
#define HI_EEMPTY              (HI_ERRNO)(HI_ERRNO_COMMON_BASE + 16)
#define HI_EUNDERFLOW          (HI_ERRNO)(HI_ERRNO_COMMON_BASE + 17)
#define HI_EOVERFLOW           (HI_ERRNO)(HI_ERRNO_COMMON_BASE + 18)
#define HI_EACCES              (HI_ERRNO)(HI_ERRNO_COMMON_BASE + 19)
#define HI_EINTR               (HI_ERRNO)(HI_ERRNO_COMMON_BASE + 20)
#define HI_ECONTINUE           (HI_ERRNO)(HI_ERRNO_COMMON_BASE + 21)
#define HI_EOVER               (HI_ERRNO)(HI_ERRNO_COMMON_BASE + 22)
#define HI_ERRNO_COMMON_BOTTOM (HI_ERRNO)(HI_ERRNO_COMMON_BASE + 23)

/*
 * 自定义错误码
 * Custom error code
 */
#define HI_ERRNO_BASE          (HI_ERRNO)(HI_ERRNO_COMMON_BASE + HI_ERRNO_COMMON_COUNT)
#define HI_EINITIALIZED        (HI_ERRNO)(HI_ERRNO_BASE + 1) // Repeated initialization
#define HI_ERRNO_CUSTOM_BOTTOM (HI_ERRNO)(HI_ERRNO_BASE + 2) // Number of error numbers that have been defined

/*
 * pthread互斥锁
 * pthread mutex lock
 */
static inline void MutexLock(pthread_mutex_t* mutex)
{
    if (pthread_mutex_lock(mutex) != 0) {
        HI_ASSERT(0);
    }
}

/*
 * 解除锁定mutex所指向的互斥锁
 * pthread mutex unlock
 */
static inline void MutexUnlock(pthread_mutex_t* mutex)
{
    if (pthread_mutex_unlock(mutex) != 0) {
        HI_ASSERT(0);
    }
}

typedef HI_S32 (*HI_OSD_GETFONTMOD_CALLBACK_FN_PTR)(HI_CHAR* Character, HI_U8** FontMod, HI_S32* FontModLen);

/*
 * OSD字体库
 * OSD Fonts Lib
 */
typedef struct hiOSD_FONTS_S {
    /* OSD Lib Font Size, in pixel */
    HI_U32 u32FontWidth;
    HI_U32 u32FontHeight;
    HI_OSD_GETFONTMOD_CALLBACK_FN_PTR pfnGetFontMod;
} HI_OSD_FONTS_S;

/*
 * OSD像素格式枚举
 * OSD pixel format enum
 */
typedef enum hiOSD_PIXEL_FMT_E {
    HI_OSD_PIXEL_FMT_RGB1555 = 0,
    HI_OSD_PIXEL_FMT_BUTT
} HI_OSD_PIXEL_FMT_E;

/*
 * OSD位图属性
 * OSD Bitmap Attribute
 */
typedef struct hiOSD_BITMAP_ATTR_S {
    HI_OSD_PIXEL_FMT_E enPixelFormat;
    HI_U32             u32Width;
    HI_U32             u32Height;
    HI_U64             u64PhyAddr;
    HI_VOID*           pvData;
} HI_OSD_BITMAP_ATTR_S;

/*
 * OSD类型枚举
 * OSD Type Enum
 */
typedef enum hiOSD_TYPE_E {
    HI_OSD_TYPE_TIME = 0,
    HI_OSD_TYPE_STRING,
    HI_OSD_TYPE_BITMAP,
    HI_OSD_TYPE_BUTT
} HI_OSD_TYPE_E;

/*
 * OSD时间格式枚举
 * OSD Time Format Enum
 */
typedef enum hiOSD_TIMEFMT_E {
    HI_OSD_TIMEFMT_YMD24H = 0, // eg. 2017-03-10 23:00:59
    HI_OSD_TIMEFMT_BUTT
} HI_OSD_TIMEFMT_E;

/*
 * OSD绑定模块枚举
 * OSD Binded Module enum
 */
typedef enum hiOSD_BIND_MOD_E {
    HI_OSD_BINDMOD_VI = 0,
    HI_OSD_BINDMOD_VPSS,
    HI_OSD_BINDMOD_AVS,
    HI_OSD_BINDMOD_VENC,
    HI_OSD_BINDMOD_VO,
    HI_OSD_BINDMOD_BUTT
} HI_OSD_BIND_MOD_E;

typedef enum hiOSD_COORDINATE_E {
    HI_OSD_COORDINATE_RATIO_COOR = 0,
    HI_OSD_COORDINATE_ABS_COOR
} HI_OSD_COORDINATE_E;

/*
 * OSD显示属性
 * OSD Display Attribute
 */
typedef struct hiOSD_DISP_ATTR_S {
    HI_BOOL bShow;
    HI_OSD_BIND_MOD_E enBindedMod;
    HI_HANDLE ModHdl;
    HI_HANDLE ChnHdl;
    HI_U32  u32FgAlpha;
    HI_U32  u32BgAlpha;
    HI_OSD_COORDINATE_E enCoordinate; // Coordinate mode of the osd start Position
    POINT_S stStartPos; // OSD Start Position
    ATTACH_DEST_E enAttachDest; // only for venc
    HI_S32 s32Batch;
} HI_OSD_DISP_ATTR_S;

/*
 * OSD内容
 * OSD Content
 */
typedef struct hiOSD_CONTENT_S {
    HI_OSD_TYPE_E enType;
    HI_OSD_TIMEFMT_E enTimeFmt;
    HI_U32  u32Color; // string color
    HI_U32  u32BgColor;
    HI_CHAR szStr[HI_OSD_MAX_STR_LEN];
    SIZE_S  stFontSize;
    HI_OSD_BITMAP_ATTR_S stBitmap; // Pixel Format: Only Support RGB1555 for now
} HI_OSD_CONTENT_S;

/*
 * OSD属性
 * OSD attribution
 */
typedef struct hiOSD_ATTR_S {
    HI_U32 u32DispNum; /* 1Binded Display Number for this OSD */
    HI_OSD_DISP_ATTR_S astDispAttr[HI_OSD_MAX_DISP_CNT];
    HI_OSD_CONTENT_S stContent;
} HI_OSD_ATTR_S;

/*
 * OSD参数
 * OSD Parameter
 */
typedef struct tagOSD_PARAM_S {
    HI_OSD_ATTR_S stAttr;
    SIZE_S stMaxSize;
    pthread_mutex_t  mutexLock;
    HI_BOOL bInit; /* OSD Attribute Set or not, Canbe modified only HI_OSD_SetAttr */
    HI_BOOL bOn; /* OSD On/Off Flag, Canbe modified only by HI_OSD_Start/HI_OSD_Stop */
} OSD_PARAM_S;

typedef struct hiOSD_TEXTBITMAP_S {
    RGN_HANDLE rgnHdl;
    HI_OSD_CONTENT_S pstContent;
    RGN_CANVAS_INFO_S stCanvasInfo;
} HI_OSD_TEXTBITMAP_S;

/*
 * OSD区域设置
 * OSD region set
 */
struct OsdSet {
    // OSD Binded Module: Static
    HI_OSD_BIND_MOD_E bindMod;
    // Binded Module Handle: Static eg.VcapPipeHdl, VpssHdl, StitchHdl, DispHdl, 0
    HI_U32 modHnd;
    // Binded Channel Handle: Static eg.PipeChnHdl, VPortHdl, StitchPortHdl, WndHdl, VencHdl
    HI_U32 chnHnd;
};

typedef struct OsdSet OsdSet;

/*
 * 在OsdSet中创建区域
 * Create a region in OsdSet
 */
int OsdsCreateRgn(OsdSet* self);

/*
 * 设置文本区域的属性值
 * Set the attribute value of the text region
 */
int TxtRgnInit(HI_OSD_ATTR_S* rgnAttr, const char* str, uint32_t begX, uint32_t begY, uint32_t color);

/*
 * 销毁OsdSet中的所有区域
 * Destroy all regions in OsdSet
 */
void OsdsClear(OsdSet* self);

/*
 * 在OsdSet中设置指定区域的属性
 * Set attributes for the specified region in OsdSet
 */
int OsdsSetRgn(OsdSet* self, int rgnHnd, const HI_OSD_ATTR_S* rgnAttr);

/*
 * 创建OsdSet
 * Creat OsdSet
 */
OsdSet* OsdsCreate(HI_OSD_BIND_MOD_E bindMod, HI_U32 modHnd, HI_U32 chnHnd);

/*
 * 销毁OsdSet
 * Destory OsdSet
 */
void OsdsDestroy(OsdSet* self);

/*
 * 初始化OsdSet库
 * Initialize OsdSet lib
 */
int OsdLibInit(void);

#ifdef __cplusplus
}
#endif
#endif