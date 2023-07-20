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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/prctl.h>

#include "hi_mipi_tx.h"
#include "sdk.h"
#include "sample_comm.h"
#include "ai_infer_process.h"
#include "cnn_trash_classify.h"
#include "hand_classify.h"
#include "vgs_img.h"
#include "osd_img.h"
#include "posix_help.h"
#include "sample_media_ai.h"

// VPSS旋转相关
// 头文件： hi_comm_vpss.h 、 mpi_vpss.h 。库文件：libmpi.a
#include "hi_comm_vpss.h"
#include "mpi_vpss.h"
#include "hi_comm_video.h" // 用到 ROTATION_E 
// VPSS旋转相关

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

static HI_BOOL g_bAiProcessStopSignal = HI_FALSE;
static HI_U32 g_num = 0;
AicMediaInfo g_aicMediaInfo = { 0 };
static pthread_t g_aiProcessThread = 0;
AiPlugLib g_workPlug = {0};

#define HAND_FRM_WIDTH    640
#define HAND_FRM_HEIGHT   384
#define AI_SAMPLE_CFG_FILE      "./sample_ai.conf"
#define BUFFER_SIZE           16    // buffer size

#define USLEEP_TIME   1000 // 1000: usleep time, in microseconds
#define G_MBUF_LENGTH 50 // 50: length of g_mbuf
#define ALIGN_DOWN_SIZE 2

#define G_MBUF_ARRAY_SUBSCRIPT_0     0
#define G_MBUF_ARRAY_SUBSCRIPT_1     1
#define G_MBUF_ARRAY_SUBSCRIPT_2     2
#define G_MBUF_ARRAY_SUBSCRIPT_3     3
#define G_MBUF_ARRAY_SUBSCRIPT_4     4
#define G_MBUF_ARRAY_SUBSCRIPT_5     5
#define G_MBUF_ARRAY_SUBSCRIPT_6     6
#define G_MBUF_ARRAY_SUBSCRIPT_7     7
#define G_MBUF_ARRAY_SUBSCRIPT_8     8
#define G_MBUF_ARRAY_SUBSCRIPT_9     9
#define G_MBUF_ARRAY_SUBSCRIPT_10    10
#define G_MBUF_ARRAY_SUBSCRIPT_11    11
#define G_MBUF_ARRAY_SUBSCRIPT_12    12
#define G_MBUF_ARRAY_SUBSCRIPT_13    13
#define G_MBUF_ARRAY_SUBSCRIPT_14    14
#define G_MBUF_ARRAY_SUBSCRIPT_15    15
#define G_MBUF_ARRAY_SUBSCRIPT_16    16

#define LANE_ID_SUBSCRIPT_0    0
#define LANE_ID_SUBSCRIPT_1    1
#define LANE_ID_SUBSCRIPT_2    2
#define LANE_ID_SUBSCRIPT_3    3

static unsigned char g_mBuf[G_MBUF_LENGTH];
static SampleVoModeMux g_sampleVoModeMux = {0};
static VO_PUB_ATTR_S stVoPubAttr = {0};
static VO_VIDEO_LAYER_ATTR_S  stLayerAttr    = {0};
static VO_CSC_S               stVideoCSC     = {0};
static RECT_S stDefDispRect  = {0, 0, 800, 480};
static SIZE_S stDefImageSize = {800, 480};

HI_CHAR acThreadName[16] = {0};

typedef struct StSampleUserVoConfigs {
    VO_SYNC_INFO_S stSyncInfo;
    VO_USER_INTFSYNC_ATTR_S stUserIntfSyncAttr;
    HI_U32 u32PreDiv;
    HI_U32 u32DevDiv;
    HI_U32 u32Framerate;
    combo_dev_cfg_t stcombo_dev_cfgl;
} SAMPLE_USER_VO_CONFIG_S;

HI_VOID SAMPLE_VOU_SYS_Exit(void)
{
    HI_MPI_SYS_Exit();
    HI_MPI_VB_Exit();
}

HI_VOID SAMPLE_VO_GetUserLayerAttr(VO_VIDEO_LAYER_ATTR_S *pstLayerAttr, SIZE_S *pstDevSize)
{
    pstLayerAttr->bClusterMode = HI_FALSE;
    pstLayerAttr->bDoubleFrame = HI_FALSE;
    pstLayerAttr->enDstDynamicRange = DYNAMIC_RANGE_SDR8;
    pstLayerAttr->enPixFormat = PIXEL_FORMAT_YVU_SEMIPLANAR_420;

    pstLayerAttr->stDispRect.s32X = 0;
    pstLayerAttr->stDispRect.s32Y = 0;
    pstLayerAttr->stDispRect.u32Height = pstDevSize->u32Height;
    pstLayerAttr->stDispRect.u32Width  = pstDevSize->u32Width;

    pstLayerAttr->stImageSize.u32Height = pstDevSize->u32Height;
    pstLayerAttr->stImageSize.u32Width = pstDevSize->u32Width;

    return;
}

HI_VOID SAMPLE_VO_GetUserChnAttr(VO_CHN_ATTR_S *pstChnAttr, SIZE_S *pstDevSize, HI_S32 VoChnNum)
{
    HI_S32 i;
    for (i = 0; i < VoChnNum; i++) {
        pstChnAttr[i].bDeflicker = HI_FALSE;
        pstChnAttr[i].u32Priority = 0;
        pstChnAttr[i].stRect.s32X = 0;
        pstChnAttr[i].stRect.s32Y = 0;
        pstChnAttr[i].stRect.u32Height = pstDevSize->u32Height;
        pstChnAttr[i].stRect.u32Width = pstDevSize->u32Width;
        }

    return;
}

/*
 * 打开MIPI Tx设备
 * Open MIPI Tx device
 */
HI_S32 SampleOpenMipiTxFd(HI_VOID)
{
    HI_S32 fd;

    fd = open("/dev/hi_mipi_tx", O_RDWR);
    if (fd < 0) {
        printf("open hi_mipi_tx dev failed\n");
    }
    return fd;
}

/*
 * 关闭MIPI Tx设备
 * Close MIPI Tx device
 */
HI_VOID SampleCloseMipiTxFd(HI_S32 fd)
{
    close(fd);
    return;
}

/*
 * 获取MIPI Tx配置信息
 * Get MIPI Tx config information
 */
HI_VOID SAMPLE_GetMipiTxConfig(combo_dev_cfg_t *pstMipiTxConfig)
{
    /*
     * 用户需要设置MIPI设备配置
     * User need set MIPI device config
     */
    pstMipiTxConfig->devno = 0;
    pstMipiTxConfig->lane_id[LANE_ID_SUBSCRIPT_0] = 0;
    pstMipiTxConfig->lane_id[LANE_ID_SUBSCRIPT_1] = 1;
    // -1: 2 lane mode configuration,lane_id[4] = {0, 1, -1, -1}
    pstMipiTxConfig->lane_id[LANE_ID_SUBSCRIPT_2] = -1;
    // -1: 2 lane mode configuration,lane_id[4] = {0, 1, -1, -1}
    pstMipiTxConfig->lane_id[LANE_ID_SUBSCRIPT_3] = -1;
    pstMipiTxConfig->output_mode = OUTPUT_MODE_DSI_VIDEO;
    pstMipiTxConfig->output_format = OUT_FORMAT_RGB_24_BIT;
    pstMipiTxConfig->video_mode = BURST_MODE;
    pstMipiTxConfig->sync_info.vid_pkt_size = 480; // 480: received packet size
    pstMipiTxConfig->sync_info.vid_hsa_pixels = 10; // 10: The number of pixels in the input line sync pulse area
    pstMipiTxConfig->sync_info.vid_hbp_pixels = 50; // 50: Number of pixels in blanking area after input
    pstMipiTxConfig->sync_info.vid_hline_pixels = 590; // 590: The total number of pixels detected per line
    pstMipiTxConfig->sync_info.vid_vsa_lines = 4; // 4: Number of frame sync pulse lines detected
    pstMipiTxConfig->sync_info.vid_vbp_lines = 20; // 20: Number of blanking area lines after frame sync pulse
    pstMipiTxConfig->sync_info.vid_vfp_lines = 20; // 20: Number of blanking area lines before frame sync pulse
    pstMipiTxConfig->sync_info.vid_active_lines = 800; // 800: VACTIVE rows
    pstMipiTxConfig->sync_info.edpi_cmd_size = 0; // 0: Write memory command bytes
    pstMipiTxConfig->phy_data_rate = 359; // 359: MIPI Tx output rate
    pstMipiTxConfig->pixel_clk = 29878; // 29878: pixel clock. The unit is KHz

    return;
}

/*
 * 设置MIPI Tx配置信息
 * Set MIPI Tx config information
 */
HI_S32 SAMPLE_SetMipiTxConfig(HI_S32 fd, combo_dev_cfg_t *pstMipiTxConfig)
{
    HI_S32 s32Ret = ioctl(fd, HI_MIPI_TX_SET_DEV_CFG, pstMipiTxConfig);
    if (s32Ret != HI_SUCCESS) {
        printf("MIPI_TX SET_DEV_CONFIG failed\n");
        SampleCloseMipiTxFd(fd);
        return s32Ret;
    }
    return s32Ret;
}

/*
 * 设置MIPI Tx设备属性
 * Set MIPI Tx device attr
 */
HI_S32 SampleSetMipiTxDevAttr(HI_S32 fd)
{
    HI_S32 s32Ret;
    combo_dev_cfg_t stMipiTxConfig;

    SAMPLE_GetMipiTxConfig(&stMipiTxConfig);
    s32Ret = SAMPLE_SetMipiTxConfig(fd, &stMipiTxConfig);

    return s32Ret;
}

/*
 * 初始化MIPI Tx设备
 * Init MIPI Tx device
 */
HI_S32 SAMPLE_USER_INIT_MIPITx(HI_S32 fd, cmd_info_t *pcmd_info)
{
    HI_S32 s32Ret = ioctl(fd, HI_MIPI_TX_SET_CMD, pcmd_info);
    if (s32Ret !=  HI_SUCCESS) {
        printf("MIPI_TX SET CMD failed\n");
        SampleCloseMipiTxFd(fd);
        return s32Ret;
    }

    return HI_SUCCESS;
}

/*
 * 配置MIPI Tx初始化序列
 * Config MIPI Tx initialization sequence
 */
HI_S32 SampleVoInitMipiTxScreen(HI_S32 fd)
{
    HI_S32 s32Ret;
    cmd_info_t cmd_info;
    memset_s(g_mBuf, G_MBUF_LENGTH, 0, G_MBUF_LENGTH);
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_0] = 0xFF;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_1] = 0x77;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_2] = 0x01;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_3] = 0x00;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_4] = 0x00;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_5] = 0x13;
    cmd_info.devno = 0;
    cmd_info.cmd_size = 6; // 6: command data size
    cmd_info.data_type = 0x29;
    cmd_info.cmd = g_mBuf;
    s32Ret = SAMPLE_USER_INIT_MIPITx(fd, &cmd_info);
    if (s32Ret != HI_SUCCESS) {
        return s32Ret;
    }
    usleep(USLEEP_TIME);

    cmd_info.devno = 0;
    cmd_info.cmd_size = 0x08ef;
    cmd_info.data_type = 0x23;
    cmd_info.cmd = NULL;
    s32Ret = SAMPLE_USER_INIT_MIPITx(fd, &cmd_info);
    if (s32Ret != HI_SUCCESS) {
        return s32Ret;
    }
    usleep(USLEEP_TIME);

    memset_s(g_mBuf, G_MBUF_LENGTH, 0, G_MBUF_LENGTH);
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_0] = 0xFF;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_1] = 0x77;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_2] = 0x01;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_3] = 0x00;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_4] = 0x00;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_5] = 0x10;
    cmd_info.devno = 0;
    cmd_info.cmd_size = 6; // 6: command data size
    cmd_info.data_type = 0x29;
    cmd_info.cmd = g_mBuf;
    s32Ret = SAMPLE_USER_INIT_MIPITx(fd, &cmd_info);
    if (s32Ret != HI_SUCCESS) {
        return s32Ret;
    }
    usleep(USLEEP_TIME);

    memset_s(g_mBuf, G_MBUF_LENGTH, 0, G_MBUF_LENGTH);
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_0] = 0xC0;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_1] = 0x63;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_2] = 0x00;
    cmd_info.devno = 0;
    cmd_info.cmd_size = 3; // 3: command data size
    cmd_info.data_type = 0x29;
    cmd_info.cmd = g_mBuf;
    s32Ret = SAMPLE_USER_INIT_MIPITx(fd, &cmd_info);
    if (s32Ret != HI_SUCCESS) {
        return s32Ret;
    }
    usleep(USLEEP_TIME);

    memset_s(g_mBuf, G_MBUF_LENGTH, 0, G_MBUF_LENGTH);
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_0] = 0xC1;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_1] = 0x10;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_2] = 0x02;
    cmd_info.devno = 0;
    cmd_info.cmd_size = 3; // 3: command data size
    cmd_info.data_type = 0x29;
    cmd_info.cmd = g_mBuf;
    s32Ret = SAMPLE_USER_INIT_MIPITx(fd, &cmd_info);
    if (s32Ret != HI_SUCCESS) {
        return s32Ret;
    }
    usleep(USLEEP_TIME);

    memset_s(g_mBuf, G_MBUF_LENGTH, 0, G_MBUF_LENGTH);
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_0] = 0xC2;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_1] = 0x01;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_2] = 0x08;
    cmd_info.devno = 0;
    cmd_info.cmd_size = 3; // 3: command data size
    cmd_info.data_type = 0x29;
    cmd_info.cmd = g_mBuf;
    s32Ret = SAMPLE_USER_INIT_MIPITx(fd, &cmd_info);
    if (s32Ret != HI_SUCCESS) {
        return s32Ret;
    }
    usleep(USLEEP_TIME);

    cmd_info.devno = 0;
    cmd_info.cmd_size = 0x18CC;
    cmd_info.data_type = 0x23;
    cmd_info.cmd = NULL;
    s32Ret = SAMPLE_USER_INIT_MIPITx(fd, &cmd_info);
    if (s32Ret != HI_SUCCESS) {
        return s32Ret;
    }
    usleep(USLEEP_TIME);

    memset_s(g_mBuf, G_MBUF_LENGTH, 0, G_MBUF_LENGTH);
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_0] = 0xB0;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_1] = 0x40;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_2] = 0xC9;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_3] = 0x8F;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_4] = 0x0D;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_5] = 0x11;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_6] = 0x07;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_7] = 0x02;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_8] = 0x09;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_9] = 0x09;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_10] = 0x1F;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_11] = 0x04;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_12] = 0x50;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_13] = 0x0F;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_14] = 0xE4;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_15] = 0x29;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_16] = 0xDF;
    cmd_info.devno = 0;
    cmd_info.cmd_size = 17; // 17: command data size
    cmd_info.data_type = 0x29;
    cmd_info.cmd = g_mBuf;
    s32Ret = SAMPLE_USER_INIT_MIPITx(fd, &cmd_info);
    if (s32Ret != HI_SUCCESS) {
        return s32Ret;
    }
    usleep(USLEEP_TIME);

    memset_s(g_mBuf, G_MBUF_LENGTH, 0, G_MBUF_LENGTH);
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_0] = 0xB1;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_1] = 0x40;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_2] = 0xCB;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_3] = 0xD3;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_4] = 0x11;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_5] = 0x8F;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_6] = 0x04;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_7] = 0x00;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_8] = 0x08;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_9] = 0x07;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_10] = 0x1C;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_11] = 0x06;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_12] = 0x53;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_13] = 0x12;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_14] = 0x63;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_15] = 0xEB;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_16] = 0xDF;
    cmd_info.devno = 0;
    cmd_info.cmd_size = 17; // 17: command data size
    cmd_info.data_type = 0x29;
    cmd_info.cmd = g_mBuf;
    s32Ret = SAMPLE_USER_INIT_MIPITx(fd, &cmd_info);
    if (s32Ret != HI_SUCCESS) {
        return s32Ret;
    }
    usleep(USLEEP_TIME);

    memset_s(g_mBuf, G_MBUF_LENGTH, 0, G_MBUF_LENGTH);
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_0] = 0xFF;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_1] = 0x77;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_2] = 0x01;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_3] = 0x00;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_4] = 0x00;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_5] = 0x11;
    cmd_info.devno = 0;
    cmd_info.cmd_size = 6; // 6: command data size
    cmd_info.data_type = 0x29;
    cmd_info.cmd = g_mBuf;
    s32Ret = SAMPLE_USER_INIT_MIPITx(fd, &cmd_info);
    if (s32Ret != HI_SUCCESS) {
        return s32Ret;
    }
    usleep(USLEEP_TIME);

    cmd_info.devno = 0;
    cmd_info.cmd_size = 0x65b0;
    cmd_info.data_type = 0x23;
    cmd_info.cmd = NULL;
    s32Ret = SAMPLE_USER_INIT_MIPITx(fd, &cmd_info);
    if (s32Ret != HI_SUCCESS) {
        return s32Ret;
    }
    usleep(USLEEP_TIME);

    cmd_info.devno = 0;
    cmd_info.cmd_size = 0x34b1;
    cmd_info.data_type = 0x23;
    cmd_info.cmd = NULL;
    s32Ret = SAMPLE_USER_INIT_MIPITx(fd, &cmd_info);
    if (s32Ret != HI_SUCCESS) {
        return s32Ret;
    }
    usleep(USLEEP_TIME);

    cmd_info.devno = 0;
    cmd_info.cmd_size = 0x87b2;
    cmd_info.data_type = 0x23;
    cmd_info.cmd = NULL;
    s32Ret = SAMPLE_USER_INIT_MIPITx(fd, &cmd_info);
    if (s32Ret != HI_SUCCESS) {
        return s32Ret;
    }
    usleep(USLEEP_TIME);

    cmd_info.devno = 0;
    cmd_info.cmd_size = 0x80b3;
    cmd_info.data_type = 0x23;
    cmd_info.cmd = NULL;
    s32Ret = SAMPLE_USER_INIT_MIPITx(fd, &cmd_info);
    if (s32Ret != HI_SUCCESS) {
        return s32Ret;
    }
    usleep(USLEEP_TIME);

    cmd_info.devno = 0;
    cmd_info.cmd_size = 0x49b5;
    cmd_info.data_type = 0x23;
    cmd_info.cmd = NULL;
    s32Ret = SAMPLE_USER_INIT_MIPITx(fd, &cmd_info);
    if (s32Ret != HI_SUCCESS) {
        return s32Ret;
    }
    usleep(USLEEP_TIME);

    cmd_info.devno = 0;
    cmd_info.cmd_size = 0x85b7;
    cmd_info.data_type = 0x23;
    cmd_info.cmd = NULL;
    s32Ret = SAMPLE_USER_INIT_MIPITx(fd, &cmd_info);
    if (s32Ret != HI_SUCCESS) {
        return s32Ret;
    }
    usleep(USLEEP_TIME);

    cmd_info.devno = 0;
    cmd_info.cmd_size = 0x20b8;
    cmd_info.data_type = 0x23;
    cmd_info.cmd = NULL;
    s32Ret = SAMPLE_USER_INIT_MIPITx(fd, &cmd_info);
    if (s32Ret != HI_SUCCESS) {
        return s32Ret;
    }
    usleep(USLEEP_TIME);

    cmd_info.devno = 0;
    cmd_info.cmd_size = 0x10b9;
    cmd_info.data_type = 0x23;
    cmd_info.cmd = NULL;
    s32Ret = SAMPLE_USER_INIT_MIPITx(fd, &cmd_info);
    if (s32Ret != HI_SUCCESS) {
        return s32Ret;
    }
    usleep(USLEEP_TIME);

    cmd_info.devno = 0;
    cmd_info.cmd_size = 0x78c1;
    cmd_info.data_type = 0x23;
    cmd_info.cmd = NULL;
    s32Ret = SAMPLE_USER_INIT_MIPITx(fd, &cmd_info);
    if (s32Ret != HI_SUCCESS) {
        return s32Ret;
    }
    usleep(USLEEP_TIME);

    cmd_info.devno = 0;
    cmd_info.cmd_size = 0x78c2;
    cmd_info.data_type = 0x23;
    cmd_info.cmd = NULL;
    s32Ret = SAMPLE_USER_INIT_MIPITx(fd, &cmd_info);
    if (s32Ret != HI_SUCCESS) {
        return s32Ret;
    }
    usleep(USLEEP_TIME);

    cmd_info.devno = 0;
    cmd_info.cmd_size = 0x88d0;
    cmd_info.data_type = 0x23;
    cmd_info.cmd = NULL;
    s32Ret = SAMPLE_USER_INIT_MIPITx(fd, &cmd_info);
    if (s32Ret != HI_SUCCESS) {
        return s32Ret;
    }
    usleep(USLEEP_TIME);
    usleep(100000);  // 100000: The process hangs for a period of time, in microseconds

    memset_s(g_mBuf, G_MBUF_LENGTH, 0, G_MBUF_LENGTH);
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_0] = 0xE0;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_1] = 0x00;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_2] = 0x19;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_3] = 0x02;
    cmd_info.devno = 0;
    cmd_info.cmd_size = 4; // 4: command data size
    cmd_info.data_type = 0x29;
    cmd_info.cmd = g_mBuf;
    s32Ret = SAMPLE_USER_INIT_MIPITx(fd, &cmd_info);
    if (s32Ret != HI_SUCCESS) {
        return s32Ret;
    }
    usleep(USLEEP_TIME);
    usleep(USLEEP_TIME);

    memset_s(g_mBuf, G_MBUF_LENGTH, 0, G_MBUF_LENGTH);
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_0] = 0xE1;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_1] = 0x05;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_2] = 0xA0;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_3] = 0x07;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_4] = 0xA0;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_5] = 0x04;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_6] = 0xA0;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_7] = 0x06;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_8] = 0xA0;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_9] = 0x00;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_10] = 0x44;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_11] = 0x44;
    cmd_info.devno = 0;
    cmd_info.cmd_size = 12; // 12: command data size
    cmd_info.data_type = 0x29;
    cmd_info.cmd = g_mBuf;
    s32Ret = SAMPLE_USER_INIT_MIPITx(fd, &cmd_info);
    if (s32Ret != HI_SUCCESS) {
        return s32Ret;
    }
    usleep(USLEEP_TIME);

    memset_s(g_mBuf, G_MBUF_LENGTH, 0, G_MBUF_LENGTH);
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_0] = 0xE2;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_1] = 0x00;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_2] = 0x00;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_3] = 0x00;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_4] = 0x00;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_5] = 0x00;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_6] = 0x00;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_7] = 0x00;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_8] = 0x00;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_9] = 0x00;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_10] = 0x00;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_11] = 0x00;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_12] = 0x00;
    cmd_info.devno = 0;
    cmd_info.cmd_size = 13; // 13: command data size
    cmd_info.data_type = 0x29;
    cmd_info.cmd = g_mBuf;
    s32Ret = SAMPLE_USER_INIT_MIPITx(fd, &cmd_info);
    if (s32Ret != HI_SUCCESS) {
        return s32Ret;
    }
    usleep(USLEEP_TIME);

    memset_s(g_mBuf, G_MBUF_LENGTH, 0, G_MBUF_LENGTH);
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_0] = 0xE3;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_1] = 0x00;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_2] = 0x00;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_3] = 0x33;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_4] = 0x33;
    cmd_info.devno = 0;
    cmd_info.cmd_size = 5; // 5: command data size
    cmd_info.data_type = 0x29;
    cmd_info.cmd = g_mBuf;
    s32Ret = SAMPLE_USER_INIT_MIPITx(fd, &cmd_info);
    if (s32Ret != HI_SUCCESS) {
        return s32Ret;
    }
    usleep(USLEEP_TIME);

    memset_s(g_mBuf, G_MBUF_LENGTH, 0, G_MBUF_LENGTH);
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_0] = 0xE4;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_1] = 0x44;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_2] = 0x44;
    cmd_info.devno = 0;
    cmd_info.cmd_size = 3; // 3: command data size
    cmd_info.data_type = 0x29;
    cmd_info.cmd = g_mBuf;
    s32Ret = SAMPLE_USER_INIT_MIPITx(fd, &cmd_info);
    if (s32Ret != HI_SUCCESS) {
        return s32Ret;
    }
    usleep(USLEEP_TIME);

    memset_s(g_mBuf, G_MBUF_LENGTH, 0, G_MBUF_LENGTH);
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_0] = 0xE5;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_1] = 0x0D;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_2] = 0x31;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_3] = 0xC8;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_4] = 0xAF;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_5] = 0x0F;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_6] = 0x33;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_7] = 0xC8;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_8] = 0xAF;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_9] = 0x09;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_10] = 0x2D;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_11] = 0xC8;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_12] = 0xAF;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_13] = 0x0B;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_14] = 0x2F;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_15] = 0xC8;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_16] = 0xAF;
    cmd_info.devno = 0;
    cmd_info.cmd_size = 17; // 17: command data size
    cmd_info.data_type = 0x29;
    cmd_info.cmd = g_mBuf;
    s32Ret = SAMPLE_USER_INIT_MIPITx(fd, &cmd_info);
    if (s32Ret != HI_SUCCESS) {
        return s32Ret;
    }
    usleep(USLEEP_TIME);

    memset_s(g_mBuf, G_MBUF_LENGTH, 0, G_MBUF_LENGTH);
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_0] = 0xE6;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_1] = 0x00;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_2] = 0x00;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_3] = 0x33;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_4] = 0x33;
    cmd_info.devno = 0;
    cmd_info.cmd_size = 5; // 5: command data size
    cmd_info.data_type = 0x29;
    cmd_info.cmd = g_mBuf;
    s32Ret = SAMPLE_USER_INIT_MIPITx(fd, &cmd_info);
    if (s32Ret != HI_SUCCESS) {
        return s32Ret;
    }
    usleep(USLEEP_TIME);

    memset_s(g_mBuf, G_MBUF_LENGTH, 0, G_MBUF_LENGTH);
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_0] = 0xE7;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_1] = 0x44;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_2] = 0x44;
    cmd_info.devno = 0;
    cmd_info.cmd_size = 3; // 3: command data size
    cmd_info.data_type = 0x29;
    cmd_info.cmd = g_mBuf;
    s32Ret = SAMPLE_USER_INIT_MIPITx(fd, &cmd_info);
    if (s32Ret != HI_SUCCESS) {
        return s32Ret;
    }
    usleep(USLEEP_TIME);

    memset_s(g_mBuf, G_MBUF_LENGTH, 0, G_MBUF_LENGTH);
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_0] = 0xE8;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_1] = 0x0C;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_2] = 0x30;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_3] = 0xC8;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_4] = 0xAF;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_5] = 0x0E;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_6] = 0x32;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_7] = 0xC8;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_8] = 0xAF;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_9] = 0x08;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_10] = 0x2C;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_11] = 0xC8;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_12] = 0xAF;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_13] = 0x0A;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_14] = 0x2E;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_15] = 0xC8;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_16] = 0xAF;
    cmd_info.devno = 0;
    cmd_info.cmd_size = 17; // 17: command data size
    cmd_info.data_type = 0x29;
    cmd_info.cmd = g_mBuf;
    s32Ret = SAMPLE_USER_INIT_MIPITx(fd, &cmd_info);
    if (s32Ret != HI_SUCCESS) {
        return s32Ret;
    }
    usleep(USLEEP_TIME);

    memset_s(g_mBuf, G_MBUF_LENGTH, 0, G_MBUF_LENGTH);
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_0] = 0xEB;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_1] = 0x02;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_2] = 0x00;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_3] = 0xE4;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_4] = 0xE4;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_5] = 0x44;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_6] = 0x00;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_7] = 0x40;
    cmd_info.devno = 0;
    cmd_info.cmd_size = 8; // 8: command data size
    cmd_info.data_type = 0x29;
    cmd_info.cmd = g_mBuf;
    s32Ret = SAMPLE_USER_INIT_MIPITx(fd, &cmd_info);
    if (s32Ret != HI_SUCCESS) {
        return s32Ret;
    }
    usleep(USLEEP_TIME);

    memset_s(g_mBuf, G_MBUF_LENGTH, 0, G_MBUF_LENGTH);
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_0] = 0xEC;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_1] = 0x3C;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_2] = 0x00;
    cmd_info.devno = 0;
    cmd_info.cmd_size = 3; // 3: command data size
    cmd_info.data_type = 0x29;
    cmd_info.cmd = g_mBuf;
    s32Ret = SAMPLE_USER_INIT_MIPITx(fd, &cmd_info);
    if (s32Ret != HI_SUCCESS) {
        return s32Ret;
    }
    usleep(USLEEP_TIME);

    memset_s(g_mBuf, G_MBUF_LENGTH, 0, G_MBUF_LENGTH);
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_0] = 0xED;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_1] = 0xAB;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_2] = 0x89;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_3] = 0x76;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_4] = 0x54;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_5] = 0x01;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_6] = 0xFF;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_7] = 0xFF;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_8] = 0xFF;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_9] = 0xFF;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_10] = 0xFF;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_11] = 0xFF;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_12] = 0x10;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_13] = 0x45;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_14] = 0x67;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_15] = 0x98;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_16] = 0xBA;
    cmd_info.devno = 0;
    cmd_info.cmd_size = 17; // 17: command data size
    cmd_info.data_type = 0x29;
    cmd_info.cmd = g_mBuf;
    s32Ret = SAMPLE_USER_INIT_MIPITx(fd, &cmd_info);
    if (s32Ret != HI_SUCCESS) {
        return s32Ret;
    }
    usleep(USLEEP_TIME);

    memset_s(g_mBuf, G_MBUF_LENGTH, 0, G_MBUF_LENGTH);
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_0] = 0xEF;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_1] = 0x08;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_2] = 0x08;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_3] = 0x08;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_4] = 0x45;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_5] = 0x3F;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_6] = 0x54;
    cmd_info.devno = 0;
    cmd_info.cmd_size = 7; // 7: command data size
    cmd_info.data_type = 0x29;
    cmd_info.cmd = g_mBuf;
    s32Ret = SAMPLE_USER_INIT_MIPITx(fd, &cmd_info);
    if (s32Ret != HI_SUCCESS) {
        return s32Ret;
    }
    usleep(USLEEP_TIME);

    memset_s(g_mBuf, G_MBUF_LENGTH, 0, G_MBUF_LENGTH);
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_0] = 0xFF;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_1] = 0x77;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_2] = 0x01;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_3] = 0x00;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_4] = 0x00;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_5] = 0x00;
    cmd_info.devno = 0;
    cmd_info.cmd_size = 6; // 6: command data size
    cmd_info.data_type = 0x29;
    cmd_info.cmd = g_mBuf;
    s32Ret = SAMPLE_USER_INIT_MIPITx(fd, &cmd_info);
    if (s32Ret != HI_SUCCESS) {
        return s32Ret;
    }
    usleep(USLEEP_TIME);

    memset_s(g_mBuf, G_MBUF_LENGTH, 0, G_MBUF_LENGTH);
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_0] = 0xFF;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_1] = 0x77;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_2] = 0x01;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_3] = 0x00;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_4] = 0x00;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_5] = 0x13;
    cmd_info.devno = 0;
    cmd_info.cmd_size = 6; // 6: command data size
    cmd_info.data_type = 0x29;
    cmd_info.cmd = g_mBuf;
    s32Ret = SAMPLE_USER_INIT_MIPITx(fd, &cmd_info);
    if (s32Ret != HI_SUCCESS) {
        return s32Ret;
    }
    usleep(USLEEP_TIME);

    memset_s(g_mBuf, G_MBUF_LENGTH, 0, G_MBUF_LENGTH);
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_0] = 0xE8;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_1] = 0x00;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_2] = 0x0E;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_3] = 0x11;
    cmd_info.devno = 0;
    cmd_info.cmd_size = 4; // 4: command data size
    cmd_info.data_type = 0x29;
    cmd_info.cmd = g_mBuf;
    s32Ret = SAMPLE_USER_INIT_MIPITx(fd, &cmd_info);
    if (s32Ret != HI_SUCCESS) {
        return s32Ret;
    }
    usleep(USLEEP_TIME);
    usleep(120000); // 120000: The process hangs for a period of time, in microseconds

    memset_s(g_mBuf, G_MBUF_LENGTH, 0, G_MBUF_LENGTH);
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_0] = 0xE8;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_1] = 0x00;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_2] = 0x0C;
    cmd_info.devno = 0;
    cmd_info.cmd_size = 3; // 3: command data size
    cmd_info.data_type = 0x29;
    cmd_info.cmd = g_mBuf;
    s32Ret = SAMPLE_USER_INIT_MIPITx(fd, &cmd_info);
    if (s32Ret != HI_SUCCESS) {
        return s32Ret;
    }
    usleep(USLEEP_TIME);
    usleep(10000); // 10000: The process hangs for a period of time, in microseconds

    memset_s(g_mBuf, G_MBUF_LENGTH, 0, G_MBUF_LENGTH);
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_0] = 0xE8;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_1] = 0x00;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_2] = 0x00;
    cmd_info.devno = 0;
    cmd_info.cmd_size = 3; // 3: command data size
    cmd_info.data_type = 0x29;
    cmd_info.cmd = g_mBuf;
    s32Ret = SAMPLE_USER_INIT_MIPITx(fd, &cmd_info);
    if (s32Ret != HI_SUCCESS) {
        return s32Ret;
    }
    usleep(USLEEP_TIME);
    usleep(10000); // 10000: The process hangs for a period of time, in microseconds

    memset_s(g_mBuf, G_MBUF_LENGTH, 0, G_MBUF_LENGTH);
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_0] = 0xFF;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_1] = 0x77;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_2] = 0x01;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_3] = 0x00;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_4] = 0x00;
    g_mBuf[G_MBUF_ARRAY_SUBSCRIPT_5] = 0x00;
    cmd_info.devno = 0;
    cmd_info.cmd_size = 6; // 6: command data size
    cmd_info.data_type = 0x29;
    cmd_info.cmd = g_mBuf;
    s32Ret = SAMPLE_USER_INIT_MIPITx(fd, &cmd_info);
    if (s32Ret != HI_SUCCESS) {
        return s32Ret;
    }
    usleep(USLEEP_TIME);
    usleep(10000); // 10000: The process hangs for a period of time, in microseconds

    cmd_info.devno = 0;
    cmd_info.cmd_size = 0x11;
    cmd_info.data_type = 0x05;
    cmd_info.cmd = NULL;
    s32Ret = SAMPLE_USER_INIT_MIPITx(fd, &cmd_info);
    if (s32Ret != HI_SUCCESS) {
        return s32Ret;
    }
    usleep(USLEEP_TIME);
    usleep(150000); // 150000: The process hangs for a period of time, in microseconds

    cmd_info.devno = 0;
    cmd_info.cmd_size = 0x29;
    cmd_info.data_type = 0x05;
    cmd_info.cmd = NULL;
    s32Ret = SAMPLE_USER_INIT_MIPITx(fd, &cmd_info);
    if (s32Ret != HI_SUCCESS) {
        return s32Ret;
    }
    usleep(USLEEP_TIME);
    usleep(50000); // 50000: The process hangs for a period of time, in microseconds

    return HI_SUCCESS;
}

/*
 * 启用MIPI Tx设备
 * Enable MIPI Tx device
 */
HI_S32 SAMPLE_VO_ENABLE_MIPITx(HI_S32 fd)
{
    HI_S32 s32Ret = ioctl(fd, HI_MIPI_TX_ENABLE);
    if (s32Ret != HI_SUCCESS) {
        printf("MIPI_TX enable failed\n");
        return s32Ret;
    }

    return s32Ret;
}

/*
 * 禁用MIPI Tx设备
 * Disable MIPI Tx device
 */
HI_S32 SAMPLE_VO_DISABLE_MIPITx(HI_S32 fd)
{
    HI_S32 s32Ret = ioctl(fd, HI_MIPI_TX_DISABLE);
    if (s32Ret != HI_SUCCESS) {
        printf("MIPI_TX disable failed\n");
        return s32Ret;
    }

    return s32Ret;
}

/*
 * 设置VO至MIPI通路，获取MIPI设备
 * Set VO config to MIPI, get MIPI device
 */
HI_S32 SAMPLE_VO_CONFIG_MIPI(HI_S32* mipiFD)
{
    HI_S32 s32Ret;
    HI_S32  fd;

    /*
     * 打开MIPI FD
     * Open MIPI FD
     */
    fd = SampleOpenMipiTxFd();
    if (fd < 0) {
        return HI_FAILURE;
    }
	*mipiFD = fd;

    /*
     * 设置MIPI Tx设备属性
     * Set MIPI Tx device attribution
     */
    s32Ret = SampleSetMipiTxDevAttr(fd);
    if (s32Ret != HI_SUCCESS) {
        return s32Ret;
    }

    usleep(10000); // 10000: The process hangs for a period of time, in microseconds
    system("cd /sys/class/gpio/;echo 5 > export;echo out > gpio5/direction;echo 1 > gpio5/value");
    usleep(200000); // 200000: The process hangs for a period of time, in microseconds
    system("echo 0 > /sys/class/gpio/gpio5/value");
    usleep(200000); // 200000: The process hangs for a period of time, in microseconds
    system("echo 1 > /sys/class/gpio/gpio5/value");
    usleep(20000); // 20000: The process hangs for a period of time, in microseconds

    /*
     * 配置MIPI Tx初始化序列
     * Config MIPI Tx initialization sequence
     */
    s32Ret = SampleVoInitMipiTxScreen(fd);
    if (s32Ret != HI_SUCCESS) {
        return s32Ret;
    }

    /*
     * 启用MIPI Tx设备
     * Enable MIPI Tx device
     */
    s32Ret = SAMPLE_VO_ENABLE_MIPITx(fd);
    if (s32Ret != HI_SUCCESS) {
        return s32Ret;
    }

    return s32Ret;
}

/*
 * 获得mipi设备的宽和高
 * Get mipi device Height and width
 */
HI_S32 SampleCommVoGetWhMipi(VO_INTF_SYNC_E enIntfSync, HI_U32* pu32W, HI_U32* pu32H, HI_U32* pu32Frm)
{
    switch (enIntfSync) {
        case VO_OUTPUT_1080P24:
            *pu32W = 1920; // 1920: VO_OUTPUT_1080P24-Width
            *pu32H = 1080; // 1080: VO_OUTPUT_1080P24-Height
            *pu32Frm = 24; // 24: VO_OUTPUT_1080P24-Frame rate
            break;
        case VO_OUTPUT_1080P25:
            *pu32W = 1920; // 1920: VO_OUTPUT_1080P25-Width
            *pu32H = 1080; // 1080: VO_OUTPUT_1080P25-Height
            *pu32Frm = 25; // 25: VO_OUTPUT_1080P25-Frame rate
            break;
        case VO_OUTPUT_1080P30:
            *pu32W = 1920; // 1920: VO_OUTPUT_1080P30-Width
            *pu32H = 1080; // 1080: VO_OUTPUT_1080P30-Height
            *pu32Frm = 30; // 30: VO_OUTPUT_1080P30-Frame rate
            break;
        case VO_OUTPUT_720P50:
            *pu32W = 1280; // 1280: VO_OUTPUT_720P50-Width
            *pu32H = 720; // 720: VO_OUTPUT_720P50-Height
            *pu32Frm = 50; // 50: VO_OUTPUT_720P50-Frame rate
            break;
        case VO_OUTPUT_720P60:
            *pu32W = 1280; // 1280: VO_OUTPUT_720P60-Width
            *pu32H = 720; // 720: VO_OUTPUT_720P60-Height
            *pu32Frm = 60; // 60: VO_OUTPUT_720P60-Frame rate
            break;
        case VO_OUTPUT_1080P50:
            *pu32W = 1920; // 1920: VO_OUTPUT_1080P50-Width
            *pu32H = 1080; // 1080: VO_OUTPUT_1080P50-Height
            *pu32Frm = 50; // 50: VO_OUTPUT_1080P50-Frame rate
            break;
        case VO_OUTPUT_1080P60:
            *pu32W = 1920; // 1920: VO_OUTPUT_1080P60-Width
            *pu32H = 1080; // 1080: VO_OUTPUT_1080P60-Height
            *pu32Frm = 60; // 60: VO_OUTPUT_1080P60-Frame rate
            break;
        case VO_OUTPUT_USER:
            *pu32W = 800; // 800: VO_OUTPUT_USER-Width
            *pu32H = 480; // 480: VO_OUTPUT_USER-Height
            *pu32Frm = 60; // 60: VO_OUTPUT_USER-Frame rate
            break;
        default:
            SAMPLE_PRT("vo enIntfSync %d not support, please config self!\n", enIntfSync);
            return HI_FAILURE;
    }

    return HI_SUCCESS;
}

HI_S32 SampleCommVoStartDevMipi(VO_DEV VoDev, VO_PUB_ATTR_S* pstPubAttr)
{
    HI_S32 s32Ret;
    VO_USER_INTFSYNC_INFO_S stUserInfo = {0};

    stUserInfo.bClkReverse = HI_TRUE;
    stUserInfo.u32DevDiv = 1;
    stUserInfo.u32PreDiv = 1;
    stUserInfo.stUserIntfSyncAttr.enClkSource = VO_CLK_SOURCE_PLL;
    stUserInfo.stUserIntfSyncAttr.stUserSyncPll.u32Fbdiv = 244; // 244: PLL integer frequency multiplier coefficient
    stUserInfo.stUserIntfSyncAttr.stUserSyncPll.u32Frac = 0x1A36;
    stUserInfo.stUserIntfSyncAttr.stUserSyncPll.u32Refdiv = 4; // 4: PLL reference clock frequency division coefficient
    // 7: PLL first stage output frequency division coefficient
    stUserInfo.stUserIntfSyncAttr.stUserSyncPll.u32Postdiv1 = 7;
    // 7: PLL second stage output frequency division coefficient
    stUserInfo.stUserIntfSyncAttr.stUserSyncPll.u32Postdiv2 = 7;
    HI_U32 u32Framerate = 60; // 60: device frame rate

    /*
     * 配置视频输出设备的公共属性
     * Set the common properties of the video output device
     */
    s32Ret = HI_MPI_VO_SetPubAttr(VoDev, pstPubAttr);
    if (s32Ret != HI_SUCCESS) {
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
        return HI_FAILURE;
    }

    /*
     * 设置设备用户时序下设备帧率
     * Set the device frame rate under the device user timing
     */
    s32Ret = HI_MPI_VO_SetDevFrameRate(VoDev, u32Framerate);
    if (s32Ret != HI_SUCCESS) {
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
        return HI_FAILURE;
    }

    /*
     * 设置用户接口时序信息，用于配置时钟源、时钟大小和时钟分频比
     *
     * Set user interface timing information, used to configure clock source,
     * clock size and clock frequency division ratio
     */
    s32Ret = HI_MPI_VO_SetUserIntfSyncInfo(VoDev, &stUserInfo);
    if (s32Ret != HI_SUCCESS) {
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
        return HI_FAILURE;
    }

    /*
     * 启用视频输出设备
     * Enable video output device
     */
    s32Ret = HI_MPI_VO_Enable(VoDev);
    if (s32Ret != HI_SUCCESS) {
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
        return HI_FAILURE;
    }

    return s32Ret;
}

HI_S32 SampleCommVoStartChnModeMux(SAMPLE_VO_MODE_E enMode)
{
    int s32Ret;
    switch (enMode) {
        case VO_MODE_1MUX:
            g_sampleVoModeMux.u32WndNum = 1;
            g_sampleVoModeMux.u32Square = 1;
            break;
        case VO_MODE_2MUX:
            g_sampleVoModeMux.u32WndNum = 2; // 2: 2MUX-WndNum
            g_sampleVoModeMux.u32Square = 2; // 2: 2MUX-Square
            break;
        case VO_MODE_4MUX:
            g_sampleVoModeMux.u32WndNum = 4; // 4: 4MUX-WndNum
            g_sampleVoModeMux.u32Square = 2; // 2: 4MUX-Square
            break;
        case VO_MODE_8MUX:
            g_sampleVoModeMux.u32WndNum = 8; // 8: 8MUX-WndNum
            g_sampleVoModeMux.u32Square = 3; // 3: 8MUX-Square
            break;
        case VO_MODE_9MUX:
            g_sampleVoModeMux.u32WndNum = 9; // 9: 9MUX-WndNum
            g_sampleVoModeMux.u32Square = 3; // 3: 9MUX-Square
            break;
        case VO_MODE_16MUX:
            g_sampleVoModeMux.u32WndNum = 16; // 16: 16MUX-WndNum
            g_sampleVoModeMux.u32Square = 4; // 4: 16MUX-Square
            break;
        case VO_MODE_25MUX:
            g_sampleVoModeMux.u32WndNum = 25; // 25: 25MUX-WndNum
            g_sampleVoModeMux.u32Square = 5; // 5: 25MUX-Square
            break;
        case VO_MODE_36MUX:
            g_sampleVoModeMux.u32WndNum = 36; // 36: 36MUX-WndNum
            g_sampleVoModeMux.u32Square = 6; // 6: 36MUX-Square
            break;
        case VO_MODE_49MUX:
            g_sampleVoModeMux.u32WndNum = 49; // 49: 49MUX-WndNum
            g_sampleVoModeMux.u32Square = 7; // 7: 49MUX-Square
            break;
        case VO_MODE_2X4:
            g_sampleVoModeMux.u32WndNum = 8; // 8: 2X4-WndNum
            g_sampleVoModeMux.u32Square = 3; // 3: 2X4-Square
            g_sampleVoModeMux.u32Row    = 4; // 4: 2X4-Row
            g_sampleVoModeMux.u32Col    = 2; // 2: 2X4-Col
            break;
        default:
            SAMPLE_PRT("failed with %#x!\n", s32Ret);
            return HI_FAILURE;
    }

    return HI_SUCCESS;
}

HI_S32 SampleCommVoStartChnMipi(VO_LAYER VoLayer, SAMPLE_VO_MODE_E enMode)
{
    HI_S32 i;
    HI_S32 s32Ret    = HI_SUCCESS;
    HI_U32 u32Width  = 0;
    HI_U32 u32Height = 0;
    VO_CHN_ATTR_S         stChnAttr;
    VO_VIDEO_LAYER_ATTR_S stLayerAttr;

    s32Ret = SampleCommVoStartChnModeMux(enMode);
    SAMPLE_CHECK_EXPR_RET(s32Ret != HI_SUCCESS, s32Ret, "for SampleCommVoStartChnModeMux FAIL, s32Ret=%x\n", s32Ret);

    /*
     * 获取视频层属性
     * Get video layer properties
     */
    s32Ret = HI_MPI_VO_GetVideoLayerAttr(VoLayer, &stLayerAttr);
    SAMPLE_CHECK_EXPR_RET(s32Ret != HI_SUCCESS, s32Ret, "for HI_MPI_VO_GetVideoLayerAttr FAIL, s32Ret=%x\n", s32Ret);
    u32Width  = stLayerAttr.stImageSize.u32Width;
    u32Height = stLayerAttr.stImageSize.u32Height;
    SAMPLE_PRT("enMode:%d, u32Width:%d, u32Height:%d, u32Square:%d\n", enMode,
        u32Width, u32Height, g_sampleVoModeMux.u32Square);

    for (i = 0; i < g_sampleVoModeMux.u32WndNum; i++) {
        if (enMode == VO_MODE_1MUX  || enMode == VO_MODE_2MUX  || enMode == VO_MODE_4MUX  ||
            enMode == VO_MODE_8MUX  || enMode == VO_MODE_9MUX  || enMode == VO_MODE_16MUX ||
            enMode == VO_MODE_25MUX || enMode == VO_MODE_36MUX || enMode == VO_MODE_49MUX) {
            stChnAttr.stRect.s32X       = HI_ALIGN_DOWN((u32Width / g_sampleVoModeMux.u32Square) *
                (i % g_sampleVoModeMux.u32Square), ALIGN_DOWN_SIZE);
            stChnAttr.stRect.s32Y       = HI_ALIGN_DOWN((u32Height / g_sampleVoModeMux.u32Square) *
                (i / g_sampleVoModeMux.u32Square), ALIGN_DOWN_SIZE);
            stChnAttr.stRect.u32Width   = HI_ALIGN_DOWN(u32Width / g_sampleVoModeMux.u32Square, ALIGN_DOWN_SIZE);
            stChnAttr.stRect.u32Height  = HI_ALIGN_DOWN(u32Height / g_sampleVoModeMux.u32Square, ALIGN_DOWN_SIZE);
            stChnAttr.u32Priority       = 0;
            stChnAttr.bDeflicker        = HI_FALSE;
        } else if (enMode == VO_MODE_2X4) {
            stChnAttr.stRect.s32X       = HI_ALIGN_DOWN((u32Width / g_sampleVoModeMux.u32Col) *
                (i % g_sampleVoModeMux.u32Col), ALIGN_DOWN_SIZE);
            stChnAttr.stRect.s32Y       = HI_ALIGN_DOWN((u32Height / g_sampleVoModeMux.u32Row) *
                (i / g_sampleVoModeMux.u32Col), ALIGN_DOWN_SIZE);
            stChnAttr.stRect.u32Width   = HI_ALIGN_DOWN(u32Width / g_sampleVoModeMux.u32Col, ALIGN_DOWN_SIZE);
            stChnAttr.stRect.u32Height  = HI_ALIGN_DOWN(u32Height / g_sampleVoModeMux.u32Row, ALIGN_DOWN_SIZE);
            stChnAttr.u32Priority       = 0;
            stChnAttr.bDeflicker        = HI_FALSE;
        }

        /*
         * 配置指定视频输出通道的属性
         * Set properties for the specified video output channel
         */
        s32Ret = HI_MPI_VO_SetChnAttr(VoLayer, i, &stChnAttr);
        SAMPLE_CHECK_EXPR_RET(s32Ret != HI_SUCCESS, s32Ret, "for HI_MPI_VO_SetChnAttr FAIL, s32Ret=%x\n", s32Ret);

        /*
         * 设置指定视频输出通道的旋转角度
         * Set video output channel rotation angle
         */
        s32Ret = HI_MPI_VO_SetChnRotation(VoLayer, i, ROTATION_90);
        SAMPLE_CHECK_EXPR_RET(s32Ret != HI_SUCCESS, s32Ret, "for HI_MPI_VO_SetChnRotation FAIL, s32Ret=%x\n", s32Ret);

        /*
         * 启用指定的视频输出通道
         * Enables the specified video output channel
         */
        s32Ret = HI_MPI_VO_EnableChn(VoLayer, i);
        SAMPLE_CHECK_EXPR_RET(s32Ret != HI_SUCCESS, s32Ret, "for HI_MPI_VO_EnableChn FAIL, s32Ret=%x\n", s32Ret);
    }

    return HI_SUCCESS;
}

static HI_VOID StVoPubAttrCfg(SAMPLE_VO_CONFIG_S *pstVoConfig)
{
    HI_ASSERT(pstVoConfig);
    /*
     * 定义视频输出公共属性结构体
     * Define the video output public attribute structure
     */
    stVoPubAttr.enIntfType  = VO_INTF_MIPI;
    stVoPubAttr.enIntfSync  = VO_OUTPUT_USER;
    stVoPubAttr.stSyncInfo.bSynm = 0;
    stVoPubAttr.stSyncInfo.bIop = 1;
    stVoPubAttr.stSyncInfo.u8Intfb = 0;

    stVoPubAttr.stSyncInfo.u16Hmid = 1;
    stVoPubAttr.stSyncInfo.u16Bvact = 1;
    stVoPubAttr.stSyncInfo.u16Bvbb = 1;
    stVoPubAttr.stSyncInfo.u16Bvfb = 1;

    stVoPubAttr.stSyncInfo.bIdv = 0;
    stVoPubAttr.stSyncInfo.bIhs = 0;
    stVoPubAttr.stSyncInfo.bIvs = 0;

    stVoPubAttr.stSyncInfo.u16Hact = 480; // 480: Horizontal effective area. Unit: pixel
    stVoPubAttr.stSyncInfo.u16Hbb = 60; // 60: Horizontal blanking of the rear shoulder. Unit: pixel
    stVoPubAttr.stSyncInfo.u16Hfb = 50; // 50: Horizontal blanking of the front shoulder. Unit: pixel
    stVoPubAttr.stSyncInfo.u16Hpw = 10; // 10: The width of the horizontal sync signal. Unit: pixel
    stVoPubAttr.stSyncInfo.u16Vact = 800; // 800: Vertical effective area. Unit: line
    stVoPubAttr.stSyncInfo.u16Vbb = 24; // 24: Vertical blanking of the rear shoulder.  Unit: line
    stVoPubAttr.stSyncInfo.u16Vfb = 20; // 20: Vertical blanking of the front shoulder.  Unit: line
    stVoPubAttr.stSyncInfo.u16Vpw = 4; // 4: The width of the vertical sync signal. Unit: line
    stVoPubAttr.u32BgColor  = pstVoConfig->u32BgColor;
}

static HI_VOID StLayerAttrCfg(SAMPLE_VO_CONFIG_S *pstVoConfig)
{
    HI_ASSERT(pstVoConfig);
    stLayerAttr.bClusterMode     = HI_FALSE;
    stLayerAttr.bDoubleFrame    = HI_FALSE;
    stLayerAttr.enPixFormat       = pstVoConfig->enPixFormat;

    stLayerAttr.stDispRect.s32X = 0;
    stLayerAttr.stDispRect.s32Y = 0;
    stLayerAttr.enDstDynamicRange     = pstVoConfig->enDstDynamicRange;
}

/*
 * 启动VO到MIPI lcd通路
 * Start VO to MIPI lcd
 */
HI_S32 SampleCommVoStartMipi(SAMPLE_VO_CONFIG_S *pstVoConfig)
{
    HI_S32 s32Ret;

    HI_ASSERT(pstVoConfig);
    StVoPubAttrCfg(pstVoConfig);
    s32Ret = SampleCommVoStartDevMipi(pstVoConfig->VoDev, &stVoPubAttr);
    SAMPLE_CHECK_EXPR_RET(s32Ret != HI_SUCCESS, s32Ret, "StartDevMipi FAIL, ret=%x\n", s32Ret);
    /*
     * 获得MIPI设备的宽和高
     * Get MIPI device Height and width
     */
    s32Ret = SampleCommVoGetWhMipi(stVoPubAttr.enIntfSync,
        &stLayerAttr.stDispRect.u32Width, &stLayerAttr.stDispRect.u32Height, &stLayerAttr.u32DispFrmRt);
    SAMPLE_CHECK_EXPR_GOTO(s32Ret != HI_SUCCESS, FAIL, "VoGetWhMipi fail, err(%#x)\n", s32Ret);

    StLayerAttrCfg(pstVoConfig);

    /*
     * 如果存在变化，设置显示矩形框
     * Set display rectangle if changed
     */
    if (memcmp(&pstVoConfig->stDispRect, &stDefDispRect, sizeof(RECT_S)) != 0) {
        memcpy_s(&stLayerAttr.stDispRect, sizeof(stLayerAttr.stDispRect),
            &pstVoConfig->stDispRect, sizeof(RECT_S));
    }

    /*
     * 如果存在变化，设置图片大小
     * Set image size if changed
     */
    if (memcmp(&pstVoConfig->stImageSize, &stDefImageSize, sizeof(SIZE_S)) != 0) {
        memcpy_s(&stLayerAttr.stImageSize, sizeof(stLayerAttr.stImageSize),
            &pstVoConfig->stImageSize, sizeof(SIZE_S));
    }
    stLayerAttr.stImageSize.u32Width  = stLayerAttr.stDispRect.u32Width = 480; // 480: video layer canvas Width
    stLayerAttr.stImageSize.u32Height = stLayerAttr.stDispRect.u32Height = 800; // 800: video layer canvas Height

    if (pstVoConfig->u32DisBufLen) {
        /*
         * 设置显示缓冲的长度
         * Set buffer length
         */
        s32Ret = HI_MPI_VO_SetDisplayBufLen(pstVoConfig->VoDev, pstVoConfig->u32DisBufLen);
        SAMPLE_CHECK_EXPR_GOTO(s32Ret != HI_SUCCESS, FAIL, "HI_MPI_VO_SetDisplayBufLen fail, err(%#x)\n", s32Ret);
    }
    if (VO_PART_MODE_MULTI == pstVoConfig->enVoPartMode) {
        /*
         * 设置视频层的分割模式
         * Set the segmentation mode of the video layer
         */
        s32Ret = HI_MPI_VO_SetVideoLayerPartitionMode(pstVoConfig->VoDev, pstVoConfig->enVoPartMode);
        SAMPLE_CHECK_EXPR_GOTO(s32Ret != HI_SUCCESS, FAIL, "SetVideoLayerMode fail, err(%#x)\n", s32Ret);
    }

    s32Ret = SAMPLE_COMM_VO_StartLayer(pstVoConfig->VoDev, &stLayerAttr); // start layer
    SAMPLE_CHECK_EXPR_GOTO(s32Ret != HI_SUCCESS, FAIL, "VO_StartLayer fail, err(%#x)\n", s32Ret);

    if (VO_INTF_MIPI == pstVoConfig->enVoIntfType) {
        s32Ret = HI_MPI_VO_GetVideoLayerCSC(pstVoConfig->VoDev, &stVideoCSC); // get video layerCSC
        SAMPLE_CHECK_EXPR_GOTO(s32Ret != HI_SUCCESS, FAIL, "GetVideoLayerCSC fail, err(%#x)\n", s32Ret);
        stVideoCSC.enCscMatrix = VO_CSC_MATRIX_BT709_TO_RGB_PC;
        s32Ret = HI_MPI_VO_SetVideoLayerCSC(pstVoConfig->VoDev, &stVideoCSC); // Set video layer CSC
        SAMPLE_CHECK_EXPR_GOTO(s32Ret != HI_SUCCESS, FAIL, "SetVideoLayerCSC fail, err(%#x)\n", s32Ret);
    }

    s32Ret = SampleCommVoStartChnMipi(pstVoConfig->VoDev, pstVoConfig->enVoMode); // start vo channels
    SAMPLE_CHECK_EXPR_GOTO(s32Ret != HI_SUCCESS, FAIL1, "VoStartChnMipi fail, err(%#x)\n", s32Ret);
    return HI_SUCCESS;

FAIL1:
    SAMPLE_COMM_VO_StopLayer(pstVoConfig->VoDev);
FAIL:
    SAMPLE_COMM_VO_StopDev(pstVoConfig->VoDev);
    return s32Ret;
}

/*
 * 初始化vi配置
 * Init ViCfg
 */
void ViCfgInit(ViCfg* self)
{
    HI_ASSERT(self);
    if (memset_s(self, sizeof(*self), 0, sizeof(*self)) != EOK) {
        HI_ASSERT(0);
    }

    SAMPLE_COMM_VI_GetSensorInfo(self);
    self->s32WorkingViNum = 1;
    self->as32WorkingViId[0] = 0;

    self->astViInfo[0].stSnsInfo.MipiDev =
        SAMPLE_COMM_VI_GetComboDevBySensor(self->astViInfo[0].stSnsInfo.enSnsType, 0);
    self->astViInfo[0].stSnsInfo.s32BusId = 0;
}

/*
 * 设置VI设备信息
 * Set VI device information
 */
void ViCfgSetDev(ViCfg* self, int devId, WDR_MODE_E wdrMode)
{
    HI_ASSERT(self);
    HI_ASSERT((int)wdrMode < WDR_MODE_BUTT);

    self->astViInfo[0].stDevInfo.ViDev = devId;
    self->astViInfo[0].stDevInfo.enWDRMode = (int)wdrMode < 0 ? WDR_MODE_NONE : wdrMode;
}

/*
 * 设置VI的PIPE信息
 * Set the PIPE information of the VI
 */
void ViCfgSetPipe(ViCfg* self, int pipe0Id, int pipe1Id, int pipe2Id, int pipe3Id)
{
    HI_ASSERT(self);

    self->astViInfo[0].stPipeInfo.aPipe[APIPE0] = pipe0Id;
    self->astViInfo[0].stPipeInfo.aPipe[APIPE1] = pipe1Id;
    self->astViInfo[0].stPipeInfo.aPipe[APIPE2] = pipe2Id;
    self->astViInfo[0].stPipeInfo.aPipe[APIPE3] = pipe3Id;
}

/*
 * 设置VI通道
 * Set up the VI channel
 */
void ViCfgSetChn(ViCfg* self, int chnId, PIXEL_FORMAT_E pixFormat,
    VIDEO_FORMAT_E videoFormat, DYNAMIC_RANGE_E dynamicRange)
{
    HI_ASSERT(self);
    HI_ASSERT((int)pixFormat < PIXEL_FORMAT_BUTT);
    HI_ASSERT((int)videoFormat < VIDEO_FORMAT_BUTT);
    HI_ASSERT((int)dynamicRange < DYNAMIC_RANGE_BUTT);

    self->astViInfo[0].stChnInfo.ViChn = chnId;
    self->astViInfo[0].stChnInfo.enPixFormat =
        (int)pixFormat < 0 ? PIXEL_FORMAT_YVU_SEMIPLANAR_420 : pixFormat;
    self->astViInfo[0].stChnInfo.enVideoFormat =
        (int)videoFormat < 0 ? VIDEO_FORMAT_LINEAR : videoFormat;
    self->astViInfo[0].stChnInfo.enDynamicRange =
        (int)dynamicRange < 0 ? DYNAMIC_RANGE_SDR8 : dynamicRange;
}

/*
 * 初始化VPSS配置
 * Init VpssCfg
 */
void VpssCfgInit(VpssCfg* self)
{
    HI_ASSERT(self);
    if (memset_s(self, sizeof(*self), 0, sizeof(*self)) != EOK) {
        HI_ASSERT(0);
    }
    self->grpId = -1;
    self->chnNum = 0;
}

/*
 * 设置VPSS组
 * Set up VPSS Group
 */
void VpssCfgSetGrp(VpssCfg* self,
    int grpId, const VPSS_GRP_ATTR_S* grpAttr, int maxWidth, int maxHeight)
{
    HI_ASSERT(self);
    HI_ASSERT(grpId >= 0);

    self->grpId = grpId;

    if (grpAttr) {
        self->grpAttr = *grpAttr;
    } else { // Set as default
        self->grpAttr.u32MaxW = maxWidth < 0 ? 0 : maxWidth;
        self->grpAttr.u32MaxH = maxHeight < 0 ? 0 : maxHeight;
        self->grpAttr.enPixelFormat = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
        self->grpAttr.enDynamicRange = DYNAMIC_RANGE_SDR8;
        self->grpAttr.stFrameRate.s32SrcFrameRate = -1;
        self->grpAttr.stFrameRate.s32DstFrameRate = -1;
        self->grpAttr.bNrEn = HI_TRUE;
    }
}

/*
 * 增加一个VPSS通道
 * Add a VPSS channel
 */
VPSS_CHN_ATTR_S* VpssCfgAddChn(VpssCfg* self,
    int chnId, const VPSS_CHN_ATTR_S* chnAttr, int width, int height)
{
    HI_ASSERT(self);
    HI_ASSERT(self->chnNum < (sizeof(self->chnCfgs) / sizeof((self->chnCfgs)[0])));
    static const uint32_t depthDef = 2;
    VpssChnCfg *chnCfg = &self->chnCfgs[self->chnNum];

    if (chnAttr) {
        chnCfg->attr = *chnAttr;
    } else { // Set as default
        chnCfg->attr.u32Width = width < 0 ? 0 : width;
        chnCfg->attr.u32Height = height < 0 ? 0 : height;
        chnCfg->attr.enChnMode = VPSS_CHN_MODE_USER;
        chnCfg->attr.enVideoFormat = VIDEO_FORMAT_LINEAR;
        chnCfg->attr.enPixelFormat = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
        chnCfg->attr.enDynamicRange = DYNAMIC_RANGE_SDR8;
        chnCfg->attr.enCompressMode = COMPRESS_MODE_NONE;
        chnCfg->attr.stFrameRate.s32SrcFrameRate = -1;
        chnCfg->attr.stFrameRate.s32DstFrameRate = -1;
        chnCfg->attr.u32Depth = depthDef;
        chnCfg->attr.bMirror = HI_FALSE;
        chnCfg->attr.bFlip = HI_FALSE;
        chnCfg->attr.stAspectRatio.enMode = ASPECT_RATIO_NONE;
    }
    chnCfg->id = chnId;

    self->chnNum++;
    return &chnCfg->attr;
}

/*
 * 根据VPSS配置来启动VPSS
 * Start VPSS according to VPSS config
 */
int VpssStart(const VpssCfg* cfg)
{
    HI_ASSERT(cfg);
    VPSS_GRP grpId = cfg->grpId;
    HI_S32 ret;

    /*
     * 创建一个VPSS GROUP
     * Create a VPSS GROUP
     */
    ret = HI_MPI_VPSS_CreateGrp(grpId, &cfg->grpAttr);
    if (ret != 0) {
        SAMPLE_PRT("HI_MPI_VPSS_CreateGrp(%d) FAIL, ret=%#x\n", grpId, ret);
        return ret;
    }

    for (int i = 0; i < cfg->chnNum; i++) {
        /*
         * 设置VPSS通道属性
         * Set VPSS channel properties
         */
        ret = HI_MPI_VPSS_SetChnAttr(grpId, cfg->chnCfgs[i].id, &cfg->chnCfgs[i].attr);
        if (ret != 0) {
            SAMPLE_PRT("HI_MPI_VPSS_SetChnAttr(%d) FAIL, ret=%#x\n", cfg->chnCfgs[i].id, ret);
            return ret;
        }

        /*
         * 启用VPSS通道
         * Enable VPSS channel
         */
        ret = HI_MPI_VPSS_EnableChn(grpId, cfg->chnCfgs[i].id);
        if (ret != 0) {
            SAMPLE_PRT("HI_MPI_VPSS_EnableChn(%d) FAIL, ret=%#x\n", cfg->chnCfgs[i].id, ret);
            return ret;
        }
    }

    /*
     * 启用VPSS GROUP
     * Enable VPSS GROUP
     */
    ret = HI_MPI_VPSS_StartGrp(grpId);
    if (ret != 0) {
        SAMPLE_PRT("HI_MPI_VPSS_StartGrp(%d) FAIL, ret=%#x\n", grpId, ret);
        return ret;
    }
    return HI_SUCCESS;
}

/*
 * 终止使用ViCfg启动的VI
 * Terminate VI started with ViCfg
 */
int ViStop(const ViCfg* viCfg)
{
    return SAMPLE_COMM_VI_StopVi((ViCfg*)viCfg);
}

/*
 * 停止使用VpssCfg启动的VPSS
 * Terminate VPSS started with VpssCfg
 */
int VpssStop(const VpssCfg* cfg)
{
    HI_ASSERT(cfg);
    VPSS_GRP grpId = cfg->grpId;
    HI_S32 ret;

    for (int i = 0; i < cfg->chnNum; i++) {
        /*
         * 禁用VPSS通道
         * Disable VPSS channel
         */
        ret = HI_MPI_VPSS_DisableChn(grpId, cfg->chnCfgs[i].id);
        if (ret != 0) {
            SAMPLE_PRT("HI_MPI_VPSS_DisableChn(%d, %d) FAIL, ret=%#x\n", grpId, cfg->chnCfgs[i].id, ret);
            return ret;
        }
    }

    /*
     * 禁用VPSS GROUP
     * Stop VPSS GROUP
     */
    ret = HI_MPI_VPSS_StopGrp(grpId);
    if (ret != 0) {
        SAMPLE_PRT("HI_MPI_VPSS_StopGrp(%d) FAIL, ret=%#x\n", grpId, ret);
        return ret;
    }

    /*
     * 销毁一个VPSS GROUP
     * Destroy a VPSS GROUP
     */
    ret = HI_MPI_VPSS_DestroyGrp(grpId);
    if (ret != 0) {
        SAMPLE_PRT("HI_MPI_VPSS_DestroyGrp(%d) FAIL, ret=%#x\n", grpId, ret);
        return ret;
    }
    return HI_SUCCESS;
}

/*
 * 根据VI配置来启动VI
 * Start VI according to ViCfg
 */
int ViStart(const ViCfg* viCfg)
{
    static const uint32_t frmRateDef = 30;
    SAMPLE_SNS_TYPE_E snsType = viCfg->astViInfo[0].stSnsInfo.enSnsType;
    ISP_CTRL_PARAM_S ispCtrlParam;
    HI_U32 frmRate;
    HI_S32 ret;

    ret = SAMPLE_COMM_VI_SetParam((ViCfg*)viCfg);
    if (ret != 0) {
        SAMPLE_PRT("SAMPLE_COMM_VI_SetParam FAIL, ret=%#x\n", ret);
        return ret;
    }

    /*
     * 通过Sensor型号来获取帧率
     * Get the frame rate through the Sensor model
     */
    SAMPLE_COMM_VI_GetFrameRateBySensor(snsType, &frmRate);
    ret = HI_MPI_ISP_GetCtrlParam(viCfg->astViInfo[0].stPipeInfo.aPipe[0], &ispCtrlParam);
    if (ret != 0) {
        SAMPLE_PRT("HI_MPI_ISP_GetCtrlParam FAIL, ret=%#x\n", ret);
        return ret;
    }

    ispCtrlParam.u32StatIntvl = frmRate / frmRateDef;
    ret = HI_MPI_ISP_SetCtrlParam(viCfg->astViInfo[0].stPipeInfo.aPipe[0], &ispCtrlParam);
    if (ret != 0) {
        SAMPLE_PRT("HI_MPI_ISP_SetCtrlParam FAIL, ret=%#x\n", ret);
        return ret;
    }

    ret = SAMPLE_COMM_VI_StartVi((ViCfg*)viCfg);
    if (ret != 0) {
        SAMPLE_PRT("SAMPLE_COMM_VI_StartVi FAIL, ret=%#x\n", ret);
        return ret;
    }

    return HI_SUCCESS;
}

/*
 * 将VI绑定到VPSS
 * Bind VI to VPSS
 */
int ViBindVpss(VI_PIPE viPipe, VI_CHN viChn, VPSS_GRP vpssGrp)
{
    MPP_CHN_S srcChn;
    MPP_CHN_S dstChn;

    srcChn.enModId = HI_ID_VI;
    srcChn.s32DevId = viPipe;
    srcChn.s32ChnId = viChn;

    dstChn.enModId = HI_ID_VPSS;
    dstChn.s32DevId = vpssGrp;
    dstChn.s32ChnId = 0;

    int ret = HI_MPI_SYS_Bind(&srcChn, &dstChn);
    if (ret != 0) {
        SAMPLE_PRT("HI_MPI_SYS_Bind(VI:%d, VPSS:%d) FAIL, ret=%#x\n", viChn, vpssGrp, ret);
        return ret;
    }

    return ret;
}

/*
 * 新的MppSess
 * New MppSess
 */
static MppSess* MppSessNew(void)
{
    MppSess *sess = (MppSess*)malloc(sizeof(*sess));
    if (sess == NULL) {
        SAMPLE_PRT("%s: malloc failed!\n", __FUNCTION__);
        HI_ASSERT(sess);
    }

    if (memset_s(sess, sizeof(*sess), 0, sizeof(*sess)) != EOK) {
        HI_ASSERT(0);
    }

    sess->vpssGrp = -1;
    sess->vpssChn0 = -1;
    sess->vpssChn1 = -1;

    return sess;
}

/*
 * 创建并启动{VI->VPSS}MppSess
 * Create and start {VI->VPSS}MppSess
 */
int ViVpssCreate(MppSess** sess, const ViCfg* viCfg, const VpssCfg* vpssCfg)
{
    HI_ASSERT(sess && viCfg && vpssCfg);
    *sess = NULL;
    int res;
    int ret;

    ret = ViStart(viCfg);
    SAMPLE_CHECK_EXPR_GOTO(ret != HI_SUCCESS, FAIL1,
        "vi start fail, err(%#x)\n", ret);

    ret = VpssStart(vpssCfg);
    SAMPLE_CHECK_EXPR_GOTO(ret != HI_SUCCESS, FAIL2,
        "vpss start fail, err(%#x)\n", ret);

    VI_PIPE pipeId = viCfg->astViInfo[0].stPipeInfo.aPipe[0];
    VI_CHN chnId = viCfg->astViInfo[0].stChnInfo.ViChn;
    ret = ViBindVpss(pipeId, chnId, vpssCfg->grpId);
    SAMPLE_CHECK_EXPR_GOTO(ret != HI_SUCCESS, FAIL3,
        "vi bind vpss fail, err(%#x)\n", ret);

    MppSess *self = MppSessNew(); // todo:realease malloc
    *sess = self;
    self->viCfg = *viCfg;
    self->vpssCfg = *vpssCfg;
    self->used |= MPP_VI;
    self->used |= MPP_VPSS;
    self->vpssGrp = vpssCfg->grpId;
    self->vpssChn0 = vpssCfg->chnCfgs[0].id;
    self->vpssChn1 = vpssCfg->chnNum > 1 ? vpssCfg->chnCfgs[1].id : -1;
    return 0;

    FAIL3:
        res = VpssStop(vpssCfg);
        SAMPLE_PRT("ViVpssCreate\n");
        if (res != 0) {
            SAMPLE_PRT("VpssStop FAIL, ret=%#x\n", res);
        }
    FAIL2:
        ViStop(viCfg);
    FAIL1:
        return ret;
}

static HI_VOID CnnTrashClassifyAiProcess(VIDEO_FRAME_INFO_S frm)
{
    int ret;
    if (GetCfgBool("trash_classify_switch:support_trash_classify", true)) {
        if (g_workPlug.model == 0) {
            HI_ASSERT(!g_aicMediaInfo.osds);
            g_aicMediaInfo.osds = OsdsCreate(HI_OSD_BINDMOD_VPSS, g_aicMediaInfo.vpssGrp, g_aicMediaInfo.vpssChn0);
            HI_ASSERT(g_aicMediaInfo.osds);
            ret = CnnTrashClassifyLoadModel(&g_workPlug.model, g_aicMediaInfo.osds);
            if (ret < 0) {
                g_workPlug.model = 0;
                SAMPLE_CHECK_EXPR_GOTO(ret < 0, TRASH_RELEASE,
                    "load cnn trash classify model err(%#x)\n", ret);
            }
        }
        VIDEO_FRAME_INFO_S *resFrm = NULL;
        ret = CnnTrashClassifyCal(g_workPlug.model, &frm, resFrm);
        SAMPLE_CHECK_EXPR_GOTO(ret < 0, TRASH_RELEASE,
            "trash classify plug cal FAIL, ret=%#x\n", ret);
    }

    TRASH_RELEASE:
        ret = HI_MPI_VPSS_ReleaseChnFrame(g_aicMediaInfo.vpssGrp, g_aicMediaInfo.vpssChn0, &frm);
        if (ret != HI_SUCCESS) {
            SAMPLE_PRT("Error(%#x),HI_MPI_VPSS_ReleaseChnFrame failed,Grp(%d) chn(%d)!\n",
                ret, g_aicMediaInfo.vpssGrp, g_aicMediaInfo.vpssChn0);
        }
}

static HI_VOID* GetVpssChnFrameCnnTrashClassify(HI_VOID* arg)
{
    int ret;
    VIDEO_FRAME_INFO_S frm;
    HI_S32 s32MilliSec = 20000;

    while (HI_FALSE == g_bAiProcessStopSignal) {
        ret = HI_MPI_VPSS_GetChnFrame(g_aicMediaInfo.vpssGrp, g_aicMediaInfo.vpssChn0, &frm, s32MilliSec);
        if (ret != 0) {
            SAMPLE_PRT("HI_MPI_VPSS_GetChnFrame FAIL, err=%#x, grp=%d, chn=%d\n",
                ret, g_aicMediaInfo.vpssGrp, g_aicMediaInfo.vpssChn0);
            ret = HI_MPI_VPSS_ReleaseChnFrame(g_aicMediaInfo.vpssGrp, g_aicMediaInfo.vpssChn0, &frm);
            if (ret != HI_SUCCESS) {
                SAMPLE_PRT("Error(%#x),HI_MPI_VPSS_ReleaseChnFrame failed,Grp(%d) chn(%d)!\n",
                    ret, g_aicMediaInfo.vpssGrp, g_aicMediaInfo.vpssChn0);
            }
            continue;
        }
        SAMPLE_PRT("get vpss frame success, weight:%d, height:%d\n", frm.stVFrame.u32Width, frm.stVFrame.u32Height);

        if (g_num == 0) {
            ConfBaseInit(AI_SAMPLE_CFG_FILE);
            g_num++;
        }
        CnnTrashClassifyAiProcess(frm);
    }

    return HI_NULL;
}

static HI_VOID HandClassifyAiProcess(VIDEO_FRAME_INFO_S frm, VO_LAYER voLayer, VO_CHN voChn)
{
    int ret;
    if (GetCfgBool("hand_classify_switch:support_hand_classify", true)) {
        if (g_workPlug.model == 0) {
            ret = Yolo2HandDetectResnetClassifyLoad(&g_workPlug.model);// 在 hand_classify.c 中，79。加载模型
            if (ret < 0) {
                g_workPlug.model = 0;
                SAMPLE_CHECK_EXPR_GOTO(ret < 0, HAND_RELEASE,
                    "load hand classify model err, ret=%#x\n", ret);
            }
        }

        VIDEO_FRAME_INFO_S resizeFrm;
        /*
        typedef struct hiVIDEO_FRAME_INFO_S {
            VIDEO_FRAME_S stVFrame;// 这个决定了是什么类型的 yuv 数据：YUV420/422
            HI_U32        u32PoolId;
            MOD_ID_E      enModId;
        } VIDEO_FRAME_INFO_S;
        */
        ret = MppFrmResize(&frm, &resizeFrm, HAND_FRM_WIDTH, HAND_FRM_HEIGHT);// 在 vgs_img.c 中，285。 
        // 对获取的一帧进行帧缩放得到 resizeFrm 此时还不是 yuv 图片，所以不在这里操作。

        ret = Yolo2HandDetectResnetClassifyCal(g_workPlug.model, &resizeFrm, &frm);// 在 hand_classify.c 中，335。
        // 将帧缩放后的图片传入模型
        SAMPLE_CHECK_EXPR_GOTO(ret < 0, HAND_RELEASE,
            "hand classify plug cal FAIL, ret=%#x\n", ret);

        ret = HI_MPI_VO_SendFrame(voLayer, voChn, &frm, 0);
        SAMPLE_CHECK_EXPR_GOTO(ret != HI_SUCCESS, HAND_RELEASE,
            "HI_MPI_VO_SendFrame fail, Error(%#x)\n", ret);

        MppFrmDestroy(&resizeFrm);
    }

    HAND_RELEASE:
        ret = HI_MPI_VPSS_ReleaseChnFrame(g_aicMediaInfo.vpssGrp, g_aicMediaInfo.vpssChn0, &frm);
        if (ret != HI_SUCCESS) {
            SAMPLE_PRT("Error(%#x),HI_MPI_VPSS_ReleaseChnFrame failed,Grp(%d) chn(%d)!\n",
                ret, g_aicMediaInfo.vpssGrp, g_aicMediaInfo.vpssChn0);
        }
}

static HI_VOID* GetVpssChnFrameHandClassify(HI_VOID* arg)
{
    int ret;
    VIDEO_FRAME_INFO_S frm;
    HI_S32 s32MilliSec = 2000;
    VO_LAYER voLayer = 0;
    VO_CHN voChn = 0;

    SAMPLE_PRT("vpssGrp:%d, vpssChn0:%d\n", g_aicMediaInfo.vpssGrp, g_aicMediaInfo.vpssChn0);

    while (HI_FALSE == g_bAiProcessStopSignal) {
        ret = HI_MPI_VPSS_GetChnFrame(g_aicMediaInfo.vpssGrp, g_aicMediaInfo.vpssChn0, &frm, s32MilliSec);
        // 用户获取一帧通道图像。
        if (ret != 0) {
            SAMPLE_PRT("HI_MPI_VPSS_GetChnFrame FAIL, err=%#x, grp=%d, chn=%d\n",
                ret, g_aicMediaInfo.vpssGrp, g_aicMediaInfo.vpssChn0);
            ret = HI_MPI_VPSS_ReleaseChnFrame(g_aicMediaInfo.vpssGrp, g_aicMediaInfo.vpssChn0, &frm);
            if (ret != HI_SUCCESS) {
                SAMPLE_PRT("Error(%#x),HI_MPI_VPSS_ReleaseChnFrame failed,Grp(%d) chn(%d)!\n",
                    ret, g_aicMediaInfo.vpssGrp, g_aicMediaInfo.vpssChn0);
            }
            continue;
        }
        SAMPLE_PRT("get vpss frame success, weight:%d, height:%d\n", frm.stVFrame.u32Width, frm.stVFrame.u32Height);

        if (g_num == 0) {
            ConfBaseInit(AI_SAMPLE_CFG_FILE);
            g_num++;
        }
        HandClassifyAiProcess(frm, voLayer, voChn);
    }

    return HI_NULL;
}

static HI_VOID Pause(HI_VOID)
{
    printf("---------------press Enter key to exit!---------------\n");
    (void)getchar();
}

HI_VOID ViPramCfg(HI_VOID)
{
    ViCfgInit(&g_aicMediaInfo.viCfg);
    ViCfgSetDev(&g_aicMediaInfo.viCfg, 0, -1);
    ViCfgSetPipe(&g_aicMediaInfo.viCfg, 0, -1, -1, -1);
    g_aicMediaInfo.viCfg.astViInfo[0].stPipeInfo.enMastPipeMode = 0;
    ViCfgSetChn(&g_aicMediaInfo.viCfg, 0, -1, -1, -1);
    g_aicMediaInfo.viCfg.astViInfo[0].stChnInfo.enCompressMode = 1;
}

static HI_VOID StVbParamCfg(VbCfg *self)
{
    memset_s(&g_aicMediaInfo.vbCfg, sizeof(VB_CONFIG_S), 0, sizeof(VB_CONFIG_S));
    // 2: The number of buffer pools that can be accommodated in the entire system
    self->u32MaxPoolCnt              = 2;

    /*
     * 获取一帧图片的buffer大小
     * Get picture buffer size
     */
    g_aicMediaInfo.u32BlkSize = COMMON_GetPicBufferSize(g_aicMediaInfo.stSize.u32Width, g_aicMediaInfo.stSize.u32Height,
        SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8, COMPRESS_MODE_SEG, DEFAULT_ALIGN);
    self->astCommPool[0].u64BlkSize  = g_aicMediaInfo.u32BlkSize;
    // 10: Number of cache blocks per cache pool. Value range: (0, 10240]
    self->astCommPool[0].u32BlkCnt   = 10;

    /*
     * 获取raw buffer的大小
     * Get raw buffer size
     */
    g_aicMediaInfo.u32BlkSize = VI_GetRawBufferSize(g_aicMediaInfo.stSize.u32Width, g_aicMediaInfo.stSize.u32Height,
        PIXEL_FORMAT_RGB_BAYER_16BPP, COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    self->astCommPool[1].u64BlkSize  = g_aicMediaInfo.u32BlkSize;
    // 4: Number of cache blocks per cache pool. Value range: (0, 10240]
    self->astCommPool[1].u32BlkCnt   = 4;
}

static HI_VOID StVoParamCfg(VoCfg *self)
{
    SAMPLE_COMM_VO_GetDefConfig(self);
    self->enDstDynamicRange = DYNAMIC_RANGE_SDR8;

    self->enVoIntfType = VO_INTF_MIPI; /* set VO int type */
    self->enIntfSync = VO_OUTPUT_USER; /* set VO output information */

    self->enPicSize = g_aicMediaInfo.enPicSize;
}

static HI_VOID VpssParamCfg(HI_VOID)
{
    VpssCfgInit(&g_aicMediaInfo.vpssCfg);
    VpssCfgSetGrp(&g_aicMediaInfo.vpssCfg, AIC_VPSS_GRP, NULL,
        g_aicMediaInfo.stSize.u32Width, g_aicMediaInfo.stSize.u32Width);
    g_aicMediaInfo.vpssCfg.grpAttr.enPixelFormat = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    VpssCfgAddChn(&g_aicMediaInfo.vpssCfg, AIC_VPSS_ZOUT_CHN, NULL, AICSTART_VI_OUTWIDTH, AICSTART_VI_OUTHEIGHT);
    HI_ASSERT(!g_aicMediaInfo.viSess);
}
//垃圾分类
static HI_S32 CnnTrashAiThreadProcess(HI_VOID)
{
    HI_S32 s32Ret;
    if (snprintf_s(acThreadName, BUFFER_SIZE, BUFFER_SIZE - 1, "AIProcess") < 0) {
        HI_ASSERT(0);
    }
    prctl(PR_SET_NAME, (unsigned long)acThreadName, 0, 0, 0);
    s32Ret = pthread_create(&g_aiProcessThread, NULL, GetVpssChnFrameCnnTrashClassify, NULL);

    return s32Ret;
}
//手势分类
static HI_S32 HandClassifyAiThreadProcess(HI_VOID)
{
    HI_S32 s32Ret;
    if (snprintf_s(acThreadName, BUFFER_SIZE, BUFFER_SIZE - 1, "AIProcess") < 0) {
        HI_ASSERT(0);
    }
    prctl(PR_SET_NAME, (unsigned long)acThreadName, 0, 0, 0);
    s32Ret = pthread_create(&g_aiProcessThread, NULL, GetVpssChnFrameHandClassify, NULL);

    return s32Ret;
}
//停止垃圾分类
static HI_S32 PauseDoUnloadCnnModel(HI_VOID)
{
    HI_S32 s32Ret = HI_SUCCESS;

    if (GetCfgBool("trash_classify_switch:support_trash_classify", true)) {
        if (memset_s(&g_workPlug, sizeof(g_workPlug), 0x00, sizeof(g_workPlug)) != EOK) {
            HI_ASSERT(0);
        }
        /*
         * 退出运行时，应卸载模型
         * When exiting the operation, the model should be unloaded
         */
        s32Ret = CnnTrashClassifyUnloadModel(g_workPlug.model);
        SAMPLE_CHECK_EXPR_RET(s32Ret != HI_SUCCESS, s32Ret, "unload cnn trash classify model err:%x\n", s32Ret);
        ConfBaseExt();
        g_num = 0;
        OsdsClear(g_aicMediaInfo.osds);
        OsdsDestroy(g_aicMediaInfo.osds);
    }

    return s32Ret;
}
//停止手势分类
static HI_S32 PauseDoUnloadHandClassifyModel(HI_VOID)
{
    HI_S32 s32Ret = HI_SUCCESS;

    if (GetCfgBool("hand_classify_switch:support_hand_classify", true)) {
        if (memset_s(&g_workPlug, sizeof(g_workPlug), 0x00, sizeof(g_workPlug)) != EOK) {
            HI_ASSERT(0);
        }
        /*
         * 退出运行时，应卸载模型
         * When exiting the operation, the model should be unloaded
         */
        s32Ret = Yolo2HandDetectResnetClassifyUnload(g_workPlug.model);
        SAMPLE_CHECK_EXPR_RET(s32Ret != HI_SUCCESS, s32Ret, "unload hand model err:%x\n", s32Ret);
        ConfBaseExt();
        g_num = 0;
    }

    return s32Ret;
}

/*
 * 将sensor采集到数据显示到液晶屏上，同时创建线程运行垃圾分类推理计算
 * 视频输入->视频处理子系统->视频输出->显示屏
 *
 * Display the data collected by the sensor on the LCD screen,
 * and at the same time create a thread to run trash classification reasoning calculations
 * VI->VPSS->VO->MIPI
 */
HI_S32 SAMPLE_MEDIA_CNN_TRASH_CLASSIFY(HI_VOID)
{
/*
    先配置VI参数，获取enPicSize，从而得到图片的宽高。配置VB参数，视频缓存池初始化以及MPI系统初始化，
    然后就可以设置VO至MIPI通路，获取MIPI设备，配置VPSS参数，配置VO参数。然后就可以启动VO到MIPI lcd通路。VPSS绑定VO
*/ 

    HI_S32             s32Ret;
    HI_S32             fd = 0;

    /*
     * 配置VI参数
     * Config VI parameter
     */
    ViPramCfg();

    /*
     * 通过Sensor型号获取enPicSize
     * Obtain enPicSize through the Sensor type
     */
    s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(g_aicMediaInfo.viCfg.astViInfo[0].stSnsInfo.enSnsType,
        &g_aicMediaInfo.enPicSize);// 在 sample_comm.h 里面
    SAMPLE_CHECK_EXPR_RET(s32Ret != HI_SUCCESS, s32Ret, "get pic size by sensor fail, s32Ret=%#x\n", s32Ret);

    /*
     * 根据enPicSize，得到图片的宽高
     * Get picture size(w*h), according enPicSize
     */
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(g_aicMediaInfo.enPicSize, &g_aicMediaInfo.stSize);
    SAMPLE_PRT("AIC: snsMaxSize=%ux%u\n", g_aicMediaInfo.stSize.u32Width, g_aicMediaInfo.stSize.u32Height);
    SAMPLE_CHECK_EXPR_RET(s32Ret != HI_SUCCESS, s32Ret, "get picture size failed, s32Ret=%#x\n", s32Ret);

    /*
     * 配置VB参数
     * Config VB parameter
     */
    StVbParamCfg(&g_aicMediaInfo.vbCfg);

    /*
     * 视频缓存池初始化以及MPI系统初始化
     * VB init & MPI system init
     */
    s32Ret = SAMPLE_COMM_SYS_Init(&g_aicMediaInfo.vbCfg);
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
    VpssParamCfg();
    s32Ret = ViVpssCreate(&g_aicMediaInfo.viSess, &g_aicMediaInfo.viCfg, &g_aicMediaInfo.vpssCfg);
    SAMPLE_CHECK_EXPR_GOTO(s32Ret != HI_SUCCESS, EXIT1, "ViVpss Sess create FAIL, ret=%#x\n", s32Ret);
    g_aicMediaInfo.vpssGrp = AIC_VPSS_GRP;
    g_aicMediaInfo.vpssChn0 = AIC_VPSS_ZOUT_CHN;

    /*
     * 配置VO参数
     * Config VO parameter
     */
    StVoParamCfg(&g_aicMediaInfo.voCfg);

    /*
     * 启动VO到MIPI lcd通路
     * Start VO to MIPI lcd
     */
    s32Ret = SampleCommVoStartMipi(&g_aicMediaInfo.voCfg);
    SAMPLE_CHECK_EXPR_GOTO(s32Ret != HI_SUCCESS, EXIT1, "start vo FAIL. s32Ret: 0x%x\n", s32Ret);

    /*
     * VPSS绑定VO
     * VPSS bind VO
     */
    s32Ret = SAMPLE_COMM_VPSS_Bind_VO(g_aicMediaInfo.vpssGrp, g_aicMediaInfo.vpssChn0, g_aicMediaInfo.voCfg.VoDev, 0);
    SAMPLE_CHECK_EXPR_GOTO(s32Ret != HI_SUCCESS, EXIT2, "vo bind vpss FAIL. s32Ret: 0x%x\n", s32Ret);
    SAMPLE_PRT("vpssGrp:%d, vpssChn:%d\n", g_aicMediaInfo.vpssGrp, g_aicMediaInfo.vpssChn0);

    /*
     * 创建工作线程运行ai
     * Create work thread to run ai
     */
    s32Ret = CnnTrashAiThreadProcess();
    SAMPLE_CHECK_EXPR_RET(s32Ret != HI_SUCCESS, s32Ret, "ai proccess thread creat fail:%s\n", strerror(s32Ret));
    Pause();
    g_bAiProcessStopSignal = HI_TRUE;
    /*
     * 等待一个线程结束，线程间同步的操作
     * Waiting for the end of a thread, the operation of synchronization between threads
     */
    pthread_join(g_aiProcessThread, NULL);
    g_aiProcessThread = 0;
    PauseDoUnloadCnnModel();

    SAMPLE_COMM_VPSS_UnBind_VO(g_aicMediaInfo.vpssGrp, g_aicMediaInfo.vpssChn0, g_aicMediaInfo.voCfg.VoDev, 0);
    SAMPLE_VO_DISABLE_MIPITx(fd);
    SampleCloseMipiTxFd(fd);
    system("echo 0 > /sys/class/gpio/gpio55/value");

EXIT2:
    SAMPLE_COMM_VO_StopVO(&g_aicMediaInfo.voCfg);
EXIT1:
    VpssStop(&g_aicMediaInfo.vpssCfg);
    SAMPLE_COMM_VI_UnBind_VPSS(g_aicMediaInfo.viCfg.astViInfo[0].stPipeInfo.aPipe[0],
        g_aicMediaInfo.viCfg.astViInfo[0].stChnInfo.ViChn, g_aicMediaInfo.vpssGrp);
    ViStop(&g_aicMediaInfo.viCfg);
    free(g_aicMediaInfo.viSess);
EXIT:
    SAMPLE_COMM_SYS_Exit();
    return s32Ret;
}

/*
 * 将sensor采集到数据显示到液晶屏上，同时创建线程运行手部检测和手势分类推理计算
 * 视频输入->视频处理子系统->视频输出->显示屏
 *
 * Display the data collected by the sensor on the LCD screen,
 * and create threads to run hand detection and gesture classification reasoning calculations
 * VI->VPSS->VO->MIPI
 */
HI_S32 SAMPLE_MEDIA_HAND_CLASSIFY(HI_VOID)
{
    HI_S32             s32Ret;
    HI_S32             fd = 0;

    /*
     * 配置VI参数
     * Config VI parameter
     */
    ViPramCfg();

    // 通过sensor型号获取图片大小
    /*
     * 通过Sensor型号获取enPicSize    
     * Obtain enPicSize through the Sensor type
     */
    s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(g_aicMediaInfo.viCfg.astViInfo[0].stSnsInfo.enSnsType,
        &g_aicMediaInfo.enPicSize);
    SAMPLE_CHECK_EXPR_RET(s32Ret != HI_SUCCESS, s32Ret, "get pic size by sensor fail, s32Ret=%#x\n", s32Ret);

    /*
     * 根据enPicSize，得到图片的宽高
     * Get picture size(w*h), according enPicSize
     */
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(g_aicMediaInfo.enPicSize, &g_aicMediaInfo.stSize);
    SAMPLE_PRT("AIC: snsMaxSize=%ux%u\n", g_aicMediaInfo.stSize.u32Width, g_aicMediaInfo.stSize.u32Height);
    SAMPLE_CHECK_EXPR_RET(s32Ret != HI_SUCCESS, s32Ret, "get picture size failed, s32Ret=%#x\n", s32Ret);

    /*
     * 配置VB参数
     * Config VB parameter
     */
    StVbParamCfg(&g_aicMediaInfo.vbCfg);

    /*
     * 视频缓存池初始化以及MPI系统初始化
     * VB init & MPI system init
     */
    s32Ret = SAMPLE_COMM_SYS_Init(&g_aicMediaInfo.vbCfg);
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
    VpssParamCfg();
    s32Ret = ViVpssCreate(&g_aicMediaInfo.viSess, &g_aicMediaInfo.viCfg, &g_aicMediaInfo.vpssCfg);
    SAMPLE_CHECK_EXPR_GOTO(s32Ret != HI_SUCCESS, EXIT1, "ViVpss Sess create FAIL, ret=%#x\n", s32Ret);
    g_aicMediaInfo.vpssGrp = AIC_VPSS_GRP;
    g_aicMediaInfo.vpssChn0 = AIC_VPSS_ZOUT_CHN;

printf("尝试设置旋转属性 \n");
//---------------------------------------------------------------- 尝试设置旋转属性 
/*
g_aicMediaInfo 结构体
enRotation enum 变量

typedef HI_S32 VPSS_GRP; typedef HI_S32 VPSS_CHN;
typedef enum hiROTATION_E
{
    ROTATION_0 = 0, ROTATION_90 = 1, ROTATION_180 = 2, ROTATION_270 = 3, ROTATION_BUTT
} ROTATION_E;

HI_S32 HI_MPI_VPSS_SetChnRotation (VPSS_GRP VpssGrp, VPSS_CHN VpssChn, ROTATION_E enRotation);
设置 VPSS 通道图像固定角度旋转属性。
头文件：hi_comm_vpss.h、mpi_vpss.h 。库文件：libmpi.a
*/

// ROTATION_E enRotation;
// enRotation = ROTATION_90;
// printf("ROTATION_90 \n");
// HI_MPI_VPSS_SetChnRotation(g_aicMediaInfo.vpssGrp, g_aicMediaInfo.vpssChn0, enRotation);// 是可行的
// printf("HI_MPI_VPSS_SetChnRotation SUCCESS \n");
// // VpssGrp 是 GROUP 号。 VpssChn 是 通道号。enRotation 是旋转属性 结构体，具体说明请参考“系统控制”章节。
//---------------------------------------------------------------- 尝试设置旋转属性 

    /*
     * 配置VO参数
     * Config VO parameter
     */
    StVoParamCfg(&g_aicMediaInfo.voCfg);

    /*
     * 启动VO到MIPI lcd通路
     * Start VO to MIPI lcd
     */
    s32Ret = SampleCommVoStartMipi(&g_aicMediaInfo.voCfg);
    SAMPLE_CHECK_EXPR_GOTO(s32Ret != HI_SUCCESS, EXIT1, "start vo FAIL. s32Ret: 0x%x\n", s32Ret);

    /*
     * VPSS绑定VO
     * VPSS bind VO
     */
    s32Ret = SAMPLE_COMM_VPSS_Bind_VO(g_aicMediaInfo.vpssGrp, g_aicMediaInfo.vpssChn0, g_aicMediaInfo.voCfg.VoDev, 0);
    SAMPLE_CHECK_EXPR_GOTO(s32Ret != HI_SUCCESS, EXIT2, "vo bind vpss FAIL. s32Ret: 0x%x\n", s32Ret);
    SAMPLE_PRT("vpssGrp:%d, vpssChn:%d\n", g_aicMediaInfo.vpssGrp, g_aicMediaInfo.vpssChn0);

    /*
     * 创建工作线程运行ai
     * Create work thread to run ai
     */
    s32Ret = HandClassifyAiThreadProcess();
    SAMPLE_CHECK_EXPR_RET(s32Ret != HI_SUCCESS, s32Ret, "ai proccess thread creat fail:%s\n", strerror(s32Ret));

    Pause();
    g_bAiProcessStopSignal = HI_TRUE;
    /*
     * 等待一个线程结束，线程间同步的操作
     * Waiting for the end of a thread, the operation of synchronization between threads
     */
    pthread_join(g_aiProcessThread, NULL);
    g_aiProcessThread = 0;
    PauseDoUnloadHandClassifyModel();

    SAMPLE_COMM_VPSS_UnBind_VO(g_aicMediaInfo.vpssGrp, g_aicMediaInfo.vpssChn0, g_aicMediaInfo.voCfg.VoDev, 0);
    SAMPLE_VO_DISABLE_MIPITx(fd);
    SampleCloseMipiTxFd(fd);
    system("echo 0 > /sys/class/gpio/gpio55/value");

EXIT2:
    SAMPLE_COMM_VO_StopVO(&g_aicMediaInfo.voCfg);
EXIT1:
    VpssStop(&g_aicMediaInfo.vpssCfg);
    SAMPLE_COMM_VI_UnBind_VPSS(g_aicMediaInfo.viCfg.astViInfo[0].stPipeInfo.aPipe[0],
        g_aicMediaInfo.viCfg.astViInfo[0].stChnInfo.ViChn, g_aicMediaInfo.vpssGrp);
    ViStop(&g_aicMediaInfo.viCfg);
    free(g_aicMediaInfo.viSess);
EXIT:
    SAMPLE_COMM_SYS_Exit();
    return s32Ret;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
