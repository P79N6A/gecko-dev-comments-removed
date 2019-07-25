









#ifndef WEBRTC_VIDEO_ENGINE_MAIN_TEST_WINDOWSTEST_CHANNELDLG_H_
#define WEBRTC_VIDEO_ENGINE_MAIN_TEST_WINDOWSTEST_CHANNELDLG_H_

#include "StdAfx.h"






#include "common_types.h"

#include "vie_base.h"
#include "vie_capture.h"
#include "vie_codec.h"
#include "vie_network.h"
#include "vie_render.h"
#include "vie_rtp_rtcp.h"
#include "vie_errors.h"
#include "vie_file.h"
#include "tbExternalTransport.h"

#include "resource.h"		


#ifndef NO_VOICE_ENGINE

#include "voe_base.h"
#include "voe_errors.h"
#include "voe_base.h"
#include "voe_network.h"
#include "voe_codec.h"
#include "voe_rtp_rtcp.h"
#endif

using namespace webrtc;
class CDXChannelDlg;
class CaptureDevicePool;
class ChannelPool;

#define TEST_MUSTPASS(expr,oklasterror)                                         \
    {                                                               \
        if ((expr))                                                 \
        {                                                           \
            CString r_msg;                                        \
            int r_lastError=_vieBase->LastError();    \
            CString exp;    \
            exp=#expr;\
            r_msg.Format(_T("\nError at line:%i, %s \nError code: %i\n"),__LINE__, exp,r_lastError);      \
            if(r_lastError!=oklasterror) \
            ::MessageBox (NULL, (LPCTSTR)r_msg, TEXT("Error Message"),  MB_OK | MB_ICONINFORMATION);                                   \
        }                                                           \
    }

class CDXChannelDlgObserver
{
public:
    virtual void ChannelDialogEnded(CDXChannelDlg* context)=0;

protected:
    virtual ~CDXChannelDlgObserver(){};

};

class CDXChannelDlg : public CDialog , public ViEEncoderObserver, public ViEDecoderObserver, public ViEBaseObserver, public ViECaptureObserver
{

public:
	CDXChannelDlg(VideoEngine* videoEngine,
        CaptureDevicePool& captureDevicePool,
        ChannelPool& channelPool,
        void* voiceEngine=NULL
    ,CWnd* pParent = NULL,CDXChannelDlgObserver* observer=NULL,int parentChannel=-1);	


	
	enum { IDD = IDD_DXQUALITY_DIALOG };
	CComboBox	m_ctrlDevice;
	CComboBox	m_ctrlCodec;	
	CComboBox	m_ctrlBitrate;
	CComboBox	m_ctrlCodecSize;
    CComboBox	m_ctrlRtcpMode;    
    CComboBox	m_ctrlPacketBurst;    
	CComboBox	m_ctrlMinFrameRate;	
    
    CListBox 	m_ctrlInfo;
	
	CStatic		m_ctrlLiveRemoteVideo;
	CStatic		m_ctrlLiveVideo;
	CEdit		m_localPort1;
	CEdit		m_remotePort1;	
	CIPAddressCtrl	m_remoteIp1;
    CButton     m_cbTmmbr;
    CButton     m_cbExternalTransport;
    CButton     m_cbFreezeLog;
    CButton     m_cbDefaultSendChannel;
    CComboBox   m_ctrlPacketLoss;
    CComboBox   m_ctrlDelay;
    
	
	

	
	
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	
	



public : 
    

    
    virtual void BrightnessAlarm(const int captureId,
                                 const Brightness brightness);

    virtual void CapturedFrameRate(const int captureId,
                                   const unsigned char frameRate);

    virtual void NoPictureAlarm(const int captureId,
                                const CaptureAlarm alarm);


    
    
    

    
    virtual void OutgoingRate(const int videoChannel,
                              const unsigned int framerate,
                              const unsigned int bitrate) ;

    
    virtual void IncomingCodecChanged(const int  videoChannel,
                                      const VideoCodec& videoCodec);

    virtual void IncomingRate(const int videoChannel,
                              const unsigned int framerate,
                              const unsigned int bitrate);
    
    virtual void RequestNewKeyFrame(const int videoChannel);

    
    virtual void PerformanceAlarm(const unsigned int cpuLoad);

    
    
    



protected:
	HICON m_hIcon;
    int _channelId;
    int _parentChannel;
    int _audioChannel;
	bool _canAddLog;

    
    CRITICAL_SECTION _critCallback;
    HANDLE _callbackThread;
    HANDLE _callbackEvent;
    char _logMsg[512];
    static  unsigned int WINAPI CallbackThread(LPVOID lpParameter);    
    void CallbackThreadProcess();



	
	virtual void ConfigureRender();

    virtual void SetCaptureDevice();
    virtual void SetLocalReceiver();
    virtual void SetSendDestination();
    virtual void SetSendCodec();

    
    void AddToInfo(const char* msg);

	

	
	
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnRButtonUp( UINT nFlags, CPoint point);	
	afx_msg BOOL OnDeviceChange( UINT, DWORD );
	afx_msg void OnPaint();
	
	afx_msg HCURSOR OnQueryDragIcon();
	virtual afx_msg void OnStartSend();	
	virtual afx_msg void OnDestroy();
	virtual afx_msg void OnStopSend();    
    virtual afx_msg void OnCancel();
	afx_msg void OnTimer(UINT nIDEvent);

	
	DECLARE_MESSAGE_MAP()

private:
	CDXChannelDlgObserver*  _dialogObserver;
	
	VideoEngine* _videoEngine;
    ViEBase*     _vieBase;
    ViECapture*  _vieCapture;
    ViERTP_RTCP* _vieRTPRTCP;
    ViERender*   _vieRender;
    ViECodec*    _vieCodec;
    ViENetwork*  _vieNetwork;
    ViEFile*      _vieFile;
    TbExternalTransport* _externalTransport;
    char             _fileName[256];


#ifndef NO_VOICE_ENGINE
    VoiceEngine*		_voiceEngine;
    VoEBase*             _veBase;
    VoENetwork*          _veNetwork;
    VoECodec*            _veCodec;
    VoERTP_RTCP*         _veRTCP;
#else
    void*                   _voiceEngine;

#endif

    VideoCodec     _sendCodec;
    int _captureId;
    CaptureDevicePool& _captureDevicePool;
    ChannelPool& _channelPool;


	afx_msg void OnCbnSelchangeCodecList();
	afx_msg void OnCbnSelchangeDevice();
	afx_msg void OnCbnSelchangeSize();
	afx_msg void OnCbnSelchangeBitrate();    
	afx_msg void OnCbnSelchangeWindowSize();	
	afx_msg void OnBnClickedversion();
	afx_msg void OnCbnSelchangeMinFrameRate();	
    afx_msg void OnBnClickedStartlisten();
    afx_msg void OnBnClickedStoplisten();
    afx_msg void OnBnClickedStopsend();
    afx_msg void OnBnClickedTmmbr();
    afx_msg void OnCbnSelchangeRtcpmode();
    afx_msg void OnBnClickedProtNack();
    afx_msg void OnBnClickedProtNone();
    afx_msg void OnBnClickedProtFec();
    afx_msg void OnBnClickedProtNackFec();  
    afx_msg void OnBnClickedFreezelog();
public:
    afx_msg void OnBnClickedExttransport();    
    afx_msg void OnCbnSelchangePacketloss();
    afx_msg void OnCbnSelchangeDelay();
    afx_msg void OnBnClickedBtnRecordIncoming();
    afx_msg void OnBnClickedBtnRecordOutgoing();
    afx_msg void OnBnClickedBtnCreateSlave();
    afx_msg void OnBnClickedVersion();
};




#endif  
