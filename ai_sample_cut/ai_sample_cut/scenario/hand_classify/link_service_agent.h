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

#ifndef LinkServiceAgent__H
#define LinkServiceAgent__H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct LinkServiceAgent {
    /*
     * this funtion to get device's property value.
     * s : ServiceAgent
     * property : device's property name
     * value : a json string of device's property value
     * len : value's length
     * StatusOk : get value successfully
     * StatusFailure : failure to get value
     * StatusDeviceOfffline : device had offline
     */
    int (*get)(struct LinkServiceAgent* s, const char* property, char* value, int len);
    /*
     * this funtion to modify device's property
     * s : ServiceAgent
     * property : device's property name
     * value : a json string of device's property
     * len : value's length
     * StatusOk : moified value successfully
     * StatusFailure : failure to moified value
     * StatusDeviceOfffline : device had offline
     */
    int (*modify)(struct LinkServiceAgent* s, const char* property, char* value, int len);
    /*
     * get device's service type
     * if type is not null, then return a string,otherwise,
     * a null string return
     */
    const char* (*type)(struct LinkServiceAgent* s);
    /*
     * get address of device's service
     */
    const char* (*addr)(struct LinkServiceAgent* s);
    /*
     * the id of device's service
     */
    int (*id)(struct LinkServiceAgent* s);
} LinkServiceAgent;

/*
 * free ServiceAgent that malloced by LinkAgent
 */
void LinkServiceAgentFree(LinkServiceAgent* s);

// /for C++
#ifdef __cplusplus
}
#endif

#endif /* __LinkServiceAgent__H__ */
