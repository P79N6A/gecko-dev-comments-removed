





















































#include "stdafx.h"
#include "MfcEmbed.h"
#include "BrowserFrm.h"
#include "Dialogs.h"




void CBrowserFrame::BrowserFrameGlueObj::UpdateStatusBarText(const PRUnichar *aMessage)
{
    METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)
    USES_CONVERSION;
    pThis->m_wndStatusBar.SetPaneText(0, aMessage ? W2CT(aMessage) : _T(""));
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
        USES_CONVERSION;
        nsEmbedCString uriString;
        aLocation->GetSpec(uriString);
        pThis->m_wndUrlBar.SetCurrentURL(A2CT(uriString.get()));
    }
}

void CBrowserFrame::BrowserFrameGlueObj::GetBrowserFrameTitle(PRUnichar **aTitle)
{
    METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)

    CString title;
    pThis->GetWindowText(title);

    if(!title.IsEmpty())
    {
        USES_CONVERSION;
        nsEmbedString nsTitle;
        nsTitle.Assign(T2CW(title.GetBuffer(0)));
        *aTitle = NS_StringCloneData(nsTitle);
    }
}

void CBrowserFrame::BrowserFrameGlueObj::SetBrowserFrameTitle(const PRUnichar *aTitle)
{
    METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)

    CString title;
    if (aTitle)
    {
        USES_CONVERSION;
        title = W2CT(aTitle);
    }
    else
    {
        
        
        
        title.LoadString(AFX_IDS_APP_TITLE);
    }
    pThis->SetWindowText(title);
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

    CMfcEmbedApp *pApp = (CMfcEmbedApp *)AfxGetApp();
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

void CBrowserFrame::BrowserFrameGlueObj::ShowContextMenu(PRUint32 aContextFlags, nsIContextMenuInfo *aInfo)
{
    METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)

    BOOL bContentHasFrames = FALSE;
    UINT nIDResource = IDR_CTXMENU_DOCUMENT;

    
    
    nsEmbedString empty;
    pThis->m_wndBrowserView.SetCtxMenuImageSrc(empty);  
    pThis->m_wndBrowserView.SetCtxMenuLinkUrl(empty);
    pThis->m_wndBrowserView.SetCurrentFrameURL(empty);

    
    
    
    if(pThis->GetEditable())
    {
        nIDResource = IDR_CTXMENU_EDITOR;

        GOTO_BUILD_CTX_MENU;
    }

    if(aContextFlags & nsIContextMenuListener2::CONTEXT_DOCUMENT)
    {
        nIDResource = IDR_CTXMENU_DOCUMENT;

        
        if (aContextFlags & nsIContextMenuListener2::CONTEXT_BACKGROUND_IMAGE)
        {
            
            nsCOMPtr<nsIURI> imgURI;
            aInfo->GetBackgroundImageSrc(getter_AddRefs(imgURI));
            if (!imgURI)
                return; 
            nsEmbedCString uri;
            imgURI->GetSpec(uri);

            nsEmbedString uri2;
            NS_CStringToUTF16(uri, NS_CSTRING_ENCODING_UTF8, uri2);

            pThis->m_wndBrowserView.SetCtxMenuImageSrc(uri2); 
        }
    }
    else if(aContextFlags & nsIContextMenuListener2::CONTEXT_TEXT)        
        nIDResource = IDR_CTXMENU_TEXT;
    else if(aContextFlags & nsIContextMenuListener2::CONTEXT_LINK)
    {
        nIDResource = IDR_CTXMENU_LINK;

        
        
        
        
        
        
        
        nsEmbedString strUrlUcs2;
        nsresult rv = aInfo->GetAssociatedLink(strUrlUcs2);
        if(NS_FAILED(rv))
            return;

        
        
        pThis->m_wndBrowserView.SetCtxMenuLinkUrl(strUrlUcs2);

        
        nsCOMPtr<nsIURI> imgURI;
        aInfo->GetImageSrc(getter_AddRefs(imgURI));
        if(imgURI)
        {
            nsEmbedCString strImgSrcUtf8;
            imgURI->GetSpec(strImgSrcUtf8);
            if(strImgSrcUtf8.Length() != 0)
            {
                
                nsEmbedString strImgSrc;
                NS_CStringToUTF16(strImgSrcUtf8, NS_CSTRING_ENCODING_UTF8, strImgSrc);
                pThis->m_wndBrowserView.SetCtxMenuImageSrc(strImgSrc);
            }
        }
    }
    else if(aContextFlags & nsIContextMenuListener2::CONTEXT_IMAGE)
    {
        nIDResource = IDR_CTXMENU_IMAGE;

        
        nsCOMPtr<nsIURI> imgURI;
        aInfo->GetImageSrc(getter_AddRefs(imgURI));
        if(!imgURI)
            return;
        nsEmbedCString strImgSrcUtf8;
        imgURI->GetSpec(strImgSrcUtf8);
        if(strImgSrcUtf8.Length() == 0)
            return;

        
        nsEmbedString strImgSrc;
        NS_CStringToUTF16(strImgSrcUtf8, NS_CSTRING_ENCODING_UTF8, strImgSrc);
        pThis->m_wndBrowserView.SetCtxMenuImageSrc(strImgSrc);
    }

    
    
    
    if(pThis->m_wndBrowserView.ViewContentContainsFrames())
    {
        bContentHasFrames = TRUE;

        
        
        nsresult rv = NS_OK;
        nsCOMPtr<nsIDOMNode> node;
        aInfo->GetTargetNode(getter_AddRefs(node));
        if(!node)
            GOTO_BUILD_CTX_MENU;

        nsCOMPtr<nsIDOMDocument> domDoc;
        rv = node->GetOwnerDocument(getter_AddRefs(domDoc));
        if(NS_FAILED(rv))
            GOTO_BUILD_CTX_MENU;

        nsCOMPtr<nsIDOMHTMLDocument> htmlDoc(do_QueryInterface(domDoc, &rv));
        if(NS_FAILED(rv))
            GOTO_BUILD_CTX_MENU;

        nsEmbedString strFrameURL;
        rv = htmlDoc->GetURL(strFrameURL);
        if(NS_FAILED(rv))
            GOTO_BUILD_CTX_MENU;

        pThis->m_wndBrowserView.SetCurrentFrameURL(strFrameURL); 
    }

BUILD_CTX_MENU:

    CMenu ctxMenu;
    if(ctxMenu.LoadMenu(nIDResource))
    {
        
        if(bContentHasFrames) 
        {
            CMenu* pCtxMenu = ctxMenu.GetSubMenu(0);
            if(pCtxMenu)
            {
                pCtxMenu->AppendMenu(MF_SEPARATOR);

                CString strMenuItem;
                strMenuItem.LoadString(IDS_VIEW_FRAME_SOURCE);
                pCtxMenu->AppendMenu(MF_STRING, ID_VIEW_FRAME_SOURCE, strMenuItem);

                strMenuItem.LoadString(IDS_OPEN_FRAME_IN_NEW_WINDOW);
                pCtxMenu->AppendMenu(MF_STRING, ID_OPEN_FRAME_IN_NEW_WINDOW, strMenuItem);
            }
        }

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

void CBrowserFrame::BrowserFrameGlueObj::UpdateSecurityStatus(PRInt32 aState)
{
    METHOD_PROLOGUE(CBrowserFrame, BrowserFrameGlueObj)

    pThis->UpdateSecurityStatus(aState);
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
