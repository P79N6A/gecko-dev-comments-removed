





































#include "nsDragService.h"
#include "nsISupportsPrimitives.h"
#include "nsXPIDLString.h"

#include "qmimedata.h"
#include "qwidget.h"

NS_IMPL_ADDREF_INHERITED(nsDragService, nsBaseDragService)
NS_IMPL_RELEASE_INHERITED(nsDragService, nsBaseDragService)
NS_IMPL_QUERY_INTERFACE2(nsDragService, nsIDragService, nsIDragSession )

nsDragService::nsDragService() : mDrag(NULL)
{
    
    qDebug("nsDragService::nsDragService");

    
    
    mHiddenWidget = new QWidget();
}

nsDragService::~nsDragService()
{
    
    qDebug("nsDragService::~nsDragService");

    delete mHiddenWidget;
    delete mDrag;
}

NS_IMETHODIMP
nsDragService::SetDropActionType( PRUint32 aActionType )
{
    mDropAction = Qt::IgnoreAction;

    if (aActionType & DRAGDROP_ACTION_COPY)
    {
        mDropAction |= Qt::CopyAction;
    }
    if (aActionType & DRAGDROP_ACTION_MOVE)
    {
        mDropAction |= Qt::MoveAction;
    }
    if (aActionType & DRAGDROP_ACTION_LINK)
    {
        mDropAction |= Qt::LinkAction;
    }

    return NS_OK;
}

NS_IMETHODIMP
nsDragService::SetupDragSession(
                                nsISupportsArray *aTransferables,
                                PRUint32 aActionType)
{
    PRUint32 itemCount = 0;
    aTransferables->Count(&itemCount);
    if (0 == itemCount)
    {
        qDebug("No items to drag?");
        return NS_ERROR_FAILURE;
    }

    if (1 != itemCount)
    {
        qDebug("Dragging more than one item, cannot do (yet?)");
        return NS_ERROR_NOT_IMPLEMENTED;
    }

    SetDropActionType(aActionType);

    QMimeData *mimeData = new QMimeData;

    nsCOMPtr<nsISupports> genericItem;
    aTransferables->GetElementAt(0, getter_AddRefs(genericItem));
    nsCOMPtr<nsITransferable> transferable(do_QueryInterface(genericItem));

    if (transferable)
    {
        nsCOMPtr <nsISupportsArray> flavorList;
        transferable->FlavorsTransferableCanExport(getter_AddRefs(flavorList));

        if (flavorList)
        {
            PRUint32 flavorCount;
            flavorList->Count( &flavorCount );

            for (PRUint32 flavor=0; flavor < flavorCount; flavor++)
            {
                nsCOMPtr<nsISupports> genericWrapper;
                flavorList->GetElementAt(flavor, getter_AddRefs(genericWrapper));
                nsCOMPtr<nsISupportsCString> currentFlavor;
                currentFlavor = do_QueryInterface(genericWrapper);

                if (currentFlavor)
                {
                    nsCOMPtr<nsISupports> data;
                    PRUint32 dataLen = 0;
                    nsXPIDLCString flavorStr;
                    currentFlavor->ToString(getter_Copies(flavorStr));


                    
                    if (!strcmp(kURLMime, flavorStr.get())
                     || !strcmp(kURLDataMime, flavorStr.get())
                     || !strcmp(kURLDescriptionMime, flavorStr.get())
                     || !strcmp(kHTMLMime, flavorStr.get())
                     || !strcmp(kUnicodeMime, flavorStr.get())
                        )
                    {
                        transferable->GetTransferData(flavorStr,getter_AddRefs(data),&dataLen);

                        nsCOMPtr<nsISupportsString> wideString;
                        wideString = do_QueryInterface(data);
                        if (!wideString)
                        {
                            return NS_ERROR_FAILURE;
                        }

                        nsAutoString utf16string;
                        wideString->GetData(utf16string);
                        QByteArray ba((const char*) utf16string.get(), dataLen);

                        mimeData->setData(flavorStr.get(), ba);
                    }
                }
            }
        }
    }

    qDebug("Creating a new QDrag object");

    mDrag = new QDrag( mHiddenWidget ); 
    mDrag->setMimeData(mimeData);

    

    return NS_OK;
}


NS_IMETHODIMP
nsDragService::InvokeDragSession(
                                nsIDOMNode *aDOMNode,
                                nsISupportsArray *aTransferables,
                                nsIScriptableRegion *aRegion,
                                PRUint32 aActionType)
{
    qDebug("nsDragService::InvokeDragSession");

    nsBaseDragService::InvokeDragSession( 
                                        aDOMNode,
                                        aTransferables,
                                        aRegion,
                                        aActionType);

    SetupDragSession( aTransferables, aActionType);

    return NS_OK;
}

NS_IMETHODIMP
nsDragService::ExecuteDrag()
{
    qDebug("Drag->exec()");
    Qt::DropAction dropAction = mDrag->exec( mDropAction );

    qDebug("Returned from drag->exec(), dropAction = %d", dropAction);

    return NS_OK;
}


NS_IMETHODIMP
nsDragService::InvokeDragSessionWithImage(
                        nsIDOMNode *aDOMNode,
                        nsISupportsArray*aTransferables,
                        nsIScriptableRegion* aRegion,
                        PRUint32 aActionType,
                        nsIDOMNode* aImage,
                        PRInt32 aImageX,
                        PRInt32 aImageY,
                        nsIDOMMouseEvent* aDragEvent)
{
    qDebug("nsDragService::InvokeDragSessionWithImage");

    nsBaseDragService::InvokeDragSessionWithImage(
                                        aDOMNode, aTransferables,
                                        aRegion, aActionType,
                                        aImage,
                                        aImageX, aImageY,
                                        aDragEvent );

    SetupDragSession( aTransferables, aActionType);

    
    if (aImage)
    {
        
        
        

        
        
        
        
        
        
        qDebug("Support for drag image not implemented");
    }

    return ExecuteDrag();
}


NS_IMETHODIMP
nsDragService::InvokeDragSessionWithSelection(nsISelection* aSelection, nsISupportsArray* aTransferables, PRUint32 aActionType, nsIDOMMouseEvent* aDragEvent)
{
    qDebug("nsDragService::InvokeDragSessionWithSelection");

    nsBaseDragService::InvokeDragSessionWithSelection(
                                        aSelection,
                                        aTransferables,
                                        aActionType,
                                        aDragEvent );

    SetupDragSession( aTransferables, aActionType);

    
    

    return ExecuteDrag();
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

    return nsBaseDragService::StartDragSession();

}


NS_IMETHODIMP
nsDragService::EndDragSession(PRBool aDoneDrag)
{
    qDebug("nsDragService::EndDragSession");

    return nsBaseDragService::EndDragSession(aDoneDrag);

}


NS_IMETHODIMP
nsDragService::FireDragEventAtSource(PRUint32 aMsg)
{
    qDebug("nsDragService::FireDragEventAtSource");

    
    return nsBaseDragService::FireDragEventAtSource(aMsg);

}


NS_IMETHODIMP
nsDragService::Suppress()
{
    qDebug("nsDragService::Suppress");

    return nsBaseDragService::Suppress();

}


NS_IMETHODIMP
nsDragService::Unsuppress()
{
    qDebug("nsDragService::Unsuppress");

    return nsBaseDragService::Unsuppress();

}

