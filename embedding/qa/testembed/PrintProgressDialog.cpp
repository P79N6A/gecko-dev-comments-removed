


#include "stdafx.h"
#include "testembed.h"
#include "PrintProgressDialog.h"
#include "BrowserView.h"
#include "nsIWebBrowser.h"
#include "nsIWebBrowserPrint.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif




class CDlgPrintListener : public nsIWebProgressListener
{

public:
	CDlgPrintListener(CPrintProgressDialog* aDlg); 

  NS_DECL_ISUPPORTS
  NS_DECL_NSIWEBPROGRESSLISTENER

  void ClearDlg() { m_PrintDlg = NULL; } 


protected:
	CPrintProgressDialog* m_PrintDlg;
};

NS_IMPL_ADDREF(CDlgPrintListener)
NS_IMPL_RELEASE(CDlgPrintListener)

NS_INTERFACE_MAP_BEGIN(CDlgPrintListener)
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIWebProgressListener)
    NS_INTERFACE_MAP_ENTRY(nsIWebProgressListener)
NS_INTERFACE_MAP_END


CDlgPrintListener::CDlgPrintListener(CPrintProgressDialog* aDlg) :
  m_PrintDlg(aDlg)
{
  
}


NS_IMETHODIMP 
CDlgPrintListener::OnStateChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, PRUint32 aStateFlags, nsresult aStatus)
{
  if (m_PrintDlg) {
    if (aStateFlags == (nsIWebProgressListener::STATE_START|nsIWebProgressListener::STATE_IS_DOCUMENT)) {
      return m_PrintDlg->OnStartPrinting();

    } else if (aStateFlags == (nsIWebProgressListener::STATE_STOP|nsIWebProgressListener::STATE_IS_DOCUMENT)) {
      return m_PrintDlg->OnEndPrinting(aStatus);
    }
  }
  return NS_OK;
}


NS_IMETHODIMP 
CDlgPrintListener::OnProgressChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, PRInt32 aCurSelfProgress, PRInt32 aMaxSelfProgress, PRInt32 aCurTotalProgress, PRInt32 aMaxTotalProgress)
{
  if (m_PrintDlg) {
    return m_PrintDlg->OnProgressPrinting(aCurSelfProgress, aMaxSelfProgress);
  }
  return NS_OK;
}


NS_IMETHODIMP 
CDlgPrintListener::OnLocationChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, nsIURI *location)
{
    return NS_OK;
}


NS_IMETHODIMP 
CDlgPrintListener::OnStatusChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, nsresult aStatus, const PRUnichar *aMessage)
{
    return NS_OK;
}


NS_IMETHODIMP 
CDlgPrintListener::OnSecurityChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, PRUint32 state)
{
    return NS_OK;
}






CPrintProgressDialog::CPrintProgressDialog(nsIWebBrowser* aWebBrowser,
                                           nsIPrintSettings* aPrintSettings,
                                           CWnd* pParent )
	: CDialog(CPrintProgressDialog::IDD, pParent),
  m_WebBrowser(aWebBrowser),
  m_PrintListener(nsnull),
  m_PrintSettings(aPrintSettings),
  m_InModalMode(PR_FALSE)
{
	
		
	
}

CPrintProgressDialog::~CPrintProgressDialog()
{
  CDlgPrintListener * pl = (CDlgPrintListener*)m_PrintListener.get();
  if (pl) {
    pl->ClearDlg();
  }
}


void CPrintProgressDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	
		
	
}


BEGIN_MESSAGE_MAP(CPrintProgressDialog, CDialog)
	
	
END_MESSAGE_MAP()



static void GetLocalRect(CWnd * aWnd, CRect& aRect, CWnd * aParent)
{
  CRect wr;
  aParent->GetWindowRect(wr);

  CRect cr;
  aParent->GetClientRect(cr);

  aWnd->GetWindowRect(aRect);

  int borderH = wr.Height() - cr.Height();
  int borderW = (wr.Width() - cr.Width())/2;
  aRect.top    -= wr.top+borderH-borderW;
  aRect.left   -= wr.left+borderW;
  aRect.right  -= wr.left+borderW;
  aRect.bottom -= wr.top+borderH-borderW;

}

BOOL CPrintProgressDialog::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	CRect clientRect;
	GetClientRect(&clientRect);

	CRect titleRect;
  GetLocalRect(GetDlgItem(IDC_PPD_DOC_TITLE_STATIC), titleRect, this);

	CRect itemRect;
  GetLocalRect(GetDlgItem(IDC_PPD_DOC_TXT), itemRect, this);

  CRect progRect;
  progRect.left   = titleRect.left;
  progRect.top    = itemRect.top+itemRect.Height()+5;
  progRect.right  = clientRect.Width()-(2*titleRect.left);
  progRect.bottom = progRect.top+titleRect.Height();


	m_wndProgress.Create (WS_CHILD | WS_VISIBLE, progRect, this, -1);
	m_wndProgress.SetPos (0);
	
	return TRUE;  
	              
}


int CPrintProgressDialog::DoModal( )
{
  PRBool doModal = PR_FALSE;
  nsCOMPtr<nsIWebBrowserPrint> print(do_GetInterface(m_WebBrowser));
  if(print) 
  {
    m_PrintListener = new CDlgPrintListener(this); 
    if (m_PrintListener) {
      
      nsIWebProgressListener * wpl = NS_STATIC_CAST(nsIWebProgressListener*, m_PrintListener);
      doModal = NS_SUCCEEDED(print->Print(m_PrintSettings, wpl)) == PR_TRUE;
    }
  }

  if (doModal) {
    m_InModalMode = PR_TRUE;
    return CDialog::DoModal();
  }
  return 0;
}



NS_IMETHODIMP 
CPrintProgressDialog::OnStartPrinting()
{
  return NS_OK;
}


NS_IMETHODIMP 
CPrintProgressDialog::OnProgressPrinting(PRUint32 aProgress, PRUint32 aProgressMax)
{
  if (m_wndProgress.m_hWnd == NULL) return NS_OK;

  
  
  if (aProgress == 0) {
	  CWnd *pWnd = GetDlgItem(IDC_PPD_DOC_TXT);
	  if(pWnd)
		  pWnd->SetWindowText(m_URL);

	  m_wndProgress.SetRange(0, aProgressMax);
    m_wndProgress.SetPos(0);
  }
	m_wndProgress.SetPos(aProgress);
  RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);

  return NS_OK;
}


NS_IMETHODIMP 
CPrintProgressDialog::OnEndPrinting(PRUint32 aStatus)
{
  
  
  
  if (m_InModalMode) {
    EndDialog(1);
  }
  return NS_OK;
}

void CPrintProgressDialog::OnCancel() 
{
  nsCOMPtr<nsIWebBrowserPrint> print(do_GetInterface(m_WebBrowser));
  if (print) {
    print->Cancel();
  }

	CDialog::OnCancel();
}

void CPrintProgressDialog::SetURI(const char* aTitle)
{
  m_URL = _T(aTitle);
}
