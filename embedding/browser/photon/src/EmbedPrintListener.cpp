





































#include <strings.h>
#include <nsXPIDLString.h>

#include "nsIURI.h"

#include "EmbedPrintListener.h"
#include "EmbedPrivate.h"

#include "PtMozilla.h"

EmbedPrintListener::EmbedPrintListener(void)
{
  mOwner = nsnull;
}

EmbedPrintListener::~EmbedPrintListener()
{
}

NS_IMPL_ISUPPORTS1(EmbedPrintListener, nsIWebProgressListener)

nsresult
EmbedPrintListener::Init(EmbedPrivate *aOwner)
{
	mOwner = aOwner;
	return NS_OK;
}

void 
EmbedPrintListener::InvokePrintCallback(int status, unsigned int cur, unsigned int max)
{
	PtMozillaWidget_t   *moz = (PtMozillaWidget_t *) mOwner->mOwningWidget;
	PtCallbackList_t 	*cb;
	PtCallbackInfo_t 	cbinfo;
	PtMozillaPrintStatusCb_t	pstatus;

	if (!moz->print_status_cb)
	    return;

	cb = moz->print_status_cb;
	memset(&cbinfo, 0, sizeof(cbinfo));
	cbinfo.reason = Pt_CB_MOZ_PRINT_STATUS;
	cbinfo.cbdata = &pstatus;

	memset(&pstatus, 0, sizeof(PtMozillaPrintStatusCb_t));
	pstatus.status = status;
	pstatus.max = max;
	pstatus.cur = cur;
	PtInvokeCallbackList(cb, (PtWidget_t *)moz, &cbinfo);
}


NS_IMETHODIMP EmbedPrintListener::OnStateChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, PRUint32 aStateFlags, nsresult aStatus)
{
  if (aStateFlags & nsIWebProgressListener::STATE_START) {
	  InvokePrintCallback(Pt_MOZ_PRINT_START, 0, 0);

  } else if (aStateFlags & nsIWebProgressListener::STATE_STOP) {
	  InvokePrintCallback(Pt_MOZ_PRINT_COMPLETE, 0, 0);
  }
  return NS_OK;
}


NS_IMETHODIMP EmbedPrintListener::OnProgressChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, PRInt32 aCurSelfProgress, PRInt32 aMaxSelfProgress, PRInt32 aCurTotalProgress, PRInt32 aMaxTotalProgress)
{
	InvokePrintCallback(Pt_MOZ_PRINT_PROGRESS, aCurTotalProgress, aMaxTotalProgress);
  return NS_OK;
}


NS_IMETHODIMP EmbedPrintListener::OnLocationChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, nsIURI *location)
{
    return NS_OK;
}


NS_IMETHODIMP EmbedPrintListener::OnStatusChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, nsresult aStatus, const PRUnichar *aMessage)
{
    return NS_OK;
}


NS_IMETHODIMP EmbedPrintListener::OnSecurityChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, PRUint32 state)
{
    return NS_OK;
}
