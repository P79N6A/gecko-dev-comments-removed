







































#include <stdio.h>
#include "nsDragService.h"
#include "nsIDocument.h"
#include "nsIRegion.h"
#include "nsITransferable.h"
#include "nsIServiceManager.h"
#include "nsISupportsPrimitives.h"
#include "nsVoidArray.h"
#include "nsXPIDLString.h"
#include "nsPrimitiveHelpers.h"
#include "nsUnitConversion.h"
#include "nsWidgetsCID.h"
#include "nsCRT.h"










#include <AppDefs.h>
#include <TypeConstants.h>
#include <DataIO.h>
#include <Mime.h>
#include <Rect.h>
#include <Region.h>
#include <String.h>
#include <View.h>

#include "prlog.h"
#include "nsIPresShell.h"
#include "nsPresContext.h"
#include "nsIFrame.h"
#include "nsIView.h"
#include "nsIWidget.h"
  
static NS_DEFINE_CID(kCDragServiceCID,   NS_DRAGSERVICE_CID);
  
static PRLogModuleInfo *sDragLm = NULL;

static nsIFrame*
GetPrimaryFrameFor(nsIDOMNode *aDOMNode)
{
    nsCOMPtr<nsIContent> aContent = do_QueryInterface(aDOMNode);
    if (nsnull == aContent)
        return nsnull;

    nsIDocument* doc = aContent->GetCurrentDoc();
    if (nsnull == doc)
        return nsnull;
    nsIPresShell* presShell = doc->GetShellAt(0);
    if ( nsnull == presShell) 
        return nsnull;
    return presShell->GetPrimaryFrameFor(aContent);
}

static bool 
IsInternalDrag(BMessage * aMsg)
{
    BString orig;
    
    return (nsnull != aMsg && B_OK == aMsg->FindString("be:originator", &orig) &&
	    0 == orig.Compare("BeZilla"));
}

NS_IMPL_THREADSAFE_ISUPPORTS3(nsDragService, nsIDragService, nsIDragSession, nsIDragSessionBeOS)







nsDragService::nsDragService()
{
    
    if (!sDragLm)
        sDragLm = PR_NewLogModule("nsDragService");
    PR_LOG(sDragLm, PR_LOG_DEBUG, ("\n\nnsDragService::nsDragService"));

    mDragMessage = NULL;
    mCanDrop = PR_FALSE;
}






nsDragService::~nsDragService()
{
    PR_LOG(sDragLm, PR_LOG_DEBUG, ("nsDragService::~nsDragService"));
    ResetDragInfo();
}










NS_IMETHODIMP
nsDragService::InvokeDragSession (nsIDOMNode *aDOMNode,
                                  nsISupportsArray * aArrayTransferables,
                                  nsIScriptableRegion * aRegion,
                                  PRUint32 aActionType)
{
    PR_LOG(sDragLm, PR_LOG_DEBUG, ("nsDragService::InvokeDragSession"));
    nsBaseDragService::InvokeDragSession (aDOMNode, aArrayTransferables,
                                         aRegion, aActionType);
    ResetDragInfo();       
    
    if (nsnull == aArrayTransferables)
        return NS_ERROR_INVALID_ARG;

    
    
    
    mSourceDataItems = aArrayTransferables;

    
    bool haveRect = false;
    BRect dragRect;
	
    if (nsnull != aRegion)
    {
        PRInt32 aX, aY, aWidth, aHeight;
        
        aRegion->GetBoundingBox(&aX, &aY, &aWidth, &aHeight);
        dragRect.Set( aX, aY, aX + aWidth, aY + aHeight);
        haveRect = true;
        
    } 
    
    
    nsIFrame *aFrame = GetPrimaryFrameFor(aDOMNode);
    if (nsnull == aFrame)
        return PR_FALSE;
    
    
    
    nsRect aRect = aFrame->GetRect();

    
    nsIView *containingView = nsnull;
    nsPoint viewOffset(0,0);
    aFrame->GetOffsetFromView(viewOffset, &containingView);
    NS_ASSERTION(containingView, "No containing view!");
    if (nsnull == containingView)
        return PR_FALSE;

    
    nsPoint aWidgetOffset;
    nsCOMPtr<nsIWidget> aWidget = containingView->GetNearestWidget(&aWidgetOffset);
    if (nsnull == aWidget)
        return PR_FALSE;

    BView *view = (BView *) aWidget->GetNativeData(NS_NATIVE_WIDGET);
    
    if (nsnull==haveRect)
    {
        float t2p =  aFrame->GetPresContext()->TwipsToPixels(); 
        
        
        nsPoint aViewPos = containingView->GetPosition();
    
        
        
        
        
        nsRect screenOffset;
        screenOffset.MoveBy ( NSTwipsToIntPixels(aWidgetOffset.x + aViewPos.x + viewOffset.x, t2p),
                            NSTwipsToIntPixels(aWidgetOffset.y + aViewPos.y + viewOffset.y, t2p));
        aWidget->WidgetToScreen ( screenOffset, screenOffset );

        dragRect.Set(screenOffset.x, screenOffset.y, 
		             screenOffset.x + NSTwipsToIntPixels(aRect.width, t2p),
		             screenOffset.y + NSTwipsToIntPixels(aRect.height, t2p));
        haveRect = true;
    }

    mDragAction = aActionType;
    mDragMessage = CreateDragMessage();

    if (!view || !mDragMessage)
        return PR_FALSE;
        
    
    
    
    if (!view->LockLooper())
        return PR_FALSE;
        
    
    if (!haveRect) 
    {
        dragRect = view->Frame();
        
    }
        
    PR_LOG(sDragLm, PR_LOG_DEBUG, ("invoking mDragView->DragMessage"));
    bool noBitmap = true;


# ifdef 0
    do
    {
        PRUint32 dataSize;
        PRUint32 noItems;
        mSourceDataItems->Count(&noItems);
        if (noItems!=1) 
        {
            PR_LOG(sDragLm, PR_LOG_DEBUG, ("Transferables are not ==1, no drag bitmap!"));
            break;
        }
        
        nsCOMPtr<nsISupports> genericItem;
        aArrayTransferables->GetElementAt(0, getter_AddRefs(genericItem));
        nsCOMPtr<nsITransferable> aTransferable (do_QueryInterface(genericItem));
        
        nsCOMPtr<nsISupports> genericDataWrapper;
        nsresult rv = aTransferable->GetTransferData(kNativeImageMime, getter_AddRefs(genericDataWrapper), &dataSize);
        if (NS_FAILED(rv))
        {
            PR_LOG(sDragLm, PR_LOG_DEBUG, ("Could not get nativeimage, no drag bitmap!"));
            break;
        }

        nsCOMPtr<nsISupportsInterfacePointer> ptrPrimitive (do_QueryInterface(genericDataWrapper));
        if (ptrPrimitive == NULL) 
        {
            PR_LOG(sDragLm, PR_LOG_DEBUG, ("Could not get ptrPrimitive, no drag bitmap!"));
            break;
        }

        nsCOMPtr<nsISupports> genericData;
        ptrPrimitive->GetData(getter_AddRefs(genericData));
        if (genericData == NULL) 
        {
            PR_LOG(sDragLm, PR_LOG_DEBUG, ("Could not get data, no drag bitmap!"));
            break;
        }

        
        nsCOMPtr<nsIImageBeOS> image (do_QueryInterface(genericData));
        if (image == NULL)
        {
            PR_LOG(sDragLm, PR_LOG_DEBUG, ("Could not get nsImage, no drag bitmap!"));
            break;
        }

        BBitmap *aBitmap;
        image->GetBitmap(&aBitmap);
        if (aBitmap==NULL || !aBitmap->IsValid())
        {
            PR_LOG(sDragLm, PR_LOG_DEBUG, ("Could not get BBitmap, no drag bitmap %s!", aBitmap==NULL?"(null)":"(not valid)" ));
            break;        
        }

        view->DragMessage(mDragMessage, aBitmap, B_OP_OVER, BPoint(-4,-4), view); 
        noBitmap = false;
    } while(false);
# endif    
    
    if (noBitmap) 
        view->DragMessage(mDragMessage, dragRect, view);
    
    StartDragSession();
    view->UnlockLooper();
    return NS_OK;
}








NS_IMETHODIMP
nsDragService::StartDragSession()
{
    PR_LOG(sDragLm, PR_LOG_DEBUG, ("nsDragService::StartDragSession()"));
    return nsBaseDragService::StartDragSession();
}








NS_IMETHODIMP
nsDragService::EndDragSession()
{
    PR_LOG(sDragLm, PR_LOG_DEBUG, ("nsDragService::EndDragSession()"));
    
    
    
    
    
    
    return nsBaseDragService::EndDragSession();
}








NS_IMETHODIMP
nsDragService::SetCanDrop(PRBool aCanDrop)
{
    PR_LOG(sDragLm, PR_LOG_DEBUG, ("nsDragService::SetCanDrop(%s)",
                                  aCanDrop == PR_TRUE?"TRUE":"FALSE"));
    return nsBaseDragService::SetCanDrop(aCanDrop);
}









NS_IMETHODIMP
nsDragService::GetCanDrop(PRBool *aCanDrop)
{
    PR_LOG(sDragLm, PR_LOG_DEBUG, ("nsDragService::GetCanDrop()"));
    return nsBaseDragService::GetCanDrop(aCanDrop);
}








NS_IMETHODIMP
nsDragService::GetNumDropItems(PRUint32 * aNumItems)
{
    PR_LOG(sDragLm, PR_LOG_DEBUG, ("nsDragService::GetNumDropItems()"));
    if (nsnull == mDragMessage)
    {
        *aNumItems = 0;
        PR_LOG(sDragLm, PR_LOG_DEBUG, ("nsDragService::GetNumDropItems(): WARNING! No dragmessage"));
        return NS_OK;
    } 
    
    if (IsInternalDrag(mDragMessage))
        mSourceDataItems->Count(aNumItems);
    else
        
        
        *aNumItems = 1;


    PR_LOG(sDragLm, PR_LOG_DEBUG, ("nsDragService::GetNumDropItems():%d", *aNumItems));
    return NS_OK;
}











NS_IMETHODIMP
nsDragService::GetData(nsITransferable * aTransferable, PRUint32 aItemIndex)
{
    PR_LOG(sDragLm, PR_LOG_DEBUG, ("nsDragService::GetData %d", aItemIndex));

    if (nsnull==mDragMessage)
        return NS_ERROR_INVALID_ARG;

    
    
    
    nsresult rv = NS_ERROR_FAILURE;
    nsCOMPtr<nsISupportsArray> flavorList;
    rv = aTransferable->FlavorsTransferableCanImport(getter_AddRefs(flavorList));
    if (NS_FAILED(rv))
        return rv;

    
    PRUint32 cnt;
    flavorList->Count (&cnt);

    nsCOMPtr<nsISupports> genericWrapper;
    nsCOMPtr<nsISupportsCString> currentFlavor;
    nsXPIDLCString flavorStr;
    nsCOMPtr<nsISupports> genericItem;   
    nsCOMPtr<nsISupports> data;
    PRUint32 tmpDataLen = 0;
    for (unsigned int i= 0; i < cnt; ++i )
    {
        flavorList->GetElementAt(i, getter_AddRefs(genericWrapper));
        currentFlavor = do_QueryInterface(genericWrapper);
        if (!currentFlavor)
            continue;
        currentFlavor->ToString(getter_Copies(flavorStr));
        
        PR_LOG(sDragLm, PR_LOG_DEBUG, ("tnsDragService::GetData trying to get transfer data for %s",
                        (const char *)flavorStr));
                        
        if (IsInternalDrag(mDragMessage))
        {
            mSourceDataItems->GetElementAt(aItemIndex, getter_AddRefs(genericItem));
            nsCOMPtr<nsITransferable> item (do_QueryInterface(genericItem));
            if (!item)
                continue;
            rv = item->GetTransferData(flavorStr, getter_AddRefs(data), &tmpDataLen);
            if (NS_FAILED(rv))
                continue;
            PR_LOG(sDragLm, PR_LOG_DEBUG, ("tnsDragService::GetData setting data."));
            return aTransferable->SetTransferData(flavorStr, data, tmpDataLen);
        } 
        else
        {
            
            
            
        	 
        }
    }
    PR_LOG(sDragLm, PR_LOG_DEBUG, ("tnsDragService::GetData failed"));
    return NS_ERROR_FAILURE;
}











NS_IMETHODIMP
nsDragService::IsDataFlavorSupported (const char *aDataFlavor,
                                      PRBool *_retval)
{
    PR_LOG(sDragLm, PR_LOG_DEBUG, ("nsDragService::IsDataFlavorSupported %s", aDataFlavor));
    if (!_retval)
        return NS_ERROR_INVALID_ARG;

    
    *_retval = PR_FALSE;

    
    if (nsnull == mDragMessage)
    {
        PR_LOG(sDragLm, PR_LOG_DEBUG, ("*** warning: IsDataFlavorSupported called without a valid drag context!"));
        return NS_OK;
    }

    if (IsInternalDrag(mDragMessage))
    {
        PRUint32 numDragItems = 0;
        
        
        if (nsnull == mSourceDataItems)
            return NS_OK;
        mSourceDataItems->Count(&numDragItems);
        if (0 == numDragItems)
            PR_LOG(sDragLm, PR_LOG_DEBUG, ("*** warning: Number of dragged items is zero!"));

        
        nsCOMPtr<nsISupports> genericItem;
        nsCOMPtr <nsISupportsArray> flavorList;
        PRUint32 numFlavors;
        for (PRUint32 itemIndex = 0; itemIndex < numDragItems; ++itemIndex)
        {
            mSourceDataItems->GetElementAt(itemIndex, getter_AddRefs(genericItem));
            nsCOMPtr<nsITransferable> currItem (do_QueryInterface(genericItem));
            if (nsnull == currItem)
                continue;
            currItem->FlavorsTransferableCanExport(getter_AddRefs(flavorList));
            if (nsnull == flavorList)
                continue;
            flavorList->Count( &numFlavors );
            
            nsCOMPtr<nsISupports> genericWrapper;
            nsXPIDLCString flavorStr;
            for ( PRUint32 flavorIndex = 0; flavorIndex < numFlavors ; ++flavorIndex )
            {
                flavorList->GetElementAt (flavorIndex, getter_AddRefs(genericWrapper));
                nsCOMPtr<nsISupportsCString> currentFlavor = do_QueryInterface(genericWrapper);
                if (nsnull == currentFlavor)
                    continue;
                currentFlavor->ToString ( getter_Copies(flavorStr) );
                PR_LOG(sDragLm, PR_LOG_DEBUG, ("nsDragService::IsDataFlavorSupported checking %s against %s", (const char *)flavorStr, aDataFlavor));
                if (0 != strcmp(flavorStr, aDataFlavor))
                    continue;
                PR_LOG(sDragLm, PR_LOG_DEBUG, ("nsDragService::IsDataFlavorSupported Got the flavor!"));
                *_retval = PR_TRUE;
                return NS_OK;
            }
        }
    }
    else 
    {
        PR_LOG(sDragLm, PR_LOG_DEBUG, ("*** warning: Native drag not implemented."));
        
    }
    PR_LOG(sDragLm, PR_LOG_DEBUG, ("nsDragService::IsDataFlavorSupported FALSE"));
    return NS_OK;
}








BMessage *
nsDragService::CreateDragMessage()
{
    PR_LOG(sDragLm, PR_LOG_DEBUG, ("nsDragService::GetInitialDragMessage"));
    if (nsnull == mSourceDataItems)
        return NULL;
    
    unsigned int numDragItems = 0;
    mSourceDataItems->Count(&numDragItems);
    
    BMessage * returnMsg = new BMessage(B_SIMPLE_DATA);
    
    returnMsg->AddString("be:originator", "BeZilla");
    returnMsg->AddString("be:clip_name","BeZilla Drag Item");
  
    if (mDragAction & DRAGDROP_ACTION_COPY)
        returnMsg->AddInt32("be:actions",B_COPY_TARGET);
    if (mDragAction & DRAGDROP_ACTION_MOVE)
        returnMsg->AddInt32("be:actions",B_MOVE_TARGET);
    if (mDragAction & DRAGDROP_ACTION_LINK)
        returnMsg->AddInt32("be:actions",B_LINK_TARGET);
  
    
    
    if (numDragItems > 1)
    {
        PR_LOG(sDragLm, PR_LOG_DEBUG, ("nsDragService:: Dragging a list of items ..."));




        return returnMsg;
    }

    PRBool addedType = PR_FALSE;

    nsCOMPtr<nsISupports> genericItem;
    nsCOMPtr <nsISupportsArray> flavorList;
    PRUint32 numFlavors;
    nsCOMPtr<nsISupports> genericWrapper;
    nsXPIDLCString flavorStr;

    for (unsigned int itemIndex = 0; itemIndex < numDragItems; ++itemIndex)
    {
        mSourceDataItems->GetElementAt(itemIndex, getter_AddRefs(genericItem));
        nsCOMPtr<nsITransferable> currItem (do_QueryInterface(genericItem));
        if (nsnull == currItem) 
            continue;

        currItem->FlavorsTransferableCanExport(getter_AddRefs(flavorList));
        if (nsnull == flavorList)
            continue;
        flavorList->Count( &numFlavors );
        for (PRUint32 flavorIndex = 0; flavorIndex < numFlavors ; ++flavorIndex )
        {
            flavorList->GetElementAt(flavorIndex, getter_AddRefs(genericWrapper));
            nsCOMPtr<nsISupportsCString> currentFlavor = do_QueryInterface(genericWrapper);
            if (nsnull == currentFlavor)
                continue;
            currentFlavor->ToString ( getter_Copies(flavorStr) );
            
            PR_LOG(sDragLm, PR_LOG_DEBUG, ("nsDragService:: Adding a flavor to our message: %s",flavorStr.get()));
            
            type_code aCode;
            if (B_OK == returnMsg->GetInfo(flavorStr.get(), &aCode))
                continue;
            returnMsg->AddString("be:types",flavorStr.get());
            
            
            returnMsg->AddString("be:filetypes",flavorStr.get());
            returnMsg->AddString("be:type_descriptions",flavorStr.get());
            
            addedType = PR_TRUE;            
            
            
            
            
            if (0 == strcmp(flavorStr, kUnicodeMime))
            {
                PR_LOG(sDragLm, PR_LOG_DEBUG, ("nsDragService:: Adding a TextMime for the UnicodeMime"));
                returnMsg->AddString("be:types",kTextMime);
                
                returnMsg->AddString("be:filetypes",kTextMime);
                returnMsg->AddString("be:type_descriptions",kTextMime);
            }
        }
    }
    
    if (addedType)
    {
        returnMsg->AddString("be:types", B_FILE_MIME_TYPE);
    }
    returnMsg->PrintToStream();
    
    NS_ASSERTION(addedType == PR_TRUE, "No flavor/mime in the drag message!");
    return returnMsg;
}









NS_IMETHODIMP
nsDragService::UpdateDragMessageIfNeeded(BMessage *aDragMessage)
{
    PR_LOG(sDragLm, PR_LOG_DEBUG, ("nsDragService::UpdateDragMessageIfNeeded()"));
    if (aDragMessage == mDragMessage) 
        return NS_OK;
    
    
    
    
    if (IsInternalDrag(aDragMessage))
        return NS_OK;

    PR_LOG(sDragLm, PR_LOG_DEBUG, ("updating."));
    ResetDragInfo();
    mDragMessage = aDragMessage;
    return NS_OK;
}












NS_IMETHODIMP
nsDragService::TransmitData(BMessage *aNegotiationReply)
{
    PR_LOG(sDragLm, PR_LOG_DEBUG, ("nsDragService::TransmitData()"));






















  
    aNegotiationReply->PrintToStream();
    delete aNegotiationReply;
    return NS_OK;
}








void
nsDragService::ResetDragInfo()
{
    PR_LOG(sDragLm, PR_LOG_DEBUG, ("nsDragService::ResetDragInfo()"));
    if (nsnull != mDragMessage) delete mDragMessage;
    mDragMessage = NULL;
    mSourceDataItems = NULL;
}

const char *
nsDragService::FlavorToBeMime(const char * flavor)
{
    
    
    if (0 == strcmp(flavor,kUnicodeMime)) return kTextMime;    
    
    
    
    
    if (0 == strcmp(flavor,kJPEGImageMime)) return "image/jpeg";
    
    
    if (0 == strcmp(flavor,kFileMime)) return "application/octet-stream";
    
    if (0 == strcmp(flavor,kURLMime)) return "application/x-vnd.Be-bookmark";
    
    
    
    
    
    
    
    
    

}
