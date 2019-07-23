





































#ifdef _WINDOWS
  #include "stdafx.h"
#endif
#include "BrowserImpl.h"
#include "IBrowserFrameGlue.h"

#include "TestEmbed.h"
#include "BrowserView.h"
#include "BrowserFrm.h"

#include "QaUtils.h"













class CBrowserView;






NS_IMETHODIMP CBrowserImpl::OnHistoryNewEntry(nsIURI *theURI)
{
	QAOutput("nsIHistoryListener::OnHistoryNewEntry()", 2);
    GetTheURI(theURI, 1);
	return NS_OK;
}


NS_IMETHODIMP CBrowserImpl::OnHistoryGoBack(nsIURI *theURI, PRBool *notify)
{
	QAOutput("nsIHistoryListener::OnHistoryGoBack()", 2);

    GetTheURI(theURI, 1);
	*notify = PR_TRUE;
	FormatAndPrintOutput("OnHistoryGoBack() notification = ", *notify, 1);

	return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::OnHistoryGoForward(nsIURI *theURI, PRBool *notify)
{
	QAOutput("nsIHistoryListener::OnHistoryGoForward()", 2);

	GetTheURI(theURI, 1);
	*notify = PR_TRUE;
	FormatAndPrintOutput("OnHistoryGoForward() notification = ", *notify, 1);

	return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::OnHistoryReload(nsIURI *theURI, PRUint32 reloadFlags, PRBool *notify)
{
	char flagString[200];

	QAOutput("nsIHistoryListener::OnHistoryReload()", 2);

	GetTheURI(theURI, 1);
	*notify = PR_TRUE;
	FormatAndPrintOutput("OnHistoryReload() notification = ", *notify, 1);

	if (reloadFlags == 0x0000)
		strcpy(flagString, "LOAD_FLAGS_NONE");
	else if (reloadFlags == 0x0010)
		strcpy(flagString, "LOAD_FLAGS_IS_REFRESH");
	else if (reloadFlags == 0x0020)
		strcpy(flagString, "LOAD_FLAGS_IS_LINK");
	else if (reloadFlags == 0x0040)
		strcpy(flagString, "LOAD_FLAGS_BYPASS_HISTORY");
	else if (reloadFlags == 0x0080)
		strcpy(flagString, "LOAD_FLAGS_REPLACE_HISTORY");
	else if (reloadFlags == 0x0100)
		strcpy(flagString, "LOAD_FLAGS_BYPASS_CACHE");
	else if (reloadFlags == 0x0200)
		strcpy(flagString, "LOAD_FLAGS_BYPASS_PROXY");
	else if (reloadFlags == 0x0400)
		strcpy(flagString, "LOAD_FLAGS_CHARSET_CHANGE");
	else if (reloadFlags == (0x0100 | 0x0200))
		strcpy(flagString, "LOAD_FLAGS_BYPASS_CACHE | LOAD_FLAGS_BYPASS_PROXY");

	FormatAndPrintOutput("OnHistoryReload() flag value = ", reloadFlags, 1);
	FormatAndPrintOutput("OnHistoryReload() flag string = ", flagString, 2);

	return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::OnHistoryGotoIndex(PRInt32 theIndex, nsIURI *theURI, PRBool *notify)
{
	QAOutput("nsIHistoryListener::OnHistoryGotoIndex()", 2);

    GetTheURI(theURI, 1);
	*notify = PR_TRUE;
	FormatAndPrintOutput("OnHistoryGotoIndex() notification = ", *notify, 1);

	FormatAndPrintOutput("OnHistoryGotoIndex() index = ", theIndex, 2);

	return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::OnHistoryPurge(PRInt32 theNumEntries, PRBool *notify)
{
	QAOutput("nsIHistoryListener::OnHistoryPurge()", 2);

	*notify = PR_TRUE;
	FormatAndPrintOutput("OnHistoryPurge() notification = ", *notify, 1);

	FormatAndPrintOutput("OnHistoryPurge() theNumEntries = ", theNumEntries, 2);

	return NS_OK;
}

