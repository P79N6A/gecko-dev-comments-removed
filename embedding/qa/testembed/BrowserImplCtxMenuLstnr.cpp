





































#ifdef _WINDOWS
  #include "stdafx.h"
#endif
#include "BrowserImpl.h"
#include "IBrowserFrameGlue.h"

#include "QaUtils.h"





NS_IMETHODIMP CBrowserImpl::OnShowContextMenu(PRUint32 aContextFlags, nsIDOMEvent *aEvent, nsIDOMNode *aNode)
{
	QAOutput("nsIContextMenuListener::OnShowContextMenu()", 1);
	if(m_pBrowserFrameGlue)
		m_pBrowserFrameGlue->ShowContextMenu(aContextFlags, aNode);

	return NS_OK;
}