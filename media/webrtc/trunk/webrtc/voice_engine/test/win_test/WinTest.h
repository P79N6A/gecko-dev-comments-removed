








#ifndef WEBRTC_VOICE_ENGINE_TEST_WIN_TEST_WINTEST_H_
#define WEBRTC_VOICE_ENGINE_TEST_WIN_TEST_WINTEST_H_

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		






class CWinTestApp : public CWinApp
{
public:
	CWinTestApp();


	public:
	virtual BOOL InitInstance();



	DECLARE_MESSAGE_MAP()
};

extern CWinTestApp theApp;

#endif
