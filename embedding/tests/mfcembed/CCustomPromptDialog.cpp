


#include "stdafx.h"
#include "mfcembed.h"
#include "CCustomPromptDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif





CCustomPromptDialog::CCustomPromptDialog(CWnd* pParent )
	: CDialog(CCustomPromptDialog::IDD, pParent)
{
	
	m_CustomText = _T("");
	
}


void CCustomPromptDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	
	DDX_Text(pDX, IDC_PROMPT_TEXT, m_CustomText);
	
}


BEGIN_MESSAGE_MAP(CCustomPromptDialog, CDialog)
	
		
	
END_MESSAGE_MAP()



