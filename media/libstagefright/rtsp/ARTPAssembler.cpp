/*
 * Copyright (C) 2010 The Android Open Source Project
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

#include "ARTPAssembler.h"

#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/AMessage.h>

#include <stdint.h>

namespace android {

static int64_t getNowUs() {
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return (int64_t)tv.tv_usec + tv.tv_sec * 1000000ll;
}

ARTPAssembler::ARTPAssembler()
    : mFirstFailureTimeUs(-1) {
}

void ARTPAssembler::PropagateTimes(
        const sp<ABuffer> &from, const sp<ABuffer> &to) {
    uint32_t rtpTime;
    CHECK(from->meta()->findInt32("rtp-time", (int32_t *)&rtpTime));

    uint64_t ntpTime = 0;
    CHECK(from->meta()->findInt64("ntp-time", (int64_t *)&ntpTime));

    to->meta()->setInt32("rtp-time", rtpTime);
    to->meta()->setInt64("ntp-time", ntpTime);
}

void ARTPAssembler::onPacketReceived(const sp<ARTPSource> &source) {
    AssemblyStatus status;
    for (;;) {
        status = assembleMore(source);

        if (status == WRONG_SEQUENCE_NUMBER) {
            if (mFirstFailureTimeUs >= 0) {
                if (getNowUs() - mFirstFailureTimeUs > 10000ll) {
                    mFirstFailureTimeUs = -1;

                    // LOG(VERBOSE) << "waited too long for packet.";
                    packetLost();
                    continue;
                }
            } else {
                mFirstFailureTimeUs = getNowUs();
            }
            break;
        } else {
            mFirstFailureTimeUs = -1;

            if (status == NOT_ENOUGH_DATA) {
                break;
            }
        }
    }
}

}  // namespace android
