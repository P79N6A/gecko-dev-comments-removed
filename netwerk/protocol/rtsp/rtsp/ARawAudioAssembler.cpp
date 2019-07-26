
















#define LOG_TAG "ARawAudioAssembler"
#include <utils/Log.h>

#include "ARawAudioAssembler.h"

#include "ARTPSource.h"
#include "ASessionDescription.h"

#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/foundation/hexdump.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MetaData.h>
#include <media/stagefright/Utils.h>

namespace android {

ARawAudioAssembler::ARawAudioAssembler(
        const sp<AMessage> &notify, const char *desc, const AString &params)
    : mNotifyMsg(notify),
      mNextExpectedSeqNoValid(false),
      mNextExpectedSeqNo(0) {
}

ARawAudioAssembler::~ARawAudioAssembler() {
}

ARTPAssembler::AssemblyStatus ARawAudioAssembler::assembleMore(
        const sp<ARTPSource> &source) {
    return addPacket(source);
}

ARTPAssembler::AssemblyStatus ARawAudioAssembler::addPacket(
        const sp<ARTPSource> &source) {
    List<sp<ABuffer> > *queue = source->queue();

    if (queue->empty()) {
        return NOT_ENOUGH_DATA;
    }

    if (mNextExpectedSeqNoValid) {
        List<sp<ABuffer> >::iterator it = queue->begin();
        while (it != queue->end()) {
            if ((uint32_t)(*it)->int32Data() >= mNextExpectedSeqNo) {
                break;
            }

            it = queue->erase(it);
        }

        if (queue->empty()) {
            return NOT_ENOUGH_DATA;
        }
    }

    sp<ABuffer> buffer = *queue->begin();

    if (!mNextExpectedSeqNoValid) {
        mNextExpectedSeqNoValid = true;
        mNextExpectedSeqNo = (uint32_t)buffer->int32Data();
    } else if ((uint32_t)buffer->int32Data() != mNextExpectedSeqNo) {
        LOGV("Not the sequence number I expected");

        return WRONG_SEQUENCE_NUMBER;
    }

    

    if (buffer->size() < 1) {
        queue->erase(queue->begin());
        ++mNextExpectedSeqNo;

        LOGV("raw audio packet too short.");

        return MALFORMED_PACKET;
    }

    sp<AMessage> msg = mNotifyMsg->dup();
    msg->setObject("access-unit", buffer);
    msg->post();

    queue->erase(queue->begin());
    ++mNextExpectedSeqNo;

    return OK;
}

void ARawAudioAssembler::packetLost() {
    CHECK(mNextExpectedSeqNoValid);
    ++mNextExpectedSeqNo;
}

void ARawAudioAssembler::onByeReceived() {
    sp<AMessage> msg = mNotifyMsg->dup();
    msg->setInt32("eos", true);
    msg->post();
}


bool ARawAudioAssembler::Supports(const char *desc) {
    return !strncmp(desc, "PCMU/", 5)
        || !strncmp(desc, "PCMA/", 5);
}


void ARawAudioAssembler::MakeFormat(
        const char *desc, const sp<MetaData> &format) {
    if (!strncmp(desc, "PCMU/", 5)) {
        format->setCString(kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_G711_MLAW);
    } else if (!strncmp(desc, "PCMA/", 5)) {
        format->setCString(kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_G711_ALAW);
    } else {
        TRESPASS();
    }

    int32_t sampleRate, numChannels;
    ASessionDescription::ParseFormatDesc(
            desc, &sampleRate, &numChannels);

    format->setInt32(kKeySampleRate, sampleRate);
    format->setInt32(kKeyChannelCount, numChannels);
}

}  

