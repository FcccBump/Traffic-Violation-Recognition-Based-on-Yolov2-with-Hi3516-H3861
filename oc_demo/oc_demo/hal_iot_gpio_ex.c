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

#include "hi_types_base.h"
#include "iot_errno.h"
#include "hi_gpio.h"
#include "hi_io.h"
#include "hi_task.h"
#include "iot_gpio_ex.h"

unsigned int IoSetPull(unsigned int id, IotIoPull val)
{
    if (id >= HI_GPIO_IDX_MAX) {
        return IOT_FAILURE;
    }
    return hi_io_set_pull((hi_io_name)id, (hi_io_pull)val);
}

unsigned int IoSetFunc(unsigned int id, unsigned char val)
{
    if (id >= HI_GPIO_IDX_MAX) {
        return IOT_FAILURE;
    }
    return hi_io_set_func((hi_io_name)id, val);
}

unsigned int TaskMsleep(unsigned int ms)
{
    if (ms <= 0) {
        return IOT_FAILURE;
    }
    return hi_sleep((hi_u32)ms);
}