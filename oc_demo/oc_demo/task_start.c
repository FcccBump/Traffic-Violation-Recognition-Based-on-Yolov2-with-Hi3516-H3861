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

#include <stdio.h>
#include <unistd.h>
#include "ohos_init.h"
#include "cmsis_os2.h"
#include "iot_watchdog.h"
#include "app_demo_multi_sample.h"

#define TASK_STACK  (1024 * 4)
#define INTERRUPT_TASK_PRIO (26)
#define TRAFFIC_TASK_PRIO   (25)
void KeyInterruptScan(const char *arg)
{
    (void)arg;
    AppMultiSampleDemo();
}

void TrafficLightDemo(const char *arg)
{
    printf("traffic light open ok\r\n");
    TrafficLightFunc();
}

static void InterruptTask(void)
{
    osThreadAttr_t attr;

    attr.name = "Interrupt";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = TASK_STACK;
    attr.priority = INTERRUPT_TASK_PRIO;

    if (osThreadNew((osThreadFunc_t)KeyInterruptScan, NULL, &attr) == NULL) {
        printf("[TrafficLight] Falied to create TrafficLightDemo!\n");
    }
}

SYS_RUN(InterruptTask);

static void StartTask(void)
{
    osThreadAttr_t attr;
    IoTWatchDogDisable();

    attr.name = "TrafficLightDemo";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = TASK_STACK;
    attr.priority = osPriorityNormal;

    if (osThreadNew((osThreadFunc_t)TrafficLightDemo, NULL, &attr) == NULL) {
        printf("[TrafficLight] Falied to create TrafficLightDemo!\n");
    }
}

SYS_RUN(StartTask);