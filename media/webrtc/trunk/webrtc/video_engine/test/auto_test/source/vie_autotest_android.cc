









#include "webrtc/video_engine/test/auto_test/interface/vie_autotest_android.h"

#include <android/log.h>
#include <stdio.h>

#include "webrtc/video_engine/test/auto_test/interface/vie_autotest.h"
#include "webrtc/video_engine/test/auto_test/interface/vie_autotest_defines.h"

int ViEAutoTestAndroid::RunAutotest(int testSelection, int subTestSelection,
                                    void* window1, void* window2,
                                    JavaVM* javaVM, void* env, void* context) {
  ViEAutoTest vieAutoTest(window1, window2);
  ViETest::Log("RunAutoTest(%d, %d)", testSelection, subTestSelection);
  webrtc::VideoEngine::SetAndroidObjects(javaVM);
#ifndef WEBRTC_ANDROID_OPENSLES
  
  webrtc::VoiceEngine::SetAndroidObjects(javaVM, env, context);
#endif

  if (subTestSelection == 0) {
    
    switch (testSelection) {
      case 0:
        vieAutoTest.ViEStandardTest();
        break;
      case 1:
        vieAutoTest.ViEAPITest();
        break;
      case 2:
        vieAutoTest.ViEExtendedTest();
        break;
      case 3:
        vieAutoTest.ViELoopbackCall();
        break;
      default:
        break;
    }
  }

  switch (testSelection) {
    case 0: 
      switch (subTestSelection) {
        case 1: 
          vieAutoTest.ViEBaseStandardTest();
          break;

        case 2: 
          vieAutoTest.ViECaptureStandardTest();
          break;

        case 3: 
          vieAutoTest.ViECodecStandardTest();
          break;

        case 6: 
          vieAutoTest.ViEImageProcessStandardTest();
          break;

        case 7: 
          vieAutoTest.ViENetworkStandardTest();
          break;

        case 8: 
          vieAutoTest.ViERenderStandardTest();
          break;

        case 9: 
          vieAutoTest.ViERtpRtcpStandardTest();
          break;

        default:
          break;
      }
      break;

    case 1:
      switch (subTestSelection) {
        case 1: 
          vieAutoTest.ViEBaseAPITest();
          break;

        case 2: 
          vieAutoTest.ViECaptureAPITest();
          break;

        case 3: 
          vieAutoTest.ViECodecAPITest();
          break;

        case 6: 
          vieAutoTest.ViEImageProcessAPITest();
          break;

        case 7: 
          vieAutoTest.ViENetworkAPITest();
          break;

        case 8: 
          vieAutoTest.ViERenderAPITest();
          break;

        case 9: 
          vieAutoTest.ViERtpRtcpAPITest();
          break;
        case 10:
          break;

        default:
          break;
      }
      break;

    case 2:
      switch (subTestSelection) {
        case 1: 
          vieAutoTest.ViEBaseExtendedTest();
          break;

        case 2: 
          vieAutoTest.ViECaptureExtendedTest();
          break;

        case 3: 
          vieAutoTest.ViECodecExtendedTest();
          break;

        case 6: 
          vieAutoTest.ViEImageProcessExtendedTest();
          break;

        case 7: 
          vieAutoTest.ViERenderExtendedTest();
          break;

        case 8: 
          vieAutoTest.ViERtpRtcpExtendedTest();
          break;

        default:
          break;
      }
      break;

    case 3:
      vieAutoTest.ViELoopbackCall();
      break;

    default:
      break;
    }

  return 0;
}

int main(int argc, char** argv) {
  
  return 0;
}
