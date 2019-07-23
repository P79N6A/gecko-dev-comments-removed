





































#include "EmbedWindowCreator.h"
#include "EmbedPrivate.h"
#include "EmbedWindow.h"


#include "PtMozilla.h"

EmbedWindowCreator::EmbedWindowCreator(void)
{
	NS_INIT_ISUPPORTS();
}

EmbedWindowCreator::~EmbedWindowCreator()
{
}

NS_IMPL_ISUPPORTS1(EmbedWindowCreator, nsIWindowCreator)

NS_IMETHODIMP
EmbedWindowCreator::CreateChromeWindow(nsIWebBrowserChrome *aParent,
				       PRUint32 aChromeFlags,
				       nsIWebBrowserChrome **_retval)
{
	NS_ENSURE_ARG_POINTER(_retval);

	EmbedPrivate 			*embedPrivate = EmbedPrivate::FindPrivateForBrowser(aParent);
	PtMozillaWidget_t       *nmoz, *moz;
	PtCallbackList_t        *cb;
	PtCallbackInfo_t        cbinfo;
	PtMozillaNewWindowCb_t  nwin;

	if (!embedPrivate)
		return NS_ERROR_FAILURE;

	moz = (PtMozillaWidget_t *)embedPrivate->mOwningWidget;
	if (!moz || !moz->new_window_cb)
		return NS_ERROR_FAILURE;

	memset(&cbinfo, 0, sizeof(cbinfo));
	cbinfo.cbdata = &nwin;
	cbinfo.reason = Pt_CB_MOZ_NEW_WINDOW;
	cb = moz->new_window_cb;
	nwin.window_flags = aChromeFlags;

	
	nwin.window_size.w = nwin.window_size.h = 0;

	PtSetParentWidget(NULL);
	if (PtInvokeCallbackList(cb, (PtWidget_t *) moz, &cbinfo) == Pt_CONTINUE)
	{
	nmoz = (PtMozillaWidget_t *) nwin.widget;
		EmbedPrivate *newEmbedPrivate = nmoz->EmbedRef;

		if (aChromeFlags & nsIWebBrowserChrome::CHROME_OPENAS_CHROME)
			newEmbedPrivate->mIsChrome = PR_TRUE;

		*_retval = static_cast<nsIWebBrowserChrome *>((newEmbedPrivate->mWindow));

		if (*_retval) 
		{
			NS_ADDREF(*_retval);
			return NS_OK;
		}
	}

	return NS_ERROR_FAILURE;
}
