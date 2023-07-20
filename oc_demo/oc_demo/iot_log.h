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

#ifndef IOT_LOG_H
#define IOT_LOG_H

#include <stdio.h>

/*
 * @brief:this defines for the log module, and
 * IOT_LOG_TRACE/IOT_LOG_DEBUG will not participate the compile in the release version
 * EN_IOT_LOG_LEVEL_TRACE:this is used as the trace function,like the function enter and function out
 * EN_IOT_LOG_LEVEL_DEBUG：this is used as the debug, you could add any debug as you wish
 * EN_IOT_LOG_LEVEL_INFO：which means it is import message, and you should known
 * EN_IOT_LOG_LEVEL_WARN：this is used as the executed result,
 * which means the status is not what we expected,but could accept
 * EN_IOT_LOG_LEVEL_ERROR：this is used as the executed result,
 * which means the status is not what we expected,could not accepta
 * EN_IOT_LOG_LEVEL_FATAL：this is used as the parameters input for the api interface, which could not accepted
 */
typedef enum {
    EN_IOT_LOG_LEVEL_TRACE = 0,
    EN_IOT_LOG_LEVEL_DEBUG,
    EN_IOT_LOG_LEVEL_INFO,
    EN_IOT_LOG_LEVEL_WARN,
    EN_IOT_LOG_LEVEL_ERROR,
    EN_IOT_LOG_LEVEL_FATAL,
    EN_IOT_LOG_LEVEL_MAX,
}EnIotLogLevelT;

/*
 * @brief:use this function to get the current output log
 * @return: the current output mask log, defined by en_iot_log_level_t
 */
EnIotLogLevelT IoTLogLevelGet(void);

/*
 * @brief: use this function to get the debug level name
 * @parameter[in]:level, the level to get
 * @return: the mapped level name
 */
const char *IoTLogLevelGetName(EnIotLogLevelT logLevel);
/*
 * @brief:use this function to set the current output log
 * @parameter[in] level:defined by en_iot_log_level_t
 * @return: 0 success while -1 failed
 */
int IoTLogLevelSet(EnIotLogLevelT level);

/*
 * @brief: this is a weak function ,and you could rewrite one
 * @param fmt: same use as the fmt for printf
 * @param unfixed: same use for printf
 * @return: don't care about it
 * @attention: and the components should not call this function directly, you'd better
 * call IOT_LOG groups
 */

#ifndef IOT_PRINT
#define IOT_PRINT(fmt, ...) \
    do \
    { \
        printf(fmt, ##__VA_ARGS__); \
    } while (0)
#endif

#ifdef CONFIG_LINKLOG_ENABLE

#define IOT_LOG(level, fmt, ...) \
    do \
    { \
        IOT_PRINT("[%s][%s] " fmt "\r\n", \
        IoTLogLevelGetName((level)), __FUNCTION__, ##__VA_ARGS__); \
    } while (0)

#define IOT_LOG_TRACE(fmt, ...) \
    do \
    { \
        if ((EN_IOT_LOG_LEVEL_TRACE) >= IoTLogLevelGet()) \
        { \
            IOT_LOG(EN_IOT_LOG_LEVEL_TRACE, fmt, ##__VA_ARGS__); \
        } \
    } while (0)

#define IOT_LOG_DEBUG(fmt, ...) \
    do \
    { \
        if ((EN_IOT_LOG_LEVEL_DEBUG) >= IoTLogLevelGet()) \
        { \
            IOT_LOG(EN_IOT_LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__); \
        } \
    } while (0)

#else

#define IOT_LOG(level, fmt, ...)
#define IOT_LOG_TRACE(fmt, ...)
#define IOT_LOG_DEBUG(fmt, ...)

#endif

#define IOT_LOG_TRACE(fmt, ...)   IOT_LOG(EN_IOT_LOG_LEVEL_TRACE, fmt, ##__VA_ARGS__)
#define IOT_LOG_INFO(fmt, ...)   IOT_LOG(EN_IOT_LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#define IOT_LOG_WARN(fmt, ...)   IOT_LOG(EN_IOT_LOG_LEVEL_WARN, fmt, ##__VA_ARGS__)
#define IOT_LOG_ERROR(fmt, ...)  IOT_LOG(EN_IOT_LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)
#define IOT_LOG_FATAL(fmt, ...)  IOT_LOG(EN_IOT_LOG_LEVEL_FATAL, fmt, ##__VA_ARGS__)


#endif /* IOT_LOG_H_ */