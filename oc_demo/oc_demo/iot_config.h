/*
 * Copyright (c) 2020 Huawei Device Co., Ltd.
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

#ifndef IOT_CONFIG_H
#define IOT_CONFIG_H

// /<CONFIG THE LOG
/* if you need the iot log for the development , please enable it, else please comment it */
#define CONFIG_LINKLOG_ENABLE 1

// /<CONFIG THE WIFI
/* Please modify the ssid and pwd for the own */
#define CONFIG_AP_SSID  "hispark" // WIFI SSID
#define CONFIG_AP_PWD "12345678" // WIFI PWD

// /<Configure the iot platform
/* Please modify the device id and pwd for your own */
// 设备ID名称，请参考华为物联网云文档获取该设备的ID。例如:60790e01ba4b2702c053ff03_helloMQTT
#define CONFIG_DEVICE_ID  "646c990eeb2ee73fc4d344a3_Hispark"
// 设备密码，请参考华为物联网云文档设置该设备密码。例如：hispark2021
#define CONFIG_DEVICE_PWD "318f225800aaa8f8232d510e20b5052c2ee55823bc925af6f27e402bb311ffb3"
#define CONFIG_CLIENTID "646c990eeb2ee73fc4d344a3_Hispark_0_0_2023052311"

static void AppDemoIot(void);
#endif