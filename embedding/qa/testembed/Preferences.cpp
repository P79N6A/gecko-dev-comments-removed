





































#include "stdafx.h"
#include "Preferences.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif




IMPLEMENT_DYNAMIC(CPreferences, CPropertySheet)

CPreferences::CPreferences(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
    AddPage(&m_startupPage);
}

CPreferences::~CPreferences()
{
}

BEGIN_MESSAGE_MAP(CPreferences, CPropertySheet)
	
		
	
END_MESSAGE_MAP()


BOOL CPreferences::OnInitDialog() 
{
	BOOL bResult = CPropertySheet::OnInitDialog();
	
    
    CWnd* pApplyButton = GetDlgItem(ID_APPLY_NOW);
    ASSERT(pApplyButton);
    pApplyButton->ShowWindow(SW_HIDE);

	return bResult;
}





IMPLEMENT_DYNCREATE(CStartupPrefsPage, CPropertyPage)

CStartupPrefsPage::CStartupPrefsPage() : CPropertyPage(CStartupPrefsPage::IDD)
{
	
	m_strHomePage = _T("");
	m_iStartupPage = -1;
	
}

CStartupPrefsPage::~CStartupPrefsPage()
{
}

void CStartupPrefsPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	
	DDX_Control(pDX, IDC_EDIT_HOMEPAGE, m_HomePage);
	DDX_Text(pDX, IDC_EDIT_HOMEPAGE, m_strHomePage);
	DDX_Radio(pDX, IDC_RADIO_BLANK_PAGE, m_iStartupPage);
	
}


BEGIN_MESSAGE_MAP(CStartupPrefsPage, CPropertyPage)
	
	
END_MESSAGE_MAP()
