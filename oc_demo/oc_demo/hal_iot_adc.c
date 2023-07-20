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

#ifndef HAL_IOT_ADC_H
#define HAL_IOT_ADC_H

#include "hi_adc.h"
#include "iot_adc.h"

unsigned int AdcRead(const IotAdcChannelIndex channel, const unsigned short *data,
                     const IotAdcEquModelSel equModel, const IotAdcCurBais curBais, unsigned short rstCnt)
{
    return hi_adc_read((hi_adc_channel_index)channel, (hi_u16*)data, (hi_adc_equ_model_sel)equModel,
                       (hi_adc_cur_bais)curBais, (hi_u16)rstCnt);
}
#endif