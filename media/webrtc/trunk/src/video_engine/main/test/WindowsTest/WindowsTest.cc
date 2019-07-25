









#include "WindowsTest.h"
#include "ChannelDlg.h"
#include "WindowsTestMainDlg.h"
#include "engine_configurations.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#if (defined(_DEBUG) && defined(_WIN32))

#endif



BEGIN_MESSAGE_MAP(CDXWindowsTestApp, CWinApp)
	
		
		
	
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()




CDXWindowsTestApp::CDXWindowsTestApp()
{
    
}




CDXWindowsTestApp theApp;




BOOL CDXWindowsTestApp::InitInstance()
{
    int result=0;
    #ifndef NO_VOICE_ENGINE
        _voiceEngine = VoiceEngine::Create();
        _veBase = VoEBase::GetInterface(_voiceEngine);
         result+=_veBase->Init();
     #else
        _voiceEngine=NULL;
    #endif

    _videoEngine = VideoEngine::Create();

    _videoEngine->SetTraceFilter(webrtc::kTraceDefault);
    _videoEngine->SetTraceFile("trace.txt");
    
    ViEBase* vieBase=ViEBase::GetInterface(_videoEngine);
    result+=vieBase->Init();
    if(result!=0)
    {
        ::MessageBox (NULL, (LPCTSTR)("failed to init VideoEngine"), TEXT("Error Message"),  MB_OK | MB_ICONINFORMATION);                
    }
    
    {
      WindowsTestMainDlg dlg(_videoEngine,_voiceEngine);

      m_pMainWnd = &dlg;
      dlg.DoModal();
    }
    
    vieBase->Release();

    if(!VideoEngine::Delete(_videoEngine))
    {
        char errorMsg[255];
        sprintf(errorMsg,"All VideoEngine interfaces are not released properly!");
        ::MessageBox (NULL, (LPCTSTR)errorMsg, TEXT("Error Message"),  MB_OK | MB_ICONINFORMATION);
    }

  #ifndef NO_VOICE_ENGINE
    
    _veBase->Terminate();
    if(_veBase->Release()!=0)        
    {
        
        char errorMsg[256];
        sprintf(errorMsg,"All VoiceEngine interfaces are not released properly!");
        ::MessageBox (NULL, (LPCTSTR)errorMsg, TEXT("Error Message"),  MB_OK | MB_ICONINFORMATION);
    }

    if (false == VoiceEngine::Delete(_voiceEngine))
    {
        char errorMsg[256];
        sprintf(errorMsg,"VoiceEngine::Delete() failed!");
        ::MessageBox (NULL, (LPCTSTR)errorMsg, TEXT("Error Message"),  MB_OK | MB_ICONINFORMATION);
    }
   #endif

	
	
	return FALSE;
}
