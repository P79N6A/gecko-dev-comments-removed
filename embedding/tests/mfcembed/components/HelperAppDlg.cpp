






























#include "stdafx.h"
#include "HelperAppService.h"
#include "HelperAppDlg.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsILocalFile.h"
#include "nsIURI.h"
#include "nsNetError.h"
#include "nsIMIMEInfo.h"
#include "nsIDOMWindow.h"
#include "nsIEmbeddingSiteWindow.h"
#include "nsIServiceManager.h"
#include "nsIWebBrowserChrome.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"































static HINSTANCE gInstance;





class ResourceState {
public:
    ResourceState() {
        mPreviousInstance = ::AfxGetResourceHandle();
        ::AfxSetResourceHandle(gInstance);
    }
    ~ResourceState() {
        ::AfxSetResourceHandle(mPreviousInstance);
    }
private:
    HINSTANCE mPreviousInstance;
};





void InitHelperAppDlg(HINSTANCE instance) 
{
  gInstance = instance;
}

nsresult NS_NewHelperAppDlgFactory(nsIFactory** aFactory)
{
  NS_ENSURE_ARG_POINTER(aFactory);
  *aFactory = nsnull;
  
  CHelperAppLauncherDialogFactory *result = new CHelperAppLauncherDialogFactory;
  if (!result)
    return NS_ERROR_OUT_OF_MEMORY;
    
  NS_ADDREF(result);
  *aFactory = result;
  
  return NS_OK;
}





NS_IMPL_ISUPPORTS1(CHelperAppLauncherDialogFactory, nsIFactory)

CHelperAppLauncherDialogFactory::CHelperAppLauncherDialogFactory() 
{
}

CHelperAppLauncherDialogFactory::~CHelperAppLauncherDialogFactory() {
}

NS_IMETHODIMP CHelperAppLauncherDialogFactory::CreateInstance(nsISupports *aOuter, const nsIID & aIID, void **aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  
  *aResult = NULL;  
  CHelperAppLauncherDialog *inst = new CHelperAppLauncherDialog;
  if (!inst)
    return NS_ERROR_OUT_OF_MEMORY;
    
  nsresult rv = inst->QueryInterface(aIID, aResult);
  if (rv != NS_OK) {  
    
    delete inst;  
  }  
    
  return rv;
}

NS_IMETHODIMP CHelperAppLauncherDialogFactory::LockFactory(PRBool lock)
{
  return NS_OK;
}





NS_IMPL_ISUPPORTS1(CHelperAppLauncherDialog, nsIHelperAppLauncherDialog)

CHelperAppLauncherDialog::CHelperAppLauncherDialog() :
      mWWatch(do_GetService(NS_WINDOWWATCHER_CONTRACTID))
{
}

CHelperAppLauncherDialog::~CHelperAppLauncherDialog() 
{
}




CWnd* CHelperAppLauncherDialog::GetParentFromContext(nsISupports *aWindowContext)
{
    nsCOMPtr<nsIDOMWindow> domWnd(do_GetInterface(aWindowContext));
    if(!domWnd) 
        return NULL;

    CWnd *retWnd = NULL;

    nsCOMPtr<nsIWebBrowserChrome> chrome;
    if(mWWatch)
    {
        nsCOMPtr<nsIDOMWindow> fosterParent;
        if (!domWnd) 
        { 
            mWWatch->GetActiveWindow(getter_AddRefs(fosterParent));
            domWnd = fosterParent;
        }
        mWWatch->GetChromeForWindow(domWnd, getter_AddRefs(chrome));
    }

    if (chrome) 
    {
        nsCOMPtr<nsIEmbeddingSiteWindow> site(do_QueryInterface(chrome));
        if (site)
        {
            HWND w;
            site->GetSiteWindow(reinterpret_cast<void **>(&w));
            retWnd = CWnd::FromHandle(w);
        }
    }

    return retWnd;
}




NS_IMETHODIMP CHelperAppLauncherDialog::Show(nsIHelperAppLauncher *aLauncher, 
                                             nsISupports *aContext,
                                             PRUint32 aReason)
{
    ResourceState setState;

    NS_ENSURE_ARG_POINTER(aLauncher);

    CChooseActionDlg dlg(aLauncher, GetParentFromContext(aContext));
    if(dlg.DoModal() == IDCANCEL)
    {
        

        aLauncher->Cancel(NS_BINDING_ABORTED);

        return NS_OK;
    }

    

    if(dlg.m_ActionChosen == CONTENT_SAVE_TO_DISK)
    {
        m_HandleContentOp = CONTENT_SAVE_TO_DISK;

        return aLauncher->SaveToDisk(nsnull, PR_FALSE);
    }
    else
    {
        m_HandleContentOp = CONTENT_LAUNCH_WITH_APP;

        m_FileName = dlg.m_OpenWithAppName;

        USES_CONVERSION;
        nsCAutoString fileName(T2CA(m_FileName));

        nsCOMPtr<nsILocalFile> openWith;
        nsresult rv = NS_NewNativeLocalFile(fileName, PR_FALSE, getter_AddRefs(openWith));
        if (NS_FAILED(rv))
            return aLauncher->LaunchWithApplication(nsnull, PR_FALSE);
        else
            return aLauncher->LaunchWithApplication(openWith, PR_FALSE);
    }
}





NS_IMETHODIMP CHelperAppLauncherDialog::PromptForSaveToFile(nsIHelperAppLauncher* aLauncher,
                                                             nsISupports *aWindowContext, 
                                                             const PRUnichar *aDefaultFile, 
                                                             const PRUnichar *aSuggestedFileExtension, 
                                                             nsILocalFile **_retval)
{
    USES_CONVERSION;

    NS_ENSURE_ARG_POINTER(_retval);

    TCHAR *lpszFilter = _T("All Files (*.*)|*.*||");
    CFileDialog cf(FALSE, W2CT(aSuggestedFileExtension), W2CT(aDefaultFile),
                    OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
                    lpszFilter, GetParentFromContext(aWindowContext));
    if(cf.DoModal() == IDOK)
    {
        m_FileName = cf.GetPathName(); 
        USES_CONVERSION;
        nsCAutoString fileName(T2CA(m_FileName));
        return NS_NewNativeLocalFile(fileName, PR_FALSE, _retval);
    }
    else
        return NS_ERROR_FAILURE;
}





CChooseActionDlg::CChooseActionDlg(nsIHelperAppLauncher *aLauncher, CWnd* pParent )
    : CDialog(CChooseActionDlg::IDD, pParent)
{
    m_HelperAppLauncher = aLauncher;
}

void CChooseActionDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_CONTENT_TYPE, m_ContentType);
}

BOOL CChooseActionDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

    
    
    nsCOMPtr<nsIMIMEInfo> mimeInfo;
    if(m_HelperAppLauncher)
        m_HelperAppLauncher->GetMIMEInfo(getter_AddRefs(mimeInfo));

    if(mimeInfo) 
    {
        
        
        
        nsCAutoString mimeType;
        nsresult rv = mimeInfo->GetMIMEType(mimeType);
        if(NS_SUCCEEDED(rv)) 
        {
            CStatic *pMimeType = (CStatic *)GetDlgItem(IDC_CONTENT_TYPE);
            if(pMimeType)
            {
                USES_CONVERSION;
                pMimeType->SetWindowText(A2CT(mimeType.get()));
            }
        }

        
        
        
        InitWithPreferredAction(mimeInfo);
    }
    else
        SelectSaveToDiskOption();
    
    return FALSE;  
}





void CChooseActionDlg::InitWithPreferredAction(nsIMIMEInfo* aMimeInfo)
{
    
    
    nsMIMEInfoHandleAction prefAction = nsIMIMEInfo::saveToDisk;
    aMimeInfo->GetPreferredAction(&prefAction);
    if(prefAction == nsIMIMEInfo::saveToDisk)
    {
        SelectSaveToDiskOption();
        return;
    }

    
    
    SelectOpenUsingAppOption();

    
    
    nsAutoString appDesc;
    nsresult rv = aMimeInfo->GetApplicationDescription(appDesc);
    if(NS_SUCCEEDED(rv)) 
    {
        USES_CONVERSION;
        m_OpenWithAppName = W2CT(appDesc.get());

        
        
        UpdateAppNameField(m_OpenWithAppName);
    }
}

void CChooseActionDlg::UpdateAppNameField(CString& appName)
{
    CStatic *pAppName = (CStatic *)GetDlgItem(IDC_APP_NAME);
    if(pAppName)
        pAppName->SetWindowText(appName);    
}

void CChooseActionDlg::SelectSaveToDiskOption()
{
    CButton *pRadioSaveToDisk = (CButton *)GetDlgItem(IDC_SAVE_TO_DISK);
    if(pRadioSaveToDisk)
    {
        pRadioSaveToDisk->SetCheck(1);

        pRadioSaveToDisk->SetFocus();

        OnSaveToDiskRadioBtnClicked();
    }
}

void CChooseActionDlg::SelectOpenUsingAppOption()
{
    CButton *pRadioOpenUsing = (CButton *)GetDlgItem(IDC_OPEN_USING);
    if(pRadioOpenUsing)
    {
        pRadioOpenUsing->SetCheck(1);

        pRadioOpenUsing->SetFocus();

        OnOpenUsingRadioBtnClicked();
    }
}

void CChooseActionDlg::EnableChooseBtn(BOOL bEnable)
{
    CButton *pChooseBtn = (CButton *)GetDlgItem(IDC_CHOOSE_APP);
    if(pChooseBtn)
    {
        pChooseBtn->EnableWindow(bEnable);
    }
}

void CChooseActionDlg::EnableAppName(BOOL bEnable)
{
    CStatic *pAppName = (CStatic *)GetDlgItem(IDC_APP_NAME);
    if(pAppName)
    {
        pAppName->EnableWindow(bEnable);
    }
}

void CChooseActionDlg::OnOpenUsingRadioBtnClicked()
{
    EnableChooseBtn(TRUE);
    EnableAppName(TRUE);
}

void CChooseActionDlg::OnSaveToDiskRadioBtnClicked()
{
    EnableChooseBtn(FALSE);
    EnableAppName(FALSE);
}

void CChooseActionDlg::OnChooseAppClicked() 
{	
    TCHAR *lpszFilter =
        _T("EXE Files Only (*.exe)|*.exe|")
        _T("All Files (*.*)|*.*||");

    CFileDialog cf(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
                    lpszFilter, this);
    if(cf.DoModal() == IDOK)
    {
        m_OpenWithAppName = cf.GetPathName(); 

        UpdateAppNameField(m_OpenWithAppName);
    }
}

void CChooseActionDlg::OnOK() 
{	
    CButton *pRadioOpenWithApp = (CButton *)GetDlgItem(IDC_OPEN_USING);

    if(pRadioOpenWithApp->GetCheck())
    {
        
        
        
        if(m_OpenWithAppName.IsEmpty())
        {
            ::MessageBox(this->m_hWnd,
                _T("You have chosen to open the content with an external application, but,\nno application has been specified.\n\nPlease click the Choose... button to select an application"),
                _T("MfcEmbed"), MB_OK);
            return;
        }
        else
            m_ActionChosen = CONTENT_LAUNCH_WITH_APP;
    }
    else
        m_ActionChosen = CONTENT_SAVE_TO_DISK;

    CDialog::OnOK();
}

void CChooseActionDlg::OnCancel() 
{	
    CDialog::OnCancel();
}

BEGIN_MESSAGE_MAP(CChooseActionDlg, CDialog)
    ON_BN_CLICKED(IDC_CHOOSE_APP, OnChooseAppClicked)
    ON_BN_CLICKED(IDC_SAVE_TO_DISK, OnSaveToDiskRadioBtnClicked)
    ON_BN_CLICKED(IDC_OPEN_USING, OnOpenUsingRadioBtnClicked)
END_MESSAGE_MAP()





NS_IMPL_ISUPPORTS2(CProgressDlg, nsIWebProgressListener, nsISupportsWeakReference)

CProgressDlg::CProgressDlg(nsIHelperAppLauncher *aLauncher, int aHandleContentOp,
                           CString& aFileName, CWnd* pParent )
	: CDialog(CProgressDlg::IDD, pParent)
{
    m_HelperAppLauncher = aLauncher;
    m_HandleContentOp = aHandleContentOp;
    m_FileName = aFileName;
}

CProgressDlg::~CProgressDlg() 
{
}

NS_IMETHODIMP CProgressDlg::OnStateChange(nsIWebProgress *aWebProgress, 
                                          nsIRequest *aRequest, PRUint32 aStateFlags, 
                                          nsresult aStatus)
{
    if((aStateFlags & STATE_STOP) && (aStateFlags & STATE_IS_DOCUMENT))
    {
        
        
        if(m_HelperAppLauncher)
            m_HelperAppLauncher->CloseProgressWindow();

        DestroyWindow();
    }

    return NS_OK;
}

NS_IMETHODIMP CProgressDlg::OnProgressChange(nsIWebProgress *aWebProgress, 
                                             nsIRequest *aRequest, 
                                             PRInt32 aCurSelfProgress, PRInt32 aMaxSelfProgress, 
                                             PRInt32 aCurTotalProgress, PRInt32 aMaxTotalProgress)
{
    

    if(::IsWindow(m_ProgressCtrl.m_hWnd))
    {
        m_ProgressCtrl.SetRange32(0, aMaxTotalProgress);
        m_ProgressCtrl.SetPos(aCurTotalProgress);
    }

    return NS_OK;
}

NS_IMETHODIMP CProgressDlg::OnLocationChange(nsIWebProgress *aWebProgress, 
                                             nsIRequest *aRequest, nsIURI *location)
{
    return NS_OK;
}

NS_IMETHODIMP CProgressDlg::OnStatusChange(nsIWebProgress *aWebProgress, 
                                           nsIRequest *aRequest, nsresult aStatus, 
                                           const PRUnichar *aMessage)
{
    return NS_OK;
}

NS_IMETHODIMP CProgressDlg::OnSecurityChange(nsIWebProgress *aWebProgress, 
                                             nsIRequest *aRequest, PRUint32 state)
{
    return NS_OK;
}

BOOL CProgressDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

    
    if(m_HelperAppLauncher)
    {
        nsCOMPtr<nsIURI> srcUri;
        nsresult rv = m_HelperAppLauncher->GetSource(getter_AddRefs(srcUri));
        if(NS_SUCCEEDED(rv))
        {
            nsCAutoString uriString;
            srcUri->GetSpec(uriString);
            USES_CONVERSION;
            m_SavingFrom.SetWindowText(A2CT(uriString.get()));
        }
    }

    
    if(m_HandleContentOp == CONTENT_SAVE_TO_DISK)
        m_Action.SetWindowText(_T("[Saving file to:] ") + m_FileName);
    else if(m_HandleContentOp == CONTENT_LAUNCH_WITH_APP)
        m_Action.SetWindowText(_T("[Opening file with:] ") + m_FileName);

    return TRUE;
}

void CProgressDlg::OnCancel() 
{
    if(m_HelperAppLauncher)
        m_HelperAppLauncher->Cancel(NS_BINDING_ABORTED);

	DestroyWindow();
}

void CProgressDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_PROGRESS, m_ProgressCtrl);
    DDX_Control(pDX, IDC_SAVING_FROM, m_SavingFrom);
    DDX_Control(pDX, IDC_ACTION, m_Action);
}

BEGIN_MESSAGE_MAP(CProgressDlg, CDialog)
END_MESSAGE_MAP()
