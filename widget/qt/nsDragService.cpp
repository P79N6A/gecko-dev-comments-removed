






#include "qmimedata.h"
#include "qwidget.h"
#include "qapplication.h"
#include "qthread.h"

#include "nsDragService.h"
#include "nsISupportsPrimitives.h"
#include "nsXPIDLString.h"
#include "nsIDOMMouseEvent.h"

NS_IMPL_ADDREF_INHERITED(nsDragService, nsBaseDragService)
NS_IMPL_RELEASE_INHERITED(nsDragService, nsBaseDragService)
NS_IMPL_QUERY_INTERFACE2(nsDragService, nsIDragService, nsIDragSession )

nsDragService::nsDragService() : mDrag(nullptr), mHiddenWidget(nullptr)
{
}

nsDragService::~nsDragService()
{
    
    delete mHiddenWidget;
}

NS_IMETHODIMP
nsDragService::SetDropActionType( uint32_t aActionType )
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
                                uint32_t aActionType)
{
    uint32_t itemCount = 0;
    aTransferables->Count(&itemCount);
    if (0 == itemCount)
    {
        NS_WARNING("No items to drag?");
        return NS_ERROR_FAILURE;
    }

    if (1 != itemCount)
    {
        NS_WARNING("Dragging more than one item, cannot do (yet?)");
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
            uint32_t flavorCount;
            flavorList->Count( &flavorCount );

            for (uint32_t flavor=0; flavor < flavorCount; flavor++)
            {
                nsCOMPtr<nsISupports> genericWrapper;
                flavorList->GetElementAt(flavor, getter_AddRefs(genericWrapper));
                nsCOMPtr<nsISupportsCString> currentFlavor;
                currentFlavor = do_QueryInterface(genericWrapper);

                if (currentFlavor)
                {
                    nsCOMPtr<nsISupports> data;
                    uint32_t dataLen = 0;
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

    if (qApp->thread() != QThread::currentThread()) {
        NS_WARNING("Cannot initialize drag session in non main thread");
        return NS_OK;
    }

    if (!mHiddenWidget) {
      mHiddenWidget = new QWidget();
    }
    mDrag = new QDrag( mHiddenWidget ); 
    mDrag->setMimeData(mimeData);

    

    return NS_OK;
}


NS_IMETHODIMP
nsDragService::InvokeDragSession(
                                nsIDOMNode *aDOMNode,
                                nsISupportsArray *aTransferables,
                                nsIScriptableRegion *aRegion,
                                uint32_t aActionType)
{
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
    if (qApp->thread() == QThread::currentThread()) {
        mDrag->exec(mDropAction);
    }

    return NS_OK;
}


NS_IMETHODIMP
nsDragService::InvokeDragSessionWithImage(
                        nsIDOMNode *aDOMNode,
                        nsISupportsArray*aTransferables,
                        nsIScriptableRegion* aRegion,
                        uint32_t aActionType,
                        nsIDOMNode* aImage,
                        int32_t aImageX,
                        int32_t aImageY,
                        nsIDOMDragEvent* aDragEvent,
                        nsIDOMDataTransfer* aDataTransfer)
{
    nsBaseDragService::InvokeDragSessionWithImage(
                                        aDOMNode, aTransferables,
                                        aRegion, aActionType,
                                        aImage,
                                        aImageX, aImageY,
                                        aDragEvent,
                                        aDataTransfer);

    SetupDragSession( aTransferables, aActionType);

    
    if (aImage)
    {
        
        
        

        
        
        
        
        
        NS_WARNING("Support for drag image not implemented");
    }

    return ExecuteDrag();
}

NS_IMETHODIMP
nsDragService::InvokeDragSessionWithSelection(nsISelection* aSelection,
                                              nsISupportsArray* aTransferableArray,
                                              uint32_t aActionType,
                                              nsIDOMDragEvent* aDragEvent,
                                              nsIDOMDataTransfer* aDataTransfer)
{
    nsBaseDragService::InvokeDragSessionWithSelection(
                                        aSelection,
                                        aTransferableArray,
                                        aActionType,
                                        aDragEvent,
                                        aDataTransfer);

    SetupDragSession( aTransferableArray, aActionType);

    
    

    return ExecuteDrag();
}


NS_IMETHODIMP
nsDragService::GetCurrentSession(nsIDragSession **_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsDragService::StartDragSession()
{
    return nsBaseDragService::StartDragSession();
}


NS_IMETHODIMP
nsDragService::EndDragSession(bool aDoneDrag)
{
    return nsBaseDragService::EndDragSession(aDoneDrag);
}


NS_IMETHODIMP
nsDragService::FireDragEventAtSource(uint32_t aMsg)
{
    return nsBaseDragService::FireDragEventAtSource(aMsg);
}


NS_IMETHODIMP
nsDragService::Suppress()
{
    return nsBaseDragService::Suppress();
}


NS_IMETHODIMP
nsDragService::Unsuppress()
{
    return nsBaseDragService::Unsuppress();
}

NS_IMETHODIMP
nsDragService::DragMoved(int32_t aX, int32_t aY)
{
    return nsBaseDragService::DragMoved(aX, aY);
}
