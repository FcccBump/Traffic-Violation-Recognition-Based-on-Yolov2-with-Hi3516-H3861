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
 
#ifndef HISIGNALLING_H
#define HISIGNALLING_H

#define HISIGNALLING_MSG_HEADER_LEN         (1)
#define HISGNALLING_MSG_FRAME_HEADER_LEN    (2)
#define HISIGNALLING_MSG_HEADER_TAIL_LEN    (3)
#define HISGNALLING_FREE_TASK_TIME          (10)
#define HISIGNALLING_MSG_MOTOR_ENGINE_LEN   (11)
#define HISIGNALLING_MSG_ONE_FRAME_LEN      (16)
#define HISIGNALLING_MSG_BUFF_LEN           (512)

/*
 * @brief 外设扩展板和Robot板的选择
 * 使用时选择打开宏，使用外设扩展板打开#define BOARD_SELECT_IS_EXPANSION_BOARD这个宏
 * 使用Robot板打开#define BOARD_SELECT_IS_ROBOT_BOARD这个宏
 *
 * @brief Selection of peripheral expansion board and Robot board
 * Select to open the macro when using it,
 * and use the peripheral expansion board to open the #define BOARD_SELECT_IS_EXPANSION_BOARD macro
 * Use the Robot board to open the #define BOARD_SELECT_IS_ROBOT_BOARD macro
 */
#define BOARD_SELECT_IS_EXPANSION_BOARD
#ifdef BOARD_SELECT_IS_EXPANSION_BOARD
#define EXPANSION_BOARD
#else
#define ROBOT_BOARD
#endif

typedef enum {
    EAGE_0 = 0,
    EAGE_1,
    EAGE_2,
    EAGE_3
} GPioEage;

typedef enum {
    FistGesture = 0x1,
    ForefingerGesture,
    OkGesture,
    PalmGesture,
    YesGesture,
    ForefingerAndThumbGesture,
    LittleFingerAndThumbGesture,
    InvalidGesture
} refuseClassification;

typedef struct {
    unsigned char frameHeader[HISGNALLING_MSG_FRAME_HEADER_LEN];
    unsigned char hisignallingMsgBuf[HISIGNALLING_MSG_BUFF_LEN];
    unsigned int hisigallingMsgLen;
    unsigned char endOfFrame;
    unsigned int hisignallingCrc32Check;
} HisignallingProtocalType;

typedef enum {
    HISIGNALLING_RET_VAL_CORRECT = 0,
    HISIGNALLING_RET_VAL_ERROR,
    HISGNALLING_RET_VAL_MAX
} HisignallingErrorType;

/*
 * hisignalling协议函数声明
 * hisignalling protocal Function declaration
 */
unsigned int UartOpenInit(void);
void UartSendRead(int fd, refuseClassification refuseType);
static HisignallingErrorType HisignallingMsgReceive(int fd, unsigned char *buf, unsigned int len);
static unsigned int HisignallingMsgSend(int fd, char *buf, unsigned int dataLen);
unsigned int HisignallingMsgTask(void);

#endif