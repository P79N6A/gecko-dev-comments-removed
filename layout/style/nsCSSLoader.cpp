
















































#include "nsCSSLoader.h"
#include "nsIContent.h"
#include "nsIDOMNode.h"
#include "nsIDOMWindow.h"
#include "nsIDocument.h"
#include "nsIDOMNSDocumentStyle.h"
#include "nsIUnicharInputStream.h"
#include "nsIConverterInputStream.h"
#include "nsICharsetAlias.h"
#include "nsUnicharUtils.h"
#include "nsHashtable.h"
#include "nsIURI.h"
#include "nsIServiceManager.h"
#include "nsNetUtil.h"
#include "nsContentUtils.h"
#include "nsCRT.h"
#include "nsIScriptSecurityManager.h"
#include "nsContentPolicyUtils.h"
#include "nsITimelineService.h"
#include "nsIHttpChannel.h"
#include "nsIScriptError.h"
#include "nsMimeTypes.h"
#include "nsIAtom.h"
#include "nsIDOM3Node.h"
#include "nsICSSStyleSheet.h"
#include "nsIStyleSheetLinkingElement.h"
#include "nsICSSLoaderObserver.h"
#include "nsICSSLoader.h"
#include "nsICSSParser.h"
#include "nsICSSImportRule.h"
#include "nsThreadUtils.h"
#include "nsGkAtoms.h"
#include "nsDocShellCID.h"
#include "nsIChannelClassifier.h"

#ifdef MOZ_XUL
#include "nsXULPrototypeCache.h"
#endif

#include "nsIMediaList.h"
#include "nsIDOMStyleSheet.h"
#include "nsIDOMCSSStyleSheet.h"
#include "nsIDOMCSSImportRule.h"
#include "nsContentErrors.h"

#ifdef MOZ_LOGGING

#endif 
#include "prlog.h"

#ifdef PR_LOGGING
static PRLogModuleInfo *gLoaderLog = PR_NewLogModule("nsCSSLoader");
#endif 

#define LOG_FORCE(args) PR_LOG(gLoaderLog, PR_LOG_ALWAYS, args)
#define LOG_ERROR(args) PR_LOG(gLoaderLog, PR_LOG_ERROR, args)
#define LOG_WARN(args) PR_LOG(gLoaderLog, PR_LOG_WARNING, args)
#define LOG_DEBUG(args) PR_LOG(gLoaderLog, PR_LOG_DEBUG, args)
#define LOG(args) LOG_DEBUG(args)

#define LOG_FORCE_ENABLED() PR_LOG_TEST(gLoaderLog, PR_LOG_ALWAYS)
#define LOG_ERROR_ENABLED() PR_LOG_TEST(gLoaderLog, PR_LOG_ERROR)
#define LOG_WARN_ENABLED() PR_LOG_TEST(gLoaderLog, PR_LOG_WARNING)
#define LOG_DEBUG_ENABLED() PR_LOG_TEST(gLoaderLog, PR_LOG_DEBUG)
#define LOG_ENABLED() LOG_DEBUG_ENABLED()

#ifdef PR_LOGGING
#define LOG_URI(format, uri)                        \
  PR_BEGIN_MACRO                                    \
    NS_ASSERTION(uri, "Logging null uri");          \
    if (LOG_ENABLED()) {                            \
      nsCAutoString _logURISpec;                    \
      uri->GetSpec(_logURISpec);                    \
      LOG((format, _logURISpec.get()));             \
    }                                               \
  PR_END_MACRO
#else 
#define LOG_URI(format, uri)
#endif 


#ifdef PR_LOGGING
static const char* const gStateStrings[] = {
  "eSheetStateUnknown",
  "eSheetNeedsParser",
  "eSheetPending",
  "eSheetLoading",
  "eSheetComplete"
};
#endif




NS_IMPL_ISUPPORTS2(SheetLoadData, nsIUnicharStreamLoaderObserver, nsIRunnable)

SheetLoadData::SheetLoadData(CSSLoaderImpl* aLoader,
                             const nsSubstring& aTitle,
                             nsIURI* aURI,
                             nsICSSStyleSheet* aSheet,
                             nsIStyleSheetLinkingElement* aOwningElement,
                             PRBool aIsAlternate,
                             nsICSSLoaderObserver* aObserver,
                             nsIPrincipal* aLoaderPrincipal)
  : mLoader(aLoader),
    mTitle(aTitle),
    mURI(aURI),
    mLineNumber(1),
    mSheet(aSheet),
    mNext(nsnull),
    mParentData(nsnull),
    mPendingChildren(0),
    mSyncLoad(PR_FALSE),
    mIsNonDocumentSheet(PR_FALSE),
    mIsLoading(PR_FALSE),
    mIsCancelled(PR_FALSE),
    mMustNotify(PR_FALSE),
    mWasAlternate(aIsAlternate),
    mAllowUnsafeRules(PR_FALSE),
    mUseSystemPrincipal(PR_FALSE),
    mOwningElement(aOwningElement),
    mObserver(aObserver),
    mLoaderPrincipal(aLoaderPrincipal)
{
  NS_PRECONDITION(mLoader, "Must have a loader!");
  NS_ADDREF(mLoader);
}

SheetLoadData::SheetLoadData(CSSLoaderImpl* aLoader,
                             nsIURI* aURI,
                             nsICSSStyleSheet* aSheet,
                             SheetLoadData* aParentData,
                             nsICSSLoaderObserver* aObserver,
                             nsIPrincipal* aLoaderPrincipal)
  : mLoader(aLoader),
    mURI(aURI),
    mLineNumber(1),
    mSheet(aSheet),
    mNext(nsnull),
    mParentData(aParentData),
    mPendingChildren(0),
    mSyncLoad(PR_FALSE),
    mIsNonDocumentSheet(PR_FALSE),
    mIsLoading(PR_FALSE),
    mIsCancelled(PR_FALSE),
    mMustNotify(PR_FALSE),
    mWasAlternate(PR_FALSE),
    mAllowUnsafeRules(PR_FALSE),
    mUseSystemPrincipal(PR_FALSE),
    mOwningElement(nsnull),
    mObserver(aObserver),
    mLoaderPrincipal(aLoaderPrincipal)
{
  NS_PRECONDITION(mLoader, "Must have a loader!");
  NS_ADDREF(mLoader);
  if (mParentData) {
    NS_ADDREF(mParentData);
    mSyncLoad = mParentData->mSyncLoad;
    mIsNonDocumentSheet = mParentData->mIsNonDocumentSheet;
    mAllowUnsafeRules = mParentData->mAllowUnsafeRules;
    mUseSystemPrincipal = mParentData->mUseSystemPrincipal;
    ++(mParentData->mPendingChildren);
  }

  NS_POSTCONDITION(!mUseSystemPrincipal || mSyncLoad,
                   "Shouldn't use system principal for async loads");
}

SheetLoadData::SheetLoadData(CSSLoaderImpl* aLoader,
                             nsIURI* aURI,
                             nsICSSStyleSheet* aSheet,
                             PRBool aSyncLoad,
                             PRBool aAllowUnsafeRules,
                             PRBool aUseSystemPrincipal,
                             const nsCString& aCharset,
                             nsICSSLoaderObserver* aObserver,
                             nsIPrincipal* aLoaderPrincipal)
  : mLoader(aLoader),
    mURI(aURI),
    mLineNumber(1),
    mSheet(aSheet),
    mNext(nsnull),
    mParentData(nsnull),
    mPendingChildren(0),
    mSyncLoad(aSyncLoad),
    mIsNonDocumentSheet(PR_TRUE),
    mIsLoading(PR_FALSE),
    mIsCancelled(PR_FALSE),
    mMustNotify(PR_FALSE),
    mWasAlternate(PR_FALSE),
    mAllowUnsafeRules(aAllowUnsafeRules),
    mUseSystemPrincipal(aUseSystemPrincipal),
    mOwningElement(nsnull),
    mObserver(aObserver),
    mLoaderPrincipal(aLoaderPrincipal),
    mCharsetHint(aCharset)
{
  NS_PRECONDITION(mLoader, "Must have a loader!");
  NS_ADDREF(mLoader);

  NS_POSTCONDITION(!mUseSystemPrincipal || mSyncLoad,
                   "Shouldn't use system principal for async loads");
}

SheetLoadData::~SheetLoadData()
{
  NS_RELEASE(mLoader);
  NS_IF_RELEASE(mParentData);
  NS_IF_RELEASE(mNext);
}

NS_IMETHODIMP
SheetLoadData::Run()
{
  mLoader->HandleLoadEvent(this);
  return NS_OK;
}






nsCOMArray<nsICSSParser>* CSSLoaderImpl::gParsers = nsnull;

CSSLoaderImpl::CSSLoaderImpl(void)
  : mDocument(nsnull), 
    mEnabled(PR_TRUE), 
    mCompatMode(eCompatibility_FullStandards),
    mDatasToNotifyOn(0)
{
}

CSSLoaderImpl::~CSSLoaderImpl(void)
{
  NS_ASSERTION((!mLoadingDatas.IsInitialized()) || mLoadingDatas.Count() == 0,
               "How did we get destroyed when there are loading data?");
  NS_ASSERTION((!mPendingDatas.IsInitialized()) || mPendingDatas.Count() == 0,
               "How did we get destroyed when there are pending data?");
  
  
  
}

NS_IMPL_ISUPPORTS1(CSSLoaderImpl, nsICSSLoader)

void
CSSLoaderImpl::Shutdown()
{
  delete gParsers;
  gParsers = nsnull;
}

NS_IMETHODIMP
CSSLoaderImpl::Init(nsIDocument* aDocument)
{
  NS_ASSERTION(! mDocument, "already initialized");

  mDocument = aDocument;

  
  
  
  nsCOMPtr<nsIDOMNSDocumentStyle> domDoc(do_QueryInterface(mDocument));
  if (domDoc) {
    domDoc->GetPreferredStyleSheetSet(mPreferredSheet);
  }
  return NS_OK;
}

NS_IMETHODIMP
CSSLoaderImpl::DropDocumentReference(void)
{
  mDocument = nsnull;
  
  
  
  if (mPendingDatas.IsInitialized()) {
    StartAlternateLoads();
  }
  return NS_OK;
}

NS_IMETHODIMP
CSSLoaderImpl::SetCompatibilityMode(nsCompatibility aCompatMode)
{
  mCompatMode = aCompatMode;
  return NS_OK;
}

static PLDHashOperator
CollectNonAlternates(nsURIAndPrincipalHashKey *aKey,
                     SheetLoadData* &aData,
                     void* aClosure)
{
  NS_PRECONDITION(aData, "Must have a data");
  NS_PRECONDITION(aClosure, "Must have an array");
  
  
  
  if (aData->mLoader->IsAlternate(aData->mTitle, PR_TRUE)) {
    return PL_DHASH_NEXT;
  }

  static_cast<CSSLoaderImpl::LoadDataArray*>(aClosure)->AppendElement(aData);
  return PL_DHASH_REMOVE;
}

NS_IMETHODIMP
CSSLoaderImpl::SetPreferredSheet(const nsAString& aTitle)
{
#ifdef DEBUG
  nsCOMPtr<nsIDOMNSDocumentStyle> doc(do_QueryInterface(mDocument));
  if (doc) {
    nsAutoString currentPreferred;
    doc->GetLastStyleSheetSet(currentPreferred);
    if (DOMStringIsNull(currentPreferred)) {
      doc->GetPreferredStyleSheetSet(currentPreferred);
    }
    NS_ASSERTION(currentPreferred.Equals(aTitle),
                 "Unexpected argument to SetPreferredSheet");    
  }
#endif
  
  mPreferredSheet = aTitle;

  
  if (mPendingDatas.IsInitialized()) {
    LoadDataArray arr(mPendingDatas.Count());
    mPendingDatas.Enumerate(CollectNonAlternates, &arr);

    mDatasToNotifyOn += arr.Length();
    for (PRUint32 i = 0; i < arr.Length(); ++i) {
      --mDatasToNotifyOn;
      LoadSheet(arr[i], eSheetNeedsParser);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
CSSLoaderImpl::GetPreferredSheet(nsAString& aTitle)
{
  aTitle.Assign(mPreferredSheet);
  return NS_OK;
}

NS_IMETHODIMP
CSSLoaderImpl::GetParserFor(nsICSSStyleSheet* aSheet, 
                            nsICSSParser** aParser)
{
  NS_PRECONDITION(aParser, "Null out param");

  *aParser = nsnull;

  if (!gParsers) {
    gParsers = new nsCOMArray<nsICSSParser>;
    if (!gParsers) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  PRInt32 count = gParsers->Count();
  if (0 < count--) {
    *aParser = gParsers->ObjectAt(count);
    NS_ADDREF(*aParser);
    gParsers->RemoveObjectAt(count);
  }

  nsresult result = NS_OK;
  if (! *aParser) {
    result = NS_NewCSSParser(aParser);
  }
  
  if (*aParser) {
    (*aParser)->SetQuirkMode(mCompatMode == eCompatibility_NavQuirks);
    if (aSheet) {
      (*aParser)->SetStyleSheet(aSheet);
    }
    (*aParser)->SetChildLoader(this);
  }
  return result;
}

NS_IMETHODIMP
CSSLoaderImpl::RecycleParser(nsICSSParser* aParser)
{
  NS_PRECONDITION(aParser, "Recycle only good parsers, please");
  NS_ENSURE_TRUE(gParsers, NS_ERROR_UNEXPECTED);

  if (!gParsers->AppendObject(aParser)) {
    return NS_ERROR_FAILURE;
  }

  
  aParser->SetStyleSheet(nsnull);
  
  return NS_OK;
}

static const char kCharsetSym[] = "@charset \"";

static nsresult GetCharsetFromData(const unsigned char* aStyleSheetData,
                                   PRUint32 aDataLength,
                                   nsACString& aCharset)
{
  aCharset.Truncate();
  if (aDataLength <= sizeof(kCharsetSym) - 1)
    return NS_ERROR_NOT_AVAILABLE;
  PRUint32 step = 1;
  PRUint32 pos = 0;
  PRBool bigEndian = PR_FALSE;
  
  
  
  
  
  if (*aStyleSheetData == 0x40 && *(aStyleSheetData+1) == 0x63  ) {
    
    step = 1;
    pos = 0;
  }
  
  
  else if (aStyleSheetData[0] == 0x00 &&
           aStyleSheetData[1] == 0x00 &&
           aStyleSheetData[2] == 0xFF &&
           aStyleSheetData[3] == 0xFE) {
    
    NS_WARNING("Our unicode decoders aren't likely  to deal with this one");
    step = 4;
    pos = 6;
    aCharset = "UTF-32";
  }
  else if (aStyleSheetData[0] == 0xFE &&
           aStyleSheetData[1] == 0xFF &&
           aStyleSheetData[2] == 0x00 &&
           aStyleSheetData[3] == 0x00) {
    
    NS_WARNING("Our unicode decoders aren't likely  to deal with this one");
    step = 4;
    pos = 5;
    aCharset = "UTF-32";
  }
  else if (nsContentUtils::CheckForBOM(aStyleSheetData,
                                       aDataLength, aCharset, &bigEndian)) {
    if (aCharset.Equals("UTF-8")) {
      step = 1;
      pos = 3;
    }
    else if (aCharset.Equals("UTF-32")) {
      step = 4;
      pos = bigEndian ? 7 : 4;
    }
    else if (aCharset.Equals("UTF-16")) {
      step = 2;
      pos = bigEndian ? 3 : 2;
    }
  }
  else if (aStyleSheetData[0] == 0x00 &&
           aStyleSheetData[1] == 0x00 &&
           aStyleSheetData[2] == 0x00 &&
           aStyleSheetData[3] == 0x40) {
    
    step = 4;
    pos = 3;
  }
  else if (aStyleSheetData[0] == 0x40 &&
           aStyleSheetData[1] == 0x00 &&
           aStyleSheetData[2] == 0x00 &&
           aStyleSheetData[3] == 0x00) {
    
    step = 4;
    pos = 0;
  }
  else if (aStyleSheetData[0] == 0x00 &&
           aStyleSheetData[1] == 0x00 &&
           aStyleSheetData[2] == 0x40 &&
           aStyleSheetData[3] == 0x00) {
    
    step = 4;
    pos = 2;
  }
  else if (aStyleSheetData[0] == 0x00 &&
           aStyleSheetData[1] == 0x40 &&
           aStyleSheetData[2] == 0x00 &&
           aStyleSheetData[3] == 0x00) {
    
    step = 4;
    pos = 1;
  }
  else if (aStyleSheetData[0] == 0x00 &&
           aStyleSheetData[1] == 0x40 &&
           aStyleSheetData[2] == 0x00 &&
           aStyleSheetData[3] == 0x63) {
    
    step = 2;
    pos = 1;
  }
  else if (aStyleSheetData[0] == 0x40 &&
           aStyleSheetData[1] == 0x00 &&
           aStyleSheetData[2] == 0x63 &&
           aStyleSheetData[3] == 0x00) {
    
    step = 2;
    pos = 0;
  }
  else {
    
    return NS_ERROR_UNEXPECTED;
  }

  PRUint32 index = 0;
  while (pos < aDataLength && index < sizeof(kCharsetSym) - 1) {
    if (aStyleSheetData[pos] != kCharsetSym[index]) {
      
      
      
      return aCharset.IsEmpty() ? NS_ERROR_NOT_AVAILABLE : NS_OK;
    }
    ++index;
    pos += step;
  }

  nsCAutoString charset;
  while (pos < aDataLength) {
    if (aStyleSheetData[pos] == '"') {
      break;
    }
    
    
    charset.Append(char(aStyleSheetData[pos]));
    pos += step;
  }

  
  pos += step;
  if (pos >= aDataLength || aStyleSheetData[pos] != ';') {
    return aCharset.IsEmpty() ? NS_ERROR_NOT_AVAILABLE : NS_OK;
  }

  aCharset = charset;
  return NS_OK;
}

NS_IMETHODIMP
SheetLoadData::OnDetermineCharset(nsIUnicharStreamLoader* aLoader,
                                  nsISupports* aContext,
                                  const char* aData,
                                  PRUint32 aDataLength,
                                  nsACString& aCharset)
{
  NS_PRECONDITION(!mOwningElement || mCharsetHint.IsEmpty(),
                  "Can't have element _and_ charset hint");

  LOG_URI("SheetLoadData::OnDetermineCharset for '%s'", mURI);
  nsCOMPtr<nsIChannel> channel;
  nsresult result = aLoader->GetChannel(getter_AddRefs(channel));
  if (NS_FAILED(result))
    channel = nsnull;

  aCharset.Truncate();

  








  if (channel) {
    channel->GetContentCharset(aCharset);
  }

  result = NS_ERROR_NOT_AVAILABLE;

#ifdef PR_LOGGING
  if (! aCharset.IsEmpty()) {
    LOG(("  Setting from HTTP to: %s", PromiseFlatCString(aCharset).get()));
  }
#endif

  if (aCharset.IsEmpty()) {
    
    
    result = GetCharsetFromData((const unsigned char*)aData,
                                aDataLength, aCharset);
#ifdef PR_LOGGING
    if (NS_SUCCEEDED(result)) {
      LOG(("  Setting from @charset rule or BOM: %s",
           PromiseFlatCString(aCharset).get()));
    }
#endif
  }

  if (aCharset.IsEmpty()) {
    
    
    if (mOwningElement) {
      nsAutoString elementCharset;
      mOwningElement->GetCharset(elementCharset);
      LossyCopyUTF16toASCII(elementCharset, aCharset);
#ifdef PR_LOGGING
      if (! aCharset.IsEmpty()) {
        LOG(("  Setting from property on element: %s",
             PromiseFlatCString(aCharset).get()));
      }
#endif
    } else {
      
      aCharset = mCharsetHint;
    }
  }

  if (aCharset.IsEmpty() && mParentData) {
    aCharset = mParentData->mCharset;
#ifdef PR_LOGGING
    if (! aCharset.IsEmpty()) {
      LOG(("  Setting from parent sheet: %s",
           PromiseFlatCString(aCharset).get()));
    }
#endif
  }
  
  if (aCharset.IsEmpty() && mLoader->mDocument) {
    
    aCharset = mLoader->mDocument->GetDocumentCharacterSet();
#ifdef PR_LOGGING
    LOG(("  Set from document: %s", PromiseFlatCString(aCharset).get()));
#endif
  }      

  if (aCharset.IsEmpty()) {
    NS_WARNING("Unable to determine charset for sheet, using ISO-8859-1!");
#ifdef PR_LOGGING
    LOG_WARN(("  Falling back to ISO-8859-1"));
#endif
    aCharset.AssignLiteral("ISO-8859-1");
  }

  mCharset = aCharset;
  return NS_OK;
}

already_AddRefed<nsIURI>
SheetLoadData::GetReferrerURI()
{
  nsIURI* uri = nsnull;
  if (mParentData)
    mParentData->mSheet->GetSheetURI(&uri);
  if (!uri && mLoader->mDocument)
    NS_IF_ADDREF(uri = mLoader->mDocument->GetDocumentURI());
  return uri;
}






NS_IMETHODIMP
SheetLoadData::OnStreamComplete(nsIUnicharStreamLoader* aLoader,
                                nsISupports* aContext,
                                nsresult aStatus,
                                nsIUnicharInputStream* aDataStream)
{
  LOG(("SheetLoadData::OnStreamComplete"));
  NS_ASSERTION(!mLoader->mSyncCallback, "Synchronous callback from necko");

  if (mIsCancelled) {
    
    
    
    return NS_OK;
  }
  
  if (!mLoader->mDocument && !mIsNonDocumentSheet) {
    
    LOG_WARN(("  No document and not non-document sheet; dropping load"));
    mLoader->SheetComplete(this, NS_BINDING_ABORTED);
    return NS_OK;
  }
  
  if (NS_FAILED(aStatus)) {
    LOG_WARN(("  Load failed: status 0x%x", aStatus));
    mLoader->SheetComplete(this, aStatus);
    return NS_OK;
  }

  nsCOMPtr<nsIChannel> channel;
  nsresult result = aLoader->GetChannel(getter_AddRefs(channel));
  if (NS_FAILED(result)) {
    LOG_WARN(("  No channel from loader"));
    mLoader->SheetComplete(this, result);
    return NS_OK;
  }
  
  nsCOMPtr<nsIURI> originalURI;
  channel->GetOriginalURI(getter_AddRefs(originalURI));

  
  
  
  
  
  nsCOMPtr<nsIURI> channelURI;
  NS_GetFinalChannelURI(channel, getter_AddRefs(channelURI));

  if (!channelURI || !originalURI) {
    NS_ERROR("Someone just violated the nsIRequest contract");
    LOG_WARN(("  Channel without a URI.  Bad!"));
    mLoader->SheetComplete(this, NS_ERROR_UNEXPECTED);
    return NS_OK;
  }

  nsCOMPtr<nsIPrincipal> principal;
  nsIScriptSecurityManager* secMan = nsContentUtils::GetSecurityManager();
  result = NS_ERROR_NOT_AVAILABLE;
  if (secMan) {  
    result = secMan->GetChannelPrincipal(channel, getter_AddRefs(principal));
  }

  if (NS_FAILED(result)) {
    LOG_WARN(("  Couldn't get principal"));
    mLoader->SheetComplete(this, result);
    return NS_OK;
  }
  
  mSheet->SetPrincipal(principal);
  
#ifdef MOZ_TIMELINE
  NS_TIMELINE_OUTDENT();
  NS_TIMELINE_MARK_CHANNEL("SheetLoadData::OnStreamComplete(%s)", channel);
#endif 
  
  
  
  nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(channel));
  if (httpChannel) {
    PRBool requestSucceeded;
    result = httpChannel->GetRequestSucceeded(&requestSucceeded);
    if (NS_SUCCEEDED(result) && !requestSucceeded) {
      LOG(("  Load returned an error page"));
      mLoader->SheetComplete(this, NS_ERROR_NOT_AVAILABLE);
      return NS_OK;
    }
  }

  if (aDataStream) {
    nsCAutoString contentType;
    if (channel) {
      channel->GetContentType(contentType);
    }
    
    PRBool validType = contentType.EqualsLiteral("text/css") ||
      contentType.EqualsLiteral(UNKNOWN_CONTENT_TYPE) ||
      contentType.IsEmpty();
                                          
    if (!validType) {
      nsCAutoString spec;
      channelURI->GetSpec(spec);

      const nsAFlatString& specUTF16 = NS_ConvertUTF8toUTF16(spec);
      const nsAFlatString& ctypeUTF16 = NS_ConvertASCIItoUTF16(contentType);
      const PRUnichar *strings[] = { specUTF16.get(), ctypeUTF16.get() };

      const char *errorMessage;
      PRUint32 errorFlag;

      if (mLoader->mCompatMode == eCompatibility_NavQuirks) {
        errorMessage = "MimeNotCssWarn";
        errorFlag = nsIScriptError::warningFlag;
      } else {
        
        aDataStream = nsnull;

        errorMessage = "MimeNotCss";
        errorFlag = nsIScriptError::errorFlag;
      }
      nsCOMPtr<nsIURI> referrer = GetReferrerURI();
      nsContentUtils::ReportToConsole(nsContentUtils::eCSS_PROPERTIES,
                                      errorMessage,
                                      strings, NS_ARRAY_LENGTH(strings),
                                      referrer, EmptyString(), 0, 0, errorFlag,
                                      "CSS Loader");
    }
  }
  
  if (!aDataStream) {
    LOG_WARN(("  No data stream; bailing"));
    mLoader->SheetComplete(this, NS_ERROR_NOT_AVAILABLE);
    return NS_OK;
  }    

  
  
  mSheet->SetURIs(channelURI, originalURI, channelURI);
  
  PRBool completed;
  return mLoader->ParseSheet(aDataStream, this, completed);
}

#ifdef MOZ_XUL
static PRBool IsChromeURI(nsIURI* aURI)
{
  NS_ASSERTION(aURI, "Have to pass in a URI");
  PRBool isChrome = PR_FALSE;
  aURI->SchemeIs("chrome", &isChrome);
  return isChrome;
}
#endif

PRBool
CSSLoaderImpl::IsAlternate(const nsAString& aTitle, PRBool aHasAlternateRel)
{
  
  
  
  
  
  if (aTitle.IsEmpty()) {
    return PR_FALSE;
  }
  
  if (!aHasAlternateRel && mDocument && mPreferredSheet.IsEmpty()) {
    
    
    mDocument->SetHeaderData(nsGkAtoms::headerDefaultStyle, aTitle);
    
    return PR_FALSE;
  }

  return !aTitle.Equals(mPreferredSheet);
}











nsresult
CSSLoaderImpl::CheckLoadAllowed(nsIPrincipal* aSourcePrincipal,
                                nsIURI* aTargetURI,
                                nsISupports* aContext)
{
  LOG(("CSSLoaderImpl::CheckLoadAllowed"));

  nsresult rv;
  
  if (aSourcePrincipal) {
    
    nsIScriptSecurityManager *secMan = nsContentUtils::GetSecurityManager();
    rv =
      secMan->CheckLoadURIWithPrincipal(aSourcePrincipal, aTargetURI,
                                        nsIScriptSecurityManager::ALLOW_CHROME);
    if (NS_FAILED(rv)) { 
      return rv;
    }

    LOG(("  Passed security check"));

    

    PRInt16 shouldLoad = nsIContentPolicy::ACCEPT;
    rv = NS_CheckContentLoadPolicy(nsIContentPolicy::TYPE_STYLESHEET,
                                   aTargetURI,
                                   aSourcePrincipal,
                                   aContext,
                                   NS_LITERAL_CSTRING("text/css"),
                                   nsnull,                     
                                   &shouldLoad,
                                   nsContentUtils::GetContentPolicy(),
                                   nsContentUtils::GetSecurityManager());

    if (NS_FAILED(rv) || NS_CP_REJECTED(shouldLoad)) {
      LOG(("  Load blocked by content policy"));
      return NS_ERROR_CONTENT_BLOCKED;
    }
  }

  return NS_OK;
}










nsresult
CSSLoaderImpl::CreateSheet(nsIURI* aURI,
                           nsIContent* aLinkingContent,
                           nsIPrincipal* aLoaderPrincipal,
                           PRBool aSyncLoad,
                           StyleSheetState& aSheetState,
                           nsICSSStyleSheet** aSheet)
{
  LOG(("CSSLoaderImpl::CreateSheet"));
  NS_PRECONDITION(aSheet, "Null out param!");

  NS_ENSURE_TRUE((mCompleteSheets.IsInitialized() || mCompleteSheets.Init()) &&
                   (mLoadingDatas.IsInitialized() || mLoadingDatas.Init()) &&
                   (mPendingDatas.IsInitialized() || mPendingDatas.Init()),
                 NS_ERROR_OUT_OF_MEMORY);
  
  nsresult rv = NS_OK;
  *aSheet = nsnull;
  aSheetState = eSheetStateUnknown;
  
  if (aURI) {
    aSheetState = eSheetComplete;
    nsCOMPtr<nsICSSStyleSheet> sheet;

    
#ifdef MOZ_XUL
    if (IsChromeURI(aURI)) {
      nsXULPrototypeCache* cache = nsXULPrototypeCache::GetInstance();
      if (cache) {
        if (cache->IsEnabled()) {
          sheet = cache->GetStyleSheet(aURI);
          LOG(("  From XUL cache: %p", sheet.get()));
        }
      }
    }
#endif

    if (!sheet) {
      
      nsURIAndPrincipalHashKey key(aURI, aLoaderPrincipal);
      
      mCompleteSheets.Get(&key, getter_AddRefs(sheet));
      LOG(("  From completed: %p", sheet.get()));
    }
    
    if (sheet) {
#ifdef DEBUG
      
      
      PRBool complete = PR_FALSE;
      sheet->GetComplete(complete);
      NS_ASSERTION(complete,
                   "Sheet thinks it's not complete while we think it is");
#endif
      
      PRBool modified = PR_TRUE;
      sheet->IsModified(&modified);
      if (modified) {
        LOG(("  Not cloning completed sheet %p because it's been modified",
             sheet.get()));
        sheet = nsnull;
      }
    }

    
    if (!sheet && !aSyncLoad) {
      aSheetState = eSheetLoading;
      SheetLoadData* loadData = nsnull;
      nsURIAndPrincipalHashKey key(aURI, aLoaderPrincipal);      
      mLoadingDatas.Get(&key, &loadData);
      if (loadData) {
        sheet = loadData->mSheet;
        LOG(("  From loading: %p", sheet.get()));

#ifdef DEBUG
        PRBool debugEqual;
        NS_ASSERTION((!aLoaderPrincipal && !loadData->mLoaderPrincipal) ||
                     (aLoaderPrincipal && loadData->mLoaderPrincipal &&
                      NS_SUCCEEDED(aLoaderPrincipal->
                                   Equals(loadData->mLoaderPrincipal,
                                          &debugEqual)) && debugEqual),
                     "Principals should be the same");
#endif
      }

      
      if (!sheet) {
        aSheetState = eSheetPending;
        SheetLoadData* loadData = nsnull;
        mPendingDatas.Get(&key, &loadData);
        if (loadData) {
          sheet = loadData->mSheet;
          LOG(("  From pending: %p", sheet.get()));

#ifdef DEBUG
          PRBool debugEqual;
          NS_ASSERTION((!aLoaderPrincipal && !loadData->mLoaderPrincipal) ||
                       (aLoaderPrincipal && loadData->mLoaderPrincipal &&
                        NS_SUCCEEDED(aLoaderPrincipal->
                                     Equals(loadData->mLoaderPrincipal,
                                            &debugEqual)) && debugEqual),
                       "Principals should be the same");
#endif
        }
      }
    }

    if (sheet) {
      
#ifdef DEBUG
      PRBool modified = PR_TRUE;
      sheet->IsModified(&modified);
      PRBool complete = PR_FALSE;
      sheet->GetComplete(complete);
      NS_ASSERTION(!modified || !complete,
                   "Unexpected modified complete sheet");
      NS_ASSERTION(complete || aSheetState != eSheetComplete,
                   "Sheet thinks it's not complete while we think it is");
#endif
      rv = sheet->Clone(nsnull, nsnull, nsnull, nsnull, aSheet);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  if (!*aSheet) {
    aSheetState = eSheetNeedsParser;
    nsIURI *sheetURI;
    nsCOMPtr<nsIURI> baseURI;
    nsIURI* originalURI;
    if (!aURI) {
      
      
      NS_ASSERTION(aLinkingContent, "Inline stylesheet without linking content?");
      baseURI = aLinkingContent->GetBaseURI();
      sheetURI = aLinkingContent->GetDocument()->GetDocumentURI();
      originalURI = nsnull;
    } else {
      baseURI = aURI;
      sheetURI = aURI;
      originalURI = aURI;
    }

    rv = NS_NewCSSStyleSheet(aSheet);
    NS_ENSURE_SUCCESS(rv, rv);
    (*aSheet)->SetURIs(sheetURI, originalURI, baseURI);
  }

  NS_ASSERTION(*aSheet, "We should have a sheet by now!");
  NS_ASSERTION(aSheetState != eSheetStateUnknown, "Have to set a state!");
  LOG(("  State: %s", gStateStrings[aSheetState]));
  
  return NS_OK;
}






nsresult
CSSLoaderImpl::PrepareSheet(nsICSSStyleSheet* aSheet,
                            const nsSubstring& aTitle,
                            const nsSubstring& aMediaString,
                            nsMediaList* aMediaList,
                            PRBool aHasAlternateRel,
                            PRBool *aIsAlternate)
{
  NS_PRECONDITION(aSheet, "Must have a sheet!");

  nsresult rv;
  nsCOMPtr<nsMediaList> mediaList(aMediaList);

  if (!aMediaString.IsEmpty()) {
    NS_ASSERTION(!aMediaList,
                 "must not provide both aMediaString and aMediaList");
    mediaList = new nsMediaList();
    NS_ENSURE_TRUE(mediaList, NS_ERROR_OUT_OF_MEMORY);
    nsCOMPtr<nsICSSParser> mediumParser;
    nsresult rv = GetParserFor(nsnull, getter_AddRefs(mediumParser));
    NS_ENSURE_SUCCESS(rv, rv);
    
    
    rv = mediumParser->ParseMediaList(aMediaString, nsnull, 0, mediaList,
                                      PR_TRUE);
    NS_ENSURE_SUCCESS(rv, rv);
    RecycleParser(mediumParser);
  }

  rv = aSheet->SetMedia(mediaList);
  NS_ENSURE_SUCCESS(rv, rv);

  aSheet->SetTitle(aTitle);
  PRBool alternate = IsAlternate(aTitle, aHasAlternateRel);
  aSheet->SetEnabled(! alternate);
  if (aIsAlternate) {
    *aIsAlternate = alternate;
  }
  return NS_OK;    
}














nsresult
CSSLoaderImpl::InsertSheetInDoc(nsICSSStyleSheet* aSheet,
                                nsIContent* aLinkingContent,
                                nsIDocument* aDocument)
{
  LOG(("CSSLoaderImpl::InsertSheetInDoc"));
  NS_PRECONDITION(aSheet, "Nothing to insert");
  NS_PRECONDITION(aDocument, "Must have a document to insert into");

  

  PRInt32 sheetCount = aDocument->GetNumberOfStyleSheets();

  







  PRInt32 insertionPoint;
  for (insertionPoint = sheetCount - 1; insertionPoint >= 0; --insertionPoint) {
    nsIStyleSheet *curSheet = aDocument->GetStyleSheetAt(insertionPoint);
    NS_ASSERTION(curSheet, "There must be a sheet here!");
    nsCOMPtr<nsIDOMStyleSheet> domSheet = do_QueryInterface(curSheet);
    NS_ASSERTION(domSheet, "All the \"normal\" sheets implement nsIDOMStyleSheet");
    nsCOMPtr<nsIDOMNode> sheetOwner;
    domSheet->GetOwnerNode(getter_AddRefs(sheetOwner));
    if (sheetOwner && !aLinkingContent) {
      
      
      continue;
    }

    if (!sheetOwner) {
      
      
      break;
    }

    nsCOMPtr<nsINode> sheetOwnerNode = do_QueryInterface(sheetOwner);
    NS_ASSERTION(aLinkingContent != sheetOwnerNode,
                 "Why do we still have our old sheet?");

    
    if (nsContentUtils::PositionIsBefore(sheetOwnerNode, aLinkingContent)) {
      
      
      break;
    }
  }

  ++insertionPoint; 
  
  
  
  nsCOMPtr<nsIStyleSheetLinkingElement>
    linkingElement = do_QueryInterface(aLinkingContent);
  if (linkingElement) {
    linkingElement->SetStyleSheet(aSheet); 
  }

  aDocument->BeginUpdate(UPDATE_STYLE);
  aDocument->InsertStyleSheetAt(aSheet, insertionPoint);
  aDocument->EndUpdate(UPDATE_STYLE);
  LOG(("  Inserting into document at position %d", insertionPoint));

  return NS_OK;
}










nsresult
CSSLoaderImpl::InsertChildSheet(nsICSSStyleSheet* aSheet,
                                nsICSSStyleSheet* aParentSheet,
                                nsICSSImportRule* aParentRule)
{
  LOG(("CSSLoaderImpl::InsertChildSheet"));
  NS_PRECONDITION(aSheet, "Nothing to insert");
  NS_PRECONDITION(aParentSheet, "Need a parent to insert into");
  NS_PRECONDITION(aParentSheet, "How did we get imported?");

  
  
  aSheet->SetEnabled(PR_TRUE);
  
  aParentSheet->AppendStyleSheet(aSheet);
  aParentRule->SetSheet(aSheet); 

  LOG(("  Inserting into parent sheet"));
  

  return NS_OK;
}









nsresult
CSSLoaderImpl::LoadSheet(SheetLoadData* aLoadData, StyleSheetState aSheetState)
{
  LOG(("CSSLoaderImpl::LoadSheet"));
  NS_PRECONDITION(aLoadData, "Need a load data");
  NS_PRECONDITION(aLoadData->mURI, "Need a URI to load");
  NS_PRECONDITION(aLoadData->mSheet, "Need a sheet to load into");
  NS_PRECONDITION(aSheetState != eSheetComplete, "Why bother?");
  NS_PRECONDITION(!aLoadData->mUseSystemPrincipal || aLoadData->mSyncLoad,
                  "Shouldn't use system principal for async loads");
  NS_ASSERTION(mLoadingDatas.IsInitialized(), "mLoadingDatas should be initialized by now.");

  LOG_URI("  Load from: '%s'", aLoadData->mURI);
  
  nsresult rv = NS_OK;  

  if (!mDocument && !aLoadData->mIsNonDocumentSheet) {
    
    LOG_WARN(("  No document and not non-document sheet; pre-dropping load"));
    SheetComplete(aLoadData, NS_BINDING_ABORTED);
    return NS_BINDING_ABORTED;
  }

  if (aLoadData->mSyncLoad) {
    LOG(("  Synchronous load"));
    NS_ASSERTION(!aLoadData->mObserver, "Observer for a sync load?");
    NS_ASSERTION(aSheetState == eSheetNeedsParser,
                 "Sync loads can't reuse existing async loads");

    
    nsCOMPtr<nsIInputStream> stream;
    nsCOMPtr<nsIChannel> channel;
    rv = NS_OpenURI(getter_AddRefs(stream), aLoadData->mURI, nsnull,
                    nsnull, nsnull, nsIRequest::LOAD_NORMAL,
                    getter_AddRefs(channel));
    if (NS_FAILED(rv)) {
      LOG_ERROR(("  Failed to open URI synchronously"));
      SheetComplete(aLoadData, rv);
      return rv;
    }

    NS_ASSERTION(channel, "NS_OpenURI lied?");
    
    
    nsCOMPtr<nsIPrincipal> principal;
    if (aLoadData->mUseSystemPrincipal) {
      rv = nsContentUtils::GetSecurityManager()->
        GetSystemPrincipal(getter_AddRefs(principal));
    } else {
      rv = nsContentUtils::GetSecurityManager()->
        GetChannelPrincipal(channel, getter_AddRefs(principal));
    }

    if (NS_FAILED(rv)) {
      LOG_ERROR(("  Failed to get a principal for the sheet"));
      SheetComplete(aLoadData, rv);
      return rv;
    }

    aLoadData->mSheet->SetPrincipal(principal);

    nsCOMPtr<nsIConverterInputStream> converterStream = 
      do_CreateInstance("@mozilla.org/intl/converter-input-stream;1", &rv);
    
    if (NS_FAILED(rv)) {
      LOG_ERROR(("  Failed to create converter stream"));
      SheetComplete(aLoadData, rv);
      return rv;
    }

    
    

    
    
    rv = converterStream->Init(stream, "UTF-8",
                               8192,
                               nsIConverterInputStream::
                                    DEFAULT_REPLACEMENT_CHARACTER);
    
    if (NS_FAILED(rv)) {
      LOG_ERROR(("  Failed to initialize converter stream"));
      SheetComplete(aLoadData, rv);
      return rv;
    }

    PRBool completed;
    rv = ParseSheet(converterStream, aLoadData, completed);
    NS_ASSERTION(completed, "sync load did not complete");
    return rv;
  }

  SheetLoadData* existingData = nsnull;

  nsURIAndPrincipalHashKey key(aLoadData->mURI, aLoadData->mLoaderPrincipal);
  if (aSheetState == eSheetLoading) {
    mLoadingDatas.Get(&key, &existingData);
    NS_ASSERTION(existingData, "CreateSheet lied about the state");
  }
  else if (aSheetState == eSheetPending){
    mPendingDatas.Get(&key, &existingData);
    NS_ASSERTION(existingData, "CreateSheet lied about the state");
  }
  
  if (existingData) {
    LOG(("  Glomming on to existing load"));
    SheetLoadData* data = existingData;
    while (data->mNext) {
      data = data->mNext;
    }
    data->mNext = aLoadData; 
    if (aSheetState == eSheetPending && !aLoadData->mWasAlternate) {
      

#ifdef DEBUG
      SheetLoadData* removedData;
      NS_ASSERTION(mPendingDatas.Get(&key, &removedData) &&
                   removedData == existingData,
                   "Bad pending table.");
#endif

      mPendingDatas.Remove(&key);

      LOG(("  Forcing load of pending data"));
      return LoadSheet(existingData, eSheetNeedsParser);
    }
    
    
    return NS_OK;
  }

#ifdef DEBUG
  mSyncCallback = PR_TRUE;
#endif
  nsCOMPtr<nsILoadGroup> loadGroup;
  if (mDocument) {
    loadGroup = mDocument->GetDocumentLoadGroup();
    NS_ASSERTION(loadGroup,
                 "No loadgroup for stylesheet; onload will fire early");
  }

#ifdef MOZ_TIMELINE
  NS_TIMELINE_MARK_URI("Loading style sheet: %s", aLoadData->mURI);
  NS_TIMELINE_INDENT();
#endif
  
  nsCOMPtr<nsIChannel> channel;
  rv = NS_NewChannel(getter_AddRefs(channel),
                     aLoadData->mURI, nsnull, loadGroup,
                     nsnull, nsIChannel::LOAD_NORMAL);
  
  if (NS_FAILED(rv)) {
#ifdef DEBUG
    mSyncCallback = PR_FALSE;
#endif
    LOG_ERROR(("  Failed to create channel"));
    SheetComplete(aLoadData, rv);
    return rv;
  }

  nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(channel));
  if (httpChannel) {
    
    httpChannel->SetRequestHeader(NS_LITERAL_CSTRING("Accept"),
                                  NS_LITERAL_CSTRING("text/css,*/*;q=0.1"),
                                  PR_FALSE);
    nsCOMPtr<nsIURI> referrerURI = aLoadData->GetReferrerURI();
    if (referrerURI)
      httpChannel->SetReferrer(referrerURI);
  }

  
  
  channel->SetContentType(NS_LITERAL_CSTRING("text/css"));

  if (aLoadData->mLoaderPrincipal) {
    PRBool inherit;
    rv = NS_URIChainHasFlags(aLoadData->mURI,
                             nsIProtocolHandler::URI_INHERITS_SECURITY_CONTEXT,
                             &inherit);
    if ((NS_SUCCEEDED(rv) && inherit) ||
        (nsContentUtils::URIIsLocalFile(aLoadData->mURI) &&
         NS_SUCCEEDED(aLoadData->mLoaderPrincipal->
                      CheckMayLoad(aLoadData->mURI, PR_FALSE)))) {
      channel->SetOwner(aLoadData->mLoaderPrincipal);
    }
  }

  
  
  
  nsCOMPtr<nsIUnicharStreamLoader> streamLoader;
  rv = NS_NewUnicharStreamLoader(getter_AddRefs(streamLoader), aLoadData);

  if (NS_SUCCEEDED(rv))
    rv = channel->AsyncOpen(streamLoader, nsnull);

#ifdef DEBUG
  mSyncCallback = PR_FALSE;
#endif

  if (NS_FAILED(rv)) {
    LOG_ERROR(("  Failed to create stream loader"));
    SheetComplete(aLoadData, rv);
    return rv;
  }

  
  nsCOMPtr<nsIChannelClassifier> classifier =
    do_CreateInstance(NS_CHANNELCLASSIFIER_CONTRACTID);
  if (classifier) {
    rv = classifier->Start(channel, PR_TRUE);
    if (NS_FAILED(rv)) {
      LOG_ERROR(("  Failed to classify URI"));
      aLoadData->mIsCancelled = PR_TRUE;
      channel->Cancel(rv);
      SheetComplete(aLoadData, rv);
      return rv;
    }
  }

  if (!mLoadingDatas.Put(&key, aLoadData)) {
    LOG_ERROR(("  Failed to put data in loading table"));
    aLoadData->mIsCancelled = PR_TRUE;
    channel->Cancel(NS_ERROR_OUT_OF_MEMORY);
    SheetComplete(aLoadData, NS_ERROR_OUT_OF_MEMORY);
    return NS_ERROR_OUT_OF_MEMORY;
  }
  
  aLoadData->mIsLoading = PR_TRUE;
  
  return NS_OK;
}







nsresult
CSSLoaderImpl::ParseSheet(nsIUnicharInputStream* aStream,
                          SheetLoadData* aLoadData,
                          PRBool& aCompleted)
{
  LOG(("CSSLoaderImpl::ParseSheet"));
  NS_PRECONDITION(aStream, "Must have data to parse");
  NS_PRECONDITION(aLoadData, "Must have load data");
  NS_PRECONDITION(aLoadData->mSheet, "Must have sheet to parse into");

  aCompleted = PR_FALSE;

  nsCOMPtr<nsICSSParser> parser;
  nsresult rv = GetParserFor(aLoadData->mSheet, getter_AddRefs(parser));
  if (NS_FAILED(rv)) {
    LOG_ERROR(("  Failed to get CSS parser"));
    SheetComplete(aLoadData, rv);
    return rv;
  }

  
  mParsingDatas.AppendElement(aLoadData);
  nsCOMPtr<nsIURI> sheetURI, baseURI;
  aLoadData->mSheet->GetSheetURI(getter_AddRefs(sheetURI));
  aLoadData->mSheet->GetBaseURI(getter_AddRefs(baseURI));
  rv = parser->Parse(aStream, sheetURI, baseURI,
                     aLoadData->mSheet->Principal(), aLoadData->mLineNumber,
                     aLoadData->mAllowUnsafeRules);
  mParsingDatas.RemoveElementAt(mParsingDatas.Length() - 1);
  RecycleParser(parser);

  NS_ASSERTION(aLoadData->mPendingChildren == 0 || !aLoadData->mSyncLoad,
               "Sync load has leftover pending children!");
  
  if (aLoadData->mPendingChildren == 0) {
    LOG(("  No pending kids from parse"));
    aCompleted = PR_TRUE;
    SheetComplete(aLoadData, NS_OK);
  }
  
  
  
  return NS_OK;
}









void
CSSLoaderImpl::SheetComplete(SheetLoadData* aLoadData, nsresult aStatus)
{
  LOG(("CSSLoaderImpl::SheetComplete"));

  
  
  
  nsAutoTArray<nsRefPtr<SheetLoadData>, 8> datasToNotify;
  DoSheetComplete(aLoadData, aStatus, datasToNotify);

  
  PRUint32 count = datasToNotify.Length();
  mDatasToNotifyOn += count;
  for (PRUint32 i = 0; i < count; ++i) {
    --mDatasToNotifyOn;
    
    SheetLoadData* data = datasToNotify[i];
    NS_ASSERTION(data && data->mMustNotify, "How did this data get here?");
    if (data->mObserver) {
      LOG(("  Notifying observer 0x%x for data 0x%x.  wasAlternate: %d",
           data->mObserver.get(), data, data->mWasAlternate));
      data->mObserver->StyleSheetLoaded(data->mSheet, data->mWasAlternate,
                                        aStatus);
    }

    nsTObserverArray<nsCOMPtr<nsICSSLoaderObserver> >::ForwardIterator iter(mObservers);
    nsCOMPtr<nsICSSLoaderObserver> obs;
    while (iter.HasMore()) {
      obs = iter.GetNext();
      LOG(("  Notifying global observer 0x%x for data 0x%s.  wasAlternate: %d",
           obs.get(), data, data->mWasAlternate));
      obs->StyleSheetLoaded(data->mSheet, data->mWasAlternate, aStatus);
    }
  }

  if (mLoadingDatas.Count() == 0 && mPendingDatas.Count() > 0) {
    LOG(("  No more loading sheets; starting alternates"));
    StartAlternateLoads();
  }
}

void
CSSLoaderImpl::DoSheetComplete(SheetLoadData* aLoadData, nsresult aStatus,
                               LoadDataArray& aDatasToNotify)
{
  LOG(("CSSLoaderImpl::DoSheetComplete"));
  NS_PRECONDITION(aLoadData, "Must have a load data!");
  NS_PRECONDITION(aLoadData->mSheet, "Must have a sheet");
  NS_ASSERTION(mLoadingDatas.IsInitialized(),"mLoadingDatas should be initialized by now.");

  LOG(("Load completed, status: 0x%x", aStatus));

  
  if (aLoadData->mURI) {
    LOG_URI("  Finished loading: '%s'", aLoadData->mURI);
    
    if (aLoadData->mIsLoading) {
      nsURIAndPrincipalHashKey key(aLoadData->mURI,
                                   aLoadData->mLoaderPrincipal);
#ifdef DEBUG
      SheetLoadData *loadingData;
      NS_ASSERTION(mLoadingDatas.Get(&key, &loadingData) &&
                   loadingData == aLoadData,
                   "Bad loading table");
#endif

      mLoadingDatas.Remove(&key);
      aLoadData->mIsLoading = PR_FALSE;
    }
  }
  
  
  SheetLoadData* data = aLoadData;
  while (data) {

    data->mSheet->SetModified(PR_FALSE); 
    data->mSheet->SetComplete();
    if (data->mMustNotify && (data->mObserver || !mObservers.IsEmpty())) {
      
      
      aDatasToNotify.AppendElement(data);

      
      
    }

    NS_ASSERTION(!data->mParentData ||
                 data->mParentData->mPendingChildren != 0,
                 "Broken pending child count on our parent");

    
    
    
    
    
    if (data->mParentData &&
        --(data->mParentData->mPendingChildren) == 0 &&
        !mParsingDatas.Contains(data->mParentData)) {
      DoSheetComplete(data->mParentData, aStatus, aDatasToNotify);
    }
    
    data = data->mNext;
  }

  
  if (NS_SUCCEEDED(aStatus) && aLoadData->mURI) {
#ifdef MOZ_XUL
    if (IsChromeURI(aLoadData->mURI)) {
      nsXULPrototypeCache* cache = nsXULPrototypeCache::GetInstance();
      if (cache && cache->IsEnabled()) {
        if (!cache->GetStyleSheet(aLoadData->mURI)) {
          LOG(("  Putting sheet in XUL prototype cache"));
          cache->PutStyleSheet(aLoadData->mSheet);
        }
      }
    }
    else {
#endif
      nsURIAndPrincipalHashKey key(aLoadData->mURI,
                                   aLoadData->mLoaderPrincipal);
      mCompleteSheets.Put(&key, aLoadData->mSheet);
#ifdef MOZ_XUL
    }
#endif
  }

  NS_RELEASE(aLoadData);  
}

NS_IMETHODIMP
CSSLoaderImpl::LoadInlineStyle(nsIContent* aElement,
                               nsIUnicharInputStream* aStream, 
                               PRUint32 aLineNumber,
                               const nsSubstring& aTitle,
                               const nsSubstring& aMedia,
                               nsICSSLoaderObserver* aObserver,
                               PRBool* aCompleted,
                               PRBool* aIsAlternate)
{
  LOG(("CSSLoaderImpl::LoadInlineStyle"));
  NS_PRECONDITION(aStream, "Must have a stream to parse!");
  NS_ASSERTION(mParsingDatas.Length() == 0, "We're in the middle of a parse?");

  *aCompleted = PR_TRUE;

  if (!mEnabled) {
    LOG_WARN(("  Not enabled"));
    return NS_ERROR_NOT_AVAILABLE;
  }

  NS_ENSURE_TRUE(mDocument, NS_ERROR_NOT_INITIALIZED);

  nsCOMPtr<nsIStyleSheetLinkingElement> owningElement(do_QueryInterface(aElement));
  NS_ASSERTION(owningElement, "Element is not a style linking element!");
  

  
  
  StyleSheetState state;
  nsCOMPtr<nsICSSStyleSheet> sheet;
  nsresult rv = CreateSheet(nsnull, aElement, nsnull, PR_FALSE, state,
                            getter_AddRefs(sheet));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ASSERTION(state == eSheetNeedsParser,
               "Inline sheets should not be cached");

  rv = PrepareSheet(sheet, aTitle, aMedia, nsnull, PR_FALSE,
                    aIsAlternate);
  NS_ENSURE_SUCCESS(rv, rv);
  
  LOG(("  Sheet is alternate: %d", *aIsAlternate));
  
  rv = InsertSheetInDoc(sheet, aElement, mDocument);
  NS_ENSURE_SUCCESS(rv, rv);
  
  SheetLoadData* data = new SheetLoadData(this, aTitle, nsnull, sheet,
                                          owningElement, *aIsAlternate,
                                          aObserver, nsnull);

  if (!data) {
    sheet->SetComplete();
    return NS_ERROR_OUT_OF_MEMORY;
  }

  
  sheet->SetPrincipal(aElement->NodePrincipal());

  NS_ADDREF(data);
  data->mLineNumber = aLineNumber;
  
  rv = ParseSheet(aStream, data, *aCompleted);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (!*aCompleted) {
    data->mMustNotify = PR_TRUE;
  }
  return rv;
}        

NS_IMETHODIMP
CSSLoaderImpl::LoadStyleLink(nsIContent* aElement,
                             nsIURI* aURL, 
                             const nsSubstring& aTitle,
                             const nsSubstring& aMedia,
                             PRBool aHasAlternateRel,
                             nsICSSLoaderObserver* aObserver,
                             PRBool* aIsAlternate)
{
  LOG(("CSSLoaderImpl::LoadStyleLink"));
  NS_PRECONDITION(aURL, "Must have URL to load");
  NS_ASSERTION(mParsingDatas.Length() == 0, "We're in the middle of a parse?");

  LOG_URI("  Link uri: '%s'", aURL);
  LOG(("  Link title: '%s'", NS_ConvertUTF16toUTF8(aTitle).get()));
  LOG(("  Link media: '%s'", NS_ConvertUTF16toUTF8(aMedia).get()));
  LOG(("  Link alternate rel: %d", aHasAlternateRel));

  if (!mEnabled) {
    LOG_WARN(("  Not enabled"));
    return NS_ERROR_NOT_AVAILABLE;
  }
  
  NS_ENSURE_TRUE(mDocument, NS_ERROR_NOT_INITIALIZED);

  nsIPrincipal* principal =
    aElement ? aElement->NodePrincipal() : mDocument->NodePrincipal();

  nsISupports* context = aElement;
  if (!context) {
    context = mDocument;
  }
  nsresult rv = CheckLoadAllowed(principal, aURL, context);
  if (NS_FAILED(rv)) return rv;

  LOG(("  Passed load check"));
  
  StyleSheetState state;
  nsCOMPtr<nsICSSStyleSheet> sheet;
  rv = CreateSheet(aURL, aElement, principal, PR_FALSE, state,
                   getter_AddRefs(sheet));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = PrepareSheet(sheet, aTitle, aMedia, nsnull, aHasAlternateRel,
                    aIsAlternate);
  NS_ENSURE_SUCCESS(rv, rv);

  LOG(("  Sheet is alternate: %d", *aIsAlternate));
  
  rv = InsertSheetInDoc(sheet, aElement, mDocument);
  NS_ENSURE_SUCCESS(rv, rv);

  if (state == eSheetComplete) {
    LOG(("  Sheet already complete: 0x%p",
         static_cast<void*>(sheet.get())));
    if (aObserver) {
      rv = PostLoadEvent(aURL, sheet, aObserver, *aIsAlternate);
      return rv;
    }

    return NS_OK;
  }

  nsCOMPtr<nsIStyleSheetLinkingElement> owningElement(do_QueryInterface(aElement));

  
  SheetLoadData* data = new SheetLoadData(this, aTitle, aURL, sheet,
                                          owningElement, *aIsAlternate,
                                          aObserver, principal);
  if (!data) {
    sheet->SetComplete();
    return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_ADDREF(data);

  
  if (aURL && state == eSheetNeedsParser && mLoadingDatas.Count() != 0 &&
      *aIsAlternate) {
    LOG(("  Deferring alternate sheet load"));
    nsURIAndPrincipalHashKey key(data->mURI, data->mLoaderPrincipal);
    if (!mPendingDatas.Put(&key, data)) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    data->mMustNotify = PR_TRUE;
    return NS_OK;
  }

  
  rv = LoadSheet(data, state);
  NS_ENSURE_SUCCESS(rv, rv);

  data->mMustNotify = PR_TRUE;
  return rv;
}

NS_IMETHODIMP
CSSLoaderImpl::LoadChildSheet(nsICSSStyleSheet* aParentSheet,
                              nsIURI* aURL, 
                              nsMediaList* aMedia,
                              nsICSSImportRule* aParentRule)
{
  LOG(("CSSLoaderImpl::LoadChildSheet"));
  NS_PRECONDITION(aURL, "Must have a URI to load");
  NS_PRECONDITION(aParentSheet, "Must have a parent sheet");

  if (!mEnabled) {
    LOG_WARN(("  Not enabled"));
    return NS_ERROR_NOT_AVAILABLE;
  }
  
  LOG_URI("  Child uri: '%s'", aURL);

  nsCOMPtr<nsIDOMNode> owningNode;

  
  
  nsCOMPtr<nsIDocument> owningDoc;
  nsresult rv = aParentSheet->GetOwningDocument(*getter_AddRefs(owningDoc));
  if (NS_SUCCEEDED(rv) && owningDoc) {
    nsCOMPtr<nsIDOMStyleSheet> nextParentSheet(do_QueryInterface(aParentSheet));
    NS_ENSURE_TRUE(nextParentSheet, NS_ERROR_FAILURE); 

    nsCOMPtr<nsIDOMStyleSheet> topSheet;
    
    do {
      topSheet.swap(nextParentSheet);
      topSheet->GetParentStyleSheet(getter_AddRefs(nextParentSheet));
    } while (nextParentSheet);

    topSheet->GetOwnerNode(getter_AddRefs(owningNode));
  }

  nsISupports* context = owningNode;
  if (!context) {
    context = mDocument;
  }

  nsIPrincipal* principal = aParentSheet->Principal();
  rv = CheckLoadAllowed(principal, aURL, context);
  if (NS_FAILED(rv)) return rv;

  LOG(("  Passed load check"));
  
  SheetLoadData* parentData = nsnull;
  nsCOMPtr<nsICSSLoaderObserver> observer;

  PRInt32 count = mParsingDatas.Length();
  if (count > 0) {
    LOG(("  Have a parent load"));
    parentData = mParsingDatas.ElementAt(count - 1);
    
    SheetLoadData* data = parentData;
    while (data && data->mURI) {
      PRBool equal;
      if (NS_SUCCEEDED(data->mURI->Equals(aURL, &equal)) && equal) {
        
        
        LOG_ERROR(("  @import cycle detected, dropping load"));
        return NS_OK;
      }
      data = data->mParentData;
    }

    NS_ASSERTION(parentData->mSheet == aParentSheet,
                 "Unexpected call to LoadChildSheet");
  } else {
    LOG(("  No parent load; must be CSSOM"));
    
    
    observer = do_QueryInterface(aParentSheet);
  }

  
  
  nsCOMPtr<nsICSSStyleSheet> sheet;
  StyleSheetState state;
  rv = CreateSheet(aURL, nsnull, principal,
                   parentData ? parentData->mSyncLoad : PR_FALSE,
                   state, getter_AddRefs(sheet));
  NS_ENSURE_SUCCESS(rv, rv);

  const nsSubstring& empty = EmptyString();
  rv = PrepareSheet(sheet, empty, empty, aMedia);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = InsertChildSheet(sheet, aParentSheet, aParentRule);
  NS_ENSURE_SUCCESS(rv, rv);
  
  if (state == eSheetComplete) {
    LOG(("  Sheet already complete"));
    
    
    
    return NS_OK;
  }

  
  SheetLoadData* data = new SheetLoadData(this, aURL, sheet, parentData,
                                          observer, principal);

  if (!data) {
    sheet->SetComplete();
    return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_ADDREF(data);
  PRBool syncLoad = data->mSyncLoad;

  
  rv = LoadSheet(data, state);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (!syncLoad) {
    data->mMustNotify = PR_TRUE;
  }
  return rv;  
}

NS_IMETHODIMP
CSSLoaderImpl::LoadSheetSync(nsIURI* aURL, PRBool aAllowUnsafeRules,
                             PRBool aUseSystemPrincipal,
                             nsICSSStyleSheet** aSheet)
{
  LOG(("CSSLoaderImpl::LoadSheetSync"));
  return InternalLoadNonDocumentSheet(aURL, aAllowUnsafeRules,
                                      aUseSystemPrincipal, nsnull,
                                      EmptyCString(), aSheet, nsnull);
}

NS_IMETHODIMP
CSSLoaderImpl::LoadSheet(nsIURI* aURL,
                         nsIPrincipal* aOriginPrincipal,
                         const nsCString& aCharset,
                         nsICSSLoaderObserver* aObserver,
                         nsICSSStyleSheet** aSheet)
{
  LOG(("CSSLoaderImpl::LoadSheet(aURL, aObserver, aSheet) api call"));
  NS_PRECONDITION(aSheet, "aSheet is null");
  return InternalLoadNonDocumentSheet(aURL, PR_FALSE, PR_FALSE,
                                      aOriginPrincipal, aCharset,
                                      aSheet, aObserver);
}

NS_IMETHODIMP
CSSLoaderImpl::LoadSheet(nsIURI* aURL,
                         nsIPrincipal* aOriginPrincipal,
                         const nsCString& aCharset,
                         nsICSSLoaderObserver* aObserver)
{
  LOG(("CSSLoaderImpl::LoadSheet(aURL, aObserver) api call"));
  return InternalLoadNonDocumentSheet(aURL, PR_FALSE, PR_FALSE,
                                      aOriginPrincipal, aCharset,
                                      nsnull, aObserver);
}

nsresult
CSSLoaderImpl::InternalLoadNonDocumentSheet(nsIURI* aURL, 
                                            PRBool aAllowUnsafeRules,
                                            PRBool aUseSystemPrincipal,
                                            nsIPrincipal* aOriginPrincipal,
                                            const nsCString& aCharset,
                                            nsICSSStyleSheet** aSheet,
                                            nsICSSLoaderObserver* aObserver)
{
  NS_PRECONDITION(aURL, "Must have a URI to load");
  NS_PRECONDITION(aSheet || aObserver, "Sheet and observer can't both be null");
  NS_PRECONDITION(!aUseSystemPrincipal || !aObserver,
                  "Shouldn't load system-principal sheets async");
  NS_ASSERTION(mParsingDatas.Length() == 0, "We're in the middle of a parse?");

  LOG_URI("  Non-document sheet uri: '%s'", aURL);
  
  if (aSheet) {
    *aSheet = nsnull;
  }
  
  if (!mEnabled) {
    LOG_WARN(("  Not enabled"));
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsresult rv = CheckLoadAllowed(aOriginPrincipal, aURL, mDocument);
  if (NS_FAILED(rv)) {
    return rv;
  }

  StyleSheetState state;
  nsCOMPtr<nsICSSStyleSheet> sheet;
  PRBool syncLoad = (aObserver == nsnull);
  
  rv = CreateSheet(aURL, nsnull, aOriginPrincipal, syncLoad, state,
                   getter_AddRefs(sheet));
  NS_ENSURE_SUCCESS(rv, rv);

  const nsSubstring& empty = EmptyString();
  rv = PrepareSheet(sheet, empty, empty, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);
  
  if (state == eSheetComplete) {
    LOG(("  Sheet already complete"));
    if (aObserver) {
      rv = PostLoadEvent(aURL, sheet, aObserver, PR_FALSE);
    }
    if (aSheet) {
      sheet.swap(*aSheet);
    }
    return rv;
  }

  SheetLoadData* data =
    new SheetLoadData(this, aURL, sheet, syncLoad, aAllowUnsafeRules,
                      aUseSystemPrincipal, aCharset, aObserver,
                      aOriginPrincipal);

  if (!data) {
    sheet->SetComplete();
    return NS_ERROR_OUT_OF_MEMORY;
  }
  
  NS_ADDREF(data);
  rv = LoadSheet(data, state);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aSheet) {
    sheet.swap(*aSheet);
  }
  if (aObserver) {
    data->mMustNotify = PR_TRUE;
  }

  return rv;
}

nsresult
CSSLoaderImpl::PostLoadEvent(nsIURI* aURI,
                             nsICSSStyleSheet* aSheet,
                             nsICSSLoaderObserver* aObserver,
                             PRBool aWasAlternate)
{
  LOG(("nsCSSLoader::PostLoadEvent"));
  NS_PRECONDITION(aSheet, "Must have sheet");
  NS_PRECONDITION(aObserver, "Must have observer");

  nsRefPtr<SheetLoadData> evt =
    new SheetLoadData(this, EmptyString(), 
                      aURI,
                      aSheet,
                      nsnull,  
                      aWasAlternate,
                      aObserver,
                      nsnull);
  NS_ENSURE_TRUE(evt, NS_ERROR_OUT_OF_MEMORY);

  if (!mPostedEvents.AppendElement(evt)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  nsresult rv = NS_DispatchToCurrentThread(evt);
  if (NS_FAILED(rv)) {
    NS_WARNING("failed to dispatch stylesheet load event");
    mPostedEvents.RemoveElement(evt);
  } else {
    
    if (mDocument) {
      mDocument->BlockOnload();
    }

    
    evt->mMustNotify = PR_TRUE;
  }

  return rv;
}

void
CSSLoaderImpl::HandleLoadEvent(SheetLoadData* aEvent)
{
  
  
  
  NS_ASSERTION(aEvent->mSheet, "Must have sheet");
  if (!aEvent->mIsCancelled) {
    
    
    NS_ADDREF(aEvent);
    SheetComplete(aEvent, NS_OK);
  }

  mPostedEvents.RemoveElement(aEvent);

  if (mDocument) {
    mDocument->UnblockOnload(PR_TRUE);
  }
}

nsresult NS_NewCSSLoader(nsIDocument* aDocument, nsICSSLoader** aLoader)
{
  CSSLoaderImpl* it = new CSSLoaderImpl();

  NS_ENSURE_TRUE(it, NS_ERROR_OUT_OF_MEMORY);

  it->Init(aDocument);
  return CallQueryInterface(it, aLoader);
}

nsresult NS_NewCSSLoader(nsICSSLoader** aLoader)
{
  CSSLoaderImpl* it = new CSSLoaderImpl();

  NS_ENSURE_TRUE(it, NS_ERROR_OUT_OF_MEMORY);

  return CallQueryInterface(it, aLoader);
}

static PLDHashOperator
StopLoadingSheetCallback(nsURIAndPrincipalHashKey* aKey,
                         SheetLoadData*& aData,
                         void* aClosure)
{
  NS_PRECONDITION(aData, "Must have a data!");
  NS_PRECONDITION(aClosure, "Must have a loader");

  aData->mIsLoading = PR_FALSE; 
  aData->mIsCancelled = PR_TRUE;
  
  static_cast<CSSLoaderImpl::LoadDataArray*>(aClosure)->AppendElement(aData);

  return PL_DHASH_REMOVE;
}

NS_IMETHODIMP
CSSLoaderImpl::Stop()
{
  PRUint32 pendingCount =
    mPendingDatas.IsInitialized() ?  mPendingDatas.Count() : 0;
  PRUint32 loadingCount =
    mLoadingDatas.IsInitialized() ? mLoadingDatas.Count() : 0;
  LoadDataArray arr(pendingCount + loadingCount + mPostedEvents.Length());
  
  if (pendingCount) {
    mPendingDatas.Enumerate(StopLoadingSheetCallback, &arr);
  }
  if (loadingCount) {
    mLoadingDatas.Enumerate(StopLoadingSheetCallback, &arr);
  }

  PRUint32 i;
  for (i = 0; i < mPostedEvents.Length(); ++i) {
    SheetLoadData* data = mPostedEvents[i];
    data->mIsCancelled = PR_TRUE;
    if (arr.AppendElement(data)) {
      
      NS_ADDREF(data);
    }
#ifdef DEBUG
    else {
      NS_NOTREACHED("We preallocated this memory... shouldn't really fail, "
                    "except we never check that preallocation succeeds.");
    }
#endif
  }
  mPostedEvents.Clear();

  mDatasToNotifyOn += arr.Length();
  for (i = 0; i < arr.Length(); ++i) {
    --mDatasToNotifyOn;
    SheetComplete(arr[i], NS_BINDING_ABORTED);
  }
  return NS_OK;
}

struct StopLoadingSheetsByURIClosure {
  StopLoadingSheetsByURIClosure(nsIURI* aURI,
                                CSSLoaderImpl::LoadDataArray& aArray) :
    uri(aURI), array(aArray)
  {}
  
  nsIURI* uri;
  CSSLoaderImpl::LoadDataArray& array;
};

static PLDHashOperator
StopLoadingSheetByURICallback(nsURIAndPrincipalHashKey* aKey,
                              SheetLoadData*& aData,
                              void* aClosure)
{
  NS_PRECONDITION(aData, "Must have a data!");
  NS_PRECONDITION(aClosure, "Must have a loader");

  StopLoadingSheetsByURIClosure* closure =
    static_cast<StopLoadingSheetsByURIClosure*>(aClosure);

  PRBool equal;
  if (NS_SUCCEEDED(aData->mURI->Equals(closure->uri, &equal)) &&
      equal) {
    aData->mIsLoading = PR_FALSE; 
    aData->mIsCancelled = PR_TRUE;

    closure->array.AppendElement(aData);
    return PL_DHASH_REMOVE;
  }

  return PL_DHASH_NEXT;
}

NS_IMETHODIMP
CSSLoaderImpl::StopLoadingSheet(nsIURI* aURL)
{
  NS_ENSURE_TRUE(aURL, NS_ERROR_NULL_POINTER);

  PRUint32 pendingCount =
    mPendingDatas.IsInitialized() ?  mPendingDatas.Count() : 0;
  PRUint32 loadingCount =
    mLoadingDatas.IsInitialized() ? mLoadingDatas.Count() : 0;
  LoadDataArray arr(pendingCount + loadingCount + mPostedEvents.Length());

  StopLoadingSheetsByURIClosure closure(aURL, arr);
  if (pendingCount) {
    mPendingDatas.Enumerate(StopLoadingSheetByURICallback, &closure);
  }
  if (loadingCount) {
    mLoadingDatas.Enumerate(StopLoadingSheetByURICallback, &closure);
  }

  PRUint32 i;
  for (i = 0; i < mPostedEvents.Length(); ++i) {
    SheetLoadData* curData = mPostedEvents[i];
    PRBool equal;
    if (curData->mURI && NS_SUCCEEDED(curData->mURI->Equals(aURL, &equal)) &&
        equal) {
      curData->mIsCancelled = PR_TRUE;
      if (arr.AppendElement(curData)) {
        
        NS_ADDREF(curData);
      }
#ifdef DEBUG
      else {
        NS_NOTREACHED("We preallocated this memory... shouldn't really fail, "
                      "except we never check that preallocation succeeds.");
      }
#endif
    }
  }
  mPostedEvents.Clear();

  mDatasToNotifyOn += arr.Length();
  for (i = 0; i < arr.Length(); ++i) {
    --mDatasToNotifyOn;
    SheetComplete(arr[i], NS_BINDING_ABORTED);
  }

  return NS_OK;
}

NS_IMETHODIMP
CSSLoaderImpl::GetEnabled(PRBool *aEnabled)
{
  NS_ENSURE_ARG_POINTER(aEnabled);
  *aEnabled = mEnabled;
  return NS_OK;
}

NS_IMETHODIMP
CSSLoaderImpl::SetEnabled(PRBool aEnabled)
{
  mEnabled = aEnabled;
  return NS_OK;
}

NS_IMETHODIMP_(PRBool)
CSSLoaderImpl::HasPendingLoads()
{
  return
    (mLoadingDatas.IsInitialized() && mLoadingDatas.Count() != 0) ||
    (mPendingDatas.IsInitialized() && mPendingDatas.Count() != 0) ||
    mPostedEvents.Length() != 0 ||
    mDatasToNotifyOn != 0;
}

NS_IMETHODIMP
CSSLoaderImpl::AddObserver(nsICSSLoaderObserver* aObserver)
{
  NS_PRECONDITION(aObserver, "Must have observer");
  if (mObservers.AppendElementUnlessExists(aObserver)) {
    return NS_OK;
  }

  return NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP_(void)
CSSLoaderImpl::RemoveObserver(nsICSSLoaderObserver* aObserver)
{
  mObservers.RemoveElement(aObserver);
}

static PLDHashOperator
CollectLoadDatas(nsURIAndPrincipalHashKey *aKey,
                 SheetLoadData* &aData,
                 void* aClosure)
{
  static_cast<CSSLoaderImpl::LoadDataArray*>(aClosure)->AppendElement(aData);
  return PL_DHASH_REMOVE;
}

void
CSSLoaderImpl::StartAlternateLoads()
{
  NS_PRECONDITION(mPendingDatas.IsInitialized(), "Don't call me!");
  LoadDataArray arr(mPendingDatas.Count());
  mPendingDatas.Enumerate(CollectLoadDatas, &arr);

  mDatasToNotifyOn += arr.Length();
  for (PRUint32 i = 0; i < arr.Length(); ++i) {
    --mDatasToNotifyOn;
    LoadSheet(arr[i], eSheetNeedsParser);
  }
}
