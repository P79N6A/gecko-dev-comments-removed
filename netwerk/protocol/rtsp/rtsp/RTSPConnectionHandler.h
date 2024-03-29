















#ifndef RTSP_CONNECTION_HANDLER_H_
#define RTSP_CONNECTION_HANDLER_H_

#include "APacketSource.h"
#include "ARTPConnection.h"
#include "ARTSPConnection.h"
#include "ASessionDescription.h"

#include "RtspPrlog.h"

#include "nsIOService.h"

#include <ctype.h>
#include <cutils/properties.h>

#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/ALooper.h>
#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/MediaErrors.h>

#include <arpa/inet.h>
#include <netdb.h>
#include "nsPrintfCString.h"

#include "mozilla/Logging.h"

#include "prio.h"
#include "prnetdb.h"

extern PRLogModuleInfo* gRtspLog;



static int64_t kAccessUnitTimeoutUs = 10000000ll;


static int64_t kActivateEndOfStreamTimerUs = 2000000ll;


static int64_t kEndOfStreamTimeoutUs = 2000000ll;



static int64_t kPlayTimeoutUs = 10000000ll;

static int64_t kDefaultKeepAliveTimeoutUs = 60000000ll;

namespace android {

static bool GetAttribute(const char *s, const char *key, AString *value) {
    value->clear();

    size_t keyLen = strlen(key);

    for (;;) {
        while (isspace(*s)) {
            ++s;
        }

        const char *colonPos = strchr(s, ';');

        size_t len =
            (colonPos == NULL) ? strlen(s) : colonPos - s;

        if (len >= keyLen + 1 && s[keyLen] == '=' && !strncmp(s, key, keyLen)) {
            value->setTo(&s[keyLen + 1], len - keyLen - 1);
            return true;
        }

        if (colonPos == NULL) {
            return false;
        }

        s = colonPos + 1;
    }
}

struct RtspConnectionHandler : public AHandler {
    enum {
        kWhatConnected = 1000,
        kWhatDisconnected,
        kWhatDescribe,
        kWhatSetup,
        kWhatPause,
        kWhatSeek,
        kWhatPlay,
        kWhatResume,
        kWhatKeepAlive,
        kWhatOptions,
        kWhatEndOfStream,
        kWhatAbort,
        kWhatTeardown,
        kWhatQuit,
        kWhatAccessUnitTimeoutCheck,
        kWhatEndOfStreamCheck,
        kWhatSeekDone,
        kWhatPausedDone,
        kWhatAccessUnitComplete,
        kWhatAccessUnit,
        kWhatSeek1,
        kWhatSeek2,
        kWhatBinary,
        kWhatTimeout,
        kWhatEOS,
        kWhatSeekDiscontinuity,
        kWhatNormalPlayTimeMapping,
        kWhatTryTCPInterleaving,
    };

    RtspConnectionHandler(
            const char *url,
            const char *userAgent,
            const sp<AMessage> &notify,
            bool uidValid = false, uid_t uid = 0)
        : mUserAgent(userAgent),
          mNotify(notify),
          mUIDValid(uidValid),
          mUID(uid),
          mNetLooper(new ALooper),
          mConn(new ARTSPConnection(mUIDValid, mUID)),
          mRTPConn(new ARTPConnection),
          mOriginalSessionURL(url),
          mSessionURL(url),
          mSetupTracksSuccessful(false),
          mSeekPending(false),
          mPausePending(false),
          mAborted(false),
          mFirstAccessUnit(true),
          mNTPAnchorUs(-1),
          mMediaAnchorUs(-1),
          mLastMediaTimeUs(0),
          mNumAccessUnitsReceived(0),
          mCheckPending(false),
          mCheckGeneration(0),
          mTryTCPInterleaving(false),
          mTryFakeRTCP(false),
          mReceivedFirstRTCPPacket(false),
          mReceivedFirstRTPPacket(false),
          mSeekable(false),
          mKeepAliveTimeoutUs(kDefaultKeepAliveTimeoutUs),
          mKeepAliveGeneration(0),
          mNumPlayTimeoutsPending(0) {
        mNetLooper->setName("rtsp net");
        mNetLooper->start(false ,
                          false ,
                          PRIORITY_HIGHEST);

        
        
        AString host, path, user, pass;
        uint16_t port;
        CHECK(ARTSPConnection::ParseURL(
                    mSessionURL.c_str(), &host, &port, &path, &user, &pass));

        if (user.size() > 0) {
            mSessionURL.clear();
            mSessionURL.append("rtsp://");
            mSessionURL.append(host);
            mSessionURL.append(":");
            mSessionURL.append(StringPrintf("%u", port));
            mSessionURL.append(path);

            LOGI("rewritten session url: '%s'", mSessionURL.c_str());
        }

        mSessionHost = host;
        mConn->MakeUserAgent(mUserAgent.c_str());
    }

    void connect() {
        looper()->registerHandler(mConn);
        (1 ? mNetLooper : looper())->registerHandler(mRTPConn);

        sp<AMessage> notify = new AMessage(kWhatBinary, id());
        mConn->observeBinaryData(notify);

        sp<AMessage> reply = new AMessage(kWhatConnected, id());
        mConn->connect(mOriginalSessionURL.c_str(), reply);
    }

    void disconnect() {
        (new AMessage(kWhatAbort, id()))->post();
    }

    void seek(int64_t timeUs) {
        sp<AMessage> msg = new AMessage(kWhatSeek, id());
        msg->setInt64("time", timeUs);
        msg->post();
    }

    void setCheckPending(bool flag) {
        mCheckPending = flag;
        for (size_t i = 0; i < mTracks.size(); ++i) {
            setEndOfStreamCheckPending(i, flag);
        }
    }

    void setEndOfStreamCheckPending(size_t trackIndex, bool flag) {
        TrackInfo *info = &mTracks.editItemAt(trackIndex);
        if (info) {
            info->mCheckPendings = flag;
        }
    }

    bool getEndOfStreamCheckPending(size_t trackIndex) {
        TrackInfo *info = &mTracks.editItemAt(trackIndex);
        return info->mCheckPendings;
    }

    void play(uint64_t timeUs) {
        AString request = "PLAY ";
        request.append(mSessionURL);
        request.append(" RTSP/1.0\r\n");

        request.append("Session: ");
        request.append(mSessionID);
        request.append("\r\n");

        request.append(nsPrintfCString("Range: npt=%lld-\r\n", timeUs / 1000000ll).get());
        request.append("\r\n");

        setCheckPending(false);

        sp<AMessage> reply = new AMessage(kWhatPlay, id());
        mConn->sendRequest(request.c_str(), reply);
    }

    void pause() {
        if (!mSeekable) {
            return;
        }
        AString request = "PAUSE ";
        request.append(mSessionURL);
        request.append(" RTSP/1.0\r\n");

        request.append("Session: ");
        request.append(mSessionID);
        request.append("\r\n");

        request.append("\r\n");
        
        
        setCheckPending(true);
        ++mCheckGeneration;

        sp<AMessage> reply = new AMessage(kWhatPause, id());
        mConn->sendRequest(request.c_str(), reply);
    }

    void resume(uint64_t timeUs) {
        AString request = "PLAY ";
        request.append(mSessionURL);
        request.append(" RTSP/1.0\r\n");

        request.append("Session: ");
        request.append(mSessionID);
        request.append("\r\n");

        request.append(nsPrintfCString("Range: npt=%lld-\r\n", timeUs / 1000000ll).get());
        request.append("\r\n");

        setCheckPending(false);

        sp<AMessage> reply = new AMessage(kWhatResume, id());
        mConn->sendRequest(request.c_str(), reply);

    }

    static void addRR(const sp<ABuffer> &buf) {
        uint8_t *ptr = buf->data() + buf->size();
        ptr[0] = 0x80 | 0;
        ptr[1] = 201;  
        ptr[2] = 0;
        ptr[3] = 1;
        ptr[4] = 0xde;  
        ptr[5] = 0xad;
        ptr[6] = 0xbe;
        ptr[7] = 0xef;

        buf->setRange(0, buf->size() + 8);
    }

    static void addSDES(PRFileDesc *s, const sp<ABuffer> &buffer,
                        const char *userAgent) {
        PRNetAddr addr;
        CHECK_EQ(PR_GetSockName(s, &addr), PR_SUCCESS);

        uint8_t *data = buffer->data() + buffer->size();
        data[0] = 0x80 | 1;
        data[1] = 202;  
        data[4] = 0xde;  
        data[5] = 0xad;
        data[6] = 0xbe;
        data[7] = 0xef;

        size_t offset = 8;

        data[offset++] = 1;  

        AString cname = "stagefright@";
        char buf[64];
        PR_NetAddrToString(&addr, buf, sizeof(buf));
        cname.append(buf);
        data[offset++] = cname.size();

        memcpy(&data[offset], cname.c_str(), cname.size());
        offset += cname.size();

        data[offset++] = 6;  

        AString tool;
        tool.setTo(userAgent);

        data[offset++] = tool.size();

        memcpy(&data[offset], tool.c_str(), tool.size());
        offset += tool.size();

        data[offset++] = 0;

        if ((offset % 4) > 0) {
            size_t count = 4 - (offset % 4);
            switch (count) {
                case 3:
                    data[offset++] = 0;
                case 2:
                    data[offset++] = 0;
                case 1:
                    data[offset++] = 0;
            }
        }

        size_t numWords = (offset / 4) - 1;
        data[2] = numWords >> 8;
        data[3] = numWords & 0xff;

        buffer->setRange(buffer->offset(), buffer->size() + offset);
    }

    
    
    
    bool pokeAHole(PRFileDesc *rtpSocket, PRFileDesc *rtcpSocket, const AString &transport) {
        PRNetAddr addr;
        addr.inet.family = PR_AF_INET;

        AString source;
        AString server_port;
        PRStatus status = PR_FAILURE;
        if (!GetAttribute(transport.c_str(),
                          "source",
                          &source)) {
            LOGW("Missing 'source' field in Transport response. Using "
                 "RTSP endpoint address.");

            char buffer[PR_NETDB_BUF_SIZE];
            PRHostEnt hostentry;
            status = PR_GetHostByName(mSessionHost.c_str(),
                buffer, PR_NETDB_BUF_SIZE, &hostentry);

            if (status == PR_FAILURE) {
                LOGE("Failed to look up address of session host '%s'",
                     mSessionHost.c_str());

                return false;
            }

            addr.inet.ip = *((uint32_t *) hostentry.h_addr_list[0]);
        } else {
            status = PR_StringToNetAddr(source.c_str(), &addr);
        }

        if (!GetAttribute(transport.c_str(),
                                 "server_port",
                                 &server_port)) {
            LOGI("Missing 'server_port' field in Transport response.");
            return false;
        }

        int rtpPort, rtcpPort;
        if (sscanf(server_port.c_str(), "%d-%d", &rtpPort, &rtcpPort) != 2
                || rtpPort <= 0 || rtpPort > 65535
                || rtcpPort <=0 || rtcpPort > 65535
                || rtcpPort != rtpPort + 1) {
            LOGE("Server picked invalid RTP/RTCP port pair %s,"
                 " RTP port must be even, RTCP port must be one higher.",
                 server_port.c_str());

            return false;
        }

        if (rtpPort & 1) {
            LOGW("Server picked an odd RTP port, it should've picked an "
                 "even one, we'll let it pass for now, but this may break "
                 "in the future.");
        }

        
        if (status == PR_FAILURE) {
            return true;
        }

        if (addr.inet.ip == PR_htonl(PR_INADDR_LOOPBACK)) {
            
            return true;
        }

        
        sp<ABuffer> buf = new ABuffer(65536);
        buf->setRange(0, 0);
        addRR(buf);
        addSDES(rtpSocket, buf, mUserAgent.c_str());

        addr.inet.port = PR_htons(rtpPort);

        if (!rtpSocket) {
            return false;
        }
        ssize_t n = PR_SendTo(rtpSocket, buf->data(), buf->size(), 0, &addr,
                              PR_INTERVAL_NO_WAIT);

        if (n < (ssize_t)buf->size()) {
            LOGE("failed to poke a hole for RTP packets");
            return false;
        }

        addr.inet.port = PR_htons(rtcpPort);

        if (!rtcpSocket) {
            return false;
        }
        n = PR_SendTo(rtcpSocket, buf->data(), buf->size(), 0,
                      &addr, PR_INTERVAL_NO_WAIT);

        if (n < (ssize_t)buf->size()) {
            LOGE("failed to poke a hole for RTCP packets");
            return false;
        }

        LOGV("successfully poked holes.");

        return true;
    }

    virtual void onMessageReceived(const sp<AMessage> &msg) {
        switch (msg->what()) {
            case kWhatConnected:
            {
                int32_t result;
                CHECK(msg->findInt32("result", &result));
                mAborted = false;

                LOGI("connection request completed with result %d (%s)",
                     result, strerror(-result));

                if (result == OK) {
                    AString request;
                    request = "DESCRIBE ";
                    request.append(mSessionURL);
                    request.append(" RTSP/1.0\r\n");
                    request.append("Accept: application/sdp\r\n");
                    request.append("\r\n");

                    sp<AMessage> reply = new AMessage(kWhatDescribe, id());
                    mConn->sendRequest(request.c_str(), reply);
                } else {
                    sp<AMessage> reply = new AMessage(kWhatDisconnected, id());
                    reply->setInt32("result", result);
                    mConn->disconnect(reply);
                }
                break;
            }

            case kWhatDisconnected:
            {
                ++mKeepAliveGeneration;

                int32_t reconnect;
                if (msg->findInt32("reconnect", &reconnect) && reconnect) {
                    sp<AMessage> reply = new AMessage(kWhatConnected, id());
                    mConn->connect(mOriginalSessionURL.c_str(), reply);
                } else {
                    int32_t result;
                    CHECK(msg->findInt32("result", &result));
                    sp<AMessage> reply = new AMessage(kWhatQuit, id());
                    reply->setInt32("result", result);
                    reply->post();
                }
                break;
            }

            case kWhatDescribe:
            {
                int32_t result;
                CHECK(msg->findInt32("result", &result));

                LOGI("DESCRIBE completed with result %d (%s)",
                     result, strerror(-result));
                if (mAborted) {
                  LOGV("we're aborted, dropping stale packet.");
                  break;
                }

                if (result == OK) {
                    sp<RefBase> obj;
                    CHECK(msg->findObject("response", &obj));
                    sp<ARTSPResponse> response =
                        static_cast<ARTSPResponse *>(obj.get());

                    if (response->mStatusCode == 302) {
                        ssize_t i = response->mHeaders.indexOfKey("location");
                        CHECK_GE(i, 0);

                        mSessionURL = response->mHeaders.valueAt(i);

                        AString request;
                        request = "DESCRIBE ";
                        request.append(mSessionURL);
                        request.append(" RTSP/1.0\r\n");
                        request.append("Accept: application/sdp\r\n");
                        request.append("\r\n");

                        sp<AMessage> reply = new AMessage(kWhatDescribe, id());
                        mConn->sendRequest(request.c_str(), reply);
                        break;
                    }

                    if (response->mStatusCode != 200) {
                        result = UNKNOWN_ERROR;
                    } else {
                        mSessionDesc = new ASessionDescription;

                        mSessionDesc->setTo(
                                response->mContent->data(),
                                response->mContent->size());

                        if (!mSessionDesc->isValid()) {
                            LOGE("Failed to parse session description.");
                            result = ERROR_MALFORMED;
                        } else {
                            ssize_t i = response->mHeaders.indexOfKey("content-base");
                            if (i >= 0) {
                                mBaseURL = response->mHeaders.valueAt(i);
                            } else {
                                i = response->mHeaders.indexOfKey("content-location");
                                if (i >= 0) {
                                    mBaseURL = response->mHeaders.valueAt(i);
                                } else {
                                    mBaseURL = mSessionURL;
                                }
                            }

                            if (!mBaseURL.startsWith("rtsp://")) {
                                
                                
                                
                                

                                LOGW("Server specified a non-absolute base URL"
                                     ", combining it with the session URL to "
                                     "get something usable...");

                                AString tmp;
                                if (!MakeURL(
                                            mSessionURL.c_str(),
                                            mBaseURL.c_str(),
                                            &tmp)) {
                                    LOGE("Fail to make url");
                                    result = ERROR_UNSUPPORTED;
                                }

                                mBaseURL = tmp;
                            }

                            int64_t duration = 0;
                            if (mSessionDesc->getDurationUs(&duration) > 0) {
                                
                                LOGI("This is not a live stream");
                                mSeekable = true;
                            } else {
                                
                                LOGI("This is a live stream");
                                mSeekable = false;
                            }

                            if (mSessionDesc->countTracks() < 2) {
                                
                                
                                

                                LOGW("Session doesn't contain any playable "
                                     "tracks. Aborting.");
                                result = ERROR_UNSUPPORTED;
                            } else {
                                setupTrack(1);
                            }
                        }
                    }
                }

                if (result != OK) {
                    sp<AMessage> reply = new AMessage(kWhatDisconnected, id());
                    reply->setInt32("result", result);
                    mConn->disconnect(reply);
                }
                break;
            }

            case kWhatSetup:
            {
                size_t index;
                CHECK(msg->findSize("index", &index));

                TrackInfo *track = NULL;
                size_t trackIndex;
                if (msg->findSize("track-index", &trackIndex)) {
                    track = &mTracks.editItemAt(trackIndex);
                }

                int32_t result;
                CHECK(msg->findInt32("result", &result));

                LOGI("SETUP(%d) completed with result %d (%s)",
                     index, result, strerror(-result));
                if (mAborted) {
                  LOGV("we're aborted, dropping stale packet.");
                  break;
                }

                if (result == OK) {
                    CHECK(track != NULL);

                    sp<RefBase> obj;
                    CHECK(msg->findObject("response", &obj));
                    sp<ARTSPResponse> response =
                        static_cast<ARTSPResponse *>(obj.get());

                    if (response->mStatusCode != 200) {
                        result = UNKNOWN_ERROR;
                    } else {
                        ssize_t i = response->mHeaders.indexOfKey("session");
                        CHECK_GE(i, 0);

                        mSessionID = response->mHeaders.valueAt(i);

                        mKeepAliveTimeoutUs = kDefaultKeepAliveTimeoutUs;
                        AString timeoutStr;
                        if (GetAttribute(
                                    mSessionID.c_str(), "timeout", &timeoutStr)) {
                            char *end;
                            unsigned long timeoutSecs =
                                strtoul(timeoutStr.c_str(), &end, 10);

                            if (end == timeoutStr.c_str() || *end != '\0') {
                                LOGW("server specified malformed timeout '%s'",
                                     timeoutStr.c_str());

                                mKeepAliveTimeoutUs = kDefaultKeepAliveTimeoutUs;
                            } else if (timeoutSecs < 15) {
                                LOGW("server specified too short a timeout "
                                     "(%lu secs), using default.",
                                     timeoutSecs);

                                mKeepAliveTimeoutUs = kDefaultKeepAliveTimeoutUs;
                            } else {
                                mKeepAliveTimeoutUs = timeoutSecs * 1000000ll;

                                LOGI("server specified timeout of %lu secs.",
                                     timeoutSecs);
                            }
                        }

                        i = mSessionID.find(";");
                        if (i >= 0) {
                            
                            mSessionID.erase(i, mSessionID.size() - i);
                        }

                        sp<AMessage> notify = new AMessage(kWhatAccessUnit, id());
                        notify->setSize("track-index", trackIndex);

                        i = response->mHeaders.indexOfKey("transport");
                        CHECK_GE(i, 0);

                        if (!track->mUsingInterleavedTCP) {
                            AString transport = response->mHeaders.valueAt(i);

                            
                            
                            pokeAHole(
                                    track->mRTPSocket,
                                    track->mRTCPSocket,
                                    transport);
                        }

                        mRTPConn->addStream(
                                track->mRTPSocket, track->mRTCPSocket,
                                track->mInterleavedRTPIdx, track->mInterleavedRTCPIdx,
                                mSessionDesc, index,
                                notify, track->mUsingInterleavedTCP);

                        mSetupTracksSuccessful = true;
                    }
                }

                if (result != OK) {
                    if (track) {
                        if (!track->mUsingInterleavedTCP) {
                            if (track->mRTPSocket) {
                                PR_Close(track->mRTPSocket);
                            }

                            if (track->mRTCPSocket) {
                                PR_Close(track->mRTCPSocket);
                            }
                        }

                        mTracks.removeItemsAt(trackIndex);
                    }
                }

                ++index;
                if (index < mSessionDesc->countTracks()) {
                    setupTrack(index);
                } else if (mSetupTracksSuccessful) {
                    ++mKeepAliveGeneration;
                    postKeepAlive();
                    sp<AMessage> msg = mNotify->dup();
                    if (mSeekable) {
                      msg->setInt32("isSeekable", 1);
                    } else {
                      msg->setInt32("isSeekable", 0);
                    }
                    
                    msg->setInt32("what", kWhatConnected);
                    msg->post();

                    
                    
                    if (mTryTCPInterleaving) {
                      sp<AMessage> msgTryTcp = mNotify->dup();
                      msgTryTcp->setInt32("what", kWhatTryTCPInterleaving);
                      msgTryTcp->post();
                    }
                } else {
                    sp<AMessage> reply = new AMessage(kWhatDisconnected, id());
                    reply->setInt32("result", result);
                    mConn->disconnect(reply);
                }
                break;
            }
            case kWhatPause:
            {
                mPausePending = true;
                LOGI("pause completed");
                sp<AMessage> msg = mNotify->dup();
                msg->setInt32("what", kWhatPausedDone);
                msg->post();
                break;
            }
            case kWhatResume:
                 break;
            case kWhatPlay:
            {
                int32_t result;
                CHECK(msg->findInt32("result", &result));

                LOGI("PLAY completed with result %d (%s)",
                     result, strerror(-result));
                if (mAborted) {
                  LOGV("we're aborted, dropping stale packet.");
                  break;
                }

                if (result == OK) {
                    sp<RefBase> obj;
                    CHECK(msg->findObject("response", &obj));
                    sp<ARTSPResponse> response =
                        static_cast<ARTSPResponse *>(obj.get());
                    if (response->mStatusCode != 200) {
                        result = UNKNOWN_ERROR;
                    } else if (!parsePlayResponse(response)) {
                        result = UNKNOWN_ERROR;
                    } else {
                        sp<AMessage> timeout = new AMessage(kWhatTimeout, id());
                        timeout->post(kPlayTimeoutUs);
                        mPausePending = false;
                        mNumPlayTimeoutsPending++;
                    }
                }

                if (result != OK) {
                    sp<AMessage> reply = new AMessage(kWhatDisconnected, id());
                    reply->setInt32("result", result);
                    mConn->disconnect(reply);
                }

                break;
            }

            case kWhatKeepAlive:
            {
                int32_t generation;
                CHECK(msg->findInt32("generation", &generation));

                if (generation != mKeepAliveGeneration) {
                    
                    break;
                }

                AString request;
                request.append("OPTIONS ");
                request.append(mSessionURL);
                request.append(" RTSP/1.0\r\n");
                request.append("Session: ");
                request.append(mSessionID);
                request.append("\r\n");
                request.append("\r\n");

                sp<AMessage> reply = new AMessage(kWhatOptions, id());
                reply->setInt32("generation", mKeepAliveGeneration);
                mConn->sendRequest(request.c_str(), reply);
                break;
            }

            case kWhatOptions:
            {
                int32_t result;
                CHECK(msg->findInt32("result", &result));

                LOGI("OPTIONS completed with result %d (%s)",
                     result, strerror(-result));

                int32_t generation;
                CHECK(msg->findInt32("generation", &generation));

                if (generation != mKeepAliveGeneration) {
                    
                    break;
                }

                postKeepAlive();
                break;
            }

            case kWhatEndOfStream:
            {
                size_t trackIndex = 0;
                msg->findSize("trackIndex", &trackIndex);
                postQueueEOS(trackIndex, ERROR_END_OF_STREAM);
                break;
            }

            case kWhatAbort:
            {
                for (size_t i = 0; i < mTracks.size(); ++i) {
                    TrackInfo *info = &mTracks.editItemAt(i);
                    if (!info) {
                        continue;
                    }

                    if (!mFirstAccessUnit) {
                        postQueueEOS(i, ERROR_END_OF_STREAM);
                    }

                    if (!info->mUsingInterleavedTCP) {
                        mRTPConn->removeStream(info->mRTPSocket, info->mRTCPSocket);

                        if (info->mRTPSocket) {
                            PR_Close(info->mRTPSocket);
                        }

                        if (info->mRTCPSocket) {
                            PR_Close(info->mRTCPSocket);
                        }
                    }
                }
                mTracks.clear();
                mSetupTracksSuccessful = false;
                mSeekPending = false;
                mPausePending = false;
                mFirstAccessUnit = true;
                mNTPAnchorUs = -1;
                mMediaAnchorUs = -1;
                mReceivedFirstRTCPPacket = false;
                mReceivedFirstRTPPacket = false;
                mSeekable = false;
                mAborted = true;

                sp<AMessage> reply = new AMessage(kWhatTeardown, id());

                int32_t reconnect;
                if (msg->findInt32("reconnect", &reconnect) && reconnect) {
                    reply->setInt32("reconnect", true);
                }

                AString request;
                request = "TEARDOWN ";

                
                request.append(mSessionURL);
                request.append(" RTSP/1.0\r\n");

                request.append("Session: ");
                request.append(mSessionID);
                request.append("\r\n");

                request.append("\r\n");

                mConn->sendRequest(request.c_str(), reply);
                break;
            }

            case kWhatTeardown:
            {
                int32_t result;
                CHECK(msg->findInt32("result", &result));

                LOGI("TEARDOWN completed with result %d (%s)",
                     result, strerror(-result));

                sp<AMessage> reply = new AMessage(kWhatDisconnected, id());
                reply->setInt32("result", result);

                int32_t reconnect;
                if (msg->findInt32("reconnect", &reconnect) && reconnect) {
                    reply->setInt32("reconnect", true);
                }

                mConn->disconnect(reply);
                break;
            }

            case kWhatQuit:
            {
                int32_t result;
                CHECK(msg->findInt32("result", &result));
                sp<AMessage> msg = mNotify->dup();
                msg->setInt32("what", kWhatDisconnected);
                msg->setInt32("result", result);
                msg->post();
                break;
            }

            case kWhatAccessUnitTimeoutCheck:
            {
                int32_t generation;
                CHECK(msg->findInt32("generation", &generation));
                if (generation != mCheckGeneration) {
                    
                    break;
                }

                if (mNumAccessUnitsReceived == 0) {
                    LOGI("stream ended? aborting.");
                    disconnect();
                    break;
                }
                mNumAccessUnitsReceived = 0;
                msg->post(kAccessUnitTimeoutUs);
                break;
            }

            case kWhatEndOfStreamCheck:
            {
                int32_t generation;
                CHECK(msg->findInt32("generation", &generation));
                if (generation != mCheckGeneration) {
                    
                    break;
                }
                size_t trackIndex;
                msg->findSize("trackIndex", &trackIndex);
                TrackInfo *track = &mTracks.editItemAt(trackIndex);
                if (!track) {
                  break;
                }

                if (track->mNumAccessUnitsReceiveds == 0) {
                    if (gIOService->IsOffline()) {
                        LOGI("stream ended? aborting.");
                        disconnect();
                        break;
                    }
                    sp<AMessage> endStreamMsg = new AMessage(kWhatEndOfStream, id());
                    endStreamMsg->setSize("trackIndex", trackIndex);
                    endStreamMsg->post();
                    break;
                }
                track->mNumAccessUnitsReceiveds = 0;
                msg->post(kEndOfStreamTimeoutUs);
                break;
            }

            case kWhatAccessUnit:
            {
                int32_t timeUpdate;
                if (msg->findInt32("time-update", &timeUpdate) && timeUpdate) {
                    size_t trackIndex;
                    CHECK(msg->findSize("track-index", &trackIndex));

                    uint32_t rtpTime;
                    uint64_t ntpTime;
                    CHECK(msg->findInt32("rtp-time", (int32_t *)&rtpTime));
                    CHECK(msg->findInt64("ntp-time", (int64_t *)&ntpTime));

                    onTimeUpdate(trackIndex, rtpTime, ntpTime);
                    break;
                }

                int32_t first;
                if (msg->findInt32("first-rtcp", &first)) {
                    mReceivedFirstRTCPPacket = true;
                    break;
                }

                if (msg->findInt32("first-rtp", &first)) {
                    mReceivedFirstRTPPacket = true;
                    break;
                }

                mNumAccessUnitsReceived++;
                postAccessUnitTimeoutCheck();

                size_t trackIndex;
                CHECK(msg->findSize("track-index", &trackIndex));

                if (trackIndex >= mTracks.size()) {
                    LOGV("late packets ignored.");
                    break;
                }

                TrackInfo *track = &mTracks.editItemAt(trackIndex);

                int32_t eos;
                if (msg->findInt32("eos", &eos)) {
                    LOGI("received BYE on track index %d", trackIndex);
                    return;
                }

                sp<RefBase> obj;
                CHECK(msg->findObject("access-unit", &obj));

                sp<ABuffer> accessUnit = static_cast<ABuffer *>(obj.get());

                uint32_t seqNum = (uint32_t)accessUnit->int32Data();

                if (mSeekPending) {
                    LOGV("we're seeking, dropping stale packet.");
                    break;
                }

                if (mPausePending) {
                    LOGV("we're pausing, dropping stale packet.");
                    break;
                }

                if (mAborted) {
                  LOGV("we're aborted, dropping stale packet.");
                  break;
                }

                if (seqNum < track->mFirstSeqNumInSegment) {
                    LOGV("dropping stale access-unit (%d < %d)",
                         seqNum, track->mFirstSeqNumInSegment);
                    break;
                }

                if (track->mNewSegment) {
                    track->mNewSegment = false;
                }

                onAccessUnitComplete(trackIndex, accessUnit);

                
                track->mNumAccessUnitsReceiveds++;
                int64_t duration, timeUs;
                mSessionDesc->getDurationUs(&duration);
                accessUnit->meta()->findInt64("timeUs", &timeUs);

                
                if (timeUs >= duration - kActivateEndOfStreamTimerUs) {
                    postEndOfStreamCheck(trackIndex);
                }
                break;
            }

            case kWhatSeek:
            {
                if (!mSeekable) {
                    LOGW("This is a live stream, ignoring seek request.");

                    sp<AMessage> msg = mNotify->dup();
                    msg->setInt32("what", kWhatSeekDone);
                    msg->post();
                    break;
                }

                int64_t timeUs;
                CHECK(msg->findInt64("time", &timeUs));

                mSeekPending = true;

                
                
                setCheckPending(true);

                ++mCheckGeneration;

                AString request = "PAUSE ";
                request.append(mSessionURL);
                request.append(" RTSP/1.0\r\n");

                request.append("Session: ");
                request.append(mSessionID);
                request.append("\r\n");

                request.append("\r\n");

                sp<AMessage> reply = new AMessage(kWhatSeek1, id());
                reply->setInt64("time", timeUs);
                mConn->sendRequest(request.c_str(), reply);
                break;
            }

            case kWhatSeek1:
            {
                if (mAborted || !mSeekPending) {
                    LOGV("We're aborted, dropping stale packet.");
                    break;
                }

                
                for (size_t i = 0; i < mTracks.size(); ++i) {
                    TrackInfo *info = &mTracks.editItemAt(i);

                    postQueueSeekDiscontinuity(i);

                    info->mRTPAnchor = 0;
                    info->mNTPAnchorUs = -1;
                }

                mNTPAnchorUs = -1;

                int64_t timeUs;
                CHECK(msg->findInt64("time", &timeUs));

                AString request = "PLAY ";
                request.append(mSessionURL);
                request.append(" RTSP/1.0\r\n");

                request.append("Session: ");
                request.append(mSessionID);
                request.append("\r\n");
                request.append(nsPrintfCString("Range: npt=%lld-\r\n", timeUs / 1000000ll).get());
                request.append("\r\n");

                sp<AMessage> reply = new AMessage(kWhatSeek2, id());
                mConn->sendRequest(request.c_str(), reply);
                break;
            }

            case kWhatSeek2:
            {
                int32_t result;
                CHECK(msg->findInt32("result", &result));

                LOGI("PLAY completed with result %d (%s)",
                     result, strerror(-result));
                if (mAborted || !mSeekPending) {
                    LOGV("We're aborted, dropping stale packet.");
                    break;
                }

                mCheckPending = false;
                postAccessUnitTimeoutCheck();

                for (size_t i = 0; i < mTracks.size(); i++) {
                    setEndOfStreamCheckPending(i, false);
                    postEndOfStreamCheck(i);
                }

                if (result == OK) {
                    sp<RefBase> obj;
                    CHECK(msg->findObject("response", &obj));
                    sp<ARTSPResponse> response =
                        static_cast<ARTSPResponse *>(obj.get());

                    if (response->mStatusCode != 200) {
                        result = UNKNOWN_ERROR;
                    } else if (!parsePlayResponse(response)) {
                        result = UNKNOWN_ERROR;
                    } else {
                        ssize_t i = response->mHeaders.indexOfKey("rtp-info");
                        if (i < 0) {
                            LOGE("No RTP info in response");
                            (new AMessage(kWhatAbort, id()))->post();
                        }

                        LOGV("rtp-info: %s", response->mHeaders.valueAt(i).c_str());

                        LOGI("seek completed.");
                    }
                }

                if (result != OK) {
                    LOGE("seek failed, aborting.");
                    (new AMessage(kWhatAbort, id()))->post();
                }

                mSeekPending = false;
                mPausePending = false;

                sp<AMessage> msg = mNotify->dup();
                msg->setInt32("what", kWhatSeekDone);
                msg->post();
                break;
            }

            case kWhatBinary:
            {
                sp<RefBase> obj;
                CHECK(msg->findObject("buffer", &obj));
                sp<ABuffer> buffer = static_cast<ABuffer *>(obj.get());

                int32_t index;
                if (!buffer->meta()->findInt32("index", &index)) {
                    LOGW("Cannot find index");
                    break;
                }

                mRTPConn->injectPacket(index, buffer);
                break;
            }

            case kWhatTimeout:
            {
                CHECK(mNumPlayTimeoutsPending >= 1);
                mNumPlayTimeoutsPending--;
                
                
                
                
                
                
                
                if (mNumPlayTimeoutsPending > 0) {
                  
                  return;
                }

                if (!mReceivedFirstRTCPPacket) {
                    if (mReceivedFirstRTPPacket && !mTryFakeRTCP) {
                        LOGW("We received RTP packets but no RTCP packets, "
                             "using fake timestamps.");

                        mTryFakeRTCP = true;

                        mReceivedFirstRTCPPacket = true;

                        fakeTimestamps();
                    } else if (!mReceivedFirstRTPPacket && !mTryTCPInterleaving) {
                        LOGW("Never received any data, switching transports.");

                        mTryTCPInterleaving = true;

                        sp<AMessage> msg = new AMessage(kWhatAbort, id());
                        msg->setInt32("reconnect", true);
                        msg->post();
                    } else {
                        LOGW("Never received any data, disconnecting.");
                        (new AMessage(kWhatAbort, id()))->post();
                    }
                }
                break;
            }

            default:
                TRESPASS();
                break;
        }
    }

    void postKeepAlive() {
        sp<AMessage> msg = new AMessage(kWhatKeepAlive, id());
        msg->setInt32("generation", mKeepAliveGeneration);
        msg->post((mKeepAliveTimeoutUs * 9) / 10);
    }

    void postEndOfStreamCheck(size_t trackIndex) {
        if (getEndOfStreamCheckPending(trackIndex)) {
            return;
        }
        setEndOfStreamCheckPending(trackIndex, true);
        sp<AMessage> check = new AMessage(kWhatEndOfStreamCheck, id());
        check->setInt32("generation", mCheckGeneration);
        check->setSize("trackIndex", trackIndex);
        check->post(kEndOfStreamTimeoutUs);
    }

    void postAccessUnitTimeoutCheck() {
        if (mCheckPending) {
            return;
        }
        mCheckPending = true;
        sp<AMessage> check = new AMessage(kWhatAccessUnitTimeoutCheck, id());
        check->setInt32("generation", mCheckGeneration);
        check->post(kAccessUnitTimeoutUs);
    }

    static void SplitString(
            const AString &s, const char *separator, List<AString> *items) {
        items->clear();
        size_t start = 0;
        while (start < s.size()) {
            ssize_t offset = s.find(separator, start);

            if (offset < 0) {
                items->push_back(AString(s, start, s.size() - start));
                break;
            }

            items->push_back(AString(s, start, offset - start));
            start = offset + strlen(separator);
        }
    }

    bool parsePlayResponse(const sp<ARTSPResponse> &response) {
        mSeekable = false;

        for (size_t i = 0; i < mTracks.size(); ++i) {
          TrackInfo *info = &mTracks.editItemAt(i);
          info->mIsPlayAcked = true;
        }

        ssize_t i = response->mHeaders.indexOfKey("range");
        if (i < 0) {
            
            
            return false;
        }

        AString range = response->mHeaders.valueAt(i);
        LOGV("Range: %s", range.c_str());

        AString val;
        if (!GetAttribute(range.c_str(), "npt", &val)) {
            LOGE("No npt attribute in range");
            return false;
        }

        float npt1, npt2;
        if (!ASessionDescription::parseNTPRange(val.c_str(), &npt1, &npt2)) {
            

            LOGI("This is a live stream");
            return false;
        }

        i = response->mHeaders.indexOfKey("rtp-info");
        if (i < 0) {
            LOGE("No RTP info");
            return false;
        }

        AString rtpInfo = response->mHeaders.valueAt(i);
        List<AString> streamInfos;
        SplitString(rtpInfo, ",", &streamInfos);

        int n = 1;
        for (List<AString>::iterator it = streamInfos.begin();
             it != streamInfos.end(); ++it) {
            (*it).trim();
            LOGV("streamInfo[%d] = %s", n, (*it).c_str());

            if (!GetAttribute((*it).c_str(), "url", &val)) {
                LOGE("No url attribute");
                return false;
            }

            size_t trackIndex = 0;
            while (trackIndex < mTracks.size()
                    && !(val == mTracks.editItemAt(trackIndex).mURL)) {
                ++trackIndex;
            }
            if (trackIndex >= mTracks.size()) {
                LOGE("No matching url");
                return false;
            }

            if (!GetAttribute((*it).c_str(), "seq", &val)) {
                LOGE("No seq attribute");
                return false;
            }

            char *end;
            unsigned long seq = strtoul(val.c_str(), &end, 10);

            TrackInfo *info = &mTracks.editItemAt(trackIndex);
            info->mFirstSeqNumInSegment = seq;
            info->mNewSegment = true;

            if (!GetAttribute((*it).c_str(), "rtptime", &val)) {
                LOGE("No rtptime attribute");
                return false;
            }

            uint32_t rtpTime = strtoul(val.c_str(), &end, 10);

            LOGV("track #%d: rtpTime=%u <=> npt=%.2f", n, rtpTime, npt1);

            info->mNormalPlayTimeRTP = rtpTime;
            info->mNormalPlayTimeUs = (int64_t)(npt1 * 1E6);

            if (!mFirstAccessUnit) {
                postNormalPlayTimeMapping(
                        trackIndex,
                        info->mNormalPlayTimeRTP, info->mNormalPlayTimeUs);
            }

            ++n;
        }

        mSeekable = true;
        return true;
    }

    sp<MetaData> getTrackFormat(size_t index, int32_t *timeScale) {
        CHECK_GE(index, 0u);
        CHECK_LT(index, mTracks.size());

        const TrackInfo &info = mTracks.itemAt(index);

        *timeScale = info.mTimeScale;

        return info.mPacketSource->getFormat();
    }

    size_t countTracks() const {
        return mTracks.size();
    }

private:
    struct TrackInfo {
        AString mURL;
        PRFileDesc *mRTPSocket;
        PRFileDesc *mRTCPSocket;
        int mInterleavedRTPIdx;
        int mInterleavedRTCPIdx;
        bool mUsingInterleavedTCP;
        uint32_t mFirstSeqNumInSegment;
        bool mNewSegment;

        uint32_t mRTPAnchor;
        int64_t mNTPAnchorUs;
        int32_t mTimeScale;

        uint32_t mNormalPlayTimeRTP;
        int64_t mNormalPlayTimeUs;

        sp<APacketSource> mPacketSource;

        
        
        List<sp<ABuffer> > mPackets;
        bool mIsPlayAcked;
        int64_t mNumAccessUnitsReceiveds;
        bool mCheckPendings;
    };

    AString mUserAgent;
    sp<AMessage> mNotify;
    bool mUIDValid;
    uid_t mUID;
    sp<ALooper> mNetLooper;
    sp<ARTSPConnection> mConn;
    sp<ARTPConnection> mRTPConn;
    sp<ASessionDescription> mSessionDesc;
    AString mOriginalSessionURL;  
    AString mSessionURL;
    AString mSessionHost;
    AString mBaseURL;
    AString mSessionID;
    bool mSetupTracksSuccessful;
    bool mSeekPending;
    bool mPausePending;
    bool mAborted;
    bool mFirstAccessUnit;

    int64_t mNTPAnchorUs;
    int64_t mMediaAnchorUs;
    int64_t mLastMediaTimeUs;

    int64_t mNumAccessUnitsReceived;
    bool mCheckPending;
    int32_t mCheckGeneration;
    bool mTryTCPInterleaving;
    bool mTryFakeRTCP;
    bool mReceivedFirstRTCPPacket;
    bool mReceivedFirstRTPPacket;
    bool mSeekable;
    int64_t mKeepAliveTimeoutUs;
    int32_t mKeepAliveGeneration;
    int32_t mNumPlayTimeoutsPending;

    Vector<TrackInfo> mTracks;

    void setupTrack(size_t index) {
        sp<APacketSource> source =
            new APacketSource(mSessionDesc, index);

        if (source->initCheck() != OK) {
            LOGW("Unsupported format. Ignoring track #%d.", index);

            sp<AMessage> reply = new AMessage(kWhatSetup, id());
            reply->setSize("index", index);
            reply->setInt32("result", ERROR_UNSUPPORTED);
            reply->post();
            return;
        }

        AString url;
        if (!mSessionDesc->findAttribute(index, "a=control", &url)) {
            LOGW("Unsupported format. Ignoring track #%d.", index);

            sp<AMessage> reply = new AMessage(kWhatSetup, id());
            reply->setSize("index", index);
            reply->setInt32("result", ERROR_UNSUPPORTED);
            reply->post();
            return;
        }

        AString trackURL;
        if (!MakeURL(mBaseURL.c_str(), url.c_str(), &trackURL)) {
            LOGW("Unsupported format. Ignoring track #%d.", index);

            sp<AMessage> reply = new AMessage(kWhatSetup, id());
            reply->setSize("index", index);
            reply->setInt32("result", ERROR_UNSUPPORTED);
            reply->post();
            return;
        }

        mTracks.push(TrackInfo());
        TrackInfo *info = &mTracks.editItemAt(mTracks.size() - 1);
        info->mURL = trackURL;
        info->mPacketSource = source;
        info->mUsingInterleavedTCP = false;
        info->mFirstSeqNumInSegment = 0;
        info->mNewSegment = true;
        info->mRTPAnchor = 0;
        info->mNTPAnchorUs = -1;
        info->mNormalPlayTimeRTP = 0;
        info->mNormalPlayTimeUs = 0ll;
        info->mIsPlayAcked = false;
        info->mNumAccessUnitsReceiveds = 0;
        info->mCheckPendings = false;

        unsigned long PT;
        AString formatDesc;
        AString formatParams;
        mSessionDesc->getFormatType(index, &PT, &formatDesc, &formatParams);

        int32_t timescale;
        int32_t numChannels;
        if (!ASessionDescription::ParseFormatDesc(
                    formatDesc.c_str(), &timescale, &numChannels)) {
            LOGW("Unsupported format. Ignoring track #%d.", index);

            sp<AMessage> reply = new AMessage(kWhatSetup, id());
            reply->setSize("index", index);
            reply->setInt32("result", ERROR_UNSUPPORTED);
            reply->post();
            return;
        }

        info->mTimeScale = timescale;

        LOGV("track #%d URL=%s", mTracks.size(), trackURL.c_str());

        AString request = "SETUP ";
        request.append(trackURL);
        request.append(" RTSP/1.0\r\n");

        if (mTryTCPInterleaving) {
            size_t interleaveIndex = 2 * (mTracks.size() - 1);
            info->mUsingInterleavedTCP = true;
            info->mInterleavedRTPIdx = interleaveIndex;
            info->mInterleavedRTCPIdx = interleaveIndex + 1;

            request.append("Transport: RTP/AVP/TCP;interleaved=");
            request.append(info->mInterleavedRTPIdx);
            request.append("-");
            request.append(info->mInterleavedRTCPIdx);
        } else {
            uint16_t rtpPort;
            ARTPConnection::MakePortPair(
                    &info->mRTPSocket, &info->mRTCPSocket, &rtpPort);

            request.append("Transport: RTP/AVP/UDP;unicast;client_port=");
            request.append(rtpPort);
            request.append("-");
            request.append(rtpPort + 1);
        }

        request.append("\r\n");

        if (index > 1) {
            request.append("Session: ");
            request.append(mSessionID);
            request.append("\r\n");
        }

        request.append("\r\n");

        sp<AMessage> reply = new AMessage(kWhatSetup, id());
        reply->setSize("index", index);
        reply->setSize("track-index", mTracks.size() - 1);
        mConn->sendRequest(request.c_str(), reply);
    }

    static bool MakeURL(const char *baseURL, const char *url, AString *out) {
        out->clear();

        if (strncasecmp("rtsp://", baseURL, 7)) {
            
            return false;
        }

        if (!strncasecmp("rtsp://", url, 7)) {
            
            out->setTo(url);
            return true;
        }

        size_t n = strlen(baseURL);
        if (baseURL[n - 1] == '/') {
            out->setTo(baseURL);
            out->append(url);
        } else {
            const char *slashPos = strrchr(baseURL, '/');

            if (slashPos > &baseURL[6]) {
                out->setTo(baseURL, slashPos - baseURL);
            } else {
                out->setTo(baseURL);
            }

            out->append("/");
            out->append(url);
        }

        return true;
    }

    void fakeTimestamps() {
        for (size_t i = 0; i < mTracks.size(); ++i) {
            onTimeUpdate(i, 0, 0ll);
        }
    }

    void onTimeUpdate(int32_t trackIndex, uint32_t rtpTime, uint64_t ntpTime) {
        LOGV("onTimeUpdate track %d, rtpTime = 0x%08x, ntpTime = 0x%016llx",
             trackIndex, rtpTime, ntpTime);

        int64_t ntpTimeUs = (int64_t)(ntpTime * 1E6 / (1ll << 32));

        TrackInfo *track = &mTracks.editItemAt(trackIndex);

        track->mRTPAnchor = rtpTime;
        track->mNTPAnchorUs = ntpTimeUs;

        if (mNTPAnchorUs < 0) {
            mNTPAnchorUs = ntpTimeUs;
            mMediaAnchorUs = mLastMediaTimeUs;
        }
    }

    void onAccessUnitComplete(
            int32_t trackIndex, const sp<ABuffer> &accessUnit) {
        LOGV("onAccessUnitComplete track %d", trackIndex);

        if (mFirstAccessUnit) {
            if (mSeekable) {
                for (size_t i = 0; i < mTracks.size(); ++i) {
                    TrackInfo *info = &mTracks.editItemAt(i);

                    postNormalPlayTimeMapping(
                            i,
                            info->mNormalPlayTimeRTP, info->mNormalPlayTimeUs);
                }
            }

            mFirstAccessUnit = false;
        }

        TrackInfo *track = &mTracks.editItemAt(trackIndex);

        if (mNTPAnchorUs < 0 || mMediaAnchorUs < 0 ||
            track->mNTPAnchorUs < 0 || !track->mIsPlayAcked) {
            LOGE("storing accessUnit, no time established yet");
            track->mPackets.push_back(accessUnit);
            return;
        }

        while (!track->mPackets.empty()) {
            sp<ABuffer> accessUnit = *track->mPackets.begin();
            track->mPackets.erase(track->mPackets.begin());

            if (addMediaTimestamp(trackIndex, track, accessUnit)) {
                postQueueAccessUnit(trackIndex, accessUnit);
            }
        }

        if (addMediaTimestamp(trackIndex, track, accessUnit)) {
            postQueueAccessUnit(trackIndex, accessUnit);
        }
    }

    bool addMediaTimestamp(
            int32_t trackIndex, const TrackInfo *track,
            const sp<ABuffer> &accessUnit) {
        uint32_t rtpTime;
        if (!accessUnit->meta()->findInt32(
                    "rtp-time", (int32_t *)&rtpTime)) {
            LOGE("No RTP time in access unit meta");
            return false;
        }

        int64_t relRtpTimeUs =
            (((int64_t)rtpTime - (int64_t)track->mNormalPlayTimeRTP) * 1000000ll)
                / track->mTimeScale;

        int64_t ntpTimeUs = track->mNTPAnchorUs + relRtpTimeUs;

        int64_t mediaTimeUs = mMediaAnchorUs + ntpTimeUs - mNTPAnchorUs;

        if (mediaTimeUs > mLastMediaTimeUs) {
            mLastMediaTimeUs = mediaTimeUs;
        }

        if (mediaTimeUs < 0) {
            LOGV("dropping early accessUnit.");
            return false;
        }

        LOGV("track %d rtpTime=%d mediaTimeUs = %lld us (%.2f secs)",
             trackIndex, rtpTime, mediaTimeUs, mediaTimeUs / 1E6);

        accessUnit->meta()->setInt64("timeUs", mediaTimeUs);

        return true;
    }

    void postQueueAccessUnit(
            size_t trackIndex, const sp<ABuffer> &accessUnit) {
        sp<AMessage> msg = mNotify->dup();
        msg->setInt32("what", kWhatAccessUnitComplete);
        msg->setSize("trackIndex", trackIndex);
        msg->setObject("accessUnit", accessUnit);
        msg->post();
    }

    void postQueueEOS(size_t trackIndex, status_t finalResult) {
        sp<AMessage> msg = mNotify->dup();
        msg->setInt32("what", kWhatEOS);
        msg->setSize("trackIndex", trackIndex);
        msg->setInt32("finalResult", finalResult);
        msg->post();
    }

    void postQueueSeekDiscontinuity(size_t trackIndex) {
        sp<AMessage> msg = mNotify->dup();
        msg->setInt32("what", kWhatSeekDiscontinuity);
        msg->setSize("trackIndex", trackIndex);
        msg->post();
    }

    void postNormalPlayTimeMapping(
            size_t trackIndex, uint32_t rtpTime, int64_t nptUs) {
        sp<AMessage> msg = mNotify->dup();
        msg->setInt32("what", kWhatNormalPlayTimeMapping);
        msg->setSize("trackIndex", trackIndex);
        msg->setInt32("rtpTime", rtpTime);
        msg->setInt64("nptUs", nptUs);
        msg->post();
    }

    DISALLOW_EVIL_CONSTRUCTORS(RtspConnectionHandler);
};

}  

#endif  
