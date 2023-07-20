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

#ifndef IOT_HMAC_H
#define IOT_HMAC_H

/**
 * This file make use the hmac to make mqtt pwd.The method is use the date string to hash the device passwd .
 * Take care that this implement depends on the hmac of the mbedtls
*/

/**
 * This function used to generate the passwd for the mqtt to connect the HuaweiIoT platform
 * @param content: This is the content for the hmac,and usually it is the device passwd set or get from the Iot Platform
 * @param content_len: The length of the content
 * @param key: This is the key for the hmac, and usually it is the time,
 * used in the client_id:the format is：yearmonthdatehour:like 1970010100
 * @param key_len: The length of the key
 * @param buf: used to storage the hmac code
 * @param buf_len:the buf length
 * @return:0 success while others failed
 */
int HmacGeneratePwd(unsigned char *content, int contentLen, unsigned char *key, int keyLen,
                    unsigned char *buf)
#endif