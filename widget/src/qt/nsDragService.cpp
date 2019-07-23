








































#ifdef NDEBUG
#define NO_DEBUG
#endif

#include "nsDragService.h"
#include "nsIServiceManager.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "nsCOMPtr.h"
#include "nsXPIDLString.h"
#include "nsPrimitiveHelpers.h"
#include "nsMime.h"
#include "nsWidgetsCID.h"
#include "nsString.h"





NS_IMPL_ADDREF_INHERITED(nsDragService, nsBaseDragService)
NS_IMPL_RELEASE_INHERITED(nsDragService, nsBaseDragService)
NS_IMPL_QUERY_INTERFACE3(nsDragService, nsIDragService, nsIDragSession, nsIDragSessionQt )




static PRBool gHaveDrag = PR_FALSE;






nsDragService::nsDragService()
{
  
  mHiddenWidget = new QWidget(0,QWidget::tr("DragDrop"),0);
}






nsDragService::~nsDragService()
{
  delete mHiddenWidget;
}


NS_IMETHODIMP
nsDragService::InvokeDragSession(nsIDOMNode *aDOMNode,
                                 nsISupportsArray *aArrayTransferables,
                                 nsIScriptableRegion *aRegion,
                                 PRUint32 aActionType)
{
  PRUint32 numItemsToDrag = 0;

  nsBaseDragService::InvokeDragSession(aDOMNode, aArrayTransferables,
                                       aRegion, aActionType);

  
  if (!aArrayTransferables) {
    return NS_ERROR_INVALID_ARG;
  }
  
  
  
  mSourceDataItems = aArrayTransferables;

  mSourceDataItems->Count(&numItemsToDrag);
  if (!numItemsToDrag) {
    return NS_ERROR_FAILURE;
  }
  if (numItemsToDrag > 1) {
    return NS_ERROR_FAILURE;
  }
  nsCOMPtr<nsISupports> genericItem;

  mSourceDataItems->GetElementAt(0,getter_AddRefs(genericItem));

  nsCOMPtr<nsITransferable> transferable(do_QueryInterface(genericItem));

  mDragObject = RegisterDragFlavors(transferable);
  gHaveDrag = PR_TRUE;

  if (aActionType == DRAGDROP_ACTION_MOVE)
    mDragObject->dragMove();
  else
    mDragObject->dragCopy();

  gHaveDrag = PR_FALSE;
  mDragObject = 0;
  return NS_OK;
}

QDragObject *nsDragService::RegisterDragFlavors(nsITransferable *transferable)
{
    nsMimeStore *pMimeStore = new nsMimeStore();
    nsCOMPtr<nsISupportsArray> flavorList;

    if (NS_SUCCEEDED(transferable->FlavorsTransferableCanExport(getter_AddRefs(flavorList)))) {
        PRUint32 numFlavors;

        flavorList->Count(&numFlavors);

        for (PRUint32 flavorIndex = 0; flavorIndex < numFlavors; ++flavorIndex) {
            nsCOMPtr<nsISupports> genericWrapper;

            flavorList->GetElementAt(flavorIndex,getter_AddRefs(genericWrapper));

            nsCOMPtr<nsISupportsCString> currentFlavor(do_QueryInterface(genericWrapper));

            if (currentFlavor) {
                nsXPIDLCString flavorStr;

                currentFlavor->ToString(getter_Copies(flavorStr));

                PRUint32   len;
                nsCOMPtr<nsISupports> clip;

                transferable->GetTransferData(flavorStr,getter_AddRefs(clip),&len);

                nsCOMPtr<nsISupportsString> wideString;
                wideString = do_QueryInterface(clip);
                if (!wideString)
                    continue;

                nsAutoString ucs2string;
                wideString->GetData(ucs2string);
                QString str = QString::fromUcs2(ucs2string.get());

                pMimeStore->AddFlavorData(flavorStr,str.utf8());
            }
        } 
    } 
#ifdef NS_DEBUG
    else
        printf(" DnD ERROR: cannot export any flavor\n");
#endif
    return new nsDragObject(pMimeStore,mHiddenWidget);
} 

NS_IMETHODIMP nsDragService::StartDragSession()
{
#ifdef NS_DEBUG
  printf(" DnD: StartDragSession\n");
#endif
  return nsBaseDragService::StartDragSession();
}

NS_IMETHODIMP nsDragService::EndDragSession()
{
#ifdef NS_DEBUG
  printf(" DnD: EndDragSession\n");
#endif
  mDragObject = 0;
  return nsBaseDragService::EndDragSession();
}


NS_IMETHODIMP nsDragService::SetCanDrop(PRBool aCanDrop)
{
  mCanDrop = aCanDrop;
  return NS_OK;
}

NS_IMETHODIMP nsDragService::GetCanDrop(PRBool *aCanDrop)
{
  *aCanDrop = mCanDrop;
  return NS_OK;
}

NS_IMETHODIMP nsDragService::GetNumDropItems(PRUint32 *aNumItems)
{
  *aNumItems = 1;
  return NS_OK;
}

NS_IMETHODIMP nsDragService::GetData(nsITransferable *aTransferable,
                                     PRUint32 aItemIndex)
{
  
  if (!aTransferable)
    return NS_ERROR_INVALID_ARG;

  nsresult rv = NS_ERROR_FAILURE;
  nsCOMPtr<nsISupportsArray> flavorList;

  rv = aTransferable->FlavorsTransferableCanImport(getter_AddRefs(flavorList));
  if (NS_FAILED(rv))
    return rv;

  
  PRUint32 cnt;

  flavorList->Count(&cnt);

  
  
  
  for (unsigned int i = 0; i < cnt; ++i) {
    nsCAutoString foundFlavor;
    nsCOMPtr<nsISupports> genericWrapper;

    flavorList->GetElementAt(i,getter_AddRefs(genericWrapper));

    nsCOMPtr<nsISupportsCString> currentFlavor;

    currentFlavor = do_QueryInterface(genericWrapper);
    if (currentFlavor) {
      nsXPIDLCString flavorStr;

      currentFlavor->ToString(getter_Copies(flavorStr));
      foundFlavor = nsCAutoString(flavorStr);

      if (mDragObject && mDragObject->provides(flavorStr)) {
	QByteArray ba = mDragObject->encodedData((const char*)flavorStr);
        nsCOMPtr<nsISupports> genericDataWrapper;
	PRUint32 len = (PRUint32)ba.count();

        nsPrimitiveHelpers::CreatePrimitiveForData(foundFlavor.get(),
 						   (void*)ba.data(),len,
                                                   getter_AddRefs(genericDataWrapper));

        aTransferable->SetTransferData(foundFlavor.get(),genericDataWrapper,len);
      }
    }
  }
  return NS_OK;
}

NS_IMETHODIMP nsDragService::IsDataFlavorSupported(const char *aDataFlavor,
                                                   PRBool *_retval)
{
  if (!_retval)
    return NS_ERROR_INVALID_ARG;

  *_retval = PR_FALSE;

  if (mDragObject)
     *_retval = mDragObject->provides(aDataFlavor);

#ifdef NS_DEBUG
  if (!*_retval)
    printf("nsDragService::IsDataFlavorSupported not provides [%s] \n", aDataFlavor);
#endif
  return NS_OK;
}

NS_IMETHODIMP nsDragService::SetDragReference(QMimeSource* aDragRef)
{
   nsMimeStore*  pMimeStore = new nsMimeStore();
   int c = 0;
   const char* format;

   while ((format = aDragRef->format(c++)) != 0) {
     
     

     QByteArray ba = aDragRef->encodedData(format);
     pMimeStore->AddFlavorData(format,ba);
   }
   mDragObject = new nsDragObject(pMimeStore,mHiddenWidget);
   return NS_OK;
}
