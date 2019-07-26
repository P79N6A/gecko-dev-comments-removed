










#include "nsContentSink.h"
#include "nsScriptLoader.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "mozilla/css/Loader.h"
#include "nsStyleLinkElement.h"
#include "nsIDocShell.h"
#include "nsILoadContext.h"
#include "nsIDocShellTreeItem.h"
#include "nsCPrefetchService.h"
#include "nsIURI.h"
#include "nsNetUtil.h"
#include "nsIHttpChannel.h"
#include "nsIContent.h"
#include "nsIPresShell.h"
#include "nsPresContext.h"
#include "nsViewManager.h"
#include "nsIAtom.h"
#include "nsGkAtoms.h"
#include "nsIDOMWindow.h"
#include "nsNetCID.h"
#include "nsIOfflineCacheUpdate.h"
#include "nsIApplicationCache.h"
#include "nsIApplicationCacheContainer.h"
#include "nsIApplicationCacheChannel.h"
#include "nsIScriptSecurityManager.h"
#include "nsICookieService.h"
#include "nsIPrompt.h"
#include "nsContentUtils.h"
#include "nsNodeInfoManager.h"
#include "nsIAppShell.h"
#include "nsIWidget.h"
#include "nsWidgetsCID.h"
#include "nsIDOMNode.h"
#include "mozAutoDocUpdate.h"
#include "nsIWebNavigation.h"
#include "nsGenericHTMLElement.h"
#include "nsHTMLDNSPrefetch.h"
#include "nsIObserverService.h"
#include "mozilla/Preferences.h"
#include "nsParserConstants.h"

using namespace mozilla;

PRLogModuleInfo* gContentSinkLogModuleInfo;

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsContentSink)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsContentSink)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsContentSink)
  NS_INTERFACE_MAP_ENTRY(nsICSSLoaderObserver)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY(nsIDocumentObserver)
  NS_INTERFACE_MAP_ENTRY(nsIMutationObserver)
  NS_INTERFACE_MAP_ENTRY(nsITimerCallback)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDocumentObserver)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTION_CLASS(nsContentSink)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsContentSink)
  if (tmp->mDocument) {
    tmp->mDocument->RemoveObserver(tmp);
  }
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mDocument)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mParser)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mNodeInfoManager)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mScriptLoader)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsContentSink)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mDocument)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mParser)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mNodeInfoManager)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mScriptLoader)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END


nsContentSink::nsContentSink()
{
  
  NS_ASSERTION(!mLayoutStarted, "What?");
  NS_ASSERTION(!mDynamicLowerValue, "What?");
  NS_ASSERTION(!mParsing, "What?");
  NS_ASSERTION(mLastSampledUserEventTime == 0, "What?");
  NS_ASSERTION(mDeflectedCount == 0, "What?");
  NS_ASSERTION(!mDroppedTimer, "What?");
  NS_ASSERTION(mInMonolithicContainer == 0, "What?");
  NS_ASSERTION(mInNotification == 0, "What?");
  NS_ASSERTION(!mDeferredLayoutStart, "What?");

#ifdef DEBUG
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

bool    nsContentSink::sNotifyOnTimer;
int32_t nsContentSink::sBackoffCount;
int32_t nsContentSink::sNotificationInterval;
int32_t nsContentSink::sInteractiveDeflectCount;
int32_t nsContentSink::sPerfDeflectCount;
int32_t nsContentSink::sPendingEventMode;
int32_t nsContentSink::sEventProbeRate;
int32_t nsContentSink::sInteractiveParseTime;
int32_t nsContentSink::sPerfParseTime;
int32_t nsContentSink::sInteractiveTime;
int32_t nsContentSink::sInitialPerfTime;
int32_t nsContentSink::sEnablePerfMode;

void
nsContentSink::InitializeStatics()
{
  Preferences::AddBoolVarCache(&sNotifyOnTimer,
                               "content.notify.ontimer", true);
  
  Preferences::AddIntVarCache(&sBackoffCount,
                              "content.notify.backoffcount", -1);
  
  
  
  
  
  
  
  Preferences::AddIntVarCache(&sNotificationInterval,
                              "content.notify.interval", 120000);
  Preferences::AddIntVarCache(&sInteractiveDeflectCount,
                              "content.sink.interactive_deflect_count", 0);
  Preferences::AddIntVarCache(&sPerfDeflectCount,
                              "content.sink.perf_deflect_count", 200);
  Preferences::AddIntVarCache(&sPendingEventMode,
                              "content.sink.pending_event_mode", 1);
  Preferences::AddIntVarCache(&sEventProbeRate,
                              "content.sink.event_probe_rate", 1);
  Preferences::AddIntVarCache(&sInteractiveParseTime,
                              "content.sink.interactive_parse_time", 3000);
  Preferences::AddIntVarCache(&sPerfParseTime,
                              "content.sink.perf_parse_time", 360000);
  Preferences::AddIntVarCache(&sInteractiveTime,
                              "content.sink.interactive_time", 750000);
  Preferences::AddIntVarCache(&sInitialPerfTime,
                              "content.sink.initial_perf_time", 2000000);
  Preferences::AddIntVarCache(&sEnablePerfMode,
                              "content.sink.enable_perf_mode", 0);
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
  mDocShell = do_QueryInterface(aContainer);
  mScriptLoader = mDocument->ScriptLoader();

  if (!mRunsToCompletion) {
    if (mDocShell) {
      uint32_t loadType = 0;
      mDocShell->GetLoadType(&loadType);
      mDocument->SetChangeScrollPosWhenScrollingToRef(
        (loadType & nsIDocShell::LOAD_CMD_HISTORY) == 0);
    }

    ProcessHTTPHeaders(aChannel);
  }

  mCSSLoader = aDoc->CSSLoader();

  mNodeInfoManager = aDoc->NodeInfoManager();

  mBackoffCount = sBackoffCount;

  if (sEnablePerfMode != 0) {
    mDynamicLowerValue = sEnablePerfMode == 1;
    FavorPerformanceHint(!mDynamicLowerValue, 0);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsContentSink::StyleSheetLoaded(nsCSSStyleSheet* aSheet,
                                bool aWasAlternate,
                                nsresult aStatus)
{
  NS_ASSERTION(!mRunsToCompletion, "How come a fragment parser observed sheets?");
  if (!aWasAlternate) {
    NS_ASSERTION(mPendingSheetCount > 0, "How'd that happen?");
    --mPendingSheetCount;

    if (mPendingSheetCount == 0 &&
        (mDeferredLayoutStart || mDeferredFlushTags)) {
      if (mDeferredFlushTags) {
        FlushTags();
      }
      if (mDeferredLayoutStart) {
        
        
        
        
        
        
        StartLayout(false);
      }

      
      ScrollToRef();
    }
    
    mScriptLoader->RemoveExecuteBlocker();
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

  
  
  
  nsAutoCString linkHeader;
  
  nsresult rv = httpchannel->GetResponseHeader(NS_LITERAL_CSTRING("link"),
                                               linkHeader);
  if (NS_SUCCEEDED(rv) && !linkHeader.IsEmpty()) {
    mDocument->SetHeaderData(nsGkAtoms::link,
                             NS_ConvertASCIItoUTF16(linkHeader));

    NS_ASSERTION(!mProcessLinkHeaderEvent.get(),
                 "Already dispatched an event?");

    mProcessLinkHeaderEvent =
      NS_NewNonOwningRunnableMethod(this,
        &nsContentSink::DoProcessLinkHeader);
    rv = NS_DispatchToCurrentThread(mProcessLinkHeaderEvent.get());
    if (NS_FAILED(rv)) {
      mProcessLinkHeaderEvent.Forget();
    }
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
    nsCOMPtr<nsIDOMWindow> window = do_QueryInterface(mDocument->GetScriptGlobalObject());
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
  else if (aHeader == nsGkAtoms::msthemecompatible) {
    
    
    nsAutoString value(aValue);
    if (value.LowerCaseEqualsLiteral("no")) {
      nsIPresShell* shell = mDocument->GetShell();
      if (shell) {
        shell->DisableThemeSupport();
      }
    }
  }

  return rv;
}


void
nsContentSink::DoProcessLinkHeader()
{
  nsAutoString value;
  mDocument->GetHeaderData(nsGkAtoms::link, value);
  ProcessLinkHeader(value);
}




bool
nsContentSink::LinkContextIsOurDocument(const nsSubstring& aAnchor)
{
  if (aAnchor.IsEmpty()) {
    
    return true;
  }

  nsIURI* docUri = mDocument->GetDocumentURI();

  
  
  
  nsCOMPtr<nsIURI> contextUri;
  nsresult rv = docUri->CloneIgnoringRef(getter_AddRefs(contextUri));
  
  if (NS_FAILED(rv)) {
    
    return false;
  }
  
  
  nsCOMPtr<nsIURI> resolvedUri;
  rv = NS_NewURI(getter_AddRefs(resolvedUri), aAnchor,
      nullptr, contextUri);
  
  if (NS_FAILED(rv)) {
    
    return false;
  }

  bool same;
  rv = contextUri->Equals(resolvedUri, &same); 
  if (NS_FAILED(rv)) {
    
    return false;
  }

  return same;
}







bool
nsContentSink::Decode5987Format(nsAString& aEncoded) {

  nsresult rv;
  nsCOMPtr<nsIMIMEHeaderParam> mimehdrpar =
  do_GetService(NS_MIMEHEADERPARAM_CONTRACTID, &rv);
  if (NS_FAILED(rv))
    return false;

  nsAutoCString asciiValue;

  const PRUnichar* encstart = aEncoded.BeginReading();
  const PRUnichar* encend = aEncoded.EndReading();

  
  
  while (encstart != encend) {
    if (*encstart > 0 && *encstart < 128) {
      asciiValue.Append((char)*encstart);
    } else {
      return false;
    }
    encstart++;
  }

  nsAutoString decoded;
  nsAutoCString language;

  rv = mimehdrpar->DecodeRFC5987Param(asciiValue, language, decoded);
  if (NS_FAILED(rv))
    return false;

  aEncoded = decoded;
  return true;
}

nsresult
nsContentSink::ProcessLinkHeader(const nsAString& aLinkData)
{
  nsresult rv = NS_OK;

  
  bool seenParameters = false;

  
  nsAutoString href;
  nsAutoString rel;
  nsAutoString title;
  nsAutoString titleStar;
  nsAutoString type;
  nsAutoString media;
  nsAutoString anchor;

  
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

    bool wasQuotedString = false;
    
    
    while (*end != kNullCh && *end != kSemicolon && *end != kComma) {
      PRUnichar ch = *end;

      if (ch == kQuote || ch == kLessThan) {
        

        PRUnichar quote = ch;
        if (quote == kLessThan) {
          quote = kGreaterThan;
        }
        
        wasQuotedString = (ch == kQuote);
        
        PRUnichar* closeQuote = (end + 1);

        
        while (*closeQuote != kNullCh && quote != *closeQuote) {
          
          if (wasQuotedString && *closeQuote == kBackSlash && *(closeQuote + 1) != kNullCh) {
            ++closeQuote;
          }

          ++closeQuote;
        }

        if (quote == *closeQuote) {
          

          
          end = closeQuote;

          last = end - 1;

          ch = *(end + 1);

          if (ch != kNullCh && ch != kSemicolon && ch != kComma) {
            
            *(++end) = kNullCh;

            ch = *(end + 1);

            
            while (ch != kNullCh && ch != kSemicolon && ch != kComma) {
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
      if ((*start == kLessThan) && (*last == kGreaterThan)) {
        *last = kNullCh;

        
        
        if (href.IsEmpty() && !seenParameters) {
          href = (start + 1);
          href.StripWhitespace();
        }
      } else {
        PRUnichar* equals = start;
        seenParameters = true;

        while ((*equals != kNullCh) && (*equals != kEqual)) {
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

          if ((*value == kQuote) && (*value == *last)) {
            *last = kNullCh;
            value++;
          }

          if (wasQuotedString) {
            
            PRUnichar* unescaped = value;
            PRUnichar *src = value;
            
            while (*src != kNullCh) {
              if (*src == kBackSlash && *(src + 1) != kNullCh) {
                src++;
              }
              *unescaped++ = *src++;
            }

            *unescaped = kNullCh;
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
          } else if (attr.LowerCaseEqualsLiteral("title*")) {
            if (titleStar.IsEmpty() && !wasQuotedString) {
              
              
              nsAutoString tmp;
              tmp = value;
              if (Decode5987Format(tmp)) {
                titleStar = tmp;
                titleStar.CompressWhitespace();
              } else {
                
                titleStar.Truncate();
              }
            }
          } else if (attr.LowerCaseEqualsLiteral("type")) {
            if (type.IsEmpty()) {
              type = value;
              type.StripWhitespace();
            }
          } else if (attr.LowerCaseEqualsLiteral("media")) {
            if (media.IsEmpty()) {
              media = value;

              
              
              nsContentUtils::ASCIIToLower(media);
            }
          } else if (attr.LowerCaseEqualsLiteral("anchor")) {
            if (anchor.IsEmpty()) {
              anchor = value;
              anchor.StripWhitespace();
            }
          }
        }
      }
    }

    if (endCh == kComma) {
      

      href.Trim(" \t\n\r\f"); 
      if (!href.IsEmpty() && !rel.IsEmpty()) {
        rv = ProcessLink(anchor, href, rel,
                         
                         titleStar.IsEmpty() ? title : titleStar,
                         type, media);
      }

      href.Truncate();
      rel.Truncate();
      title.Truncate();
      type.Truncate();
      media.Truncate();
      anchor.Truncate();
      
      seenParameters = false;
    }

    start = ++end;
  }
                
  href.Trim(" \t\n\r\f"); 
  if (!href.IsEmpty() && !rel.IsEmpty()) {
    rv = ProcessLink(anchor, href, rel,
                     
                     titleStar.IsEmpty() ? title : titleStar,
                     type, media);
  }

  return rv;
}


nsresult
nsContentSink::ProcessLink(const nsSubstring& aAnchor, const nsSubstring& aHref,
                           const nsSubstring& aRel, const nsSubstring& aTitle,
                           const nsSubstring& aType, const nsSubstring& aMedia)
{
  uint32_t linkTypes = nsStyleLinkElement::ParseLinkTypes(aRel);

  
  
  
  
  if (!LinkContextIsOurDocument(aAnchor)) {
    return NS_OK;
  }
  
  bool hasPrefetch = linkTypes & PREFETCH;
  
  if (hasPrefetch || (linkTypes & NEXT)) {
    PrefetchHref(aHref, mDocument, hasPrefetch);
  }

  if (!aHref.IsEmpty() && (linkTypes & DNS_PREFETCH)) {
    PrefetchDNS(aHref);
  }

  
  if (!(linkTypes & STYLESHEET)) {
    return NS_OK;
  }

  bool isAlternate = linkTypes & ALTERNATE;
  return ProcessStyleLink(nullptr, aHref, isAlternate, aTitle, aType,
                          aMedia);
}

nsresult
nsContentSink::ProcessStyleLink(nsIContent* aElement,
                                const nsSubstring& aHref,
                                bool aAlternate,
                                const nsSubstring& aTitle,
                                const nsSubstring& aType,
                                const nsSubstring& aMedia)
{
  if (aAlternate && aTitle.IsEmpty()) {
    
    return NS_OK;
  }

  nsAutoString  mimeType;
  nsAutoString  params;
  nsContentUtils::SplitMimeType(aType, mimeType, params);

  
  if (!mimeType.IsEmpty() && !mimeType.LowerCaseEqualsLiteral("text/css")) {
    
    return NS_OK;
  }

  nsCOMPtr<nsIURI> url;
  nsresult rv = NS_NewURI(getter_AddRefs(url), aHref, nullptr,
                          mDocument->GetDocBaseURI());
  
  if (NS_FAILED(rv)) {
    
    return NS_OK;
  }

  NS_ASSERTION(!aElement ||
               aElement->NodeType() == nsIDOMNode::PROCESSING_INSTRUCTION_NODE,
               "We only expect processing instructions here");

  
  
  bool isAlternate;
  rv = mCSSLoader->LoadStyleLink(aElement, url, aTitle, aMedia, aAlternate,
                                 CORS_NONE,
                                 mRunsToCompletion ? nullptr : this, &isAlternate);
  NS_ENSURE_SUCCESS(rv, rv);
  
  if (!isAlternate && !mRunsToCompletion) {
    ++mPendingSheetCount;
    mScriptLoader->AddExecuteBlocker();
  }

  return NS_OK;
}


nsresult
nsContentSink::ProcessMETATag(nsIContent* aContent)
{
  NS_ASSERTION(aContent, "missing meta-element");

  nsresult rv = NS_OK;

  
  nsAutoString header;
  aContent->GetAttr(kNameSpaceID_None, nsGkAtoms::httpEquiv, header);
  if (!header.IsEmpty()) {
    nsAutoString result;
    aContent->GetAttr(kNameSpaceID_None, nsGkAtoms::content, result);
    if (!result.IsEmpty()) {
      nsContentUtils::ASCIIToLower(header);
      nsCOMPtr<nsIAtom> fieldAtom(do_GetAtom(header));
      rv = ProcessHeaderData(fieldAtom, result, aContent); 
    }
  }
  NS_ENSURE_SUCCESS(rv, rv);

  if (aContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::name,
                            nsGkAtoms::handheldFriendly, eIgnoreCase)) {
    nsAutoString result;
    aContent->GetAttr(kNameSpaceID_None, nsGkAtoms::content, result);
    if (!result.IsEmpty()) {
      nsContentUtils::ASCIIToLower(result);
      mDocument->SetHeaderData(nsGkAtoms::handheldFriendly, result);
    }
  }

  return rv;
}


void
nsContentSink::PrefetchHref(const nsAString &aHref,
                            nsINode *aSource,
                            bool aExplicit)
{
  
  
  
  
  
  
  if (!mDocShell)
    return;

  nsCOMPtr<nsIDocShell> docshell = mDocShell;

  nsCOMPtr<nsIDocShellTreeItem> treeItem, parentItem;
  do {
    uint32_t appType = 0;
    nsresult rv = docshell->GetAppType(&appType);
    if (NS_FAILED(rv) || appType == nsIDocShell::APP_TYPE_MAIL)
      return; 
    treeItem = do_QueryInterface(docshell);
    if (treeItem) {
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
              charset.IsEmpty() ? nullptr : PromiseFlatCString(charset).get(),
              mDocument->GetDocBaseURI());
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
  else {
    nsCOMPtr<nsIURI> uri;
    NS_NewURI(getter_AddRefs(uri), aHref);
    if (!uri) {
      return;
    }
    nsAutoCString host;
    uri->GetHost(host);
    CopyUTF8toUTF16(host, hostname);
  }

  if (!hostname.IsEmpty() && nsHTMLDNSPrefetch::IsAllowed(mDocument)) {
    nsHTMLDNSPrefetch::PrefetchLow(hostname);
  }
}

nsresult
nsContentSink::SelectDocAppCache(nsIApplicationCache *aLoadApplicationCache,
                                 nsIURI *aManifestURI,
                                 bool aFetchedWithHTTPGetOrEquiv,
                                 CacheSelectionAction *aAction)
{
  nsresult rv;

  *aAction = CACHE_SELECTION_NONE;

  nsCOMPtr<nsIApplicationCacheContainer> applicationCacheDocument =
    do_QueryInterface(mDocument);
  NS_ASSERTION(applicationCacheDocument,
               "mDocument must implement nsIApplicationCacheContainer.");

  if (aLoadApplicationCache) {
    nsCOMPtr<nsIURI> groupURI;
    rv = aLoadApplicationCache->GetManifestURI(getter_AddRefs(groupURI));
    NS_ENSURE_SUCCESS(rv, rv);

    bool equal = false;
    rv = groupURI->Equals(aManifestURI, &equal);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!equal) {
      
      

      *aAction = CACHE_SELECTION_RELOAD;
    }
    else {
      
      
      
#ifdef DEBUG
      nsAutoCString docURISpec, clientID;
      mDocumentURI->GetAsciiSpec(docURISpec);
      aLoadApplicationCache->GetClientID(clientID);
      SINK_TRACE(gContentSinkLogModuleInfo, SINK_TRACE_CALLS,
          ("Selection: assigning app cache %s to document %s", clientID.get(), docURISpec.get()));
#endif

      rv = applicationCacheDocument->SetApplicationCache(aLoadApplicationCache);
      NS_ENSURE_SUCCESS(rv, rv);

      
      
      *aAction = CACHE_SELECTION_UPDATE;
    }
  }
  else {
    
    
    

    if (!aFetchedWithHTTPGetOrEquiv) {
      
      
      
      *aAction = CACHE_SELECTION_RESELECT_WITHOUT_MANIFEST;
    }
    else {
      
      *aAction = CACHE_SELECTION_UPDATE;
    }
  }

  return NS_OK;
}

nsresult
nsContentSink::SelectDocAppCacheNoManifest(nsIApplicationCache *aLoadApplicationCache,
                                           nsIURI **aManifestURI,
                                           CacheSelectionAction *aAction)
{
  *aManifestURI = nullptr;
  *aAction = CACHE_SELECTION_NONE;

  nsresult rv;

  if (aLoadApplicationCache) {
    
    
    nsCOMPtr<nsIApplicationCacheContainer> applicationCacheDocument =
      do_QueryInterface(mDocument);
    NS_ASSERTION(applicationCacheDocument,
                 "mDocument must implement nsIApplicationCacheContainer.");

#ifdef DEBUG
    nsAutoCString docURISpec, clientID;
    mDocumentURI->GetAsciiSpec(docURISpec);
    aLoadApplicationCache->GetClientID(clientID);
    SINK_TRACE(gContentSinkLogModuleInfo, SINK_TRACE_CALLS,
        ("Selection, no manifest: assigning app cache %s to document %s", clientID.get(), docURISpec.get()));
#endif

    rv = applicationCacheDocument->SetApplicationCache(aLoadApplicationCache);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    rv = aLoadApplicationCache->GetManifestURI(aManifestURI);
    NS_ENSURE_SUCCESS(rv, rv);

    *aAction = CACHE_SELECTION_UPDATE;
  }

  return NS_OK;
}

void
nsContentSink::ProcessOfflineManifest(nsIContent *aElement)
{
  
  if (aElement != mDocument->GetRootElement()) {
    return;
  }

  
  
  if (!mDocShell) {
    return;
  }

  
  nsAutoString manifestSpec;
  aElement->GetAttr(kNameSpaceID_None, nsGkAtoms::manifest, manifestSpec);
  ProcessOfflineManifest(manifestSpec);
}

void
nsContentSink::ProcessOfflineManifest(const nsAString& aManifestSpec)
{
  
  
  if (!mDocShell) {
    return;
  }

  
  
  nsCOMPtr<nsILoadContext> loadContext = do_QueryInterface(mDocShell);
  if (loadContext->UsePrivateBrowsing()) {
    return;
  }

  nsresult rv;

  
  nsCOMPtr<nsIApplicationCache> applicationCache;

  nsCOMPtr<nsIApplicationCacheChannel> applicationCacheChannel =
    do_QueryInterface(mDocument->GetChannel());
  if (applicationCacheChannel) {
    bool loadedFromApplicationCache;
    rv = applicationCacheChannel->GetLoadedFromApplicationCache(
      &loadedFromApplicationCache);
    if (NS_FAILED(rv)) {
      return;
    }

    if (loadedFromApplicationCache) {
      rv = applicationCacheChannel->GetApplicationCache(
        getter_AddRefs(applicationCache));
      if (NS_FAILED(rv)) {
        return;
      }
    }
  }

  if (aManifestSpec.IsEmpty() && !applicationCache) {
    
    
    return;
  }

  CacheSelectionAction action = CACHE_SELECTION_NONE;
  nsCOMPtr<nsIURI> manifestURI;

  if (aManifestSpec.IsEmpty()) {
    action = CACHE_SELECTION_RESELECT_WITHOUT_MANIFEST;
  }
  else {
    nsContentUtils::NewURIWithDocumentCharset(getter_AddRefs(manifestURI),
                                              aManifestSpec, mDocument,
                                              mDocumentURI);
    if (!manifestURI) {
      return;
    }

    
    rv = mDocument->NodePrincipal()->CheckMayLoad(manifestURI, true, false);
    if (NS_FAILED(rv)) {
      action = CACHE_SELECTION_RESELECT_WITHOUT_MANIFEST;
    }
    else {
      
      if (!nsContentUtils::OfflineAppAllowed(mDocument->NodePrincipal())) {
        return;
      }

      bool fetchedWithHTTPGetOrEquiv = false;
      nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(mDocument->GetChannel()));
      if (httpChannel) {
        nsAutoCString method;
        rv = httpChannel->GetRequestMethod(method);
        if (NS_SUCCEEDED(rv))
          fetchedWithHTTPGetOrEquiv = method.Equals("GET");
      }

      rv = SelectDocAppCache(applicationCache, manifestURI,
                             fetchedWithHTTPGetOrEquiv, &action);
      if (NS_FAILED(rv)) {
        return;
      }
    }
  }

  if (action == CACHE_SELECTION_RESELECT_WITHOUT_MANIFEST) {
    rv = SelectDocAppCacheNoManifest(applicationCache,
                                     getter_AddRefs(manifestURI),
                                     &action);
    if (NS_FAILED(rv)) {
      return;
    }
  }

  switch (action)
  {
  case CACHE_SELECTION_NONE:
    break;
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
    
    
    
    
    
    

    applicationCacheChannel->MarkOfflineCacheEntryAsForeign();

    nsCOMPtr<nsIWebNavigation> webNav = do_QueryInterface(mDocShell);

    webNav->Stop(nsIWebNavigation::STOP_ALL);
    webNav->Reload(nsIWebNavigation::LOAD_FLAGS_NONE);
    break;
  }
  default:
    NS_ASSERTION(false,
          "Cache selection algorithm didn't decide on proper action");
    break;
  }
}

void
nsContentSink::ScrollToRef()
{
  mDocument->ScrollToRef();
}

void
nsContentSink::StartLayout(bool aIgnorePendingSheets)
{
  if (mLayoutStarted) {
    
    return;
  }
  
  mDeferredLayoutStart = true;

  if (!aIgnorePendingSheets && WaitForPendingSheets()) {
    
    return;
  }

  mDeferredLayoutStart = false;

  
  
  
  
  
  
  FlushTags();

  mLayoutStarted = true;
  mLastNotificationTime = PR_Now();

  mDocument->SetMayStartLayout(true);
  nsCOMPtr<nsIPresShell> shell = mDocument->GetShell();
  
  
  
  
  
  if (shell && !shell->DidInitialize()) {
    nsRect r = shell->GetPresContext()->GetVisibleArea();
    nsCOMPtr<nsIPresShell> shellGrip = shell;
    nsresult rv = shell->Initialize(r.width, r.height);
    if (NS_FAILED(rv)) {
      return;
    }
  }

  
  

  mDocument->SetScrollToRef(mDocumentURI);
}

void
nsContentSink::NotifyAppend(nsIContent* aContainer, uint32_t aStartIndex)
{
  if (aContainer->GetCurrentDoc() != mDocument) {
    
    
    return;
  }

  mInNotification++;
  
  {
    
    MOZ_AUTO_DOC_UPDATE(mDocument, UPDATE_CONTENT_MODEL, !mBeganUpdate);
    nsNodeUtils::ContentAppended(aContainer,
                                 aContainer->GetChildAt(aStartIndex),
                                 aStartIndex);
    mLastNotificationTime = PR_Now();
  }

  mInNotification--;
}

NS_IMETHODIMP
nsContentSink::Notify(nsITimer *timer)
{
  if (mParsing) {
    
    mDroppedTimer = true;
    return NS_OK;
  }
  
#ifdef MOZ_DEBUG
  {
    PRTime now = PR_Now();

    int64_t interval = GetNotificationInterval();
    delay = int32_t(now - mLastNotificationTime - interval) / PR_USEC_PER_MSEC;

    mBackoffCount--;
    SINK_TRACE(gContentSinkLogModuleInfo, SINK_TRACE_REFLOW,
               ("nsContentSink::Notify: reflow on a timer: %d milliseconds "
                "late, backoff count: %d", delay, mBackoffCount));
  }
#endif

  if (WaitForPendingSheets()) {
    mDeferredFlushTags = true;
  } else {
    FlushTags();

    
    
    ScrollToRef();
  }

  mNotificationTimer = nullptr;
  return NS_OK;
}

bool
nsContentSink::IsTimeToNotify()
{
  if (!sNotifyOnTimer || !mLayoutStarted || !mBackoffCount ||
      mInMonolithicContainer) {
    return false;
  }

  if (WaitForPendingSheets()) {
    mDeferredFlushTags = true;
    return false;
  }

  PRTime now = PR_Now();
  int64_t interval, diff;

  LL_I2L(interval, GetNotificationInterval());
  diff = now - mLastNotificationTime;

  if (diff > interval) {
    mBackoffCount--;
    return true;
  }

  return false;
}

nsresult
nsContentSink::WillInterruptImpl()
{
  nsresult result = NS_OK;

  SINK_TRACE(gContentSinkLogModuleInfo, SINK_TRACE_CALLS,
             ("nsContentSink::WillInterrupt: this=%p", this));
#ifndef SINK_NO_INCREMENTAL
  if (WaitForPendingSheets()) {
    mDeferredFlushTags = true;
  } else if (sNotifyOnTimer && mLayoutStarted) {
    if (mBackoffCount && !mInMonolithicContainer) {
      int64_t now = PR_Now();
      int64_t interval = GetNotificationInterval();
      int64_t diff = now - mLastNotificationTime;

      
      if (diff > interval || mDroppedTimer) {
        mBackoffCount--;
        SINK_TRACE(gContentSinkLogModuleInfo, SINK_TRACE_REFLOW,
                   ("nsContentSink::WillInterrupt: flushing tags since we've "
                    "run out time; backoff count: %d", mBackoffCount));
        result = FlushTags();
        if (mDroppedTimer) {
          ScrollToRef();
          mDroppedTimer = false;
        }
      } else if (!mNotificationTimer) {
        interval -= diff;
        int32_t delay = interval;

        
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
            mNotificationTimer = nullptr;
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

  mParsing = false;

  return result;
}

nsresult
nsContentSink::WillResumeImpl()
{
  SINK_TRACE(gContentSinkLogModuleInfo, SINK_TRACE_CALLS,
             ("nsContentSink::WillResume: this=%p", this));

  mParsing = true;

  return NS_OK;
}

nsresult
nsContentSink::DidProcessATokenImpl()
{
  if (mRunsToCompletion || !mParser) {
    return NS_OK;
  }

  
  nsIPresShell *shell = mDocument->GetShell();
  if (!shell) {
    
    
    return NS_OK;
  }

  
  ++mDeflectedCount;

  
  if (sPendingEventMode != 0 && !mHasPendingEvent &&
      (mDeflectedCount % sEventProbeRate) == 0) {
    nsViewManager* vm = shell->GetViewManager();
    NS_ENSURE_TRUE(vm, NS_ERROR_FAILURE);
    nsCOMPtr<nsIWidget> widget;
    vm->GetRootWidget(getter_AddRefs(widget));
    mHasPendingEvent = widget && widget->HasPendingInputEvent();
  }

  if (mHasPendingEvent && sPendingEventMode == 2) {
    return NS_ERROR_HTMLPARSER_INTERRUPTED;
  }

  
  if (!mHasPendingEvent &&
      mDeflectedCount < uint32_t(mDynamicLowerValue ? sInteractiveDeflectCount :
                                                      sPerfDeflectCount)) {
    return NS_OK;
  }

  mDeflectedCount = 0;

  
  if (PR_IntervalToMicroseconds(PR_IntervalNow()) > mCurrentParseEndTime) {
    return NS_ERROR_HTMLPARSER_INTERRUPTED;
  }

  return NS_OK;
}



void
nsContentSink::FavorPerformanceHint(bool perfOverStarvation, uint32_t starvationDelay)
{
  static NS_DEFINE_CID(kAppShellCID, NS_APPSHELL_CID);
  nsCOMPtr<nsIAppShell> appShell = do_GetService(kAppShellCID);
  if (appShell)
    appShell->FavorPerformanceHint(perfOverStarvation, starvationDelay);
}

void
nsContentSink::BeginUpdate(nsIDocument *aDocument, nsUpdateType aUpdateType)
{
  
  if (mInNotification > 0 && mUpdatesInNotification < 2) {
    ++mUpdatesInNotification;
  }

  
  
  
  
  

  if (!mInNotification++) {
    FlushTags();
  }
}

void
nsContentSink::EndUpdate(nsIDocument *aDocument, nsUpdateType aUpdateType)
{
  
  
  
  
  
  if (!--mInNotification) {
    UpdateChildCounts();
  }
}

void
nsContentSink::DidBuildModelImpl(bool aTerminated)
{
  if (mDocument) {
    MOZ_ASSERT(mDocument->GetReadyStateEnum() ==
               nsIDocument::READYSTATE_LOADING, "Bad readyState");
    mDocument->SetReadyStateInternal(nsIDocument::READYSTATE_INTERACTIVE);
  }

  if (mScriptLoader) {
    mScriptLoader->ParsingComplete(aTerminated);
  }

  if (!mDocument->HaveFiredDOMTitleChange()) {
    mDocument->NotifyPossibleTitleChange(false);
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
  
  
  
  
  
  
  
  
  nsRefPtr<nsParserBase> kungFuDeathGrip(mParser.forget());

  if (mDynamicLowerValue) {
    
    
    FavorPerformanceHint(true, 0);
  }

  if (!mRunsToCompletion) {
    mDocument->UnblockOnload(true);
  }
}

bool
nsContentSink::IsScriptExecutingImpl()
{
  return !!mScriptLoader->GetCurrentScript();
}

nsresult
nsContentSink::WillParseImpl(void)
{
  if (mRunsToCompletion) {
    return NS_OK;
  }

  nsIPresShell *shell = mDocument->GetShell();
  if (!shell) {
    return NS_OK;
  }

  uint32_t currentTime = PR_IntervalToMicroseconds(PR_IntervalNow());

  if (sEnablePerfMode == 0) {
    nsViewManager* vm = shell->GetViewManager();
    NS_ENSURE_TRUE(vm, NS_ERROR_FAILURE);
    uint32_t lastEventTime;
    vm->GetLastUserEventTime(lastEventTime);

    bool newDynLower =
      mDocument->IsInBackgroundWindow() ||
      ((currentTime - mBeginLoadTime) > uint32_t(sInitialPerfTime) &&
       (currentTime - lastEventTime) < uint32_t(sInteractiveTime));
    
    if (mDynamicLowerValue != newDynLower) {
      FavorPerformanceHint(!newDynLower, 0);
      mDynamicLowerValue = newDynLower;
    }
  }
  
  mDeflectedCount = 0;
  mHasPendingEvent = false;

  mCurrentParseEndTime = currentTime +
    (mDynamicLowerValue ? sInteractiveParseTime : sPerfParseTime);

  return NS_OK;
}

void
nsContentSink::WillBuildModelImpl()
{
  if (!mRunsToCompletion) {
    mDocument->BlockOnload();

    mBeginLoadTime = PR_IntervalToMicroseconds(PR_IntervalNow());
  }

  mDocument->ResetScrolledToRefAlready();

  if (mProcessLinkHeaderEvent.get()) {
    mProcessLinkHeaderEvent.Revoke();

    DoProcessLinkHeader();
  }
}


void
nsContentSink::NotifyDocElementCreated(nsIDocument* aDoc)
{
  nsCOMPtr<nsIObserverService> observerService =
    mozilla::services::GetObserverService();
  if (observerService) {
    nsCOMPtr<nsIDOMDocument> domDoc = do_QueryInterface(aDoc);
    observerService->
      NotifyObservers(domDoc, "document-element-inserted",
                      EmptyString().get());
  }
}
