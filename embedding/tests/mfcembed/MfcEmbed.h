

































#ifndef _MFCEMBED_H
#define _MFCEMBED_H

#if _MSC_VER > 1000
#pragma once
#endif 

#ifndef __AFXWIN_H__
    #error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       






class CBrowserFrame;
class CProfileMgr;
class nsProfileDirServiceProvider;

class CMfcEmbedApp : public CWinApp,
                     public nsIObserver,
                     public nsIWindowCreator,
                     public nsSupportsWeakReference
{
public:
    CMfcEmbedApp();
    
    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER
    NS_DECL_NSIWINDOWCREATOR

    CBrowserFrame* CreateNewBrowserFrame(PRUint32 chromeMask = nsIWebBrowserChrome::CHROME_ALL, 
                            PRInt32 x = -1, PRInt32 y = -1, 
                            PRInt32 cx = -1, PRInt32 cy = -1, PRBool bShowWindow = PR_TRUE,
                            PRBool bIsEditor=PR_FALSE);
    void RemoveFrameFromList(CBrowserFrame* pFrm, BOOL bCloseAppOnLastFrame = TRUE);

    void ShowDebugConsole();
    nsresult OverrideComponents();

    
    
    
    public:
    virtual BOOL InitInstance();
    virtual int ExitInstance();
    virtual BOOL OnIdle(LONG lCount);
    

    CObList m_FrameWndLst;

    BOOL m_bChrome;
    CString m_strHomePage;
    inline BOOL GetHomePage(CString& strHomePage) {
        strHomePage = m_strHomePage;
        return TRUE;
    }

    int m_iStartupPage; 
    inline int GetStartupPageMode() {
        return m_iStartupPage;
    }



public:
    
    afx_msg void OnAppAbout();
    afx_msg void OnNewBrowser();
    afx_msg void OnNewEditor();
    afx_msg void OnManageProfiles();
    afx_msg void OnEditPreferences();
        
        
    
    DECLARE_MESSAGE_MAP()

private:
    BOOL            InitializeProfiles();
    BOOL            CreateHiddenWindow();
    nsresult        InitializePrefs();
    nsresult        InitializeWindowCreator();
    
private:

#ifdef USE_PROFILES
    CProfileMgr     *m_ProfileMgr;
#else
    nsProfileDirServiceProvider *m_ProfileDirServiceProvider;
#endif
};







#endif 
