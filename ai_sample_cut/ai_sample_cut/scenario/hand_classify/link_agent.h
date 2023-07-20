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

#ifndef LINK_AGENT_H
#define LINK_AGENT_H

// #include <link_service_agent.h>
// #include <link_platform.h>
#include "link_service_agent.h"
#include "link_platform.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct QueryResult {
    int (*count)(struct QueryResult *q);
    struct LinkServiceAgent *(*at)(struct QueryResult *q, int pos);
} QueryResult;

typedef struct LinkAgent {
    struct QueryResult *(*query)(struct LinkAgent *agent, const char *type);
} LinkAgent;

struct LinkAgent *LinkAgentGet(struct LinkPlatform *p);// // 函数在静态库里面，移植的时候主要注意包括静态库，否则编译报错
void LinkAgentFree(struct LinkAgent *agent);
void QueryResultFree(struct QueryResult *q);

#if defined(__cplusplus)
}
#endif

#endif /* __LINK_AGENT_H__ */