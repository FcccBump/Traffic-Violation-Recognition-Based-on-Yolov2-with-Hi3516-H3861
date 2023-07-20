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

#ifndef BASE_INTERFACE_H
#define BASE_INTERFACE_H

#include <stdint.h>
#include <stdbool.h>

#if __cplusplus
extern "C" {
#endif

HI_S32 ConfBaseInit(const char* cfgFilePath);
bool IsMainThrd(void);
void ConfBaseExt(void);

int GetCfgInt(const char* key, int defVal);
double GetCfgDouble(const char* key, double defVal);
bool GetCfgBool(const char* key, bool defVal);
const char* GetCfgStr(const char* key, const char* defVal);

int SectGetCfgInt(const char* section, const char* field, int defVal);
double SectGetCfgDouble(const char* section, const char* field, double defVal);
bool SectGetCfgBool(const char* section, const char* field, bool defVal);
const char* SectGetCfgStr(const char* section, const char* field, const char* defVal);

int HiStrxfrm(char *s1, char *s2, int n);

#ifdef __cplusplus
}
#endif
#endif
