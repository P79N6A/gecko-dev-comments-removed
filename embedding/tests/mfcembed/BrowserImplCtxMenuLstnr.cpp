






























#ifdef _WINDOWS
  #include "stdafx.h"
#endif
#include "BrowserImpl.h"
#include "IBrowserFrameGlue.h"





NS_IMETHODIMP CBrowserImpl::OnShowContextMenu(PRUint32 aContextFlags, nsIContextMenuInfo *aInfo)
{
    if(m_pBrowserFrameGlue)
        m_pBrowserFrameGlue->ShowContextMenu(aContextFlags, aInfo);

    return NS_OK;
}