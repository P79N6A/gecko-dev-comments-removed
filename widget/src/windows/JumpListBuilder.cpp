






































#if MOZ_WINSDK_TARGETVER >= MOZ_NTDDI_WIN7

#include "JumpListBuilder.h"

#include "nsError.h"
#include "nsCOMPtr.h"
#include "nsServiceManagerUtils.h"
#include "nsAutoPtr.h"
#include "nsString.h"
#include "nsArrayUtils.h"
#include "nsIMutableArray.h"
#include "nsWidgetsCID.h"
#include "WinTaskbar.h"
#include "nsDirectoryServiceUtils.h"
#include "nsISimpleEnumerator.h"
#include "mozilla/Preferences.h"
#include "imgIContainer.h"
#include "imgITools.h"
#include "nsStringStream.h"
#include "nsNetUtil.h"
#include "nsThreadUtils.h"

namespace mozilla {
namespace widget {

static NS_DEFINE_CID(kJumpListItemCID,     NS_WIN_JUMPLISTITEM_CID);
static NS_DEFINE_CID(kJumpListLinkCID,     NS_WIN_JUMPLISTLINK_CID);
static NS_DEFINE_CID(kJumpListShortcutCID, NS_WIN_JUMPLISTSHORTCUT_CID);


extern const wchar_t *gMozillaJumpListIDGeneric;

bool JumpListBuilder::sBuildingList = false;
const char kPrefTaskbarEnabled[] = "browser.taskbar.lists.enabled";

NS_IMPL_ISUPPORTS2(JumpListBuilder, nsIJumpListBuilder, nsIObserver)
NS_IMPL_ISUPPORTS1(AsyncFaviconDataReady, nsIFaviconDataCallback)
NS_IMPL_THREADSAFE_ISUPPORTS1(AsyncWriteIconToDisk, nsIRunnable)
NS_IMPL_THREADSAFE_ISUPPORTS1(AsyncDeleteIconFromDisk, nsIRunnable)
NS_IMPL_THREADSAFE_ISUPPORTS1(AsyncDeleteAllFaviconsFromDisk, nsIRunnable)

JumpListBuilder::JumpListBuilder() :
  mMaxItems(0),
  mHasCommit(PR_FALSE)
{
  ::CoInitialize(NULL);
  
  CoCreateInstance(CLSID_DestinationList, NULL, CLSCTX_INPROC_SERVER,
                   IID_ICustomDestinationList, getter_AddRefs(mJumpListMgr));

  NS_NewThread(getter_AddRefs(mIOThread));
  Preferences::AddStrongObserver(this, kPrefTaskbarEnabled);
}

JumpListBuilder::~JumpListBuilder()
{
  mIOThread->Shutdown();
  Preferences::RemoveObserver(this, kPrefTaskbarEnabled);
  mJumpListMgr = nsnull;
  ::CoUninitialize();
}


NS_IMETHODIMP JumpListBuilder::GetAvailable(PRInt16 *aAvailable)
{
  *aAvailable = PR_FALSE;

  if (mJumpListMgr)
    *aAvailable = PR_TRUE;

  return NS_OK;
}


NS_IMETHODIMP JumpListBuilder::GetIsListCommitted(bool *aCommit)
{
  *aCommit = mHasCommit;

  return NS_OK;
}


NS_IMETHODIMP JumpListBuilder::GetMaxListItems(PRInt16 *aMaxItems)
{
  if (!mJumpListMgr)
    return NS_ERROR_NOT_AVAILABLE;

  *aMaxItems = 0;

  if (sBuildingList) {
    *aMaxItems = mMaxItems;
    return NS_OK;
  }

  IObjectArray *objArray;
  if (SUCCEEDED(mJumpListMgr->BeginList(&mMaxItems, IID_PPV_ARGS(&objArray)))) {
    *aMaxItems = mMaxItems;

    if (objArray)
      objArray->Release();

    mJumpListMgr->AbortList();
  }

  return NS_OK;
}


NS_IMETHODIMP JumpListBuilder::InitListBuild(nsIMutableArray *removedItems, bool *_retval)
{
  NS_ENSURE_ARG_POINTER(removedItems);

  *_retval = PR_FALSE;

  if (!mJumpListMgr)
    return NS_ERROR_NOT_AVAILABLE;

  if(sBuildingList)
    AbortListBuild();

  IObjectArray *objArray;

  if (SUCCEEDED(mJumpListMgr->BeginList(&mMaxItems, IID_PPV_ARGS(&objArray)))) {
    if (objArray) {
      TransferIObjectArrayToIMutableArray(objArray, removedItems);
      objArray->Release();
    }

    RemoveIconCacheForItems(removedItems);

    sBuildingList = PR_TRUE;
    *_retval = PR_TRUE;
    return NS_OK;
  }

  return NS_OK;
}



nsresult JumpListBuilder::RemoveIconCacheForItems(nsIMutableArray *items) 
{
  NS_ENSURE_ARG_POINTER(items);
  
  nsresult rv;
  PRUint32 length;
  items->GetLength(&length);
  for (PRUint32 i = 0; i < length; ++i) {

    
    nsCOMPtr<nsIJumpListItem> item = do_QueryElementAt(items, i);
    if (!item) {
      continue;
    }
    PRInt16 type;
    if (NS_FAILED(item->GetType(&type))) {
      continue;
    }

    
    if (type == nsIJumpListItem::JUMPLIST_ITEM_SHORTCUT) {
      nsCOMPtr<nsIJumpListShortcut> shortcut = do_QueryInterface(item);
      if (shortcut) {
        nsCOMPtr<nsIURI> uri;
        rv = shortcut->GetFaviconPageUri(getter_AddRefs(uri));
        if (NS_SUCCEEDED(rv) && uri) {
          
          
          
          nsCAutoString spec;
          nsresult rv = uri->GetSpec(spec);
          NS_ENSURE_SUCCESS(rv, rv);

          nsCOMPtr<nsIRunnable> event 
            = new AsyncDeleteIconFromDisk(NS_ConvertUTF8toUTF16(spec));
          mIOThread->Dispatch(event, NS_DISPATCH_NORMAL);

          
          
          
          
          shortcut->SetFaviconPageUri(nsnull);
        }
      }
    }

  } 

  return NS_OK;
}


nsresult JumpListBuilder::RemoveIconCacheForAllItems() 
{
  
  nsCOMPtr<nsIFile> jumpListCacheDir;
  nsresult rv = NS_GetSpecialDirectory("ProfLDS", 
                                       getter_AddRefs(jumpListCacheDir));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = jumpListCacheDir->AppendNative(nsDependentCString(JumpListItem::kJumpListCacheDir));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsISimpleEnumerator> entries;
  rv = jumpListCacheDir->GetDirectoryEntries(getter_AddRefs(entries));
  NS_ENSURE_SUCCESS(rv, rv);
  
  
  do {
    bool hasMore = false;
    if (NS_FAILED(entries->HasMoreElements(&hasMore)) || !hasMore)
      break;

    nsCOMPtr<nsISupports> supp;
    if (NS_FAILED(entries->GetNext(getter_AddRefs(supp))))
      break;

    nsCOMPtr<nsIFile> currFile(do_QueryInterface(supp));
    nsAutoString path;
    if (NS_FAILED(currFile->GetPath(path)))
      continue;

    PRInt32 len = path.Length();
    if (StringTail(path, 4).LowerCaseEqualsASCII(".ico")) {
      
      bool exists;
      if (NS_FAILED(currFile->Exists(&exists)) || !exists)
        continue;

      
      currFile->Remove(PR_FALSE);
    }
  } while(true);

  return NS_OK;
}


NS_IMETHODIMP JumpListBuilder::AddListToBuild(PRInt16 aCatType, nsIArray *items, const nsAString &catName, bool *_retval)
{
  nsresult rv;

  *_retval = PR_FALSE;

  if (!mJumpListMgr)
    return NS_ERROR_NOT_AVAILABLE;

  switch(aCatType) {
    case nsIJumpListBuilder::JUMPLIST_CATEGORY_TASKS:
    {
      NS_ENSURE_ARG_POINTER(items);

      HRESULT hr;
      nsRefPtr<IObjectCollection> collection;
      hr = CoCreateInstance(CLSID_EnumerableObjectCollection, NULL, CLSCTX_INPROC_SERVER,
                            IID_IObjectCollection, getter_AddRefs(collection));
      if (FAILED(hr))
        return NS_ERROR_UNEXPECTED;

      
      PRUint32 length;
      items->GetLength(&length);
      for (PRUint32 i = 0; i < length; ++i) {
        nsCOMPtr<nsIJumpListItem> item = do_QueryElementAt(items, i);
        if (!item)
          continue;
        
        if (IsSeparator(item)) {
          nsRefPtr<IShellLinkW> link;
          rv = JumpListSeparator::GetSeparator(link);
          if (NS_FAILED(rv))
            return rv;
          collection->AddObject(link);
          continue;
        }
        
        nsRefPtr<IShellLinkW> link;
        rv = JumpListShortcut::GetShellLink(item, link, mIOThread);
        if (NS_FAILED(rv))
          return rv;
        collection->AddObject(link);
      }

      
      nsRefPtr<IObjectArray> pArray;
      hr = collection->QueryInterface(IID_IObjectArray, getter_AddRefs(pArray));
      if (FAILED(hr))
        return NS_ERROR_UNEXPECTED;

      
      hr = mJumpListMgr->AddUserTasks(pArray);
      if (SUCCEEDED(hr))
        *_retval = PR_TRUE;
      return NS_OK;
    }
    break;
    case nsIJumpListBuilder::JUMPLIST_CATEGORY_RECENT:
    {
      if (SUCCEEDED(mJumpListMgr->AppendKnownCategory(KDC_RECENT)))
        *_retval = PR_TRUE;
      return NS_OK;
    }
    break;
    case nsIJumpListBuilder::JUMPLIST_CATEGORY_FREQUENT:
    {
      if (SUCCEEDED(mJumpListMgr->AppendKnownCategory(KDC_FREQUENT)))
        *_retval = PR_TRUE;
      return NS_OK;
    }
    break;
    case nsIJumpListBuilder::JUMPLIST_CATEGORY_CUSTOMLIST:
    {
      NS_ENSURE_ARG_POINTER(items);

      if (catName.IsEmpty())
        return NS_ERROR_INVALID_ARG;

      HRESULT hr;
      nsRefPtr<IObjectCollection> collection;
      hr = CoCreateInstance(CLSID_EnumerableObjectCollection, NULL, CLSCTX_INPROC_SERVER,
                            IID_IObjectCollection, getter_AddRefs(collection));
      if (FAILED(hr))
        return NS_ERROR_UNEXPECTED;

      PRUint32 length;
      items->GetLength(&length);
      for (PRUint32 i = 0; i < length; ++i) {
        nsCOMPtr<nsIJumpListItem> item = do_QueryElementAt(items, i);
        if (!item)
          continue;
        PRInt16 type;
        if (NS_FAILED(item->GetType(&type)))
          continue;
        switch(type) {
          case nsIJumpListItem::JUMPLIST_ITEM_SEPARATOR:
          {
            nsRefPtr<IShellLinkW> shellItem;
            rv = JumpListSeparator::GetSeparator(shellItem);
            if (NS_FAILED(rv))
              return rv;
            collection->AddObject(shellItem);
          }
          break;
          case nsIJumpListItem::JUMPLIST_ITEM_LINK:
          {
            nsRefPtr<IShellItem2> shellItem;
            rv = JumpListLink::GetShellItem(item, shellItem);
            if (NS_FAILED(rv))
              return rv;
            collection->AddObject(shellItem);
          }
          break;
          case nsIJumpListItem::JUMPLIST_ITEM_SHORTCUT:
          {
            nsRefPtr<IShellLinkW> shellItem;
            rv = JumpListShortcut::GetShellLink(item, shellItem, mIOThread);
            if (NS_FAILED(rv))
              return rv;
            collection->AddObject(shellItem);
          }
          break;
        }
      }

      
      nsRefPtr<IObjectArray> pArray;
      hr = collection->QueryInterface(IID_IObjectArray, (LPVOID*)&pArray);
      if (FAILED(hr))
        return NS_ERROR_UNEXPECTED;

      
      hr = mJumpListMgr->AppendCategory(catName.BeginReading(), pArray);
      if (SUCCEEDED(hr))
        *_retval = PR_TRUE;
      return NS_OK;
    }
    break;
  }
  return NS_OK;
}


NS_IMETHODIMP JumpListBuilder::AbortListBuild()
{
  if (!mJumpListMgr)
    return NS_ERROR_NOT_AVAILABLE;

  mJumpListMgr->AbortList();
  sBuildingList = PR_FALSE;

  return NS_OK;
}


NS_IMETHODIMP JumpListBuilder::CommitListBuild(bool *_retval)
{
  *_retval = PR_FALSE;

  if (!mJumpListMgr)
    return NS_ERROR_NOT_AVAILABLE;

  HRESULT hr = mJumpListMgr->CommitList();
  sBuildingList = PR_FALSE;

  
  if (SUCCEEDED(hr)) {
    *_retval = PR_TRUE;
    mHasCommit = PR_TRUE;
  }

  return NS_OK;
}


NS_IMETHODIMP JumpListBuilder::DeleteActiveList(bool *_retval)
{
  *_retval = PR_FALSE;

  if (!mJumpListMgr)
    return NS_ERROR_NOT_AVAILABLE;

  if(sBuildingList)
    AbortListBuild();

  nsAutoString uid;
  if (!WinTaskbar::GetAppUserModelID(uid))
    return NS_OK;

  if (SUCCEEDED(mJumpListMgr->DeleteList(uid.get())))
    *_retval = PR_TRUE;

  return NS_OK;
}



bool JumpListBuilder::IsSeparator(nsCOMPtr<nsIJumpListItem>& item)
{
  PRInt16 type;
  item->GetType(&type);
  if (NS_FAILED(item->GetType(&type)))
    return PR_FALSE;
    
  if (type == nsIJumpListItem::JUMPLIST_ITEM_SEPARATOR)
    return PR_TRUE;
  return PR_FALSE;
}



nsresult JumpListBuilder::TransferIObjectArrayToIMutableArray(IObjectArray *objArray, nsIMutableArray *removedItems)
{
  NS_ENSURE_ARG_POINTER(objArray);
  NS_ENSURE_ARG_POINTER(removedItems);

  nsresult rv;

  PRUint32 count = 0;
  objArray->GetCount(&count);

  nsCOMPtr<nsIJumpListItem> item;

  for (PRUint32 idx = 0; idx < count; idx++) {
    IShellLinkW * pLink = nsnull;
    IShellItem * pItem = nsnull;

    if (SUCCEEDED(objArray->GetAt(idx, IID_IShellLinkW, (LPVOID*)&pLink))) {
      nsCOMPtr<nsIJumpListShortcut> shortcut = 
        do_CreateInstance(kJumpListShortcutCID, &rv);
      if (NS_FAILED(rv))
        return NS_ERROR_UNEXPECTED;
      rv = JumpListShortcut::GetJumpListShortcut(pLink, shortcut);
      item = do_QueryInterface(shortcut);
    }
    else if (SUCCEEDED(objArray->GetAt(idx, IID_IShellItem, (LPVOID*)&pItem))) {
      nsCOMPtr<nsIJumpListLink> link = 
        do_CreateInstance(kJumpListLinkCID, &rv);
      if (NS_FAILED(rv))
        return NS_ERROR_UNEXPECTED;
      rv = JumpListLink::GetJumpListLink(pItem, link);
      item = do_QueryInterface(link);
    }

    if (pLink)
      pLink->Release();
    if (pItem)
      pItem->Release();

    if (NS_SUCCEEDED(rv)) {
      removedItems->AppendElement(item, PR_FALSE);
    }
  }
  return NS_OK;
}

NS_IMETHODIMP JumpListBuilder::Observe(nsISupports* aSubject,
                                        const char* aTopic,
                                        const PRUnichar* aData)
{
  if (nsDependentString(aData).EqualsASCII(kPrefTaskbarEnabled)) {
    bool enabled = Preferences::GetBool(kPrefTaskbarEnabled, true);
    if (!enabled) {
      
      nsCOMPtr<nsIRunnable> event = new AsyncDeleteAllFaviconsFromDisk();
      mIOThread->Dispatch(event, NS_DISPATCH_NORMAL);
    }
  }
  return NS_OK;
}


AsyncFaviconDataReady::AsyncFaviconDataReady(nsIURI *aNewURI, 
                                             nsCOMPtr<nsIThread> &aIOThread) 
                      : mNewURI(aNewURI), 
                        mIOThread(aIOThread)
{
}

NS_IMETHODIMP
AsyncFaviconDataReady::OnFaviconDataAvailable(nsIURI *aFaviconURI, 
                                              PRUint32 aDataLen,
                                              const PRUint8 *aData, 
                                              const nsACString &aMimeType)
{
  if (!aDataLen || !aData) {
    return NS_OK;
  }

  nsCOMPtr<nsIFile> icoFile;
  nsresult rv = JumpListShortcut::GetOutputIconPath(mNewURI, icoFile);
  NS_ENSURE_SUCCESS(rv, rv);
  nsAutoString path;
  rv = icoFile->GetPath(path);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  const fallible_t fallible = fallible_t();
  PRUint8 *data = new (fallible) PRUint8[aDataLen];
  if (!data) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  memcpy(data, aData, aDataLen);

  
  nsCOMPtr<nsIRunnable> event = new AsyncWriteIconToDisk(path, aMimeType, 
                                                         data, 
                                                         aDataLen);
  mIOThread->Dispatch(event, NS_DISPATCH_NORMAL);

  return NS_OK;
}


AsyncWriteIconToDisk::AsyncWriteIconToDisk(const nsAString &aIconPath,
                                           const nsACString &aMimeTypeOfInputData,
                                           PRUint8 *aBuffer, 
                                           PRUint32 aBufferLength)
                     : mIconPath(aIconPath),
                       mMimeTypeOfInputData(aMimeTypeOfInputData),
                       mBuffer(aBuffer),
                       mBufferLength(aBufferLength)

{
}

NS_IMETHODIMP AsyncWriteIconToDisk::Run()
{
  NS_PRECONDITION(!NS_IsMainThread(), "Should not be called on the main thread.");

  
  nsCOMPtr<nsIInputStream> stream;
  nsresult rv = NS_NewByteInputStream(getter_AddRefs(stream),
                                      reinterpret_cast<const char*>(mBuffer.get()), 
                                      mBufferLength,
                                      NS_ASSIGNMENT_DEPEND);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<imgIContainer> container;
  nsCOMPtr<imgITools> imgtool = do_CreateInstance("@mozilla.org/image/tools;1");
  rv = imgtool->DecodeImageData(stream, mMimeTypeOfInputData, 
                                getter_AddRefs(container));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  
  PRInt32 systemIconWidth = GetSystemMetrics(SM_CXSMICON);
  PRInt32 systemIconHeight = GetSystemMetrics(SM_CYSMICON);
  if (systemIconWidth == 0 || systemIconHeight == 0) {
    systemIconWidth = 16;
    systemIconHeight = 16;
  }
  
  mMimeTypeOfInputData.AssignLiteral("image/vnd.microsoft.icon");
  nsCOMPtr<nsIInputStream> iconStream;
  rv = imgtool->EncodeScaledImage(container, mMimeTypeOfInputData,
                                  systemIconWidth,
                                  systemIconHeight,
                                  getter_AddRefs(iconStream));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsILocalFile> icoFile
    = do_CreateInstance("@mozilla.org/file/local;1");
  NS_ENSURE_TRUE(icoFile, NS_ERROR_FAILURE);
  rv = icoFile->InitWithPath(mIconPath);

  
  nsCOMPtr<nsIOutputStream> outputStream;
  rv = NS_NewLocalFileOutputStream(getter_AddRefs(outputStream), icoFile);
  NS_ENSURE_SUCCESS(rv, rv);

  
  PRUint32 bufSize;
  rv = iconStream->Available(&bufSize);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  nsCOMPtr<nsIOutputStream> bufferedOutputStream;
  rv = NS_NewBufferedOutputStream(getter_AddRefs(bufferedOutputStream),
                                  outputStream, bufSize);
  NS_ENSURE_SUCCESS(rv, rv);

  
  PRUint32 wrote;
  rv = bufferedOutputStream->WriteFrom(iconStream, bufSize, &wrote);
  NS_ASSERTION(bufSize == wrote, "Icon wrote size should be equal to requested write size");

  
  bufferedOutputStream->Close();
  outputStream->Close();
  return rv;
}

AsyncWriteIconToDisk::~AsyncWriteIconToDisk()
{
}

AsyncDeleteIconFromDisk::AsyncDeleteIconFromDisk(const nsAString &aIconPath)
                        : mIconPath(aIconPath)
{
}

NS_IMETHODIMP AsyncDeleteIconFromDisk::Run()
{
  
  nsCOMPtr<nsILocalFile> icoFile = do_CreateInstance("@mozilla.org/file/local;1");
  NS_ENSURE_TRUE(icoFile, NS_ERROR_FAILURE);
  nsresult rv = icoFile->InitWithPath(mIconPath);
  NS_ENSURE_SUCCESS(rv, rv);

  
  bool exists;
  rv = icoFile->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (StringTail(mIconPath, 4).LowerCaseEqualsASCII(".ico")) {
    
    bool exists;
    if (NS_FAILED(icoFile->Exists(&exists)) || !exists)
      return NS_ERROR_FAILURE;

    
    icoFile->Remove(PR_FALSE);
  }

  return NS_OK;
}

AsyncDeleteIconFromDisk::~AsyncDeleteIconFromDisk()
{
}

AsyncDeleteAllFaviconsFromDisk::AsyncDeleteAllFaviconsFromDisk()
{
}

NS_IMETHODIMP AsyncDeleteAllFaviconsFromDisk::Run()
{
  
  nsCOMPtr<nsIFile> jumpListCacheDir;
  nsresult rv = NS_GetSpecialDirectory("ProfLDS", 
    getter_AddRefs(jumpListCacheDir));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = jumpListCacheDir->AppendNative(nsDependentCString(JumpListItem::kJumpListCacheDir));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsISimpleEnumerator> entries;
  rv = jumpListCacheDir->GetDirectoryEntries(getter_AddRefs(entries));
  NS_ENSURE_SUCCESS(rv, rv);

  
  do {
    bool hasMore = false;
    if (NS_FAILED(entries->HasMoreElements(&hasMore)) || !hasMore)
      break;

    nsCOMPtr<nsISupports> supp;
    if (NS_FAILED(entries->GetNext(getter_AddRefs(supp))))
      break;

    nsCOMPtr<nsIFile> currFile(do_QueryInterface(supp));
    nsAutoString path;
    if (NS_FAILED(currFile->GetPath(path)))
      continue;

    PRInt32 len = path.Length();
    if (StringTail(path, 4).LowerCaseEqualsASCII(".ico")) {
      
      bool exists;
      if (NS_FAILED(currFile->Exists(&exists)) || !exists)
        continue;

      
      currFile->Remove(PR_FALSE);
    }
  } while(true);

  return NS_OK;
}

AsyncDeleteAllFaviconsFromDisk::~AsyncDeleteAllFaviconsFromDisk()
{
}


} 
} 

#endif 
