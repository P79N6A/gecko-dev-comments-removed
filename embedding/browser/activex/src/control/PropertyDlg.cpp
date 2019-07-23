




































#include "stdafx.h"

#include "PropertyDlg.h"
#include "resource.h"

#include "nsIMIMEInfo.h"
#include "nsIMIMEService.h"

CPropertyDlg::CPropertyDlg() :
    mPPage(NULL)
{
}

HRESULT CPropertyDlg::AddPage(CPPageDlg *pPage)
{
    mPPage = pPage;
    return S_OK;
}


LRESULT CPropertyDlg::OnInitDialog(UINT uMsg, WPARAM wParam,  LPARAM lParam, BOOL& bHandled)
{
    if (mPPage)
    {
        
        RECT rc;
        ::GetWindowRect(GetDlgItem(IDC_PPAGE_MARKER), &rc);
        ScreenToClient(&rc);
        mPPage->Create(m_hWnd, rc);
        mPPage->SetWindowPos(HWND_TOP, &rc, SWP_SHOWWINDOW);
    }
    return 1;
}


LRESULT CPropertyDlg::OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    if (mPPage)
    {
        mPPage->DestroyWindow();
    }
    EndDialog(IDOK);
    return 1;
}


LRESULT CPropertyDlg::OnClose(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    if (mPPage)
    {
        mPPage->DestroyWindow();
    }
    EndDialog(IDCLOSE);
    return 1;
}





LRESULT CPPageDlg::OnInitDialog(UINT uMsg, WPARAM wParam,  LPARAM lParam, BOOL& bHandled)
{
    nsAutoString desc;
    if (!mType.IsEmpty())
    {
        nsresult rv;
        nsCOMPtr<nsIMIMEService> mimeService;
        mimeService = do_GetService("@mozilla.org/mime;1", &rv);
        NS_ENSURE_TRUE(mimeService, NS_ERROR_FAILURE);

        nsCOMPtr<nsIMIMEInfo> mimeInfo;
        nsCAutoString contentType;
        
        LossyCopyUTF16toASCII(mType, contentType);
        mimeService->GetFromTypeAndExtension(contentType, EmptyCString(), getter_AddRefs(mimeInfo));
        if (mimeInfo)
        {
            mimeInfo->GetDescription(desc);
        }
    }

    USES_CONVERSION;
    SetDlgItemText(IDC_PROTOCOL, W2T(desc.get()));
    SetDlgItemText(IDC_TYPE, W2T(mType.get()));
    SetDlgItemText(IDC_ADDRESS, W2T(mURL.get()));

    return 1;
}
