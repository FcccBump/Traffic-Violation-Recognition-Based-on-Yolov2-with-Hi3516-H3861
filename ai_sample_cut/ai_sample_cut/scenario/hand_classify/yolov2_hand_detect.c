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
#include <errno.h>

#include "sample_comm_nnie.h"
#include "ai_infer_process.h"
#include "sample_media_ai.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#define MODEL_FILE_HAND    "/userdata/models/hand_classify/hand_detect.wk" // darknet framework wk model
#define PIRIOD_NUM_MAX     49 // Logs are printed when the number of targets is detected
#define DETECT_OBJ_MAX     32 // detect max obj

static uintptr_t g_handModel = 0;
//创建模型
static HI_S32 Yolo2FdLoad(uintptr_t* model)
{
    SAMPLE_SVP_NNIE_CFG_S *self = NULL;
    HI_S32 ret;

    ret = Yolo2Create(&self, MODEL_FILE_HAND);
    *model = ret < 0 ? 0 : (uintptr_t)self;
    SAMPLE_PRT("Yolo2FdLoad ret:%d\n", ret);

    return ret;
}
//初始化模型
HI_S32 HandDetectInit()
{
    return Yolo2FdLoad(&g_handModel);
}
//销毁模型
static HI_S32 Yolo2FdUnload(uintptr_t model)
{
    Yolo2Destory((SAMPLE_SVP_NNIE_CFG_S*)model);
    return 0;
}
//退出模型
HI_S32 HandDetectExit()
{
    return Yolo2FdUnload(g_handModel);
}

static HI_S32 HandDetect(uintptr_t model, IVE_IMAGE_S *srcYuv, DetectObjInfo boxs[])
{
    SAMPLE_SVP_NNIE_CFG_S *self = (SAMPLE_SVP_NNIE_CFG_S*)model;
    int objNum;// 存储模型识别一帧图片得到的目标数
    int ret = Yolo2CalImg(self, srcYuv, boxs, DETECT_OBJ_MAX, &objNum);// 在 ai_infer_process.c 中，963
    // 对一帧yuv图片进行推理。 DETECT_OBJ_MAX = 32，定义在前面。 
    if (ret < 0) {
        SAMPLE_PRT("Hand detect Yolo2CalImg FAIL, for cal FAIL, ret:%d\n", ret);
        return ret;
    }

    return objNum;
}

HI_S32 HandDetectCal(IVE_IMAGE_S *srcYuv, DetectObjInfo resArr[])
{
    int ret = HandDetect(g_handModel, srcYuv, resArr);
    return ret;// 返回模型识别一帧图片得到的所有种类的目标数
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
