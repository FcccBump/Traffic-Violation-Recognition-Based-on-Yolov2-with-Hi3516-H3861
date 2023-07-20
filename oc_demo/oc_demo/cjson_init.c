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

#include <stdint.h>
#include <cJSON.h>
#include <hi_mem.h>

static void *CJsonMalloc(size_t sz)
{
    return hi_malloc(0, sz);
}

static void CJsonFree(hi_void *p)
{
    hi_free(0, p);
}

void CJsonInit(void)
{
    cJSON_Hooks hooks;
    hooks.malloc_fn = CJsonMalloc;
    hooks.free_fn = CJsonFree;
    cJSON_InitHooks(&hooks);

    return;
}