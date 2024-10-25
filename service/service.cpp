// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

//#define LOG_NDEBUG 0
#define LOG_TAG "android.hardware.media.c2-service-v4l2"

#include <C2Component.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <base/logging.h>
#include <codec2/aidl/ComponentStore.h>
#include <log/log.h>
#include <minijail.h>

#include <v4l2_codec2/components/V4L2ComponentStore.h>

using namespace ::aidl::android::hardware::media::c2;

// This is the absolute on-device path of the prebuild_etc module
// "android.hardware.media.c2-default-seccomp_policy" in Android.bp.
static constexpr char kBaseSeccompPolicyPath[] =
        "/vendor/etc/seccomp_policy/"
        "android.hardware.media.c2-default-seccomp_policy";

// Additional seccomp permissions can be added in this file.
// This file does not exist by default.
static constexpr char kExtSeccompPolicyPath[] =
        "/vendor/etc/seccomp_policy/"
        "android.hardware.media.c2-extended-seccomp_policy";

int main(int /* argc */, char** /* argv */) {
    ALOGD("Service starting...");

    // Set up minijail to limit system calls.
    signal(SIGPIPE, SIG_IGN);
    android::SetUpMinijail(kBaseSeccompPolicyPath, kExtSeccompPolicyPath);

    // Extra threads may be needed to handle a stacked IPC sequence that
    // contains alternating binder and hwbinder calls. (See b/35283480.)
    ABinderProcess_setThreadPoolMaxThreadCount(8);
    ABinderProcess_startThreadPool();

#if LOG_NDEBUG == 0
    ALOGD("Enable all verbose logging of libchrome");
    logging::SetMinLogLevel(-5);
#endif

    // Create IComponentStore service.
    ALOGD("Instantiating Codec2's V4L2 IComponentStore service...");
    std::shared_ptr<IComponentStore> store = ndk::SharedRefBase::make<utils::ComponentStore>(
            android::V4L2ComponentStore::Create());

    const std::string instance = std::string() + IComponentStore::descriptor + "/v4l2";
    binder_status_t status = AServiceManager_addService(store->asBinder().get(), instance.c_str());
    CHECK(status == STATUS_OK);

    ABinderProcess_joinThreadPool();
    return EXIT_FAILURE;  // should not reach
}
