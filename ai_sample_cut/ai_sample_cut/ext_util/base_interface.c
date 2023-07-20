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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include <unistd.h>
#include <signal.h>

#include "iniparser.h"
#include "sample_media_ai.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#define CFG_KEY_MAX         256 // Maximum length of config key buf

static dictionary *g_appCfg = NULL; // Global configuration
static pthread_t g_mainThrdId = 0; // Main thread ID, the thread that called app_base_init()

HI_S32 ConfBaseInit(const char* cfgFilePath)
{
    /*
     * 不允许重复初始化
     * Do not allow repeated init
     */
    HI_ASSERT(!g_mainThrdId);

    g_mainThrdId = pthread_self();
    SAMPLE_PRT("mainThrdId=%ld\n", (long)g_mainThrdId);

    if (cfgFilePath && *cfgFilePath) {
        g_appCfg = iniparser_load(cfgFilePath);
        if (!g_appCfg) {
            SAMPLE_PRT("load '%s' FAIL, all config to default\n", cfgFilePath);
        }
    } else {
        SAMPLE_PRT("no config file provided, all config to default\n");
        g_appCfg = NULL;
    }
    SAMPLE_PRT("conf file init success\n");

    return 0;
}

void ConfBaseExt(void)
{
    g_mainThrdId = 0;
    if (g_appCfg) {
        iniparser_freedict(g_appCfg);
        g_appCfg = NULL;
    }
    SAMPLE_PRT("conf file exit success\n");
}

/*
 * 获取key对应的int类型配置项
 * Get the int type configuration item corresponding to the key
 */
int GetCfgInt(const char* key, int defVal)
{
    HI_ASSERT(key && *key);
    return g_appCfg ? iniparser_getint(g_appCfg, key, defVal) : defVal;
}

/*
 * 获取key对应的double类型配置项
 * Get the double configuration item corresponding to the key
 */
double GetCfgDouble(const char* key, double defVal)
{
    HI_ASSERT(key && *key);
    return g_appCfg ? iniparser_getdouble(g_appCfg, key, defVal) : defVal;
}

/*
 * 获取key对应的string类型配置项
 * Get the string configuration corresponding to the key
 */
const char* GetCfgStr(const char* key, const char* defVal)
{
    HI_ASSERT(key && *key);
    return g_appCfg ? iniparser_getstring(g_appCfg, key, defVal) : defVal;
}

/*
 * 获取key对应的bool类型配置项
 * Get the bool type configuration item corresponding to the key
 */
bool GetCfgBool(const char* key, bool defVal)
{
    static const size_t trueSize = 4;
    static const size_t falseSize = 5;
    const char *val;

    val = GetCfgStr(key, NULL);
    if (val && *val) {
        if (val[0] == '1' || val[0] == 'y' || val[0] == 'Y') {
            return true;
        } else if (val[0] == '0' || val[0] == 'n' || val[0] == 'N') {
            return false;
        } else if (strncmp(val, "true", trueSize) == 0) {
            return true;
        } else if (strncmp(val, "false", falseSize) == 0) {
            return false;
        } else {
            return defVal;
        }
    } else {
        return defVal;
    }
}

/*
 * 获取section+field对应的int类型配置项
 * Get the int type configuration item corresponding to section+field
 */
int SectGetCfgInt(const char* section, const char* field, int defVal)
{
    HI_ASSERT(field && *field);
    char key[CFG_KEY_MAX];

    if (snprintf_s(key, sizeof(key), sizeof(key) - 1, "%s:%s", (section ? section : ""), field) < 0) {
        HI_ASSERT(0);
    }
    return GetCfgInt(key, defVal);
}

/*
 * 获取section+field对应的double类型配置项
 * Get the double configuration item corresponding to section+field
 */
double SectGetCfgDouble(const char* section, const char* field, double defVal)
{
    HI_ASSERT(field && *field);
    char key[CFG_KEY_MAX];

    if (snprintf_s(key, sizeof(key), sizeof(key) - 1, "%s:%s", (section ? section : ""), field) < 0) {
        HI_ASSERT(0);
    }
    return GetCfgDouble(key, defVal);
}

/*
 * 获取section+field对应的bool类型配置项
 * Get the bool type configuration item corresponding to section+field
 */
bool SectGetCfgBool(const char* section, const char* field, bool defVal)
{
    HI_ASSERT(field && *field);
    char key[CFG_KEY_MAX];

    if (snprintf_s(key, sizeof(key), sizeof(key) - 1, "%s:%s", (section ? section : ""), field) < 0) {
        HI_ASSERT(0);
    }
    return GetCfgBool(key, defVal);
}

/*
 * 获取section+field对应的string类型配置项
 * Get the string configuration item corresponding to section+field
 */
const char* SectGetCfgStr(const char* section, const char* field, const char* defVal)
{
    HI_ASSERT(field && *field);
    char key[CFG_KEY_MAX];

    if (snprintf_s(key, sizeof(key), sizeof(key) - 1, "%s:%s", (section ? section : ""), field) < 0) {
        HI_ASSERT(0);
    }
    return GetCfgStr(key, defVal);
}

/*
 * 字符处理接口
 * Character processing interface
 */
int HiStrxfrm(char *s1, char *s2, int n)
{
    int i;

    for (i = 0; (i < n - 1) && s2[i]; i++) {
        s1[i] = s2[i];
    }
    s1[i] = 0;
    return i;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
