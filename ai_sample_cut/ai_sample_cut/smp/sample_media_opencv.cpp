/*
 * Copyright (c) 2022 HiSilicon (Shanghai) Technologies CO., LIMITED.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <csignal>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/prctl.h>

#include "hi_mipi_tx.h"
#include "sdk.h"
#include "sample_comm.h"
#include "ai_infer_process.h"
#include "tennis_detect.h"
#include "vgs_img.h"
#include "base_interface.h"
#include "posix_help.h"
#include "sample_media_ai.h"
#include "sample_media_opencv.h"

using namespace std;

static HI_BOOL s_bOpenCVProcessStopSignal = HI_FALSE;
static pthread_t g_openCVProcessThread = 0;
static int g_opencv = 0;
static AicMediaInfo g_aicTennisMediaInfo = { 0 };
static AiPlugLib g_tennisWorkPlug = {0};
static HI_CHAR tennisDetectThreadName[16] = {0};

/*
 * 设置VI设备信息
 * Set VI device information
 */
static void TennisViCfgSetDev(ViCfg* self, int devId, WDR_MODE_E wdrMode)
{
    HI_ASSERT(self);
    HI_ASSERT((int)wdrMode < WDR_MODE_BUTT);

    self->astViInfo[0].stDevInfo.ViDev = devId;
    self->astViInfo[0].stDevInfo.enWDRMode = wdrMode;
}

/*
 * 设置VI通道
 * Set up the VI channel
 */
static void TennisViCfgSetChn(ViCfg* self, int chnId, PIXEL_FORMAT_E pixFormat,
    VIDEO_FORMAT_E videoFormat, DYNAMIC_RANGE_E dynamicRange)
{
    HI_ASSERT(self);
    HI_ASSERT((int)pixFormat < PIXEL_FORMAT_BUTT);
    HI_ASSERT((int)videoFormat < VIDEO_FORMAT_BUTT);
    HI_ASSERT((int)dynamicRange < DYNAMIC_RANGE_BUTT);

    self->astViInfo[0].stChnInfo.ViChn = chnId;
    self->astViInfo[0].stChnInfo.enPixFormat = pixFormat;
    self->astViInfo[0].stChnInfo.enVideoFormat = videoFormat;
    self->astViInfo[0].stChnInfo.enDynamicRange = dynamicRange;
}

static HI_VOID TennisViPramCfg(HI_VOID)
{
    ViCfgInit(&g_aicTennisMediaInfo.viCfg);
    TennisViCfgSetDev(&g_aicTennisMediaInfo.viCfg, 0, WDR_MODE_NONE);
    ViCfgSetPipe(&g_aicTennisMediaInfo.viCfg, 0, -1, -1, -1);
    g_aicTennisMediaInfo.viCfg.astViInfo[0].stPipeInfo.enMastPipeMode = VI_OFFLINE_VPSS_OFFLINE;
    TennisViCfgSetChn(&g_aicTennisMediaInfo.viCfg, 0, PIXEL_FORMAT_YVU_SEMIPLANAR_420,
        VIDEO_FORMAT_LINEAR, DYNAMIC_RANGE_SDR8);
    g_aicTennisMediaInfo.viCfg.astViInfo[0].stChnInfo.enCompressMode = COMPRESS_MODE_SEG;
}

static HI_VOID TennisStVbParamCfg(VbCfg *self)
{
    memset_s(&g_aicTennisMediaInfo.vbCfg, sizeof(VB_CONFIG_S), 0, sizeof(VB_CONFIG_S));
    // 2: The number of buffer pools that can be accommodated in the entire system
    self->u32MaxPoolCnt              = 2;

    /*
     * 获取一帧图片的buffer大小
     * Get picture buffer size
     */
    g_aicTennisMediaInfo.u32BlkSize = COMMON_GetPicBufferSize(g_aicTennisMediaInfo.stSize.u32Width,
        g_aicTennisMediaInfo.stSize.u32Height, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8, COMPRESS_MODE_SEG, DEFAULT_ALIGN);
    self->astCommPool[0].u64BlkSize  = g_aicTennisMediaInfo.u32BlkSize;
    // 10: Number of cache blocks per cache pool. Value range: (0, 10240]
    self->astCommPool[0].u32BlkCnt   = 10;

    /*
     * 获取raw buffer的大小
     * Get raw buffer size
     */
    g_aicTennisMediaInfo.u32BlkSize = VI_GetRawBufferSize(g_aicTennisMediaInfo.stSize.u32Width,
        g_aicTennisMediaInfo.stSize.u32Height, PIXEL_FORMAT_RGB_BAYER_16BPP, COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    self->astCommPool[1].u64BlkSize  = g_aicTennisMediaInfo.u32BlkSize;
    // 4: Number of cache blocks per cache pool. Value range: (0, 10240]
    self->astCommPool[1].u32BlkCnt   = 4;
}

static HI_VOID TennisVpssParamCfg(HI_VOID)
{
    VpssCfgInit(&g_aicTennisMediaInfo.vpssCfg);
    VpssCfgSetGrp(&g_aicTennisMediaInfo.vpssCfg, 0, NULL,
        g_aicTennisMediaInfo.stSize.u32Width, g_aicTennisMediaInfo.stSize.u32Width);
    g_aicTennisMediaInfo.vpssCfg.grpAttr.enPixelFormat = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    // 1920:AICSTART_VI_OUTWIDTH, 1080: AICSTART_VI_OUTHEIGHT
    VpssCfgAddChn(&g_aicTennisMediaInfo.vpssCfg, 1, NULL, 1920, 1080);
    HI_ASSERT(!g_aicTennisMediaInfo.viSess);
}

static HI_VOID TennisStVoParamCfg(VoCfg *self)
{
    SAMPLE_COMM_VO_GetDefConfig(self);
    self->enDstDynamicRange = DYNAMIC_RANGE_SDR8;

    self->enVoIntfType = VO_INTF_MIPI; /* set VO int type */
    self->enIntfSync = VO_OUTPUT_USER; /* set VO output information */

    self->enPicSize = g_aicTennisMediaInfo.enPicSize;
}

static HI_VOID TennisDetectAiProcess(VIDEO_FRAME_INFO_S frm, VO_LAYER voLayer, VO_CHN voChn)
{
    int ret;
    tennis_detect opencv;
    if (GetCfgBool("tennis_detect_switch:support_tennis_detect", true)) {
        if (g_tennisWorkPlug.model == 0) {
            ret = opencv.TennisDetectLoad(&g_tennisWorkPlug.model);
            if (ret < 0) {
                g_tennisWorkPlug.model = 0;
                SAMPLE_CHECK_EXPR_GOTO(ret < 0, TENNIS_RELEASE, "TennisDetectLoad err, ret=%#x\n", ret);
            }
        }

        VIDEO_FRAME_INFO_S calFrm;
        ret = MppFrmResize(&frm, &calFrm, 640, 480); // 640: FRM_WIDTH, 480: FRM_HEIGHT
        ret = opencv.TennisDetectCal(g_tennisWorkPlug.model, &calFrm, &frm);
        SAMPLE_CHECK_EXPR_GOTO(ret < 0, TENNIS_RELEASE, "TennisDetectCal err, ret=%#x\n", ret);

        ret = HI_MPI_VO_SendFrame(voLayer, voChn, &frm, 0);
        SAMPLE_CHECK_EXPR_GOTO(ret != HI_SUCCESS, TENNIS_RELEASE,
            "HI_MPI_VO_SendFrame err, ret=%#x\n", ret);
        MppFrmDestroy(&calFrm);
    }

    TENNIS_RELEASE:
        ret = HI_MPI_VPSS_ReleaseChnFrame(g_aicTennisMediaInfo.vpssGrp, g_aicTennisMediaInfo.vpssChn0, &frm);
        if (ret != HI_SUCCESS) {
            SAMPLE_PRT("Error(%#x),HI_MPI_VPSS_ReleaseChnFrame failed,Grp(%d) chn(%d)!\n",
                ret, g_aicTennisMediaInfo.vpssGrp, g_aicTennisMediaInfo.vpssChn0);
        }
}

static HI_VOID* GetVpssChnFrameTennisDetect(HI_VOID* arg)
{
    int ret;
    VIDEO_FRAME_INFO_S frm;
    HI_S32 s32MilliSec = 2000;
    VO_LAYER voLayer = 0;
    VO_CHN voChn = 0;

    SAMPLE_PRT("vpssGrp:%d, vpssChn0:%d\n", g_aicTennisMediaInfo.vpssGrp, g_aicTennisMediaInfo.vpssChn0);

    while (HI_FALSE == s_bOpenCVProcessStopSignal) {
        ret = HI_MPI_VPSS_GetChnFrame(g_aicTennisMediaInfo.vpssGrp, g_aicTennisMediaInfo.vpssChn0, &frm, s32MilliSec);
        if (ret != 0) {
            SAMPLE_PRT("HI_MPI_VPSS_GetChnFrame FAIL, err=%#x, grp=%d, chn=%d\n",
                ret, g_aicTennisMediaInfo.vpssGrp, g_aicTennisMediaInfo.vpssChn0);
            ret = HI_MPI_VPSS_ReleaseChnFrame(g_aicTennisMediaInfo.vpssGrp, g_aicTennisMediaInfo.vpssChn0, &frm);
            if (ret != HI_SUCCESS) {
                SAMPLE_PRT("Error(%#x),HI_MPI_VPSS_ReleaseChnFrame failed,Grp(%d) chn(%d)!\n",
                    ret, g_aicTennisMediaInfo.vpssGrp, g_aicTennisMediaInfo.vpssChn0);
            }
            continue;
        }
        SAMPLE_PRT("get vpss frame success, weight:%d, height:%d\n", frm.stVFrame.u32Width, frm.stVFrame.u32Height);

        if (g_opencv == 0) {
            ConfBaseInit("./sample_ai.conf");
            g_opencv++;
        }
        TennisDetectAiProcess(frm, voLayer, voChn);
    }

    return HI_NULL;
}

static HI_VOID PauseDoUnloadTennisModel(HI_VOID)
{
    if (GetCfgBool("tennis_detect_switch:support_tennis_detect", true)) {
        memset_s(&g_tennisWorkPlug, sizeof(g_tennisWorkPlug), 0x00, sizeof(g_tennisWorkPlug));
        ConfBaseExt();
        SAMPLE_PRT("tennis detect exit success\n");
        g_opencv = 0;
    }
}

static HI_S32 TennisDetectAiThreadProcess(HI_VOID)
{
    HI_S32 s32Ret;
    if (snprintf_s(tennisDetectThreadName, sizeof(tennisDetectThreadName),
        sizeof(tennisDetectThreadName) - 1, "OpencvProcess") < 0) {
        HI_ASSERT(0);
    }
    prctl(PR_SET_NAME, (unsigned long)tennisDetectThreadName, 0, 0, 0);
    s32Ret = pthread_create(&g_openCVProcessThread, NULL, GetVpssChnFrameTennisDetect, NULL);

    return s32Ret;
}

/*
 * 将sensor采集到数据显示到液晶屏上，同时创建线程运行网球检测推理计算
 * 视频输入->视频处理子系统->视频输出->显示屏
 *
 * Display the data collected by the sensor on the LCD screen,
 * and at the same time create a thread to run tennis detect reasoning calculations
 * VI->VPSS->VO->MIPI
 */
HI_S32 sample_media_opencv::SAMPLE_MEDIA_TENNIS_DETECT(HI_VOID)
{
    HI_S32 s32Ret;
    HI_S32 fd = 0;

    /*
     * 配置VI参数
     * Config VI parameter
     */
    TennisViPramCfg();

    /*
     * 通过Sensor型号获取enPicSize
     * Obtain enPicSize through the Sensor type
     */
    s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(g_aicTennisMediaInfo.viCfg.astViInfo[0].stSnsInfo.enSnsType,
        &g_aicTennisMediaInfo.enPicSize);
    SAMPLE_CHECK_EXPR_RET(s32Ret != HI_SUCCESS, s32Ret, "get pic size by sensor fail, s32Ret=%#x\n", s32Ret);

    /*
     * 根据enPicSize，得到图片的宽高
     * Get picture size(w*h), according enPicSize
     */
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(g_aicTennisMediaInfo.enPicSize, &g_aicTennisMediaInfo.stSize);
    SAMPLE_PRT("AIC: snsMaxSize=%ux%u\n", g_aicTennisMediaInfo.stSize.u32Width, g_aicTennisMediaInfo.stSize.u32Height);
    SAMPLE_CHECK_EXPR_RET(s32Ret != HI_SUCCESS, s32Ret, "get picture size failed, s32Ret=%#x\n", s32Ret);

    /*
     * 配置VB参数
     * Config VB parameter
     */
    TennisStVbParamCfg(&g_aicTennisMediaInfo.vbCfg);

    /*
     * 视频缓存池初始化以及MPI系统初始化
     * VB init & MPI system init
     */
    s32Ret = SAMPLE_COMM_SYS_Init(&g_aicTennisMediaInfo.vbCfg);
    SAMPLE_CHECK_EXPR_RET(s32Ret != HI_SUCCESS, s32Ret, "system init failed, s32Ret=%#x\n", s32Ret);

    /*
     * 设置VO至MIPI通路，获取MIPI设备
     * Set VO config to MIPI, get MIPI device
     */
    s32Ret = SAMPLE_VO_CONFIG_MIPI(&fd);
    SAMPLE_CHECK_EXPR_GOTO(s32Ret != HI_SUCCESS, EXIT, "CONFIG MIPI FAIL.s32Ret:0x%x\n", s32Ret);

    /*
     * 配置VPSS参数
     * Config VPSS parameter
     */
    TennisVpssParamCfg();
    s32Ret = ViVpssCreate(&g_aicTennisMediaInfo.viSess, &g_aicTennisMediaInfo.viCfg, &g_aicTennisMediaInfo.vpssCfg);
    SAMPLE_CHECK_EXPR_GOTO(s32Ret != HI_SUCCESS, EXIT1, "ViVpss Sess create FAIL, ret=%#x\n", s32Ret);
    g_aicTennisMediaInfo.vpssGrp = 0;
    g_aicTennisMediaInfo.vpssChn0 = 1;

    /*
     * 配置VO参数
     * Config VO parameter
     */
    TennisStVoParamCfg(&g_aicTennisMediaInfo.voCfg);

    /*
     * 启动VO到MIPI lcd通路
     * Start VO to MIPI lcd
     */
    s32Ret = SampleCommVoStartMipi(&g_aicTennisMediaInfo.voCfg);
    SAMPLE_CHECK_EXPR_GOTO(s32Ret != HI_SUCCESS, EXIT1, "start vo FAIL. s32Ret: 0x%x\n", s32Ret);

    /*
     * VPSS绑定VO
     * VPSS bind VO
     */
    s32Ret = SAMPLE_COMM_VPSS_Bind_VO(g_aicTennisMediaInfo.vpssGrp,
        g_aicTennisMediaInfo.vpssChn0, g_aicTennisMediaInfo.voCfg.VoDev, 0);
    SAMPLE_CHECK_EXPR_GOTO(s32Ret != HI_SUCCESS, EXIT2, "vo bind vpss FAIL. s32Ret: 0x%x\n", s32Ret);
    SAMPLE_PRT("vpssGrp:%d, vpssChn:%d\n", g_aicTennisMediaInfo.vpssGrp, g_aicTennisMediaInfo.vpssChn0);

    /*
     * 创建工作线程运行ai
     * Create work thread to run ai
     */
    s32Ret = TennisDetectAiThreadProcess();
    SAMPLE_CHECK_EXPR_RET(s32Ret != HI_SUCCESS, s32Ret, "ai proccess thread creat fail:%s\n", strerror(s32Ret));
    PAUSE();
    s_bOpenCVProcessStopSignal = HI_TRUE;
    /*
     * 等待一个线程结束，线程间同步的操作
     * Waiting for the end of a thread, the operation of synchronization between threads
     */
    pthread_join(g_openCVProcessThread, nullptr);
    g_openCVProcessThread = 0;
    PauseDoUnloadTennisModel();

    SAMPLE_COMM_VPSS_UnBind_VO(g_aicTennisMediaInfo.vpssGrp,
        g_aicTennisMediaInfo.vpssChn0, g_aicTennisMediaInfo.voCfg.VoDev, 0);
    SAMPLE_VO_DISABLE_MIPITx(fd);
    SampleCloseMipiTxFd(fd);
    system("echo 0 > /sys/class/gpio/gpio55/value");

EXIT2:
    SAMPLE_COMM_VO_StopVO(&g_aicTennisMediaInfo.voCfg);
EXIT1:
    VpssStop(&g_aicTennisMediaInfo.vpssCfg);
    SAMPLE_COMM_VI_UnBind_VPSS(g_aicTennisMediaInfo.viCfg.astViInfo[0].stPipeInfo.aPipe[0],
        g_aicTennisMediaInfo.viCfg.astViInfo[0].stChnInfo.ViChn, g_aicTennisMediaInfo.vpssGrp);
    ViStop(&g_aicTennisMediaInfo.viCfg);
    free(g_aicTennisMediaInfo.viSess);
EXIT:
    SAMPLE_COMM_SYS_Exit();
    return s32Ret;
}