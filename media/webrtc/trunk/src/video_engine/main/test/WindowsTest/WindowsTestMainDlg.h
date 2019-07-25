









#pragma once
#include "StdAfx.h"
#include "WindowsTestResource.h"

#include "ChannelDlg.h"
#include "CaptureDevicePool.h"
#include "ChannelPool.h"


namespace webrtc {
    class VideoEngine;
    class VoiceEngine;
}
using namespace webrtc;
class CDXCaptureDlg;


class WindowsTestMainDlg : public CDialog, private CDXChannelDlgObserver
{
	DECLARE_DYNAMIC(WindowsTestMainDlg)

public:
	WindowsTestMainDlg(VideoEngine* videoEngine,void* voiceEngine=NULL,CWnd* pParent = NULL);   
	virtual ~WindowsTestMainDlg();


	enum { IDD = IDD_WINDOWSTEST_MAIN };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    

	DECLARE_MESSAGE_MAP()
public:
     afx_msg void OnBnClickedChannel1();
     afx_msg void OnBnClickedChannel2();
     afx_msg void OnBnClickedChannel3();
     afx_msg void OnBnClickedChannel4();


     VideoEngine* _videoEngine;
    VoiceEngine*		_voiceEngine;
    VoEBase* _veBase;

    CDXChannelDlg* _testDlg1;
    CDXChannelDlg* _testDlg2;
    CDXChannelDlg* _testDlg3;
    CDXChannelDlg* _testDlg4;

    int _externalInWidth;   
    int _externalInHeight;
    int _externalInVideoType;

    CaptureDevicePool _captureDevicePool;
    ChannelPool       _channelPool;


private:
    virtual void ChannelDialogEnded(CDXChannelDlg* context);

public:

};
