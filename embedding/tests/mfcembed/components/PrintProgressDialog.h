#if !defined(AFX_PRINTPROGRESSDIALOG_H__1E3AA1B5_B8BB_4B25_86A5_A90E663D137F__INCLUDED_)
#define AFX_PRINTPROGRESSDIALOG_H__1E3AA1B5_B8BB_4B25_86A5_A90E663D137F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif 



#include "resource.h"
#include "nsIWebProgressListener.h"
#include "nsIWebBrowserPrint.h"
#include "nsIPrintSettings.h"
#include "nsIPrintProgressParams.h"
#include "nsCOMPtr.h"




class CPrintProgressDialog : public CDialog
{

public:
	CPrintProgressDialog(CWnd*                   pParent,
                       BOOL                    aIsForPrinting,
                       nsIPrintProgressParams* aPPParams,
                       nsIWebBrowserPrint*     aWebBrowserPrint, 
                       nsIPrintSettings*       aPrintSettings);
	virtual ~CPrintProgressDialog();

  
  void SetDocAndURL();

  NS_IMETHOD OnStartPrinting(void);
  NS_IMETHOD OnProgressPrinting(PRUint32 aProgress, PRUint32 aProgressMax);
  NS_IMETHOD OnEndPrinting(PRUint32 aStatus);


	
	enum { IDD = IDD_PRINT_PROGRESS_DIALOG };
		
	



	
	
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    
	


protected:
	CProgressCtrl                    m_wndProgress;
  CString                          m_URL;
  nsCOMPtr<nsIWebBrowserPrint>     m_WebBrowserPrint;
  nsCOMPtr<nsIPrintProgressParams> m_PPParams;
  nsCOMPtr<nsIWebProgressListener> m_PrintListener;
  nsCOMPtr<nsIPrintSettings>       m_PrintSettings;
  BOOL                             m_HasStarted;
  BOOL                             m_IsForPrinting;

	
	
	virtual BOOL OnInitDialog();
	virtual void OnCancel();
	
	DECLARE_MESSAGE_MAP()
};




#endif
