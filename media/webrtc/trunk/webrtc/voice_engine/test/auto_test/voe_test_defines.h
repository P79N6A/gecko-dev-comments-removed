









#ifndef WEBRTC_VOICE_ENGINE_VOE_TEST_DEFINES_H
#define WEBRTC_VOICE_ENGINE_VOE_TEST_DEFINES_H

#include "webrtc/voice_engine/test/auto_test/voe_test_common.h"


#include "webrtc/engine_configurations.h"




#define _TEST_BASE_
#define _TEST_RTP_RTCP_
#define _TEST_HARDWARE_
#define _TEST_CODEC_
#define _TEST_DTMF_
#define _TEST_VOLUME_
#define _TEST_AUDIO_PROCESSING_
#define _TEST_FILE_
#define _TEST_NETWORK_
#define _TEST_VIDEO_SYNC_
#define _TEST_NETEQ_STATS_
#define _TEST_XMEDIA_

#define TESTED_AUDIO_LAYER kAudioPlatformDefault













#ifndef WEBRTC_VOICE_ENGINE_CODEC_API
#undef _TEST_CODEC_
#endif
#ifndef WEBRTC_VOICE_ENGINE_VOLUME_CONTROL_API
#undef _TEST_VOLUME_
#endif
#ifndef WEBRTC_VOICE_ENGINE_DTMF_API
#undef _TEST_DTMF_
#endif
#ifndef WEBRTC_VOICE_ENGINE_RTP_RTCP_API
#undef _TEST_RTP_RTCP_
#endif
#ifndef WEBRTC_VOICE_ENGINE_AUDIO_PROCESSING_API
#undef _TEST_AUDIO_PROCESSING_
#endif
#ifndef WEBRTC_VOICE_ENGINE_FILE_API
#undef _TEST_FILE_
#endif
#ifndef WEBRTC_VOICE_ENGINE_VIDEO_SYNC_API
#undef _TEST_VIDEO_SYNC_
#endif
#ifndef WEBRTC_VOICE_ENGINE_HARDWARE_API
#undef _TEST_HARDWARE_
#endif
#ifndef WEBRTC_VOICE_ENGINE_EXTERNAL_MEDIA_API
#undef _TEST_XMEDIA_
#endif
#ifndef WEBRTC_VOICE_ENGINE_NETEQ_STATS_API
#undef _TEST_NETEQ_STATS_
#endif


#ifdef __INSURE__
#define _INSTRUMENTATION_TESTING_
#endif

#define MARK() TEST_LOG("."); fflush(NULL);             // Add test marker
#define ANL() TEST_LOG("\n")                            // Add New Line
#define AOK() TEST_LOG("[Test is OK]"); fflush(NULL);   // Add OK
#if defined(_WIN32)
#define PAUSE                                      \
    {                                               \
        TEST_LOG("Press any key to continue...");   \
        _getch();                                   \
        TEST_LOG("\n");                             \
    }
#else
#define PAUSE                                          \
    {                                                   \
        TEST_LOG("Continuing (pause not supported)\n"); \
    }
#endif

#define TEST(s)                         \
    {                                   \
        TEST_LOG("Testing: %s", #s);    \
    }                                   \

#ifdef _INSTRUMENTATION_TESTING_

#define TEST_MUSTPASS(expr)                                               \
    {                                                                     \
        if ((expr))                                                       \
        {                                                                 \
            TEST_LOG_ERROR("Error at line:%i, %s \n",__LINE__, #expr);    \
            TEST_LOG_ERROR("Error code: %i\n",voe_base_->LastError());    \
        }                                                                 \
    }
#define TEST_ERROR(code)                                                \
    {                                                                   \
        int err = voe_base_->LastError();                               \
        if (err != code)                                                \
        {                                                               \
            TEST_LOG_ERROR("Invalid error code (%d, should be %d) at line %d\n",
                           code, err, __LINE__);
}
}
#else
#define ASSERT_TRUE(expr) TEST_MUSTPASS(!(expr))
#define ASSERT_FALSE(expr) TEST_MUSTPASS(expr)
#define TEST_MUSTFAIL(expr) TEST_MUSTPASS(!((expr) == -1))
#define TEST_MUSTPASS(expr)                                              \
    {                                                                    \
        if ((expr))                                                      \
        {                                                                \
            TEST_LOG_ERROR("\nError at line:%i, %s \n",__LINE__, #expr); \
            TEST_LOG_ERROR("Error code: %i\n", voe_base_->LastError());  \
            PAUSE                                                        \
            return -1;                                                   \
        }                                                                \
    }
#define TEST_ERROR(code) \
    {																                                         \
      int err = voe_base_->LastError();                                      \
      if (err != code)                                                       \
      {                                                                      \
        TEST_LOG_ERROR("Invalid error code (%d, should be %d) at line %d\n", \
                       err, code, __LINE__);                                 \
        PAUSE                                                                \
        return -1;                                                           \
      }															                                         \
    }
#endif  
#define EXCLUDE()                                                   \
    {                                                               \
        TEST_LOG("\n>>> Excluding test at line: %i <<<\n\n",__LINE__);  \
    }

#define INCOMPLETE()                                                \
    {                                                               \
        TEST_LOG("\n>>> Incomplete test at line: %i <<<\n\n",__LINE__);  \
    }

#endif 
