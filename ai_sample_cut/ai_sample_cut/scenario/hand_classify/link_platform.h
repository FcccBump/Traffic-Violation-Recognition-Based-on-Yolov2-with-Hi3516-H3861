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

#ifndef LINKPLATFORM__H
#define LINKPLATFORM__H

// #include <link_service.h>
#include "link_service.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct LinkPlatform {
    int (*open)(struct LinkPlatform* linkP);
    int (*close)(struct LinkPlatform* linkP);
    int (*discover)(struct LinkPlatform* linkP, const char *type);
    int (*setDebuglevel)(struct LinkPlatform* linkP, int level);
    int (*addLinkService)(struct LinkPlatform* linkP, LinkService *service, int flag);
} LinkPlatform;

LinkPlatform* LinkPlatformGet(void);
void LinkPlatformFree(LinkPlatform* linkP);

// /for C++
#ifdef __cplusplus
}
#endif

#endif /* __LINKPLATFORM__H__ */
