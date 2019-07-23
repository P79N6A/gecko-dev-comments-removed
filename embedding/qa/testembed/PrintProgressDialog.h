#if !defined(AFX_PRINTPROGRESSDIALOG_H__1BAF9B13_1875_11D5_9773_000064657374__INCLUDED_)
#define AFX_PRINTPROGRESSDIALOG_H__1BAF9B13_1875_11D5_9773_000064657374__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif 



#include "nsIWebProgressListener.h"
class nsIWebBrowser;




class CPrintProgressDialog : public CDialog
{

public:
	CPrintProgressDialog(nsIWebBrowser* aWebBrowser, 
                       nsIPrintSettings* aPrintSettings,
                       CWnd* pParent = NULL);
	virtual ~CPrintProgressDialog();
  virtual int DoModal( );

  
  void SetURI(const char* aTitle);

  NS_IMETHOD OnStartPrinting(void);
  NS_IMETHOD OnProgressPrinting(PRUint32 aProgress, PRUint32 aProgressMax);
  NS_IMETHOD OnEndPrinting(PRUint32 aStatus);


	
	enum { IDD = IDD_PRINT_PROGRESS_DIALOG };
		
	



	
	
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    
	


protected:
	CProgressCtrl              m_wndProgress;
  CString                    m_URL;
  nsIWebBrowser*             m_WebBrowser;
  nsCOMPtr<nsIWebProgressListener> m_PrintListener;
  nsIPrintSettings*          m_PrintSettings;
  BOOL                       m_InModalMode;

	
	
	virtual BOOL OnInitDialog();
	virtual void OnCancel();
	
	DECLARE_MESSAGE_MAP()
};




#endif
