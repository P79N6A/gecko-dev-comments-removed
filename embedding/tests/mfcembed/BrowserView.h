



































#ifndef _BROWSERVIEW_H
#define _BROWSERVIEW_H

#if _MSC_VER > 1000
    #pragma once
#endif

#include "IBrowserFrameGlue.h"
#include "nsIPrintSettings.h"




class CBrowserFrame;
class CBrowserImpl;
class CFindDialog;
class CPrintProgressDialog;

class CBrowserView : public CWnd
{
public:
    CBrowserView();
    virtual ~CBrowserView();

    
    HRESULT CreateBrowser();
    HRESULT DestroyBrowser();
    void OpenURL(const char* pUrl);
    void OpenURL(const PRUnichar* pUrl);
    CBrowserFrame* CreateNewBrowserFrame(PRUint32 chromeMask = nsIWebBrowserChrome::CHROME_ALL, 
                            PRInt32 x = -1, PRInt32 y = -1, 
                            PRInt32 cx = -1, PRInt32 cy = -1,
                            PRBool bShowWindow = PR_TRUE);
    void OpenURLInNewWindow(const PRUnichar* pUrl);
    void LoadHomePage();

    void GetBrowserWindowTitle(nsAString& title);
    
    
    
    void SetBrowserFrame(CBrowserFrame* pBrowserFrame);
    CBrowserFrame* mpBrowserFrame;

    
    
    
    void SetBrowserFrameGlue(PBROWSERFRAMEGLUE pBrowserFrameGlue);
    PBROWSERFRAMEGLUE mpBrowserFrameGlue;

    
    
    
    CBrowserImpl* mpBrowserImpl;

    
    
    nsCOMPtr<nsIWebBrowser> mWebBrowser;
    nsCOMPtr<nsIBaseWindow> mBaseWindow;
    nsCOMPtr<nsIWebNavigation> mWebNav;    

    void UpdateBusyState(PRBool aBusy);
    PRBool mbDocumentLoading;

    void SetCtxMenuLinkUrl(nsEmbedString& strLinkUrl);
    nsEmbedString mCtxMenuLinkUrl;

    void SetCtxMenuImageSrc(nsEmbedString& strImgSrc);
    nsEmbedString mCtxMenuImgSrc;

    void SetCurrentFrameURL(nsEmbedString& strCurrentFrameURL);
    nsEmbedString mCtxMenuCurrentFrameURL;

    inline void ClearFindDialog() { m_pFindDlg = NULL; }
    CFindDialog* m_pFindDlg;
    CPrintProgressDialog* m_pPrintProgressDlg;
    
    
    
    
    
    BOOL m_bUrlBarClipOp;

    
    BOOL m_bCurrentlyPrinting;

    void Activate(UINT nState, CWnd* pWndOther, BOOL bMinimized);

    BOOL OpenViewSourceWindow(const PRUnichar* pUrl);
    BOOL OpenViewSourceWindow(const char* pUrl);
    BOOL IsViewSourceUrl(CString& strUrl);

    enum _securityState {
        SECURITY_STATE_SECURE,
        SECURITY_STATE_INSECURE,
        SECURITY_STATE_BROKEN
    };
    int m_SecurityState;
    void ShowSecurityInfo();

    BOOL ViewContentContainsFrames();
    
    
    
    
    protected:
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
    


    
protected:
    nsCOMPtr<nsIPrintSettings> m_PrintSettings;
  BOOL                       m_InPrintPreview;

    
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnDestroy();
    afx_msg void OnSize( UINT, int, int );
    
    
    afx_msg void OnUrlSelectedInUrlBar();
    afx_msg void OnNewUrlEnteredInUrlBar();

    
    
    afx_msg void OnFileOpen();
    afx_msg void OnFileSaveAs();
    afx_msg void OnViewSource();
    afx_msg void OnViewInfo();
    afx_msg void OnNavBack();
    afx_msg void OnNavForward();
    afx_msg void OnNavHome();
    afx_msg void OnNavReload();
    afx_msg void OnNavStop();
    afx_msg void OnCut();
    afx_msg void OnCopy();
    afx_msg void OnPaste();
    afx_msg void OnUndoUrlBarEditOp();
    afx_msg void OnSelectAll();
    afx_msg void OnSelectNone();
    afx_msg void OnCopyLinkLocation();
    afx_msg void OnOpenLinkInNewWindow();
    afx_msg void OnViewImageInNewWindow();
    afx_msg void OnSaveLinkAs();
    afx_msg void OnSaveImageAs();
    afx_msg void OnShowFindDlg();
    afx_msg void OnFilePrint();
    afx_msg void OnFilePrintPreview();
    afx_msg void OnFilePrintSetup();
    afx_msg void OnUpdateFilePrint(CCmdUI* pCmdUI);
    afx_msg void OnUpdatePrintSetup(CCmdUI* pCmdUI);
    afx_msg LRESULT OnFindMsg(WPARAM wParam, LPARAM lParam);
    afx_msg void OnViewFrameSource();
    afx_msg void OnOpenFrameInNewWindow();

    
    
    afx_msg void OnUpdateNavBack(CCmdUI* pCmdUI);
    afx_msg void OnUpdateNavForward(CCmdUI* pCmdUI);
    afx_msg void OnUpdateNavStop(CCmdUI* pCmdUI);
    afx_msg void OnUpdateCut(CCmdUI* pCmdUI);
    afx_msg void OnUpdateCopy(CCmdUI* pCmdUI);
    afx_msg void OnUpdatePaste(CCmdUI* pCmdUI);
    afx_msg void OnUpdateViewImage(CCmdUI* pCmdUI);
    
    DECLARE_MESSAGE_MAP()
};

#endif
