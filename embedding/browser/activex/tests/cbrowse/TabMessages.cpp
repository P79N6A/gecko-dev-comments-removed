


#include "stdafx.h"
#include "cbrowse.h"
#include "TabMessages.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif




IMPLEMENT_DYNCREATE(CTabMessages, CPropertyPage)

CTabMessages::CTabMessages() : CPropertyPage(CTabMessages::IDD, CTabMessages::IDD)
{
	
	m_szStatus = _T("");
	
}


CTabMessages::~CTabMessages()
{
}


void CTabMessages::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	
	DDX_Control(pDX, IDC_PROGRESS, m_pcProgress);
	DDX_Control(pDX, IDC_OUTPUT, m_lbMessages);
	DDX_Text(pDX, IDC_STATUS, m_szStatus);
	
}


BEGIN_MESSAGE_MAP(CTabMessages, CPropertyPage)
	
		
	
END_MESSAGE_MAP()



