











































#include "nsContentSink.h"
#include "nsScriptLoader.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "mozilla/css/Loader.h"
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
#include "nsIDOMWindow.h"
#include "nsIPrincipal.h"
#include "nsIScriptGlobalObject.h"
#include "nsNetCID.h"
#include "nsIOfflineCacheUpdate.h"
#include "nsIApplicationCache.h"
#include "nsIApplicationCacheContainer.h"
#include "nsIApplicationCacheChannel.h"
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
#include "nsIAppShell.h"
#include "nsIWidget.h"
#include "nsWidgetsCID.h"
#include "nsIRequest.h"
#include "nsNodeUtils.h"
#include "nsIDOMNode.h"
#include "nsThreadUtils.h"
#include "nsPIDOMWindow.h"
#include "mozAutoDocUpdate.h"
#include "nsIWebNavigation.h"
#include "nsIDocumentLoader.h"
#include "nsICachingChannel.h"
#include "nsICacheEntryDescriptor.h"
#include "nsGenericHTMLElement.h"
#include "nsHTMLDNSPrefetch.h"
#include "nsISupportsPrimitives.h"
#include "mozilla/Preferences.h"
#include "nsParserConstants.h"

using namespace mozilla;

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
                                             bool aIsInline,
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
                                             bool aIsInline)
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
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mScriptLoader)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMARRAY(mScriptElements)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsContentSink)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mDocument)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mParser)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NATIVE_MEMBER(mNodeInfoManager,
                                                  nsNodeInfoManager)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mScriptLoader)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMARRAY(mScriptElements)
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

bool    nsContentSink::sNotifyOnTimer;
PRInt32 nsContentSink::sBackoffCount;
PRInt32 nsContentSink::sNotificationInterval;
PRInt32 nsContentSink::sInteractiveDeflectCount;
PRInt32 nsContentSink::sPerfDeflectCount;
PRInt32 nsContentSink::sPendingEventMode;
PRInt32 nsContentSink::sEventProbeRate;
PRInt32 nsContentSink::sInteractiveParseTime;
PRInt32 nsContentSink::sPerfParseTime;
PRInt32 nsContentSink::sInteractiveTime;
PRInt32 nsContentSink::sInitialPerfTime;
PRInt32 nsContentSink::sEnablePerfMode;
bool    nsContentSink::sCanInterruptParser;

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
  Preferences::AddBoolVarCache(&sCanInterruptParser,
                               "content.interrupt.parsing", true);
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

  if (!mFragmentMode) {
    if (mDocShell) {
      PRUint32 loadType = 0;
      mDocShell->GetLoadType(&loadType);
      mDocument->SetChangeScrollPosWhenScrollingToRef(
        (loadType & nsIDocShell::LOAD_CMD_HISTORY) == 0);
    }

    
    nsCOMPtr<nsIScriptLoaderObserver> proxy =
      new nsScriptLoaderObserverProxy(this);
    NS_ENSURE_TRUE(proxy, NS_ERROR_OUT_OF_MEMORY);

    mScriptLoader->AddObserver(proxy);

    ProcessHTTPHeaders(aChannel);
  }

  mCSSLoader = aDoc->CSSLoader();

  mNodeInfoManager = aDoc->NodeInfoManager();

  mBackoffCount = sBackoffCount;

  if (sEnablePerfMode != 0) {
    mDynamicLowerValue = sEnablePerfMode == 1;
    FavorPerformanceHint(!mDynamicLowerValue, 0);
  }

  
  
  mCanInterruptParser = !mFragmentMode && sCanInterruptParser;

  return NS_OK;
}

NS_IMETHODIMP
nsContentSink::StyleSheetLoaded(nsCSSStyleSheet* aSheet,
                                bool aWasAlternate,
                                nsresult aStatus)
{
  NS_ASSERTION(!mFragmentMode, "How come a fragment parser observed sheets?");
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

NS_IMETHODIMP
nsContentSink::ScriptAvailable(nsresult aResult,
                               nsIScriptElement *aElement,
                               bool aIsInline,
                               nsIURI *aURI,
                               PRInt32 aLineNo)
{
  PRUint32 count = mScriptElements.Count();

  
  
  
  NS_ASSERTION(count == 0 ||
               mScriptElements.IndexOf(aElement) == PRInt32(count - 1) ||
               mScriptElements.IndexOf(aElement) == -1,
               "script found at unexpected position");

  
  if (count == 0 || aElement != mScriptElements[count - 1]) {
    return NS_OK;
  }

  NS_ASSERTION(!aElement->GetScriptDeferred(), "defer script was in mScriptElements");
  NS_ASSERTION(!aElement->GetScriptAsync(), "async script was in mScriptElements");

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
                               bool aIsInline)
{
  mDeflectedCount = sPerfDeflectCount;

  
  PRInt32 count = mScriptElements.Count();
  if (count == 0 || aElement != mScriptElements[count - 1]) {
    return NS_OK;
  }

  NS_ASSERTION(!aElement->GetScriptDeferred(), "defer script was in mScriptElements");
  NS_ASSERTION(!aElement->GetScriptAsync(), "async script was in mScriptElements");

  
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
  ProcessLinkHeader(nsnull, value);
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
      nsnull, contextUri);
  
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

nsresult
nsContentSink::ProcessLinkHeader(nsIContent* aElement,
                                 const nsAString& aLinkData)
{
  nsresult rv = NS_OK;

  
  bool seenParameters = false;

  
  nsAutoString href;
  nsAutoString rel;
  nsAutoString title;
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

    bool needsUnescape = false;
    
    
    while (*end != kNullCh && *end != kSemicolon && *end != kComma) {
      PRUnichar ch = *end;

      if (ch == kQuote || ch == kLessThan) {
        

        PRUnichar quote = ch;
        if (quote == kLessThan) {
          quote = kGreaterThan;
        }
        
        needsUnescape = (ch == kQuote);
        
        PRUnichar* closeQuote = (end + 1);

        
        while (*closeQuote != kNullCh && quote != *closeQuote) {
          
          if (needsUnescape && *closeQuote == kBackSlash && *(closeQuote + 1) != kNullCh) {
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

          if (needsUnescape) {
            
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
        rv = ProcessLink(aElement, anchor, href, rel, title, type, media);
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
    rv = ProcessLink(aElement, anchor, href, rel, title, type, media);
  }

  return rv;
}


nsresult
nsContentSink::ProcessLink(nsIContent* aElement,
                           const nsSubstring& aAnchor, const nsSubstring& aHref,
                           const nsSubstring& aRel, const nsSubstring& aTitle,
                           const nsSubstring& aType, const nsSubstring& aMedia)
{
  
  nsTArray<nsString> linkTypes;
  nsStyleLinkElement::ParseLinkTypes(aRel, linkTypes);

  
  
  
  
  if (!LinkContextIsOurDocument(aAnchor)) {
    return NS_OK;
  }
  
  bool hasPrefetch = linkTypes.Contains(NS_LITERAL_STRING("prefetch"));
  
  if (hasPrefetch || linkTypes.Contains(NS_LITERAL_STRING("next"))) {
    PrefetchHref(aHref, aElement, hasPrefetch);
  }

  if ((!aHref.IsEmpty()) && linkTypes.Contains(NS_LITERAL_STRING("dns-prefetch"))) {
    PrefetchDNS(aHref);
  }

  
  if (!linkTypes.Contains(NS_LITERAL_STRING("stylesheet"))) {
    return NS_OK;
  }

  bool isAlternate = linkTypes.Contains(NS_LITERAL_STRING("alternate"));
  return ProcessStyleLink(aElement, aHref, isAlternate, aTitle, aType,
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
  nsParserUtils::SplitMimeType(aType, mimeType, params);

  
  if (!mimeType.IsEmpty() && !mimeType.LowerCaseEqualsLiteral("text/css")) {
    
    return NS_OK;
  }

  nsCOMPtr<nsIURI> url;
  nsresult rv = NS_NewURI(getter_AddRefs(url), aHref, nsnull,
                          mDocument->GetDocBaseURI());
  
  if (NS_FAILED(rv)) {
    
    return NS_OK;
  }

  
  bool isAlternate;
  rv = mCSSLoader->LoadStyleLink(aElement, url, aTitle, aMedia, aAlternate,
                                 mFragmentMode ? nsnull : this, &isAlternate);
  NS_ENSURE_SUCCESS(rv, rv);
  
  if (!isAlternate && !mFragmentMode) {
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
      ToLowerCase(header);
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
      ToLowerCase(result);
      mDocument->SetHeaderData(nsGkAtoms::handheldFriendly, result);
    }
  }

  return rv;
}


void
nsContentSink::PrefetchHref(const nsAString &aHref,
                            nsIContent *aSource,
                            bool aExplicit)
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
    nsCAutoString host;
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
    nsCAutoString groupID;
    rv = aLoadApplicationCache->GetGroupID(groupID);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIURI> groupURI;
    rv = NS_NewURI(getter_AddRefs(groupURI), groupID);
    NS_ENSURE_SUCCESS(rv, rv);

    bool equal = false;
    rv = groupURI->Equals(aManifestURI, &equal);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!equal) {
      
      

      *aAction = CACHE_SELECTION_RELOAD;
    }
    else {
      
      
      
#ifdef NS_DEBUG
      nsCAutoString docURISpec, clientID;
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
  *aManifestURI = nsnull;
  *aAction = CACHE_SELECTION_NONE;

  nsresult rv;

  if (aLoadApplicationCache) {
    
    
    nsCOMPtr<nsIApplicationCacheContainer> applicationCacheDocument =
      do_QueryInterface(mDocument);
    NS_ASSERTION(applicationCacheDocument,
                 "mDocument must implement nsIApplicationCacheContainer.");

#ifdef NS_DEBUG
    nsCAutoString docURISpec, clientID;
    mDocumentURI->GetAsciiSpec(docURISpec);
    aLoadApplicationCache->GetClientID(clientID);
    SINK_TRACE(gContentSinkLogModuleInfo, SINK_TRACE_CALLS,
        ("Selection, no manifest: assigning app cache %s to document %s", clientID.get(), docURISpec.get()));
#endif

    rv = applicationCacheDocument->SetApplicationCache(aLoadApplicationCache);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    nsCAutoString groupID;
    rv = aLoadApplicationCache->GetGroupID(groupID);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = NS_NewURI(aManifestURI, groupID);
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

    
    rv = mDocument->NodePrincipal()->CheckMayLoad(manifestURI, true);
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
        nsCAutoString method;
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
  
  
  
  
  
  if (shell && !shell->DidInitialReflow()) {
    nsRect r = shell->GetPresContext()->GetVisibleArea();
    nsCOMPtr<nsIPresShell> shellGrip = shell;
    nsresult rv = shell->InitialReflow(r.width, r.height);
    if (NS_FAILED(rv)) {
      return;
    }
  }

  
  

  mDocument->SetScrollToRef(mDocumentURI);
}

void
nsContentSink::NotifyAppend(nsIContent* aContainer, PRUint32 aStartIndex)
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
    mDeferredFlushTags = true;
  } else {
    FlushTags();

    
    
    ScrollToRef();
  }

  mNotificationTimer = nsnull;
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
  PRInt64 interval, diff;

  LL_I2L(interval, GetNotificationInterval());
  LL_SUB(diff, now, mLastNotificationTime);

  if (LL_CMP(diff, >, interval)) {
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
          mDroppedTimer = false;
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
  if (!mCanInterruptParser || !mParser || !mParser->CanInterrupt()) {
    return NS_OK;
  }

  
  nsIPresShell *shell = mDocument->GetShell();
  if (!shell) {
    
    
    return NS_OK;
  }

  
  ++mDeflectedCount;

  
  if (sPendingEventMode != 0 && !mHasPendingEvent &&
      (mDeflectedCount % sEventProbeRate) == 0) {
    nsIViewManager* vm = shell->GetViewManager();
    NS_ENSURE_TRUE(vm, NS_ERROR_FAILURE);
    nsCOMPtr<nsIWidget> widget;
    vm->GetRootWidget(getter_AddRefs(widget));
    mHasPendingEvent = widget && widget->HasPendingInputEvent();
  }

  if (mHasPendingEvent && sPendingEventMode == 2) {
    return NS_ERROR_HTMLPARSER_INTERRUPTED;
  }

  
  if (!mHasPendingEvent &&
      mDeflectedCount < PRUint32(mDynamicLowerValue ? sInteractiveDeflectCount :
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
nsContentSink::FavorPerformanceHint(bool perfOverStarvation, PRUint32 starvationDelay)
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
  if (mDocument && !aTerminated) {
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
  
  
  
  
  
  
  
  
  nsCOMPtr<nsIParser> kungFuDeathGrip(mParser.forget());

  if (mDynamicLowerValue) {
    
    
    FavorPerformanceHint(true, 0);
  }

  if (mCanInterruptParser) {
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
  if (!mCanInterruptParser) {
    return NS_OK;
  }

  nsIPresShell *shell = mDocument->GetShell();
  if (!shell) {
    return NS_OK;
  }

  PRUint32 currentTime = PR_IntervalToMicroseconds(PR_IntervalNow());

  if (sEnablePerfMode == 0) {
    nsIViewManager* vm = shell->GetViewManager();
    NS_ENSURE_TRUE(vm, NS_ERROR_FAILURE);
    PRUint32 lastEventTime;
    vm->GetLastUserEventTime(lastEventTime);

    bool newDynLower =
      (currentTime - mBeginLoadTime) > PRUint32(sInitialPerfTime) &&
      (currentTime - lastEventTime) < PRUint32(sInteractiveTime);
    
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
  if (mCanInterruptParser) {
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
nsContentSink::ContinueInterruptedParsingIfEnabled()
{
  
  if (mParser && mParser->IsParserEnabled()) {
    mParser->ContinueInterruptedParsing();
  }
}


void
nsContentSink::ContinueInterruptedParsingAsync()
{
  nsCOMPtr<nsIRunnable> ev = NS_NewRunnableMethod(this,
    &nsContentSink::ContinueInterruptedParsingIfEnabled);

  NS_DispatchToCurrentThread(ev);
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
