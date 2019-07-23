






























#include "stdafx.h"
#include "Dialogs.h"
#include "BrowserView.h"










CFindDialog::CFindDialog(CString& csSearchStr, PRBool bMatchCase,
                PRBool bMatchWholeWord, PRBool bWrapAround,
                PRBool bSearchBackwards, CBrowserView* pOwner)
                : CFindReplaceDialog()
{
    
    
    
    m_csSearchStr = csSearchStr;
    m_bMatchCase = bMatchCase;
    m_bMatchWholeWord = bMatchWholeWord;
    m_bWrapAround = bWrapAround;
    m_bSearchBackwards = bSearchBackwards;
    m_pOwner = pOwner;

    
    
    m_fr.Flags |= FR_ENABLETEMPLATE;
    m_fr.hInstance = AfxGetInstanceHandle();
    m_fr.lpTemplateName = MAKEINTRESOURCE(IDD_FINDDLG);
}

BOOL CFindDialog::OnInitDialog() 
{
    CFindReplaceDialog::OnInitDialog();

    CEdit* pEdit = (CEdit *)GetDlgItem(IDC_FIND_EDIT);
    if(pEdit)
        pEdit->SetWindowText(m_csSearchStr);

    CButton* pChk = (CButton *)GetDlgItem(IDC_MATCH_CASE);
    if(pChk)
        pChk->SetCheck(m_bMatchCase);

    pChk = (CButton *)GetDlgItem(IDC_MATCH_WHOLE_WORD);
    if(pChk)
        pChk->SetCheck(m_bMatchWholeWord);

    pChk = (CButton *)GetDlgItem(IDC_WRAP_AROUND);    
    if(pChk)
        pChk->SetCheck(m_bWrapAround);

    pChk = (CButton *)GetDlgItem(IDC_SEARCH_BACKWARDS);
    if(pChk)
        pChk->SetCheck(m_bSearchBackwards);

    return TRUE; 
}

void CFindDialog::PostNcDestroy()    
{
    
    if(m_pOwner != NULL)    
        m_pOwner->ClearFindDialog();

    CFindReplaceDialog::PostNcDestroy();
}

BOOL CFindDialog::WrapAround()
{
    CButton* pChk = (CButton *)GetDlgItem(IDC_WRAP_AROUND);

    return pChk ? pChk->GetCheck() : FALSE;
}

BOOL CFindDialog::SearchBackwards()
{
    CButton* pChk = (CButton *)GetDlgItem(IDC_SEARCH_BACKWARDS);

    return pChk ? pChk->GetCheck() : FALSE;
}



CLinkPropertiesDlg::CLinkPropertiesDlg(CWnd* pParent )
	: CDialog(CLinkPropertiesDlg::IDD, pParent)
{
	m_LinkText = _T("");
	m_LinkLocation = _T("");
}

void CLinkPropertiesDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_LINK_TEXT, m_LinkText);
	DDX_Text(pDX, IDC_EDIT_LINK_LOCATION, m_LinkLocation);
}


BEGIN_MESSAGE_MAP(CLinkPropertiesDlg, CDialog)
END_MESSAGE_MAP()

void CLinkPropertiesDlg::OnOK() 
{
	UpdateData(TRUE);

	if (m_LinkLocation.IsEmpty() || (m_LinkText.IsEmpty() && m_LinkLocation.IsEmpty()))
	{
		MessageBox(_T("Please enter a Link Location"));
		return;
	}

	if (m_LinkText.IsEmpty())
	{
		m_LinkText = m_LinkLocation;
	}

	EndDialog(IDOK);
}
