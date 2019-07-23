































#include "stdafx.h"
#include <afxpriv.h>
#include "mfcembed.h"
#include "ProfilesDlg.h"


#include "nsIProfile.h"
#include "nsIServiceManager.h"
#include "nsServiceManagerUtils.h"
#include "nsMemory.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


static void ValidateProfileName(const CString& profileName, CDataExchange* pDX)
{
    USES_CONVERSION;

    nsresult rv;
    PRBool exists = FALSE;

    {
        nsCOMPtr<nsIProfile> profileService = 
                 do_GetService(NS_PROFILE_CONTRACTID, &rv);
        rv = profileService->ProfileExists(T2CW(profileName), &exists);
    }

    if (NS_SUCCEEDED(rv) && exists)
    {
        CString errMsg;
        errMsg.Format(_T("Error: A profile named \"%s\" already exists."), profileName);
        AfxMessageBox( errMsg, MB_ICONEXCLAMATION );
        errMsg.Empty();
        pDX->Fail();
    }

    if (profileName.FindOneOf(_T("\\/")) != -1)
    {
        AfxMessageBox( _T("Error: A profile name cannot contain the characters \"\\\" or \"/\"."), MB_ICONEXCLAMATION );
        pDX->Fail();
    }
}





CNewProfileDlg::CNewProfileDlg(CWnd* pParent )
    : CDialog(CNewProfileDlg::IDD, pParent)
{
    
    m_LocaleIndex = -1;
    m_Name = _T("");
    
}


void CNewProfileDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    
    DDX_CBIndex(pDX, IDC_LOCALE_COMBO, m_LocaleIndex);
    DDX_Text(pDX, IDC_NEW_PROF_NAME, m_Name);
    

    pDX->PrepareEditCtrl(IDC_NEW_PROF_NAME);
    if (pDX->m_bSaveAndValidate)
    {
        ValidateProfileName(m_Name, pDX);
    }
}


BEGIN_MESSAGE_MAP(CNewProfileDlg, CDialog)
    
        
    
END_MESSAGE_MAP()









CRenameProfileDlg::CRenameProfileDlg(CWnd* pParent )
    : CDialog(CRenameProfileDlg::IDD, pParent)
{
    
    m_NewName = _T("");
    
}


void CRenameProfileDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    
    DDX_Text(pDX, IDC_NEW_NAME, m_NewName);
    

    pDX->PrepareEditCtrl(IDC_NEW_NAME);
    if (pDX->m_bSaveAndValidate)
    {
        ValidateProfileName(m_NewName, pDX);
    }
}


BEGIN_MESSAGE_MAP(CRenameProfileDlg, CDialog)
    
        
    
END_MESSAGE_MAP()









CProfilesDlg::CProfilesDlg(CWnd* pParent )
    : CDialog(CProfilesDlg::IDD, pParent)
{
    
    m_bAtStartUp = FALSE;
    m_bAskAtStartUp = FALSE;
    
}


void CProfilesDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    
    DDX_Control(pDX, IDC_LIST1, m_ProfileList);
    DDX_Check(pDX, IDC_CHECK_ASK_AT_START, m_bAskAtStartUp);
    

    if (pDX->m_bSaveAndValidate)
    {
        USES_CONVERSION;

        int itemIndex = m_ProfileList.GetCurSel();
        if (itemIndex != LB_ERR)
        {
            CString itemText;
            m_ProfileList.GetText(itemIndex, itemText);
            m_SelectedProfile.Assign(T2CW(itemText));
        }
    }
}


BEGIN_MESSAGE_MAP(CProfilesDlg, CDialog)
    
    ON_BN_CLICKED(IDC_PROF_NEW, OnNewProfile)
    ON_BN_CLICKED(IDC_PROF_RENAME, OnRenameProfile)
    ON_BN_CLICKED(IDC_PROF_DELETE, OnDeleteProfile)
    ON_LBN_DBLCLK(IDC_LIST1, OnDblclkProfile)
    
END_MESSAGE_MAP()




BOOL CProfilesDlg::OnInitDialog() 
{
    USES_CONVERSION;

    CDialog::OnInitDialog();
    
    PRUnichar *curProfileName = nsnull;

    
    nsresult rv;
    nsCOMPtr<nsIProfile> profileService = 
             do_GetService(NS_PROFILE_CONTRACTID, &rv);
    profileService->GetCurrentProfile(&curProfileName);

    PRInt32     selectedRow = 0;
    PRUint32    listLen;
    PRUnichar   **profileList;
    rv = profileService->GetProfileList(&listLen, &profileList);

    for (PRUint32 index = 0; index < listLen; index++)
    {
        CString tmpStr(W2T(profileList[index]));
        m_ProfileList.AddString(tmpStr);
        if (wcscmp(profileList[index], curProfileName) == 0)
            selectedRow = index;
    }
    nsMemory::Free(curProfileName);

    m_ProfileList.SetCurSel(selectedRow);

    if (m_bAtStartUp)
    {
        GetDlgItem(IDCANCEL)->EnableWindow(FALSE);
    }
    
    return TRUE;  
                  
}

void CProfilesDlg::OnNewProfile() 
{
    CNewProfileDlg dialog;

    if (dialog.DoModal() == IDOK)
    {
        nsresult rv;

        nsCOMPtr<nsIProfile> profileService = 
                 do_GetService(NS_PROFILE_CONTRACTID, &rv);
        ASSERT(NS_SUCCEEDED(rv));
        if (NS_SUCCEEDED(rv))
        {
            USES_CONVERSION;

            rv = profileService->CreateNewProfile(T2CW(dialog.m_Name), nsnull, nsnull, PR_FALSE);
            ASSERT(NS_SUCCEEDED(rv));
            if (NS_SUCCEEDED(rv))
            {
                int item = m_ProfileList.AddString(dialog.m_Name);
                m_ProfileList.SetCurSel(item);
                GetDlgItem(IDOK)->EnableWindow(TRUE);
            }
        }
    }
}

void CProfilesDlg::OnRenameProfile() 
{
    CRenameProfileDlg dialog;

    int itemIndex = m_ProfileList.GetCurSel();
    ASSERT(itemIndex != LB_ERR);
    if (itemIndex == LB_ERR)
        return;

    m_ProfileList.GetText(itemIndex, dialog.m_CurrentName);

    if (dialog.DoModal() == IDOK)
    {
        USES_CONVERSION;

        nsresult rv;

        nsCOMPtr<nsIProfile> profileService = 
                 do_GetService(NS_PROFILE_CONTRACTID, &rv);
        ASSERT(NS_SUCCEEDED(rv));
        if (NS_SUCCEEDED(rv))
        {
            rv = profileService->RenameProfile(T2CW(dialog.m_CurrentName), T2CW(dialog.m_NewName));
            ASSERT(NS_SUCCEEDED(rv));
        }
    }    
}

void CProfilesDlg::OnDeleteProfile() 
{
    int itemIndex = m_ProfileList.GetCurSel();
    ASSERT(itemIndex != LB_ERR);
    if (itemIndex == LB_ERR)
        return;

    CString selectedProfile;
    m_ProfileList.GetText(itemIndex, selectedProfile);
    
    nsresult rv;
    nsCOMPtr<nsIProfile> profileService = 
             do_GetService(NS_PROFILE_CONTRACTID, &rv);
    ASSERT(NS_SUCCEEDED(rv));
    if (NS_SUCCEEDED(rv))
    {
        USES_CONVERSION;

        rv = profileService->DeleteProfile(T2CW(selectedProfile), PR_TRUE);
        ASSERT(NS_SUCCEEDED(rv));
        if (NS_SUCCEEDED(rv))
        {
            int itemCount = m_ProfileList.DeleteString(itemIndex);
            if (itemCount == 0)
                GetDlgItem(IDOK)->EnableWindow(FALSE);
        }
    }    
}

void CProfilesDlg::OnDblclkProfile() 
{
    OnOK();
}
