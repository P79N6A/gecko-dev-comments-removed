










































#include "PromptService.h"
#include "nsCOMPtr.h"
#include "nsMemory.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsIDOMWindow.h"
#include "nsIEmbeddingSiteWindow.h"
#include "nsIFactory.h"
#include "nsIPromptService.h"
#include "nsPIPromptService.h"
#include "nsIServiceManager.h"
#include "nsIWebBrowserChrome.h"
#include "nsIWindowWatcher.h"
#include "PtMozilla.h"






class CPromptService: public nsIPromptService, public nsPIPromptService {
public:
                 CPromptService();
  virtual       ~CPromptService();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIPROMPTSERVICE
  NS_DECL_NSPIPROMPTSERVICE

private:
	nsCOMPtr<nsIWindowWatcher> mWWatch;
	PtWidget_t *GetWebBrowser(nsIDOMWindow *aWindow);
	int InvokeDialogCallback(PtWidget_t *w, int type, char *title, char *text, char *checkbox_msg, int *value);
};




NS_IMPL_ISUPPORTS2(CPromptService, nsIPromptService, nsPIPromptService)

CPromptService::CPromptService() :
	mWWatch(do_GetService("@mozilla.org/embedcomp/window-watcher;1")) {
	NS_INIT_ISUPPORTS();
	}

CPromptService::~CPromptService() {
	}

PtWidget_t *CPromptService::GetWebBrowser(nsIDOMWindow *aWindow)
{
  nsCOMPtr<nsIWebBrowserChrome> chrome;
  PtWidget_t *val = 0;

	nsCOMPtr<nsIWindowWatcher> wwatch(do_GetService("@mozilla.org/embedcomp/window-watcher;1"));
	if (!wwatch) return nsnull;

  if( wwatch ) {
    nsCOMPtr<nsIDOMWindow> fosterParent;
    if (!aWindow) { 
      wwatch->GetActiveWindow(getter_AddRefs(fosterParent));
      aWindow = fosterParent;
    }
    wwatch->GetChromeForWindow(aWindow, getter_AddRefs(chrome));
  }

  if (chrome) {
    nsCOMPtr<nsIEmbeddingSiteWindow> site(do_QueryInterface(chrome));
    if (site) {
      site->GetSiteWindow(reinterpret_cast<void **>(&val));
    }
  }

  return val;
}

int 
CPromptService::InvokeDialogCallback(PtWidget_t *w, int type, char *title, char *text, char *checkbox_msg, int *value)
{
    PtMozillaWidget_t   *moz = (PtMozillaWidget_t *) w;
    PtCallbackList_t    *cb;
    PtCallbackInfo_t    cbinfo;
    PtMozillaDialogCb_t dlg;
		PtWidget_t *parent;
    int ret;

    if (!moz->dialog_cb)
        return NS_OK;

    cb = moz->dialog_cb;
    memset(&cbinfo, 0, sizeof(cbinfo));
    cbinfo.reason = Pt_CB_MOZ_DIALOG;
    cbinfo.cbdata = &dlg;

    memset(&dlg, 0, sizeof(PtMozillaDialogCb_t));
    dlg.type = type;
    dlg.title = title;
    dlg.text = text;
   	dlg.checkbox_message = checkbox_msg;
		parent = PtFindDisjoint( w );
		dlg.parent = parent ? parent : w;	

    ret = PtInvokeCallbackList(cb, (PtWidget_t *)moz, &cbinfo);
    if (value)
        *value = dlg.ret_value;
    return (ret);
}

NS_IMETHODIMP CPromptService::Alert(nsIDOMWindow *parent, const PRUnichar *dialogTitle,
                                    const PRUnichar *text)
{
	nsString 			mTitle(dialogTitle);
	nsString 			mText(text);
	PtWidget_t *w = GetWebBrowser( parent );
	char *title = ToNewCString(mTitle), *ptext = ToNewCString(mText);

	if( w ) InvokeDialogCallback(w, Pt_MOZ_DIALOG_ALERT, title, ptext, nsnull, nsnull);
	else {
		char const *btns[] = { "&Ok" };
		PtAlert( NULL, NULL, title, NULL, ptext, NULL, 1, btns, NULL, 1, 1, 0 );
		}

	if( title ) nsMemory::Free( (void*)title );
	if( ptext ) nsMemory::Free( (void*)ptext );

	return NS_OK;
	}

NS_IMETHODIMP CPromptService::AlertCheck(nsIDOMWindow *parent,
                                         const PRUnichar *dialogTitle,
                                         const PRUnichar *text,
                                         const PRUnichar *checkboxMsg,
                                         PRBool *checkValue)
{
	nsString 	mTitle(dialogTitle);
	nsString 	mText(text);
	nsString 	mMsg(checkboxMsg);
	int 		ret = 0;
	PtWidget_t *w = GetWebBrowser( parent );
	char *title = ToNewCString(mTitle), *ptext = ToNewCString(mText), *msg = ToNewCString(mMsg);

	InvokeDialogCallback(w, Pt_MOZ_DIALOG_ALERT, title, ptext, msg, &ret);
	*checkValue = ret;

	if( title ) nsMemory::Free( (void*)title );
	if( ptext ) nsMemory::Free( (void*)ptext );
	if( msg ) nsMemory::Free( (void*)msg );

	return NS_OK;
	}


NS_IMETHODIMP CPromptService::Confirm(nsIDOMWindow *parent,
                                      const PRUnichar *dialogTitle,
                                      const PRUnichar *text,
                                      PRBool *_retval)
{
	nsString 			mTitle(dialogTitle);
	nsString 			mText(text);
	PtWidget_t *w = GetWebBrowser( parent );
	char *title = ToNewCString(mTitle), *ptext = ToNewCString(mText);

	if(InvokeDialogCallback(w, Pt_MOZ_DIALOG_CONFIRM, title, ptext, nsnull, nsnull) == Pt_CONTINUE)
		*_retval = PR_TRUE;
	else
		*_retval = PR_FALSE;

	if( title ) nsMemory::Free( (void*)title );
	if( ptext ) nsMemory::Free( (void*)ptext );

	return NS_OK;
}

NS_IMETHODIMP CPromptService::ConfirmCheck(nsIDOMWindow *parent,
                                           const PRUnichar *dialogTitle,
                                           const PRUnichar *text,
                                           const PRUnichar *checkboxMsg,
                                           PRBool *checkValue,
                                           PRBool *_retval)
{
  nsString  mTitle(dialogTitle);
  nsString  mText(text);
  nsString  mMsg(checkboxMsg);
  int     ret = 0;
  PtWidget_t *w = GetWebBrowser( parent );
	char *title = ToNewCString(mTitle), *ptext = ToNewCString(mText), *msg = ToNewCString(mMsg);

  if (InvokeDialogCallback(w, Pt_MOZ_DIALOG_CONFIRM_CHECK, title, ptext, msg, &ret) == Pt_CONTINUE)
    *_retval = PR_TRUE;
  else
    *_retval = PR_FALSE;
  if (checkValue)
    *checkValue = ret;

	if( title ) nsMemory::Free( (void*)title );
	if( ptext ) nsMemory::Free( (void*)ptext );
	if( msg ) nsMemory::Free( (void*)msg );

  return NS_OK;

}

NS_IMETHODIMP CPromptService::Prompt(nsIDOMWindow *parent,
                                     const PRUnichar *dialogTitle,
                                     const PRUnichar *text,
                                     PRUnichar **value,
                                     const PRUnichar *checkboxMsg,
                                     PRBool *checkValue,
                                     PRBool *_retval)
{
	nsString 	mTitle(dialogTitle);
	nsString 	mText(text);
	nsString 	mMsg(checkboxMsg);
	int				ret = 0;
	PtWidget_t *w = GetWebBrowser( parent );
	char *title = ToNewCString(mTitle), *ptext = ToNewCString(mText), *msg = ToNewCString(mMsg);

	if(InvokeDialogCallback(w, Pt_MOZ_DIALOG_CONFIRM, title, ptext, msg, &ret) == Pt_CONTINUE)
		*_retval = PR_TRUE;
	else
		*_retval = PR_FALSE;
	if (checkValue)
		*checkValue = ret;

	if( title ) nsMemory::Free( (void*)title );
	if( ptext ) nsMemory::Free( (void*)ptext );
	if( msg ) nsMemory::Free( (void*)msg );

	return NS_OK;
}

NS_IMETHODIMP CPromptService::PromptUsernameAndPassword(nsIDOMWindow *parent,
                                                        const PRUnichar *dialogTitle,
                                                        const PRUnichar *text,
                                                        PRUnichar **username,
                                                        PRUnichar **password,
                                                        const PRUnichar *checkboxMsg,
                                                        PRBool *checkValue,
                                                        PRBool *_retval)
{
	PtWidget_t *w = GetWebBrowser( parent );
	PtMozillaWidget_t 			*moz = (PtMozillaWidget_t *) w;
	PtCallbackList_t 			*cb;
	PtCallbackInfo_t 			cbinfo;
	PtMozillaAuthenticateCb_t   auth;
	nsString 					mTitle(dialogTitle);
	nsString 					mRealm(checkboxMsg);

	if (!moz->auth_cb)
	    return NS_OK;

	cb = moz->auth_cb;
	memset(&cbinfo, 0, sizeof(cbinfo));
	cbinfo.reason = Pt_CB_MOZ_AUTHENTICATE;
	cbinfo.cbdata = &auth;

	memset(&auth, 0, sizeof(PtMozillaAuthenticateCb_t));
	auth.title = ToNewCString(mTitle);
	auth.realm = ToNewCString(mRealm);

  	if (PtInvokeCallbackList(cb, (PtWidget_t *)moz, &cbinfo) == Pt_CONTINUE)
    {
		nsCString	mUser(auth.user);
		nsCString	mPass(auth.pass);
		*username = ToNewUnicode(mUser);
		*password = ToNewUnicode(mPass);
    	*_retval = PR_TRUE;
    }
    else
    	*_retval = PR_FALSE;

	free( auth.title );
	free( auth.realm );

	return NS_OK;
}

NS_IMETHODIMP CPromptService::PromptPassword(nsIDOMWindow *parent,
                                             const PRUnichar *dialogTitle,
                                             const PRUnichar *text,
                                             PRUnichar **password,
                                             const PRUnichar *checkboxMsg,
                                             PRBool *checkValue,
                                             PRBool *_retval)
{
	return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP CPromptService::Select(nsIDOMWindow *parent,
                                     const PRUnichar *dialogTitle,
                                     const PRUnichar *text, PRUint32 count,
                                     const PRUnichar **selectList,
                                     PRInt32 *outSelection,
                                     PRBool *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP CPromptService::ConfirmEx(nsIDOMWindow *parent,
                                        const PRUnichar *dialogTitle,
                                        const PRUnichar *text,
                                        PRUint32 buttonFlags,
                                        const PRUnichar *button0Title,
                                        const PRUnichar *button1Title,
                                        const PRUnichar *button2Title,
                                        const PRUnichar *checkMsg,
                                        PRBool *checkValue,
                                        PRInt32 *buttonPressed)
{
	if (checkValue)
		*checkValue = PR_FALSE;
	if (buttonPressed)
		*buttonPressed = 0;
	return NS_OK;
}

nsresult
CPromptService::DoDialog(nsIDOMWindow *aParent,
                   nsIDialogParamBlock *aParamBlock, const char *aChromeURL)
{
#if 0
  NS_ENSURE_ARG(aParamBlock);
  NS_ENSURE_ARG(aChromeURL);
  if (!mWatcher)
    return NS_ERROR_FAILURE;

  nsresult rv = NS_OK;

  
  
  
  nsCOMPtr<nsIDOMWindow> activeParent; 
  if (!aParent) {
    mWatcher->GetActiveWindow(getter_AddRefs(activeParent));
    aParent = activeParent;
  }

  nsCOMPtr<nsISupports> arguments(do_QueryInterface(aParamBlock));
  nsCOMPtr<nsIDOMWindow> dialog;
  rv = mWatcher->OpenWindow(aParent, aChromeURL, "_blank",
                            "centerscreen,chrome,modal,titlebar", arguments,
                            getter_AddRefs(dialog));

  return rv;
#else
	return NS_OK;
#endif
}
 




class CPromptServiceFactory : public nsIFactory {
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIFACTORY

  CPromptServiceFactory();
  virtual ~CPromptServiceFactory();
};



NS_IMPL_ISUPPORTS1(CPromptServiceFactory, nsIFactory)

CPromptServiceFactory::CPromptServiceFactory() {

  NS_INIT_ISUPPORTS();
}

CPromptServiceFactory::~CPromptServiceFactory() {
}

NS_IMETHODIMP CPromptServiceFactory::CreateInstance(nsISupports *aOuter, const nsIID & aIID, void **aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  
  *aResult = NULL;  
  CPromptService *inst = new CPromptService;    
  if (!inst)
    return NS_ERROR_OUT_OF_MEMORY;
    
  nsresult rv = inst->QueryInterface(aIID, aResult);
  if (rv != NS_OK) {  
    
    delete inst;  
  }  
    
  return rv;
}

NS_IMETHODIMP CPromptServiceFactory::LockFactory(PRBool lock)
{
  return NS_OK;
}



void InitPromptService( ) {
}

nsresult NS_NewPromptServiceFactory(nsIFactory** aFactory)
{
  NS_ENSURE_ARG_POINTER(aFactory);
  *aFactory = nsnull;
  
  CPromptServiceFactory *result = new CPromptServiceFactory;
  if (!result)
    return NS_ERROR_OUT_OF_MEMORY;
    
  NS_ADDREF(result);
  *aFactory = result;
  
  return NS_OK;
}
