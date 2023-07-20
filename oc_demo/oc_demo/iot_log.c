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

#include <iot_log.h>

static EnIotLogLevelT g_ioTLogLevel = EN_IOT_LOG_LEVEL_TRACE;
static const char *g_ioTLogLevelNames[] = {
    "TRACE",
    "DEBUG",
    "INFO ",
    "WARN ",
    "ERROR",
    "FATAL"
};

int IoTLogLevelSet(EnIotLogLevelT level)
{
    int ret = -1;
    if (level < EN_IOT_LOG_LEVEL_MAX) {
        g_ioTLogLevel = level;
        ret = 0;
    }
    return ret;
}

EnIotLogLevelT IoTLogLevelGet(void)
{
    return g_ioTLogLevel;
}

const char *IoTLogLevelGetName(EnIotLogLevelT logLevel)
{
    if (logLevel >= EN_IOT_LOG_LEVEL_MAX) {
        return "NULL ";
    } else {
        return g_ioTLogLevelNames[logLevel];
    }
}