





#include "nsLocation.h"
#include "nsIScriptSecurityManager.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsIScriptContext.h"
#include "nsIDocShell.h"
#include "nsIDocShellLoadInfo.h"
#include "nsIWebNavigation.h"
#include "nsCDefaultURIFixup.h"
#include "nsIURIFixup.h"
#include "nsIURL.h"
#include "nsIJARURI.h"
#include "nsNetUtil.h"
#include "nsCOMPtr.h"
#include "nsEscape.h"
#include "nsIDOMWindow.h"
#include "nsIDocument.h"
#include "nsIPresShell.h"
#include "nsPresContext.h"
#include "nsError.h"
#include "nsDOMClassInfoID.h"
#include "nsReadableUtils.h"
#include "nsITextToSubURI.h"
#include "nsJSUtils.h"
#include "nsContentUtils.h"
#include "mozilla/Likely.h"
#include "nsCycleCollectionParticipant.h"

static nsresult
GetDocumentCharacterSetForURI(const nsAString& aHref, nsACString& aCharset)
{
  aCharset.Truncate();

  JSContext *cx = nsContentUtils::GetCurrentJSContext();
  if (cx) {
    nsCOMPtr<nsPIDOMWindow> window =
      do_QueryInterface(nsJSUtils::GetDynamicScriptGlobal(cx));
    NS_ENSURE_TRUE(window, NS_ERROR_FAILURE);

    if (nsIDocument* doc = window->GetDoc()) {
      aCharset = doc->GetDocumentCharacterSet();
    }
  }

  return NS_OK;
}

nsLocation::nsLocation(nsIDocShell *aDocShell)
{
  mDocShell = do_GetWeakReference(aDocShell);
  nsCOMPtr<nsIDOMWindow> outer = do_GetInterface(aDocShell);
  mOuter = do_GetWeakReference(outer);
}

nsLocation::~nsLocation()
{
}

DOMCI_DATA(Location, nsLocation)


NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsLocation)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsIDOMLocation)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(Location)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_0(nsLocation)
NS_IMPL_CYCLE_COLLECTING_ADDREF(nsLocation)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsLocation)

void
nsLocation::SetDocShell(nsIDocShell *aDocShell)
{
   mDocShell = do_GetWeakReference(aDocShell);
}

nsIDocShell *
nsLocation::GetDocShell()
{
  nsCOMPtr<nsIDocShell> docshell(do_QueryReferent(mDocShell));
  return docshell;
}

nsresult
nsLocation::CheckURL(nsIURI* aURI, nsIDocShellLoadInfo** aLoadInfo)
{
  *aLoadInfo = nullptr;

  nsCOMPtr<nsIDocShell> docShell(do_QueryReferent(mDocShell));
  NS_ENSURE_TRUE(docShell, NS_ERROR_NOT_AVAILABLE);

  nsCOMPtr<nsISupports> owner;
  nsCOMPtr<nsIURI> sourceURI;

  if (JSContext *cx = nsContentUtils::GetCurrentJSContext()) {
    
    
    
    
    

    
    nsIScriptSecurityManager* ssm = nsContentUtils::GetSecurityManager();
    NS_ENSURE_STATE(ssm);

    
    nsresult rv = ssm->CheckLoadURIFromScript(cx, aURI);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    
    
    
    

    nsCOMPtr<nsIDocument> doc;
    nsCOMPtr<nsIURI> docOriginalURI, docCurrentURI, principalURI;
    nsCOMPtr<nsPIDOMWindow> entryPoint =
      do_QueryInterface(nsJSUtils::GetDynamicScriptGlobal(cx));
    if (entryPoint) {
      doc = entryPoint->GetDoc();
    }
    if (doc) {
      docOriginalURI = doc->GetOriginalURI();
      docCurrentURI = doc->GetDocumentURI();
      rv = doc->NodePrincipal()->GetURI(getter_AddRefs(principalURI));
      NS_ENSURE_SUCCESS(rv, rv);
    }

    bool urisEqual = false;
    if (docOriginalURI && docCurrentURI && principalURI) {
      principalURI->Equals(docOriginalURI, &urisEqual);
    }

    if (urisEqual) {
      sourceURI = docCurrentURI;
    }
    else {
      sourceURI = principalURI;
    }

    owner = do_QueryInterface(ssm->GetCxSubjectPrincipal(cx));
  }

  
  nsCOMPtr<nsIDocShellLoadInfo> loadInfo;
  docShell->CreateLoadInfo(getter_AddRefs(loadInfo));
  NS_ENSURE_TRUE(loadInfo, NS_ERROR_FAILURE);

  loadInfo->SetOwner(owner);

  if (sourceURI) {
    loadInfo->SetReferrer(sourceURI);
  }

  loadInfo.swap(*aLoadInfo);

  return NS_OK;
}

nsresult
nsLocation::GetURI(nsIURI** aURI, bool aGetInnermostURI)
{
  *aURI = nullptr;

  nsresult rv;
  nsCOMPtr<nsIDocShell> docShell(do_QueryReferent(mDocShell));
  nsCOMPtr<nsIWebNavigation> webNav(do_QueryInterface(docShell, &rv));
  if (NS_FAILED(rv)) {
    return rv;
  }

  nsCOMPtr<nsIURI> uri;
  rv = webNav->GetCurrentURI(getter_AddRefs(uri));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  if (!uri) {
    return NS_OK;
  }

  if (aGetInnermostURI) {
    nsCOMPtr<nsIJARURI> jarURI(do_QueryInterface(uri));
    while (jarURI) {
      jarURI->GetJARFile(getter_AddRefs(uri));
      jarURI = do_QueryInterface(uri);
    }
  }

  NS_ASSERTION(uri, "nsJARURI screwed up?");

  nsCOMPtr<nsIURIFixup> urifixup(do_GetService(NS_URIFIXUP_CONTRACTID, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  return urifixup->CreateExposableURI(uri, aURI);
}

nsresult
nsLocation::GetWritableURI(nsIURI** aURI)
{
  *aURI = nullptr;

  nsCOMPtr<nsIURI> uri;

  nsresult rv = GetURI(getter_AddRefs(uri));
  if (NS_FAILED(rv) || !uri) {
    return rv;
  }

  return uri->Clone(aURI);
}

nsresult
nsLocation::SetURI(nsIURI* aURI, bool aReplace)
{
  nsCOMPtr<nsIDocShell> docShell(do_QueryReferent(mDocShell));
  if (docShell) {
    nsCOMPtr<nsIDocShellLoadInfo> loadInfo;
    nsCOMPtr<nsIWebNavigation> webNav(do_QueryInterface(docShell));

    if(NS_FAILED(CheckURL(aURI, getter_AddRefs(loadInfo))))
      return NS_ERROR_FAILURE;

    if (aReplace) {
      loadInfo->SetLoadType(nsIDocShellLoadInfo::loadStopContentAndReplace);
    } else {
      loadInfo->SetLoadType(nsIDocShellLoadInfo::loadStopContent);
    }

    return docShell->LoadURI(aURI, loadInfo,
                             nsIWebNavigation::LOAD_FLAGS_NONE, true);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsLocation::GetHash(nsAString& aHash)
{
  if (!CallerSubsumes())
    return NS_ERROR_DOM_SECURITY_ERR;

  aHash.SetLength(0);

  nsCOMPtr<nsIURI> uri;
  nsresult rv = GetURI(getter_AddRefs(uri));
  if (NS_FAILED(rv) || !uri) {
    return rv;
  }

  nsAutoCString ref;
  nsAutoString unicodeRef;

  rv = uri->GetRef(ref);
  if (NS_SUCCEEDED(rv)) {
    nsCOMPtr<nsITextToSubURI> textToSubURI(
        do_GetService(NS_ITEXTTOSUBURI_CONTRACTID, &rv));

    if (NS_SUCCEEDED(rv)) {
      nsAutoCString charset;
      uri->GetOriginCharset(charset);
        
      rv = textToSubURI->UnEscapeURIForUI(charset, ref, unicodeRef);
    }
      
    if (NS_FAILED(rv)) {
      
      NS_UnescapeURL(ref);
      CopyASCIItoUTF16(ref, unicodeRef);
      rv = NS_OK;
    }
  }

  if (NS_SUCCEEDED(rv) && !unicodeRef.IsEmpty()) {
    aHash.Assign(PRUnichar('#'));
    aHash.Append(unicodeRef);
  }

  if (aHash == mCachedHash) {
    
    
    
    aHash = mCachedHash;
  } else {
    mCachedHash = aHash;
  }

  return rv;
}

NS_IMETHODIMP
nsLocation::SetHash(const nsAString& aHash)
{
  nsCOMPtr<nsIURI> uri;
  nsresult rv = GetWritableURI(getter_AddRefs(uri));
  if (NS_FAILED(rv) || !uri) {
    return rv;
  }

  NS_ConvertUTF16toUTF8 hash(aHash);
  if (hash.IsEmpty() || hash.First() != PRUnichar('#')) {
    hash.Insert(PRUnichar('#'), 0);
  }
  rv = uri->SetRef(hash);
  if (NS_SUCCEEDED(rv)) {
    SetURI(uri);
  }

  return rv;
}

NS_IMETHODIMP
nsLocation::GetHost(nsAString& aHost)
{
  if (!CallerSubsumes())
    return NS_ERROR_DOM_SECURITY_ERR;

  aHost.Truncate();

  nsCOMPtr<nsIURI> uri;
  nsresult result;

  result = GetURI(getter_AddRefs(uri), true);

  if (uri) {
    nsAutoCString hostport;

    result = uri->GetHostPort(hostport);

    if (NS_SUCCEEDED(result)) {
      AppendUTF8toUTF16(hostport, aHost);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsLocation::SetHost(const nsAString& aHost)
{
  if (!CallerSubsumes())
    return NS_ERROR_DOM_SECURITY_ERR;

  nsCOMPtr<nsIURI> uri;
  nsresult rv = GetWritableURI(getter_AddRefs(uri));

  if (uri) {
    rv = uri->SetHostPort(NS_ConvertUTF16toUTF8(aHost));
    if (NS_SUCCEEDED(rv)) {
      SetURI(uri);
    }
  }

  return rv;
}

NS_IMETHODIMP
nsLocation::GetHostname(nsAString& aHostname)
{
  if (!CallerSubsumes())
    return NS_ERROR_DOM_SECURITY_ERR;

  aHostname.Truncate();

  nsCOMPtr<nsIURI> uri;
  nsresult result;

  result = GetURI(getter_AddRefs(uri), true);

  if (uri) {
    nsAutoCString host;

    result = uri->GetHost(host);

    if (NS_SUCCEEDED(result)) {
      AppendUTF8toUTF16(host, aHostname);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsLocation::SetHostname(const nsAString& aHostname)
{
  if (!CallerSubsumes())
    return NS_ERROR_DOM_SECURITY_ERR;

  nsCOMPtr<nsIURI> uri;
  nsresult rv = GetWritableURI(getter_AddRefs(uri));

  if (uri) {
    rv = uri->SetHost(NS_ConvertUTF16toUTF8(aHostname));
    if (NS_SUCCEEDED(rv)) {
      SetURI(uri);
    }
  }

  return rv;
}

NS_IMETHODIMP
nsLocation::GetHref(nsAString& aHref)
{
  if (!CallerSubsumes())
    return NS_ERROR_DOM_SECURITY_ERR;

  aHref.Truncate();

  nsCOMPtr<nsIURI> uri;
  nsresult result;

  result = GetURI(getter_AddRefs(uri));

  if (uri) {
    nsAutoCString uriString;

    result = uri->GetSpec(uriString);

    if (NS_SUCCEEDED(result)) {
      AppendUTF8toUTF16(uriString, aHref);
    }
  }

  return result;
}

NS_IMETHODIMP
nsLocation::SetHref(const nsAString& aHref)
{
  nsAutoString oldHref;
  nsresult rv = NS_OK;

  JSContext *cx = nsContentUtils::GetCurrentJSContext();
  if (cx) {
    rv = SetHrefWithContext(cx, aHref, false);
  } else {
    rv = GetHref(oldHref);

    if (NS_SUCCEEDED(rv)) {
      nsCOMPtr<nsIURI> oldUri;

      rv = NS_NewURI(getter_AddRefs(oldUri), oldHref);

      if (oldUri) {
        rv = SetHrefWithBase(aHref, oldUri, false);
      }
    }
  }

  return rv;
}

nsresult
nsLocation::SetHrefWithContext(JSContext* cx, const nsAString& aHref,
                               bool aReplace)
{
  nsCOMPtr<nsIURI> base;

  
  nsresult result = GetSourceBaseURL(cx, getter_AddRefs(base));

  if (NS_FAILED(result)) {
    return result;
  }

  return SetHrefWithBase(aHref, base, aReplace);
}

nsresult
nsLocation::SetHrefWithBase(const nsAString& aHref, nsIURI* aBase,
                            bool aReplace)
{
  nsresult result;
  nsCOMPtr<nsIURI> newUri;

  nsCOMPtr<nsIDocShell> docShell(do_QueryReferent(mDocShell));

  nsAutoCString docCharset;
  if (NS_SUCCEEDED(GetDocumentCharacterSetForURI(aHref, docCharset)))
    result = NS_NewURI(getter_AddRefs(newUri), aHref, docCharset.get(), aBase);
  else
    result = NS_NewURI(getter_AddRefs(newUri), aHref, nullptr, aBase);

  if (newUri) {
    









    bool inScriptTag=false;
    JSContext *cx = nsContentUtils::GetCurrentJSContext();
    if (cx) {
      nsIScriptContext *scriptContext =
        nsJSUtils::GetDynamicScriptContext(cx);

      if (scriptContext) {
        if (scriptContext->GetProcessingScriptTag()) {
          
          
          
          nsCOMPtr<nsIScriptGlobalObject> ourGlobal(do_GetInterface(docShell));
          inScriptTag = (ourGlobal == scriptContext->GetGlobalObject());
        }
      }  
    } 

    return SetURI(newUri, aReplace || inScriptTag);
  }

  return result;
}

NS_IMETHODIMP
nsLocation::GetOrigin(nsAString& aOrigin)
{
  if (!CallerSubsumes())
    return NS_ERROR_DOM_SECURITY_ERR;

  aOrigin.Truncate();

  nsCOMPtr<nsIURI> uri;
  nsresult rv = GetURI(getter_AddRefs(uri), true);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString origin;
  rv = nsContentUtils::GetUTFOrigin(uri, origin);
  NS_ENSURE_SUCCESS(rv, rv);

  aOrigin = origin;
  return NS_OK;
}

NS_IMETHODIMP
nsLocation::GetPathname(nsAString& aPathname)
{
  if (!CallerSubsumes())
    return NS_ERROR_DOM_SECURITY_ERR;

  aPathname.Truncate();

  nsCOMPtr<nsIURI> uri;
  nsresult result = NS_OK;

  result = GetURI(getter_AddRefs(uri));

  nsCOMPtr<nsIURL> url(do_QueryInterface(uri));
  if (url) {
    nsAutoCString file;

    result = url->GetFilePath(file);

    if (NS_SUCCEEDED(result)) {
      AppendUTF8toUTF16(file, aPathname);
    }
  }

  return result;
}

NS_IMETHODIMP
nsLocation::SetPathname(const nsAString& aPathname)
{
  if (!CallerSubsumes())
    return NS_ERROR_DOM_SECURITY_ERR;

  nsCOMPtr<nsIURI> uri;
  nsresult rv = GetWritableURI(getter_AddRefs(uri));

  if (uri) {
    rv = uri->SetPath(NS_ConvertUTF16toUTF8(aPathname));
    if (NS_SUCCEEDED(rv)) {
      SetURI(uri);
    }
  }

  return rv;
}

NS_IMETHODIMP
nsLocation::GetPort(nsAString& aPort)
{
  if (!CallerSubsumes())
    return NS_ERROR_DOM_SECURITY_ERR;

  aPort.SetLength(0);

  nsCOMPtr<nsIURI> uri;
  nsresult result = NS_OK;

  result = GetURI(getter_AddRefs(uri), true);

  if (uri) {
    int32_t port;
    result = uri->GetPort(&port);

    if (NS_SUCCEEDED(result) && -1 != port) {
      nsAutoString portStr;
      portStr.AppendInt(port);
      aPort.Append(portStr);
    }

    
    result = NS_OK;
  }

  return result;
}

NS_IMETHODIMP
nsLocation::SetPort(const nsAString& aPort)
{
  if (!CallerSubsumes())
    return NS_ERROR_DOM_SECURITY_ERR;

  nsCOMPtr<nsIURI> uri;
  nsresult rv = GetWritableURI(getter_AddRefs(uri));

  if (uri) {
    
    NS_ConvertUTF16toUTF8 portStr(aPort);
    const char *buf = portStr.get();
    int32_t port = -1;

    if (buf) {
      if (*buf == ':') {
        port = atol(buf+1);
      }
      else {
        port = atol(buf);
      }
    }

    rv = uri->SetPort(port);
    if (NS_SUCCEEDED(rv)) {
      SetURI(uri);
    }
  }

  return rv;
}

NS_IMETHODIMP
nsLocation::GetProtocol(nsAString& aProtocol)
{
  if (!CallerSubsumes())
    return NS_ERROR_DOM_SECURITY_ERR;

  aProtocol.SetLength(0);

  nsCOMPtr<nsIURI> uri;
  nsresult result = NS_OK;

  result = GetURI(getter_AddRefs(uri));

  if (uri) {
    nsAutoCString protocol;

    result = uri->GetScheme(protocol);

    if (NS_SUCCEEDED(result)) {
      CopyASCIItoUTF16(protocol, aProtocol);
      aProtocol.Append(PRUnichar(':'));
    }
  }

  return result;
}

NS_IMETHODIMP
nsLocation::SetProtocol(const nsAString& aProtocol)
{
  if (!CallerSubsumes())
    return NS_ERROR_DOM_SECURITY_ERR;

  nsCOMPtr<nsIURI> uri;
  nsresult rv = GetWritableURI(getter_AddRefs(uri));

  if (uri) {
    rv = uri->SetScheme(NS_ConvertUTF16toUTF8(aProtocol));
    if (NS_SUCCEEDED(rv)) {
      SetURI(uri);
    }
  }

  return rv;
}

NS_IMETHODIMP
nsLocation::GetSearch(nsAString& aSearch)
{
  if (!CallerSubsumes())
    return NS_ERROR_DOM_SECURITY_ERR;

  aSearch.SetLength(0);

  nsCOMPtr<nsIURI> uri;
  nsresult result = NS_OK;

  result = GetURI(getter_AddRefs(uri));

  nsCOMPtr<nsIURL> url(do_QueryInterface(uri));

  if (url) {
    nsAutoCString search;

    result = url->GetQuery(search);

    if (NS_SUCCEEDED(result) && !search.IsEmpty()) {
      aSearch.Assign(PRUnichar('?'));
      AppendUTF8toUTF16(search, aSearch);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsLocation::SetSearch(const nsAString& aSearch)
{
  if (!CallerSubsumes())
    return NS_ERROR_DOM_SECURITY_ERR;

  nsCOMPtr<nsIURI> uri;
  nsresult rv = GetWritableURI(getter_AddRefs(uri));

  nsCOMPtr<nsIURL> url(do_QueryInterface(uri));
  if (url) {
    rv = url->SetQuery(NS_ConvertUTF16toUTF8(aSearch));
    if (NS_SUCCEEDED(rv)) {
      SetURI(uri);
    }
  }

  return rv;
}

NS_IMETHODIMP
nsLocation::Reload(bool aForceget)
{
  if (!CallerSubsumes())
    return NS_ERROR_DOM_SECURITY_ERR;

  nsresult rv;
  nsCOMPtr<nsIDocShell> docShell(do_QueryReferent(mDocShell));
  nsCOMPtr<nsIWebNavigation> webNav(do_QueryInterface(docShell));
  nsCOMPtr<nsPIDOMWindow> window(do_GetInterface(docShell));

  if (window && window->IsHandlingResizeEvent()) {
    
    
    
    
    
    

    nsCOMPtr<nsIDocument> doc = window->GetExtantDoc();

    nsIPresShell *shell;
    nsPresContext *pcx;
    if (doc && (shell = doc->GetShell()) && (pcx = shell->GetPresContext())) {
      pcx->RebuildAllStyleData(NS_STYLE_HINT_REFLOW);
    }

    return NS_OK;
  }

  if (webNav) {
    uint32_t reloadFlags = nsIWebNavigation::LOAD_FLAGS_NONE;

    if (aForceget) {
      reloadFlags = nsIWebNavigation::LOAD_FLAGS_BYPASS_CACHE | 
                    nsIWebNavigation::LOAD_FLAGS_BYPASS_PROXY;
    }
    rv = webNav->Reload(reloadFlags);
    if (rv == NS_BINDING_ABORTED) {
      
      
      
      rv = NS_OK;
    }
  } else {
    rv = NS_ERROR_FAILURE;
  }

  return rv;
}

NS_IMETHODIMP
nsLocation::Replace(const nsAString& aUrl)
{
  nsresult rv = NS_OK;
  if (JSContext *cx = nsContentUtils::GetCurrentJSContext()) {
    return SetHrefWithContext(cx, aUrl, true);
  }

  nsAutoString oldHref;

  rv = GetHref(oldHref);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIURI> oldUri;

  rv = NS_NewURI(getter_AddRefs(oldUri), oldHref);
  NS_ENSURE_SUCCESS(rv, rv);

  return SetHrefWithBase(aUrl, oldUri, true);
}

NS_IMETHODIMP
nsLocation::Assign(const nsAString& aUrl)
{
  if (!CallerSubsumes())
    return NS_ERROR_DOM_SECURITY_ERR;

  nsAutoString oldHref;
  nsresult result = NS_OK;

  result = GetHref(oldHref);

  if (NS_SUCCEEDED(result)) {
    nsCOMPtr<nsIURI> oldUri;

    result = NS_NewURI(getter_AddRefs(oldUri), oldHref);

    if (oldUri) {
      result = SetHrefWithBase(aUrl, oldUri, false);
    }
  }

  return result;
}

NS_IMETHODIMP
nsLocation::ToString(nsAString& aReturn)
{
  
  return GetHref(aReturn);
}

NS_IMETHODIMP
nsLocation::ValueOf(nsIDOMLocation** aReturn)
{
  nsCOMPtr<nsIDOMLocation> loc(this);
  loc.forget(aReturn);
  return NS_OK;
}

nsresult
nsLocation::GetSourceBaseURL(JSContext* cx, nsIURI** sourceURL)
{

  *sourceURL = nullptr;
  nsCOMPtr<nsIScriptGlobalObject> sgo = nsJSUtils::GetDynamicScriptGlobal(cx);
  
  
  
  
  
  
  if (!sgo && GetDocShell()) {
    sgo = do_GetInterface(GetDocShell());
  }
  NS_ENSURE_TRUE(sgo, NS_OK);
  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(sgo);
  NS_ENSURE_TRUE(window, NS_ERROR_UNEXPECTED);
  nsIDocument* doc = window->GetDoc();
  NS_ENSURE_TRUE(doc, NS_OK);
  *sourceURL = doc->GetBaseURI().get();
  return NS_OK;
}

bool
nsLocation::CallerSubsumes()
{
  
  nsCOMPtr<nsIDOMWindow> outer = do_QueryReferent(mOuter);
  if (MOZ_UNLIKELY(!outer))
    return false;
  nsCOMPtr<nsIScriptObjectPrincipal> sop = do_QueryInterface(outer);
  bool subsumes = false;
  nsresult rv = nsContentUtils::GetSubjectPrincipal()->Subsumes(sop->GetPrincipal(), &subsumes);
  NS_ENSURE_SUCCESS(rv, false);
  return subsumes;
}
