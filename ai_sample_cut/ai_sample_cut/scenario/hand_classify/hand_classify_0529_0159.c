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

/*
 * 该文件提供了基于yolov2的手部检测以及基于resnet18的手势识别，属于两个wk串行推理。
 * 该文件提供了手部检测和手势识别的模型加载、模型卸载、模型推理以及AI flag业务处理的API接口。
 * 若一帧图像中出现多个手，我们通过算法将最大手作为目标手送分类网进行推理，
 * 并将目标手标记为绿色，其他手标记为红色。
 *
 * This file provides hand detection based on yolov2 and gesture recognition based on resnet18,
 * which belongs to two wk serial inferences. This file provides API interfaces for model loading,
 * model unloading, model reasoning, and AI flag business processing for hand detection
 * and gesture recognition. If there are multiple hands in one frame of image,
 * we use the algorithm to use the largest hand as the target hand for inference,
 * and mark the target hand as green and the other hands as red.
 */

/*
0527:
    2353:
        发现不能使用 SkPair g_stmChn ， #include "posix_help.h" ，不然报错。现在可以了。
0528:
    2037:
        可以实现手势检测到后播放音频，但是会播放两遍。暂时不知道调节哪里。
0529：
    0159：
        可以将yuv图片保存到本地了。

*/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

// 音频相关如下
#include <sys/prctl.h>// PR_SET_NAME
// #include "posix_help.h"// SkPair ， SkPairCreate() ， FdReadMsg()
#include "audio_aac_adp.h"
#include "base_interface.h"
#include "osd_img.h"
// 音频相关如上

// 保存到磁盘相关
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "hi_debug.h"
#include "hi_comm_ive.h"
#include "sample_comm_ive.h"
// 保存到磁盘相关

#include "sample_comm_nnie.h"
#include "sample_media_ai.h"
#include "ai_infer_process.h"
#include "yolov2_hand_detect.h"
#include "vgs_img.h"
#include "ive_img.h"
#include "misc_util.h"
#include "hisignalling.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#define HAND_FRM_WIDTH     640
#define HAND_FRM_HEIGHT    384
#define DETECT_OBJ_MAX     32
#define RET_NUM_MAX        4
#define DRAW_RETC_THICK    2    // Draw the width of the line
#define WIDTH_LIMIT        32
#define HEIGHT_LIMIT       32
#define IMAGE_WIDTH        224  // The resolution of the model IMAGE sent to the classification is 224*224
#define IMAGE_HEIGHT       224
#define MODEL_FILE_GESTURE    "/userdata/models/hand_classify/hand_gesture.wk" // darknet framework wk model

// 音频相关如下
#define SCORE_MAX           4096    // The score corresponding to the maximum probability

#define AUDIO_SCORE        40       // Confidence can be configured by yourself
#define AUDIO_FRAME        14       // Recognize once every 15 frames, can be configured by yourself
#define MULTIPLE_OF_EXPANSION 100   // Multiple of expansion
#define UNKOWN_WASTE          20    // Unkown Waste
#define BUFFER_SIZE           16    // buffer size
#define MIN_OF_BOX            16    // min of box
#define MAX_OF_BOX            240   // max of box
// 音频相关如上

static int biggestBoxIndex;
static IVE_IMAGE_S img;

static DetectObjInfo objs[DETECT_OBJ_MAX] = {0};
static RectBox boxs[DETECT_OBJ_MAX] = {0};
static RectBox objBoxs[DETECT_OBJ_MAX] = {0};
static RectBox remainingBoxs[DETECT_OBJ_MAX] = {0};
static RectBox cnnBoxs[DETECT_OBJ_MAX] = {0}; // Store the results of the classification network
static RecogNumInfo numInfo[RET_NUM_MAX] = {0};
static IVE_IMAGE_S imgIn;
static IVE_IMAGE_S imgDst;
static VIDEO_FRAME_INFO_S frmIn;
static VIDEO_FRAME_INFO_S frmDst;
int uartFd = 0;

// 音频相关如下
static int g_num = 108;
static int g_count = 0;
static HI_BOOL g_bAudioProcessStopSignal = HI_FALSE;
static pthread_t g_audioProcessThread = 0;
static OsdSet* g_osdsTrash = NULL;
static HI_S32 g_osd0Trash = -1;

// static SkPair g_stmChn = {
//     .in = -1,
//     .out = -1
// };

static int global_audio_objNum = 0;
// 音频相关如上

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ 音频相关如下 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */ 
/*
 * 将识别的结果进行音频播放
 * Audio playback of the recognition results
 */
static HI_VOID PlayAudio(const RecogNumInfo items)
{
    if  (g_count < AUDIO_FRAME) {
        g_count++;
        return;
    }

    const RecogNumInfo *item = &items;
    uint32_t score = item->score * MULTIPLE_OF_EXPANSION / SCORE_MAX;// 1800*100/4096
    if ((score > AUDIO_SCORE) && (g_num != item->num)) {// 满足条件才会执行音频播放程序
    // g_num != item->num 说明每种类别只会播放一次声音，实测确实是这样的
        g_num = item->num;
        if (g_num != UNKOWN_WASTE) {
            AudioTest(g_num, -1);// 在 audio_test.c 中，482。 g_num 是识别出来的垃圾类别。
        }
    }
    g_count = 0;
}

static HI_VOID PlayAudio2(const RecogNumInfo items)// 受手势识别模型的输出参数 global_audio_objNum 的控制
{
    if  (g_count < AUDIO_FRAME) {
        g_count++;
        return;
    }

    const RecogNumInfo *item = &items;
    uint32_t score = item->score * MULTIPLE_OF_EXPANSION / SCORE_MAX;// 1800*100/4096
    if ((score > AUDIO_SCORE) && (global_audio_objNum != 0)) {// 满足条件才会执行音频播放程序
        g_num = item->num;
        if (g_num != UNKOWN_WASTE) {
            AudioTest(g_num, -1);// 在 audio_test.c 中，482。 g_num 是识别出来的垃圾类别。
        }
        global_audio_objNum = 0;
    }
    g_count = 0;
}

static HI_VOID* GetAudioFileName(HI_VOID* arg)
{
    RecogNumInfo resBuf = {0};
/*
    typedef struct RecogNumInfo {
    uint32_t num; // Recognized numeric value, 0~9
    uint32_t score; // The credibility score of a number, the value range of which is defined by a specific model
    } RecogNumInfo;
*/
    int ret;

    while (g_bAudioProcessStopSignal == false) {// 因为一直满足循环条件，所以会一直执行 PlayAudio ，
    // 但是还有下层函数，所以不是一直播放
        // ret = FdReadMsg(g_stmChn.in, &resBuf, sizeof(RecogNumInfo));
        // if (ret == sizeof(RecogNumInfo)) {
        //     PlayAudio(resBuf);
        // }
        resBuf.num = 1;resBuf.score = 1800;
        // PlayAudio(resBuf);// 单纯播放指定文件
        PlayAudio2(resBuf);// 受手势识别模型的输出参数 objs 的控制
    }

    return NULL;
}
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ 音频相关如上 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */ 

/*
 * 加载手部检测和手势分类模型
 * Load hand detect and classify model
 */
HI_S32 Yolo2HandDetectResnetClassifyLoad(uintptr_t* model)
{
    SAMPLE_SVP_NNIE_CFG_S *self = NULL;
    HI_S32 ret;
    HI_CHAR audioThreadName[BUFFER_SIZE] = {0};// 音频

    ret = CnnCreate(&self, MODEL_FILE_GESTURE);
    *model = ret < 0 ? 0 : (uintptr_t)self;
    HandDetectInit(); // Initialize the hand detection model
    SAMPLE_PRT("Load hand detect claasify model success\n");
    /*
     * Uart串口初始化
     * Uart open init
     */
    uartFd = UartOpenInit();
    if (uartFd < 0) {
        printf("uart1 open failed\r\n");
    } else {
        printf("uart1 open successed\r\n");
    }

///////////////////////////////////////////////////////// 音频
    if (GetCfgBool("audio_player:support_audio", true)) {
        // ret = SkPairCreate(&g_stmChn);// 在 posix_help.c 中，44。配置输入输出通道为0、1
        HI_ASSERT(ret == 0);
        if (snprintf_s(audioThreadName, BUFFER_SIZE, BUFFER_SIZE - 1, "AudioProcess") < 0) {
            HI_ASSERT(0);
        }
        prctl(PR_SET_NAME, (unsigned long)audioThreadName, 0, 0, 0);// 在 prctl.h 中 #define PR_SET_NAME    15
        //  在 prctl.h 中， int prctl (int, ...);
        ret = pthread_create(&g_audioProcessThread, NULL, GetAudioFileName, NULL);// 创建音频播放的线程
        if (ret != 0) {
            SAMPLE_PRT("audio proccess thread creat fail:%s\n", strerror(ret));
            return ret;
        }
    }
///////////////////////////////////////////////////////// 音频

    return ret;
}

/*
 * 卸载手部检测和手势分类模型
 * Unload hand detect and classify model
 */
HI_S32 Yolo2HandDetectResnetClassifyUnload(uintptr_t model)
{
    CnnDestroy((SAMPLE_SVP_NNIE_CFG_S*)model);
    HandDetectExit(); // Uninitialize the hand detection model
    close(uartFd);
    SAMPLE_PRT("Unload hand detect claasify model success\n");

    return 0;
}

/*
 * 获得最大的手
 * Get the maximum hand
 */
static HI_S32 GetBiggestHandIndex(RectBox boxs[], int detectNum)
{
    HI_S32 handIndex = 0;
    HI_S32 biggestBoxIndex = handIndex;
    HI_S32 biggestBoxWidth = boxs[handIndex].xmax - boxs[handIndex].xmin + 1;
    HI_S32 biggestBoxHeight = boxs[handIndex].ymax - boxs[handIndex].ymin + 1;
    HI_S32 biggestBoxArea = biggestBoxWidth * biggestBoxHeight;

    for (handIndex = 1; handIndex < detectNum; handIndex++) {
        HI_S32 boxWidth = boxs[handIndex].xmax - boxs[handIndex].xmin + 1;
        HI_S32 boxHeight = boxs[handIndex].ymax - boxs[handIndex].ymin + 1;
        HI_S32 boxArea = boxWidth * boxHeight;
        if (biggestBoxArea < boxArea) {
            biggestBoxArea = boxArea;
            biggestBoxIndex = handIndex;
        }
        biggestBoxWidth = boxs[biggestBoxIndex].xmax - boxs[biggestBoxIndex].xmin + 1;
        biggestBoxHeight = boxs[biggestBoxIndex].ymax - boxs[biggestBoxIndex].ymin + 1;
    }

    if ((biggestBoxWidth == 1) || (biggestBoxHeight == 1) || (detectNum == 0)) {
        biggestBoxIndex = -1;
    }

    return biggestBoxIndex;
}

/*
 * 手势识别信息
 * Hand gesture recognition info
 */
static void HandDetectFlag(const RecogNumInfo resBuf)
{
    HI_CHAR *gestureName = NULL;
    switch (resBuf.num) {
        case 0u:
            gestureName = "gesture fist";
            UartSendRead(uartFd, FistGesture); // 拳头手势
            SAMPLE_PRT("----gesture name----:%s\n", gestureName);
            break;
        case 1u:
            gestureName = "gesture indexUp";
            UartSendRead(uartFd, ForefingerGesture); // 食指手势
            SAMPLE_PRT("----gesture name----:%s\n", gestureName);
            break;
        case 2u:
            gestureName = "gesture OK";
            UartSendRead(uartFd, OkGesture); // OK手势
            SAMPLE_PRT("----gesture name----:%s\n", gestureName);
            break;
        case 3u:
            gestureName = "gesture palm";
            UartSendRead(uartFd, PalmGesture); // 手掌手势
            SAMPLE_PRT("----gesture name----:%s\n", gestureName);
            break;
        case 4u:
            gestureName = "gesture yes";
            UartSendRead(uartFd, YesGesture); // yes手势
            SAMPLE_PRT("----gesture name----:%s\n", gestureName);
            break;
        case 5u:
            gestureName = "gesture pinchOpen";
            UartSendRead(uartFd, ForefingerAndThumbGesture); // 食指 + 大拇指
            SAMPLE_PRT("----gesture name----:%s\n", gestureName);
            break;
        case 6u:
            gestureName = "gesture phoneCall";
            UartSendRead(uartFd, LittleFingerAndThumbGesture); // 大拇指 + 小拇指
            SAMPLE_PRT("----gesture name----:%s\n", gestureName);
            break;
        default:
            gestureName = "gesture others";
            UartSendRead(uartFd, InvalidGesture); // 无效值
            SAMPLE_PRT("----gesture name----:%s\n", gestureName);
            break;
    }
    SAMPLE_PRT("hand gesture success\n");
}

/*
 * 手部检测和手势分类推理
 * Hand detect and classify calculation
 */
HI_S32 Yolo2HandDetectResnetClassifyCal(uintptr_t model, VIDEO_FRAME_INFO_S *srcFrm, VIDEO_FRAME_INFO_S *dstFrm)
{
    SAMPLE_SVP_NNIE_CFG_S *self = (SAMPLE_SVP_NNIE_CFG_S*)model;
    HI_S32 resLen = 0;
    int objNum;// 存储模型识别一帧图片得到的目标数
    int ret;
    int num = 0;

// typedef struct hiIVE_IMAGE_S {
//     HI_U64 au64PhyAddr[3];   /* RW;The physical address of the image */
//     HI_U64 au64VirAddr[3];   /* RW;The virtual address of the image */
//     HI_U32 au32Stride[3];    /* RW;The stride of the image */
//     HI_U32 u32Width;         /* RW;The width of the image */
//     HI_U32 u32Height;        /* RW;The height of the image */
//     IVE_IMAGE_TYPE_E enType; /* RW;The type of the image */
// } IVE_IMAGE_S;


    ret = FrmToOrigImg((VIDEO_FRAME_INFO_S*)srcFrm, &img);// 在 ive_img.c 中，52。将帧缩放后的一帧转为初始的 yuv 图片
    // 将 *VIDEO_FRAME_INFO_S 格式转换成 IVE_IMAGE_S 格式，具体是 YUV420/422 ，但是到底是哪一个还不清楚
    // 根据 FrmToOrigImg 函数里面的打印信息可知是 IVE_IMAGE_TYPE_YUV420SP
    SAMPLE_CHECK_EXPR_RET(ret != HI_SUCCESS, ret, "hand detect for YUV Frm to Img FAIL, ret=%#x\n", ret);

    // ====================================== 以下尝试将 img 保存到磁盘 ======================================
    HI_CHAR achDstFileName[IVE_FILE_NAME_LEN];
    if (snprintf_s(achDstFileName, sizeof(achDstFileName), sizeof(achDstFileName) - 1,
        "/userdata/complete_%s.yuv", "resize") < 0) {
        HI_ASSERT(0);
    }
    HI_CHAR* achDstFileName2;
    achDstFileName2=&achDstFileName;
    FILE *pFp;
    // FILE *fopen(const char *restrict filename, const char *restrict mode)
    pFp=fopen(achDstFileName2, "wb");
    int i=0;
    for(i=0; i<10 ; i++){
        printf("开始保存到磁盘 开始保存到磁盘 开始保存到磁盘 开始保存到磁盘 开始保存到磁盘 开始保存到磁盘\n");
    }

    // HI_S32 SAMPLE_COMM_IVE_WriteFile(IVE_IMAGE_S *pstImg, FILE *pFp)
    SAMPLE_COMM_IVE_WriteFile(&img, pFp);// 好像不用 #include "sample_comm_ive.h"
    // // 运行时报错，因为输入的参数：要打开的文件不对，现在可以运行了，但是没有结果。因为输入的参数还是错的，
    // 打印信息：func:SAMPLE_COMM_IVE_WriteFile, line:272, NULL pointer。空指针，输入指针才不会报错，
    // 但是空指针说明输入还是错的，现在对了。fopen 的输入必须是指针，该指针需要指向一个字符数组的首地址

    for(i=0; i<10 ; i++){
        printf("结束保存到磁盘 结束保存到磁盘 结束保存到磁盘 结束保存到磁盘 结束保存到磁盘 结束保存到磁盘\n");
    }
    // ====================================== 以上尝试将 img 保存到磁盘 ======================================

    objNum = HandDetectCal(&img, objs);// 在 yolov2_hand_detect.c ，78。 Send IMG to the detection net for reasoning
    // objs 存储识别到的目标的框的数据，返回值是识别到的手势种类，存储在 objNum 中。
    global_audio_objNum = objNum;
    for (int i = 0; i < objNum; i++) {// 有几个目标就循环几次
        cnnBoxs[i] = objs[i].box;
        RectBox *box = &objs[i].box;
        RectBoxTran(box, HAND_FRM_WIDTH, HAND_FRM_HEIGHT,
            dstFrm->stVFrame.u32Width, dstFrm->stVFrame.u32Height);
        SAMPLE_PRT("yolo2_out: {%d, %d, %d, %d}\n", box->xmin, box->ymin, box->xmax, box->ymax);
        boxs[i] = *box;
    }
    biggestBoxIndex = GetBiggestHandIndex(boxs, objNum);
    SAMPLE_PRT("biggestBoxIndex:%d, objNum:%d\n", biggestBoxIndex, objNum);

    /*
     * 当检测到对象时，在DSTFRM中绘制一个矩形
     * When an object is detected, a rectangle is drawn in the DSTFRM
     */
    if (biggestBoxIndex >= 0) {
        objBoxs[0] = boxs[biggestBoxIndex];
        MppFrmDrawRects(dstFrm, objBoxs, 1, RGB888_GREEN, DRAW_RETC_THICK); // Target hand objnum is equal to 1

        for (int j = 0; (j < objNum) && (objNum > 1); j++) {
            if (j != biggestBoxIndex) {
                remainingBoxs[num++] = boxs[j];
                /*
                 * 其他手objnum等于objnum -1
                 * Others hand objnum is equal to objnum -1
                 */
                MppFrmDrawRects(dstFrm, remainingBoxs, objNum - 1, RGB888_RED, DRAW_RETC_THICK);
            }
        }

        /*
         * 裁剪出来的图像通过预处理送分类网进行推理
         * The cropped image is preprocessed and sent to the classification network for inference
         */
        ret = ImgYuvCrop(&img, &imgIn, &cnnBoxs[biggestBoxIndex]);
        SAMPLE_CHECK_EXPR_RET(ret < 0, ret, "ImgYuvCrop FAIL, ret=%#x\n", ret);

        if ((imgIn.u32Width >= WIDTH_LIMIT) && (imgIn.u32Height >= HEIGHT_LIMIT)) {
            COMPRESS_MODE_E enCompressMode = srcFrm->stVFrame.enCompressMode;
            ret = OrigImgToFrm(&imgIn, &frmIn);
            frmIn.stVFrame.enCompressMode = enCompressMode;
            SAMPLE_PRT("crop u32Width = %d, img.u32Height = %d\n", imgIn.u32Width, imgIn.u32Height);
            ret = MppFrmResize(&frmIn, &frmDst, IMAGE_WIDTH, IMAGE_HEIGHT);
            ret = FrmToOrigImg(&frmDst, &imgDst);
            ret = CnnCalImg(self,  &imgDst, numInfo, sizeof(numInfo) / sizeof((numInfo)[0]), &resLen);
            SAMPLE_CHECK_EXPR_RET(ret < 0, ret, "CnnCalImg FAIL, ret=%#x\n", ret);
            HI_ASSERT(resLen <= sizeof(numInfo) / sizeof(numInfo[0]));
            HandDetectFlag(numInfo[0]);
            MppFrmDestroy(&frmDst);
        }
        IveImgDestroy(&imgIn);
    }

    return ret;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
