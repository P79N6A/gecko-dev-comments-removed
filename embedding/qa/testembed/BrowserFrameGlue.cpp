




























































#include "stdafx.h"
#include "TestEmbed.h"
#include "BrowserFrm.h"
#include "Dialogs.h"
#include "nsReadableUtils.h"




void CBrowserFrame::BrowserFrameGlueObj::UpdateStatusBarText(const PRUnichar *aMessage)
{
	METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)

	nsCString strStatus; 

    if(aMessage)
        strStatus.AssignWithConversion(aMessage);

    pThis->m_wndStatusBar.SetPaneText(0, strStatus.get());
}

void CBrowserFrame::BrowserFrameGlueObj::UpdateProgress(PRInt32 aCurrent, PRInt32 aMax)
{
	METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)

    pThis->m_wndProgressBar.SetRange32(0, aMax);
    pThis->m_wndProgressBar.SetPos(aCurrent);
}

void CBrowserFrame::BrowserFrameGlueObj::UpdateBusyState(PRBool aBusy)
{	
	METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)

	
	
	

	pThis->m_wndBrowserView.UpdateBusyState(aBusy);
}






void CBrowserFrame::BrowserFrameGlueObj::UpdateCurrentURI(nsIURI *aLocation)
{
	METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)

	if(aLocation)
	{
		nsCAutoString uriString; 

		aLocation->GetSpec(uriString);

		pThis->m_wndUrlBar.SetCurrentURL(uriString.get());
	}
}

void CBrowserFrame::BrowserFrameGlueObj::GetBrowserFrameTitle(PRUnichar **aTitle)
{
	METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)

	CString title;
	pThis->GetWindowText(title);

	if(!title.IsEmpty())
	{
		nsString nsTitle;
		nsTitle.AssignWithConversion(title.GetBuffer(0));

		*aTitle = ToNewUnicode(nsTitle);
	}
}

void CBrowserFrame::BrowserFrameGlueObj::SetBrowserFrameTitle(const PRUnichar *aTitle)
{
	METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)

	USES_CONVERSION;

	if(W2T(aTitle))
	{
		pThis->SetWindowText(W2T(aTitle));
	}
	else
	{
		
		
		
		CString cs;
		cs.LoadString(AFX_IDS_APP_TITLE);
		pThis->SetWindowText(cs);
	}
}

void CBrowserFrame::BrowserFrameGlueObj::SetBrowserFrameSize(PRInt32 aCX, PRInt32 aCY)
{
	METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)
	
	pThis->SetWindowPos(NULL, 0, 0, aCX, aCY, 
				SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER
			);
}

void CBrowserFrame::BrowserFrameGlueObj::GetBrowserFrameSize(PRInt32 *aCX, PRInt32 *aCY)
{
	METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)

	RECT wndRect;
	pThis->GetWindowRect(&wndRect);

	if (aCX)
		*aCX = wndRect.right - wndRect.left;

	if (aCY)
		*aCY = wndRect.bottom - wndRect.top;
}

void CBrowserFrame::BrowserFrameGlueObj::SetBrowserFramePosition(PRInt32 aX, PRInt32 aY)
{
	METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)	

	pThis->SetWindowPos(NULL, aX, aY, 0, 0, 
				SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);
}

void CBrowserFrame::BrowserFrameGlueObj::GetBrowserFramePosition(PRInt32 *aX, PRInt32 *aY)
{
	METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)

	RECT wndRect;
	pThis->GetWindowRect(&wndRect);

	if (aX)
		*aX = wndRect.left;

	if (aY)
		*aY = wndRect.top;
}

void CBrowserFrame::BrowserFrameGlueObj::GetBrowserFramePositionAndSize(PRInt32 *aX, PRInt32 *aY, PRInt32 *aCX, PRInt32 *aCY)
{
	METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)

	RECT wndRect;
	pThis->GetWindowRect(&wndRect);

	if (aX)
		*aX = wndRect.left;

	if (aY)
		*aY = wndRect.top;

	if (aCX)
		*aCX = wndRect.right - wndRect.left;

	if (aCY)
		*aCY = wndRect.bottom - wndRect.top;
}

void CBrowserFrame::BrowserFrameGlueObj::SetBrowserFramePositionAndSize(PRInt32 aX, PRInt32 aY, PRInt32 aCX, PRInt32 aCY, PRBool fRepaint)
{
	METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)

	pThis->SetWindowPos(NULL, aX, aY, aCX, aCY, 
				SWP_NOACTIVATE | SWP_NOZORDER);
}

void CBrowserFrame::BrowserFrameGlueObj::SetFocus()
{
	METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)

	pThis->SetFocus();
}

void CBrowserFrame::BrowserFrameGlueObj::FocusAvailable(PRBool *aFocusAvail)
{
	METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)

	HWND focusWnd = GetFocus()->m_hWnd;

	if ((focusWnd == pThis->m_hWnd) || ::IsChild(pThis->m_hWnd, focusWnd))
		*aFocusAvail = PR_TRUE;
	else
		*aFocusAvail = PR_FALSE;
}

void CBrowserFrame::BrowserFrameGlueObj::ShowBrowserFrame(PRBool aShow)
{
	METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)

	if(aShow)
	{
		pThis->ShowWindow(SW_SHOW);
		pThis->SetActiveWindow();
		pThis->UpdateWindow();
	}
	else
	{
		pThis->ShowWindow(SW_HIDE);
	}
}

void CBrowserFrame::BrowserFrameGlueObj::GetBrowserFrameVisibility(PRBool *aVisible)
{
	METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)

	
	if (GetActiveWindow()->m_hWnd != pThis->m_hWnd)
	{
		*aVisible = PR_FALSE;
		return;
	}

	
	
	WINDOWPLACEMENT wpl;
	pThis->GetWindowPlacement(&wpl);

	if ((wpl.showCmd == SW_RESTORE) || (wpl.showCmd == SW_MAXIMIZE))
		*aVisible = PR_TRUE;
	else
		*aVisible = PR_FALSE;
}

PRBool CBrowserFrame::BrowserFrameGlueObj::CreateNewBrowserFrame(PRUint32 chromeMask, 
							PRInt32 x, PRInt32 y, 
							PRInt32 cx, PRInt32 cy,
							nsIWebBrowser** aWebBrowser)
{
   NS_ENSURE_ARG_POINTER(aWebBrowser);

   *aWebBrowser = nsnull;

	CTestEmbedApp *pApp = (CTestEmbedApp *)AfxGetApp();
	if(!pApp)
		return PR_FALSE;

	
	
	
	
	
	
	
	

	CBrowserFrame* pFrm = pApp->CreateNewBrowserFrame(chromeMask, x, y, cx, cy, PR_FALSE);
    if(!pFrm)
		return PR_FALSE;

	
	
	
	
	
	

	NS_IF_ADDREF(*aWebBrowser = pFrm->m_wndBrowserView.mWebBrowser);

	return PR_TRUE;
}

void CBrowserFrame::BrowserFrameGlueObj::DestroyBrowserFrame()
{
	METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)

	pThis->PostMessage(WM_CLOSE);
}

#define GOTO_BUILD_CTX_MENU { bContentHasFrames = FALSE; goto BUILD_CTX_MENU; }

void CBrowserFrame::BrowserFrameGlueObj::ShowContextMenu(PRUint32 aContextFlags, nsIDOMNode *aNode)
{
	METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)

	UINT nIDResource = IDR_CTXMENU_DOCUMENT;

	if(aContextFlags & nsIContextMenuListener::CONTEXT_DOCUMENT)
		nIDResource = IDR_CTXMENU_DOCUMENT;
	else if(aContextFlags & nsIContextMenuListener::CONTEXT_TEXT)		
		nIDResource = IDR_CTXMENU_TEXT;
	else if(aContextFlags & nsIContextMenuListener::CONTEXT_LINK)
	{
		nIDResource = IDR_CTXMENU_LINK;

		
		
		
		
		
		
		
		
		
		
		nsAutoString strUrlUcs2;
		pThis->m_wndBrowserView.SetCtxMenuLinkUrl(strUrlUcs2);

		
		
		
		nsresult rv = NS_OK;
		nsCOMPtr<nsIDOMHTMLAnchorElement> linkElement(do_QueryInterface(aNode, &rv));
		if(NS_FAILED(rv))
			return;

		rv = linkElement->GetHref(strUrlUcs2);
		if(NS_FAILED(rv))
			return;

		
		
		pThis->m_wndBrowserView.SetCtxMenuLinkUrl(strUrlUcs2);
	}
	else if(aContextFlags & nsIContextMenuListener::CONTEXT_IMAGE)
	{
		nIDResource = IDR_CTXMENU_IMAGE;

		nsAutoString strImgSrcUcs2;
		pThis->m_wndBrowserView.SetCtxMenuImageSrc(strImgSrcUcs2); 

		
		nsresult rv = NS_OK;
		nsCOMPtr<nsIDOMHTMLImageElement> imgElement(do_QueryInterface(aNode, &rv));
		if(NS_FAILED(rv))
			return;

		rv = imgElement->GetSrc(strImgSrcUcs2);
		if(NS_FAILED(rv))
			return;

		pThis->m_wndBrowserView.SetCtxMenuImageSrc(strImgSrcUcs2); 
	}

	CMenu ctxMenu;
	if(ctxMenu.LoadMenu(nIDResource))
	{
		POINT cursorPos;
		GetCursorPos(&cursorPos);

		(ctxMenu.GetSubMenu(0))->TrackPopupMenu(TPM_LEFTALIGN, cursorPos.x, cursorPos.y, pThis);
	}
}

HWND CBrowserFrame::BrowserFrameGlueObj::GetBrowserFrameNativeWnd()
{
	METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)
	return pThis->m_hWnd;
}

void CBrowserFrame::BrowserFrameGlueObj::ShowTooltip(PRInt32 aXCoords, PRInt32 aYCoords, const PRUnichar *aTipText)
{
    METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)
    pThis->m_wndTooltip.SetTipText(CString(aTipText));
    pThis->m_wndTooltip.Show(&pThis->m_wndBrowserView, aXCoords, aYCoords);
}

void CBrowserFrame::BrowserFrameGlueObj::HideTooltip()
{
    METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)
    pThis->m_wndTooltip.Hide();
}

