/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ANDROID_EFFECT_SRS_H_
#define ANDROID_EFFECT_SRS_H_

#include <hardware/audio_effect.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef OPENSL_ES_H_
static const effect_uuid_t SL_IID_SRS_ =
    {0x9b059920, 0x016b, 0x11e2, 0x88b9, {0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b}};
const effect_uuid_t * const SL_IID_SRS= &SL_IID_SRS_;
#endif //OPENSL_ES_H_


// Available SRS parameters
typedef enum
{
    SRS_PARAM_TRUEBASS_ENABLE,
    SRS_PARAM_DIALOGCLARITY_ENABLE,
    SRS_PARAM_SURROUND_ENABLE,
    SRS_PARAM_TRUEBASS_SPKER_SIZE,
    SRS_PARAM_TRUEBASS_GAIN,
    SRS_PARAM_DIALOGCLARTY_GAIN,
    SRS_PARAM_DEFINITION_GAIN,
    SRS_PARAM_SURROUND_GAIN,
} tshd_srs_param_t;

// Parameter type definitions:
typedef struct tshd_user_config_s
{
	unsigned truebass_enable;
	unsigned dialogclarity_enable;
	unsigned surround_enable;
	unsigned truebass_spker_size;
	float	 truebass_gain;
	float	 dialogclarity_gain;
	float	 definition_gain;
	float	 surround_gain;
}tshd_user_config_t;


#ifdef __cplusplus
}  // extern "C"
#endif


#endif /*ANDROID_EFFECT_SRS_H_*/
