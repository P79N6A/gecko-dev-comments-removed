


































#ifndef _IBROWSERFRM_H
#define _IBROWSERFRM_H

#if _MSC_VER > 1000
#pragma once
#endif 

#include "BrowserView.h"
#include "BrowserToolTip.h"
#include "IBrowserFrameGlue.h"
#include "MostRecentUrls.h"


class CUrlBar : public CComboBoxEx
{
public:
    inline void GetEnteredURL(CString& url) {
        GetEditCtrl()->GetWindowText(url);
    }
    inline void GetSelectedURL(CString& url) {
        GetLBText(GetCurSel(), url);
    }
    inline void SetCurrentURL(LPCTSTR pUrl) {
        SetWindowText(pUrl);
    }
    inline void AddURLToList(CString& url, bool bAddToMRUList = true) {
        USES_CONVERSION;
        COMBOBOXEXITEM ci;
        ci.mask = CBEIF_TEXT; ci.iItem = -1;
        ci.pszText = const_cast<TCHAR *>((LPCTSTR)url);
        InsertItem(&ci);

        if(bAddToMRUList)
            m_MRUList.AddURL(T2CA(url));
    }
    inline void LoadMRUList() {
        for (int i=0;i<m_MRUList.GetNumURLs();i++) 
        {
            USES_CONVERSION;
            CString urlStr(A2CT(m_MRUList.GetURL(i)));
            AddURLToList(urlStr, false); 
        }
    }
    inline BOOL EditCtrlHasFocus() {
        return (GetEditCtrl()->m_hWnd == CWnd::GetFocus()->m_hWnd);
    }
    inline BOOL EditCtrlHasSelection() {
        int nStartChar = 0, nEndChar = 0;
        if(EditCtrlHasFocus())
            GetEditCtrl()->GetSel(nStartChar, nEndChar);
        return (nEndChar > nStartChar) ? TRUE : FALSE;
    }
    inline BOOL CanCutToClipboard() {
        return EditCtrlHasSelection();
    }
    inline void CutToClipboard() {
        GetEditCtrl()->Cut();
    }
    inline BOOL CanCopyToClipboard() {
        return EditCtrlHasSelection();
    }
    inline void CopyToClipboard() {
        GetEditCtrl()->Copy();
    }
    inline BOOL CanPasteFromClipboard() {
        return EditCtrlHasFocus();
    }
    inline void PasteFromClipboard() {
        GetEditCtrl()->Paste();
    }
    inline BOOL CanUndoEditOp() {
        return EditCtrlHasFocus() ? GetEditCtrl()->CanUndo() : FALSE;
    }
    inline void UndoEditOp() {        
        if(EditCtrlHasFocus())
            GetEditCtrl()->Undo();
    }

protected:
      CMostRecentUrls m_MRUList;
};


class CMyStatusBar : public CStatusBar
{
public:
    CMyStatusBar();
    virtual ~CMyStatusBar();

protected:
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);

    DECLARE_MESSAGE_MAP()
};

class CBrowserFrame : public CFrameWnd
{    
public:
    CBrowserFrame();
    CBrowserFrame(PRUint32 chromeMask);

protected: 
    DECLARE_DYNAMIC(CBrowserFrame)

public:
    inline CBrowserImpl *GetBrowserImpl() { return m_wndBrowserView.mpBrowserImpl; }

    CToolBar    m_wndToolBar;
    CMyStatusBar  m_wndStatusBar;
    CProgressCtrl m_wndProgressBar;
    CUrlBar m_wndUrlBar;
    CReBar m_wndReBar;
    CBrowserToolTip m_wndTooltip;

    
    
    CBrowserView    m_wndBrowserView;

    void UpdateSecurityStatus(PRInt32 aState);
    void ShowSecurityInfo();

    
    inline BOOL CanCutUrlBarSelection() { return m_wndUrlBar.CanCutToClipboard(); }
    inline void CutUrlBarSelToClipboard() { m_wndUrlBar.CutToClipboard(); }
    inline BOOL CanCopyUrlBarSelection() { return m_wndUrlBar.CanCopyToClipboard(); }
    inline void CopyUrlBarSelToClipboard() { m_wndUrlBar.CopyToClipboard(); }
    inline BOOL CanPasteToUrlBar() { return m_wndUrlBar.CanPasteFromClipboard(); }
    inline void PasteFromClipboardToUrlBar() { m_wndUrlBar.PasteFromClipboard(); }
    inline BOOL CanUndoUrlBarEditOp() { return m_wndUrlBar.CanUndoEditOp(); }
    inline void UndoUrlBarEditOp() { m_wndUrlBar.UndoEditOp(); }

    
    
    PRUint32 m_chromeMask;

protected:
    
    
    
    
    
    class BrowserFrameGlueObj : public IBrowserFrameGlue 
    {
        
        
        
        
        

        NS_DECL_BROWSERFRAMEGLUE

    } m_xBrowserFrameGlueObj;
    friend class BrowserFrameGlueObj;

public:
    void SetupFrameChrome();
    void SetEditable(BOOL isEditor) { mIsEditor = isEditor; }
    BOOL GetEditable() { return mIsEditor; }


    
    
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
    virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
    


public:
    virtual ~CBrowserFrame();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif


protected:
    
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSetFocus(CWnd *pOldWnd);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnClose();
    afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
    
    DECLARE_MESSAGE_MAP()

private:
    BOOL mIsEditor;
};






#endif
