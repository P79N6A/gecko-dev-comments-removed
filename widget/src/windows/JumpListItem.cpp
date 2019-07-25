






































#if MOZ_WINSDK_TARGETVER >= MOZ_NTDDI_WIN7

#include "JumpListItem.h"

#include <shellapi.h>
#include <propvarutil.h>
#include <propkey.h>

#include "nsIFile.h"
#include "nsILocalFile.h"
#include "nsNetUtil.h"
#include "nsCRT.h"
#include "nsNetCID.h"
#include "nsCExternalHandlerService.h"
#include "nsCycleCollectionParticipant.h"
#include "imgIContainer.h"
#include "imgITools.h"
#include "nsIFaviconService.h"
#include "mozilla/Preferences.h"
#include "nsStringStream.h"

namespace mozilla {
namespace widget {



JumpListLink::SHCreateItemFromParsingNamePtr JumpListLink::createItemFromParsingName = nsnull;
const PRUnichar JumpListLink::kSehllLibraryName[] =  L"shell32.dll";
const char JumpListItem::kJumpListCacheDir[] = "jumpListCache";
HMODULE JumpListLink::sShellDll = nsnull;


NS_IMPL_ISUPPORTS1(JumpListItem,
                   nsIJumpListItem)

NS_IMPL_ISUPPORTS_INHERITED1(JumpListSeparator,
                             JumpListItem,
                             nsIJumpListSeparator)

NS_IMPL_ISUPPORTS_INHERITED1(JumpListLink,
                             JumpListItem,
                             nsIJumpListLink)

NS_IMPL_CYCLE_COLLECTION_CLASS(JumpListShortcut)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(JumpListShortcut)
  NS_INTERFACE_MAP_ENTRY(nsIJumpListShortcut)
NS_INTERFACE_MAP_END_INHERITING(JumpListItem)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(JumpListShortcut)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mHandlerApp)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(JumpListShortcut)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mHandlerApp)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(JumpListShortcut)
NS_IMPL_CYCLE_COLLECTING_RELEASE(JumpListShortcut)


NS_IMETHODIMP JumpListItem::GetType(PRInt16 *aType)
{
  NS_ENSURE_ARG_POINTER(aType);

  *aType = mItemType;

  return NS_OK;
}


NS_IMETHODIMP JumpListItem::Equals(nsIJumpListItem *aItem, PRBool *aResult)
{
  NS_ENSURE_ARG_POINTER(aItem);

  *aResult = PR_FALSE;

  PRInt16 theType = nsIJumpListItem::JUMPLIST_ITEM_EMPTY;
  if (NS_FAILED(aItem->GetType(&theType)))
    return NS_OK;

  
  if (Type() != theType)
    return NS_OK;

  *aResult = PR_TRUE;

  return NS_OK;
}




NS_IMETHODIMP JumpListLink::GetUri(nsIURI **aURI)
{
  NS_IF_ADDREF(*aURI = mURI);

  return NS_OK;
}

NS_IMETHODIMP JumpListLink::SetUri(nsIURI *aURI)
{
  mURI = aURI;
  
  return NS_OK;
}


NS_IMETHODIMP JumpListLink::SetUriTitle(const nsAString &aUriTitle)
{
  mUriTitle.Assign(aUriTitle);

  return NS_OK;
}

NS_IMETHODIMP JumpListLink::GetUriTitle(nsAString& aUriTitle)
{
  aUriTitle.Assign(mUriTitle);
  
  return NS_OK;
}


NS_IMETHODIMP JumpListLink::GetUriHash(nsACString& aUriHash)
{
  if (!mURI)
    return NS_ERROR_NOT_AVAILABLE;

  return JumpListItem::HashURI(mCryptoHash, mURI, aUriHash);
}


NS_IMETHODIMP JumpListLink::CompareHash(nsIURI *aUri, PRBool *aResult)
{
  nsresult rv;

  if (!mURI) {
    *aResult = !aUri;
    return NS_OK;
  }

  NS_ENSURE_ARG_POINTER(aUri);

  nsCAutoString hash1, hash2;

  rv = JumpListItem::HashURI(mCryptoHash, mURI, hash1);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = JumpListItem::HashURI(mCryptoHash, aUri, hash2);
  NS_ENSURE_SUCCESS(rv, rv);

  *aResult = hash1.Equals(hash2);

  return NS_OK;
}


NS_IMETHODIMP JumpListLink::Equals(nsIJumpListItem *aItem, PRBool *aResult)
{
  NS_ENSURE_ARG_POINTER(aItem);

  nsresult rv;

  *aResult = PR_FALSE;

  PRInt16 theType = nsIJumpListItem::JUMPLIST_ITEM_EMPTY;
  if (NS_FAILED(aItem->GetType(&theType)))
    return NS_OK;

  
  if (Type() != theType)
    return NS_OK;

  nsCOMPtr<nsIJumpListLink> link = do_QueryInterface(aItem, &rv);
  if (NS_FAILED(rv))
    return rv;

  
  nsAutoString title;
  link->GetUriTitle(title);
  if (!mUriTitle.Equals(title))
    return NS_OK;

  
  nsCOMPtr<nsIURI> theUri;
  PRBool equals = PR_FALSE;
  if (NS_SUCCEEDED(link->GetUri(getter_AddRefs(theUri)))) {
    if (!theUri) {
      if (!mURI)
        *aResult = PR_TRUE;
      return NS_OK;
    }
    if (NS_SUCCEEDED(theUri->Equals(mURI, &equals)) && equals) {
      *aResult = PR_TRUE;
    }
  }

  return NS_OK;
}




NS_IMETHODIMP JumpListShortcut::GetApp(nsILocalHandlerApp **aApp)
{
  NS_IF_ADDREF(*aApp = mHandlerApp);
  
  return NS_OK;
}

NS_IMETHODIMP JumpListShortcut::SetApp(nsILocalHandlerApp *aApp)
{
  mHandlerApp = aApp;

  
  if (!ExecutableExists(mHandlerApp))
    return NS_ERROR_FILE_NOT_FOUND;

  return NS_OK;
}


NS_IMETHODIMP JumpListShortcut::GetIconIndex(PRInt32 *aIconIndex)
{
  NS_ENSURE_ARG_POINTER(aIconIndex);

  *aIconIndex = mIconIndex;
  return NS_OK;
}

NS_IMETHODIMP JumpListShortcut::SetIconIndex(PRInt32 aIconIndex)
{
  mIconIndex = aIconIndex;
  return NS_OK;
}


NS_IMETHODIMP JumpListShortcut::GetIconImageUri(nsIURI **aIconImageURI)
{
  NS_IF_ADDREF(*aIconImageURI = mIconImageURI);

  return NS_OK;
}

NS_IMETHODIMP JumpListShortcut::SetIconImageUri(nsIURI *aIconImageURI)
{
  mIconImageURI = aIconImageURI;
  return NS_OK;
}


NS_IMETHODIMP JumpListShortcut::Equals(nsIJumpListItem *aItem, PRBool *aResult)
{
  NS_ENSURE_ARG_POINTER(aItem);

  nsresult rv;

  *aResult = PR_FALSE;

  PRInt16 theType = nsIJumpListItem::JUMPLIST_ITEM_EMPTY;
  if (NS_FAILED(aItem->GetType(&theType)))
    return NS_OK;

  
  if (Type() != theType)
    return NS_OK;

  nsCOMPtr<nsIJumpListShortcut> shortcut = do_QueryInterface(aItem, &rv);
  if (NS_FAILED(rv))
    return rv;

  
  
  
  
  
  

  
  nsCOMPtr<nsILocalHandlerApp> theApp;
  PRBool equals = PR_FALSE;
  if (NS_SUCCEEDED(shortcut->GetApp(getter_AddRefs(theApp)))) {
    if (!theApp) {
      if (!mHandlerApp)
        *aResult = PR_TRUE;
      return NS_OK;
    }
    if (NS_SUCCEEDED(theApp->Equals(mHandlerApp, &equals)) && equals) {
      *aResult = PR_TRUE;
    }
  }

  return NS_OK;
}




nsresult JumpListSeparator::GetSeparator(nsRefPtr<IShellLinkW>& aShellLink)
{
  HRESULT hr;
  IShellLinkW* psl;

  
  hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, 
                        IID_IShellLinkW, (LPVOID*)&psl);
  if (FAILED(hr))
    return NS_ERROR_UNEXPECTED;

  IPropertyStore* pPropStore = nsnull;
  hr = psl->QueryInterface(IID_IPropertyStore, (LPVOID*)&pPropStore);
  if (FAILED(hr))
    return NS_ERROR_UNEXPECTED;

  PROPVARIANT pv;
  InitPropVariantFromBoolean(TRUE, &pv);

  pPropStore->SetValue(PKEY_AppUserModel_IsDestListSeparator, pv);
  pPropStore->Commit();
  pPropStore->Release();

  PropVariantClear(&pv);

  aShellLink = dont_AddRef(psl);

  return NS_OK;
}


static PRInt32 GetICOCacheSecondsTimeout() {

  
  
  
  
  
  const PRInt32 kSecondsPerDay = 86400;
  static PRBool alreadyObtained = PR_FALSE;
  static PRInt32 icoReCacheSecondsTimeout = kSecondsPerDay;
  if (alreadyObtained) {
    return icoReCacheSecondsTimeout;
  }

  
  const char PREF_ICOTIMEOUT[]  = "browser.taskbar.lists.icoTimeoutInSeconds";
  icoReCacheSecondsTimeout = Preferences::GetInt(PREF_ICOTIMEOUT, 
                                                 kSecondsPerDay);
  alreadyObtained = PR_TRUE;
  return icoReCacheSecondsTimeout;
}



nsresult JumpListShortcut::ObtainCachedIconFile(nsCOMPtr<nsIURI> aIconURI,
                                                nsString &aICOFilePath)
{
  
  nsCOMPtr<nsIFile> icoFile;
  nsresult rv = GetOutputIconPath(aIconURI, icoFile);
  NS_ENSURE_SUCCESS(rv, rv);

  
  PRBool exists;
  rv = icoFile->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);

  if (exists) {

    
    PRInt64 fileModTime = LL_ZERO;
    rv = icoFile->GetLastModifiedTime(&fileModTime);
    fileModTime /= PR_MSEC_PER_SEC;
    PRInt32 icoReCacheSecondsTimeout = GetICOCacheSecondsTimeout();
    PRInt64 nowTime = PR_Now() / PRInt64(PR_USEC_PER_SEC);

    
    
    if (NS_FAILED(rv) ||
        (nowTime - fileModTime) > icoReCacheSecondsTimeout) {
      rv = CacheIconFileFromIconURI(aIconURI, icoFile);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  } else {
    
    rv = CacheIconFileFromIconURI(aIconURI, icoFile);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  rv = icoFile->GetPath(aICOFilePath);
  return rv;
}




nsresult JumpListShortcut::GetOutputIconPath(nsCOMPtr<nsIURI> aIconURI,
                                             nsCOMPtr<nsIFile> &aICOFile) 
{
  
  nsCAutoString inputURIHash;
  nsCOMPtr<nsICryptoHash> cryptoHash;
  nsresult rv = JumpListItem::HashURI(cryptoHash, aIconURI, inputURIHash);
  NS_ENSURE_SUCCESS(rv, rv);
  char* cur = inputURIHash.BeginWriting();
  char* end = inputURIHash.EndWriting();
  for (; cur < end; ++cur) {
    if ('/' == *cur) {
      *cur = '_';
    }
  }

  
  rv = NS_GetSpecialDirectory("ProfLDS", getter_AddRefs(aICOFile));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aICOFile->AppendNative(nsDependentCString(JumpListItem::kJumpListCacheDir));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = aICOFile->Create(nsIFile::DIRECTORY_TYPE, 0777);
  if (NS_FAILED(rv) && rv != NS_ERROR_FILE_ALREADY_EXISTS) {
    return rv;
  }
  
  
  inputURIHash.Append(".ico");
  rv = aICOFile->AppendNative(inputURIHash);

  return rv;
}



nsresult JumpListShortcut::CacheIconFileFromIconURI(nsCOMPtr<nsIURI> aIconURI, 
                                                    nsCOMPtr<nsIFile> aICOFile) 
{
  
  nsCOMPtr<nsIFaviconService> favIconSvc(
                      do_GetService("@mozilla.org/browser/favicon-service;1"));
  NS_ENSURE_TRUE(favIconSvc, NS_ERROR_FAILURE);

  
  nsCString mimeType;
  PRUint32 dataLength;
  PRUint8 *data;
  
  nsresult rv = favIconSvc->GetFaviconData(aIconURI, mimeType, 
                                           &dataLength, &data);
  NS_ENSURE_SUCCESS(rv, rv);
  nsAutoArrayPtr<PRUint8> freeMeWhenScopeEnds(data);
  if(dataLength == 0 || !data) {
    return NS_ERROR_FAILURE;
  }

  
  nsCOMPtr<nsIInputStream> stream;
  rv = NS_NewByteInputStream(getter_AddRefs(stream),
                             reinterpret_cast<const char*>(data), dataLength,
                             NS_ASSIGNMENT_DEPEND);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<imgIContainer> container;
  nsCOMPtr<imgITools> imgtool = do_CreateInstance("@mozilla.org/image/tools;1");
  rv = imgtool->DecodeImageData(stream, mimeType, getter_AddRefs(container));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  
  PRInt32 systemIconWidth = GetSystemMetrics(SM_CXSMICON);
  PRInt32 systemIconHeight = GetSystemMetrics(SM_CYSMICON);
  if (systemIconWidth == 0 || systemIconHeight == 0) {
    systemIconWidth = 16;
    systemIconHeight = 16;
  }
  
  mimeType.AssignLiteral("image/vnd.microsoft.icon");
  nsCOMPtr<nsIInputStream> iconStream;
  rv = imgtool->EncodeScaledImage(container, mimeType,
                                  systemIconWidth,
                                  systemIconHeight,
                                  getter_AddRefs(iconStream));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIOutputStream> outputStream;
  rv = NS_NewLocalFileOutputStream(getter_AddRefs(outputStream), aICOFile);
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
  NS_ENSURE_SUCCESS(rv, rv);
  if(bufSize != wrote) {
    return NS_ERROR_FAILURE;
  }

  
  bufferedOutputStream->Close();
  outputStream->Close();

  return NS_OK;
}



nsresult JumpListShortcut::RemoveCacheIcon(nsCOMPtr<nsIURI> aUri)
{
  
  nsCAutoString spec;
  nsresult rv = aUri->GetSpec(spec);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsILocalFile> icoFile = do_CreateInstance("@mozilla.org/file/local;1");
  NS_ENSURE_TRUE(icoFile, NS_ERROR_FAILURE);
  rv = icoFile->InitWithPath(NS_ConvertUTF8toUTF16(spec));
  NS_ENSURE_SUCCESS(rv, rv);

  
  PRBool exists;
  rv = icoFile->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (exists) {
    icoFile->Remove(PR_FALSE);
  }

  return NS_OK;
}


nsresult JumpListShortcut::GetShellLink(nsCOMPtr<nsIJumpListItem>& item, nsRefPtr<IShellLinkW>& aShellLink)
{
  HRESULT hr;
  IShellLinkW* psl;
  nsresult rv;

  
  
  

  PRInt16 type;
  if (NS_FAILED(item->GetType(&type)))
    return NS_ERROR_INVALID_ARG;

  if (type != nsIJumpListItem::JUMPLIST_ITEM_SHORTCUT)
    return NS_ERROR_INVALID_ARG;

  nsCOMPtr<nsIJumpListShortcut> shortcut = do_QueryInterface(item, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsILocalHandlerApp> handlerApp;
  rv = shortcut->GetApp(getter_AddRefs(handlerApp));
  NS_ENSURE_SUCCESS(rv, rv);

  
  hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                        IID_IShellLinkW, (LPVOID*)&psl);
  if (FAILED(hr))
    return NS_ERROR_UNEXPECTED;

  
  nsAutoString appPath, appTitle, appDescription, appArgs;
  PRInt32 appIconIndex = 0;

  
  nsCOMPtr<nsIFile> executable;
  handlerApp->GetExecutable(getter_AddRefs(executable));
  nsCOMPtr<nsILocalFile> localFile = do_QueryInterface(executable, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = localFile->GetPath(appPath);
  NS_ENSURE_SUCCESS(rv, rv);

  
  PRUint32 count = 0;
  handlerApp->GetParameterCount(&count);
  for (PRUint32 idx = 0; idx < count; idx++) {
    if (idx > 0)
      appArgs.Append(NS_LITERAL_STRING(" "));
    nsAutoString param;
    rv = handlerApp->GetParameter(idx, param);
    if (NS_FAILED(rv))
      return rv;
    appArgs.Append(param);
  }

  handlerApp->GetName(appTitle);
  handlerApp->GetDetailedDescription(appDescription);

  PRBool useUriIcon = PR_FALSE; 
  PRBool usedUriIcon = PR_FALSE; 
  shortcut->GetIconIndex(&appIconIndex);
  
  nsCOMPtr<nsIURI> iconUri;
  rv = shortcut->GetIconImageUri(getter_AddRefs(iconUri));
  if (NS_SUCCEEDED(rv) && iconUri) {
    useUriIcon = PR_TRUE;
  }

  
  if (appTitle.Length() > 0) {
    IPropertyStore* pPropStore = nsnull;
    hr = psl->QueryInterface(IID_IPropertyStore, (LPVOID*)&pPropStore);
    if (FAILED(hr))
      return NS_ERROR_UNEXPECTED;

    PROPVARIANT pv;
    InitPropVariantFromString(appTitle.get(), &pv);

    pPropStore->SetValue(PKEY_Title, pv);
    pPropStore->Commit();
    pPropStore->Release();

    PropVariantClear(&pv);
  }

  
  psl->SetPath(appPath.get());
  psl->SetDescription(appDescription.get());
  psl->SetArguments(appArgs.get());

  if (useUriIcon) {
    nsString icoFilePath;
    rv = ObtainCachedIconFile(iconUri, icoFilePath);
    if (NS_SUCCEEDED(rv)) {
      
      
      psl->SetIconLocation(icoFilePath.get(), 0);
      usedUriIcon = PR_TRUE;
    }
  }

  
  if (!usedUriIcon) {
    psl->SetIconLocation(appPath.get(), appIconIndex);
  }

  aShellLink = dont_AddRef(psl);

  return NS_OK;
}



static nsresult IsPathInOurIconCache(nsCOMPtr<nsIJumpListShortcut>& aShortcut, 
                                     PRUnichar *aPath, PRBool *aSame)
{
  NS_ENSURE_ARG_POINTER(aPath);
  NS_ENSURE_ARG_POINTER(aSame);
 
  *aSame = PR_FALSE;

  
  nsCOMPtr<nsIFile> jumpListCache;
  nsresult rv = NS_GetSpecialDirectory("ProfLDS", getter_AddRefs(jumpListCache));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = jumpListCache->AppendNative(nsDependentCString(JumpListItem::kJumpListCacheDir));
  NS_ENSURE_SUCCESS(rv, rv);
  nsAutoString jumpListCachePath;
  rv = jumpListCache->GetPath(jumpListCachePath);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsILocalFile> passedInFile = do_CreateInstance("@mozilla.org/file/local;1");
  NS_ENSURE_TRUE(passedInFile, NS_ERROR_FAILURE);
  nsAutoString passedInPath(aPath);
  rv = passedInFile->InitWithPath(passedInPath);
  nsCOMPtr<nsIFile> passedInParentFile;
  passedInFile->GetParent(getter_AddRefs(passedInParentFile));
  nsAutoString passedInParentPath;
  rv = jumpListCache->GetPath(passedInParentPath);
  NS_ENSURE_SUCCESS(rv, rv);

  *aSame = jumpListCachePath.Equals(passedInParentPath);
  return NS_OK;
}


nsresult JumpListShortcut::GetJumpListShortcut(IShellLinkW *pLink, nsCOMPtr<nsIJumpListShortcut>& aShortcut)
{
  NS_ENSURE_ARG_POINTER(pLink);

  nsresult rv;
  HRESULT hres;

  nsCOMPtr<nsILocalHandlerApp> handlerApp = 
    do_CreateInstance(NS_LOCALHANDLERAPP_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUnichar buf[MAX_PATH];

  
  hres = pLink->GetPath((LPWSTR)&buf, MAX_PATH, NULL, SLGP_UNCPRIORITY);
  if (FAILED(hres))
    return NS_ERROR_INVALID_ARG;

  nsCOMPtr<nsILocalFile> file;
  nsDependentString filepath(buf);
  rv = NS_NewLocalFile(filepath, PR_FALSE, getter_AddRefs(file));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = handlerApp->SetExecutable(file);
  NS_ENSURE_SUCCESS(rv, rv);

  
  hres = pLink->GetArguments((LPWSTR)&buf, MAX_PATH);
  if (SUCCEEDED(hres)) {
    LPWSTR *arglist;
    PRInt32 numArgs;
    PRInt32 idx;

    arglist = ::CommandLineToArgvW(buf, &numArgs);
    if(arglist) {
      for (idx = 0; idx < numArgs; idx++) {
        
        nsDependentString arg(arglist[idx]);
        handlerApp->AppendParameter(arg);
      }
      ::LocalFree(arglist);
    }
  }

  rv = aShortcut->SetApp(handlerApp);
  NS_ENSURE_SUCCESS(rv, rv);

  
  int iconIdx = 0;
  hres = pLink->GetIconLocation((LPWSTR)&buf, MAX_PATH, &iconIdx);
  if (SUCCEEDED(hres)) {
    
    aShortcut->SetIconIndex(iconIdx);

    
    
    PRBool isInOurCache;
    if (NS_SUCCEEDED(IsPathInOurIconCache(aShortcut, buf, &isInOurCache)) && 
        isInOurCache) {
      nsCOMPtr<nsIURI> iconUri;
      nsAutoString path(buf);
      rv = NS_NewURI(getter_AddRefs(iconUri), path);
      if (NS_SUCCEEDED(rv)) {
        aShortcut->SetIconImageUri(iconUri);
      }
    }
  }

  
  

  return NS_OK;
}



nsresult JumpListLink::GetShellItem(nsCOMPtr<nsIJumpListItem>& item, nsRefPtr<IShellItem2>& aShellItem)
{
  IShellItem2 *psi = nsnull;
  nsresult rv;

  PRInt16 type; 
  if (NS_FAILED(item->GetType(&type)))
    return NS_ERROR_INVALID_ARG;

  if (type != nsIJumpListItem::JUMPLIST_ITEM_LINK)
    return NS_ERROR_INVALID_ARG;

  nsCOMPtr<nsIJumpListLink> link = do_QueryInterface(item, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIURI> uri;
  rv = link->GetUri(getter_AddRefs(uri));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString spec;
  rv = uri->GetSpec(spec);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (createItemFromParsingName == nsnull) {
    if (sShellDll)
      return NS_ERROR_UNEXPECTED;
    sShellDll = ::LoadLibraryW(kSehllLibraryName);
    if (sShellDll)
      createItemFromParsingName = (SHCreateItemFromParsingNamePtr)GetProcAddress(sShellDll, "SHCreateItemFromParsingName");
    if (createItemFromParsingName == nsnull)
      return NS_ERROR_UNEXPECTED;
  }

  
  if (FAILED(createItemFromParsingName(NS_ConvertASCIItoUTF16(spec).get(),
             NULL, IID_PPV_ARGS(&psi))))
    return NS_ERROR_INVALID_ARG;

  
  nsAutoString linkTitle;
  link->GetUriTitle(linkTitle);

  IPropertyStore* pPropStore = nsnull;
  HRESULT hres = psi->GetPropertyStore(GPS_DEFAULT, IID_IPropertyStore, (void**)&pPropStore);
  if (FAILED(hres))
    return NS_ERROR_UNEXPECTED;

  PROPVARIANT pv;
  InitPropVariantFromString(linkTitle.get(), &pv);

  
  pPropStore->SetValue(PKEY_ItemName, pv);
  pPropStore->Commit();
  pPropStore->Release();

  PropVariantClear(&pv);

  aShellItem = dont_AddRef(psi);

  return NS_OK;
}


nsresult JumpListLink::GetJumpListLink(IShellItem *pItem, nsCOMPtr<nsIJumpListLink>& aLink)
{
  NS_ENSURE_ARG_POINTER(pItem);

  
  
  nsresult rv;
  LPWSTR lpstrName = NULL;

  if (SUCCEEDED(pItem->GetDisplayName(SIGDN_URL, &lpstrName))) {
    nsCOMPtr<nsIURI> uri;
    nsAutoString spec(lpstrName);

    rv = NS_NewURI(getter_AddRefs(uri), NS_ConvertUTF16toUTF8(spec));
    if (NS_FAILED(rv))
      return NS_ERROR_INVALID_ARG;

    aLink->SetUri(uri);

    ::CoTaskMemFree(lpstrName);
  }

  return NS_OK;
}


PRBool JumpListShortcut::ExecutableExists(nsCOMPtr<nsILocalHandlerApp>& handlerApp)
{
  nsresult rv;

  if (!handlerApp)
    return PR_FALSE;

  nsCOMPtr<nsIFile> executable;
  rv = handlerApp->GetExecutable(getter_AddRefs(executable));
  if (NS_SUCCEEDED(rv) && executable) {
    PRBool exists;
    executable->Exists(&exists);
    return exists;
  }
  return PR_FALSE;
}


nsresult JumpListItem::HashURI(nsCOMPtr<nsICryptoHash> &aCryptoHash, 
                               nsIURI *aUri, nsACString& aUriHash)
{
  if (!aUri)
    return NS_ERROR_INVALID_ARG;

  nsCAutoString spec;
  nsresult rv = aUri->GetSpec(spec);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!aCryptoHash) {
    aCryptoHash = do_CreateInstance(NS_CRYPTO_HASH_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = aCryptoHash->Init(nsICryptoHash::MD5);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aCryptoHash->Update(reinterpret_cast<const PRUint8*>(spec.BeginReading()), 
                                                            spec.Length());
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aCryptoHash->Finish(PR_TRUE, aUriHash);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

} 
} 

#endif 
