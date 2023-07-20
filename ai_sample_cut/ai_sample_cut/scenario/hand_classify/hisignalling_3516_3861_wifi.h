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

#ifndef HISIGNALLING_3516_3861_wifi_H
#define HISIGNALLING_3516_3861_wifi_H

#define HISIGNALLING_MSG_HEADER_LEN         (1)
#define NUM7         7
#define SEND_BUFF_LEN         22
#define HISGNALLING_MSG_FRAME_HEADER_LEN    (2)
#define HISIGNALLING_MSG_HEADER_TAIL_LEN    (3)
#define HISGNALLING_FREE_TASK_TIME          (10)
#define HISIGNALLING_MSG_MOTOR_ENGINE_LEN	(11)
#define HISIGNALLING_MSG_ONE_FRAME_LEN		(16)
#define HISIGNALLING_MSG_BUFF_LEN           (512)

/**
 * @brief: use this hisignalling Transmission protocol frame format
 *
 * @param frameHeader: Transmission protocol frame header
 *
 * @param hisignallingMsgBuf: Transmission protocol frame buffer
 *
 * @param hisigallingMsgLen: Transmission protocol frame buffer len
 *
 * @param endOfFrame: Transmission protocol frame tail
 *
 * @param hisignallingCrc32Check: Transmission protocol crc32 check
 *
 * */
// typedef struct {// // 定义在 interconnection_server/hisignalling.h 里面，而且该文件有其他东西，更全
//     unsigned char frameHeader[HISGNALLING_MSG_FRAME_HEADER_LEN];
//     unsigned char hisignallingMsgBuf[HISIGNALLING_MSG_BUFF_LEN];
//     unsigned int hisigallingMsgLen;
//     unsigned char endOfFrame;
//     unsigned int hisignallingCrc32Check;
// }HisignallingProtocalType;

/**
 * @brief: use this hisignalling return type
 *
 * @param HISIGNALLING_RET_VAL_CORRECT: return type is correct
 *
 * @param HISIGNALLING_RET_VAL_ERROR: return type is error
 *
 * @param HISIGNALLING_RET_VAL_ERROR: return type is  unknown type
 * */
// typedef enum {// // 定义在 interconnection_server/hisignalling.h 里面，而且该文件有其他东西，更全
//     HISIGNALLING_RET_VAL_CORRECT = 0,
//     HISIGNALLING_RET_VAL_ERROR,
//     HISGNALLING_RET_VAL_MAX
// }HisignallingErrorType;

// // // my add. as histreaming_client_server.c use.
// unsigned int HisignallingDataPackage(HisignallingProtocalType *buf,
//     unsigned int len, unsigned char *hisignallingDataBuf);
// // // my add

#endif