






























#ifndef __HelperAppDlg_h
#define __HelperAppDlg_h

#include "nsError.h"
#include "resource.h"
#include "nsIFactory.h"
#include "nsIExternalHelperAppService.h"
#include "nsIHelperAppLauncherDialog.h"
#include "nsIWebProgressListener.h"
#include "nsWeakReference.h"
#include "nsIWindowWatcher.h"


#ifndef XP_WIN
#define XP_WIN
#endif

class CHelperAppLauncherDialogFactory : public nsIFactory {
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIFACTORY

    CHelperAppLauncherDialogFactory();
    virtual ~CHelperAppLauncherDialogFactory();
};

enum {
    CONTENT_SAVE_TO_DISK,
    CONTENT_LAUNCH_WITH_APP
};



class CHelperAppLauncherDialog : public nsIHelperAppLauncherDialog {
public:
    CHelperAppLauncherDialog();
    virtual ~CHelperAppLauncherDialog();

    NS_DECL_ISUPPORTS
    NS_DECL_NSIHELPERAPPLAUNCHERDIALOG

    int m_HandleContentOp;
    CString m_FileName;

private:
  nsCOMPtr<nsIWindowWatcher> mWWatch;
  CWnd* GetParentFromContext(nsISupports *aWindowContext);
};




class CProgressDlg : public CDialog, 
                     public nsIWebProgressListener,
                     public nsSupportsWeakReference
{
public:
    enum { IDD = IDD_PROGRESS_DIALOG };

    CProgressDlg(nsIHelperAppLauncher *aLauncher, int aHandleContentOp,
                CString& aFileName, CWnd* pParent = NULL);
    virtual ~CProgressDlg();

    int m_HandleContentOp;
    CString m_FileName;
    CStatic	m_SavingFrom;
    CStatic	m_Action;
    CProgressCtrl m_ProgressCtrl;

    nsCOMPtr<nsIHelperAppLauncher> m_HelperAppLauncher;

    NS_DECL_ISUPPORTS
    NS_DECL_NSIWEBPROGRESSLISTENER

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnCancel();

	DECLARE_MESSAGE_MAP()
};





class CChooseActionDlg : public CDialog
{
public:
    CChooseActionDlg(nsIHelperAppLauncher* aLauncher, CWnd* pParent = NULL);

    enum { IDD = IDD_CHOOSE_ACTION_DIALOG };
    CStatic	m_ContentType;

    int m_ActionChosen;
    CString m_OpenWithAppName;
    nsCOMPtr<nsIHelperAppLauncher> m_HelperAppLauncher;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);

protected:
    virtual BOOL OnInitDialog();
    afx_msg void OnChooseAppClicked();
    virtual void OnOK();
    virtual void OnCancel();
    afx_msg void OnOpenUsingRadioBtnClicked();
    afx_msg void OnSaveToDiskRadioBtnClicked();

    void EnableAppName(BOOL bEnable);
    void EnableChooseBtn(BOOL bEnable);
    void InitWithPreferredAction(nsIMIMEInfo* aMimeInfo);
    void SelectSaveToDiskOption();
    void SelectOpenUsingAppOption();
    void UpdateAppNameField(CString& appName);

    DECLARE_MESSAGE_MAP()
};

#endif
