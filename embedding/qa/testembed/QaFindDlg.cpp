


#include "stdafx.h"
#include "testembed.h"
#include "QaFindDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif





CQaFindDlg::CQaFindDlg(CWnd* pParent )
	: CDialog(CQaFindDlg::IDD, pParent)
{
	
		
	
	m_textfield = _T("text");
}


void CQaFindDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	
		
	DDX_Text(pDX, IDC_TEXTFIELD, m_textfield);
	
}


BEGIN_MESSAGE_MAP(CQaFindDlg, CDialog)
	
		
	
END_MESSAGE_MAP()



