


#include "stdafx.h"
#include "cbrowse.h"
#include "TabDOM.h"
#include "CBrowseDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif




IMPLEMENT_DYNCREATE(CTabDOM, CPropertyPage)

CTabDOM::CTabDOM() : CPropertyPage(CTabDOM::IDD, CTabDOM::IDD)
{
	
		
	
}


CTabDOM::~CTabDOM()
{
}


void CTabDOM::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	
	DDX_Control(pDX, IDC_DOMLIST, m_tcDOM);
	
}


BEGIN_MESSAGE_MAP(CTabDOM, CPropertyPage)
	
	ON_BN_CLICKED(IDC_REFRESHDOM, OnRefreshDOM)
	
END_MESSAGE_MAP()




void CTabDOM::OnRefreshDOM() 
{
	if (m_pBrowseDlg)
	{
		m_pBrowseDlg->OnRefreshDOM();
	}
}


BOOL CTabDOM::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	
	m_tcDOM.SetImageList(&m_pBrowseDlg->m_cImageList, TVSIL_NORMAL);
	
	return TRUE;  
	              
}
