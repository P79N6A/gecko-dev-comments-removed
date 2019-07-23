







































#include <ole2.h>
#include <oleidl.h>
#include <shlobj.h>
#include <shlwapi.h>


#include <shellapi.h>

#include "nsDragService.h"
#include "nsITransferable.h"
#include "nsDataObj.h"

#include "nsWidgetsCID.h"
#include "nsNativeDragTarget.h"
#include "nsNativeDragSource.h"
#include "nsClipboard.h"
#include "nsISupportsArray.h"
#include "nsIDocument.h"
#include "nsDataObjCollection.h"

#include "nsAutoPtr.h"

#include "nsString.h"
#include "nsEscape.h"
#include "nsISupportsPrimitives.h"
#include "nsNetUtil.h"
#include "nsIURL.h"
#include "nsCWebBrowserPersist.h"
#include "nsToolkit.h"
#include "nsCRT.h"
#include "nsDirectoryServiceDefs.h"
#include "nsUnicharUtils.h"






nsDragService::nsDragService()
  : mNativeDragSrc(nsnull), mNativeDragTarget(nsnull), mDataObject(nsnull)
{
}






nsDragService::~nsDragService()
{
  NS_IF_RELEASE(mNativeDragSrc);
  NS_IF_RELEASE(mNativeDragTarget);
  NS_IF_RELEASE(mDataObject);
}



NS_IMETHODIMP
nsDragService::InvokeDragSession(nsIDOMNode *aDOMNode,
                                 nsISupportsArray *anArrayTransferables,
                                 nsIScriptableRegion *aRegion,
                                 PRUint32 aActionType)
{
  nsBaseDragService::InvokeDragSession(aDOMNode, anArrayTransferables, aRegion,
                                       aActionType);
  nsresult rv;

  
  nsIURI *uri = nsnull;

  nsCOMPtr<nsIDocument> doc(do_QueryInterface(mSourceDocument));
  if (doc) {
    uri = doc->GetDocumentURI();
  }

  PRUint32 numItemsToDrag = 0;
  rv = anArrayTransferables->Count(&numItemsToDrag);
  if (!numItemsToDrag)
    return NS_ERROR_FAILURE;

  
  

  
  
  
  
  nsRefPtr<IDataObject> itemToDrag;
  if (numItemsToDrag > 1) {
    nsDataObjCollection * dataObjCollection = new nsDataObjCollection();
    if (!dataObjCollection)
      return NS_ERROR_OUT_OF_MEMORY;
    itemToDrag = dataObjCollection;
    for (PRUint32 i=0; i<numItemsToDrag; ++i) {
      nsCOMPtr<nsISupports> supports;
      anArrayTransferables->GetElementAt(i, getter_AddRefs(supports));
      nsCOMPtr<nsITransferable> trans(do_QueryInterface(supports));
      if (trans) {
        nsRefPtr<IDataObject> dataObj;
        rv = nsClipboard::CreateNativeDataObject(trans,
                                                 getter_AddRefs(dataObj), uri);
        NS_ENSURE_SUCCESS(rv, rv);

        dataObjCollection->AddDataObject(dataObj);
      }
    }
  } 
  else {
    nsCOMPtr<nsISupports> supports;
    anArrayTransferables->GetElementAt(0, getter_AddRefs(supports));
    nsCOMPtr<nsITransferable> trans(do_QueryInterface(supports));
    if (trans) {
      rv = nsClipboard::CreateNativeDataObject(trans,
                                               getter_AddRefs(itemToDrag),
                                               uri);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  } 

  
  return StartInvokingDragSession(itemToDrag, aActionType);
}


NS_IMETHODIMP
nsDragService::StartInvokingDragSession(IDataObject * aDataObj,
                                        PRUint32 aActionType)
{
  
  
  NS_IF_RELEASE(mNativeDragSrc);
  mNativeDragSrc = (IDropSource *)new nsNativeDragSource();
  if (!mNativeDragSrc)
    return NS_ERROR_OUT_OF_MEMORY;

  mNativeDragSrc->AddRef();

  
  DWORD dropRes;
  DWORD effects = DROPEFFECT_SCROLL;
  if (aActionType & DRAGDROP_ACTION_COPY) {
    effects |= DROPEFFECT_COPY;
  }
  if (aActionType & DRAGDROP_ACTION_MOVE) {
    effects |= DROPEFFECT_MOVE;
  }
  if (aActionType & DRAGDROP_ACTION_LINK) {
    effects |= DROPEFFECT_LINK;
  }

  
  
  mDragAction = aActionType;
  mDoingDrag  = PR_TRUE;

  
  StartDragSession();

  
  PRUint64 lShellVersion = GetShellVersion();
  IAsyncOperation *pAsyncOp = NULL;
  PRBool isAsyncAvailable = LL_UCMP(lShellVersion, >=, LL_INIT(5, 0));
  if (isAsyncAvailable)
  {
    
    if (SUCCEEDED(aDataObj->QueryInterface(IID_IAsyncOperation,
                                          (void**)&pAsyncOp)))
      pAsyncOp->SetAsyncMode(TRUE);
  }

  
  HRESULT res = ::DoDragDrop(aDataObj, mNativeDragSrc, effects, &dropRes);

  if (isAsyncAvailable)
  {
    
    
    BOOL isAsync = FALSE;
    if (pAsyncOp)
    {
      pAsyncOp->InOperation(&isAsync);
      if (!isAsync)
        aDataObj->Release();
    }
  }

  
  EndDragSession(PR_TRUE);

  
  
  
  
  
  
  
  static CLIPFORMAT PerformedDropEffect =
    ::RegisterClipboardFormat(CFSTR_PERFORMEDDROPEFFECT);

  FORMATETC fmte =
    {
      (CLIPFORMAT)PerformedDropEffect,
      NULL,
      DVASPECT_CONTENT,
      -1,
      TYMED_NULL
    };

  STGMEDIUM medium;
  medium.tymed = TYMED_NULL;
  medium.pUnkForRelease = NULL;
  aDataObj->SetData(&fmte, &medium, FALSE);

  mDoingDrag = PR_FALSE;

  return DRAGDROP_S_DROP == res ? NS_OK : NS_ERROR_FAILURE;
}



nsDataObjCollection*
nsDragService::GetDataObjCollection(IDataObject* aDataObj)
{
  nsDataObjCollection * dataObjCol = nsnull;
  if (aDataObj) {
    nsIDataObjCollection* dataObj;
    if (aDataObj->QueryInterface(IID_IDataObjCollection,
                                 (void**)&dataObj) == S_OK) {
      dataObjCol = static_cast<nsDataObjCollection*>(aDataObj);
      dataObj->Release();
    }
  }

  return dataObjCol;
}


NS_IMETHODIMP
nsDragService::GetNumDropItems(PRUint32 * aNumItems)
{
  if (!mDataObject) {
    *aNumItems = 0;
    return NS_OK;
  }

  if (IsCollectionObject(mDataObject)) {
    nsDataObjCollection * dataObjCol = GetDataObjCollection(mDataObject);
    if (dataObjCol)
      *aNumItems = dataObjCol->GetNumDataObjects();
  }
  else {
    
    
    
    FORMATETC fe2;
    SET_FORMATETC(fe2, CF_HDROP, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL);
    if (mDataObject->QueryGetData(&fe2) == S_OK) {
      STGMEDIUM stm;
      if (mDataObject->GetData(&fe2, &stm) == S_OK) {
        HDROP hdrop = (HDROP)GlobalLock(stm.hGlobal);
        *aNumItems = ::DragQueryFileW(hdrop, 0xFFFFFFFF, NULL, 0);
        ::GlobalUnlock(stm.hGlobal);
        ::ReleaseStgMedium(&stm);
      }
    }
    else
      *aNumItems = 1;
  }

  return NS_OK;
}


NS_IMETHODIMP
nsDragService::GetData(nsITransferable * aTransferable, PRUint32 anItem)
{
  
  
  
  if (!mDataObject)
    return NS_ERROR_FAILURE;

  nsresult dataFound = NS_ERROR_FAILURE;

  if (IsCollectionObject(mDataObject)) {
    
    nsDataObjCollection * dataObjCol = GetDataObjCollection(mDataObject);
    PRUint32 cnt = dataObjCol->GetNumDataObjects();
    if (anItem >= 0 && anItem < cnt) {
      IDataObject * dataObj = dataObjCol->GetDataObjectAt(anItem);
      dataFound = nsClipboard::GetDataFromDataObject(dataObj, 0, nsnull,
                                                     aTransferable);
    }
    else
      NS_WARNING("Index out of range!");
  }
  else {
    
    if (anItem == 0) {
       dataFound = nsClipboard::GetDataFromDataObject(mDataObject, anItem,
                                                      nsnull, aTransferable);
    } else {
      
      FORMATETC fe2;
      SET_FORMATETC(fe2, CF_HDROP, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL);
      if (mDataObject->QueryGetData(&fe2) == S_OK)
        dataFound = nsClipboard::GetDataFromDataObject(mDataObject, anItem,
                                                       nsnull, aTransferable);
      else
        NS_WARNING("Reqesting non-zero index, but clipboard data is not a collection!");
    }
  }
  return dataFound;
}


NS_IMETHODIMP
nsDragService::SetIDataObject(IDataObject * aDataObj)
{
  
  
  NS_IF_RELEASE(mDataObject);
  mDataObject = aDataObj;
  NS_IF_ADDREF(mDataObject);

  return NS_OK;
}


NS_IMETHODIMP
nsDragService::IsDataFlavorSupported(const char *aDataFlavor, PRBool *_retval)
{
  if (!aDataFlavor || !mDataObject || !_retval)
    return NS_ERROR_FAILURE;

#ifdef NS_DEBUG
  if (strcmp(aDataFlavor, kTextMime) == 0)
    NS_WARNING("DO NOT USE THE text/plain DATA FLAVOR ANY MORE. USE text/unicode INSTEAD");
#endif

  *_retval = PR_FALSE;

  FORMATETC fe;
  UINT format = 0;

  if (IsCollectionObject(mDataObject)) {
    
    format = nsClipboard::GetFormat(aDataFlavor);
    SET_FORMATETC(fe, format, 0, DVASPECT_CONTENT, -1,
                  TYMED_HGLOBAL | TYMED_FILE | TYMED_GDI);

    
    
    nsDataObjCollection* dataObjCol = GetDataObjCollection(mDataObject);
    if (dataObjCol) {
      PRUint32 cnt = dataObjCol->GetNumDataObjects();
      for (PRUint32 i=0;i<cnt;++i) {
        IDataObject * dataObj = dataObjCol->GetDataObjectAt(i);
        if (S_OK == dataObj->QueryGetData(&fe))
          *_retval = PR_TRUE;             
      }
    }
  } 
  else {
    
    
    
    
    format = nsClipboard::GetFormat(aDataFlavor);
    SET_FORMATETC(fe, format, 0, DVASPECT_CONTENT, -1,
                  TYMED_HGLOBAL | TYMED_FILE | TYMED_GDI);
    if (mDataObject->QueryGetData(&fe) == S_OK)
      *_retval = PR_TRUE;                 
    else {
      
      
      
      if (strcmp(aDataFlavor, kUnicodeMime) == 0) {
        
        
        
        format = nsClipboard::GetFormat(kTextMime);
        SET_FORMATETC(fe, format, 0, DVASPECT_CONTENT, -1,
                      TYMED_HGLOBAL | TYMED_FILE | TYMED_GDI);
        if (mDataObject->QueryGetData(&fe) == S_OK)
          *_retval = PR_TRUE;                 
      }
      else if (strcmp(aDataFlavor, kURLMime) == 0) {
        
        
        
        format = nsClipboard::GetFormat(kFileMime);
        SET_FORMATETC(fe, format, 0, DVASPECT_CONTENT, -1,
                      TYMED_HGLOBAL | TYMED_FILE | TYMED_GDI);
        if (mDataObject->QueryGetData(&fe) == S_OK)
          *_retval = PR_TRUE;                 
      }
    } 
  }

  return NS_OK;
}









PRBool
nsDragService::IsCollectionObject(IDataObject* inDataObj)
{
  PRBool isCollection = PR_FALSE;

  
  
  static UINT sFormat = 0;
  static FORMATETC sFE;
  if (!sFormat) {
    sFormat = nsClipboard::GetFormat(MULTI_MIME);
    SET_FORMATETC(sFE, sFormat, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL);
  }

  
  
  if (inDataObj->QueryGetData(&sFE) == S_OK)
    isCollection = PR_TRUE;

  return isCollection;

} 









NS_IMETHODIMP
nsDragService::EndDragSession(PRBool aDoneDrag)
{
  nsBaseDragService::EndDragSession(aDoneDrag);
  NS_IF_RELEASE(mDataObject);

  return NS_OK;
}


PRUint64 nsDragService::GetShellVersion()
{
  PRUint64 lVersion = LL_INIT(0, 0);
  PRUint64 lMinor = lVersion;

  
  PRLibrary *libShell = PR_LoadLibrary("shell32.dll");
  if (libShell == NULL)
    return lVersion;

  do
  {
    DLLGETVERSIONPROC versionProc = NULL;
    versionProc = (DLLGETVERSIONPROC)PR_FindFunctionSymbol(libShell, "DllGetVersion");
    if (versionProc == NULL)
      break;

    DLLVERSIONINFO versionInfo;
    ::ZeroMemory(&versionInfo, sizeof(DLLVERSIONINFO));
    versionInfo.cbSize = sizeof(DLLVERSIONINFO);
    if (FAILED(versionProc(&versionInfo)))
      break;

    
    LL_UI2L(lVersion, versionInfo.dwMajorVersion);
    LL_SHL(lVersion, lVersion, 32);
    LL_UI2L(lMinor, versionInfo.dwMinorVersion);
    LL_OR2(lVersion, lMinor);
  } while (false);

  PR_UnloadLibrary(libShell);
  libShell = NULL;

  return lVersion;
}
