









































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
		COMBOBOXEXITEM ci;
		ci.mask = CBEIF_TEXT; ci.iItem = -1;
		ci.pszText = (LPTSTR)(LPCTSTR)url;
		InsertItem(&ci);

        if(bAddToMRUList)
            m_MRUList.AddURL((LPTSTR)(LPCTSTR)url);
	}
    inline void LoadMRUList() {
        for (int i=0;i<m_MRUList.GetNumURLs();i++) 
        {
            CString urlStr(_T(m_MRUList.GetURL(i)));
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

class CBrowserFrame : public CFrameWnd
{	
public:
	CBrowserFrame(PRUint32 chromeMask);

protected: 
	DECLARE_DYNAMIC(CBrowserFrame)

public:
	inline CBrowserImpl *GetBrowserImpl() { return m_wndBrowserView.mpBrowserImpl; }

	CToolBar    m_wndToolBar;
	CStatusBar  m_wndStatusBar;
	CProgressCtrl m_wndProgressBar;
	CUrlBar m_wndUrlBar;
	CReBar m_wndReBar;
	
	
	CBrowserToolTip m_wndTooltip;
	CBrowserView    m_wndBrowserView;

    
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
};






#endif
