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

#ifndef IOT_MAIN_H
#define IOT_MAIN_H

typedef void  (*FnMsgCallBack)(int qos, const char *topic, const char *payload);

/*
 * This is the iot main function. Please call this function first
*/
int IoTMain(void);


/**
 * Use this function to set the message call back function, when some messages comes,
 * the callback will be called, if you don't care about the message, set it to NULL
 */
int IoTSetMsgCallback(FnMsgCallBack msgCallback);

/*
 * When you want to send some messages to the iot server(including the response message),
 * please call this api
 * @param qos: the mqtt qos,:0,1,2
 * @param topic: the iot mqtt topic
 * @param payload: the mqtt payload
 * @return 0 success while others failed
 * @instruction: if success means we write the message to the queue susccess,
 * not means communicate with the server success
 */
int IotSendMsg(int qos, const char *topic, const char *payload);

#endif /* IOT_MAIN_H_ */