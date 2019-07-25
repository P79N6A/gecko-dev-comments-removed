









#ifndef WEBRTC_VIDEO_ENGINE_MAIN_TEST_WINDOWSTEST_WINDOWSTEST_H_
#define WEBRTC_VIDEO_ENGINE_MAIN_TEST_WINDOWSTEST_WINDOWSTEST_H_


#include "StdAfx.h"
#include "resource.h"		






namespace webrtc {
    class VoiceEngine;
    class VoEBase;
    class VideoEngine;
}
using namespace webrtc;

class CDXWindowsTestApp : public CWinApp
{
public:
	CDXWindowsTestApp();


	
	
	public:
	virtual BOOL InitInstance();
	



	
		
		
	
	DECLARE_MESSAGE_MAP()

	VideoEngine*  _videoEngine;
    VoiceEngine*  _voiceEngine;
    VoEBase*       _veBase;
};







#endif 
