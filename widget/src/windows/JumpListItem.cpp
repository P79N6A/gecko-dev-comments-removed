





































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

namespace mozilla {
namespace widget {



JumpListLink::SHCreateItemFromParsingNamePtr JumpListLink::createItemFromParsingName = nsnull;
const PRUnichar JumpListLink::kSehllLibraryName[] =  L"shell32.dll";
HMODULE JumpListLink::sShellDll = nsnull;


NS_IMPL_ISUPPORTS1(JumpListItem,
                   nsIJumpListItem)

NS_IMPL_ISUPPORTS_INHERITED1(JumpListSeparator,
                             JumpListItem,
                             nsIJumpListSeparator)

NS_IMPL_ISUPPORTS_INHERITED1(JumpListLink,
                             JumpListItem,
                             nsIJumpListLink)

NS_IMPL_ISUPPORTS_INHERITED1(JumpListShortcut,
                             JumpListItem,
                             nsIJumpListShortcut)


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

  return HashURI(mURI, aUriHash);
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

  rv = HashURI(mURI, hash1);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = HashURI(aUri, hash2);
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
  shortcut->GetIconIndex(&appIconIndex);

  
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
  psl->SetIconLocation(appPath.get(), appIconIndex);

  aShellLink = dont_AddRef(psl);

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

nsresult JumpListLink::HashURI(nsIURI *aUri, nsACString& aUriHash)
{
  nsresult rv;
  
  if (!aUri)
    return NS_ERROR_INVALID_ARG;

  nsCAutoString spec;
  rv = aUri->GetSpec(spec);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!mCryptoHash) {
    mCryptoHash = do_CreateInstance(NS_CRYPTO_HASH_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = mCryptoHash->Init(nsICryptoHash::MD5);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mCryptoHash->Update(reinterpret_cast<const PRUint8*>(spec.BeginReading()), spec.Length());
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mCryptoHash->Finish(PR_TRUE, aUriHash);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

} 
} 

#endif 
