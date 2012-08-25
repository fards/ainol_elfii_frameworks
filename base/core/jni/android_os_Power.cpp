/* //device/libs/android_runtime/android_os_Power.cpp
**
** Copyright 2006, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#define LOG_TAG "power-JNI"

#include "JNIHelp.h"
#include "jni.h"
#include "android_runtime/AndroidRuntime.h"
#include <utils/misc.h>
#include <hardware_legacy/power.h>
#include <cutils/android_reboot.h>

#include <fcntl.h>
#include <utils/Log.h>

namespace android
{
static void writeSysFs(const char *path, const char *val)
{
    int fd;
   
    if((fd = open(path, O_RDWR)) < 0) {
        LOGE("writeSysFs, open %s fail.", path);
        goto exit;
    }

    LOGI("write %s, val:%s\n", path, val);
    
    write(fd, val, strlen(val));
 
exit:
    close(fd);
}

static void readSysFs(const char *path, char *buf, int count)
{
    int fd, r;

    if( NULL == buf )
    {
        LOGE("buf is NULL");
        return;
    }
    
    if((fd = open(path, O_RDONLY)) < 0) {
        LOGE("readSysFs, open %s fail.", path);
        goto exit;
    }

    r = read(fd, buf, count);
    if (r < 0) 
	{
        LOGE("read error: %s, %s\n", path, strerror(errno));
    }

    LOGI("read %s, val:%s\n", path, buf);
 
exit:
    close(fd);
}
 
static bool
closeCPU1()
{
    const char *CPU1_ONLINE = "/sys/devices/system/cpu/cpu1/online"; 
    const char *CPU_ONLINE = "/sys/devices/system/cpu/online"; 

    bool ret = true;
    char online[10] = {0};
    
    readSysFs(CPU_ONLINE, (char*)online, 10);
    if( ( '0' == online[0]) && ( 10 == online[1]) )
    {
        LOGI("now CPU1 is closed");
    }
    else
    {
        memset(online, 0, 10);
        writeSysFs(CPU1_ONLINE, "0");
        readSysFs(CPU_ONLINE, online, 10);
        if( ( '0' == online[0]) && ( 10 == online[1]) )
        {
            LOGI("close CPU1 done");
        }
        else
        {
            LOGI("close CPU1 fail");
            ret = false;
        }
    }

    return ret;
}

static void
acquireWakeLock(JNIEnv *env, jobject clazz, jint lock, jstring idObj)
{
    if (idObj == NULL) {
        jniThrowNullPointerException(env, "id is null");
        return ;
    }

    const char *id = env->GetStringUTFChars(idObj, NULL);

    acquire_wake_lock(lock, id);

    env->ReleaseStringUTFChars(idObj, id);
}

static void
releaseWakeLock(JNIEnv *env, jobject clazz, jstring idObj)
{
    if (idObj == NULL) {
        jniThrowNullPointerException(env, "id is null");
        return ;
    }

    const char *id = env->GetStringUTFChars(idObj, NULL);

    release_wake_lock(id);

    env->ReleaseStringUTFChars(idObj, id);

}

static int
setLastUserActivityTimeout(JNIEnv *env, jobject clazz, jlong timeMS)
{
    return set_last_user_activity_timeout(timeMS/1000);
}

static int
setScreenState(JNIEnv *env, jobject clazz, jboolean on)
{
    return set_screen_state(on);
}

static void android_os_Power_shutdown(JNIEnv *env, jobject clazz)
{
    if( closeCPU1() )
        android_reboot(ANDROID_RB_POWEROFF, 0, 0);
}

static void android_os_Power_reboot(JNIEnv *env, jobject clazz, jstring reason)
{
    if( !closeCPU1() )
        return;
    
    if (reason == NULL) {
        android_reboot(ANDROID_RB_RESTART, 0, 0);
    } else {
        const char *chars = env->GetStringUTFChars(reason, NULL);
        android_reboot(ANDROID_RB_RESTART2, 0, (char *) chars);
        env->ReleaseStringUTFChars(reason, chars);  // In case it fails.
    }
    jniThrowIOException(env, errno);
}

static JNINativeMethod method_table[] = {
    { "acquireWakeLock", "(ILjava/lang/String;)V", (void*)acquireWakeLock },
    { "releaseWakeLock", "(Ljava/lang/String;)V", (void*)releaseWakeLock },
    { "setLastUserActivityTimeout", "(J)I", (void*)setLastUserActivityTimeout },
    { "setScreenState", "(Z)I", (void*)setScreenState },
    { "shutdown", "()V", (void*)android_os_Power_shutdown },
    { "rebootNative", "(Ljava/lang/String;)V", (void*)android_os_Power_reboot },
};

int register_android_os_Power(JNIEnv *env)
{
    return AndroidRuntime::registerNativeMethods(
        env, "android/os/Power",
        method_table, NELEM(method_table));
}

};
