





































#include "nsDragService.h"

NS_IMPL_ISUPPORTS1(nsDragService, nsIDragService)

nsDragService::nsDragService()
{
    
    qDebug("nsDragService::nsDragService");
}

nsDragService::~nsDragService()
{
    
    qDebug("nsDragService::~nsDragService");
}


NS_IMETHODIMP
nsDragService::InvokeDragSession(nsIDOMNode *aDOMNode, nsISupportsArray *aTransferables, nsIScriptableRegion *aRegion, PRUint32 aActionType)
{
    qDebug("nsDragService::InvokeDragSession");

    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsDragService::InvokeDragSessionWithImage(nsIDOMNode *aDOMNode, nsISupportsArray*aTransferables, nsIScriptableRegion* aRegion, PRUint32 aActionType, nsIDOMNode* aImage, PRInt32 aImageX, PRInt32 aImageY, nsIDOMMouseEvent* aDragEvent)
{
    qDebug("nsDragService::InvokeDragSessionWithImage");

    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsDragService::InvokeDragSessionWithSelection(nsISelection* aSelection, nsISupportsArray* aTransferables, PRUint32 aActionType, nsIDOMMouseEvent* aDragEvent)
{
    qDebug("nsDragService::InvokeDragSessionWithSelection");

    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsDragService::GetCurrentSession(nsIDragSession **_retval)
{
    qDebug("nsDragService::GetCurrentSession");

    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsDragService::StartDragSession()
{
    qDebug("nsDragService::StartDragSession");

    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsDragService::EndDragSession(PRBool aDoneDrag)
{
    qDebug("nsDragService::EndDragSession");

    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsDragService::FireDragEventAtSource(PRUint32 aMsg)
{
    qDebug("nsDragService::FireDragEventAtSource");

    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDragService::Suppress()
{
    qDebug("nsDragService::Suppress");

    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDragService::Unsuppress()
{
    qDebug("nsDragService::Unsuppress");

    return NS_ERROR_NOT_IMPLEMENTED;
}

