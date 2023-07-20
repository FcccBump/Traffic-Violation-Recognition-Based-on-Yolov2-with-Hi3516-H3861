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
        可以将 yuv 图片保存到本地了。
0601：
    实现了将 yuv 图片转成 jpg 图片并保存到本地。
0602：
    实现了 socket 连接云服务器。
0604：
    1651:
        实现了将 yuv 图片转成 jpg 图片并保存到本地。读取本地 jpg 图片并发送给云服务器。
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

// 音频相关如下
#include <sys/prctl.h>// PR_SET_NAME
// #include "posix_help.h"// SkPair ， SkPairCreate() ， FdReadMsg() ，不能在这里 include ，否则报错
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

// yuv 转 jpg
#include <jpeglib.h>// 使用SDK自带库，其他库都不需要
// #include "jpeglib.h"
// #include <jmorecfg.h> // 加上这个编译就会错误
// #include <jerror.h>
// #include <jconfig.h>
// yuv 转 jpg

// tcp 相关如下
// #include <stdio.h>
// #include <stdlib.h>
// #include <unistd.h>
// #include <string.h>
// #include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
// tcp 相关如上

// udp 相关如下
// #include <netinet/in.h>                         // for sockaddr_in  
// #include <sys/types.h>                          // for socket  
// #include <sys/socket.h>                         // for socket  
// #include <stdio.h>                              // for printf  
// #include <stdlib.h>                             // for exit  
// #include <string.h>                             // for bzero 
// #include <fcntl.h>
// #include <unistd.h>
// #include <arpa/inet.h>
#include <sys/time.h> 
// udp 相关如上

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

#define AUDIO_SCORE           40    // Confidence can be configured by yourself
#define AUDIO_FRAME           14    // Recognize once every 15 frames, can be configured by yourself
#define MULTIPLE_OF_EXPANSION 100   // Multiple of expansion
#define UNKOWN_WASTE          20    // Unkown Waste
#define BUFFER_SIZE           16    // buffer size
#define MIN_OF_BOX            16    // min of box
#define MAX_OF_BOX            240   // max of box
// 音频相关如上

// tcp 相关如下
#define SERVER_IP2 "123.60.185.153"// 这个是购买的华为云服务器的 ip地址，也是最后要连接的。
#define SERVER_PORT2 3880 // 连接华为云服务器的端口号
// #define SERVER_IP1 "100.107.18.75"// 这个是 APP 连接敬萱的热点后的 ip地址。
// #define SERVER_IP1 "10.106.187.67"// my 热点 第一次的
// #define SERVER_IP1 "10.107.95.225"// my 热点 0420_1202
// #define SERVER_IP1 "10.107.48.207"// my 热点 0421_1459
#define SERVER_IP1 "10.140.19.60"// my 热点 0503_2017
#define SERVER_PORT1 10000 // 连接 APP 的端口号。

#define BufSize 1024
// tcp 相关如上

// udp 相关如下
// #define SERVER_PORT      		        8888
#define SERVER_PORT      		        9001// UDP
#define BUFFER_SIZE                   	1024  
#define FILE_NAME_MAX_SIZE            	512  
// #define SERVER_IP "10.106.187.67" // UDP my 热点 第一次的
// #define SERVER_IP "10.107.95.225"// UDP my 热点 0420_1202
// #define SERVER_IP "10.107.48.207"// my 热点 0421_1459
#define SERVER_IP "10.140.19.60"// my 热点 0503_2017
// udp 相关如上

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
// 音频相关如上

//////////////////////// socket
/////////////////// http
static int sockfd1;// APP
static int sockfd2;// Huawei_cloud
static int send_to_Huawei_cloud=0;
static int count_send_to_Huawei_cloud=0;
/////////////////// http
//////////////////////////////// udp
static int client_socket;
struct sockaddr_in  server_addr;
int server_addr_length;
int send_to_APP=0;
char sendto_APP_file_name[128]="/demo/bin/test7.jpg";int length=0;
char start_send_to_APP[10]="1111";char finish_send_to_APP[10]="0000";
char buffer[BUFFER_SIZE];char buffer_end[128] = "endendend!@!2!@!";// 结束标志
//////////////////////////////// udp
//////////////////////// socket

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

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ jpg 相关如下 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */ 
int yuv420sp_to_jpeg1(HI_CHAR * filename, IVE_IMAGE_S *pstImg, int quality)
{
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);

    FILE * outfile;// target file  
    if ((outfile = fopen(filename, "wb")) == HI_NULL) {
        printf("can't open file\n");  
    }
    printf("yuv_to_jpg 000 \n");
    jpeg_stdio_dest(&cinfo, outfile);
    printf("yuv_to_jpg 111 \n");

int image_width, image_height, Stride0, Stride1;//00000000000000000000000000000000000000000
image_width = pstImg->u32Width;
image_height = pstImg->u32Height;
Stride0=pstImg->au32Stride[0];
Stride1=pstImg->au32Stride[0];
printf("image_width = %d, image_height = %d\n", image_width, image_height);
printf("Stride0 = %d, Stride1 = %d\n", Stride0, Stride1);
// image_width=640, image_height=384, Stride0=640, Stride1=640

cinfo.image_width = pstImg->u32Width;// image width and height, in pixels  
cinfo.image_height = pstImg->u32Height;//00000000000000000000000000000000000000000

    cinfo.input_components = 3;// # of color components per pixel
    cinfo.in_color_space = JCS_YCbCr;//colorspace of input image 
    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, TRUE );// 一般 quality=100
    printf("yuv_to_jpg 222 \n");
    //  cinfo.raw_data_in = TRUE;
    cinfo.jpeg_color_space = JCS_YCbCr;
    cinfo.comp_info[0].h_samp_factor = 2;
    cinfo.comp_info[0].v_samp_factor = 2;

    jpeg_start_compress(&cinfo, TRUE);  
    printf("yuv_to_jpg 333 \n");
    JSAMPROW row_pointer[1];

    unsigned char *yuvbuf;// unsigned char 等价于 HI_U8
    if((yuvbuf=(unsigned char *)malloc(image_width*3))!=NULL)
        memset(yuvbuf,0,image_width*3);
    printf("yuv_to_jpg 444 \n");
    unsigned char *ybase,*ubase;

//00000000000000000000000000000000000000000
// unsigned char* pdata_y;
// unsigned char* pdata_u;
// pdata_y=&(pstImg->au64VirAddr[0]);
// pdata_u=&(pstImg->au64VirAddr[1]);
// ybase=pdata_y;
// ubase=pdata_u;
//00000000000000000000000000000000000000000

//00000000000000000000000000000000000000000
/*
    获取 yuv 格式的图片数据的参考
    HI_U8 *pU8 = NULL;
    pU8 = SAMPLE_COMM_IVE_CONVERT_64BIT_ADDR(HI_U8, pstImg->au64VirAddr[0]);
*/

HI_U8 *pU8_y = NULL;
HI_U8 *pU8_u = NULL;
// printf("length(pstImg->au64VirAddr[0])=%d, length(pstImg->au64VirAddr[1])=%d \n", 
    // strlen(pstImg->au64VirAddr[0]), strlen(pstImg->au64VirAddr[1]));// 这个 printf 会 error
printf("yuv_to_jpg 555 \n");
pU8_y= SAMPLE_COMM_IVE_CONVERT_64BIT_ADDR(HI_U8, pstImg->au64VirAddr[0]);
pU8_u= SAMPLE_COMM_IVE_CONVERT_64BIT_ADDR(HI_U8, pstImg->au64VirAddr[1]);
// ybase=(const char*)pU8_y;
// ubase=(const char*)pU8_u;
ybase=(unsigned char*)pU8_y;
ubase=(unsigned char*)pU8_u;
//00000000000000000000000000000000000000000

    int j=0;
    while (cinfo.next_scanline < cinfo.image_height) 
    {
        int idx=0;
        for(int i=0; i<image_width; i++)
        {
            yuvbuf[idx++]=ybase[i + j * image_width];
            yuvbuf[idx++]=ubase[j / 2 * image_width + (i/2) * 2];
            yuvbuf[idx++]=ubase[j / 2 * image_width + (i/2) * 2 + 1];
        }
        row_pointer[0] = yuvbuf;
        jpeg_write_scanlines(&cinfo, row_pointer, 1);
        j++;
        if(j>380) printf("j=%d\n",j);// j 确实循环到384
    }
    /*
        error_0602_0050，运行一段时间，大概5分钟后突然卡住了，串口终端不打印东西了： [0][W:4815]{hmac_set_psm_timeout::pm off.}

        error: Application transferred too few scanlines 。因为 while (cinfo.next_scanline < cinfo.image_height) 循环次数
        不够

        error: Output file write error --- out of disk space? 就是 jpeg_write_scanlines(&cinfo, row_pointer, 1) 这个不对，
        可能是当时库文件没弄好。因为当时代码编译时自己包括了一个别的地方带来的 .so 库，实际上板端已经指定了，所以只需要 -ljpeg 即可。

    */

    printf("yuv_to_jpg 666 \n");
    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
    fclose(outfile);
    free(yuvbuf);// 00000000000000000000000000000000000000000000000000
    printf("yuv_to_jpg end \n");
    return 0;
}
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ jpg 相关如上 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */ 

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

// ///////////////////////////////////////////// 连接 Huawei_cloud socket 初始化如下 /////////////////////////////////////////////
struct sockaddr_in server_addr2 = {0};
/* 打开套接字，得到套接字描述符 */
sockfd2 = socket(AF_INET, SOCK_STREAM, 0);
if (0 > sockfd2) {perror("socket error");exit(EXIT_FAILURE);}
printf("link to Huawei_cloud 000\n");
/* 调用 connect 连接远端服务器 */
server_addr2.sin_family = AF_INET;
server_addr2.sin_port = htons(SERVER_PORT2);//端口号
inet_pton(AF_INET, SERVER_IP2, &server_addr2.sin_addr);//将服务器的 IP 地址 存入 结构体 server_addr
printf("link to Huawei_cloud 111\n");
ret = connect(sockfd2, (struct sockaddr *)&server_addr2, sizeof(server_addr2));
if (0 > ret) {
    printf("服务器连接失败 服务器连接失败 服务器连接失败 服务器连接失败 服务器连接失败 服务器连接失败\n");
  // perror("connect error");
    close(sockfd2);
    printf("服务器连接失败 服务器连接失败 服务器连接失败 服务器连接失败 服务器连接失败 服务器连接失败\n");
}
else{
        for(int i = 0; i < 5; i++){
            printf("服务器连接成功 服务器连接成功 服务器连接成功 服务器连接成功 服务器连接成功 服务器连接成功\n");
            printf("...................................... 服务器连接成功 ...................................... \n");
            printf("服务器连接成功 服务器连接成功 服务器连接成功 服务器连接成功 服务器连接成功 服务器连接成功\n");
        }
}
sleep(1);
// ///////////////////////////////////////////// 连接 Huawei_cloud socket 初始化如上 /////////////////////////////////////////////



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
    HI_S32 resLen = 0;// 在 hi_type.h 中，51。还有一些其他的海思自定义的类型

    int objNum;// 存储模型识别一帧图片得到的目标数
    int ret;
    int num = 0;

// typedef struct hiIVE_IMAGE_S {
//     HI_U64 au64PhyAddr[3];   /* RW;The physical address of the image */

//     HI_U64 au64VirAddr[3];   /* RW;The virtual address of the image */
//==== 三个数组，但是只使用通道0、1，所以只用前两个数组

//     HI_U32 au32Stride[3];    /* RW;The stride of the image */
//     HI_U32 u32Width;         /* RW;The width of the image */
//     HI_U32 u32Height;        /* RW;The height of the image */
//     IVE_IMAGE_TYPE_E enType; /* RW;The type of the image */
// } IVE_IMAGE_S;

    ret = FrmToOrigImg((VIDEO_FRAME_INFO_S*)srcFrm, &img);// 在 ive_img.c 中，52。 srcFrm 是宽640*高384的。
    // 将帧缩放后的一帧转为初始的 yuv 图片
    // 将 *VIDEO_FRAME_INFO_S 格式转换成 IVE_IMAGE_S 格式，具体是 YUV420/422 ，但是到底是哪一个还不清楚
    // 根据 FrmToOrigImg 函数里面的打印信息可知是 IVE_IMAGE_TYPE_YUV420SP
    SAMPLE_CHECK_EXPR_RET(ret != HI_SUCCESS, ret, "hand detect for YUV Frm to Img FAIL, ret=%#x\n", ret);

// ====================================== 以下尝试将 img 保存到磁盘 ======================================
        HI_CHAR achDstFileName[IVE_FILE_NAME_LEN];// 在 sample_comm_ive.h 中，41。 #define IVE_FILE_NAME_LEN   256
        if (snprintf_s(achDstFileName, sizeof(achDstFileName), sizeof(achDstFileName) - 1,
            "/userdata/complete_%s.yuv", "resize") < 0) {
            HI_ASSERT(0);
        }
        HI_CHAR* achDstFileName1;
        achDstFileName1=&achDstFileName;
        FILE *pFp;
        // FILE *fopen(const char *restrict filename, const char *restrict mode)
        pFp=fopen(achDstFileName1, "wb");
        int i=0;
        for(i=0; i<1 ; i++){
            printf("开始保存到磁盘 开始保存到磁盘 开始保存到磁盘 开始保存到磁盘 开始保存到磁盘 开始保存到磁盘\n");
        }

        // HI_S32 SAMPLE_COMM_IVE_WriteFile(IVE_IMAGE_S *pstImg, FILE *pFp)
        SAMPLE_COMM_IVE_WriteFile(&img, pFp);// 好像不用 #include "sample_comm_ive.h"
        /*
            运行时报错，因为输入的参数：要打开的文件不对，现在可以运行了，但是没有结果。因为输入的参数还是错的，
            打印信息：func:SAMPLE_COMM_IVE_WriteFile, line:272, NULL pointer。
            分析：空指针，输入指针才不会报错，但是空指针说明输入还是错的，现在对了。
            总结：fopen 的输入必须是指针，该指针需要指向一个字符数组的首地址，字符数组由 snprintf_s 得到。
        */
        for(i=0; i<1 ; i++){
            printf("结束保存到磁盘 结束保存到磁盘 结束保存到磁盘 结束保存到磁盘 结束保存到磁盘 结束保存到磁盘\n");
        }
// ====================================== 以上尝试将 img 保存到磁盘 ======================================

// ====================================== 以下尝试将 yuv 转 jpg 并保存到磁盘 ======================================
        HI_CHAR jpgFileName0[IVE_FILE_NAME_LEN];
        if (snprintf_s(jpgFileName0, sizeof(jpgFileName0), sizeof(jpgFileName0) - 1,
            "/userdata/complete_%s.jpg", "resize") < 0) {
            HI_ASSERT(0);
        }
        HI_CHAR* jpgFileNamepointer0;
        jpgFileNamepointer0=&jpgFileName0;
        /*
            因为写入磁盘是也是使用 au64VirAddr ，所以现在转成 jpg 应该也是用 au64VirAddr 。但是它有三个数组，
            写入磁盘时用了前两个，现在？也是前两个，第一个是y，第二个是uv。
        */
        // FILE *fopen(const char *restrict filename, const char *restrict mode)
        // int yuv420sp_to_jpeg1(HI_CHAR * filename, IVE_IMAGE_S *pstImg, int quality)
        yuv420sp_to_jpeg1(jpgFileNamepointer0, &img, 100);
// ====================================== 以上尝试将 yuv 转 jpg 并保存到磁盘 ======================================
    printf("2032_0604_1640\n");
    usleep(1000);// 这里需要一个延时，因为上面刚刚讲一个 yuv 图片数据以 jpg 写入本地，下面就要打开这个 jpg 图片读取数据
    // 。如果不延时，就会出现打开后的文件数据缺失、混乱的问题。
// ====================================== 以下读取硬盘 jpg 图片再发送 ======================================
    printf("读取硬盘 jpg 图片再发送流程开始！\n");
    /*
    // char *buf[614989] = {0};// 614989 足够大
    // char buf_end[128] = "endendend!@!2!@!";
    // char buf_begin[128] = "beginbeginbegin!@!2!@!";
    // 上述这三个 char 的定义会报错，具体哪一个还不清楚！！！
    */
    HI_CHAR buf_end[IVE_FILE_NAME_LEN];HI_CHAR buf_begin[IVE_FILE_NAME_LEN];
    if (snprintf_s(buf_end, sizeof(buf_end), sizeof(buf_end) - 1,
        "endendend!@!2!@!") < 0) {
        HI_ASSERT(0);
    }printf("initialize buf_end \n");
    if (snprintf_s(buf_begin, sizeof(buf_begin), sizeof(buf_begin) - 1,
        "beginbeginbegin!@!2!@!") < 0) {
        HI_ASSERT(0);
    }printf("initialize buf_begin \n");
    // HI_CHAR *buf[614989] = {0}; //================================================== 这种定义 error
    /*
        打开文件，读取数据的方法可以参考很多例程，比如 nnie, resize_crop 等
        在 sample_nnie.c 中，298。
        SAMPLE_SVP_NNIE_FillSrcData(){
            HI_U8 *pu8PicAddr = NULL;
            for (i = 0; i < *(pu32StepAddr + n); i++) {// 可能是每次读取一行
                    ulSize = fread(pu8PicAddr, u32Dim * u32VarSize, 1, fp);
                    SAMPLE_SVP_CHECK_EXPR_GOTO(ulSize != 1, FAIL, SAMPLE_SVP_ERR_LEVEL_ERROR,
                        "Error,Read image file failed!\n");
                    pu8PicAddr += u32Stride;// 每次读取一行，所以这里每次加一行的长度，也就是宽度（跨度）
                }
        }
    */
    HI_U8 *buf = NULL;// 仅仅这样使用，会导致 fread 时失败，因为该指针没有相应的空间存储 fread 读到的数据。
    /*
        可以参考上面的 yuv420sp_to_jpeg1() 函数里面使用分配内存的方法：
        if((yuvbuf=(unsigned char *)malloc(image_width*3))!=NULL)
        memset(yuvbuf,0,image_width*3);
    */
    int buf_max_length = 120000;
    /*
        现在的 jpg 图片大小是640*384，大概是 105000 个字节。因为是从文件中读取，
        所以每次很可能略有不同，所以最大值要设的大一些。这里一开始不知道为啥出错了，读取 jpg 文件时只有 48300 个字节。
        然后图片也是混乱的，条纹状。后来发现是读写该文件的代码是连续的，可能内部某些处理还没结束，所以手动延时了1ms，
        结果正确。
    */

    if((buf=(unsigned char *)malloc(buf_max_length))!=NULL)// 使用 malloc 分配内存
    memset(buf, 0, buf_max_length);// 使用 memset 初始化
    printf("initialize buf \n");

    FILE *tocloud_fp = NULL;
    int size;int length;
    printf("读取硬盘 jpg 图片再发送！\n");
    /* 打开文件 */
    if ((tocloud_fp = fopen(jpgFileNamepointer0, "rb")) == HI_NULL) {// right，rb表示只读一个二进制文件
    // if ((tocloud_fp = fopen(jpgFileNamepointer0, "r")) == HI_NULL) {// right，r表示只读
        printf("can't open \n");
    }
    printf("待发送的 jpg 文件打开成功！\n");
    /* 将读写位置移动到文件尾部 */
    if (0 > fseek(tocloud_fp, 0, SEEK_END)) {printf("fseek error");}
    printf("将读写位置移动到文件尾部！\n");
    /* 获取当前位置偏移量 */
    if (0 > (length = ftell(tocloud_fp))) {printf("ftell error");}
    printf("文件大小: %d 个字节\n", length);
    /* 将读写位置移动到文件头部 */
    if (0 > fseek(tocloud_fp, 0, SEEK_SET)) {printf("fseek error");}
    printf("将读写位置移动到文件头部！\n");
    if(buf_max_length>length) {
        printf("待发送文件字节数 <<< <<< <<< <<< <<< 预设内存最大容量\n");
        if (1 > (size = fread(buf, length, 1, tocloud_fp))) {// 读取一个数据项。/* 如果未发生错误则意味着已经到达了文件末尾 */
        printf("fread error\n");}
        printf("成功读取 %d 个字节数据\n", size*length);
        // 因为上述 fread 参数设置就是会读取一个数据项，长度 length 个字节，最后返回的 size 其实是数据项个数。
        // extern ssize_t send (int __fd, const void *__buf, size_t __n, int __flags);
        ret = send(sockfd2, buf, length, 0);// 如果是字符数组buf，那么就要用&buf。
        // 如果是字符数组指针buf，那就直接send(buf)。// 需要单独发送结束标志
        if(0 > ret){printf("send error send error send error send error send error send error\n");}
        printf("....................... 发送图片结束 发送图片结束 ....................... \n");
        printf("发送图片结束 发送图片结束 ....................... 发送图片结束 发送图片结束 \n");
        printf("....................... 发送图片结束 发送图片结束 ....................... \n");
        // ret = send(sockfd2,&buf_end,strlen(buf_end),0);// 发送结束标志
        ret = send(sockfd2, buf_end, strlen(buf_end), 0);// 发送结束标志
        if(0 > ret){printf("send error send error send error send error send error send error\n");}
        printf("strlen(buf_end)=%d   send end_flag over\n", strlen(buf_end));
    }
    else printf("待发送文件字节数 >>> >>> >>> >>> >>> 预设内存最大容量\n");
    /* 关闭文件 */
    fclose(tocloud_fp);
    free(buf);
    sleep(1);// 需要延时，否则发送图片会出错。
    printf("读取硬盘 jpg 图片再发送流程到此结束！\n");
// ====================================== 以上读取硬盘 jpg 图片再发送 ======================================s

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
