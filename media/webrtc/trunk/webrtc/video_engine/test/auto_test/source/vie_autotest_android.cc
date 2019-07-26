









#include "../interface/vie_autotest_android.h"

#include <android/log.h>
#include <stdio.h>

#include "vie_autotest.h"
#include "vie_autotest_defines.h"

int ViEAutoTestAndroid::RunAutotest(int testSelection, int subTestSelection,
                                    void* window1, void* window2,
                                    void* javaVM, void* env, void* context) {
  ViEAutoTest vieAutoTest(window1, window2);
  ViETest::Log("RunAutoTest(%d, %d)", testSelection, subTestSelection);
  webrtc::VideoEngine::SetAndroidObjects(javaVM, context);
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

        case 5: 
          vieAutoTest.ViEEncryptionStandardTest();
          break;

        case 6: 
          vieAutoTest.ViEFileStandardTest();
          break;

        case 7: 
          vieAutoTest.ViEImageProcessStandardTest();
          break;

        case 8: 
          vieAutoTest.ViENetworkStandardTest();
          break;

        case 9: 
          vieAutoTest.ViERenderStandardTest();
          break;

        case 10: 
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

        case 5: 
          vieAutoTest.ViEEncryptionAPITest();
          break;

        case 6: 
          vieAutoTest.ViEFileAPITest();
          break;

        case 7: 
          vieAutoTest.ViEImageProcessAPITest();
          break;

        case 8: 
          vieAutoTest.ViENetworkAPITest();
          break;

        case 9: 
          vieAutoTest.ViERenderAPITest();
          break;

        case 10: 
          vieAutoTest.ViERtpRtcpAPITest();
          break;
        case 11:
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

        case 5: 
          vieAutoTest.ViEEncryptionExtendedTest();
          break;

        case 6: 
          vieAutoTest.ViEFileExtendedTest();
          break;

        case 7: 
          vieAutoTest.ViEImageProcessExtendedTest();
          break;

        case 8: 
          vieAutoTest.ViENetworkExtendedTest();
          break;

        case 9: 
          vieAutoTest.ViERenderExtendedTest();
          break;

        case 10: 
          vieAutoTest.ViERtpRtcpExtendedTest();
          break;

        case 11:
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
