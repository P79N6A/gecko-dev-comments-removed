









#ifndef WEBRTC_VIDEO_ENGINE_MAIN_TEST_AUTOTEST_INTERFACE_VIE_AUTOTEST_ANDROID_H_
#define WEBRTC_VIDEO_ENGINE_MAIN_TEST_AUTOTEST_INTERFACE_VIE_AUTOTEST_ANDROID_H_

class ViEAutoTestAndroid
{
public:
	static int RunAutotest(int testSelection,
						   int subTestSelection,
						   void* window1,
						   void* window2,
						   void* javaVM,
						   void* env,
						   void* context);
};

#endif  
