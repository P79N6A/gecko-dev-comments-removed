




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
#include "gfxContext.h"
#include "nsRect.h"
#include "nsMathUtils.h"
#include "gfxWindowsPlatform.h"
#include "mozilla/gfx/2D.h"
#include "mozilla/gfx/DataSurfaceHelpers.h"
#include "mozilla/gfx/Tools.h"

using namespace mozilla;
using namespace mozilla::gfx;






nsDragService::nsDragService()
  : mDataObject(nullptr), mSentLocalDropEvent(false)
{
}






nsDragService::~nsDragService()
{
  NS_IF_RELEASE(mDataObject);
}

bool
nsDragService::CreateDragImage(nsIDOMNode *aDOMNode,
                               nsIScriptableRegion *aRegion,
                               SHDRAGIMAGE *psdi)
{
  if (!psdi)
    return false;

  memset(psdi, 0, sizeof(SHDRAGIMAGE));
  if (!aDOMNode) 
    return false;

  
  nsIntRect dragRect;
  RefPtr<SourceSurface> surface;
  nsPresContext* pc;
  DrawDrag(aDOMNode, aRegion,
           mScreenX, mScreenY,
           &dragRect, &surface, &pc);
  if (!surface)
    return false;

  uint32_t bmWidth = dragRect.width, bmHeight = dragRect.height;

  if (bmWidth == 0 || bmHeight == 0)
    return false;

  psdi->crColorKey = CLR_NONE;

  RefPtr<DataSourceSurface> dataSurface =
    Factory::CreateDataSourceSurface(IntSize(bmWidth, bmHeight),
                                     SurfaceFormat::B8G8R8A8);
  NS_ENSURE_TRUE(dataSurface, false);

  DataSourceSurface::MappedSurface map;
  if (!dataSurface->Map(DataSourceSurface::MapType::READ_WRITE, &map)) {
    return false;
  }

  RefPtr<DrawTarget> dt =
    Factory::CreateDrawTargetForData(BackendType::CAIRO,
                                     map.mData,
                                     dataSurface->GetSize(),
                                     map.mStride,
                                     dataSurface->GetFormat());
  if (!dt) {
    dataSurface->Unmap();
    return false;
  }

  dt->DrawSurface(surface,
                  Rect(0, 0, dataSurface->GetSize().width, dataSurface->GetSize().height),
                  Rect(0, 0, surface->GetSize().width, surface->GetSize().height),
                  DrawSurfaceOptions(),
                  DrawOptions(1.0f, CompositionOp::OP_SOURCE));
  dt->Flush();

  BITMAPV5HEADER bmih;
  memset((void*)&bmih, 0, sizeof(BITMAPV5HEADER));
  bmih.bV5Size        = sizeof(BITMAPV5HEADER);
  bmih.bV5Width       = bmWidth;
  bmih.bV5Height      = -(int32_t)bmHeight; 
  bmih.bV5Planes      = 1;
  bmih.bV5BitCount    = 32;
  bmih.bV5Compression = BI_BITFIELDS;
  bmih.bV5RedMask     = 0x00FF0000;
  bmih.bV5GreenMask   = 0x0000FF00;
  bmih.bV5BlueMask    = 0x000000FF;
  bmih.bV5AlphaMask   = 0xFF000000;

  HDC hdcSrc = CreateCompatibleDC(nullptr);
  void *lpBits = nullptr;
  if (hdcSrc) {
    psdi->hbmpDragImage = 
    ::CreateDIBSection(hdcSrc, (BITMAPINFO*)&bmih, DIB_RGB_COLORS,
                       (void**)&lpBits, nullptr, 0);
    if (psdi->hbmpDragImage && lpBits) {
      CopySurfaceDataToPackedArray(map.mData, static_cast<uint8_t*>(lpBits),
                                   dataSurface->GetSize(), map.mStride,
                                   BytesPerPixel(dataSurface->GetFormat()));
    }

    psdi->sizeDragImage.cx = bmWidth;
    psdi->sizeDragImage.cy = bmHeight;

    
    if (mScreenX == -1 || mScreenY == -1) {
      psdi->ptOffset.x = (uint32_t)((float)bmWidth/2.0f);
      psdi->ptOffset.y = (uint32_t)((float)bmHeight/2.0f);
    } else {
      int32_t sx = mScreenX, sy = mScreenY;
      ConvertToUnscaledDevPixels(pc, &sx, &sy);
      psdi->ptOffset.x = sx - dragRect.x;
      psdi->ptOffset.y = sy - dragRect.y;
    }

    DeleteDC(hdcSrc);
  }

  dataSurface->Unmap();

  return psdi->hbmpDragImage != nullptr;
}


NS_IMETHODIMP
nsDragService::InvokeDragSession(nsIDOMNode *aDOMNode,
                                 nsISupportsArray *anArrayTransferables,
                                 nsIScriptableRegion *aRegion,
                                 uint32_t aActionType)
{
  nsresult rv = nsBaseDragService::InvokeDragSession(aDOMNode,
                                                     anArrayTransferables,
                                                     aRegion,
                                                     aActionType);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsIURI *uri = nullptr;

  nsCOMPtr<nsIDocument> doc(do_QueryInterface(mSourceDocument));
  if (doc) {
    uri = doc->GetDocumentURI();
  }

  uint32_t numItemsToDrag = 0;
  rv = anArrayTransferables->Count(&numItemsToDrag);
  if (!numItemsToDrag)
    return NS_ERROR_FAILURE;

  
  

  
  
  
  
  nsRefPtr<IDataObject> itemToDrag;
  if (numItemsToDrag > 1) {
    nsDataObjCollection * dataObjCollection = new nsDataObjCollection();
    if (!dataObjCollection)
      return NS_ERROR_OUT_OF_MEMORY;
    itemToDrag = dataObjCollection;
    for (uint32_t i=0; i<numItemsToDrag; ++i) {
      nsCOMPtr<nsISupports> supports;
      anArrayTransferables->GetElementAt(i, getter_AddRefs(supports));
      nsCOMPtr<nsITransferable> trans(do_QueryInterface(supports));
      if (trans) {
        
        trans->SetRequestingNode(aDOMNode);
        nsRefPtr<IDataObject> dataObj;
        rv = nsClipboard::CreateNativeDataObject(trans,
                                                 getter_AddRefs(dataObj), uri);
        NS_ENSURE_SUCCESS(rv, rv);
        
        rv = nsClipboard::SetupNativeDataObject(trans, dataObjCollection);
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
      
      trans->SetRequestingNode(aDOMNode);
      rv = nsClipboard::CreateNativeDataObject(trans,
                                               getter_AddRefs(itemToDrag),
                                               uri);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  } 

  
  IDragSourceHelper *pdsh;
  if (SUCCEEDED(CoCreateInstance(CLSID_DragDropHelper, nullptr,
                                 CLSCTX_INPROC_SERVER,
                                 IID_IDragSourceHelper, (void**)&pdsh))) {
    SHDRAGIMAGE sdi;
    if (CreateDragImage(aDOMNode, aRegion, &sdi)) {
      if (FAILED(pdsh->InitializeFromBitmap(&sdi, itemToDrag)))
        DeleteObject(sdi.hbmpDragImage);
    }
    pdsh->Release();
  }

  
  return StartInvokingDragSession(itemToDrag, aActionType);
}


NS_IMETHODIMP
nsDragService::StartInvokingDragSession(IDataObject * aDataObj,
                                        uint32_t aActionType)
{
  
  
  nsRefPtr<nsNativeDragSource> nativeDragSrc =
    new nsNativeDragSource(mDataTransfer);

  
  DWORD winDropRes;
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
  mSentLocalDropEvent = false;

  
  StartDragSession();
  OpenDragPopup();

  nsRefPtr<IAsyncOperation> pAsyncOp;
  
  if (SUCCEEDED(aDataObj->QueryInterface(IID_IAsyncOperation,
                                         getter_AddRefs(pAsyncOp)))) {
    pAsyncOp->SetAsyncMode(VARIANT_TRUE);
  } else {
    NS_NOTREACHED("When did our data object stop being async");
  }

  
  HRESULT res = ::DoDragDrop(aDataObj, nativeDragSrc, effects, &winDropRes);

  
  
  if (!mSentLocalDropEvent) {
    uint32_t dropResult;
    
    if (winDropRes & DROPEFFECT_COPY)
        dropResult = DRAGDROP_ACTION_COPY;
    else if (winDropRes & DROPEFFECT_LINK)
        dropResult = DRAGDROP_ACTION_LINK;
    else if (winDropRes & DROPEFFECT_MOVE)
        dropResult = DRAGDROP_ACTION_MOVE;
    else
        dropResult = DRAGDROP_ACTION_NONE;
    
    if (mDataTransfer) {
      if (res == DRAGDROP_S_DROP) 
        mDataTransfer->SetDropEffectInt(dropResult);
      else
        mDataTransfer->SetDropEffectInt(DRAGDROP_ACTION_NONE);
    }
  }

  mUserCancelled = nativeDragSrc->UserCancelled();

  
  
  
  
  
  DWORD pos = ::GetMessagePos();
  FLOAT dpiScale = gfxWindowsPlatform::GetPlatform()->GetDPIScale();
  nsIntPoint logPos(NSToIntRound(GET_X_LPARAM(pos) / dpiScale),
                    NSToIntRound(GET_Y_LPARAM(pos) / dpiScale));
  SetDragEndPoint(logPos);
  EndDragSession(true);

  mDoingDrag = false;

  return DRAGDROP_S_DROP == res ? NS_OK : NS_ERROR_FAILURE;
}



nsDataObjCollection*
nsDragService::GetDataObjCollection(IDataObject* aDataObj)
{
  nsDataObjCollection * dataObjCol = nullptr;
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
nsDragService::GetNumDropItems(uint32_t * aNumItems)
{
  if (!mDataObject) {
    *aNumItems = 0;
    return NS_OK;
  }

  if (IsCollectionObject(mDataObject)) {
    nsDataObjCollection * dataObjCol = GetDataObjCollection(mDataObject);
    if (dataObjCol) {
      *aNumItems = dataObjCol->GetNumDataObjects();
    }
    else {
      
      
      
      
      
      *aNumItems = 0;
    }
  }
  else {
    
    
    
    FORMATETC fe2;
    SET_FORMATETC(fe2, CF_HDROP, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL);
    if (mDataObject->QueryGetData(&fe2) == S_OK) {
      STGMEDIUM stm;
      if (mDataObject->GetData(&fe2, &stm) == S_OK) {
        HDROP hdrop = (HDROP)GlobalLock(stm.hGlobal);
        *aNumItems = ::DragQueryFileW(hdrop, 0xFFFFFFFF, nullptr, 0);
        ::GlobalUnlock(stm.hGlobal);
        ::ReleaseStgMedium(&stm);
        
        if (*aNumItems == 0)
          *aNumItems = 1;
      }
      else
        *aNumItems = 1;
    }
    else
      *aNumItems = 1;
  }

  return NS_OK;
}


NS_IMETHODIMP
nsDragService::GetData(nsITransferable * aTransferable, uint32_t anItem)
{
  
  
  
  if (!mDataObject)
    return NS_ERROR_FAILURE;

  nsresult dataFound = NS_ERROR_FAILURE;

  if (IsCollectionObject(mDataObject)) {
    
    nsDataObjCollection * dataObjCol = GetDataObjCollection(mDataObject);
    uint32_t cnt = dataObjCol->GetNumDataObjects();
    if (anItem < cnt) {
      IDataObject * dataObj = dataObjCol->GetDataObjectAt(anItem);
      dataFound = nsClipboard::GetDataFromDataObject(dataObj, 0, nullptr,
                                                     aTransferable);
    }
    else
      NS_WARNING("Index out of range!");
  }
  else {
    
    if (anItem == 0) {
       dataFound = nsClipboard::GetDataFromDataObject(mDataObject, anItem,
                                                      nullptr, aTransferable);
    } else {
      
      FORMATETC fe2;
      SET_FORMATETC(fe2, CF_HDROP, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL);
      if (mDataObject->QueryGetData(&fe2) == S_OK)
        dataFound = nsClipboard::GetDataFromDataObject(mDataObject, anItem,
                                                       nullptr, aTransferable);
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


void
nsDragService::SetDroppedLocal()
{
  
  
  mSentLocalDropEvent = true;
  return;
}


NS_IMETHODIMP
nsDragService::IsDataFlavorSupported(const char *aDataFlavor, bool *_retval)
{
  if (!aDataFlavor || !mDataObject || !_retval)
    return NS_ERROR_FAILURE;

#ifdef DEBUG
  if (strcmp(aDataFlavor, kTextMime) == 0)
    NS_WARNING("DO NOT USE THE text/plain DATA FLAVOR ANY MORE. USE text/unicode INSTEAD");
#endif

  *_retval = false;

  FORMATETC fe;
  UINT format = 0;

  if (IsCollectionObject(mDataObject)) {
    
    format = nsClipboard::GetFormat(aDataFlavor);
    SET_FORMATETC(fe, format, 0, DVASPECT_CONTENT, -1,
                  TYMED_HGLOBAL | TYMED_FILE | TYMED_GDI);

    
    
    nsDataObjCollection* dataObjCol = GetDataObjCollection(mDataObject);
    if (dataObjCol) {
      uint32_t cnt = dataObjCol->GetNumDataObjects();
      for (uint32_t i=0;i<cnt;++i) {
        IDataObject * dataObj = dataObjCol->GetDataObjectAt(i);
        if (S_OK == dataObj->QueryGetData(&fe))
          *_retval = true;             
      }
    }
  } 
  else {
    
    
    
    
    format = nsClipboard::GetFormat(aDataFlavor);
    SET_FORMATETC(fe, format, 0, DVASPECT_CONTENT, -1,
                  TYMED_HGLOBAL | TYMED_FILE | TYMED_GDI);
    if (mDataObject->QueryGetData(&fe) == S_OK)
      *_retval = true;                 
    else {
      
      
      
      if (strcmp(aDataFlavor, kUnicodeMime) == 0) {
        
        
        
        format = nsClipboard::GetFormat(kTextMime);
        SET_FORMATETC(fe, format, 0, DVASPECT_CONTENT, -1,
                      TYMED_HGLOBAL | TYMED_FILE | TYMED_GDI);
        if (mDataObject->QueryGetData(&fe) == S_OK)
          *_retval = true;                 
      }
      else if (strcmp(aDataFlavor, kURLMime) == 0) {
        
        
        
        format = nsClipboard::GetFormat(kFileMime);
        SET_FORMATETC(fe, format, 0, DVASPECT_CONTENT, -1,
                      TYMED_HGLOBAL | TYMED_FILE | TYMED_GDI);
        if (mDataObject->QueryGetData(&fe) == S_OK)
          *_retval = true;                 
      }
    } 
  }

  return NS_OK;
}









bool
nsDragService::IsCollectionObject(IDataObject* inDataObj)
{
  bool isCollection = false;

  
  
  static UINT sFormat = 0;
  static FORMATETC sFE;
  if (!sFormat) {
    sFormat = nsClipboard::GetFormat(MULTI_MIME);
    SET_FORMATETC(sFE, sFormat, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL);
  }

  
  
  if (inDataObj->QueryGetData(&sFE) == S_OK)
    isCollection = true;

  return isCollection;

} 









NS_IMETHODIMP
nsDragService::EndDragSession(bool aDoneDrag)
{
  
  
  
  if (::GetCapture()) {
    ::ReleaseCapture();
  }

  nsBaseDragService::EndDragSession(aDoneDrag);
  NS_IF_RELEASE(mDataObject);

  return NS_OK;
}
