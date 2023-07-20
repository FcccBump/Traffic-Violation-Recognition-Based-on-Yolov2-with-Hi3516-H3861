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

/**
 * This file make use the hmac to make mqtt pwd.The method is use the date string to hash the device passwd .
 * Take care that this implement depends on the hmac of the mbedtls
*/

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include "md.h"
#include "md_internal.h"

#define CN_HMAC256_LEN   32
#define BUF_OUT_LEN_1 (1)
#define BUF_OUT_LEN_2 (2)

#define IOT_BIT_4 (4)
#define LOW_BIT_4 ((unsigned char)0x0f)
#define IOT_HEX_TO_STRING_1 (9)
#define IOT_HEX_TO_STRING_2 (10)

// make a byte to 2 ascii hex
static int Byte2HexStr(unsigned char *bufin, int len, char *bufout)
{
    int i;
    unsigned char  tmp_l;
    unsigned char  tmp_h;
    if ((bufin == NULL) || (len <= 0) || (bufout == NULL)) {
        return -1;
    }
    for (i = 0; i < len; i++) {
        tmp_h = (bufin[i] >> IOT_BIT_4) & LOW_BIT_4;
        tmp_l = bufin[i] & LOW_BIT_4;
        bufout[BUF_OUT_LEN_2 * i] = (tmp_h > IOT_HEX_TO_STRING_1) ? \
            (tmp_h - IOT_HEX_TO_STRING_2 + 'a') : (tmp_h +'0');
        bufout[BUF_OUT_LEN_2 * i + BUF_OUT_LEN_1] = (tmp_l > IOT_HEX_TO_STRING_1) ? \
            (tmp_l - IOT_HEX_TO_STRING_2 + 'a') : (tmp_l +'0');
    }
    bufout[BUF_OUT_LEN_2 * len] = '\0';

    return 0;
}

#define HMAC_PWD_LEN   65

int HmacGeneratePwd(unsigned char *content, int contentLen, unsigned char *key, int keyLen,
                    unsigned char *buf)
{
    int ret = -1;
    mbedtls_md_context_t mbedtls_md_ctx;
    const mbedtls_md_info_t *mdInfo;
    unsigned char hash[CN_HMAC256_LEN];
    if (key == NULL || content == NULL || buf == NULL || keyLen == 0 || contentLen == 0 ||
        ((CN_HMAC256_LEN * 2 + 1) > HMAC_PWD_LEN)) { /* 2倍的CN_HMAC256_LEN+1判断buflen是否合理 */
        return ret;
    }

    mdInfo = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    if (mdInfo == NULL || (size_t)mdInfo->size > CN_HMAC256_LEN) {
        return ret;
    }

    mbedtls_md_init(&mbedtls_md_ctx);
    ret = mbedtls_md_setup(&mbedtls_md_ctx, mdInfo, 1);
    if (ret != 0) {
        mbedtls_md_free(&mbedtls_md_ctx);
        return ret;
    }

    (void)mbedtls_md_hmac_starts(&mbedtls_md_ctx, key, keyLen);
    (void)mbedtls_md_hmac_update(&mbedtls_md_ctx, content, contentLen);
    (void)mbedtls_md_hmac_finish(&mbedtls_md_ctx, hash);

    // transfer the hash code to the string mode
    Byte2HexStr(hash, CN_HMAC256_LEN, (char *)buf);
    return ret;
}

