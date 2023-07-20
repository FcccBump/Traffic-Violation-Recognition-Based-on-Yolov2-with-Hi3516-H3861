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
        实现了将 yuv 图片转成 jpg 图片并保存到本地。读取本地 jpg 图片并发送给云服务器。但是 libjpeg.so 库有问题，所以
        红色和蓝色互换了。
    1949：
        实现了将 yuv 图片先转成 bgr 格式数据，再转为 jpg 图片并保存在本地。解决了红色和蓝色互换的问题。
        读取本地 jpg 图片并发送给云服务器。
0605:
    0032：
        微调了 “发送图片给云服务器的控制程序” ，添加全局变量参与控制。还没测试。
    0100：
        添加了 “tcp 连接 app 线程” ，还没测试。
    1327：
        使用了 “发送图片给云服务器的控制程序” ，但是没有连接云并测试。效果是在实际没有手的情况下也检测到了，并播放音频。
        就是模型准确率差了很多。不知道是因为新开了仿照 “音频播放线程” 的 “tcp 连接 app 线程” 导致的，还是别的。

        测试了 “tcp 连接 app 线程” ，可行。增加了 udp 给 app 发送的功能，可以发送，
        但是 app 没有显示实时图片，可能的原因是板端读取文件失败，没读到数据！！！
0605:
    1040:
        昨天的给 app 发送图片， app 没有显示的问题是因为 ip 不对，昨天没改对。今天启用了服务器相关的程序，重新试了一下
        给 app 发送图片，形成的视频流不错。到此， socket 通信基本完成。
        存在的问题： 音频播放程序有点混乱，明明没有识别到，但是却一直播放。不知道是误识别还是别的原因。
    2300:
        注释了和 app 相关的，直接从内存向 cloud 发送图片，可行。效率好像并没有比从磁盘读取再发送要高。
0607:
    0236:
        尝试添加将画框后的图片发送给 cloud 的代码，失败。
0609:
    1302:
        先不画框，全部从内存发送图片，给 cloud 是10帧发一帧， cloud 接收到是6s一帧。感觉比读取磁盘文件再发送变慢了。
    1510:
        加上画框了，可以跑，但是几分钟后崩溃。
    1613：
        把加框注释了，但是有 srcFrm2 输入，跑10分钟崩溃。
    1649:
        把 srcFrm2 都注释了，把将 yuv 保存到磁盘注释了。还是8分钟报错。把 app 的 tcp 线程注释了，还是报错。
    1816：
        排查出至少 bgr_to_jpg_memmory1() 函数有问题！！！导致跑10分钟崩溃。单纯获取 rgb 内存的函数就可以运行十几分钟。
    2003:
        在 bgr_to_jpg_memmory1() 函数里面怎么搞都是错的，那个压缩函数分配了内存，然后就用这个来发送，再释放，结果都是错的。
    2200:
        发现即使注释了 bgr_to_jpg_memmory1() 函数，还是10分钟报错。继续注释，全部改用保存到磁盘的函数，还是10分钟报错。
    2400：
        发现了真正的问题，是 fopen 了一个文件，但是没有 fclose ，就是最开始把 yuv 保存到本地的测试代码使用的文件描述符。
0610：
    1200：
        fclose 了之前打开但是忘记关闭的文件描述符。也启用了 音频播放线程， tcp_app 线程，连接 cloud 的 tcp 初始化，udp 初始化。
        先把图片保存到磁盘，再读取后发送给 app 。 给 cloud 发送图片的流程是复制了初始化的 yuv 内存图片数据，再根据模型结果画框，
        然后保存到磁盘，再读取后发送给 cloud 。板端每10帧（不是识别到的10帧，就是直接10帧）发一帧，云服务器接收的是7s一帧。
        给 app 发送的视频流也是挺好的，有明显延时，但是不丢包，不乱序，总的来说可以。运行了一个小时，都没崩溃。
        问题：
            运行一段时间，可能是10分钟，会卡一段时间。这时板端不运行主线程了，服务器上却1s收到一帧图片，但都是坏的。大概1~2分钟
            后板端又变好了，云服务器也好了。
    1400：
        尝试从内存直接发送。发现不止上面发现的错误， bgr_to_jpg_memmory1() 函数也是错的！！！不能用，一用就是10分钟报错！！！
        所以暂时放弃从内存直接发送。
0615:
    1800:
        将官方例程的模型替换为我们自己的模型，然后启用云服务器，app，都是先存储到磁盘，可行。
        注意云服务器要打开 apptcp.js ，否则程序直接崩溃。
        注意摄像头正反！！！不然反着模型识别不出来。
0617:
    2300:
        修改了一点。int cycle_Boundary_point = 4;// 经过测试，大概2~3s收到一帧。

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

// 图片格式转换相关
// yuv 转 jpg
#include <jpeglib.h>// 使用SDK自带库，其他库都不需要
// #include "jpeglib.h"
// #include <jmorecfg.h> // 加上这个编译就会错误
// #include <jerror.h>
// #include <jconfig.h>
// yuv 转 jpg

// yuv 转 bgr

// yuv 转 bgr

// bgr 转 jpg

// bgr 转 jpg
// 图片格式转换相关

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

// 图片格式转换相关
#include "hand_classify.h"// JPG_BUFTYPE 定义在里面了
// 图片格式转换相关

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
// #define SERVER_IP1 "10.140.19.60"// my 热点 0503_2017
// #define SERVER_IP1 "10.139.30.245"// my 热点 0605_1011
// #define SERVER_IP1 "10.137.248.116"// my 热点 0606_1036
// #define SERVER_IP1 "10.139.30.86"// my 热点 0609_1213
// #define SERVER_IP1 "10.139.168.166"// my 热点 0610_1204
#define SERVER_IP1 "10.36.29.197"// my 热点 0617_2200
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
// #define SERVER_IP "10.137.248.116"// my 热点 0606_1036
// #define SERVER_IP "10.139.30.86"// my 热点 0609_1213
// #define SERVER_IP "10.139.168.166"// my 热点 0610_1204
#define SERVER_IP "10.36.29.197"// my 热点 0617_2200
// udp 相关如上

// 图片格式转换相关
#define JPEG_QUALITY 100 //图片质量
// 图片格式转换相关

static int biggestBoxIndex;
static IVE_IMAGE_S img;
static IVE_IMAGE_S img2;////////////////////////////////

static DetectObjInfo objs[DETECT_OBJ_MAX] = {0};
static RectBox boxs[DETECT_OBJ_MAX] = {0};
static RectBox boxs_initial[DETECT_OBJ_MAX] = {0};////////////////////////////////
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
static HI_S32 global_audio_flag = 0;
// 音频相关如上

//////////////////////// socket
/////////////////// tcp
static int sockfd1;// APP
static int sockfd2;// Huawei_cloud
static int send_to_Huawei_cloud=0;
static int count_send_to_Huawei_cloud=0;
static pthread_t tcp_app_ProcessThread = 0;
/////////////////// tcp
//////////////////////////////// udp
static int udp_client_socket;
struct sockaddr_in  server_addr;
int server_addr_length;
int send_to_APP=0;
char start_send_to_APP[10]="1111";char finish_send_to_APP[10]="0000";
char buffer[BUFFER_SIZE];char buffer_end[128] = "endendend!@!2!@!";// 结束标志
//////////////////////////////// udp
HI_CHAR buf_end[IVE_FILE_NAME_LEN];HI_CHAR buf_begin[IVE_FILE_NAME_LEN];

// 将 jpg 保存到内存相关如下
JPG_BUFTYPE jpg_buf1 = {0};// app
JPG_BUFTYPE jpg_buf2 = {0};// cloud
unsigned long jpg_buf_size1=0;// app
unsigned long jpg_buf_size2=0;// cloud
// 将 jpg 保存到内存相关如上
//////////////////////// socket

// static SkPair g_stmChn = {
//     .in = -1,
//     .out = -1
// };

// ai_control_audio 相关如下
static int global_audio_objNum = 0;
static float cnnScores[DETECT_OBJ_MAX];
// ai_control_audio 相关如上

// ai_control_cloud 相关如下
static int count_cycle = 0;
// ai_control_cloud 相关如上

// 提前声明如下
static int sendto_app2(JPG_BUFTYPE *jpg_buf);
// 提前声明如上

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
    if ((score > AUDIO_SCORE) && (g_num != item->num)) {// 满足条件才会执行音频播放程序。 AUDIO_SCORE = 40 ，定义在前面。 
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
    if  (g_count < AUDIO_FRAME) {// AUDIO_FRAME = 14 。相当于进入 PlayAudio2() 函数的前14次都直接返回，
    // 第15次才判断是否需要播放音频。还是进入该函数后先计数到14才执行下面的程序？
        g_count++;
        return;
    }
    const RecogNumInfo *item = &items;
    uint32_t score = item->score * MULTIPLE_OF_EXPANSION / SCORE_MAX;// 1800*100/4096
    if ((score > AUDIO_SCORE) && (global_audio_objNum != 0) && (global_audio_flag == 0)) {
        // 满足条件才会执行音频播放程序。 AUDIO_SCORE = 40 ，定义在前面 。所以 item->score 最小为1639。
        global_audio_objNum = 0;global_audio_flag = 1;
        g_num = item->num;
        for(int i = 0; i < 2; i++){
            printf("满足条件： global_audio_objNum = %d, global_audio_flag = %d, 执行音频播放程序 \n", 
                global_audio_objNum, global_audio_flag);
        }
        if (g_num != UNKOWN_WASTE) {// UNKOWN_WASTE = 20
            AudioTest(g_num, -1);// 在 audio_test.c 中，482。 g_num 是识别出来的垃圾类别。
        }
        printf("播放结束时： global_audio_flag = %d, global_audio_flag = %d \n", global_audio_flag, global_audio_flag);
        global_audio_flag = 0;
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
        resBuf.num = 1;// 指定播放音频的类别
        resBuf.score = 1800;// 手动指定置信度，保证一定能播放。
        // PlayAudio(resBuf);// 单纯播放指定文件
        PlayAudio2(resBuf);// 受手势识别模型的输出参数 objs 的控制
    }

    return NULL;
}
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ 音频相关如上 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */ 

static HI_VOID* tcp_app(HI_VOID* arg)
{
    int ret;
    for(int i = 0; i < 4; i++){
        printf("tcp_111_app 循环，为了接收 APP 传来的开始、结束发送图片命令 \n");
    }

    HI_U8 *recvbuf = NULL;
    int recvbuf_max_length = 4;
    if((recvbuf=(unsigned char *)malloc(recvbuf_max_length))!=NULL)// 使用 malloc 分配内存
        memset(recvbuf, 0, recvbuf_max_length);// 使用 memset 初始化
    printf("tcp_111_app initialize buf \n");

// ==================================== 连接 APP socket 初始化如下 ====================================
    struct sockaddr_in server_addr1 = {0};
    while(1){
        printf("tcp_111_app 连接 APP socket \n");
        sockfd1 = socket(AF_INET, SOCK_STREAM, 0);/* 打开套接字，得到套接字描述符 */
        if (0 > sockfd1) {printf("tcp_111_app socket error \n");}
        // printf("new_thread_start 000\n");
        /* 调用 connect 连接远端服务器 */
        server_addr1.sin_family = AF_INET;
        server_addr1.sin_port = htons(SERVER_PORT1);// 端口号
        inet_pton(AF_INET, SERVER_IP1, &server_addr1.sin_addr);// 将服务器的 IP 地址 存入 结构体 server_addr
        // printf("new_thread_start 111\n");
        ret = connect(sockfd1, (struct sockaddr *)&server_addr1, sizeof(server_addr1));
        if (0 > ret) {
            for(int i = 0; i < 4; i++){
                printf("连接 APP 失败 连接 APP 失败 连接 APP 失败 连接 APP 失败 连接 APP 失败 连接 APP 失败 \n");
            }
            close(sockfd1);
        }
        else{
            for(int i = 0; i < 2; i++){
                printf("APP 连接成功 APP 连接成功 APP 连接成功 APP 连接成功 APP 连接成功 APP 连接成功 \n");
                printf("APP 连接成功                                                  APP 连接成功 \n");
                printf("            APP 连接成功                          APP 连接成功\n");
                printf("                        APP 连接成功 APP 连接成功\n");
                printf(" ............................. APP 连接成功 ............................. \n");
                printf("                        APP 连接成功 APP 连接成功\n");
                printf("            APP 连接成功                          APP 连接成功\n");
                printf("APP 连接成功                                                  APP 连接成功 \n");
                printf("APP 连接成功 APP 连接成功 APP 连接成功 APP 连接成功 APP 连接成功 APP 连接成功 \n");
            }
            break;
        }
        sleep(1);// 这里间隔1us会导致其他线程直接失效。间隔长一点试一下
        // usleep(1000);
    }
// ==================================== 连接 APP socket 初始化如上 ====================================
    
// ==================================== 接收 APP 传来的开始、结束发送命令的模块如下 ====================================
    while(1){
        memset(recvbuf, 0x0, sizeof(recvbuf));// 接收缓冲区清零
        printf("tcp_111_app recv data send by sockfd1 \n");
        ret = recv(sockfd1, recvbuf, sizeof(recvbuf), 0);// 读数据
        // printf("from app: %s\n", recvbuf);// 将读取到的数据以字符串形式打印出来
        // 如果读取到"1111"则准备向 APP 发送图片
        if (0 == strncmp("1111", recvbuf, 4)) {
            send_to_APP=1;
            for(int i = 0; i < 4; i++){
                printf("哈哈哈哈哈哈哈哈哈哈哈哈哈哈哈哈哈哈哈哈哈哈哈哈哈哈哈哈哈哈哈哈哈哈哈哈哈哈哈哈哈哈哈哈哈哈哈哈 \n");
            }
            for(int i = 0; i < 4; i++){
                printf("send_to_APP=1 开始向 APP 发送图片 send_to_APP=1 \n");
            }
        }
        // 如果读取到"1111"则准备向 APP 发送图片
        if (0 == strncmp("0000", recvbuf, 4)) {
            send_to_APP=0;
            for(int i = 0; i < 4; i++){
                printf("啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊 \n");
            }
            for(int i = 0; i < 4; i++){
                printf("send_to_APP=0 结束向 APP 发送图片 send_to_APP=0 \n");
            }
        }
        // usleep(1000);
        sleep(1);
    }
// ==================================== 接收 APP 传来的开始、结束发送命令的模块如上 ====================================
    free(recvbuf);
    return NULL;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ yuv_to_jpg below ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */ 
int yuv420sp_to_jpeg1(HI_CHAR * filename, IVE_IMAGE_S *pstImg, int quality)
{
    printf("yuv_to_111_jpg start \n");
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);

    FILE * outfile;// target file  
    if ((outfile = fopen(filename, "wb")) == HI_NULL) {
        printf("can't open file\n");  
    }
    printf("yuv_to_111_jpg open file success \n");
    jpeg_stdio_dest(&cinfo, outfile);

//00000000000000000000000000000000000000000
int image_width, image_height, Stride0, Stride1;
image_width = pstImg->u32Width;
image_height = pstImg->u32Height;
Stride0=pstImg->au32Stride[0];
Stride1=pstImg->au32Stride[0];
printf("yuv_to_111_jpg image_width = %d, image_height = %d \n", image_width, image_height);
printf("yuv_to_111_jpg Stride0 = %d, Stride1 = %d \n", Stride0, Stride1);
// image_width=640, image_height=384, Stride0=640, Stride1=640
cinfo.image_width = pstImg->u32Width;// image width and height, in pixels  
cinfo.image_height = pstImg->u32Height;
//00000000000000000000000000000000000000000

    cinfo.input_components = 3;// # of color components per pixel
    cinfo.in_color_space = JCS_YCbCr;//colorspace of input image 
    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, TRUE );// 一般 quality=100
    //  cinfo.raw_data_in = TRUE;
    cinfo.jpeg_color_space = JCS_YCbCr;
    cinfo.comp_info[0].h_samp_factor = 2;
    cinfo.comp_info[0].v_samp_factor = 2;
    jpeg_start_compress(&cinfo, TRUE);
    JSAMPROW row_pointer[1];
    printf("yuv_to_111_jpg initialize cinfo success \n");

    unsigned char *yuvbuf;// unsigned char 等价于 HI_U8
    if((yuvbuf=(unsigned char *)malloc(image_width*3))!=NULL)
        memset(yuvbuf,0,image_width*3);
    printf("yuv_to_111_jpg malloc memory success \n");
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
printf("yuv_to_111_jpg finish compless \n");
pU8_y= SAMPLE_COMM_IVE_CONVERT_64BIT_ADDR(HI_U8, pstImg->au64VirAddr[0]);
pU8_u= SAMPLE_COMM_IVE_CONVERT_64BIT_ADDR(HI_U8, pstImg->au64VirAddr[1]);
ybase=(unsigned char*)pU8_y;
ubase=(unsigned char*)pU8_u;
//00000000000000000000000000000000000000000

    printf("yuv_to_111_jpg start compless \n");
    int j=0;
    while (cinfo.next_scanline < cinfo.image_height) {
        int idx=0;
        for(int i=0; i<image_width; i++){
            yuvbuf[idx++]=ybase[i + j * image_width];
            yuvbuf[idx++]=ubase[j / 2 * image_width + (i/2) * 2];
            yuvbuf[idx++]=ubase[j / 2 * image_width + (i/2) * 2 + 1];
        }
        row_pointer[0] = yuvbuf;
        jpeg_write_scanlines(&cinfo, row_pointer, 1);
        j++;
        if(j>380) printf("j=%d \n",j);// j 确实循环到384
    }
    /*
        error_0602_0050，运行一段时间，大概5分钟后突然卡住了，串口终端不打印东西了： [0][W:4815]{hmac_set_psm_timeout::pm off.}

        error: Application transferred too few scanlines 。因为 while (cinfo.next_scanline < cinfo.image_height) 循环次数
        不够

        error: Output file write error --- out of disk space? 就是 jpeg_write_scanlines(&cinfo, row_pointer, 1) 这个不对，
        可能是当时库文件没弄好。因为当时代码编译时自己包括了一个别的地方带来的 .so 库，实际上板端已经指定了，所以只需要 -ljpeg 即可。

    */
    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
    printf("yuv_to_111_jpg finish compless \n");
    fclose(outfile);
    free(yuvbuf);// 00000000000000000000000000000000000000000000000000
    printf("yuv_to_111_jpg finish \n");
    return 0;
}
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ yuv_to_jpg above ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */ 

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ yuv_to_bgr below ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */ 
void YUV420SP_TO_BGR24(IVE_IMAGE_S *pstImg, unsigned char *rgb, int image_width, int image_height)
{
    printf("yuv_to_111_bgr start \n");

//00000000000000000000000000000000000000000
int Stride0, Stride1;
Stride0=pstImg->au32Stride[0];
Stride1=pstImg->au32Stride[0];
printf("Stride0 = %d, Stride1 = %d \n", Stride0, Stride1);
//Stride0=640, Stride1=640
//00000000000000000000000000000000000000000

    unsigned char *ybase;
    unsigned char *ubase;

//00000000000000000000000000000000000000000
HI_U8 *pU8_y = NULL;
HI_U8 *pU8_u = NULL;
pU8_y= SAMPLE_COMM_IVE_CONVERT_64BIT_ADDR(HI_U8, pstImg->au64VirAddr[0]);
pU8_u= SAMPLE_COMM_IVE_CONVERT_64BIT_ADDR(HI_U8, pstImg->au64VirAddr[1]);
ybase=(unsigned char*)pU8_y;
ubase=(unsigned char*)pU8_u;
printf("yuv_to_111_bgr get yuv data success \n");
//00000000000000000000000000000000000000000

    printf("yuv_to_111_bgr start convert \n");
    int index = 0;
    for (int y = 0; y < image_height; y++) {
        for (int x = 0; x < image_width; x++) {// 循环给rgb[]赋值，所以需要提前分配内存，rgb[3*image_width*image_height]
            //YYYYYYYYUVUV
            unsigned char Y = ybase[x + y * image_width];
            unsigned char U = ubase[y / 2 * image_width + (x / 2) * 2];
            unsigned char V = ubase[y / 2 * image_width + (x / 2) * 2 + 1];

            // rgb[index++] = Y + 1.402 * (V - 128);// R
            // rgb[index++] = Y - 0.34413 * (U - 128) - 0.71414 * (V - 128);// G
            // rgb[index++] = Y + 1.772 * (U - 128);// B
            // 调换顺序，改为 BGR
            rgb[index++] = Y + 1.772 * (U - 128);// B
            rgb[index++] = Y - 0.34413 * (U - 128) - 0.71414 * (V - 128);// G
            rgb[index++] = Y + 1.402 * (V - 128);// R
        }
    }
    printf("yuv_to_111_bgr finish convert \n");

    printf("yuv_to_111_bgr finish \n");
}
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ yuv_to_bgr above ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */ 

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ bgr_to_jpg below ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */ 
static int bgr_to_jpg1(unsigned char *pdata, HI_CHAR *jpg_file, int width, int height)
{// 分别为BGR数据，要保存的jpg文件名，图片长宽。因为 libjpeg.so 的库有问题，会将 rgb 顺序颠倒为 bgr ，所以图片的红色和蓝色互换。
// 为了得到颜色正确的 jpg ,输入该函数的 图像数据就必须是 bgr 。
    printf("bgr_to_111_jpg1 start \n");
    int depth = 3;
    JSAMPROW row_pointer[1];// 指向一行图像数据的指针
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    FILE *outfile;

    cinfo.err = jpeg_std_error(&jerr);// 要首先初始化错误信息
    // Now we can initialize the JPEG compression object.

    jpeg_create_compress(&cinfo);
    printf("bgr_to_111_jpg1 create compress object cinfo success \n");

    if ((outfile = fopen(jpg_file, "w")) == NULL){
        printf("bgr_to_111_jpg1 error can't open %s\n", jpg_file);
        return -1;
    }
    printf("bgr_to_111_jpg1 open file success \n");

    jpeg_stdio_dest(&cinfo, outfile);

    cinfo.image_width = width;     // image width and height, in pixels
    cinfo.image_height = height;
    cinfo.input_components = depth;// of color components per pixel
    cinfo.in_color_space = JCS_RGB;// colorspace of input image
    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, JPEG_QUALITY, TRUE );//* limit to baseline-JPEG values
    
    printf("bgr_to_111_jpg1 start compress \n");
    jpeg_start_compress(&cinfo, TRUE);
    int row_stride = width * 3;
    while (cinfo.next_scanline < cinfo.image_height){
        row_pointer[0] = (JSAMPROW)(pdata + cinfo.next_scanline * row_stride);// 一行一行数据的传，jpeg 为大端数据格式
        jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }
    printf("bgr_to_111_jpg1 finish compress \n");

    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);// 这几个函数都是固定流程
    fclose(outfile);
    printf("bgr_to_111_jpg1 finish \n");

    return 0;
}
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ bgr_to_jpg above ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */ 

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ send_to_Huawei_cloud below ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
static int send_to_Huawei_cloud1(HI_CHAR *jpgFileNamepointer)
{
    printf("send_to_111_Huawei_cloud1 读取硬盘 jpg 图片再发送流程开始！\n");
    int ret;
    /*
    char *buf[614989] = {0};// 614989 足够大
    char buf_end[128] = "endendend!@!2!@!";
    char buf_begin[128] = "beginbeginbegin!@!2!@!";
    上述这三个 char 的定义会报错，具体哪一个还不清楚！！！感觉是第一个就错了。、时
    */
    // HI_CHAR buf_end[IVE_FILE_NAME_LEN];HI_CHAR buf_begin[IVE_FILE_NAME_LEN];
    // if (snprintf_s(buf_end, sizeof(buf_end), sizeof(buf_end) - 1, "endendend!@!2!@!") < 0) {
    //     HI_ASSERT(0);
    // }printf("send_to_111_Huawei_cloud1 initialize buf_end success \n");
    // if (snprintf_s(buf_begin, sizeof(buf_begin), sizeof(buf_begin) - 1, "beginbeginbegin!@!2!@!") < 0) {
    //     HI_ASSERT(0);
    // }printf("send_to_111_Huawei_cloud1 initialize buf_begin \n");
    // HI_CHAR *buf[614989] = {0}; // ================================================== 这种定义 error
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
    int buf_max_length = 200000;// 使用 yuv_bgr_jpg 路线，读取 jpg 的平均字节数为110000。 0609，但是现在好像有145000这么大。
    // 所以就用200000。
    /*
        现在的 jpg 图片大小是640*384，大概是 105000 个字节。因为是从文件中读取，
        所以每次很可能略有不同，所以最大值要设的大一些。这里一开始不知道为啥出错了，读取 jpg 文件时只有 48300 个字节。
        然后图片也是混乱的，条纹状。后来发现是读写该文件的代码是连续的，可能内部某些处理还没结束，所以手动延时了1ms，
        结果正确。
    */
    if((buf=(unsigned char *)malloc(buf_max_length))!=NULL)// 使用 malloc 分配内存
        memset(buf, 0, buf_max_length);// 使用 memset 初始化
    printf("send_to_111_Huawei_cloud1 initialize buf \n");

    FILE *tocloud_fp = NULL;
    int size;int length;
    printf("send_to_111_Huawei_cloud1 读取硬盘 jpg 图片再发送！\n");
    /* 打开文件 */
    if ((tocloud_fp = fopen(jpgFileNamepointer, "rb")) == HI_NULL) {// right，rb表示只读一个二进制文件
    // if ((tocloud_fp = fopen(jpgFileNamepointer, "r")) == HI_NULL) {// right，r表示只读
        printf("send_to_111_Huawei_cloud1 can't open \n");
    }
    printf("send_to_111_Huawei_cloud1 待发送的 jpg 文件打开成功！\n");
    /* 将读写位置移动到文件尾部 */
    if (0 > fseek(tocloud_fp, 0, SEEK_END)) {printf("fseek error");}
    printf("send_to_111_Huawei_cloud1 将读写位置移动到文件尾部！\n");
    /* 获取当前位置偏移量 */
    if (0 > (length = ftell(tocloud_fp))) {printf("ftell error");}
    printf("send_to_111_Huawei_cloud1 文件大小: %d 个字节 \n", length);
    /* 将读写位置移动到文件头部 */
    if (0 > fseek(tocloud_fp, 0, SEEK_SET)) {printf("fseek error");}
    printf("send_to_111_Huawei_cloud1 将读写位置移动到文件头部！\n");
    printf("buf_max_length = %d, length = %d \n", buf_max_length, length);
    if(buf_max_length>length) {
        printf("send_to_111_Huawei_cloud1 待发送文件字节数 <<< <<< <<< <<< <<< 预设内存最大容量\n");
        if (1 > (size = fread(buf, length, 1, tocloud_fp))) {// 读取一个数据项。/* 如果未发生错误则意味着已经到达了文件末尾 */
        printf("fread error\n");}
        printf("send_to_111_Huawei_cloud1 成功读取 %d 个字节数据\n", size*length);
        // 因为上述 fread 参数设置就是会读取一个数据项，长度 length 个字节，最后返回的 size 其实是数据项个数。
        // extern ssize_t send (int __fd, const void *__buf, size_t __n, int __flags);
        ret = send(sockfd2, buf, length, 0);// 如果是字符数组buf，那么就要用&buf。
        // 如果是字符数组指针buf，那就直接send(buf)。// 需要单独发送结束标志
        if(0 > ret){printf("send error send error send error send error send error send error \n");}
        printf("....................... 发送图片结束 发送图片结束 ....................... \n");
        printf("发送图片结束 发送图片结束 ....................... 发送图片结束 发送图片结束 \n");
        printf("....................... 发送图片结束 发送图片结束 ....................... \n");
        // ret = send(sockfd2,&buf_end,strlen(buf_end),0);// 发送结束标志
        ret = send(sockfd2, buf_end, strlen(buf_end), 0);// 发送结束标志
        if(0 > ret){printf("send error send error send error send error send error send error \n");}
        printf("strlen(buf_end)=%d   send end_flag over \n", strlen(buf_end));
    }
    else printf("send_to_111_Huawei_cloud1 待发送文件字节数 >>> >>> >>> >>> >>> 预设内存最大容量 \n");
    /* 关闭文件 */
    fclose(tocloud_fp);
    free(buf);
    // sleep(1);// 需要延时，否则频繁给 cloud 发送图片会出错。也可以在函数外延时。
    printf("send_to_111_Huawei_cloud1 读取硬盘 jpg 图片再发送流程到此结束！\n");

    return 0;
}
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ send_to_Huawei_cloud above ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */ 

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ sendto_app below ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */ 
static int sendto_app1()
{
    printf("sendto_111_app1 以下读取硬盘 jpg 图片再 udp 发送给 app 流程，开始 \n");
    HI_CHAR sendto_APP_file_name0[IVE_FILE_NAME_LEN];
    if (snprintf_s(sendto_APP_file_name0, sizeof(sendto_APP_file_name0), sizeof(sendto_APP_file_name0) - 1,
        "/userdata/app.jpg") < 0) {
        HI_ASSERT(0);
    }
    HI_CHAR* sendto_APP_file_name0_pointer0;
    sendto_APP_file_name0_pointer0 = &sendto_APP_file_name0;
    if(send_to_APP==1){
        for(int i=0; i<2; i++){
            printf("sendto_111_app1 send_to_APP = %d send_to_APP = %d ，开启视频传输模式 \n", send_to_APP, send_to_APP);
        }
        
        FILE *fp = fopen(sendto_APP_file_name0_pointer0, "r");
        if (fp == NULL){
            for(int i=0; i<3; i++){
                printf("sendto_111_app1 File:\t%s Not Found! 传输无法开始\n", sendto_APP_file_name0);
            }
        }
        else{
            printf(" ............. sendto_111_app1 打开文件 sendto_APP_file_name0_pointer0 成功 ............. \n");
            for(int i=0; i<3; i++){
                printf(".................................. sendto_111_app1 udp_sendto_APP .................................. \n");
            }
            bzero(buffer, BUFFER_SIZE);
            int file_block_length = 0;
            while( (file_block_length = fread(buffer, sizeof(char), BUFFER_SIZE, fp)) > 0){
                // 从 udp_client_socket 发送 buffer 中的字符串到 server_addr ,就是从客户端发送给服务端
                if (sendto(udp_client_socket, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&server_addr, 
                    server_addr_length) < 0)
                {
                    printf("sendto_111_app1 Send File:\t%s Failed! \n", sendto_APP_file_name0);
                    break;
                }
                bzero(buffer, sizeof(buffer));
            }
            if (sendto(udp_client_socket, buffer_end, BUFFER_SIZE, 0, (struct sockaddr *)&server_addr, 
                server_addr_length) < 0){
                printf("sendto_111_app1 Send finish_flag Failed! \n");
            }
            for(int i=0; i<3; i++){
                printf(" ............................. udp_sendto_APP SUCCESS ............................. \n");
            }
            fclose(fp);
            printf("sendto_111_app1 File:\t%s Transfer Finish! \n", sendto_APP_file_name0);
        }
    }// if(send_to_APP==1)
    else 
        for(int i=0; i<1; i++){
            printf("sendto_111_app1 send_to_APP = %d send_to_APP = %d ，不开启视频传输模式 \n", send_to_APP, send_to_APP);
        }
    printf("sendto_111_app1 以上读取硬盘 jpg 图片再 udp 发送给 app ，结束 \n", send_to_APP);
}
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ sendto_app above ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */ 

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ bgr_to_jpg_memmory below ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */ 
static int bgr_to_jpg_memmory1(unsigned char *pdata, HI_CHAR *jpg_file, int width, int height, 
    JPG_BUFTYPE *jpg_buf0, unsigned long *partial_jpg_buf_size)
{// 分别为RGB数据，保存到磁盘的jpg文件名，图片宽，图片长，保存图片的 jpg 格式的内存，图片大小（长度）
    printf("bgr_to_111_jpg_memmory1 将内存中的 bgr 图片数据转为 jpg 数据并保存到另一块内存中的流程 开始 \n");
    int depth = 3;
    JSAMPROW row_pointer[1];// 指向一行图像数据的指针
    struct jpeg_compress_struct cinfo;struct jpeg_error_mgr jerr;
    cinfo.err = jpeg_std_error(&jerr);// 要首先初始化错误信息
    //* Now we can initialize the JPEG compression object.
    jpeg_create_compress(&cinfo);

    // ====================================== 分配局部内存 partial_jpg_buf ======================================
    printf("bgr_to_111_jpg_memmory1 malloc memmory \n");
    unsigned char * partial_jpg_buf=(unsigned char *)malloc(300000);
    // bgr_to_jpg 的大小是变化的。比较灰暗的图片，一般只有59000左右。 所以只要分配60000即可。
    // 但是色彩比较丰富的图片， yuv_to_bgr 那一步就会很大，再转 jpg 后又120000,甚至140000以上。 所以干脆分配200000。
    jpeg_mem_dest(&cinfo, &partial_jpg_buf, partial_jpg_buf_size);
    // ====================================== 分配局部内存 partial_jpg_buf ======================================

    cinfo.image_width = width;     //* image width and height, in pixels
    cinfo.image_height = height;
    cinfo.input_components = depth;//* # of color components per pixel
    cinfo.in_color_space = JCS_RGB;//* colorspace of input image
    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, JPEG_QUALITY, TRUE );//* limit to baseline-JPEG values
    printf("bgr_to_111_jpg_memmory1 start_compress \n");
    jpeg_start_compress(&cinfo, TRUE);
    int row_stride = width * 3;
    while (cinfo.next_scanline < cinfo.image_height){
        row_pointer[0] = &pdata[cinfo.next_scanline * row_stride];
        jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }
    jpeg_finish_compress(&cinfo);// 上面这几个函数都是固定流程
    jpeg_destroy_compress(&cinfo);// 以上这几个函数都是固定流程
    printf("bgr_to_111_jpg_memmory1 finish compress \n");

    unsigned long length=0;
    length=*partial_jpg_buf_size;
    printf("bgr_to_111_jpg_memmory1 length=%ld, partial_jpg_buf_size=%ld \n", length, *partial_jpg_buf_size);
    // 110000 是对的
    // ========================== 以下从内存空间 partial_jpg_buf 拷贝到内存空间 jpg_buf 中 ==========================
    /*
        业务逻辑中有图像数据的拷贝过程。图像数据是unsinged char* 类型，如果直接用“=”去赋值的话，只是获取的右值的指针，
        而内部的具体内容并没有赋值，需要用memcpy()获取一下右值指针的内容。
        void *memcpy(void *destin, void *source, unsigned n);
    */
    
    // // === 使用输入参数 jpg_buf
    // jpg_buf->length=(int)length;
    // // jpg_buf->start=new uint8_t[length];// 这样也是一种内存分配的方法。是 c++ 的方法，不需要添加新的库，已经在默认路径里面了。
    // // 但是现在是 c 文件，所以编译过不了。
    // if((jpg_buf->start=(uint8_t *)malloc(length))!=NULL)
    //     memset(jpg_buf->start, 0, length);
    // printf("bgr_to_111_jpg_memmory1 malloc memory success \n");
    // memcpy(jpg_buf->start, partial_jpg_buf, length);// 使用这个 memcpy ，partial_jpg_buf 必须是 malloc 分配了内存的
    // partial_jpg_buf2=partial_jpg_buf;// 这样赋值好像没用，在该函数外面就用不了 partial_jpg_buf2
    // // === 使用输入参数 jpg_buf

    // // === 使用局部变量 jpg_buf
    // JPG_BUFTYPE jpg_buf;// 局部结构体变量
    // jpg_buf.length=(int)length;
    // // jpg_buf->start=new uint8_t[length];// 这样也是一种内存分配的方法。是 c++ 的方法，不需要添加新的库，已经在默认路径里面了。
    // // 但是现在是 c 文件，所以编译过不了。
    // // if((jpg_buf.start=(uint8_t *)malloc(length))!=NULL)
    // //     memset(jpg_buf.start, 0, length);
    //     jpg_buf.start=(uint8_t *)malloc(length);
    //     memset(jpg_buf.start, 0, length);
    // printf("bgr_to_111_jpg_memmory1 malloc memory success \n");
    // memcpy(jpg_buf.start, partial_jpg_buf, length);
    // // === 使用局部变量 jpg_buf
    // ========================== 以上从内存空间 partial_jpg_buf 拷贝到内存空间 jpg_buf 中 ==========================

    // sendto_app2(&jpg_buf);
    
        // ============================== 以下保存为 .jpg 图片，测试压缩函数是否正确 ==============================
        // FILE *outfile;
        // if((outfile = fopen(jpg_file, "w")) == NULL){
        //     printf("bgr_to_111_jpg_memmory1 can't open %s \n", jpg_file);
        //     return -1;
        // }
        // printf("bgr_to_111_jpg_memmory1 open jpg_file success \n");
        // // if(length>fwrite(partial_jpg_buf, 1, length, outfile))// partial_jpg_buf result ok
        // if(length>fwrite(jpg_buf->start, 1, length, outfile))// jpg_buf->start result ok
        // {
        //     printf("bgr_to_111_jpg_memmory1 fwrite error fwrite error fwrite error fwrite error fwrite error \n");
        // }
        // printf("bgr_to_111_jpg_memmory1 write over \n");
        // fclose(outfile);
        // printf("bgr_to_111_jpg_memmory1 seve jpg_file success \n");
        // ============================== 以上保存为 .jpg 图片，测试压缩函数是否正确 ==============================
    
    // // 使用 partial_jpg_buf 内存发送如下 
    // int sendtoed_size = 0;// 已发送长度
    // uint8_t *jpg_buf;
    // jpg_buf = (uint8_t *)partial_jpg_buf;
    // while(length - sendtoed_size > BufSize){
    //     if (sendto(udp_client_socket, jpg_buf + sendtoed_size, BufSize, 0, 
    //         (struct sockaddr *)&server_addr,server_addr_length) < 0)
    //     {
    //         printf("sendto_111_app2 Send Failed! \n");
    //         break;
    //     }
    //     sendtoed_size += BufSize;
    // }
    // if (sendto(udp_client_socket, buffer_end, BufSize, 0,(struct sockaddr *)&server_addr,server_addr_length) < 0){
    //     printf("sendto_111_app2 Send finish_flag Failed!\n");
    // }
    // 效果还行，偶尔会出现图片底部丢包的问题！！！偶尔会乱码，总体稳定！！！但是依然会崩溃。
    // // 使用 partial_jpg_buf 内存发送如上 

    // partial_jpg_buf=NULL;
    free(partial_jpg_buf);// 记得要释放内存！！！
    // free(jpg_buf.start);
    // jpeg_destroy_compress(&cinfo);// 以上这几个函数都是固定流程
    printf("length=%ld, partial_jpg_buf_size=%ld\n", length, *partial_jpg_buf_size);// length=110000 是对的
    printf("bgr_to_111_jpg_memmory1 将内存中的 bgr 图片数据转为 jpg 数据并保存到另一块内存中的流程 结束 \n");

    return 0;
    // 有点离谱，不知道为啥，现在图片很大，有142942，上一次是13多，总之，比原先正常的11多大了很多很多！！！
}
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ bgr_to_jpg_memmory above ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */ 

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ send_to_Huawei_cloud below ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
static int send_to_Huawei_cloud2(JPG_BUFTYPE *jpg_buf)
{
    printf("send_to_111_Huawei_cloud2 发送内存中的 jpg 图片到 cloud 流程开始！\n");
    int ret;
    // ret = send(sockfd2, (unsigned char *)jpg_buf.start, jpg_buf_size, 0);// result ok
    ret = send(sockfd2, (unsigned char *)jpg_buf->start, jpg_buf->length, 0);// result ok
    if(0 > ret){printf("send error send error send error send error send error send error\n");}
    printf("....................... 发送图片结束 发送图片结束 ....................... \n");
    printf("发送图片结束 发送图片结束 ....................... 发送图片结束 发送图片结束 \n");
    printf("....................... 发送图片结束 发送图片结束 ....................... \n");
    ret = send(sockfd2, buf_end, strlen(buf_end),0);// 发送结束标志
    if(0 > ret){printf("send error send error send error send error send error send error\n");}
    printf("strlen(buf_end)=%d   send end_flag over\n", strlen(buf_end));
    printf("send_to_111_Huawei_cloud2 发送内存中的 jpg 图片到 cloud 流程结束！\n");

    return 0;
}
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ send_to_Huawei_cloud above ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */ 

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ sendto_app below ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */ 
static int sendto_app2(JPG_BUFTYPE *jpg_buf)
{
    printf("sendto_111_app2 发送内存中的 jpg 图片到 app 流程开始！\n");
    if(send_to_APP==1)
    {
        for(int i=0; i<3; i++){
            printf("sendto_111_app2 send_to_APP = %d send_to_APP = %d ，开启视频传输模式 \n", send_to_APP, send_to_APP);
        }

        /* 直接从内存读取然后发送 */
        int sendtoed_size = 0;// 已发送长度
        while(jpg_buf->length - sendtoed_size > BufSize){
            if (sendto(udp_client_socket, jpg_buf->start + sendtoed_size, BufSize, 0, 
                (struct sockaddr *)&server_addr,server_addr_length) < 0)
            {
                printf("sendto_111_app2 Send Failed! \n");
                break;
            }
            sendtoed_size += BufSize;
        }
        if (sendto(udp_client_socket, buffer_end, BufSize, 0,(struct sockaddr *)&server_addr,server_addr_length) < 0){
            printf("sendto_111_app2 Send finish_flag Failed!\n");
        }
        for(int i=0; i<3; i++){
            printf(" ....................... sendto_111_app2 udp_sendto_APP SUCCESS ....................... \n");
        }
        printf("sendto_111_app2 Transfer Finished! \n");
        /* 直接从内存读取然后发送 */

    }// if(send_to_APP==1)
    else 
        for(int i=0; i<3; i++){
            printf("sendto_111_app2 send_to_APP = %d send_to_APP = %d ，不开启视频传输模式 \n", send_to_APP, send_to_APP);
        }
    printf("sendto_111_app2 发送内存中的 jpg 图片到 app 流程结束！\n");
}
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ sendto_app above ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */ 

/*
 * 加载手部检测和手势分类模型
 * Load hand detect and classify model
 */
HI_S32 Yolo2HandDetectResnetClassifyLoad(uintptr_t* model)
{
    SAMPLE_SVP_NNIE_CFG_S *self = NULL;
    HI_S32 ret;
    HI_CHAR audioThreadName[BUFFER_SIZE] = {0};// 音频
    HI_CHAR tcp_app_ThreadName[BUFFER_SIZE] = {0};// tcp 连接手机 APP 

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



// ====================================== 连接 Huawei_cloud socket 初始化如下 ======================================
        struct sockaddr_in server_addr2 = {0};
        /* 打开套接字，得到套接字描述符 */
        sockfd2 = socket(AF_INET, SOCK_STREAM, 0);
        if (0 > sockfd2) {printf("cloud_socket error cloud_socket error cloud_socket error");}
        else printf("link to Huawei_cloud 000\n");
        /* 调用 connect 连接远端服务器 */
        server_addr2.sin_family = AF_INET;
        server_addr2.sin_port = htons(SERVER_PORT2);//端口号
        inet_pton(AF_INET, SERVER_IP2, &server_addr2.sin_addr);//将服务器的 IP 地址 存入 结构体 server_addr
        printf("link to Huawei_cloud 111\n");
        ret = connect(sockfd2, (struct sockaddr *)&server_addr2, sizeof(server_addr2));
        if (0 > ret) {
            printf("服务器连接失败 服务器连接失败 服务器连接失败 服务器连接失败 服务器连接失败 服务器连接失败\n");
            close(sockfd2);
            printf("服务器连接失败 服务器连接失败 服务器连接失败 服务器连接失败 服务器连接失败 服务器连接失败\n");
        }else{
            for(int i = 0; i < 5; i++){
                printf("服务器连接成功 服务器连接成功 服务器连接成功 服务器连接成功 服务器连接成功 服务器连接成功\n");
                printf("...................................... 服务器连接成功 ...................................... \n");
                printf("服务器连接成功 服务器连接成功 服务器连接成功 服务器连接成功 服务器连接成功 服务器连接成功\n");
            }
        }
        sleep(1);
// ====================================== 连接 Huawei_cloud socket 初始化如上 ======================================



// ====================================== 创建音频播放的线程如下 ======================================
        if (GetCfgBool("audio_player:support_audio", true)) {
            for(int i = 0; i < 2; i++) {
                printf("创建音频播放的线程开始 创建音频播放的线程开始 创建音频播放的线程开始 \n");
            }
            // ret = SkPairCreate(&g_stmChn);// 在 posix_help.c 中，44。配置输入输出通道为0、1
            HI_ASSERT(ret == 0);
            if (snprintf_s(audioThreadName, BUFFER_SIZE, BUFFER_SIZE - 1, "AudioProcess") < 0) {
                HI_ASSERT(0);
            }
            prctl(PR_SET_NAME, (unsigned long)audioThreadName, 0, 0, 0);// 在 prctl.h 中 #define PR_SET_NAME    15
            //  在 prctl.h 中， int prctl (int, ...);// 不懂！！！============================================================

            ret = pthread_create(&g_audioProcessThread, NULL, GetAudioFileName, NULL);// 创建音频播放的线程
            if (ret != 0) {
                SAMPLE_PRT("audio proccess thread creat fail:%s\n", strerror(ret));
                return ret;
            }
            for(int i = 0; i < 2; i++) {
                printf("创建音频播放的线程结束 创建音频播放的线程结束 创建音频播放的线程结束 \n");
            }
        }else{
            for(int i = 0; i < 2; i++) {
                printf("不启用音频 不启用音频 不启用音频 不启用音频\n");
            }
        }
// ====================================== 创建音频播放的线程如上 ======================================



// ========================== 创建 tcp_link_app_receive_from_app 的线程如下 ==========================
        for(int i = 0; i < 2; i++) {
            printf("创建 tcp_link_app_receive_from_app 的线程开始 创建 tcp_link_app_receive_from_app 的线程开始  \n");
        }
        if (snprintf_s(tcp_app_ThreadName, BUFFER_SIZE, BUFFER_SIZE - 1, "tcp_app_Process") < 0) {HI_ASSERT(0);}
        // prctl(PR_SET_NAME, (unsigned long)tcp_app_ThreadName, 0, 0, 0);// 在 prctl.h 中 #define PR_SET_NAME    15
        //  在 prctl.h 中， int prctl (int, ...);
        ret = pthread_create(&tcp_app_ProcessThread, NULL, tcp_app, NULL);// 创建 tcp 连接 app 的线程
        if (ret != 0) {
            SAMPLE_PRT("tcp_app proccess thread creat fail:%s\n", strerror(ret));
            return ret;
        }
        for(int i = 0; i < 2; i++) {
            printf("创建 tcp_link_app_receive_from_app 的线程结束 创建 tcp_link_app_receive_from_app 的线程结束  \n");
        }
// ========================== 创建 tcp_link_app_receive_from_app 的线程如上 ==========================



// ====================================== udp initial below ======================================  
        for(int i=0; i<3; i++){
            printf("udp initial start udp initial start udp initial start udp initial start \n");
            printf("udp initial start ................................... udp initial start \n");
        }
        // 设置一个 socket 地址结构 client_addr , 代表客户机的 internet 地址和端口
        struct sockaddr_in client_addr;
        bzero(&client_addr, sizeof(client_addr));
        client_addr.sin_family = AF_INET;               // internet 协议族  
        client_addr.sin_addr.s_addr = htons(INADDR_ANY);// INADDR_ANY 表示自动获取本机地址  
        client_addr.sin_port = htons(0);                // auto allocated , 让系统自动分配一个空闲端口  
        // 创建用于 internet 的流协议(TCP)类型 socket ，用 udp_client_socket 代表客户端 socket  
        udp_client_socket = socket(AF_INET, SOCK_DGRAM, 0);  
        if (udp_client_socket < 0)  {printf("Create Socket Failed!\n");}
        // 把客户端的 socket 和客户端的 socket 地址结构绑定   
        if (bind(udp_client_socket, (struct sockaddr*)&client_addr, sizeof(client_addr)))  
        {printf("Client Bind Port Failed!\n");}  
        // 设置一个 socket 地址结构 server_addr ,代表服务器的 internet 地址和端口  
        bzero(&server_addr, sizeof(server_addr));server_addr_length = sizeof(server_addr);
        // 设置服务器（手机APP）的IP
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(SERVER_PORT);
        if (inet_aton(SERVER_IP, &server_addr.sin_addr) == 0) {printf("Server IP Address Error! \n");}
        for(int i=0; i<3; i++){
            printf("udp initial finish udp initial finish udp initial finish udp initial finish \n");
            printf("udp initial finish ..................................... udp initial finish \n");
        }
        sleep(1);
// ====================================== udp initial above ====================================== 

        if (snprintf_s(buf_end, sizeof(buf_end), sizeof(buf_end) - 1, "endendend!@!2!@!") < 0) {
            HI_ASSERT(0);
        }printf("initialize buf_end success \n");
        if (snprintf_s(buf_begin, sizeof(buf_begin), sizeof(buf_begin) - 1, "beginbeginbegin!@!2!@!") < 0) {
            HI_ASSERT(0);
        }printf("initialize buf_begin \n");

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
// HI_S32 Yolo2HandDetectResnetClassifyCal2(uintptr_t model, VIDEO_FRAME_INFO_S *srcFrm, VIDEO_FRAME_INFO_S *dstFrm)
HI_S32 Yolo2HandDetectResnetClassifyCal2(uintptr_t model, VIDEO_FRAME_INFO_S *srcFrm, VIDEO_FRAME_INFO_S *srcFrm2, 
    VIDEO_FRAME_INFO_S *dstFrm)
{
    SAMPLE_SVP_NNIE_CFG_S *self = (SAMPLE_SVP_NNIE_CFG_S *)model;
    HI_S32 resLen = 0;// 在 hi_type.h 中，51。还有一些其他的海思自定义的类型

    int objNum;// 存储模型识别一帧图片得到的目标数
    int ret;
    int num = 0;

// typedef struct hiIVE_IMAGE_S {
//     HI_U64 au64PhyAddr[3];   /* RW;The physical address of the image */

//     HI_U64 au64VirAddr[3];   /* RW;The virtual address of the image */
// === 三个数组，但是只使用通道0、1，所以只用前两个数组

//     HI_U32 au32Stride[3];    /* RW;The stride of the image */
//     HI_U32 u32Width;         /* RW;The width of the image */
//     HI_U32 u32Height;        /* RW;The height of the image */
//     IVE_IMAGE_TYPE_E enType; /* RW;The type of the image */
// } IVE_IMAGE_S;

    ret = FrmToOrigImg((VIDEO_FRAME_INFO_S *)srcFrm, &img);// 在 ive_img.c 中，52。 srcFrm 是宽640*高384的。
    // 将帧缩放后的一帧转为初始的 yuv 图片。复制数据指针。
    // 将 *VIDEO_FRAME_INFO_S 格式转换成 IVE_IMAGE_S 格式，不复制数据，所以不需要释放 img 内存？
    // 具体格式是 YUV420/422 ，但是到底是哪一个还不清楚
    // 根据 FrmToOrigImg 函数里面的打印信息可知是 IVE_IMAGE_TYPE_YUV420SP
    SAMPLE_CHECK_EXPR_RET(ret != HI_SUCCESS, ret, "hand detect for YUV Frm to Img FAIL, ret=%#x\n", ret);

// ================================= 复制内存 srcFrm ，为了后面画框相关如下 ================================
    // printf("复制内存 srcFrm ，为了后面画框相关如下 \n");
    // VIDEO_FRAME_INFO_S *srcFrm2 = NULL;
    // if((srcFrm2=(VIDEO_FRAME_INFO_S *)malloc(sizeof(VIDEO_FRAME_INFO_S))) != NULL)
    //         memset(srcFrm2, 0, sizeof(VIDEO_FRAME_INFO_S));
    // memcpy(srcFrm2, srcFrm, sizeof(VIDEO_FRAME_INFO_S));
    // printf("复制内存 srcFrm ，为了后面画框相关如上 \n");
// ================================= 复制内存 srcFrm ，为了后面画框相关如上 ================================

// ====================================== 以下将 img 保存到磁盘 ======================================
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
        for(i=0; i<1; i++){
            printf("开始保存 yuv 到磁盘 开始保存 yuv 到磁盘 开始保存 yuv 到磁盘 开始保存 yuv 到磁盘  \n");
        }

        // HI_S32 SAMPLE_COMM_IVE_WriteFile(IVE_IMAGE_S *pstImg, FILE *pFp)
        SAMPLE_COMM_IVE_WriteFile(&img, pFp);// 好像不用 #include "sample_comm_ive.h"
// // ========================== func:SAMPLE_COMM_IVE_WriteFile, line:272, NULL pointer 。0605_1313
// // ========================== func:SAMPLE_COMM_IVE_WriteFile, line:272, NULL pointer 。
// // 0609_1635 。运行10分钟，就开始报错。即使别的没啥问题，这个函数也会报错，不过不影响。

// ================================================================= ///////////////////////////////////////////////
        ///////////////////////////////////////////////
        fclose(pFp);// 终于找到错误源头了！！！
        ///////////////////////////////////////////////
// ================================================================= ///////////////////////////////////////////////

        /*
            运行时报错，因为输入的参数：要打开的文件不对，现在可以运行了，但是没有结果。因为输入的参数还是错的，
            打印信息：func:SAMPLE_COMM_IVE_WriteFile, line:272, NULL pointer。
            分析：空指针，输入指针才不会报错，但是空指针说明输入还是错的，现在对了。
            总结：fopen 的输入必须是指针，该指针需要指向一个字符数组的首地址，字符数组由 snprintf_s 得到。
        */
        for(i=0; i<1; i++){
            printf("结束保存 yuv 到磁盘 结束保存 yuv 到磁盘 结束保存 yuv 到磁盘 结束保存 yuv 到磁盘 \n");
        }
// ====================================== 以上将 img 保存到磁盘 ======================================



// ====================================== 以下将 yuv 转 jpg 并保存到磁盘 ======================================
        HI_CHAR jpgFileName0[IVE_FILE_NAME_LEN];
        if (snprintf_s(jpgFileName0, sizeof(jpgFileName0), sizeof(jpgFileName0) - 1,
            "/userdata/complete_%s.jpg", "resize") < 0) {
            HI_ASSERT(0);
        }
        HI_CHAR* jpgFileNamepointer0;
        jpgFileNamepointer0=&jpgFileName0;

        // === app
        HI_CHAR jpgFileName1[IVE_FILE_NAME_LEN];
        if (snprintf_s(jpgFileName1, sizeof(jpgFileName1), sizeof(jpgFileName1) - 1,
            "/userdata/app.jpg") < 0) {
            HI_ASSERT(0);
        }
        HI_CHAR* jpgFileNamepointer1;
        jpgFileNamepointer1=&jpgFileName1;
        // === app

        // === cloud
        HI_CHAR jpgFileName2[IVE_FILE_NAME_LEN];
        if (snprintf_s(jpgFileName2, sizeof(jpgFileName2), sizeof(jpgFileName2) - 1,
            "/userdata/cloud.jpg") < 0) {
            HI_ASSERT(0);
        }
        HI_CHAR* jpgFileNamepointer2;
        jpgFileNamepointer2=&jpgFileName2;
        // === cloud

        /*
            因为写入磁盘是也是使用 au64VirAddr ，所以现在转成 jpg 应该也是用 au64VirAddr 。但是它有三个数组，
            写入磁盘时用了前两个，现在？也是前两个，第一个是y，第二个是uv。
        */
        // ============================== convert yuv to jpg directly below ==============================
        // FILE *fopen(const char *restrict filename, const char *restrict mode)
        // int yuv420sp_to_jpeg1(HI_CHAR * filename, IVE_IMAGE_S *pstImg, int quality)
        // yuv420sp_to_jpeg1(jpgFileNamepointer0, &img, 100);// right
        // ============================== convert yuv to jpg directly above ==============================

        // ============================== convert yuv to bgr below ==============================
        int image_width, image_height;
        image_width = img.u32Width;
        image_height = img.u32Height;
        printf("image_width = %d, image_height = %d\n", image_width, image_height);
        // image_width=640, image_height=384
        unsigned char *rgb1;
        if((rgb1=(unsigned char *)malloc(3*image_width*image_height)) != NULL)
            memset(rgb1, 0, 3*image_width*image_height);
        // void YUV420SP_TO_BGR24(IVE_IMAGE_S *pstImg, unsigned char *rgb, int image_width, int image_height) 
        YUV420SP_TO_BGR24(&img, rgb1, image_width, image_height);
        // ============================== convert yuv to bgr above ==============================

        usleep(1000);

        // ============================== convert bgr to jpg below ==============================
        // static int bgr_to_jpg1(unsigned char *pdata, HI_CHAR *jpg_file, int width, int height)
        bgr_to_jpg1(rgb1, jpgFileNamepointer1, image_width, image_height);

        // jpg_buf->start=new uint8_t[length];// 这样也是一种内存分配的方法。是 c++ 的方法，不需要添加新的库，已经在默认路径里面了。
        // 但是现在是 c 文件，所以编译过不了。
        // if((jpg_buf1.start=(uint8_t *)malloc(200000))!=NULL)
        //     memset(jpg_buf1.start, 0, 200000);

        // static int bgr_to_jpg_memmory1(unsigned char *pdata, HI_CHAR *jpg_file, int width, int height, 
            // JPG_BUFTYPE *jpg_buf, unsigned long *partial_jpg_buf_size)
        // bgr_to_jpg_memmory1(rgb1, jpgFileNamepointer1, image_width, image_height, &jpg_buf1, &jpg_buf_size1);// app
        free(rgb1);

        // ============================== convert bgr to jpg above ==============================

// ====================================== 以上将 yuv 转 jpg 并保存到磁盘 ======================================

        printf("2032_0609_1214 \n");
        usleep(1000);// 这里需要一个延时，因为上面刚刚将一个 yuv 图片数据以 jpg 写入本地，下面就要打开这个 jpg 图片读取数据。
        // 如果不延时，就会出现打开后的文件数据缺失、混乱的问题。

// ====================================== 以下读取硬盘 jpg 图片或内存再发送给 cloud ======================================
        // if(count_cycle==0){
        //     // static int send_to_Huawei_cloud1(HI_CHAR *jpgFileNamepointer0)
        //     // send_to_Huawei_cloud1(jpgFileNamepointer0);
        //     // static int send_to_Huawei_cloud2(JPG_BUFTYPE *jpg_buf)// 从内存获取图片再发送给 cloud
        //     // send_to_Huawei_cloud2(&jpg_buf2);
        // }
// ====================================== 以上读取硬盘 jpg 图片或内存再发送给 cloud ======================================

        usleep(1000);

// ====================================== 以下读取硬盘 jpg 图片或内存再 udp 发送给 app ======================================
        // static int sendto_app1()
        sendto_app1();
        // static int sendto_app2(JPG_BUFTYPE *jpg_buf)// 从内存获取图片再 udp 发送给 app
        // sendto_app2(&jpg_buf1);
        // send_to_Huawei_cloud2(&jpg_buf1);// 可以发送给 cloud ，说明 jpg_buf1 数据没问题。最后查出来问题是内存满了。
        // 具体哪个导致的不太清楚，但是和 jpg_buf1 有关系。
        // jpg_buf1.start = NULL;////////////////////////////////
        // free(jpg_buf1.start);////////////////////////////////
        // sleep(3);
// ====================================== 以上读取硬盘 jpg 图片或内存再 udp 发送给 app ======================================



// =============================================== 输入模型与后处理如下 ===============================================
    printf("输入模型与后处理如下 \n");
    objNum = HandDetectCal(&img, objs);// 在 yolov2_hand_detect.c ，78。 Send IMG to the detection net for reasoning
    // objs 存储识别到的目标的框的数据，返回值是识别到的手势种类，存储在 objNum 中。
    printf("模型检测完毕 \n");
    global_audio_objNum = objNum;
    printf("将模型识别结果赋值给全局变量 global_audio_objNum = %d \n", global_audio_objNum);
    // sleep(6);

    for (int i = 0; i < objNum; i++) {// 有几个目标就循环几次
        cnnBoxs[i] = objs[i].box;
        RectBox *box = &objs[i].box;
        cnnScores[i] = objs[i].score;// 存储识别结果的置信度。置信度大于0.25的就被保存在 objs 里面了，但是这太低了。所以
        // 需要对置信度再划分一下，从而实现对音频、给 cloud 发送图片的控制。

        // ==================================== 保存模型输出的框的大小如下 ====================================
        // boxs_initial[i] = *box;
        boxs_initial[i] = objs[i].box;
        // printf("保存模型输出的框的大小 \n");
        SAMPLE_PRT("yolo2_out_initial: {%d, %d, %d, %d}\n", box->xmin, box->ymin, box->xmax, box->ymax);
        // 到这里必定错了！！！，没有打印输出 yolo2_out_initial ，如果注释该部分，就有下面的 yolo2_out 。
        // 0609。其实有打印的，如下所示
        /*
        连续多次的模型输出的框的结果
            yolo2_out_initial: {436, 306, 500, 365}
            yolo2_out: {1308, 860, 1500, 1026}
            yolo2_out_initial: {436, 305, 501, 365}
            yolo2_out: {1308, 857, 1503, 1026}
            但是事实上没有手在摄像头范围内，感觉是误识别。可能是我把那个置信度设置为一直通过了。也就是识别到像手的东西，
            但是置信度不高，也被认为是手了。
        */
        // ==================================== 保存模型输出的框的大小如上 ====================================

        RectBoxTran(box, HAND_FRM_WIDTH, HAND_FRM_HEIGHT,// HAND_FRM_WIDTH = 640，HAND_FRM_HEIGHT=384
            dstFrm->stVFrame.u32Width, dstFrm->stVFrame.u32Height);// 
        // 在 misc_util.c 中，58。按比例转换坐标，转成适合输出的大小并保存在 box 中。
        SAMPLE_PRT("yolo2_out: {%d, %d, %d, %d}\n", box->xmin, box->ymin, box->xmax, box->ymax);
        boxs[i] = *box;
    }
    biggestBoxIndex = GetBiggestHandIndex(boxs, objNum);
    SAMPLE_PRT("biggestBoxIndex:%d, objNum:%d\n", biggestBoxIndex, objNum);
    printf("确定适配 dstFrm 的框的参数 \n");
    /*
     * 当检测到对象时，在DSTFRM中绘制一个矩形
     * When an object is detected, a rectangle is drawn in the DSTFRM
     */
    if (biggestBoxIndex >= 0) {
        objBoxs[0] = boxs[biggestBoxIndex];
        MppFrmDrawRects(dstFrm, objBoxs, 1, RGB888_GREEN, DRAW_RETC_THICK);// Target hand objnum is equal to 1
        // 在 vgs_img.c 中，446。在frame中叠加一个或多个矩形框。
        // dstFrm 是 VIDEO_FRAME_INFO_S 结构体格式的，也是摄像头获取的格式。不是我们能直接操作的格式，需要使用
        // FrmToOrigImg 转成 IVE_IMAGE_S 结构体格式的。 IVE_IMAGE_S 内部包含 yuv420sp 格式的图片数据的首地址，
        // 用3个元素的数组存储。 
        num = 0;
        for (int j = 0; (j < objNum) && (objNum > 1); j++) {// 当检测到的目标数大于1时，进入该循环
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
        // ret = ImgYuvCrop(&img, &imgIn, &cnnBoxs[biggestBoxIndex]);
        // SAMPLE_CHECK_EXPR_RET(ret < 0, ret, "ImgYuvCrop FAIL, ret=%#x\n", ret);
        // if ((imgIn.u32Width >= WIDTH_LIMIT) && (imgIn.u32Height >= HEIGHT_LIMIT)) {
        //     COMPRESS_MODE_E enCompressMode = srcFrm->stVFrame.enCompressMode;
        //     ret = OrigImgToFrm(&imgIn, &frmIn);
        //     frmIn.stVFrame.enCompressMode = enCompressMode;
        //     SAMPLE_PRT("crop u32Width = %d, img.u32Height = %d\n", imgIn.u32Width, imgIn.u32Height);
        //     ret = MppFrmResize(&frmIn, &frmDst, IMAGE_WIDTH, IMAGE_HEIGHT);
        //     ret = FrmToOrigImg(&frmDst, &imgDst);
        //     ret = CnnCalImg(self,  &imgDst, numInfo, sizeof(numInfo) / sizeof((numInfo)[0]), &resLen);
        //     SAMPLE_CHECK_EXPR_RET(ret < 0, ret, "CnnCalImg FAIL, ret=%#x\n", ret);
        //     HI_ASSERT(resLen <= sizeof(numInfo) / sizeof(numInfo[0]));
        //     HandDetectFlag(numInfo[0]);
        //     MppFrmDestroy(&frmDst);
        // }
        // IveImgDestroy(&imgIn);
    }
    printf("给 dstFrm 画框结束 \n");
// =============================================== 输入模型与后处理如上 ===============================================

// ================================= 从磁盘读取文件或者从内存给 cloud 发送不带框的图片如下 =================================
        // unsigned char *rgb2;
        // if((rgb2=(unsigned char *)malloc(3*image_width*image_height)) != NULL)
        //     memset(rgb2, 0, 3*image_width*image_height);
        // YUV420SP_TO_BGR24(&img, rgb2, image_width, image_height);
        // // bgr_to_jpg_memmory1(rgb2, jpgFileNamepointer2, image_width, image_height, &jpg_buf2, &jpg_buf_size2);// cloud
        // bgr_to_jpg1(rgb2, jpgFileNamepointer2, image_width, image_height);
        // free(rgb2);
        // usleep(1000);// 如果不延时，就可能会出现图片数据缺失、混乱的问题。
        // if(count_cycle==0){
        //     // static int send_to_Huawei_cloud1(HI_CHAR *jpgFileNamepointer)
        //     send_to_Huawei_cloud1(jpgFileNamepointer2);
        //     // static int send_to_Huawei_cloud2(JPG_BUFTYPE *jpg_buf)
        //     // send_to_Huawei_cloud2(&jpg_buf2);
        //     // free(jpg_buf2.start);
        // }
        // count_cycle++;
        // int cycle_Boundary_point = 10;
        // if(count_cycle==cycle_Boundary_point) count_cycle=0;
// ================================= 从磁盘读取文件或者内存给 cloud 发送不带框的图片如上 =================================

// ======================================== 对复制的内存 srcFrm2 画框如下 ========================================
        printf("对复制的内存 srcFrm2 画框如下 \n");
        if (biggestBoxIndex >= 0) {
            objBoxs[0] = boxs_initial[biggestBoxIndex];
            MppFrmDrawRects(srcFrm2, objBoxs, 1, RGB888_GREEN, DRAW_RETC_THICK);// Target hand objnum is equal to 1
            num = 0;
            for (int j = 0; (j < objNum) && (objNum > 1); j++) {
                if (j != biggestBoxIndex) {
                    remainingBoxs[num++] = boxs_initial[j];
                    /*
                    * 其他手objnum等于objnum -1
                    * Others hand objnum is equal to objnum -1
                    */
                    MppFrmDrawRects(srcFrm2, remainingBoxs, objNum - 1, RGB888_RED, DRAW_RETC_THICK);
                }
            }
        }
        printf("对复制的内存 srcFrm2 画框如上 \n");
// ======================================== 对复制的内存 srcFrm2 画框如上 ========================================

// ===================================== 获取带框的图片并从内存发给 cloud 如下 =====================================
    printf("获取带框的图片并从内存发给 cloud 如下 \n");
    ret = FrmToOrigImg((VIDEO_FRAME_INFO_S *)srcFrm2, &img2);// 复制数据指针，不复制数据，所以不用释放 img2 。
    // 但是 srcFrm2 肯定要释放的。
    unsigned char *rgb2;
    if((rgb2=(unsigned char *)malloc(3*image_width*image_height)) != NULL)
        memset(rgb2, 0, 3*image_width*image_height);
    YUV420SP_TO_BGR24(&img2, rgb2, image_width, image_height);
    // bgr_to_jpg_memmory1(rgb2, jpgFileNamepointer2, image_width, image_height, &jpg_buf2, &jpg_buf_size2);// cloud
    bgr_to_jpg1(rgb2, jpgFileNamepointer2, image_width, image_height);
    free(rgb2);
    usleep(1000);// 如果不延时，就可能会出现图片数据缺失、混乱的问题。
    if(count_cycle==0){
        // static int send_to_Huawei_cloud1(HI_CHAR *jpgFileNamepointer)
        send_to_Huawei_cloud1(jpgFileNamepointer2);
        // static int send_to_Huawei_cloud2(JPG_BUFTYPE *jpg_buf)
        // send_to_Huawei_cloud2(&jpg_buf2);
        // free(jpg_buf2.start);
    }
    count_cycle++;
    int cycle_Boundary_point = 4;// 2~3s
    if(count_cycle==cycle_Boundary_point) count_cycle=0;
    // free(srcFrm2);// 把 srcFrm2 作为该函数的输入参数，所以在该函数的上层函数中释放即可。
    printf("获取带框的图片并从内存发给 cloud 如上 \n");
// ===================================== 获取带框的图片并从内存发给 cloud 如上 =====================================

    return ret;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
