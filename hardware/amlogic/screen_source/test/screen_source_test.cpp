/*
 * Copyright (C) 2013 The Android Open Source Project
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

#define LOG_NDEBUG 0
#define LOG_TAG "screen_source_test"
#include <hardware/hardware.h>
#include <hardware/aml_screen.h>

#include <errno.h>
#include <cutils/log.h>
#include <cutils/atomic.h>

#include <stdio.h>
#include <assert.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/stat.h>


#ifndef LOGD
#define LOGD ALOGD
#endif
#ifndef LOGV
#define LOGV ALOGV
#endif
#ifndef LOGE
#define LOGE ALOGE
#endif
#ifndef LOGI
#define LOGI ALOGI
#endif
/*****************************************************************************/
int main(int argc, char **argv) {
    aml_screen_module_t* mModule;
    aml_screen_device_t* mDev;
    int ret = 0;
    int dumpfd;
	char * pSrcBufffer;
	int i = 100;
	
    if (hw_get_module(AML_SCREEN_HARDWARE_MODULE_ID,
                (const hw_module_t **)&mModule) < 0)
    {
    	LOGE("can not get screen source module");
    	ret = -1;
    }
    else
    {
        LOGV("succeed to get screen source module");
        mModule->common.methods->open((const hw_module_t *)mModule,
            AML_SCREEN_SOURCE, (struct hw_device_t**)&mDev);
        //do test here, we can use ops of mDev to operate vdin source
    }

    mDev->ops.start(mDev);

	dumpfd = open("/tmp/yuvsource", O_CREAT | O_RDWR | O_TRUNC, 0644);
	LOGV("dumpfd:%d\n");

    while(i--)
    {
		pSrcBufffer = mDev->ops.aquire_buffer(mDev);

		write(dumpfd, pSrcBufffer, 640*480*3/2);
		
		//usleep(100000);
        LOGV("release_buffer %x pSrcBufffer:%x", mDev->ops.release_buffer, pSrcBufffer);
		mDev->ops.release_buffer(mDev);
	}
	
    mDev->ops.stop(mDev);
	
    return ret;
}
/*****************************************************************************/
