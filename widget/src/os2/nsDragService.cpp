




































#define INCL_DOSMISC
#define INCL_DOSERRORS

#include "nsDragService.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "nsString.h"
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
#include "nsIWebBrowserPersist.h"
#include "nsILocalFile.h"
#include "nsIURI.h"
#include "nsIURL.h"
#include "nsNetUtil.h"
#include "nsOS2Uni.h"
#include "nsdefs.h"
#include "wdgtos2rc.h"
#include "nsILocalFileOS2.h"
#include "nsIDocument.h"
#include "nsGUIEvent.h"
#include "nsISelection.h"





#ifndef DC_PREPAREITEM
  #define DC_PREPAREITEM  0x0040
#endif


#define MAXTITLELTH 31
#define TITLESEPARATOR (L' ')

#define DTSHARE_NAME    "\\SHAREMEM\\MOZ_DND"
#define DTSHARE_RMF     "<DRM_DTSHARE, DRF_TEXT>"

#define OS2FILE_NAME    "MOZ_TGT.TMP"
#define OS2FILE_TXTRMF  "<DRM_OS2FILE, DRF_TEXT>"
#define OS2FILE_UNKRMF  "<DRM_OS2FILE, DRF_UNKNOWN>"




nsresult RenderToOS2File( PDRAGITEM pditem, HWND hwnd);
nsresult RenderToOS2FileComplete(PDRAGTRANSFER pdxfer, USHORT usResult,
                                 PRBool content, char** outText);
nsresult RenderToDTShare( PDRAGITEM pditem, HWND hwnd);
nsresult RenderToDTShareComplete(PDRAGTRANSFER pdxfer, USHORT usResult,
                                 char** outText);
nsresult RequestRendering( PDRAGITEM pditem, HWND hwnd, PCSZ pRMF, PCSZ pName);
nsresult GetAtom( ATOM aAtom, char** outText);
nsresult GetFileName(PDRAGITEM pditem, char** outText);
nsresult GetFileContents(PCSZ pszPath, char** outText);
nsresult GetTempFileName(char** outText);
void     SaveTypeAndSource(nsILocalFile *file, nsIDOMDocument *domDoc,
                           PCSZ pszType);
int      UnicodeToCodepage( const nsAString& inString, char **outText);
int      CodepageToUnicode( const nsACString& inString, PRUnichar **outText);
void     RemoveCarriageReturns(char * pszText);
MRESULT EXPENTRY nsDragWindowProc(HWND hWnd, ULONG msg, MPARAM mp1, MPARAM mp2);




static HPOINTER gPtrArray[IDC_DNDCOUNT];
static char *   gTempFile = 0;




nsDragService::nsDragService()
{
    
  mDragWnd = WinCreateWindow( HWND_DESKTOP, WC_STATIC, 0, 0, 0, 0, 0, 0,
                              HWND_DESKTOP, HWND_BOTTOM, 0, 0, 0);
  WinSubclassWindow( mDragWnd, nsDragWindowProc);

  HMODULE hModResources = NULLHANDLE;
  DosQueryModFromEIP(&hModResources, NULL, 0, NULL, NULL, (ULONG) &gPtrArray);
  for (int i = 0; i < IDC_DNDCOUNT; i++)
    gPtrArray[i] = ::WinLoadPointer(HWND_DESKTOP, hModResources, i+IDC_DNDBASE);
}



nsDragService::~nsDragService()
{
    
  WinDestroyWindow(mDragWnd);

  for (int i = 0; i < IDC_DNDCOUNT; i++) {
    WinDestroyPointer(gPtrArray[i]);
    gPtrArray[i] = 0;
  }
}

NS_IMPL_ISUPPORTS_INHERITED1(nsDragService, nsBaseDragService, nsIDragSessionOS2)



NS_IMETHODIMP nsDragService::InvokeDragSession(nsIDOMNode *aDOMNode,
                                            nsISupportsArray *aTransferables, 
                                            nsIScriptableRegion *aRegion,
                                            PRUint32 aActionType)
{
  if (mDoingDrag)
    return NS_ERROR_UNEXPECTED;

  nsresult rv = nsBaseDragService::InvokeDragSession(aDOMNode, aTransferables,
                                                     aRegion, aActionType);
  NS_ENSURE_SUCCESS(rv, rv);

  mSourceDataItems = aTransferables;
  WinSetCapture(HWND_DESKTOP, NULLHANDLE);

    
  PDRAGINFO pDragInfo = DrgAllocDraginfo(1);
  if (!pDragInfo)
    return NS_ERROR_UNEXPECTED;

  pDragInfo->usOperation = DO_DEFAULT;

  DRAGITEM dragitem;
  dragitem.hwndItem            = mDragWnd;
  dragitem.ulItemID            = (ULONG)this;
  dragitem.fsControl           = DC_OPEN;
  dragitem.cxOffset            = 2;
  dragitem.cyOffset            = 2;
  dragitem.fsSupportedOps      = DO_COPYABLE|DO_MOVEABLE|DO_LINKABLE;

    
  dragitem.hstrContainerName   = NULLHANDLE;
  dragitem.hstrSourceName      = NULLHANDLE;

  rv = NS_ERROR_FAILURE;
  ULONG idIcon = 0;

    
  {
    nsCOMPtr<nsISupports> genericItem;
    mSourceDataItems->GetElementAt(0, getter_AddRefs(genericItem));
    nsCOMPtr<nsITransferable> transItem (do_QueryInterface(genericItem));

    nsCOMPtr<nsISupports> genericData;
    PRUint32 len = 0;

      
      

    if (NS_SUCCEEDED(transItem->GetTransferData(kURLMime,
                              getter_AddRefs(genericData), &len))) {
      nsXPIDLCString targetName;
      rv = GetUrlAndTitle( genericData, getter_Copies(targetName));
      if (NS_SUCCEEDED(rv)) {
        
        
        dragitem.fsControl     |= DC_PREPAREITEM;
        dragitem.hstrType       = DrgAddStrHandle("UniformResourceLocator");
        dragitem.hstrRMF        = DrgAddStrHandle("<DRM_OS2FILE,DRF_TEXT>");
        dragitem.hstrTargetName = DrgAddStrHandle(targetName.get());
        idIcon = IDC_DNDURL;
      }
    }
    else
    if (NS_SUCCEEDED(transItem->GetTransferData(kUnicodeMime,
                                getter_AddRefs(genericData), &len))) {
      nsXPIDLCString targetName;
      rv = GetUniTextTitle( genericData, getter_Copies(targetName));
      if (NS_SUCCEEDED(rv)) {
        dragitem.hstrType       = DrgAddStrHandle("Plain Text");
        dragitem.hstrRMF        = DrgAddStrHandle("<DRM_OS2FILE,DRF_TEXT>");
        dragitem.hstrTargetName = DrgAddStrHandle(targetName.get());
        idIcon = IDC_DNDTEXT;
      }
    }
  }

    
    
  if (NS_FAILED(rv)) {
    mMimeType = 0;
    dragitem.hstrType       = DrgAddStrHandle("Unknown");
    dragitem.hstrRMF        = DrgAddStrHandle("<DRM_UNKNOWN,DRF_UNKNOWN>");
    dragitem.hstrTargetName = NULLHANDLE;
  }
  DrgSetDragitem(pDragInfo, &dragitem, sizeof(DRAGITEM), 0);

  DRAGIMAGE dragimage;
  memset(&dragimage, 0, sizeof(DRAGIMAGE));
  dragimage.cb = sizeof(DRAGIMAGE);
  dragimage.fl = DRG_ICON;
  if (idIcon)
    dragimage.hImage = gPtrArray[idIcon-IDC_DNDBASE];
  if (dragimage.hImage) {
    dragimage.cyOffset = 8;
    dragimage.cxOffset = 2;
  }
  else
    dragimage.hImage  = WinQuerySysPointer(HWND_DESKTOP, SPTR_FILE, FALSE);
    
  mDoingDrag = PR_TRUE;
  LONG escState = WinGetKeyState(HWND_DESKTOP, VK_ESC) & 0x01;
  HWND hwndDest = DrgDrag(mDragWnd, pDragInfo, &dragimage, 1, VK_BUTTON2,
                  (void*)0x80000000L); 

    
  if (hwndDest == 0 && (WinGetKeyState(HWND_DESKTOP, VK_ESC) & 0x01) != escState)
    mUserCancelled = PR_TRUE;
  FireDragEventAtSource(NS_DRAGDROP_END);
  mDoingDrag = PR_FALSE;

    
    
  if (hwndDest == 0)
      DrgDeleteDraginfoStrHandles(pDragInfo);
  DrgFreeDraginfo(pDragInfo);

    
  mSourceDataItems = 0;
  mSourceData = 0;
  mMimeType = 0;

    
  mSourceDocument = nsnull;
  mSourceNode = nsnull;
  mSelection = nsnull;
  mDataTransfer = nsnull;
  mUserCancelled = PR_FALSE;
  mHasImage = PR_FALSE;
  mImage = nsnull;
  mImageX = 0;
  mImageY = 0;
  mScreenX = -1;
  mScreenY = -1;

  return NS_OK;
}



MRESULT EXPENTRY nsDragWindowProc(HWND hWnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  switch (msg) {

      
      
      
    case DM_RENDERPREPARE: {
      PDRAGTRANSFER  pdxfer = (PDRAGTRANSFER)mp1;
      nsDragService* dragservice = (nsDragService*)pdxfer->pditem->ulItemID;
  
      if (pdxfer->usOperation == DO_COPY &&
          (WinGetKeyState(HWND_DESKTOP, VK_CTRL) & 0x8000) &&
          !strcmp(dragservice->mMimeType, kURLMime)) {
          
        nsCOMPtr<nsIURL> urlObject(do_QueryInterface(dragservice->mSourceData));
        if (urlObject) {
          nsCAutoString filename;
          urlObject->GetFileName(filename);
          if (filename.IsEmpty()) {
            urlObject->GetHost(filename);
            filename.Append("/file");
          }
          DrgDeleteStrHandle(pdxfer->pditem->hstrTargetName);
          pdxfer->pditem->hstrTargetName = DrgAddStrHandle(filename.get());
        }
      }
      return (MRESULT)TRUE;
    }
  
    case DM_RENDER: {
      nsresult       rv = NS_ERROR_FAILURE;
      PDRAGTRANSFER  pdxfer = (PDRAGTRANSFER)mp1;
      nsDragService* dragservice = (nsDragService*)pdxfer->pditem->ulItemID;
      char           chPath[CCHMAXPATH];

      DrgQueryStrName(pdxfer->hstrRenderToName, CCHMAXPATH, chPath);

        
        
        
        

      if (!strcmp(dragservice->mMimeType, kURLMime)) {
        if (pdxfer->usOperation == DO_COPY &&
            (WinGetKeyState(HWND_DESKTOP, VK_CTRL) & 0x8000)) {
          nsCOMPtr<nsIURL> urlObject(do_QueryInterface(dragservice->mSourceData));
          if (urlObject)
            rv = dragservice->SaveAsContents(chPath, urlObject);
        }
        if (!NS_SUCCEEDED(rv)) {
          nsCOMPtr<nsIURI> uriObject(do_QueryInterface(dragservice->mSourceData));
          if (uriObject)
            rv = dragservice->SaveAsURL(chPath, uriObject);
        }
      }
      else
          
        if (!strcmp(dragservice->mMimeType, kUnicodeMime)) {
          nsCOMPtr<nsISupportsString> strObject(
                                 do_QueryInterface(dragservice->mSourceData));
          if (strObject)
            rv = dragservice->SaveAsText(chPath, strObject);
        }
  
      DrgPostTransferMsg(pdxfer->hwndClient, DM_RENDERCOMPLETE, pdxfer,
                         (NS_SUCCEEDED(rv) ? DMFL_RENDEROK : DMFL_RENDERFAIL),
                         0, TRUE);
      DrgFreeDragtransfer(pdxfer);
      return (MRESULT)TRUE;
    }

      
    case DM_DRAGOVERNOTIFY:
    case DM_ENDCONVERSATION:
      return 0;
  
    default:
      break;
  }

  return ::WinDefWindowProc(hWnd, msg, mp1, mp2);
}







NS_IMETHODIMP nsDragService::StartDragSession()
{
  NS_ERROR("OS/2 version of StartDragSession() should never be called!");
  return NS_OK;
}

NS_IMETHODIMP nsDragService::EndDragSession(PRBool aDragDone)
{
  NS_ERROR("OS/2 version of EndDragSession() should never be called!");
  return NS_OK;
}



NS_IMETHODIMP nsDragService::GetNumDropItems(PRUint32 *aNumDropItems)
{
  if (mSourceDataItems)
    mSourceDataItems->Count(aNumDropItems);
  else
    *aNumDropItems = 0;

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
  flavorList->Count (&cnt);

  for (unsigned int i= 0; i < cnt; ++i ) {
    nsCOMPtr<nsISupports> genericWrapper;
    flavorList->GetElementAt(i, getter_AddRefs(genericWrapper));
    nsCOMPtr<nsISupportsCString> currentFlavor;
    currentFlavor = do_QueryInterface(genericWrapper);
    if (currentFlavor) {
      nsXPIDLCString flavorStr;
      currentFlavor->ToString(getter_Copies(flavorStr));
  
      nsCOMPtr<nsISupports> genericItem;
  
      mSourceDataItems->GetElementAt(aItemIndex, getter_AddRefs(genericItem));
      nsCOMPtr<nsITransferable> item (do_QueryInterface(genericItem));
      if (item) {
        nsCOMPtr<nsISupports> data;
        PRUint32 tmpDataLen = 0;
        rv = item->GetTransferData(flavorStr, getter_AddRefs(data),
                                   &tmpDataLen);
        if (NS_SUCCEEDED(rv)) {
          rv = aTransferable->SetTransferData(flavorStr, data, tmpDataLen);
          break;
        }
      }
    }
  }

  return rv;
}









NS_IMETHODIMP nsDragService::IsDataFlavorSupported(const char *aDataFlavor,
                                                   PRBool *_retval)
{
  if (!_retval)
    return NS_ERROR_INVALID_ARG;

  *_retval = PR_FALSE;

  PRUint32 numDragItems = 0;
  if (mSourceDataItems)
    mSourceDataItems->Count(&numDragItems);
  if (!numDragItems)
    return NS_OK;







  for (PRUint32 itemIndex = 0;
       itemIndex < numDragItems && !(*_retval); ++itemIndex) {

    nsCOMPtr<nsISupports> genericItem;
    mSourceDataItems->GetElementAt(itemIndex, getter_AddRefs(genericItem));
    nsCOMPtr<nsITransferable> currItem (do_QueryInterface(genericItem));

    if (currItem) {
      nsCOMPtr <nsISupportsArray> flavorList;
      currItem->FlavorsTransferableCanExport(getter_AddRefs(flavorList));

      if (flavorList) {
        PRUint32 numFlavors;
        flavorList->Count( &numFlavors );

        for (PRUint32 flavorIndex=0; flavorIndex < numFlavors; ++flavorIndex) {
          nsCOMPtr<nsISupports> genericWrapper;
          flavorList->GetElementAt(flavorIndex, getter_AddRefs(genericWrapper));
          nsCOMPtr<nsISupportsCString> currentFlavor;
          currentFlavor = do_QueryInterface(genericWrapper);

          if (currentFlavor) {
            nsXPIDLCString flavorStr;
            currentFlavor->ToString ( getter_Copies(flavorStr) );
            if (strcmp(flavorStr, aDataFlavor) == 0) {
              *_retval = PR_TRUE;
              break;
            }
          }
        } 
      }
    }
  }

  return NS_OK;
}





nsresult nsDragService::SaveAsContents(PCSZ pszDest, nsIURL* aURL)
{
  nsCOMPtr<nsIURI> linkURI(do_QueryInterface(aURL));
  if (!linkURI)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIWebBrowserPersist> webPersist(
    do_CreateInstance("@mozilla.org/embedding/browser/nsWebBrowserPersist;1"));
  if (!webPersist)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsILocalFile> file;
  NS_NewNativeLocalFile(nsDependentCString(pszDest), PR_TRUE,
                        getter_AddRefs(file));
  if (!file)
    return NS_ERROR_FAILURE;

  FILE* fp;
  if (NS_FAILED(file->OpenANSIFileDesc("wb+", &fp)))
    return NS_ERROR_FAILURE;

  fwrite("", 0, 1, fp);
  fclose(fp);
  webPersist->SaveURI(linkURI, nsnull, nsnull, nsnull, nsnull, file);

  return NS_OK;
}





nsresult nsDragService::SaveAsURL(PCSZ pszDest, nsIURI* aURI)
{
  nsCAutoString strUri;
  aURI->GetSpec(strUri);

  if (strUri.IsEmpty())
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsILocalFile> file;
  NS_NewNativeLocalFile(nsDependentCString(pszDest), PR_TRUE,
                        getter_AddRefs(file));
  if (!file)
    return NS_ERROR_FAILURE;

  FILE* fp;
  if (NS_FAILED(file->OpenANSIFileDesc("wb+", &fp)))
    return NS_ERROR_FAILURE;

  fwrite(strUri.get(), strUri.Length(), 1, fp);
  fclose(fp);

  nsCOMPtr<nsIDOMDocument> domDoc;
  GetSourceDocument(getter_AddRefs(domDoc));
  SaveTypeAndSource(file, domDoc, "UniformResourceLocator");

  return NS_OK;
}





nsresult nsDragService::SaveAsText(PCSZ pszDest, nsISupportsString* aString)
{
  nsAutoString strData;
  aString->GetData(strData);

  if (strData.IsEmpty())
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsILocalFile> file;
  NS_NewNativeLocalFile(nsDependentCString(pszDest), PR_TRUE,
                        getter_AddRefs(file));
  if (!file)
    return NS_ERROR_FAILURE;

  nsXPIDLCString textStr;
  int cnt = UnicodeToCodepage(strData, getter_Copies(textStr));
  if (!cnt)
    return NS_ERROR_FAILURE;

  FILE* fp;
  if (NS_FAILED(file->OpenANSIFileDesc("wb+", &fp)))
    return NS_ERROR_FAILURE;

  fwrite(textStr.get(), cnt, 1, fp);
  fclose(fp);

  nsCOMPtr<nsIDOMDocument> domDoc;
  GetSourceDocument(getter_AddRefs(domDoc));
  SaveTypeAndSource(file, domDoc, "Plain Text");

  return NS_OK;
}






nsresult  nsDragService::GetUrlAndTitle(nsISupports *aGenericData,
                                        char **aTargetName)
{
    
  nsCOMPtr<nsISupportsString> strObject ( do_QueryInterface(aGenericData));
  if (!strObject)
    return NS_ERROR_FAILURE;
  nsAutoString strData;
  strObject->GetData(strData);

    
    
  PRInt32 lineIndex = strData.FindChar ('\n');
  if (lineIndex == 0)
    return NS_ERROR_FAILURE;

    
  nsAutoString strUrl;
  if (lineIndex == -1)
    strUrl = strData;
  else
    strData.Left(strUrl, lineIndex);

    
  nsCOMPtr<nsIURI> saveURI;
  NS_NewURI(getter_AddRefs(saveURI), strUrl);
  if (!saveURI)
    return NS_ERROR_FAILURE;

    
    

  if (++lineIndex && lineIndex != (int)strData.Length() &&
      !strUrl.Equals(Substring(strData, lineIndex, strData.Length()))) {
    PRUint32 strLth = NS_MIN((int)strData.Length()-lineIndex, MAXTITLELTH);
    nsAutoString strTitle;
    strData.Mid(strTitle, lineIndex, strLth);
    if (!UnicodeToCodepage(strTitle, aTargetName))
      return NS_ERROR_FAILURE;

    mSourceData = do_QueryInterface(saveURI);
    mMimeType = kURLMime;
    return NS_OK;
  }

    
    
    

  nsCAutoString strTitle;

  nsCOMPtr<nsIURL> urlObj( do_QueryInterface(saveURI));
  if (urlObj) {
    nsCAutoString strFile;

    urlObj->GetHost(strTitle);
    urlObj->GetFileName(strFile);
    if (!strFile.IsEmpty()) {
      strTitle.AppendLiteral("/");
      strTitle.Append(strFile);
    }
    else {
      urlObj->GetDirectory(strFile);
      if (strFile.Length() > 1) {
        nsCAutoString::const_iterator start, end, curr;
        strFile.BeginReading(start);
        strFile.EndReading(end);
        strFile.EndReading(curr);
        for (curr.advance(-2); curr != start; --curr)
          if (*curr == '/')
            break;
        strTitle.Append(Substring(curr, end));
      }
    }
  }
  else {
    saveURI->GetSpec(strTitle);
    PRInt32 index = strTitle.FindChar (':');
    if (index != -1) {
      if ((strTitle.get())[++index] == '/')
        if ((strTitle.get())[++index] == '/')
          ++index;
      strTitle.Cut(0, index);
    }
    if (strTitle.Length() > MAXTITLELTH)
      strTitle.Truncate(MAXTITLELTH);
  }

  *aTargetName = ToNewCString(strTitle);

  mSourceData = do_QueryInterface(saveURI);
  mMimeType = kURLMime;
  return NS_OK;
}







nsresult  nsDragService::GetUniTextTitle(nsISupports *aGenericData,
                                         char **aTargetName)
{
    
  nsCOMPtr<nsISupportsString> strObject ( do_QueryInterface(aGenericData));
  if (!strObject)
    return NS_ERROR_FAILURE;

    
  int bufsize = (MAXTITLELTH+1)*2;
  PRUnichar * buffer = (PRUnichar*)nsMemory::Alloc(bufsize);
  if (!buffer)
    return NS_ERROR_FAILURE;

  nsAutoString strData;
  strObject->GetData(strData);
  nsAutoString::const_iterator start, end;
  strData.BeginReading(start);
  strData.EndReading(end);

    
  for( ; start != end; ++start)
    if (UniQueryChar( *start, CT_ALNUM))
      break;

    
    
  int ctr, sep;
  for (ctr=0, sep=0; start != end && ctr < MAXTITLELTH; ++start) {
    if (UniQueryChar( *start, CT_ALNUM)) {
      buffer[ctr] = *start;
      ctr++;
      sep = 0;
    }
    else
      if (!sep) {
        buffer[ctr] = TITLESEPARATOR;
        ctr++;
        sep = 1;
      }
  }
    
    
  if (sep)
    ctr--;
  if (ctr >= MAXTITLELTH - sep && buffer[ctr-2] == TITLESEPARATOR)
    ctr -= 2;
  buffer[ctr] = 0;

    
    
  if (!ctr) {
    *aTargetName = ToNewCString(NS_LITERAL_CSTRING("text"));
    ctr = 1;
  }
  else
    ctr = UnicodeToCodepage( nsDependentString(buffer), aTargetName);

    
  nsMemory::Free(buffer);

  if (!ctr)
  return NS_ERROR_FAILURE;

  mSourceData = aGenericData;
  mMimeType = kUnicodeMime;
  return NS_OK;
}









NS_IMETHODIMP nsDragService::DragOverMsg(PDRAGINFO pdinfo, MRESULT &mr,
                                         PRUint32* dragFlags)
{
  nsresult  rv = NS_ERROR_FAILURE;

  if (!&mr || !dragFlags || !pdinfo || !DrgAccessDraginfo(pdinfo))
    return rv;

  *dragFlags = 0;
  mr = MRFROM2SHORT(DOR_NEVERDROP, 0);

    
    
  if (!mDoingDrag)
    if (NS_SUCCEEDED(NativeDragEnter(pdinfo)))
      *dragFlags |= DND_DISPATCHENTEREVENT;

    
  if (mDoingDrag) {
    SetCanDrop(PR_FALSE);
    switch (pdinfo->usOperation) {
      case DO_COPY:
        SetDragAction(DRAGDROP_ACTION_COPY);
        break;
      case DO_LINK:
        SetDragAction(DRAGDROP_ACTION_LINK);
        break;
      default:
        SetDragAction(DRAGDROP_ACTION_MOVE);
        break;
    }
    if (mSourceNode)
      *dragFlags |= DND_DISPATCHEVENT | DND_GETDRAGOVERRESULT | DND_MOZDRAG;
    else
      *dragFlags |= DND_DISPATCHEVENT | DND_GETDRAGOVERRESULT | DND_NATIVEDRAG;
    rv = NS_OK;
  }

  DrgFreeDraginfo(pdinfo);
  return rv;
}







NS_IMETHODIMP nsDragService::NativeDragEnter(PDRAGINFO pdinfo)
{
  nsresult  rv = NS_ERROR_FAILURE;
  PRBool    isFQFile = FALSE;
  PRBool    isAtom = FALSE;
  PDRAGITEM pditem = 0;

  if (pdinfo->cditem != 1)
    return rv;

  pditem = DrgQueryDragitemPtr(pdinfo, 0);

  if (pditem) {
    if (DrgVerifyRMF(pditem, "DRM_ATOM", 0)) {
      isAtom = TRUE;
      rv = NS_OK;
    }
    else
    if (DrgVerifyRMF(pditem, "DRM_DTSHARE", 0))
      rv = NS_OK;
    else
    if (DrgVerifyRMF(pditem, "DRM_OS2FILE", 0)) {
      rv = NS_OK;
      if (pditem->hstrContainerName && pditem->hstrSourceName)
        isFQFile = TRUE;
    }
  }

  if (NS_SUCCEEDED(rv)) {
    rv = NS_ERROR_FAILURE;
    nsCOMPtr<nsITransferable> trans(
            do_CreateInstance("@mozilla.org/widget/transferable;1", &rv));
    if (trans) {

      PRBool isUrl = DrgVerifyType(pditem, "UniformResourceLocator");
      PRBool isAlt = (WinGetKeyState(HWND_DESKTOP, VK_ALT) & 0x8000);

        
        
      if ((isFQFile && !isAlt) || isUrl) {
        trans->AddDataFlavor(kURLMime);
        trans->AddDataFlavor(kHTMLMime);
      }

        
      trans->AddDataFlavor(kUnicodeMime);

        
      nsCOMPtr<nsISupportsArray> transArray(
                    do_CreateInstance("@mozilla.org/supports-array;1", &rv));
      if (transArray) {
        transArray->InsertElementAt(trans, 0);
        mSourceDataItems = transArray;

        
        
        
        nsXPIDLCString someText;
        if (isAtom) {
          if (NS_SUCCEEDED(GetAtom(pditem->ulItemID, getter_Copies(someText))))
            NativeDataToTransferable( someText.get(), 0, isUrl);
        }
        else
        if (isFQFile && !isAlt &&
            NS_SUCCEEDED(GetFileName(pditem, getter_Copies(someText)))) {
          nsCOMPtr<nsILocalFile> file;
          if (NS_SUCCEEDED(NS_NewNativeLocalFile(someText, PR_TRUE,
                                                 getter_AddRefs(file)))) {
            nsCAutoString textStr;
            NS_GetURLSpecFromFile(file, textStr);
            if (!textStr.IsEmpty()) {
              someText.Assign(ToNewCString(textStr));
              NativeDataToTransferable( someText.get(), 0, TRUE);
            }
          }
        }

        mSourceNode = 0;
        mSourceDocument = 0;
        mDoingDrag = TRUE;
        rv = NS_OK;
      }
    }
  }

  return rv;
}






NS_IMETHODIMP nsDragService::GetDragoverResult(MRESULT& mr)
{
  nsresult  rv = NS_ERROR_FAILURE;
  if (!&mr)
    return rv;

  if (mDoingDrag) {

    PRBool canDrop = PR_FALSE;
    USHORT usDrop;
    GetCanDrop(&canDrop);
    if (canDrop)
      usDrop = DOR_DROP;
    else
      usDrop = DOR_NODROP;

    PRUint32 action;
    USHORT   usOp;
    GetDragAction(&action);
    if (action & DRAGDROP_ACTION_COPY)
      usOp = DO_COPY;
    else
    if (action & DRAGDROP_ACTION_LINK)
      usOp = DO_LINK;
    else {
      if (mSourceNode)
        usOp = DO_MOVE;
      else
        usOp = DO_UNKNOWN+1;
      if (action == DRAGDROP_ACTION_NONE)
        usDrop = DOR_NODROP;
    }

    mr = MRFROM2SHORT(usDrop, usOp);
    rv = NS_OK;
  }
  else
    mr = MRFROM2SHORT(DOR_NEVERDROP, 0);

  return rv;
}





NS_IMETHODIMP nsDragService::DragLeaveMsg(PDRAGINFO pdinfo, PRUint32* dragFlags)
{
  if (!mDoingDrag || !dragFlags)
    return NS_ERROR_FAILURE;

  if (mSourceNode)
    *dragFlags = DND_DISPATCHEVENT | DND_EXITSESSION | DND_MOZDRAG;
  else
    *dragFlags = DND_DISPATCHEVENT | DND_EXITSESSION | DND_NATIVEDRAG;

  return NS_OK;
}






NS_IMETHODIMP nsDragService::DropHelpMsg(PDRAGINFO pdinfo, PRUint32* dragFlags)
{
  if (!mDoingDrag)
    return NS_ERROR_FAILURE;

  if (pdinfo && DrgAccessDraginfo(pdinfo)) {
    DrgDeleteDraginfoStrHandles(pdinfo);
    DrgFreeDraginfo(pdinfo);
  }

  if (!dragFlags)
    return NS_ERROR_FAILURE;

  if (mSourceNode)
    *dragFlags = DND_DISPATCHEVENT | DND_EXITSESSION | DND_MOZDRAG;
  else
    *dragFlags = DND_DISPATCHEVENT | DND_EXITSESSION | DND_NATIVEDRAG;

  return NS_OK;
}






NS_IMETHODIMP nsDragService::ExitSession(PRUint32* dragFlags)
{
  if (!mDoingDrag)
    return NS_ERROR_FAILURE;

  if (!mSourceNode) {
    mSourceDataItems = 0;
    mDataTransfer = 0;
    mDoingDrag = FALSE;

      
    if (gTempFile) {
      DosDelete(gTempFile);
      nsMemory::Free(gTempFile);
      gTempFile = 0;
    }
  }

  if (!dragFlags)
    return NS_ERROR_FAILURE;
  *dragFlags = 0;

  return NS_OK;
}







NS_IMETHODIMP nsDragService::DropMsg(PDRAGINFO pdinfo, HWND hwnd,
                                     PRUint32* dragFlags)
{
  if (!mDoingDrag || !dragFlags || !pdinfo || !DrgAccessDraginfo(pdinfo))
    return NS_ERROR_FAILURE;

  switch (pdinfo->usOperation) {
    case DO_MOVE:
      SetDragAction(DRAGDROP_ACTION_MOVE);
      break;
    case DO_COPY:
      SetDragAction(DRAGDROP_ACTION_COPY);
      break;
    case DO_LINK:
      SetDragAction(DRAGDROP_ACTION_LINK);
      break;
    default:  
      if (mSourceNode)
        SetDragAction(DRAGDROP_ACTION_MOVE);
      else
        SetDragAction(DRAGDROP_ACTION_COPY);
      break;
  }

    
    
  nsresult rv = NS_OK;
  PRBool rendering = PR_FALSE;
  if (!mSourceNode)
    rv = NativeDrop( pdinfo, hwnd, &rendering);

    
    
    

    
    
    
  if (rendering)
    *dragFlags = 0;
  else {
    

    *dragFlags = DND_EXITSESSION;
    if (NS_SUCCEEDED(rv)) {
      if (mSourceNode)
        *dragFlags |= DND_DISPATCHEVENT | DND_INDROP | DND_MOZDRAG;
      else
        *dragFlags |= DND_DISPATCHEVENT | DND_INDROP | DND_NATIVEDRAG;
    }

    DrgDeleteDraginfoStrHandles(pdinfo);
    DrgFreeDraginfo(pdinfo);
    rv = NS_OK;
  }

  return rv;
}






NS_IMETHODIMP nsDragService::NativeDrop(PDRAGINFO pdinfo, HWND hwnd,
                                        PRBool* rendering)
{
  *rendering = PR_FALSE;

  nsresult rv = NS_ERROR_FAILURE;
  PDRAGITEM pditem = DrgQueryDragitemPtr(pdinfo, 0);
  if (!pditem)
    return rv;

  nsXPIDLCString dropText;
  PRBool isUrl = DrgVerifyType(pditem, "UniformResourceLocator");

    

    
    
  if (DrgVerifyRMF(pditem, "DRM_ATOM", 0))
    rv = GetAtom(pditem->ulItemID, getter_Copies(dropText));
  else
  if (DrgVerifyRMF(pditem, "DRM_DTSHARE", 0)) {
    rv = RenderToDTShare( pditem, hwnd);
    if (NS_SUCCEEDED(rv))
      *rendering = PR_TRUE;
  }

    
    
  else
  if (DrgVerifyRMF(pditem, "DRM_OS2FILE", 0)) {
    PRBool isAlt = (WinGetKeyState(HWND_DESKTOP, VK_ALT) & 0x8000);

      
      
    if (!pditem->hstrContainerName || !pditem->hstrSourceName) {
      rv = RenderToOS2File( pditem, hwnd);
      if (NS_SUCCEEDED(rv))
        *rendering = PR_TRUE;
    }
      
      
    else {
      nsXPIDLCString fileName;
      if (NS_SUCCEEDED(GetFileName(pditem, getter_Copies(fileName)))) {
        if (isUrl || isAlt)
          rv = GetFileContents(fileName.get(), getter_Copies(dropText));
        else {
          isUrl = PR_TRUE;
          nsCOMPtr<nsILocalFile> file;
          if (NS_SUCCEEDED(NS_NewNativeLocalFile(fileName,
                                         PR_TRUE, getter_AddRefs(file)))) {
            nsCAutoString textStr;
            NS_GetURLSpecFromFile(file, textStr);
            if (!textStr.IsEmpty()) {
              dropText.Assign(ToNewCString(textStr));
              rv = NS_OK;
            }
          }
        } 
      } 
    } 
  } 

    
    
  if (NS_SUCCEEDED(rv)) {

      
    nsXPIDLCString titleText;
    if (isUrl &&
        pditem->hstrTargetName &&
        NS_SUCCEEDED(GetAtom(pditem->hstrTargetName, getter_Copies(titleText))))
      for (char* ptr=strchr(titleText.BeginWriting(),'\n'); ptr; ptr=strchr(ptr, '\n'))
        *ptr = ' ';

    rv = NativeDataToTransferable( dropText.get(), titleText.get(), isUrl);
  }

    
  if (!*rendering)
    DrgSendTransferMsg( pditem->hwndItem, DM_ENDCONVERSATION,
                        (MPARAM)pditem->ulItemID,
                        (MPARAM)DMFL_TARGETSUCCESSFUL);

  return (rv);
}









NS_IMETHODIMP nsDragService::RenderCompleteMsg(PDRAGTRANSFER pdxfer,
                                        USHORT usResult, PRUint32* dragFlags)
{
  nsresult rv = NS_ERROR_FAILURE;
  if (!mDoingDrag || !pdxfer)
    return rv;

    
  if (!mSourceNode)
    rv = NativeRenderComplete(pdxfer, usResult);

    
  PDRAGINFO pdinfo = (PDRAGINFO)MAKEULONG(0x2c, HIUSHORT(pdxfer->pditem));

  DrgDeleteStrHandle(pdxfer->hstrSelectedRMF);
  DrgDeleteStrHandle(pdxfer->hstrRenderToName);
  DrgFreeDragtransfer(pdxfer);

    
  if (pdinfo && !mSourceNode) {
    DrgDeleteDraginfoStrHandles(pdinfo);
    DrgFreeDraginfo(pdinfo);
  }

    
  if (!dragFlags)
    return (ExitSession(dragFlags));

    
  *dragFlags = DND_EXITSESSION;
  if (NS_SUCCEEDED(rv))
    *dragFlags |= DND_DISPATCHEVENT;

    
  return NS_OK;
}






NS_IMETHODIMP nsDragService::NativeRenderComplete(PDRAGTRANSFER pdxfer,
                                                  USHORT usResult)
{
  nsresult rv = NS_ERROR_FAILURE;
  nsXPIDLCString rmf;

    
  if (NS_SUCCEEDED(GetAtom(pdxfer->hstrSelectedRMF, getter_Copies(rmf)))) {
    nsXPIDLCString dropText;
    if (!strcmp(rmf.get(), DTSHARE_RMF))
      rv = RenderToDTShareComplete(pdxfer, usResult, getter_Copies(dropText));
    else
    if (!strcmp(rmf.get(), OS2FILE_TXTRMF) ||
        !strcmp(rmf.get(), OS2FILE_UNKRMF))
      rv = RenderToOS2FileComplete(pdxfer, usResult, PR_TRUE,
                                   getter_Copies(dropText));

    if (NS_SUCCEEDED(rv)) {
      PRBool isUrl = PR_FALSE;
      IsDataFlavorSupported(kURLMime, &isUrl);
      rv = NativeDataToTransferable( dropText.get(), 0, isUrl);
    }
  }

  DrgSendTransferMsg(pdxfer->hwndClient, DM_ENDCONVERSATION,
                     (MPARAM)pdxfer->ulTargetInfo,
                     (MPARAM)DMFL_TARGETSUCCESSFUL);

  return rv;
}






NS_IMETHODIMP nsDragService::NativeDataToTransferable( PCSZ pszText,
                                                PCSZ pszTitle, PRBool isUrl)
{
  nsresult rv = NS_ERROR_FAILURE;
    
 if (!mSourceDataItems)
    return rv;

  nsCOMPtr<nsISupports> genericItem;
  mSourceDataItems->GetElementAt(0, getter_AddRefs(genericItem));
  nsCOMPtr<nsITransferable> trans (do_QueryInterface(genericItem));
  if (!trans)
    return rv;

    
  if (!isUrl) {
    trans->RemoveDataFlavor(kURLMime);
    trans->RemoveDataFlavor(kHTMLMime);
  }

    
    
  if (!pszText || !*pszText) {
    if (isUrl && pszTitle && *pszTitle) {
      nsXPIDLString outTitle;
      if (CodepageToUnicode(nsDependentCString(pszTitle),
                                               getter_Copies(outTitle))) {
        nsCOMPtr<nsISupportsString> urlPrimitive(
                        do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID));
        if (urlPrimitive ) {
          urlPrimitive->SetData(outTitle);
          trans->SetTransferData(kURLDescriptionMime, urlPrimitive,
                                 2*outTitle.Length());
        }
      }
    }
    return NS_OK;
  }

  nsXPIDLString outText;
  if (!CodepageToUnicode(nsDependentCString(pszText), getter_Copies(outText)))
    return rv;

  if (isUrl) {

      
    nsXPIDLString outTitle;
    if (pszTitle && *pszTitle) {
      if (!CodepageToUnicode(nsDependentCString(pszTitle),
                             getter_Copies(outTitle)))
        return rv;
    }
    else {
      PRUint32 len;
      nsCOMPtr<nsISupports> genericData;
      if (NS_SUCCEEDED(trans->GetTransferData(kURLDescriptionMime,
                                   getter_AddRefs(genericData), &len))) {
        nsCOMPtr<nsISupportsString> strObject(do_QueryInterface(genericData));
        if (strObject)
          strObject->GetData(outTitle);
      }
    }

      
    nsCOMPtr<nsISupportsString> urlPrimitive(
                            do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID));
    if (urlPrimitive ) {
      if (outTitle.IsEmpty()) {
        urlPrimitive->SetData(outText);
        trans->SetTransferData(kURLMime, urlPrimitive, 2*outText.Length());
      }
      else {
        nsString urlStr( outText + NS_LITERAL_STRING("\n") + outTitle);
        urlPrimitive->SetData(urlStr);
        trans->SetTransferData(kURLMime, urlPrimitive, 2*urlStr.Length());
      }
      rv = NS_OK;
    }

      
      
    nsCOMPtr<nsISupportsString> htmlPrimitive(
                            do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID));
    if (htmlPrimitive ) {
      nsString htmlStr;
      nsCOMPtr<nsIURI> uri;

      rv = NS_ERROR_FAILURE;
      if (NS_SUCCEEDED(NS_NewURI(getter_AddRefs(uri), pszText))) {
        nsCOMPtr<nsIURL> url (do_QueryInterface(uri));
        if (url) {
          nsCAutoString extension;
          url->GetFileExtension(extension);
          if (!extension.IsEmpty()) {
            if (extension.LowerCaseEqualsLiteral("gif") ||
                extension.LowerCaseEqualsLiteral("jpg") ||
                extension.LowerCaseEqualsLiteral("png") ||
                extension.LowerCaseEqualsLiteral("jpeg"))
              rv = NS_OK;
          }
        }
      }

      if (NS_SUCCEEDED(rv))
        htmlStr.Assign(NS_LITERAL_STRING("<img src=\"") +
                       outText +
                       NS_LITERAL_STRING("\" alt=\"") +
                       outTitle +
                       NS_LITERAL_STRING("\"/>") );
      else
        htmlStr.Assign(NS_LITERAL_STRING("<a href=\"") +
                       outText +
                       NS_LITERAL_STRING("\">") +
                       (outTitle.IsEmpty() ? outText : outTitle) +
                       NS_LITERAL_STRING("</a>") );

      htmlPrimitive->SetData(htmlStr);
      trans->SetTransferData(kHTMLMime, htmlPrimitive, 2*htmlStr.Length());
      rv = NS_OK;
    }
  }

    
  nsCOMPtr<nsISupportsString> textPrimitive(
                            do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID));
  if (textPrimitive ) {
    textPrimitive->SetData(nsDependentString(outText));
    trans->SetTransferData(kUnicodeMime, textPrimitive, 2*outText.Length());
    rv = NS_OK;
  }

    
  return rv;
}








nsresult RenderToOS2File( PDRAGITEM pditem, HWND hwnd)
{
  nsresult rv = NS_ERROR_FAILURE;
  nsXPIDLCString fileName;

  if (NS_SUCCEEDED(GetTempFileName(getter_Copies(fileName)))) {
    const char * pszRMF;
    if (DrgVerifyRMF(pditem, "DRM_OS2FILE", "DRF_TEXT"))
      pszRMF = OS2FILE_TXTRMF;
    else
      pszRMF = OS2FILE_UNKRMF;

    rv = RequestRendering( pditem, hwnd, pszRMF, fileName.get());
  }

  return rv;
}





nsresult RenderToOS2FileComplete(PDRAGTRANSFER pdxfer, USHORT usResult,
                                 PRBool content, char** outText)
{
  nsresult rv = NS_ERROR_FAILURE;

    
  content = PR_TRUE;

  if (usResult & DMFL_RENDEROK) {
    if (NS_SUCCEEDED(GetAtom( pdxfer->hstrRenderToName, &gTempFile))) {
      if (content)
        rv = GetFileContents(gTempFile, outText);
      else {
        nsCOMPtr<nsILocalFile> file;
        if (NS_SUCCEEDED(NS_NewNativeLocalFile(nsDependentCString(gTempFile),
                                         PR_TRUE, getter_AddRefs(file)))) {
          nsCAutoString textStr;
          NS_GetURLSpecFromFile(file, textStr);
          if (!textStr.IsEmpty()) {
            *outText = ToNewCString(textStr);
            rv = NS_OK;
          }
        }
      }
    }
  }
    

  return rv;
}






nsresult RenderToDTShare( PDRAGITEM pditem, HWND hwnd)
{
  nsresult rv;
  void *   pMem;

#ifdef MOZ_OS2_HIGH_MEMORY
  APIRET rc = DosAllocSharedMem( &pMem, DTSHARE_NAME, 0x100000,
                                 PAG_WRITE | PAG_READ | OBJ_ANY);
  if (rc != NO_ERROR &&
      rc != ERROR_ALREADY_EXISTS) { 
    
    
    
    rc = DosAllocSharedMem( &pMem, DTSHARE_NAME, 0x100000,
                            PAG_WRITE | PAG_READ);
  }
#else
  APIRET rc = DosAllocSharedMem( &pMem, DTSHARE_NAME, 0x100000,
                                 PAG_WRITE | PAG_READ);
#endif

  if (rc == ERROR_ALREADY_EXISTS)
    rc = DosGetNamedSharedMem( &pMem, DTSHARE_NAME,
                               PAG_WRITE | PAG_READ);
  if (rc)
    rv = NS_ERROR_FAILURE;
  else
    rv = RequestRendering( pditem, hwnd, DTSHARE_RMF, DTSHARE_NAME);

  return rv;
}





nsresult RenderToDTShareComplete(PDRAGTRANSFER pdxfer, USHORT usResult,
                                 char** outText)
{
  nsresult rv = NS_ERROR_FAILURE;
  void * pMem;
  char * pszText = 0;

  APIRET rc = DosGetNamedSharedMem( &pMem, DTSHARE_NAME, PAG_WRITE | PAG_READ);

  if (!rc) {
    if (usResult & DMFL_RENDEROK) {
      pszText = (char*)nsMemory::Alloc( ((ULONG*)pMem)[0] + 1);
      if (pszText) {
        strcpy(pszText, &((char*)pMem)[sizeof(ULONG)] );
        RemoveCarriageReturns(pszText);
        *outText = pszText;
        rv = NS_OK;
      }
    }
      
      
    DosFreeMem(pMem);
    DosFreeMem(pMem);
  }

  return rv;
}





nsresult RequestRendering( PDRAGITEM pditem, HWND hwnd, PCSZ pRMF, PCSZ pName)
{
  PDRAGTRANSFER pdxfer = DrgAllocDragtransfer( 1);
  if (!pdxfer)
    return NS_ERROR_FAILURE;
 
  pdxfer->cb = sizeof(DRAGTRANSFER);
  pdxfer->hwndClient = hwnd;
  pdxfer->pditem = pditem;
  pdxfer->hstrSelectedRMF = DrgAddStrHandle( pRMF);
  pdxfer->hstrRenderToName = 0;
  pdxfer->ulTargetInfo = pditem->ulItemID;
  pdxfer->usOperation = (USHORT)DO_COPY;
  pdxfer->fsReply = 0;
 
    
  if (pditem->fsControl & DC_PREPAREITEM)
    DrgSendTransferMsg( pditem->hwndItem, DM_RENDERPREPARE, (MPARAM)pdxfer, 0);
 
  pdxfer->hstrRenderToName = DrgAddStrHandle( pName);
 
    
  if ((pditem->fsControl & (DC_PREPARE | DC_PREPAREITEM)) == DC_PREPARE)
    DrgSendTransferMsg( pditem->hwndItem, DM_RENDERPREPARE, (MPARAM)pdxfer, 0);
 
    
  if (!DrgSendTransferMsg( pditem->hwndItem, DM_RENDER, (MPARAM)pdxfer, 0))
    return NS_ERROR_FAILURE;
 
  return NS_OK;
}






nsresult GetAtom( ATOM aAtom, char** outText)
{
  nsresult rv = NS_ERROR_FAILURE;

  ULONG ulInLength = DrgQueryStrNameLen(aAtom);
  if (ulInLength) {
    char* pszText = (char*)nsMemory::Alloc(++ulInLength);
    if (pszText) {
      DrgQueryStrName(aAtom, ulInLength, pszText);
      RemoveCarriageReturns(pszText);
      *outText = pszText;
      rv = NS_OK;
    }
  }
  return rv;
}






nsresult GetFileName(PDRAGITEM pditem, char** outText)
{
  nsresult rv = NS_ERROR_FAILURE;
  ULONG cntCnr = DrgQueryStrNameLen(pditem->hstrContainerName);
  ULONG cntSrc = DrgQueryStrNameLen(pditem->hstrSourceName);

  char* pszText = (char*)nsMemory::Alloc(cntCnr+cntSrc+1);
  if (pszText) {
    DrgQueryStrName(pditem->hstrContainerName, cntCnr+1, pszText);
    DrgQueryStrName(pditem->hstrSourceName, cntSrc+1, &pszText[cntCnr]);
    pszText[cntCnr+cntSrc] = 0;
    *outText = pszText;
    rv = NS_OK;
  }
  return rv;
}






nsresult GetFileContents(PCSZ pszPath, char** outText)
{
  nsresult rv = NS_ERROR_FAILURE;
  char* pszText = 0;

  if (pszPath) {
    FILE *fp = fopen(pszPath, "r");
    if (fp) {
      fseek(fp, 0, SEEK_END);
      ULONG filesize = ftell(fp);
      fseek(fp, 0, SEEK_SET);
      if (filesize > 0) {
        size_t readsize = (size_t)filesize;
        pszText = (char*)nsMemory::Alloc(readsize+1);
        if (pszText) {
          readsize = fread((void *)pszText, 1, readsize, fp);
          if (readsize) {
            pszText[readsize] = '\0';
            RemoveCarriageReturns(pszText);
            *outText = pszText;
            rv = NS_OK;
          }
          else {
            nsMemory::Free(pszText);
            pszText = 0;
          }
        }
      }
      fclose(fp);
    }
  }

  return rv;
}





nsresult GetTempFileName(char** outText)
{
  char * pszText = (char*)nsMemory::Alloc(CCHMAXPATH);
  if (!pszText)
    return NS_ERROR_FAILURE;

  const char * pszPath;
  if (!DosScanEnv("TEMP", &pszPath) || !DosScanEnv("TMP", &pszPath))
    strcpy(pszText, pszPath);
  else
    if (DosQueryPathInfo(".\\.", FIL_QUERYFULLNAME, pszText, CCHMAXPATH))
      pszText[0] = 0;

  strcat(pszText, "\\");
  strcat(pszText, OS2FILE_NAME);
  *outText = pszText;

  return NS_OK;
}







void SaveTypeAndSource(nsILocalFile *file, nsIDOMDocument *domDoc,
                       PCSZ pszType)
{
  if (!file)
    return;

  nsCOMPtr<nsILocalFileOS2> os2file(do_QueryInterface(file));
  if (!os2file ||
      NS_FAILED(os2file->SetFileTypes(nsDependentCString(pszType))))
    return;

  
  
  if (!domDoc)
    return;

  nsCOMPtr<nsIDocument> doc(do_QueryInterface(domDoc));
  if (!doc)
    return;

  
  
  
  
  
  nsIDocument *prevDoc;
  nsIDocument *currDoc = doc;
  nsIDocument *nextDoc = doc;
  do {
    prevDoc = currDoc;
    currDoc = nextDoc;
    nextDoc = currDoc->GetParentDocument();
  } while (nextDoc);

  nsIURI* srcUri = prevDoc->GetDocumentURI();
  if (!srcUri)
    return;

  
  PRBool ignore = PR_FALSE;
  srcUri->SchemeIs("chrome", &ignore);
  if (ignore)
    return;

  nsCAutoString url;
  srcUri->GetSpec(url);
  os2file->SetFileSource(url);

  return;
}






int UnicodeToCodepage(const nsAString& aString, char **aResult)
{
  nsAutoCharBuffer buffer;
  PRInt32 bufLength;
  WideCharToMultiByte(0, PromiseFlatString(aString).get(), aString.Length(),
                      buffer, bufLength);
  *aResult = ToNewCString(nsDependentCString(buffer.Elements()));
  return bufLength;
}



int CodepageToUnicode(const nsACString& aString, PRUnichar **aResult)
{
  nsAutoChar16Buffer buffer;
  PRInt32 bufLength;
  MultiByteToWideChar(0, PromiseFlatCString(aString).get(),
                      aString.Length(), buffer, bufLength);
  *aResult = ToNewUnicode(nsDependentString(buffer.Elements()));
  return bufLength;
}






void RemoveCarriageReturns(char * pszText)
{
  ULONG  cnt;
  char * next;
  char * source;
  char * target;

  target = strchr(pszText, 0x0d);
  if (!target)
    return;

  source = target + 1;

  while ((next = strchr(source, 0x0d)) != 0) {

    cnt = next - source;
    memcpy(target, source, cnt);
    target += cnt;
    source = next + 1;

  }

  strcpy(target, source);
  return;
}


