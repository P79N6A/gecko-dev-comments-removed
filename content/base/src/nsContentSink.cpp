











































#include "nsContentSink.h"
#include "nsScriptLoader.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsICSSLoader.h"
#include "nsStyleConsts.h"
#include "nsStyleLinkElement.h"
#include "nsINodeInfo.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsCPrefetchService.h"
#include "nsIURI.h"
#include "nsNetUtil.h"
#include "nsIHttpChannel.h"
#include "nsIContent.h"
#include "nsIScriptElement.h"
#include "nsIParser.h"
#include "nsContentErrors.h"
#include "nsIPresShell.h"
#include "nsPresContext.h"
#include "nsIViewManager.h"
#include "nsIContentViewer.h"
#include "nsIAtom.h"
#include "nsGkAtoms.h"
#include "nsIDOMWindowInternal.h"
#include "nsIPrincipal.h"
#include "nsIScriptGlobalObject.h"
#include "nsNetCID.h"
#include "nsIOfflineCacheUpdate.h"
#include "nsIApplicationCache.h"
#include "nsIApplicationCacheContainer.h"
#include "nsIApplicationCacheChannel.h"
#include "nsIApplicationCacheService.h"
#include "nsIScriptSecurityManager.h"
#include "nsIDOMLoadStatus.h"
#include "nsICookieService.h"
#include "nsIPrompt.h"
#include "nsServiceManagerUtils.h"
#include "nsContentUtils.h"
#include "nsParserUtils.h"
#include "nsCRT.h"
#include "nsEscape.h"
#include "nsWeakReference.h"
#include "nsUnicharUtils.h"
#include "nsNodeInfoManager.h"
#include "nsTimer.h"
#include "nsIAppShell.h"
#include "nsWidgetsCID.h"
#include "nsIDOMNSDocument.h"
#include "nsIRequest.h"
#include "nsNodeUtils.h"
#include "nsIDOMNode.h"
#include "nsThreadUtils.h"
#include "nsPresShellIterator.h"
#include "nsPIDOMWindow.h"
#include "mozAutoDocUpdate.h"
#include "nsIWebNavigation.h"
#include "nsIDocumentLoader.h"
#include "nsICachingChannel.h"
#include "nsICacheEntryDescriptor.h"
#include "nsGenericHTMLElement.h"
#include "nsHTMLDNSPrefetch.h"

PRLogModuleInfo* gContentSinkLogModuleInfo;

class nsScriptLoaderObserverProxy : public nsIScriptLoaderObserver
{
public:
  nsScriptLoaderObserverProxy(nsIScriptLoaderObserver* aInner)
    : mInner(do_GetWeakReference(aInner))
  {
  }
  virtual ~nsScriptLoaderObserverProxy()
  {
  }
  
  NS_DECL_ISUPPORTS
  NS_DECL_NSISCRIPTLOADEROBSERVER

  nsWeakPtr mInner;
};

NS_IMPL_ISUPPORTS1(nsScriptLoaderObserverProxy, nsIScriptLoaderObserver)

NS_IMETHODIMP
nsScriptLoaderObserverProxy::ScriptAvailable(nsresult aResult,
                                             nsIScriptElement *aElement,
                                             PRBool aIsInline,
                                             nsIURI *aURI,
                                             PRInt32 aLineNo)
{
  nsCOMPtr<nsIScriptLoaderObserver> inner = do_QueryReferent(mInner);

  if (inner) {
    return inner->ScriptAvailable(aResult, aElement, aIsInline, aURI,
                                  aLineNo);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsScriptLoaderObserverProxy::ScriptEvaluated(nsresult aResult,
                                             nsIScriptElement *aElement,
                                             PRBool aIsInline)
{
  nsCOMPtr<nsIScriptLoaderObserver> inner = do_QueryReferent(mInner);

  if (inner) {
    return inner->ScriptEvaluated(aResult, aElement, aIsInline);
  }

  return NS_OK;
}


NS_IMPL_CYCLE_COLLECTING_ADDREF(nsContentSink)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsContentSink)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsContentSink)
  NS_INTERFACE_MAP_ENTRY(nsICSSLoaderObserver)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY(nsIScriptLoaderObserver)
  NS_INTERFACE_MAP_ENTRY(nsIDocumentObserver)
  NS_INTERFACE_MAP_ENTRY(nsIMutationObserver)
  NS_INTERFACE_MAP_ENTRY(nsITimerCallback)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIScriptLoaderObserver)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTION_CLASS(nsContentSink)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsContentSink)
  if (tmp->mDocument) {
    tmp->mDocument->RemoveObserver(tmp);
  }
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mDocument)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mParser)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mNodeInfoManager)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsContentSink)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mDocument)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mParser)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NATIVE_MEMBER(mNodeInfoManager,
                                                  nsNodeInfoManager)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END


nsContentSink::nsContentSink()
{
  
  NS_ASSERTION(mLayoutStarted == PR_FALSE, "What?");
  NS_ASSERTION(mDynamicLowerValue == PR_FALSE, "What?");
  NS_ASSERTION(mParsing == PR_FALSE, "What?");
  NS_ASSERTION(mLastSampledUserEventTime == 0, "What?");
  NS_ASSERTION(mDeflectedCount == 0, "What?");
  NS_ASSERTION(mDroppedTimer == PR_FALSE, "What?");
  NS_ASSERTION(mInMonolithicContainer == 0, "What?");
  NS_ASSERTION(mInNotification == 0, "What?");
  NS_ASSERTION(mDeferredLayoutStart == PR_FALSE, "What?");

#ifdef NS_DEBUG
  if (!gContentSinkLogModuleInfo) {
    gContentSinkLogModuleInfo = PR_NewLogModule("nscontentsink");
  }
#endif
}

nsContentSink::~nsContentSink()
{
  if (mDocument) {
    
    
    mDocument->RemoveObserver(this);
  }
}

nsresult
nsContentSink::Init(nsIDocument* aDoc,
                    nsIURI* aURI,
                    nsISupports* aContainer,
                    nsIChannel* aChannel)
{
  NS_PRECONDITION(aDoc, "null ptr");
  NS_PRECONDITION(aURI, "null ptr");

  if (!aDoc || !aURI) {
    return NS_ERROR_NULL_POINTER;
  }

  mDocument = aDoc;

  mDocumentURI = aURI;
  mDocumentBaseURI = aURI;
  mDocShell = do_QueryInterface(aContainer);
  if (mDocShell) {
    PRUint32 loadType = 0;
    mDocShell->GetLoadType(&loadType);
    mChangeScrollPosWhenScrollingToRef =
      ((loadType & nsIDocShell::LOAD_CMD_HISTORY) == 0);
  }

  
  nsCOMPtr<nsIScriptLoaderObserver> proxy =
      new nsScriptLoaderObserverProxy(this);
  NS_ENSURE_TRUE(proxy, NS_ERROR_OUT_OF_MEMORY);

  mScriptLoader = mDocument->ScriptLoader();
  mScriptLoader->AddObserver(proxy);

  mCSSLoader = aDoc->CSSLoader();

  ProcessHTTPHeaders(aChannel);

  mNodeInfoManager = aDoc->NodeInfoManager();

  mNotifyOnTimer =
    nsContentUtils::GetBoolPref("content.notify.ontimer", PR_TRUE);

  
  mBackoffCount =
    nsContentUtils::GetIntPref("content.notify.backoffcount", -1);

  
  
  
  
  
  
  
  mNotificationInterval =
    nsContentUtils::GetIntPref("content.notify.interval", 120000);

  
  
  
  
  
  
  
  
  

  
  
  

  mMaxTokenProcessingTime =
    nsContentUtils::GetIntPref("content.max.tokenizing.time",
                               mNotificationInterval * 3);

  
  mDynamicIntervalSwitchThreshold =
    nsContentUtils::GetIntPref("content.switch.threshold", 750000);

  mCanInterruptParser =
    nsContentUtils::GetBoolPref("content.interrupt.parsing", PR_TRUE);

  return NS_OK;

}

NS_IMETHODIMP
nsContentSink::StyleSheetLoaded(nsICSSStyleSheet* aSheet,
                                PRBool aWasAlternate,
                                nsresult aStatus)
{
  if (!aWasAlternate) {
    NS_ASSERTION(mPendingSheetCount > 0, "How'd that happen?");
    --mPendingSheetCount;

    if (mPendingSheetCount == 0 &&
        (mDeferredLayoutStart || mDeferredFlushTags)) {
      if (mDeferredFlushTags) {
        FlushTags();
      }
      if (mDeferredLayoutStart) {
        
        
        
        
        
        
        StartLayout(PR_FALSE);
      }

      
      ScrollToRef();
    }
    
    mScriptLoader->RemoveExecuteBlocker();
  }

  return NS_OK;
}

NS_IMETHODIMP
nsContentSink::ScriptAvailable(nsresult aResult,
                               nsIScriptElement *aElement,
                               PRBool aIsInline,
                               nsIURI *aURI,
                               PRInt32 aLineNo)
{
  PRUint32 count = mScriptElements.Count();

  if (count == 0) {
    return NS_OK;
  }

  
  
  
  NS_ASSERTION(mScriptElements.IndexOf(aElement) == count - 1 ||
               mScriptElements.IndexOf(aElement) == PRUint32(-1),
               "script found at unexpected position");

  
  if (aElement != mScriptElements[count - 1]) {
    return NS_OK;
  }

  if (mParser && !mParser->IsParserEnabled()) {
    
    
    
    
    mParser->UnblockParser();
  }

  if (NS_SUCCEEDED(aResult)) {
    PreEvaluateScript();
  } else {
    mScriptElements.RemoveObjectAt(count - 1);

    if (mParser && aResult != NS_BINDING_ABORTED) {
      
      
      
      
      
      
      
      
      ContinueInterruptedParsingAsync();
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsContentSink::ScriptEvaluated(nsresult aResult,
                               nsIScriptElement *aElement,
                               PRBool aIsInline)
{
  
  PRInt32 count = mScriptElements.Count();
  if (count == 0) {
    return NS_OK;
  }
  
  if (aElement != mScriptElements[count - 1]) {
    return NS_OK;
  }

  
  mScriptElements.RemoveObjectAt(count - 1); 

  if (NS_SUCCEEDED(aResult)) {
    PostEvaluateScript(aElement);
  }

  if (mParser && mParser->IsParserEnabled()) {
    ContinueInterruptedParsingAsync();
  }

  return NS_OK;
}

nsresult
nsContentSink::ProcessHTTPHeaders(nsIChannel* aChannel)
{
  nsCOMPtr<nsIHttpChannel> httpchannel(do_QueryInterface(aChannel));
  
  if (!httpchannel) {
    return NS_OK;
  }

  
  
  
  nsCAutoString linkHeader;
  
  nsresult rv = httpchannel->GetResponseHeader(NS_LITERAL_CSTRING("link"),
                                               linkHeader);
  if (NS_SUCCEEDED(rv) && !linkHeader.IsEmpty()) {
    ProcessHeaderData(nsGkAtoms::link,
                      NS_ConvertASCIItoUTF16(linkHeader));
  }
  
  return NS_OK;
}

nsresult
nsContentSink::ProcessHeaderData(nsIAtom* aHeader, const nsAString& aValue,
                                 nsIContent* aContent)
{
  nsresult rv = NS_OK;
  

  mDocument->SetHeaderData(aHeader, aValue);

  if (aHeader == nsGkAtoms::setcookie) {
    
    
    
    
    nsCOMPtr<nsICookieService> cookieServ =
      do_GetService(NS_COOKIESERVICE_CONTRACTID, &rv);
    if (NS_FAILED(rv)) {
      return rv;
    }

    

    
    

    
    
    nsCOMPtr<nsIURI> codebaseURI;
    rv = mDocument->NodePrincipal()->GetURI(getter_AddRefs(codebaseURI));
    NS_ENSURE_TRUE(codebaseURI, rv);

    nsCOMPtr<nsIPrompt> prompt;
    nsCOMPtr<nsIDOMWindowInternal> window (do_QueryInterface(mDocument->GetScriptGlobalObject()));
    if (window) {
      window->GetPrompter(getter_AddRefs(prompt));
    }

    nsCOMPtr<nsIChannel> channel;
    if (mParser) {
      mParser->GetChannel(getter_AddRefs(channel));
    }

    rv = cookieServ->SetCookieString(codebaseURI,
                                     prompt,
                                     NS_ConvertUTF16toUTF8(aValue).get(),
                                     channel);
    if (NS_FAILED(rv)) {
      return rv;
    }
  }
  else if (aHeader == nsGkAtoms::link) {
    rv = ProcessLinkHeader(aContent, aValue);
  }
  else if (aHeader == nsGkAtoms::msthemecompatible) {
    
    
    nsAutoString value(aValue);
    if (value.LowerCaseEqualsLiteral("no")) {
      nsIPresShell* shell = mDocument->GetPrimaryShell();
      if (shell) {
        shell->DisableThemeSupport();
      }
    }
  }
  
  
  else if (aHeader != nsGkAtoms::refresh && mParser) {
    
    
    
    
    
    nsCOMPtr<nsIChannel> channel;
    if (NS_SUCCEEDED(mParser->GetChannel(getter_AddRefs(channel)))) {
      nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(channel));
      if (httpChannel) {
        const char* header;
        (void)aHeader->GetUTF8String(&header);
        (void)httpChannel->SetResponseHeader(nsDependentCString(header),
                                             NS_ConvertUTF16toUTF8(aValue),
                                             PR_TRUE);
      }
    }
  }

  return rv;
}


static const PRUnichar kSemiCh = PRUnichar(';');
static const PRUnichar kCommaCh = PRUnichar(',');
static const PRUnichar kEqualsCh = PRUnichar('=');
static const PRUnichar kLessThanCh = PRUnichar('<');
static const PRUnichar kGreaterThanCh = PRUnichar('>');

nsresult
nsContentSink::ProcessLinkHeader(nsIContent* aElement,
                                 const nsAString& aLinkData)
{
  nsresult rv = NS_OK;

  
  nsAutoString href;
  nsAutoString rel;
  nsAutoString title;
  nsAutoString type;
  nsAutoString media;

  
  nsAutoString stringList(aLinkData);

  
  stringList.Append(kNullCh);

  PRUnichar* start = stringList.BeginWriting();
  PRUnichar* end   = start;
  PRUnichar* last  = start;
  PRUnichar  endCh;

  while (*start != kNullCh) {
    
    while ((*start != kNullCh) && nsCRT::IsAsciiSpace(*start)) {
      ++start;
    }

    end = start;
    last = end - 1;

    
    while (*end != kNullCh && *end != kSemiCh && *end != kCommaCh) {
      PRUnichar ch = *end;

      if (ch == kApostrophe || ch == kQuote || ch == kLessThanCh) {
        

        PRUnichar quote = *end;
        if (quote == kLessThanCh) {
          quote = kGreaterThanCh;
        }

        PRUnichar* closeQuote = (end + 1);

        
        while (*closeQuote != kNullCh && quote != *closeQuote) {
          ++closeQuote;
        }

        if (quote == *closeQuote) {
          

          
          end = closeQuote;

          last = end - 1;

          ch = *(end + 1);

          if (ch != kNullCh && ch != kSemiCh && ch != kCommaCh) {
            
            *(++end) = kNullCh;

            ch = *(end + 1);

            
            while (ch != kNullCh && ch != kSemiCh && ch != kCommaCh) {
              ++end;

              ch = *end;
            }
          }
        }
      }

      ++end;
      ++last;
    }

    endCh = *end;

    
    *end = kNullCh;

    if (start < end) {
      if ((*start == kLessThanCh) && (*last == kGreaterThanCh)) {
        *last = kNullCh;

        if (href.IsEmpty()) { 
          href = (start + 1);
          href.StripWhitespace();
        }
      } else {
        PRUnichar* equals = start;

        while ((*equals != kNullCh) && (*equals != kEqualsCh)) {
          equals++;
        }

        if (*equals != kNullCh) {
          *equals = kNullCh;
          nsAutoString  attr(start);
          attr.StripWhitespace();

          PRUnichar* value = ++equals;
          while (nsCRT::IsAsciiSpace(*value)) {
            value++;
          }

          if (((*value == kApostrophe) || (*value == kQuote)) &&
              (*value == *last)) {
            *last = kNullCh;
            value++;
          }

          if (attr.LowerCaseEqualsLiteral("rel")) {
            if (rel.IsEmpty()) {
              rel = value;
              rel.CompressWhitespace();
            }
          } else if (attr.LowerCaseEqualsLiteral("title")) {
            if (title.IsEmpty()) {
              title = value;
              title.CompressWhitespace();
            }
          } else if (attr.LowerCaseEqualsLiteral("type")) {
            if (type.IsEmpty()) {
              type = value;
              type.StripWhitespace();
            }
          } else if (attr.LowerCaseEqualsLiteral("media")) {
            if (media.IsEmpty()) {
              media = value;

              
              ToLowerCase(media);
            }
          }
        }
      }
    }

    if (endCh == kCommaCh) {
      

      if (!href.IsEmpty() && !rel.IsEmpty()) {
        rv = ProcessLink(aElement, href, rel, title, type, media);
      }

      href.Truncate();
      rel.Truncate();
      title.Truncate();
      type.Truncate();
      media.Truncate();
    }

    start = ++end;
  }

  if (!href.IsEmpty() && !rel.IsEmpty()) {
    rv = ProcessLink(aElement, href, rel, title, type, media);
  }

  return rv;
}


nsresult
nsContentSink::ProcessLink(nsIContent* aElement,
                           const nsSubstring& aHref, const nsSubstring& aRel,
                           const nsSubstring& aTitle, const nsSubstring& aType,
                           const nsSubstring& aMedia)
{
  
  nsStringArray linkTypes;
  nsStyleLinkElement::ParseLinkTypes(aRel, linkTypes);

  PRBool hasPrefetch = (linkTypes.IndexOf(NS_LITERAL_STRING("prefetch")) != -1);
  
  if (hasPrefetch || linkTypes.IndexOf(NS_LITERAL_STRING("next")) != -1) {
    PrefetchHref(aHref, aElement, hasPrefetch);
  }

  if ((!aHref.IsEmpty()) && linkTypes.IndexOf(NS_LITERAL_STRING("dns-prefetch")) != -1) {
    PrefetchDNS(aHref);
  }

  
  if (linkTypes.IndexOf(NS_LITERAL_STRING("stylesheet")) == -1) {
    return NS_OK;
  }

  PRBool isAlternate = linkTypes.IndexOf(NS_LITERAL_STRING("alternate")) != -1;
  return ProcessStyleLink(aElement, aHref, isAlternate, aTitle, aType,
                          aMedia);
}

nsresult
nsContentSink::ProcessStyleLink(nsIContent* aElement,
                                const nsSubstring& aHref,
                                PRBool aAlternate,
                                const nsSubstring& aTitle,
                                const nsSubstring& aType,
                                const nsSubstring& aMedia)
{
  if (aAlternate && aTitle.IsEmpty()) {
    
    return NS_OK;
  }

  nsAutoString  mimeType;
  nsAutoString  params;
  nsParserUtils::SplitMimeType(aType, mimeType, params);

  
  if (!mimeType.IsEmpty() && !mimeType.LowerCaseEqualsLiteral("text/css")) {
    
    return NS_OK;
  }

  nsCOMPtr<nsIURI> url;
  nsresult rv = NS_NewURI(getter_AddRefs(url), aHref, nsnull, mDocumentBaseURI);
  
  if (NS_FAILED(rv)) {
    
    return NS_OK;
  }

  PRBool isAlternate;
  rv = mCSSLoader->LoadStyleLink(aElement, url, aTitle, aMedia, aAlternate,
                                 this, &isAlternate);
  NS_ENSURE_SUCCESS(rv, rv);
  
  if (!isAlternate) {
    ++mPendingSheetCount;
    mScriptLoader->AddExecuteBlocker();
  }

  return NS_OK;
}


nsresult
nsContentSink::ProcessMETATag(nsIContent* aContent)
{
  NS_ASSERTION(aContent, "missing base-element");

  nsresult rv = NS_OK;

  
  nsAutoString header;
  aContent->GetAttr(kNameSpaceID_None, nsGkAtoms::httpEquiv, header);
  if (!header.IsEmpty()) {
    nsAutoString result;
    aContent->GetAttr(kNameSpaceID_None, nsGkAtoms::content, result);
    if (!result.IsEmpty()) {
      ToLowerCase(header);
      nsCOMPtr<nsIAtom> fieldAtom(do_GetAtom(header));
      rv = ProcessHeaderData(fieldAtom, result, aContent); 
    }
  }

  return rv;
}


void
nsContentSink::PrefetchHref(const nsAString &aHref,
                            nsIContent *aSource,
                            PRBool aExplicit)
{
  
  
  
  
  
  
  if (!mDocShell)
    return;

  nsCOMPtr<nsIDocShell> docshell = mDocShell;

  nsCOMPtr<nsIDocShellTreeItem> treeItem, parentItem;
  do {
    PRUint32 appType = 0;
    nsresult rv = docshell->GetAppType(&appType);
    if (NS_FAILED(rv) || appType == nsIDocShell::APP_TYPE_MAIL)
      return; 
    if (treeItem = do_QueryInterface(docshell)) {
      treeItem->GetParent(getter_AddRefs(parentItem));
      if (parentItem) {
        treeItem = parentItem;
        docshell = do_QueryInterface(treeItem);
        if (!docshell) {
          NS_ERROR("cannot get a docshell from a treeItem!");
          return;
        }
      }
    }
  } while (parentItem);
  
  
  
  nsCOMPtr<nsIPrefetchService> prefetchService(do_GetService(NS_PREFETCHSERVICE_CONTRACTID));
  if (prefetchService) {
    
    const nsACString &charset = mDocument->GetDocumentCharacterSet();
    nsCOMPtr<nsIURI> uri;
    NS_NewURI(getter_AddRefs(uri), aHref,
              charset.IsEmpty() ? nsnull : PromiseFlatCString(charset).get(),
              mDocumentBaseURI);
    if (uri) {
      nsCOMPtr<nsIDOMNode> domNode = do_QueryInterface(aSource);
      prefetchService->PrefetchURI(uri, mDocumentURI, domNode, aExplicit);
    }
  }
}

void
nsContentSink::PrefetchDNS(const nsAString &aHref)
{
  nsAutoString hostname;

  if (StringBeginsWith(aHref, NS_LITERAL_STRING("//")))  {
    hostname = Substring(aHref, 2);
  }
  else
    nsGenericHTMLElement::GetHostnameFromHrefString(aHref, hostname);
      
  nsRefPtr<nsHTMLDNSPrefetch> prefetch = new nsHTMLDNSPrefetch(hostname, mDocument);
  if (prefetch) {
    prefetch->PrefetchLow();
  }
}

nsresult
nsContentSink::GetChannelCacheKey(nsIChannel* aChannel, nsACString& aCacheKey)
{
  aCacheKey.Truncate();

  nsresult rv;
  nsCOMPtr<nsICachingChannel> cachingChannel = do_QueryInterface(aChannel, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISupports> token;
  rv = cachingChannel->GetCacheToken(getter_AddRefs(token));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsICacheEntryDescriptor> descriptor = do_QueryInterface(token, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = descriptor->GetKey(aCacheKey);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
nsContentSink::SelectDocAppCache(nsIApplicationCache *aLoadApplicationCache,
                                 nsIURI *aManifestURI,
                                 PRBool aIsTopDocument,
                                 PRBool aFetchedWithHTTPGetOrEquiv,
                                 CacheSelectionAction *aAction)
{
  *aAction = CACHE_SELECTION_NONE;

  nsCOMPtr<nsIApplicationCacheContainer> applicationCacheDocument =
    do_QueryInterface(mDocument);
  NS_ASSERTION(applicationCacheDocument,
               "mDocument must implement nsIApplicationCacheContainer.");

  nsresult rv;

  
  nsCOMPtr<nsIApplicationCache> applicationCache = aLoadApplicationCache;

  if (applicationCache) {
    nsCAutoString groupID;
    rv = applicationCache->GetGroupID(groupID);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIURI> groupURI;
    rv = NS_NewURI(getter_AddRefs(groupURI), groupID);
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool equal = PR_FALSE;
    rv = groupURI->Equals(aManifestURI, &equal);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!equal) {
      
      
      
      

      nsCAutoString cachekey;
      rv = GetChannelCacheKey(mDocument->GetChannel(), cachekey);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = applicationCache->MarkEntry(cachekey,
                                       nsIApplicationCache::ITEM_FOREIGN);
      NS_ENSURE_SUCCESS(rv, rv);

      if (aIsTopDocument) {
        *aAction = CACHE_SELECTION_RELOAD;
      }

      return NS_OK;
    }

    if (aIsTopDocument) {
      
      
      
      
      rv = applicationCacheDocument->SetApplicationCache(applicationCache);
      NS_ENSURE_SUCCESS(rv, rv);

      *aAction = CACHE_SELECTION_UPDATE;
    }
  }
  else {
    
    
    

    if (!aFetchedWithHTTPGetOrEquiv) {
      
      
      

      return NS_OK;
    }

    
    
    nsCAutoString manifestURISpec;
    rv = aManifestURI->GetAsciiSpec(manifestURISpec);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIApplicationCacheService> appCacheService =
      do_GetService(NS_APPLICATIONCACHESERVICE_CONTRACTID);
    if (!appCacheService) {
      
      return NS_OK;
    }

    rv = appCacheService->GetActiveCache(manifestURISpec,
                                         getter_AddRefs(applicationCache));
    NS_ENSURE_SUCCESS(rv, rv);

    if (applicationCache) {
      rv = applicationCacheDocument->SetApplicationCache(applicationCache);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
      
      
      
    }

    
    *aAction = CACHE_SELECTION_UPDATE;
  }

  if (applicationCache) {
    
    
    nsCAutoString cachekey;
    rv = GetChannelCacheKey(mDocument->GetChannel(), cachekey);
    if (NS_SUCCEEDED(rv)) {
      rv = applicationCache->MarkEntry(cachekey,
                                       nsIApplicationCache::ITEM_IMPLICIT);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return NS_OK;
}

nsresult
nsContentSink::SelectDocAppCacheNoManifest(nsIApplicationCache *aLoadApplicationCache,
                                           PRBool aIsTopDocument,
                                           nsIURI **aManifestURI,
                                           CacheSelectionAction *aAction)
{
  *aManifestURI = nsnull;
  *aAction = CACHE_SELECTION_NONE;

  if (!aIsTopDocument || !aLoadApplicationCache) {
    return NS_OK;
  }

  nsresult rv;

  
  
  nsCOMPtr<nsIApplicationCacheContainer> applicationCacheDocument =
    do_QueryInterface(mDocument);
  NS_ASSERTION(applicationCacheDocument,
               "mDocument must implement nsIApplicationCacheContainer.");

  rv = applicationCacheDocument->SetApplicationCache(aLoadApplicationCache);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  nsCAutoString groupID;
  rv = aLoadApplicationCache->GetGroupID(groupID);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = NS_NewURI(aManifestURI, groupID);
  NS_ENSURE_SUCCESS(rv, rv);

  *aAction = CACHE_SELECTION_UPDATE;

  return NS_OK;
}

void
nsContentSink::ProcessOfflineManifest(nsIContent *aElement)
{
  
  if (aElement != mDocument->GetRootContent()) {
    return;
  }

  nsresult rv;

  
  nsAutoString manifestSpec;
  aElement->GetAttr(kNameSpaceID_None, nsGkAtoms::manifest, manifestSpec);

  
  nsCOMPtr<nsIApplicationCache> applicationCache;

  nsCOMPtr<nsIApplicationCacheChannel> applicationCacheChannel =
    do_QueryInterface(mDocument->GetChannel());
  if (applicationCacheChannel) {
    rv = applicationCacheChannel->GetApplicationCache(
      getter_AddRefs(applicationCache));
    if (NS_FAILED(rv)) {
      return;
    }
  }

  if (manifestSpec.IsEmpty() && !applicationCache) {
    
    
    return;
  }

  
  
  nsCOMPtr<nsIDOMWindow> window = mDocument->GetWindow();
  if (!window)
    return;
  nsCOMPtr<nsIDOMWindow> parent;
  window->GetParent(getter_AddRefs(parent));
  PRBool isTop = (parent == window);

  CacheSelectionAction action = CACHE_SELECTION_NONE;
  nsCOMPtr<nsIURI> manifestURI;

  if (manifestSpec.IsEmpty()) {
    rv = SelectDocAppCacheNoManifest(applicationCache,
                                     isTop,
                                     getter_AddRefs(manifestURI),
                                     &action);
    if (NS_FAILED(rv)) {
      return;
    }
  }
  else {
    nsContentUtils::NewURIWithDocumentCharset(getter_AddRefs(manifestURI),
                                              manifestSpec, mDocument,
                                              mDocumentURI);
    if (!manifestURI) {
      return;
    }

    
    rv = mDocument->NodePrincipal()->CheckMayLoad(manifestURI, PR_TRUE);
    if (NS_FAILED(rv)) {
      return;
    }

    
    if (!nsContentUtils::OfflineAppAllowed(mDocument->NodePrincipal())) {
      return;
    }

    PRBool fetchedWithHTTPGetOrEquiv = PR_FALSE;
    nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(mDocument->GetChannel()));
    if (httpChannel) {
      nsCAutoString method;
      rv = httpChannel->GetRequestMethod(method);
      if (NS_SUCCEEDED(rv))
        fetchedWithHTTPGetOrEquiv = method.Equals("GET");
    }

    rv = SelectDocAppCache(applicationCache, manifestURI, isTop,
                           fetchedWithHTTPGetOrEquiv, &action);
    if (NS_FAILED(rv)) {
      return;
    }
  }

  switch (action)
  {
  case CACHE_SELECTION_NONE:
    return;
  case CACHE_SELECTION_UPDATE: {
    nsCOMPtr<nsIOfflineCacheUpdateService> updateService =
      do_GetService(NS_OFFLINECACHEUPDATESERVICE_CONTRACTID);

    if (updateService) {
      nsCOMPtr<nsIDOMDocument> domdoc = do_QueryInterface(mDocument);
      updateService->ScheduleOnDocumentStop(manifestURI, mDocumentURI, domdoc);
    }
    break;
  }
  case CACHE_SELECTION_RELOAD: {
    
    
    NS_ASSERTION(isTop, "Should only reload toplevel documents!");
    nsCOMPtr<nsIWebNavigation> webNav = do_QueryInterface(mDocShell);

    webNav->Stop(nsIWebNavigation::STOP_ALL);
    webNav->Reload(nsIWebNavigation::LOAD_FLAGS_NONE);
    break;
  }
  }
}

void
nsContentSink::ScrollToRef()
{
  if (mRef.IsEmpty()) {
    return;
  }

  if (mScrolledToRefAlready) {
    return;
  }

  char* tmpstr = ToNewCString(mRef);
  if (!tmpstr) {
    return;
  }

  nsUnescape(tmpstr);
  nsCAutoString unescapedRef;
  unescapedRef.Assign(tmpstr);
  nsMemory::Free(tmpstr);

  nsresult rv = NS_ERROR_FAILURE;
  
  
  NS_ConvertUTF8toUTF16 ref(unescapedRef);

  nsPresShellIterator iter(mDocument);
  nsCOMPtr<nsIPresShell> shell;
  while ((shell = iter.GetNextShell())) {
    
    if (!ref.IsEmpty()) {
      
      rv = shell->GoToAnchor(ref, mChangeScrollPosWhenScrollingToRef);
    } else {
      rv = NS_ERROR_FAILURE;
    }

    
    

    if (NS_FAILED(rv)) {
      const nsACString &docCharset = mDocument->GetDocumentCharacterSet();

      rv = nsContentUtils::ConvertStringFromCharset(docCharset, unescapedRef, ref);

      if (NS_SUCCEEDED(rv) && !ref.IsEmpty())
        rv = shell->GoToAnchor(ref, mChangeScrollPosWhenScrollingToRef);
    }
    if (NS_SUCCEEDED(rv)) {
      mScrolledToRefAlready = PR_TRUE;
    }
  }
}

nsresult
nsContentSink::RefreshIfEnabled(nsIViewManager* vm)
{
  if (!vm) {
    
    return NS_OK;
  }

  NS_ENSURE_TRUE(mDocShell, NS_ERROR_FAILURE);

  nsCOMPtr<nsIContentViewer> contentViewer;
  mDocShell->GetContentViewer(getter_AddRefs(contentViewer));
  if (contentViewer) {
    PRBool enabled;
    contentViewer->GetEnableRendering(&enabled);
    if (enabled) {
      vm->EnableRefresh(NS_VMREFRESH_IMMEDIATE);
    }
  }

  return NS_OK;
}

void
nsContentSink::StartLayout(PRBool aIgnorePendingSheets)
{
  if (mLayoutStarted) {
    
    return;
  }
  
  mDeferredLayoutStart = PR_TRUE;

  if (!aIgnorePendingSheets && WaitForPendingSheets()) {
    
    return;
  }

  mDeferredLayoutStart = PR_FALSE;

  
  
  
  
  
  
  FlushTags();

  mLayoutStarted = PR_TRUE;
  mLastNotificationTime = PR_Now();

  mDocument->SetMayStartLayout(PR_TRUE);
  nsPresShellIterator iter(mDocument);
  nsCOMPtr<nsIPresShell> shell;
  while ((shell = iter.GetNextShell())) {
    
    
    
    
    

    PRBool didInitialReflow = PR_FALSE;
    shell->GetDidInitialReflow(&didInitialReflow);
    if (didInitialReflow) {
      
      
      
      

      continue;
    }

    nsRect r = shell->GetPresContext()->GetVisibleArea();
    nsCOMPtr<nsIPresShell> shellGrip = shell;
    nsresult rv = shell->InitialReflow(r.width, r.height);
    if (NS_FAILED(rv)) {
      return;
    }

    
    RefreshIfEnabled(shell->GetViewManager());
  }

  
  

  if (mDocumentURI) {
    nsCAutoString ref;

    
    
    
    

    mDocumentURI->GetSpec(ref);

    nsReadingIterator<char> start, end;

    ref.BeginReading(start);
    ref.EndReading(end);

    if (FindCharInReadable('#', start, end)) {
      ++start; 

      mRef = Substring(start, end);
    }
  }
}

void
nsContentSink::NotifyAppend(nsIContent* aContainer, PRUint32 aStartIndex)
{
  if (aContainer->GetCurrentDoc() != mDocument) {
    
    
    return;
  }

  mInNotification++;

  MOZ_TIMER_DEBUGLOG(("Save and stop: nsHTMLContentSink::NotifyAppend()\n"));
  MOZ_TIMER_SAVE(mWatch)
  MOZ_TIMER_STOP(mWatch);

  {
    
    MOZ_AUTO_DOC_UPDATE(mDocument, UPDATE_CONTENT_MODEL, !mBeganUpdate);
    nsNodeUtils::ContentAppended(aContainer, aStartIndex);
    mLastNotificationTime = PR_Now();
  }

  MOZ_TIMER_DEBUGLOG(("Restore: nsHTMLContentSink::NotifyAppend()\n"));
  MOZ_TIMER_RESTORE(mWatch);

  mInNotification--;
}

NS_IMETHODIMP
nsContentSink::Notify(nsITimer *timer)
{
  MOZ_TIMER_DEBUGLOG(("Start: nsHTMLContentSink::Notify()\n"));
  MOZ_TIMER_START(mWatch);

  if (mParsing) {
    
    mDroppedTimer = PR_TRUE;
    return NS_OK;
  }
  
#ifdef MOZ_DEBUG
  {
    PRTime now = PR_Now();
    PRInt64 diff, interval;
    PRInt32 delay;

    LL_I2L(interval, GetNotificationInterval());
    LL_SUB(diff, now, mLastNotificationTime);

    LL_SUB(diff, diff, interval);
    LL_L2I(delay, diff);
    delay /= PR_USEC_PER_MSEC;

    mBackoffCount--;
    SINK_TRACE(gContentSinkLogModuleInfo, SINK_TRACE_REFLOW,
               ("nsContentSink::Notify: reflow on a timer: %d milliseconds "
                "late, backoff count: %d", delay, mBackoffCount));
  }
#endif

  if (WaitForPendingSheets()) {
    mDeferredFlushTags = PR_TRUE;
  } else {
    FlushTags();

    
    
    ScrollToRef();
  }

  mNotificationTimer = nsnull;
  MOZ_TIMER_DEBUGLOG(("Stop: nsHTMLContentSink::Notify()\n"));
  MOZ_TIMER_STOP(mWatch);
  return NS_OK;
}

PRBool
nsContentSink::IsTimeToNotify()
{
  if (!mNotifyOnTimer || !mLayoutStarted || !mBackoffCount ||
      mInMonolithicContainer) {
    return PR_FALSE;
  }

  if (WaitForPendingSheets()) {
    mDeferredFlushTags = PR_TRUE;
    return PR_FALSE;
  }

  PRTime now = PR_Now();
  PRInt64 interval, diff;

  LL_I2L(interval, GetNotificationInterval());
  LL_SUB(diff, now, mLastNotificationTime);

  if (LL_CMP(diff, >, interval)) {
    mBackoffCount--;
    return PR_TRUE;
  }

  return PR_FALSE;
}

nsresult
nsContentSink::WillInterruptImpl()
{
  nsresult result = NS_OK;

  SINK_TRACE(gContentSinkLogModuleInfo, SINK_TRACE_CALLS,
             ("nsContentSink::WillInterrupt: this=%p", this));
#ifndef SINK_NO_INCREMENTAL
  if (WaitForPendingSheets()) {
    mDeferredFlushTags = PR_TRUE;
  } else if (mNotifyOnTimer && mLayoutStarted) {
    if (mBackoffCount && !mInMonolithicContainer) {
      PRInt64 now = PR_Now();
      PRInt64 interval = GetNotificationInterval();
      PRInt64 diff = now - mLastNotificationTime;

      
      if (diff > interval || mDroppedTimer) {
        mBackoffCount--;
        SINK_TRACE(gContentSinkLogModuleInfo, SINK_TRACE_REFLOW,
                   ("nsContentSink::WillInterrupt: flushing tags since we've "
                    "run out time; backoff count: %d", mBackoffCount));
        result = FlushTags();
        if (mDroppedTimer) {
          ScrollToRef();
          mDroppedTimer = PR_FALSE;
        }
      } else if (!mNotificationTimer) {
        interval -= diff;
        PRInt32 delay = interval;

        
        delay /= PR_USEC_PER_MSEC;

        mNotificationTimer = do_CreateInstance("@mozilla.org/timer;1",
                                               &result);
        if (NS_SUCCEEDED(result)) {
          SINK_TRACE(gContentSinkLogModuleInfo, SINK_TRACE_REFLOW,
                     ("nsContentSink::WillInterrupt: setting up timer with "
                      "delay %d", delay));

          result =
            mNotificationTimer->InitWithCallback(this, delay,
                                                 nsITimer::TYPE_ONE_SHOT);
          if (NS_FAILED(result)) {
            mNotificationTimer = nsnull;
          }
        }
      }
    }
  } else {
    SINK_TRACE(gContentSinkLogModuleInfo, SINK_TRACE_REFLOW,
               ("nsContentSink::WillInterrupt: flushing tags "
                "unconditionally"));
    result = FlushTags();
  }
#endif

  mParsing = PR_FALSE;

  return result;
}

nsresult
nsContentSink::WillResumeImpl()
{
  SINK_TRACE(gContentSinkLogModuleInfo, SINK_TRACE_CALLS,
             ("nsContentSink::WillResume: this=%p", this));

  mParsing = PR_TRUE;

  return NS_OK;
}

nsresult
nsContentSink::DidProcessATokenImpl()
{
  if (!mCanInterruptParser) {
    return NS_OK;
  }
  
  
  
  
  
  
  
  
  
  
  

  
  nsIPresShell *shell = mDocument->GetPrimaryShell();

  if (!shell) {
    
    
    return NS_OK;
  }

  nsIViewManager* vm = shell->GetViewManager();
  NS_ENSURE_TRUE(vm, NS_ERROR_FAILURE);
  PRUint32 eventTime;
  nsCOMPtr<nsIWidget> widget;
  nsresult rv = vm->GetWidget(getter_AddRefs(widget));
  if (!widget || NS_FAILED(widget->GetLastInputEventTime(eventTime))) {
      
      
      rv = vm->GetLastUserEventTime(eventTime);
      NS_ENSURE_SUCCESS(rv , NS_ERROR_FAILURE);
  }


  NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

  if (!mDynamicLowerValue && mLastSampledUserEventTime == eventTime) {
    
    
    
    
    if (mDeflectedCount < NS_MAX_TOKENS_DEFLECTED_IN_LOW_FREQ_MODE) {
      mDeflectedCount++;
      
      
      
      

      return NS_OK;
    }

    
    
    
    mDeflectedCount = 0;
  }
  mLastSampledUserEventTime = eventTime;

  PRUint32 currentTime = PR_IntervalToMicroseconds(PR_IntervalNow());

  
  
  
  
  
  
  
  
  
  
  
  

  PRUint32 delayBeforeLoweringThreshold =
    static_cast<PRUint32>(((2 * mDynamicIntervalSwitchThreshold) +
                              NS_DELAY_FOR_WINDOW_CREATION));

  if ((currentTime - mBeginLoadTime) > delayBeforeLoweringThreshold) {
    if ((currentTime - eventTime) <
        static_cast<PRUint32>(mDynamicIntervalSwitchThreshold)) {

      if (!mDynamicLowerValue) {
        
        
        mDynamicLowerValue = PR_TRUE;
        
        
        
        FavorPerformanceHint(PR_FALSE, 0);
      }

    }
    else if (mDynamicLowerValue) {
      
      
      mDynamicLowerValue = PR_FALSE;
      
      FavorPerformanceHint(PR_TRUE, 0);
    }
  }

  if ((currentTime - mDelayTimerStart) >
      static_cast<PRUint32>(GetMaxTokenProcessingTime())) {
    return NS_ERROR_HTMLPARSER_INTERRUPTED;
  }

  return NS_OK;
}



void
nsContentSink::FavorPerformanceHint(PRBool perfOverStarvation, PRUint32 starvationDelay)
{
  static NS_DEFINE_CID(kAppShellCID, NS_APPSHELL_CID);
  nsCOMPtr<nsIAppShell> appShell = do_GetService(kAppShellCID);
  if (appShell)
    appShell->FavorPerformanceHint(perfOverStarvation, starvationDelay);
}

void
nsContentSink::BeginUpdate(nsIDocument *aDocument, nsUpdateType aUpdateType)
{
  
  if (mInNotification && mUpdatesInNotification < 2) {
    ++mUpdatesInNotification;
  }

  
  
  
  
  

  
  
  
  

  
  
  
  
  NS_ASSERTION(aUpdateType && (aUpdateType & UPDATE_ALL) == aUpdateType,
               "Weird update type bitmask");
  if (aUpdateType != UPDATE_CONTENT_STATE && !mInNotification++) {
    FlushTags();
  }
}

void
nsContentSink::EndUpdate(nsIDocument *aDocument, nsUpdateType aUpdateType)
{
  
  
  
  
  
  
  
  
  NS_ASSERTION(aUpdateType && (aUpdateType & UPDATE_ALL) == aUpdateType,
               "Weird update type bitmask");
  if (aUpdateType != UPDATE_CONTENT_STATE && !--mInNotification) {
    UpdateChildCounts();
  }
}

void
nsContentSink::DidBuildModelImpl(void)
{
  if (!mDocument->HaveFiredDOMTitleChange()) {
    mDocument->NotifyPossibleTitleChange(PR_FALSE);
  }

  
  if (mNotificationTimer) {
    SINK_TRACE(gContentSinkLogModuleInfo, SINK_TRACE_REFLOW,
               ("nsContentSink::DidBuildModel: canceling notification "
                "timeout"));
    mNotificationTimer->Cancel();
    mNotificationTimer = 0;
  }	
}

void
nsContentSink::DropParserAndPerfHint(void)
{
  if (!mParser) {
    
    return;
  }
  
  
  
  
  
  
  nsCOMPtr<nsIParser> kungFuDeathGrip(mParser);

  
  
  mParser = nsnull;

  if (mDynamicLowerValue) {
    
    
    FavorPerformanceHint(PR_TRUE, 0);
  }

  if (mCanInterruptParser) {
    mDocument->UnblockOnload(PR_TRUE);
  }
}

nsresult
nsContentSink::WillParseImpl(void)
{
  if (mCanInterruptParser) {
    mDelayTimerStart = PR_IntervalToMicroseconds(PR_IntervalNow());
  }

  return NS_OK;
}

void
nsContentSink::WillBuildModelImpl()
{
  if (mCanInterruptParser) {
    mDocument->BlockOnload();

    mBeginLoadTime = PR_IntervalToMicroseconds(PR_IntervalNow());
  }

  mScrolledToRefAlready = PR_FALSE;
}

void
nsContentSink::ContinueInterruptedParsing()
{
  if (mParser) {
    mParser->ContinueInterruptedParsing();
  }
}

void
nsContentSink::ContinueInterruptedParsingIfEnabled()
{
  if (mParser && mParser->IsParserEnabled()) {
    mParser->ContinueInterruptedParsing();
  }
}

void
nsContentSink::ContinueInterruptedParsingAsync()
{
  nsCOMPtr<nsIRunnable> ev = new nsRunnableMethod<nsContentSink>(this,
    &nsContentSink::ContinueInterruptedParsingIfEnabled);

  NS_DispatchToCurrentThread(ev);
}


PRBool 
IsAttrURI(nsIAtom *aName)
{
  return (aName == nsGkAtoms::action ||
          aName == nsGkAtoms::href ||
          aName == nsGkAtoms::src ||
          aName == nsGkAtoms::longdesc ||
          aName == nsGkAtoms::usemap ||
          aName == nsGkAtoms::cite ||
          aName == nsGkAtoms::background);
}





nsIAtom** const kDefaultAllowedTags [] = {
  &nsGkAtoms::a,
  &nsGkAtoms::abbr,
  &nsGkAtoms::acronym,
  &nsGkAtoms::address,
  &nsGkAtoms::area,
  &nsGkAtoms::b,
  &nsGkAtoms::bdo,
  &nsGkAtoms::big,
  &nsGkAtoms::blockquote,
  &nsGkAtoms::br,
  &nsGkAtoms::button,
  &nsGkAtoms::caption,
  &nsGkAtoms::center,
  &nsGkAtoms::cite,
  &nsGkAtoms::code,
  &nsGkAtoms::col,
  &nsGkAtoms::colgroup,
  &nsGkAtoms::dd,
  &nsGkAtoms::del,
  &nsGkAtoms::dfn,
  &nsGkAtoms::dir,
  &nsGkAtoms::div,
  &nsGkAtoms::dl,
  &nsGkAtoms::dt,
  &nsGkAtoms::em,
  &nsGkAtoms::fieldset,
  &nsGkAtoms::font,
  &nsGkAtoms::form,
  &nsGkAtoms::h1,
  &nsGkAtoms::h2,
  &nsGkAtoms::h3,
  &nsGkAtoms::h4,
  &nsGkAtoms::h5,
  &nsGkAtoms::h6,
  &nsGkAtoms::hr,
  &nsGkAtoms::i,
  &nsGkAtoms::img,
  &nsGkAtoms::input,
  &nsGkAtoms::ins,
  &nsGkAtoms::kbd,
  &nsGkAtoms::label,
  &nsGkAtoms::legend,
  &nsGkAtoms::li,
  &nsGkAtoms::listing,
  &nsGkAtoms::map,
  &nsGkAtoms::menu,
  &nsGkAtoms::nobr,
  &nsGkAtoms::ol,
  &nsGkAtoms::optgroup,
  &nsGkAtoms::option,
  &nsGkAtoms::p,
  &nsGkAtoms::pre,
  &nsGkAtoms::q,
  &nsGkAtoms::s,
  &nsGkAtoms::samp,
  &nsGkAtoms::select,
  &nsGkAtoms::small,
  &nsGkAtoms::span,
  &nsGkAtoms::strike,
  &nsGkAtoms::strong,
  &nsGkAtoms::sub,
  &nsGkAtoms::sup,
  &nsGkAtoms::table,
  &nsGkAtoms::tbody,
  &nsGkAtoms::td,
  &nsGkAtoms::textarea,
  &nsGkAtoms::tfoot,
  &nsGkAtoms::th,
  &nsGkAtoms::thead,
  &nsGkAtoms::tr,
  &nsGkAtoms::tt,
  &nsGkAtoms::u,
  &nsGkAtoms::ul,
  &nsGkAtoms::var,
  nsnull
};

nsIAtom** const kDefaultAllowedAttributes [] = {
  &nsGkAtoms::abbr,
  &nsGkAtoms::accept,
  &nsGkAtoms::acceptcharset,
  &nsGkAtoms::accesskey,
  &nsGkAtoms::action,
  &nsGkAtoms::align,
  &nsGkAtoms::alt,
  &nsGkAtoms::autocomplete,
  &nsGkAtoms::axis,
  &nsGkAtoms::background,
  &nsGkAtoms::bgcolor,
  &nsGkAtoms::border,
  &nsGkAtoms::cellpadding,
  &nsGkAtoms::cellspacing,
  &nsGkAtoms::_char,
  &nsGkAtoms::charoff,
  &nsGkAtoms::charset,
  &nsGkAtoms::checked,
  &nsGkAtoms::cite,
  &nsGkAtoms::_class,
  &nsGkAtoms::clear,
  &nsGkAtoms::cols,
  &nsGkAtoms::colspan,
  &nsGkAtoms::color,
  &nsGkAtoms::compact,
  &nsGkAtoms::coords,
  &nsGkAtoms::datetime,
  &nsGkAtoms::dir,
  &nsGkAtoms::disabled,
  &nsGkAtoms::enctype,
  &nsGkAtoms::_for,
  &nsGkAtoms::frame,
  &nsGkAtoms::headers,
  &nsGkAtoms::height,
  &nsGkAtoms::href,
  &nsGkAtoms::hreflang,
  &nsGkAtoms::hspace,
  &nsGkAtoms::id,
  &nsGkAtoms::ismap,
  &nsGkAtoms::label,
  &nsGkAtoms::lang,
  &nsGkAtoms::longdesc,
  &nsGkAtoms::maxlength,
  &nsGkAtoms::media,
  &nsGkAtoms::method,
  &nsGkAtoms::multiple,
  &nsGkAtoms::name,
  &nsGkAtoms::nohref,
  &nsGkAtoms::noshade,
  &nsGkAtoms::nowrap,
  &nsGkAtoms::pointSize,
  &nsGkAtoms::prompt,
  &nsGkAtoms::readonly,
  &nsGkAtoms::rel,
  &nsGkAtoms::rev,
  &nsGkAtoms::role,
  &nsGkAtoms::rows,
  &nsGkAtoms::rowspan,
  &nsGkAtoms::rules,
  &nsGkAtoms::scope,
  &nsGkAtoms::selected,
  &nsGkAtoms::shape,
  &nsGkAtoms::size,
  &nsGkAtoms::span,
  &nsGkAtoms::src,
  &nsGkAtoms::start,
  &nsGkAtoms::summary,
  &nsGkAtoms::tabindex,
  &nsGkAtoms::target,
  &nsGkAtoms::title,
  &nsGkAtoms::type,
  &nsGkAtoms::usemap,
  &nsGkAtoms::valign,
  &nsGkAtoms::value,
  &nsGkAtoms::vspace,
  &nsGkAtoms::width,
  nsnull
};
