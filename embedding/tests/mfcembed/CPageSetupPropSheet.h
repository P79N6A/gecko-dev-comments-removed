






























#if !defined(AFX_CPAGESETUPPROPSHEET_H__E8A6D703_7EAD_4729_8FBA_9E0515AB9822__INCLUDED_)
#define AFX_CPAGESETUPPROPSHEET_H__E8A6D703_7EAD_4729_8FBA_9E0515AB9822__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif 


#include "CFormatOptionTab.h"
#include "CMarginHeaderFooter.h"




class CPageSetupPropSheet : public CPropertySheet
{
	DECLARE_DYNAMIC(CPageSetupPropSheet)


public:
	CPageSetupPropSheet(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	CPageSetupPropSheet(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

  void SetPrintSettingsValues(nsIPrintSettings* aPrintSettings);
  void GetPrintSettingsValues(nsIPrintSettings* aPrintSettings);

protected:
	void AddControlPages(void);


public:
	CFormatOptionTab     m_FormatOptionTab;
	CMarginHeaderFooter  m_MarginHeaderFooterTab;


public:


	
	
	


public:
	virtual ~CPageSetupPropSheet();

	
protected:
	
		
	
	DECLARE_MESSAGE_MAP()
};






#endif
