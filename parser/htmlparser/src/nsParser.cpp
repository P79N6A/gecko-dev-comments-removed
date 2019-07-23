






































#include "nsIAtom.h"
#include "nsParser.h"
#include "nsString.h"
#include "nsCRT.h"
#include "nsScanner.h"
#include "plstr.h"
#include "nsIStringStream.h"
#include "nsIChannel.h"
#include "nsICachingChannel.h"
#include "nsICacheEntryDescriptor.h"
#include "nsICharsetAlias.h"
#include "nsICharsetConverterManager.h"
#include "nsIInputStream.h"
#include "CNavDTD.h"
#include "prenv.h"
#include "prlock.h"
#include "prcvar.h"
#include "nsAutoLock.h"
#include "nsParserCIID.h"
#include "nsReadableUtils.h"
#include "nsCOMPtr.h"
#include "nsExpatDriver.h"
#include "nsIServiceManager.h"
#include "nsICategoryManager.h"
#include "nsISupportsPrimitives.h"
#include "nsIFragmentContentSink.h"
#include "nsStreamUtils.h"
#include "nsHTMLTokenizer.h"
#include "nsIDocument.h"
#include "nsNetUtil.h"
#include "nsScriptLoader.h"
#include "nsDataHashtable.h"
#include "nsIThreadPool.h"
#include "nsXPCOMCIDInternal.h"
#include "nsICSSStyleSheet.h"
#include "nsICSSLoaderObserver.h"
#include "nsICSSLoader.h"

#ifdef MOZ_VIEW_SOURCE
#include "nsViewSourceHTML.h"
#endif

#define NS_PARSER_FLAG_PARSER_ENABLED         0x00000002
#define NS_PARSER_FLAG_OBSERVERS_ENABLED      0x00000004
#define NS_PARSER_FLAG_PENDING_CONTINUE_EVENT 0x00000008
#define NS_PARSER_FLAG_CAN_INTERRUPT          0x00000010
#define NS_PARSER_FLAG_FLUSH_TOKENS           0x00000020
#define NS_PARSER_FLAG_CAN_TOKENIZE           0x00000040

static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
static NS_DEFINE_CID(kCParserCID, NS_PARSER_CID);
static NS_DEFINE_IID(kIParserIID, NS_IPARSER_IID);



nsCOMArray<nsIUnicharStreamListener> *nsParser::sParserDataListeners;

























































class nsParserContinueEvent : public nsRunnable
{
public:
  nsRefPtr<nsParser> mParser;

  nsParserContinueEvent(nsParser* aParser)
    : mParser(aParser)
  {}

  NS_IMETHOD Run()
  {
    mParser->HandleParserContinueEvent(this);
    return NS_OK;
  }
};



template <class Type>
class Holder {
public:
  typedef void (*Reaper)(Type *);

  Holder(Reaper aReaper)
    : mHoldee(nsnull), mReaper(aReaper)
  {
  }

  ~Holder() {
    if (mHoldee) {
      mReaper(mHoldee);
    }
  }

  Type *get() {
    return mHoldee;
  }
  const Holder &operator =(Type *aHoldee) {
    if (mHoldee && aHoldee != mHoldee) {
      mReaper(mHoldee);
    }
    mHoldee = aHoldee;
    return *this;
  }

private:
  Type *mHoldee;
  Reaper mReaper;
};

class nsSpeculativeScriptThread : public nsIRunnable {
public:
  nsSpeculativeScriptThread()
    : mLock(nsAutoLock::DestroyLock),
      mCVar(PR_DestroyCondVar),
      mKeepParsing(PR_FALSE),
      mCurrentlyParsing(PR_FALSE),
      mNumConsumed(0),
      mContext(nsnull),
      mTerminated(PR_FALSE) {
  }

  ~nsSpeculativeScriptThread() {
    NS_ASSERTION(NS_IsMainThread() || !mDocument,
                 "Destroying the document on the wrong thread");
  }

  NS_DECL_ISUPPORTS
  NS_DECL_NSIRUNNABLE

  nsresult StartParsing(nsParser *aParser);
  void StopParsing(PRBool aFromDocWrite);

  enum PrefetchType { NONE, SCRIPT, STYLESHEET, IMAGE };
  struct PrefetchEntry {
    PrefetchType type;
    nsString uri;
    nsString charset;
    nsString elementType;
  };

  nsIDocument *GetDocument() {
    NS_ASSERTION(NS_IsMainThread(), "Potential threadsafety hazard");
    return mDocument;
  }

  PRBool Parsing() {
    return mCurrentlyParsing;
  }

  CParserContext *Context() {
    return mContext;
  }

  typedef nsDataHashtable<nsCStringHashKey, PRBool> PreloadedType;
  PreloadedType& GetPreloadedURIs() {
    return mPreloadedURIs;
  }

  void Terminate() {
    mTerminated = PR_TRUE;
    StopParsing(PR_FALSE);
  }
  PRBool Terminated() {
    return mTerminated;
  }

private:

  void ProcessToken(CToken *aToken);

  void AddToPrefetchList(const nsAString &src,
                         const nsAString &charset,
                         const nsAString &elementType,
                         PrefetchType type);

  void FlushURIs();

  
  nsTokenAllocator mTokenAllocator;

  
  
  Holder<PRLock> mLock;
  Holder<PRCondVar> mCVar;

  volatile PRBool mKeepParsing;
  volatile PRBool mCurrentlyParsing;
  nsRefPtr<nsHTMLTokenizer> mTokenizer;
  nsAutoPtr<nsScanner> mScanner;

  enum { kBatchPrefetchURIs = 5 };
  nsAutoTArray<PrefetchEntry, kBatchPrefetchURIs> mURIs;

  
  PRUint32 mNumConsumed;

  
  nsCOMPtr<nsIDocument> mDocument;
  CParserContext *mContext;
  PreloadedType mPreloadedURIs;
  PRBool mTerminated;
};





class nsDummyCSSLoaderObserver : public nsICSSLoaderObserver {
public:
  NS_IMETHOD
  StyleSheetLoaded(nsICSSStyleSheet* aSheet, PRBool aWasAlternate, nsresult aStatus) {
      return NS_OK;
  }
  NS_DECL_ISUPPORTS
};

NS_IMPL_ISUPPORTS1(nsDummyCSSLoaderObserver, nsICSSLoaderObserver)

class nsPreloadURIs : public nsIRunnable {
public:
  nsPreloadURIs(nsAutoTArray<nsSpeculativeScriptThread::PrefetchEntry, 5> &aURIs,
                nsSpeculativeScriptThread *aScriptThread)
    : mURIs(aURIs),
      mScriptThread(aScriptThread) {
  }

  NS_DECL_ISUPPORTS
  NS_DECL_NSIRUNNABLE

  static void PreloadURIs(const nsAutoTArray<nsSpeculativeScriptThread::PrefetchEntry, 5> &aURIs,
                          nsSpeculativeScriptThread *aScriptThread);

private:
  nsAutoTArray<nsSpeculativeScriptThread::PrefetchEntry, 5> mURIs;
  nsRefPtr<nsSpeculativeScriptThread> mScriptThread;
};

NS_IMPL_THREADSAFE_ISUPPORTS1(nsPreloadURIs, nsIRunnable)

NS_IMETHODIMP
nsPreloadURIs::Run()
{
  PreloadURIs(mURIs, mScriptThread);
  return NS_OK;
}

void
nsPreloadURIs::PreloadURIs(const nsAutoTArray<nsSpeculativeScriptThread::PrefetchEntry, 5> &aURIs,
                           nsSpeculativeScriptThread *aScriptThread)
{
  NS_ASSERTION(NS_IsMainThread(), "Touching non-threadsafe objects off thread");

  if (aScriptThread->Terminated()) {
    return;
  }

  nsIDocument *doc = aScriptThread->GetDocument();
  NS_ASSERTION(doc, "We shouldn't have started preloading without a document");

  
  
  
  
  
  nsIURI *base = doc->GetBaseURI();
  const nsCString &charset = doc->GetDocumentCharacterSet();
  nsSpeculativeScriptThread::PreloadedType &alreadyPreloaded =
    aScriptThread->GetPreloadedURIs();
  for (PRUint32 i = 0, e = aURIs.Length(); i < e; ++i) {
    const nsSpeculativeScriptThread::PrefetchEntry &pe = aURIs[i];
    nsCOMPtr<nsIURI> uri;
    nsresult rv = NS_NewURI(getter_AddRefs(uri), pe.uri, charset.get(), base);
    if (NS_FAILED(rv)) {
      NS_WARNING("Failed to create a URI");
      continue;
    }

    nsCAutoString spec;
    uri->GetSpec(spec);
    PRBool answer;
    if (alreadyPreloaded.Get(spec, &answer)) {
      
      continue;
    }

    alreadyPreloaded.Put(spec, PR_TRUE);

    switch (pe.type) {
      case nsSpeculativeScriptThread::SCRIPT:
        doc->ScriptLoader()->PreloadURI(uri, pe.charset, pe.elementType);
        break;
      case nsSpeculativeScriptThread::IMAGE:
        doc->MaybePreLoadImage(uri);
        break;
      case nsSpeculativeScriptThread::STYLESHEET: {
        nsCOMPtr<nsICSSLoaderObserver> obs = new nsDummyCSSLoaderObserver();
        doc->CSSLoader()->LoadSheet(uri, doc->NodePrincipal(),
                                    NS_LossyConvertUTF16toASCII(pe.charset),
                                    obs);
        break;
      }
      case nsSpeculativeScriptThread::NONE:
        NS_NOTREACHED("Uninitialized preload entry?");
        break;
    }
  }
}

NS_IMPL_THREADSAFE_ISUPPORTS1(nsSpeculativeScriptThread, nsIRunnable)

NS_IMETHODIMP
nsSpeculativeScriptThread::Run()
{
  NS_ASSERTION(!NS_IsMainThread(), "Speculative parsing on the main thread?");

  mNumConsumed = 0;

  mTokenizer->WillTokenize(PR_FALSE, &mTokenAllocator);
  while (mKeepParsing) {
    PRBool flushTokens = PR_FALSE;
    nsresult rv = mTokenizer->ConsumeToken(*mScanner, flushTokens);
    if (NS_FAILED(rv)) {
      break;
    }

    mNumConsumed += mScanner->Mark();

    
    CToken *token;
    while (mKeepParsing && (token = mTokenizer->PopToken())) {
      ProcessToken(token);
    }
  }
  mTokenizer->DidTokenize(PR_FALSE);

  if (mKeepParsing) {
    
    
    
    if (!mURIs.IsEmpty()) {
      FlushURIs();
    }
  }

  {
    nsAutoLock al(mLock.get());

    mCurrentlyParsing = PR_FALSE;
    PR_NotifyCondVar(mCVar.get());
  }
  return NS_OK;
}

nsresult
nsSpeculativeScriptThread::StartParsing(nsParser *aParser)
{
  NS_ASSERTION(NS_IsMainThread(), "Called on the wrong thread");
  NS_ASSERTION(!mCurrentlyParsing, "Bad race happening");

  if (!aParser->ThreadPool()) {
    return NS_OK;
  }

  nsIContentSink *sink = aParser->GetContentSink();
  if (!sink) {
    return NS_OK;
  }

  nsCOMPtr<nsIDocument> doc = do_QueryInterface(sink->GetTarget());
  if (!doc) {
    return NS_OK;
  }

  nsAutoString toScan;
  CParserContext *context = aParser->PeekContext();
  if (!mLock.get()) {
    mLock = nsAutoLock::NewLock("nsSpeculativeScriptThread::mLock");
    if (!mLock.get()) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    mCVar = PR_NewCondVar(mLock.get());
    if (!mCVar.get()) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    if (!mPreloadedURIs.Init(15)) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    mTokenizer = new nsHTMLTokenizer(context->mDTDMode, context->mDocType,
                                     context->mParserCommand, 0);
    if (!mTokenizer) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    mTokenizer->CopyState(context->mTokenizer);
    context->mScanner->CopyUnusedData(toScan);
    if (toScan.IsEmpty()) {
      return NS_OK;
    }
  } else if (context == mContext) {
    
    nsScannerIterator end;
    context->mScanner->EndReading(end);

    nsScannerIterator start;
    context->mScanner->CurrentPosition(start);

    if (mNumConsumed > context->mNumConsumed) {
      
      
      PRUint32 distance = Distance(start, end);
      start.advance(PR_MIN(mNumConsumed - context->mNumConsumed, distance));
    }

    if (start == end) {
      
      return NS_OK;
    }

    CopyUnicodeTo(start, end, toScan);
  } else {
    
    context->mScanner->CopyUnusedData(toScan);
    if (toScan.IsEmpty()) {
      
      return NS_OK;
    }
  }

  nsCAutoString charset;
  PRInt32 source;
  aParser->GetDocumentCharset(charset, source);

  mScanner = new nsScanner(toScan, charset, source);
  if (!mScanner) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  mScanner->SetIncremental(PR_TRUE);

  mDocument.swap(doc);
  mKeepParsing = PR_TRUE;
  mCurrentlyParsing = PR_TRUE;
  mContext = context;
  return aParser->ThreadPool()->Dispatch(this, NS_DISPATCH_NORMAL);
}

void
nsSpeculativeScriptThread::StopParsing(PRBool )
{
  NS_ASSERTION(NS_IsMainThread(), "Can't stop parsing from another thread");

  if (!mLock.get()) {
    
    return;
  }

  {
    nsAutoLock al(mLock.get());

    mKeepParsing = PR_FALSE;
    if (mCurrentlyParsing) {
      PR_WaitCondVar(mCVar.get(), PR_INTERVAL_NO_TIMEOUT);
      NS_ASSERTION(!mCurrentlyParsing, "Didn't actually stop parsing?");
    }
  }

  
  if (mTerminated) {
    
    
    
    mDocument = nsnull;
    mTokenizer = nsnull;
    mScanner = nsnull;
  } else if (mURIs.Length()) {
    
    nsPreloadURIs::PreloadURIs(mURIs, this);
    mURIs.Clear();
  }

  
  
  
}

void
nsSpeculativeScriptThread::ProcessToken(CToken *aToken)
{
  

  CHTMLToken *token = static_cast<CHTMLToken *>(aToken);
  switch (static_cast<eHTMLTokenTypes>(token->GetTokenType())) {
    case eToken_start: {
        CStartToken *start = static_cast<CStartToken *>(aToken);
        nsHTMLTag tag = static_cast<nsHTMLTag>(start->GetTypeID());
        PRInt16 attrs = start->GetAttributeCount();
        PRInt16 i = 0;
        nsAutoString src;
        nsAutoString elementType;
        nsAutoString charset;
        nsAutoString href;
        nsAutoString rel;
        PrefetchType ptype = NONE;

        switch (tag) {
          case eHTMLTag_link:
              ptype = STYLESHEET;
              break;

          case eHTMLTag_img:
              ptype = IMAGE;
              break;

          case eHTMLTag_script:
              ptype = SCRIPT;
              break;

          default:
              break;
        }

        
        
        
        if (ptype != NONE) {
            
            for (; i < attrs ; ++i) {
              CAttributeToken *attr = static_cast<CAttributeToken *>(mTokenizer->PopToken());
              NS_ASSERTION(attr->GetTokenType() == eToken_attribute, "Weird token");

              if (attr->GetKey().EqualsLiteral("src")) {
                src.Assign(attr->GetValue());
              } else if (attr->GetKey().EqualsLiteral("href")) {
                href.Assign(attr->GetValue());
              } else if (attr->GetKey().EqualsLiteral("rel")) {
                rel.Assign(attr->GetValue());
              } else if (attr->GetKey().EqualsLiteral("charset")) {
                charset.Assign(attr->GetValue());
              } else if (attr->GetKey().EqualsLiteral("type")) {
                elementType.Assign(attr->GetValue());
              }

              IF_FREE(attr, &mTokenAllocator);
            }

            
            if (ptype == STYLESHEET) {
              if (rel.EqualsLiteral("stylesheet")) {
                src = href; 
              } else {
                src.Truncate(); 
              }
            }

            
            if (!src.IsEmpty()) {
              AddToPrefetchList(src, charset, elementType, ptype);
            }
        } else {
            
            for (; i < attrs ; ++i) {
              CToken *attr = mTokenizer->PopToken();
              IF_FREE(attr, &mTokenAllocator);
            }
        }
        break;
      }

    default:
      break;
  }

  IF_FREE(aToken, &mTokenAllocator);
}

void
nsSpeculativeScriptThread::AddToPrefetchList(const nsAString &src,
                                             const nsAString &charset,
                                             const nsAString &elementType,
                                             PrefetchType type)
{
  PrefetchEntry *pe = mURIs.AppendElement();
  pe->type = type;
  pe->uri = src;
  pe->charset = charset;
  pe->elementType = elementType;

  if (mURIs.Length() == kBatchPrefetchURIs) {
    FlushURIs();
  }
}

void
nsSpeculativeScriptThread::FlushURIs()
{
  nsCOMPtr<nsIRunnable> r = new nsPreloadURIs(mURIs, this);
  if (!r) {
    return;
  }

  mURIs.Clear();
  NS_DispatchToMainThread(r, NS_DISPATCH_NORMAL);
}

nsICharsetAlias* nsParser::sCharsetAliasService = nsnull;
nsICharsetConverterManager* nsParser::sCharsetConverterManager = nsnull;
nsIThreadPool* nsParser::sSpeculativeThreadPool = nsnull;





nsresult
nsParser::Init()
{
  nsresult rv;
  nsCOMPtr<nsICategoryManager> cm =
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISimpleEnumerator> e;
  rv = cm->EnumerateCategory("Parser data listener", getter_AddRefs(e));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString categoryEntry;
  nsXPIDLCString contractId;
  nsCOMPtr<nsISupports> entry;

  while (NS_SUCCEEDED(e->GetNext(getter_AddRefs(entry)))) {
    nsCOMPtr<nsISupportsCString> category(do_QueryInterface(entry));

    if (!category) {
      NS_WARNING("Category entry not an nsISupportsCString!");
      continue;
    }

    rv = category->GetData(categoryEntry);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = cm->GetCategoryEntry("Parser data listener", categoryEntry.get(),
                              getter_Copies(contractId));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIUnicharStreamListener> listener =
      do_CreateInstance(contractId.get());

    if (listener) {
      if (!sParserDataListeners) {
        sParserDataListeners = new nsCOMArray<nsIUnicharStreamListener>();

        if (!sParserDataListeners)
          return NS_ERROR_OUT_OF_MEMORY;
      }

      sParserDataListeners->AppendObject(listener);
    }
  }

  nsCOMPtr<nsICharsetAlias> charsetAlias =
    do_GetService(NS_CHARSETALIAS_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsICharsetConverterManager> charsetConverter =
    do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  charsetAlias.swap(sCharsetAliasService);
  charsetConverter.swap(sCharsetConverterManager);

  nsCOMPtr<nsIThreadPool> threadPool =
    do_CreateInstance(NS_THREADPOOL_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = threadPool->SetThreadLimit(kSpeculativeThreadLimit);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = threadPool->SetIdleThreadLimit(kIdleThreadLimit);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = threadPool->SetIdleThreadTimeout(kIdleThreadTimeout);
  NS_ENSURE_SUCCESS(rv, rv);

  threadPool.swap(sSpeculativeThreadPool);

  return NS_OK;
}






void nsParser::Shutdown()
{
  delete sParserDataListeners;
  sParserDataListeners = nsnull;

  NS_IF_RELEASE(sCharsetAliasService);
  NS_IF_RELEASE(sCharsetConverterManager);
  if (sSpeculativeThreadPool) {
    sSpeculativeThreadPool->Shutdown();
    NS_RELEASE(sSpeculativeThreadPool);
  }
}

#ifdef DEBUG
static PRBool gDumpContent=PR_FALSE;
#endif




nsParser::nsParser()
{
  Initialize(PR_TRUE);
}

nsParser::~nsParser()
{
  Cleanup();
}

void
nsParser::Initialize(PRBool aConstructor)
{
#ifdef NS_DEBUG
  if (!gDumpContent) {
    gDumpContent = PR_GetEnv("PARSER_DUMP_CONTENT") != nsnull;
  }
#endif

  if (aConstructor) {
    
    mParserContext = 0;
  }
  else {
    
    mObserver = nsnull;
    mParserFilter = nsnull;
    mUnusedInput.Truncate();
  }

  mContinueEvent = nsnull;
  mCharsetSource = kCharsetUninitialized;
  mCharset.AssignLiteral("ISO-8859-1");
  mInternalState = NS_OK;
  mStreamStatus = 0;
  mCommand = eViewNormal;
  mFlags = NS_PARSER_FLAG_OBSERVERS_ENABLED |
           NS_PARSER_FLAG_PARSER_ENABLED |
           NS_PARSER_FLAG_CAN_TOKENIZE;

  mProcessingNetworkData = PR_FALSE;
}

void
nsParser::Cleanup()
{
#ifdef NS_DEBUG
  if (gDumpContent) {
    if (mSink) {
      
      
      nsresult result = NS_OK;
      nsCOMPtr<nsIDebugDumpContent> trigger = do_QueryInterface(mSink, &result);
      if (NS_SUCCEEDED(result)) {
        trigger->DumpContentModel();
      }
    }
  }
#endif

#ifdef DEBUG
  if (mParserContext && mParserContext->mPrevContext) {
    NS_WARNING("Extra parser contexts still on the parser stack");
  }
#endif

  while (mParserContext) {
    CParserContext *pc = mParserContext->mPrevContext;
    delete mParserContext;
    mParserContext = pc;
  }

  
  
  
  NS_ASSERTION(!(mFlags & NS_PARSER_FLAG_PENDING_CONTINUE_EVENT), "bad");
  if (mSpeculativeScriptThread) {
    mSpeculativeScriptThread->Terminate();
    mSpeculativeScriptThread = nsnull;
  }
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsParser)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsParser)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mDTD)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mSink)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mObserver)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsParser)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mDTD)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mSink)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mObserver)
  CParserContext *pc = tmp->mParserContext;
  while (pc) {
    cb.NoteXPCOMChild(pc->mTokenizer);
    pc = pc->mPrevContext;
  }
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTING_ADDREF_AMBIGUOUS(nsParser, nsIParser)
NS_IMPL_CYCLE_COLLECTING_RELEASE_AMBIGUOUS(nsParser, nsIParser)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsParser)
  NS_INTERFACE_MAP_ENTRY(nsIStreamListener)
  NS_INTERFACE_MAP_ENTRY(nsIParser)
  NS_INTERFACE_MAP_ENTRY(nsIRequestObserver)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIParser)
NS_INTERFACE_MAP_END






nsresult
nsParser::PostContinueEvent()
{
  if (!(mFlags & NS_PARSER_FLAG_PENDING_CONTINUE_EVENT)) {
    
    NS_ASSERTION(!mContinueEvent, "bad");

    
    
    nsCOMPtr<nsIRunnable> event = new nsParserContinueEvent(this);
    if (NS_FAILED(NS_DispatchToCurrentThread(event))) {
        NS_WARNING("failed to dispatch parser continuation event");
    } else {
        mFlags |= NS_PARSER_FLAG_PENDING_CONTINUE_EVENT;
        mContinueEvent = event;
    }
  }
  return NS_OK;
}

NS_IMETHODIMP_(void)
nsParser::SetParserFilter(nsIParserFilter * aFilter)
{
  mParserFilter = aFilter;
}

NS_IMETHODIMP_(void)
nsParser::GetCommand(nsCString& aCommand)
{
  aCommand = mCommandStr;
}








NS_IMETHODIMP_(void)
nsParser::SetCommand(const char* aCommand)
{
  mCommandStr.Assign(aCommand);
  if (mCommandStr.Equals(kViewSourceCommand)) {
    mCommand = eViewSource;
  } else if (mCommandStr.Equals(kViewFragmentCommand)) {
    mCommand = eViewFragment;
  } else {
    mCommand = eViewNormal;
  }
}








NS_IMETHODIMP_(void)
nsParser::SetCommand(eParserCommands aParserCommand)
{
  mCommand = aParserCommand;
}








NS_IMETHODIMP_(void)
nsParser::SetDocumentCharset(const nsACString& aCharset, PRInt32 aCharsetSource)
{
  mCharset = aCharset;
  mCharsetSource = aCharsetSource;
  if (mParserContext && mParserContext->mScanner) {
     mParserContext->mScanner->SetDocumentCharset(aCharset, aCharsetSource);
  }
}

void
nsParser::SetSinkCharset(nsACString& aCharset)
{
  if (mSink) {
    mSink->SetDocumentCharset(aCharset);
  }
}







NS_IMETHODIMP_(void)
nsParser::SetContentSink(nsIContentSink* aSink)
{
  NS_PRECONDITION(aSink, "sink cannot be null!");
  mSink = aSink;

  if (mSink) {
    mSink->SetParser(this);
  }
}





NS_IMETHODIMP_(nsIContentSink*)
nsParser::GetContentSink()
{
  return mSink;
}














static PRInt32
ParsePS(const nsString& aBuffer, PRInt32 aIndex)
{
  for (;;) {
    PRUnichar ch = aBuffer.CharAt(aIndex);
    if ((ch == PRUnichar(' ')) || (ch == PRUnichar('\t')) ||
        (ch == PRUnichar('\n')) || (ch == PRUnichar('\r'))) {
      ++aIndex;
    } else if (ch == PRUnichar('-')) {
      PRInt32 tmpIndex;
      if (aBuffer.CharAt(aIndex+1) == PRUnichar('-') &&
          kNotFound != (tmpIndex=aBuffer.Find("--",PR_FALSE,aIndex+2,-1))) {
        aIndex = tmpIndex + 2;
      } else {
        return aIndex;
      }
    } else {
      return aIndex;
    }
  }
}

#define PARSE_DTD_HAVE_DOCTYPE          (1<<0)
#define PARSE_DTD_HAVE_PUBLIC_ID        (1<<1)
#define PARSE_DTD_HAVE_SYSTEM_ID        (1<<2)
#define PARSE_DTD_HAVE_INTERNAL_SUBSET  (1<<3)


static PRBool
ParseDocTypeDecl(const nsString &aBuffer,
                 PRInt32 *aResultFlags,
                 nsString &aPublicID,
                 nsString &aSystemID)
{
  PRBool haveDoctype = PR_FALSE;
  *aResultFlags = 0;

  
  
  PRInt32 theIndex = 0;
  do {
    theIndex = aBuffer.FindChar('<', theIndex);
    if (theIndex == kNotFound) break;
    PRUnichar nextChar = aBuffer.CharAt(theIndex+1);
    if (nextChar == PRUnichar('!')) {
      PRInt32 tmpIndex = theIndex + 2;
      if (kNotFound !=
          (theIndex=aBuffer.Find("DOCTYPE", PR_TRUE, tmpIndex, 0))) {
        haveDoctype = PR_TRUE;
        theIndex += 7; 
        break;
      }
      theIndex = ParsePS(aBuffer, tmpIndex);
      theIndex = aBuffer.FindChar('>', theIndex);
    } else if (nextChar == PRUnichar('?')) {
      theIndex = aBuffer.FindChar('>', theIndex);
    } else {
      break;
    }
  } while (theIndex != kNotFound);

  if (!haveDoctype)
    return PR_TRUE;
  *aResultFlags |= PARSE_DTD_HAVE_DOCTYPE;

  theIndex = ParsePS(aBuffer, theIndex);
  theIndex = aBuffer.Find("HTML", PR_TRUE, theIndex, 0);
  if (kNotFound == theIndex)
    return PR_FALSE;
  theIndex = ParsePS(aBuffer, theIndex+4);
  PRInt32 tmpIndex = aBuffer.Find("PUBLIC", PR_TRUE, theIndex, 0);

  if (kNotFound != tmpIndex) {
    theIndex = ParsePS(aBuffer, tmpIndex+6);

    
    

    
    

    PRUnichar lit = aBuffer.CharAt(theIndex);
    if ((lit != PRUnichar('\"')) && (lit != PRUnichar('\'')))
      return PR_FALSE;

    
    

    PRInt32 PublicIDStart = theIndex + 1;
    PRInt32 PublicIDEnd = aBuffer.FindChar(lit, PublicIDStart);
    if (kNotFound == PublicIDEnd)
      return PR_FALSE;
    theIndex = ParsePS(aBuffer, PublicIDEnd + 1);
    PRUnichar next = aBuffer.CharAt(theIndex);
    if (next == PRUnichar('>')) {
      
      
      
      
      
    } else if ((next == PRUnichar('\"')) ||
               (next == PRUnichar('\''))) {
      
      *aResultFlags |= PARSE_DTD_HAVE_SYSTEM_ID;
      PRInt32 SystemIDStart = theIndex + 1;
      PRInt32 SystemIDEnd = aBuffer.FindChar(next, SystemIDStart);
      if (kNotFound == SystemIDEnd)
        return PR_FALSE;
      aSystemID =
        Substring(aBuffer, SystemIDStart, SystemIDEnd - SystemIDStart);
    } else if (next == PRUnichar('[')) {
      
      *aResultFlags |= PARSE_DTD_HAVE_INTERNAL_SUBSET;
    } else {
      
      return PR_FALSE;
    }

    
    
    aPublicID = Substring(aBuffer, PublicIDStart, PublicIDEnd - PublicIDStart);
    aPublicID.CompressWhitespace(PR_TRUE, PR_TRUE);
    *aResultFlags |= PARSE_DTD_HAVE_PUBLIC_ID;
  } else {
    tmpIndex=aBuffer.Find("SYSTEM", PR_TRUE, theIndex, 0);
    if (kNotFound != tmpIndex) {
      
      *aResultFlags |= PARSE_DTD_HAVE_SYSTEM_ID;

      theIndex = ParsePS(aBuffer, tmpIndex+6);
      PRUnichar next = aBuffer.CharAt(theIndex);
      if (next != PRUnichar('\"') && next != PRUnichar('\''))
        return PR_FALSE;

      PRInt32 SystemIDStart = theIndex + 1;
      PRInt32 SystemIDEnd = aBuffer.FindChar(next, SystemIDStart);

      if (kNotFound == SystemIDEnd)
        return PR_FALSE;
      aSystemID =
        Substring(aBuffer, SystemIDStart, SystemIDEnd - SystemIDStart);
      theIndex = ParsePS(aBuffer, SystemIDEnd + 1);
    }

    PRUnichar nextChar = aBuffer.CharAt(theIndex);
    if (nextChar == PRUnichar('['))
      *aResultFlags |= PARSE_DTD_HAVE_INTERNAL_SUBSET;
    else if (nextChar != PRUnichar('>'))
      return PR_FALSE;
  }
  return PR_TRUE;
}

struct PubIDInfo
{
  enum eMode {
    eQuirks,         
    eAlmostStandards,
    eFullStandards   
      




  };

  const char* name;
  eMode mode_if_no_sysid;
  eMode mode_if_sysid;
};

#define ELEMENTS_OF(array_) (sizeof(array_)/sizeof(array_[0]))










static const PubIDInfo kPublicIDs[] = {
  {"+//silmaril//dtd html pro v0r11 19970101//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//advasoft ltd//dtd html 3.0 aswedit + extensions//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//as//dtd html 3.0 aswedit + extensions//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html 2.0 level 1//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html 2.0 level 2//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html 2.0 strict level 1//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html 2.0 strict level 2//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html 2.0 strict//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html 2.0//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html 2.1e//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html 3.0//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html 3.0//en//" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html 3.2 final//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html 3.2//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html 3//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html level 0//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html level 0//en//2.0" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html level 1//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html level 1//en//2.0" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html level 2//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html level 2//en//2.0" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html level 3//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html level 3//en//3.0" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html strict level 0//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html strict level 0//en//2.0" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html strict level 1//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html strict level 1//en//2.0" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html strict level 2//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html strict level 2//en//2.0" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html strict level 3//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html strict level 3//en//3.0" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html strict//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html strict//en//2.0" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html strict//en//3.0" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html//en//2.0" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//ietf//dtd html//en//3.0" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//metrius//dtd metrius presentational//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//microsoft//dtd internet explorer 2.0 html strict//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//microsoft//dtd internet explorer 2.0 html//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//microsoft//dtd internet explorer 2.0 tables//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//microsoft//dtd internet explorer 3.0 html strict//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//microsoft//dtd internet explorer 3.0 html//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//microsoft//dtd internet explorer 3.0 tables//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//netscape comm. corp.//dtd html//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//netscape comm. corp.//dtd strict html//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//o'reilly and associates//dtd html 2.0//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//o'reilly and associates//dtd html extended 1.0//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//o'reilly and associates//dtd html extended relaxed 1.0//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//softquad software//dtd hotmetal pro 6.0::19990601::extensions to html 4.0//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//softquad//dtd hotmetal pro 4.0::19971010::extensions to html 4.0//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//spyglass//dtd html 2.0 extended//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//sq//dtd html 2.0 hotmetal + extensions//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//sun microsystems corp.//dtd hotjava html//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//sun microsystems corp.//dtd hotjava strict html//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//w3c//dtd html 3 1995-03-24//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//w3c//dtd html 3.2 draft//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//w3c//dtd html 3.2 final//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//w3c//dtd html 3.2//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//w3c//dtd html 3.2s draft//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//w3c//dtd html 4.0 frameset//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//w3c//dtd html 4.0 transitional//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//w3c//dtd html 4.01 frameset//en" , PubIDInfo::eQuirks, PubIDInfo::eAlmostStandards},
  {"-//w3c//dtd html 4.01 transitional//en" , PubIDInfo::eQuirks, PubIDInfo::eAlmostStandards},
  {"-//w3c//dtd html experimental 19960712//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//w3c//dtd html experimental 970421//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//w3c//dtd w3 html//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//w3c//dtd xhtml 1.0 frameset//en" , PubIDInfo::eAlmostStandards, PubIDInfo::eAlmostStandards},
  {"-//w3c//dtd xhtml 1.0 transitional//en" , PubIDInfo::eAlmostStandards, PubIDInfo::eAlmostStandards},
  {"-//w3o//dtd w3 html 3.0//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//w3o//dtd w3 html 3.0//en//" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//w3o//dtd w3 html strict 3.0//en//" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//webtechs//dtd mozilla html 2.0//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-//webtechs//dtd mozilla html//en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"-/w3c/dtd html 4.0 transitional/en" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
  {"html" , PubIDInfo::eQuirks, PubIDInfo::eQuirks},
};

#ifdef DEBUG
static void
VerifyPublicIDs()
{
  static PRBool gVerified = PR_FALSE;
  if (!gVerified) {
    gVerified = PR_TRUE;
    PRUint32 i;
    for (i = 0; i < ELEMENTS_OF(kPublicIDs) - 1; ++i) {
      if (nsCRT::strcmp(kPublicIDs[i].name, kPublicIDs[i+1].name) >= 0) {
        NS_NOTREACHED("doctypes out of order");
        printf("Doctypes %s and %s out of order.\n",
               kPublicIDs[i].name, kPublicIDs[i+1].name);
      }
    }
    for (i = 0; i < ELEMENTS_OF(kPublicIDs); ++i) {
      nsCAutoString lcPubID(kPublicIDs[i].name);
      ToLowerCase(lcPubID);
      if (nsCRT::strcmp(kPublicIDs[i].name, lcPubID.get()) != 0) {
        NS_NOTREACHED("doctype not lower case");
        printf("Doctype %s not lower case.\n", kPublicIDs[i].name);
      }
    }
  }
}
#endif

static void
DetermineHTMLParseMode(const nsString& aBuffer,
                       nsDTDMode& aParseMode,
                       eParserDocType& aDocType)
{
#ifdef DEBUG
  VerifyPublicIDs();
#endif
  PRInt32 resultFlags;
  nsAutoString publicIDUCS2, sysIDUCS2;
  if (ParseDocTypeDecl(aBuffer, &resultFlags, publicIDUCS2, sysIDUCS2)) {
    if (!(resultFlags & PARSE_DTD_HAVE_DOCTYPE)) {
      
      aParseMode = eDTDMode_quirks;
      aDocType = eHTML_Quirks;
    } else if ((resultFlags & PARSE_DTD_HAVE_INTERNAL_SUBSET) ||
               !(resultFlags & PARSE_DTD_HAVE_PUBLIC_ID)) {
      
      
      aDocType = eHTML_Strict;
      aParseMode = eDTDMode_full_standards;

      
      if (!(resultFlags & PARSE_DTD_HAVE_INTERNAL_SUBSET) &&
          sysIDUCS2 == NS_LITERAL_STRING(
               "http://www.ibm.com/data/dtd/v11/ibmxhtml1-transitional.dtd")) {
        aParseMode = eDTDMode_quirks;
        aDocType = eHTML_Quirks;
      }

    } else {
      
      
      nsCAutoString publicID;
      publicID.AssignWithConversion(publicIDUCS2);

      
      
      ToLowerCase(publicID);

      
      
      
      PRInt32 minimum = 0;
      PRInt32 maximum = ELEMENTS_OF(kPublicIDs) - 1;
      PRInt32 index;
      for (;;) {
        index = (minimum + maximum) / 2;
        PRInt32 comparison =
            nsCRT::strcmp(publicID.get(), kPublicIDs[index].name);
        if (comparison == 0)
          break;
        if (comparison < 0)
          maximum = index - 1;
        else
          minimum = index + 1;

        if (maximum < minimum) {
          
          aParseMode = eDTDMode_full_standards;
          aDocType = eHTML_Strict;
          return;
        }
      }

      switch ((resultFlags & PARSE_DTD_HAVE_SYSTEM_ID)
                ? kPublicIDs[index].mode_if_sysid
                : kPublicIDs[index].mode_if_no_sysid)
      {
        case PubIDInfo::eQuirks:
          aParseMode = eDTDMode_quirks;
          aDocType = eHTML_Quirks;
          break;
        case PubIDInfo::eAlmostStandards:
          aParseMode = eDTDMode_almost_standards;
          aDocType = eHTML_Strict;
          break;
        case PubIDInfo::eFullStandards:
          aParseMode = eDTDMode_full_standards;
          aDocType = eHTML_Strict;
          break;
        default:
          NS_NOTREACHED("no other cases!");
      }
    }
  } else {
    
    aParseMode = eDTDMode_quirks;
    aDocType = eHTML_Quirks;
  }
}

static void
DetermineParseMode(const nsString& aBuffer, nsDTDMode& aParseMode,
                   eParserDocType& aDocType, const nsACString& aMimeType)
{
  if (aMimeType.EqualsLiteral(kHTMLTextContentType)) {
    DetermineHTMLParseMode(aBuffer, aParseMode, aDocType);
  } else if (aMimeType.EqualsLiteral(kPlainTextContentType) ||
             aMimeType.EqualsLiteral(kTextCSSContentType) ||
             aMimeType.EqualsLiteral(kApplicationJSContentType) ||
             aMimeType.EqualsLiteral(kApplicationXJSContentType) ||
             aMimeType.EqualsLiteral(kTextECMAScriptContentType) ||
             aMimeType.EqualsLiteral(kApplicationECMAScriptContentType) ||
             aMimeType.EqualsLiteral(kTextJSContentType)) {
    aDocType = ePlainText;
    aParseMode = eDTDMode_quirks;
  } else { 
    aDocType = eXML;
    aParseMode = eDTDMode_full_standards;
  }
}

static nsIDTD*
FindSuitableDTD(CParserContext& aParserContext)
{
  
  aParserContext.mAutoDetectStatus = ePrimaryDetect;

#ifdef MOZ_VIEW_SOURCE
  
  if (aParserContext.mParserCommand == eViewSource) {
    return new CViewSourceHTML();
  }
#endif

  
  
  if (aParserContext.mDocType != eXML) {
    return new CNavDTD();
  }

  
  NS_ASSERTION(aParserContext.mDocType == eXML, "What are you trying to send me, here?");
  return new nsExpatDriver();
}

NS_IMETHODIMP
nsParser::CancelParsingEvents()
{
  if (mFlags & NS_PARSER_FLAG_PENDING_CONTINUE_EVENT) {
    NS_ASSERTION(mContinueEvent, "mContinueEvent is null");
    
    mContinueEvent = nsnull;
    mFlags &= ~NS_PARSER_FLAG_PENDING_CONTINUE_EVENT;
  }
  return NS_OK;
}















































#define PREFER_LATTER_ERROR_CODE(EXPR1, EXPR2, RV) {                          \
  nsresult RV##__temp = EXPR1;                                                \
  RV = EXPR2;                                                                 \
  if (NS_FAILED(RV)) {                                                        \
    RV = RV##__temp;                                                          \
  }                                                                           \
}








nsresult
nsParser::WillBuildModel(nsString& aFilename)
{
  if (!mParserContext)
    return kInvalidParserContext;

  if (eUnknownDetect != mParserContext->mAutoDetectStatus)
    return NS_OK;

  if (eDTDMode_unknown == mParserContext->mDTDMode ||
      eDTDMode_autodetect == mParserContext->mDTDMode) {
    PRUnichar buf[1025];
    nsFixedString theBuffer(buf, 1024, 0);

    
    
    mParserContext->mScanner->Peek(theBuffer, 1024, mParserContext->mScanner->FirstNonWhitespacePosition());
    DetermineParseMode(theBuffer, mParserContext->mDTDMode,
                       mParserContext->mDocType, mParserContext->mMimeType);
  }

  NS_ASSERTION(!mDTD || !mParserContext->mPrevContext,
               "Clobbering DTD for non-root parser context!");
  mDTD = FindSuitableDTD(*mParserContext);
  NS_ENSURE_TRUE(mDTD, NS_ERROR_OUT_OF_MEMORY);

  nsITokenizer* tokenizer;
  nsresult rv = mParserContext->GetTokenizer(mDTD, mSink, tokenizer);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDTD->WillBuildModel(*mParserContext, tokenizer, mSink);
  nsresult sinkResult = mSink->WillBuildModel(mDTD->GetMode());
  
  
  
  
  
  
  
  return NS_FAILED(sinkResult) ? sinkResult : rv;
}






nsresult
nsParser::DidBuildModel(nsresult anErrorCode)
{
  nsresult result = anErrorCode;

  if (IsComplete()) {
    if (mParserContext && !mParserContext->mPrevContext) {
      
      
      PRBool terminated = mInternalState == NS_ERROR_HTMLPARSER_STOPPARSING;
      if (mDTD && mSink) {
        nsresult dtdResult =  mDTD->DidBuildModel(anErrorCode),
                sinkResult = mSink->DidBuildModel(terminated);
        
        
        
        
        
        
        
        result = NS_FAILED(sinkResult) ? sinkResult : dtdResult;
      }

      
      mParserContext->mRequest = 0;

      if (mSpeculativeScriptThread) {
        mSpeculativeScriptThread->Terminate();
        mSpeculativeScriptThread = nsnull;
      }
    }
  }

  return result;
}

void
nsParser::SpeculativelyParse()
{
  if (mParserContext->mParserCommand == eViewNormal &&
      !mParserContext->mMimeType.EqualsLiteral("text/html")) {
    return;
  }

  if (!mSpeculativeScriptThread) {
    mSpeculativeScriptThread = new nsSpeculativeScriptThread();
    if (!mSpeculativeScriptThread) {
      return;
    }
  }

  nsresult rv = mSpeculativeScriptThread->StartParsing(this);
  if (NS_FAILED(rv)) {
    mSpeculativeScriptThread = nsnull;
  }
}







void
nsParser::PushContext(CParserContext& aContext)
{
  NS_ASSERTION(aContext.mPrevContext == mParserContext,
               "Trying to push a context whose previous context differs from "
               "the current parser context.");
  mParserContext = &aContext;
}








CParserContext*
nsParser::PopContext()
{
  CParserContext* oldContext = mParserContext;
  if (oldContext) {
    mParserContext = oldContext->mPrevContext;
    if (mParserContext) {
      
      
      
      if (mParserContext->mStreamListenerState != eOnStop) {
        mParserContext->mStreamListenerState = oldContext->mStreamListenerState;
      }
      
      
      
      if (mParserContext->mTokenizer) {
        mParserContext->mTokenizer->CopyState(oldContext->mTokenizer);
      }
    }
  }
  return oldContext;
}









void
nsParser::SetUnusedInput(nsString& aBuffer)
{
  mUnusedInput = aBuffer;
}

NS_IMETHODIMP_(void *)
nsParser::GetRootContextKey()
{
  CParserContext* pc = mParserContext;
  if (!pc) {
    return nsnull;
  }

  while (pc->mPrevContext) {
    pc = pc->mPrevContext;
  }

  return pc->mKey;
}






NS_IMETHODIMP
nsParser::Terminate(void)
{
  
  
  if (mInternalState == NS_ERROR_HTMLPARSER_STOPPARSING) {
    return NS_OK;
  }

  nsresult result = NS_OK;
  
  
  nsCOMPtr<nsIParser> kungFuDeathGrip(this);
  mInternalState = result = NS_ERROR_HTMLPARSER_STOPPARSING;

  
  
  
  
  
  CancelParsingEvents();
  if (mSpeculativeScriptThread) {
    mSpeculativeScriptThread->Terminate();
    mSpeculativeScriptThread = nsnull;
  }

  
  
  
  
  
  while (mParserContext && mParserContext->mPrevContext) {
    CParserContext *prev = mParserContext->mPrevContext;
    delete mParserContext;
    mParserContext = prev;
  }

  if (mDTD) {
    mDTD->Terminate();
    DidBuildModel(result);
  } else if (mSink) {
    
    
    result = mSink->DidBuildModel(PR_TRUE);
    NS_ENSURE_SUCCESS(result, result);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsParser::ContinueParsing()
{
  if (mFlags & NS_PARSER_FLAG_PARSER_ENABLED) {
    NS_WARNING("Trying to continue parsing on a unblocked parser.");
    return NS_OK;
  }

  mFlags |= NS_PARSER_FLAG_PARSER_ENABLED;

  return ContinueInterruptedParsing();
}

NS_IMETHODIMP
nsParser::ContinueInterruptedParsing()
{
  
  
  
  if (!IsOkToProcessNetworkData()) {
    return NS_OK;
  }

  
  
  
  
  nsresult result=NS_OK;
  nsCOMPtr<nsIParser> kungFuDeathGrip(this);

#ifdef DEBUG
  if (!(mFlags & NS_PARSER_FLAG_PARSER_ENABLED)) {
    NS_WARNING("Don't call ContinueInterruptedParsing on a blocked parser.");
  }
#endif

  if (mSpeculativeScriptThread) {
    mSpeculativeScriptThread->StopParsing(PR_FALSE);
  }

  PRBool isFinalChunk = mParserContext &&
                        mParserContext->mStreamListenerState == eOnStop;

  mProcessingNetworkData = PR_TRUE;
  if (mSink) {
    mSink->WillParse();
  }
  result = ResumeParse(PR_TRUE, isFinalChunk); 
  mProcessingNetworkData = PR_FALSE;

  if (result != NS_OK) {
    result=mInternalState;
  }

  return result;
}





NS_IMETHODIMP_(void)
nsParser::BlockParser()
{
  mFlags &= ~NS_PARSER_FLAG_PARSER_ENABLED;
}







NS_IMETHODIMP_(void)
nsParser::UnblockParser()
{
  if (!(mFlags & NS_PARSER_FLAG_PARSER_ENABLED)) {
    mFlags |= NS_PARSER_FLAG_PARSER_ENABLED;
  } else {
    NS_WARNING("Trying to unblock an unblocked parser.");
  }
}




NS_IMETHODIMP_(PRBool)
nsParser::IsParserEnabled()
{
  return (mFlags & NS_PARSER_FLAG_PARSER_ENABLED) != 0;
}




NS_IMETHODIMP_(PRBool)
nsParser::IsComplete()
{
  return !(mFlags & NS_PARSER_FLAG_PENDING_CONTINUE_EVENT);
}


void nsParser::HandleParserContinueEvent(nsParserContinueEvent *ev)
{
  
  if (mContinueEvent != ev)
    return;

  mFlags &= ~NS_PARSER_FLAG_PENDING_CONTINUE_EVENT;
  mContinueEvent = nsnull;

  NS_ASSERTION(IsOkToProcessNetworkData(),
               "Interrupted in the middle of a script?");
  ContinueInterruptedParsing();
}

nsresult
nsParser::DataAdded(const nsSubstring& aData, nsIRequest *aRequest)
{
  NS_ASSERTION(sParserDataListeners,
               "Don't call this with no parser data listeners!");

  if (!mSink || !aRequest) {
    return NS_OK;
  }

  nsISupports *ctx = mSink->GetTarget();
  PRInt32 count = sParserDataListeners->Count();
  nsresult rv = NS_OK;
  PRBool canceled = PR_FALSE;

  while (count--) {
    rv |= sParserDataListeners->ObjectAt(count)->
      OnUnicharDataAvailable(aRequest, ctx, aData);

    if (NS_FAILED(rv) && !canceled) {
      aRequest->Cancel(rv);

      canceled = PR_TRUE;
    }
  }

  return rv;
}

PRBool
nsParser::CanInterrupt()
{
  return (mFlags & NS_PARSER_FLAG_CAN_INTERRUPT) != 0;
}

PRBool
nsParser::IsInsertionPointDefined()
{
  return PR_TRUE;
}

void
nsParser::BeginEvaluatingParserInsertedScript()
{
}

void
nsParser::EndEvaluatingParserInsertedScript()
{
}

void
nsParser::MarkAsNotScriptCreated()
{
}

PRBool
nsParser::IsScriptCreated()
{
  return PR_FALSE;
}

void
nsParser::SetCanInterrupt(PRBool aCanInterrupt)
{
  if (aCanInterrupt) {
    mFlags |= NS_PARSER_FLAG_CAN_INTERRUPT;
  } else {
    mFlags &= ~NS_PARSER_FLAG_CAN_INTERRUPT;
  }
}








NS_IMETHODIMP
nsParser::Parse(nsIURI* aURL,
                nsIRequestObserver* aListener,
                void* aKey,
                nsDTDMode aMode)
{

  NS_PRECONDITION(aURL, "Error: Null URL given");
  NS_ASSERTION(!mSpeculativeScriptThread, "Can't reuse a parser like this");

  nsresult result=kBadURL;
  mObserver = aListener;

  if (aURL) {
    nsCAutoString spec;
    nsresult rv = aURL->GetSpec(spec);
    if (rv != NS_OK) {
      return rv;
    }
    NS_ConvertUTF8toUTF16 theName(spec);

    nsScanner* theScanner = new nsScanner(theName, PR_FALSE, mCharset,
                                          mCharsetSource);
    CParserContext* pc = new CParserContext(mParserContext, theScanner, aKey,
                                            mCommand, aListener);
    if (pc && theScanner) {
      pc->mMultipart = PR_TRUE;
      pc->mContextType = CParserContext::eCTURL;
      pc->mDTDMode = aMode;
      PushContext(*pc);

      
      
      
      
      theScanner->SetParser(this);

      result = NS_OK;
    } else {
      result = mInternalState = NS_ERROR_HTMLPARSER_BADCONTEXT;
    }
  }
  return result;
}









NS_IMETHODIMP
nsParser::Parse(const nsAString& aSourceBuffer,
                void* aKey,
                const nsACString& aMimeType,
                PRBool aLastCall,
                nsDTDMode aMode)
{
  nsresult result = NS_OK;

  
  if (mInternalState == NS_ERROR_HTMLPARSER_STOPPARSING) {
    return result;
  }

  if (!aLastCall && aSourceBuffer.IsEmpty()) {
    
    
    
    
    
    return result;
  }

  if (mSpeculativeScriptThread) {
    mSpeculativeScriptThread->StopParsing(PR_TRUE);
  }

  
  
  if (aMode == eDTDMode_fragment)
    mCommand = eViewFragment;

  
  
  nsCOMPtr<nsIParser> kungFuDeathGrip(this);

  if (aLastCall || !aSourceBuffer.IsEmpty() || !mUnusedInput.IsEmpty()) {
    
    
    
    
    CParserContext* pc = mParserContext;
    while (pc && pc->mKey != aKey) {
      pc = pc->mPrevContext;
    }

    if (!pc) {
      
      
      nsScanner* theScanner = new nsScanner(mUnusedInput, mCharset, mCharsetSource);
      NS_ENSURE_TRUE(theScanner, NS_ERROR_OUT_OF_MEMORY);

      eAutoDetectResult theStatus = eUnknownDetect;

      if (mParserContext && mParserContext->mMimeType == aMimeType) {
        
        NS_ASSERTION(mDTD, "How come the DTD is null?");

        if (mParserContext) {
          theStatus = mParserContext->mAutoDetectStatus;
          
        }
      }

      pc = new CParserContext(mParserContext, theScanner, aKey, mCommand,
                              0, theStatus, aLastCall);
      NS_ENSURE_TRUE(pc, NS_ERROR_OUT_OF_MEMORY);

      PushContext(*pc);

      pc->mMultipart = !aLastCall; 
      if (pc->mPrevContext) {
        pc->mMultipart |= pc->mPrevContext->mMultipart;
      }

      
      if (pc->mMultipart) {
        pc->mStreamListenerState = eOnDataAvail;
        if (pc->mScanner) {
          pc->mScanner->SetIncremental(PR_TRUE);
        }
      } else {
        pc->mStreamListenerState = eOnStop;
        if (pc->mScanner) {
          pc->mScanner->SetIncremental(PR_FALSE);
        }
      }
      

      pc->mContextType=CParserContext::eCTString;
      pc->SetMimeType(aMimeType);
      if (pc->mPrevContext && aMode == eDTDMode_autodetect) {
        
        pc->mDTDMode = pc->mPrevContext->mDTDMode;
      } else {
        pc->mDTDMode = aMode;
      }

      mUnusedInput.Truncate();

      pc->mScanner->Append(aSourceBuffer);
      
      result = ResumeParse(PR_FALSE, PR_FALSE, PR_FALSE);
    } else {
      pc->mScanner->Append(aSourceBuffer);
      if (!pc->mPrevContext) {
        
        
        if (aLastCall) {
          pc->mStreamListenerState = eOnStop;
          pc->mScanner->SetIncremental(PR_FALSE);
        }

        if (pc == mParserContext) {
          
          
          
          ResumeParse(PR_FALSE, PR_FALSE, PR_FALSE);
        }
      }
    }
  }

  return result;
}

NS_IMETHODIMP
nsParser::ParseFragment(const nsAString& aSourceBuffer,
                        nsISupports* aTargetNode,
                        nsIAtom* aContextLocalName,
                        PRInt32 aContextNamespace,
                        PRBool aQuirks)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsParser::ParseFragment(const nsAString& aSourceBuffer,
                        void* aKey,
                        nsTArray<nsString>& aTagStack,
                        PRBool aXMLMode,
                        const nsACString& aMimeType,
                        nsDTDMode aMode)
{
  nsresult result = NS_OK;
  nsAutoString  theContext;
  PRUint32 theCount = aTagStack.Length();
  PRUint32 theIndex = 0;

  
  mFlags &= ~NS_PARSER_FLAG_OBSERVERS_ENABLED;

  NS_ASSERTION(!mSpeculativeScriptThread, "Can't reuse a parser like this");

  for (theIndex = 0; theIndex < theCount; theIndex++) {
    theContext.AppendLiteral("<");
    theContext.Append(aTagStack[theCount - theIndex - 1]);
    theContext.AppendLiteral(">");
  }

  if (theCount == 0) {
    
    
    theContext.AssignLiteral(" ");
  }

  
  
  result = Parse(theContext, (void*)&theContext, aMimeType, PR_FALSE, aMode);
  if (NS_FAILED(result)) {
    mFlags |= NS_PARSER_FLAG_OBSERVERS_ENABLED;
    return result;
  }

  if (!mSink) {
    
    NS_ASSERTION(aXMLMode, "Unexpected!");
    return NS_ERROR_HTMLPARSER_STOPPARSING;
  }

  nsCOMPtr<nsIFragmentContentSink> fragSink = do_QueryInterface(mSink);
  NS_ASSERTION(fragSink, "ParseFragment requires a fragment content sink");

  if (!aXMLMode && theCount) {
    
    
    
    NS_ASSERTION(mParserContext, "Parsing didn't create a parser context?");

    CNavDTD* dtd = static_cast<CNavDTD*>
                              (static_cast<nsIDTD*>(mDTD));
    NS_ASSERTION(dtd, "How did we parse anything without a dtd?");

    CStartToken bodyToken(NS_LITERAL_STRING("BODY"), eHTMLTag_body);
    nsCParserNode bodyNode(&bodyToken, 0);

    dtd->OpenContainer(&bodyNode, eHTMLTag_body);

    
    result = BuildModel();
    if (NS_FAILED(result)) {
      mFlags |= NS_PARSER_FLAG_OBSERVERS_ENABLED;
      return result;
    }

    
    
    NS_ASSERTION(mParserContext->mScanner, "Where'd the scanner go?");

    PRUnichar next;
    if (NS_SUCCEEDED(mParserContext->mScanner->Peek(next))) {
      
      
      
      
      NS_ASSERTION(next == '<', "The tokenizer failed to consume a token");
      fragSink->IgnoreFirstContainer();
    }
  }

  fragSink->WillBuildContent();
  
  
  
  
  if (!aXMLMode || (theCount == 0)) {
    result = Parse(aSourceBuffer, &theContext, aMimeType,
                   PR_TRUE, aMode);
    fragSink->DidBuildContent();
  } else {
    
    
    result = Parse(aSourceBuffer + NS_LITERAL_STRING("</"),
                   &theContext, aMimeType, PR_FALSE, aMode);
    fragSink->DidBuildContent();

    if (NS_SUCCEEDED(result)) {
      nsAutoString endContext;
      for (theIndex = 0; theIndex < theCount; theIndex++) {
         
        if (theIndex > 0) {
          endContext.AppendLiteral("</");
        }

        nsString& thisTag = aTagStack[theIndex];
        
        PRInt32 endOfTag = thisTag.FindChar(PRUnichar(' '));
        if (endOfTag == -1) {
          endContext.Append(thisTag);
        } else {
          endContext.Append(Substring(thisTag,0,endOfTag));
        }

        endContext.AppendLiteral(">");
      }

      result = Parse(endContext, &theContext, aMimeType,
                     PR_TRUE, aMode);
    }
  }

  mFlags |= NS_PARSER_FLAG_OBSERVERS_ENABLED;

  return result;
}























nsresult
nsParser::ResumeParse(PRBool allowIteration, PRBool aIsFinalChunk,
                      PRBool aCanInterrupt)
{
  nsresult result = NS_OK;

  if ((mFlags & NS_PARSER_FLAG_PARSER_ENABLED) &&
      mInternalState != NS_ERROR_HTMLPARSER_STOPPARSING) {
    NS_ASSERTION(!mSpeculativeScriptThread || !mSpeculativeScriptThread->Parsing(),
                 "Bad races happening, expect to crash!");

    result = WillBuildModel(mParserContext->mScanner->GetFilename());
    if (NS_FAILED(result)) {
      mFlags &= ~NS_PARSER_FLAG_CAN_TOKENIZE;
      return result;
    }

    if (mDTD) {
      mSink->WillResume();
      PRBool theIterationIsOk = PR_TRUE;

      while (result == NS_OK && theIterationIsOk) {
        if (!mUnusedInput.IsEmpty() && mParserContext->mScanner) {
          
          
          
          
          mParserContext->mScanner->UngetReadable(mUnusedInput);
          mUnusedInput.Truncate(0);
        }

        
        
        SetCanInterrupt(aCanInterrupt);
        nsresult theTokenizerResult = (mFlags & NS_PARSER_FLAG_CAN_TOKENIZE)
                                      ? Tokenize(aIsFinalChunk)
                                      : NS_OK;
        result = BuildModel();

        if (result == NS_ERROR_HTMLPARSER_INTERRUPTED && aIsFinalChunk) {
          PostContinueEvent();
        }
        SetCanInterrupt(PR_FALSE);

        theIterationIsOk = theTokenizerResult != kEOF &&
                           result != NS_ERROR_HTMLPARSER_INTERRUPTED;

        
        
        
        

        
        
        if (NS_ERROR_HTMLPARSER_BLOCK == result) {
          mSink->WillInterrupt();
          if (mFlags & NS_PARSER_FLAG_PARSER_ENABLED) {
            
            BlockParser();
            SpeculativelyParse();
          }
          return NS_OK;
        }
        if (NS_ERROR_HTMLPARSER_STOPPARSING == result) {
          
          if (mInternalState != NS_ERROR_HTMLPARSER_STOPPARSING) {
            DidBuildModel(mStreamStatus);
            mInternalState = result;
          }

          return NS_OK;
        }
        if ((NS_OK == result && theTokenizerResult == kEOF) ||
             result == NS_ERROR_HTMLPARSER_INTERRUPTED) {
          PRBool theContextIsStringBased =
            CParserContext::eCTString == mParserContext->mContextType;

          if (mParserContext->mStreamListenerState == eOnStop ||
              !mParserContext->mMultipart || theContextIsStringBased) {
            if (!mParserContext->mPrevContext) {
              if (mParserContext->mStreamListenerState == eOnStop) {
                DidBuildModel(mStreamStatus);
                return NS_OK;
              }
            } else {
              CParserContext* theContext = PopContext();
              if (theContext) {
                theIterationIsOk = allowIteration && theContextIsStringBased;
                if (theContext->mCopyUnused) {
                  theContext->mScanner->CopyUnusedData(mUnusedInput);
                }

                delete theContext;
              }

              result = mInternalState;
              aIsFinalChunk = mParserContext &&
                              mParserContext->mStreamListenerState == eOnStop;
              
            }
          }
        }

        if (theTokenizerResult == kEOF ||
            result == NS_ERROR_HTMLPARSER_INTERRUPTED) {
          result = (result == NS_ERROR_HTMLPARSER_INTERRUPTED) ? NS_OK : result;
          mSink->WillInterrupt();
        }
      }
    } else {
      mInternalState = result = NS_ERROR_HTMLPARSER_UNRESOLVEDDTD;
    }
  }

  return (result == NS_ERROR_HTMLPARSER_INTERRUPTED) ? NS_OK : result;
}





nsresult
nsParser::BuildModel()
{
  nsITokenizer* theTokenizer = nsnull;

  nsresult result = NS_OK;
  if (mParserContext) {
    result = mParserContext->GetTokenizer(mDTD, mSink, theTokenizer);
  }

  if (NS_SUCCEEDED(result)) {
    if (mDTD) {
      
      PRBool inDocWrite = !!mParserContext->mPrevContext;
      result = mDTD->BuildModel(theTokenizer,
                                
                                CanInterrupt() && !inDocWrite,
                                !inDocWrite, 
                                &mCharset);
    }
  } else {
    mInternalState = result = NS_ERROR_HTMLPARSER_BADTOKENIZER;
  }
  return result;
}





nsresult
nsParser::OnStartRequest(nsIRequest *request, nsISupports* aContext)
{
  NS_PRECONDITION(eNone == mParserContext->mStreamListenerState,
                  "Parser's nsIStreamListener API was not setup "
                  "correctly in constructor.");
  if (mObserver) {
    mObserver->OnStartRequest(request, aContext);
  }
  mParserContext->mStreamListenerState = eOnStart;
  mParserContext->mAutoDetectStatus = eUnknownDetect;
  mParserContext->mRequest = request;

  NS_ASSERTION(!mParserContext->mPrevContext,
               "Clobbering DTD for non-root parser context!");
  mDTD = nsnull;

  nsresult rv;
  nsCAutoString contentType;
  nsCOMPtr<nsIChannel> channel = do_QueryInterface(request);
  if (channel) {
    rv = channel->GetContentType(contentType);
    if (NS_SUCCEEDED(rv)) {
      mParserContext->SetMimeType(contentType);
    }
  }

  rv = NS_OK;

  if (sParserDataListeners && mSink) {
    nsISupports *ctx = mSink->GetTarget();
    PRInt32 count = sParserDataListeners->Count();

    while (count--) {
      rv |= sParserDataListeners->ObjectAt(count)->
              OnStartRequest(request, ctx);
    }
  }

  return rv;
}


#define UTF16_BOM "UTF-16"
#define UTF16_BE "UTF-16BE"
#define UTF16_LE "UTF-16LE"
#define UCS4_BOM "UTF-32"
#define UCS4_BE "UTF-32BE"
#define UCS4_LE "UTF-32LE"
#define UCS4_2143 "X-ISO-10646-UCS-4-2143"
#define UCS4_3412 "X-ISO-10646-UCS-4-3412"
#define UTF8 "UTF-8"

static inline PRBool IsSecondMarker(unsigned char aChar)
{
  switch (aChar) {
    case '!':
    case '?':
    case 'h':
    case 'H':
      return PR_TRUE;
    default:
      return PR_FALSE;
  }
}

static PRBool
DetectByteOrderMark(const unsigned char* aBytes, PRInt32 aLen,
                    nsCString& oCharset, PRInt32& oCharsetSource)
{
 oCharsetSource= kCharsetFromAutoDetection;
 oCharset.Truncate();
 
 
 
 
 
 switch(aBytes[0])
	 {
   case 0x00:
     if(0x00==aBytes[1]) {
        
        if((0xFE==aBytes[2]) && (0xFF==aBytes[3])) {
           
           oCharset.Assign(UCS4_BOM);
        } else if((0x00==aBytes[2]) && (0x3C==aBytes[3])) {
           
           oCharset.Assign(UCS4_BE);
        } else if((0xFF==aBytes[2]) && (0xFE==aBytes[3])) {
           
           oCharset.Assign(UCS4_2143);
        } else if((0x3C==aBytes[2]) && (0x00==aBytes[3])) {
           
           oCharset.Assign(UCS4_2143);
        } 
        oCharsetSource = kCharsetFromByteOrderMark;
     } else if((0x3C==aBytes[1]) && (0x00==aBytes[2])) {
        
        if(IsSecondMarker(aBytes[3])) {
           
           oCharset.Assign(UTF16_BE); 
        } else if((0x00==aBytes[3])) {
           
           oCharset.Assign(UCS4_3412);
        } 
        oCharsetSource = kCharsetFromByteOrderMark;
     }
   break;
   case 0x3C:
     if(0x00==aBytes[1] && (0x00==aBytes[3])) {
        
        if(IsSecondMarker(aBytes[2])) {
           
           oCharset.Assign(UTF16_LE); 
        } else if((0x00==aBytes[2])) {
           
           oCharset.Assign(UCS4_LE); 
        } 
        oCharsetSource = kCharsetFromByteOrderMark;
     
     
     } else if(                     (0x3F==aBytes[1]) &&
               (0x78==aBytes[2]) && (0x6D==aBytes[3]) &&
               (0 == PL_strncmp("<?xml", (char*)aBytes, 5 ))) {
       
       
       
       
       
       PRInt32 i;
       PRBool versionFound = PR_FALSE, encodingFound = PR_FALSE;
       for (i=6; i < aLen && !encodingFound; ++i) {
         
         if ((((char*)aBytes)[i] == '?') && 
           ((i+1) < aLen) &&
           (((char*)aBytes)[i+1] == '>')) {
           break;
         }
         
         if (!versionFound) {
           
           
           
           
           
           if ((((char*)aBytes)[i] == 'n') &&
             (i >= 12) && 
             (0 == PL_strncmp("versio", (char*)(aBytes+i-6), 6 ))) {
             
             char q = 0;
             for (++i; i < aLen; ++i) {
               char qi = ((char*)aBytes)[i];
               if (qi == '\'' || qi == '"') {
                 if (q && q == qi) {
                   
                   versionFound = PR_TRUE;
                   break;
                 } else {
                   
                   q = qi;
                 }
               }
             }
           }
         } else {
           
           
           
           
           
           
           if ((((char*)aBytes)[i] == 'g') &&
             (i >= 25) && 
             (0 == PL_strncmp("encodin", (char*)(aBytes+i-7), 7 ))) {
             PRInt32 encStart = 0;
             char q = 0;
             for (++i; i < aLen; ++i) {
               char qi = ((char*)aBytes)[i];
               if (qi == '\'' || qi == '"') {
                 if (q && q == qi) {
                   PRInt32 count = i - encStart;
                   
                   if (count > 0 && 
                     (0 != PL_strcmp("UTF-16", (char*)(aBytes+encStart)))) {
                     oCharset.Assign((char*)(aBytes+encStart),count);
                     oCharsetSource = kCharsetFromMetaTag;
                   }
                   encodingFound = PR_TRUE;
                   break;
                 } else {
                   encStart = i+1;
                   q = qi;
                 }
               }
             }
           }
         } 
       } 
     }
   break;
   case 0xEF:  
     if((0xBB==aBytes[1]) && (0xBF==aBytes[2])) {
        
        
        oCharset.Assign(UTF8); 
        oCharsetSource= kCharsetFromByteOrderMark;
     }
   break;
   case 0xFE:
     if(0xFF==aBytes[1]) {
        if(0x00==aBytes[2] && 0x00==aBytes[3]) {
          
          oCharset.Assign(UCS4_3412);
        } else {
          
          oCharset.Assign(UTF16_BOM); 
        }
        oCharsetSource= kCharsetFromByteOrderMark;
     }
   break;
   case 0xFF:
     if(0xFE==aBytes[1]) {
        if(0x00==aBytes[2] && 0x00==aBytes[3]) 
         
           oCharset.Assign(UCS4_BOM); 
        else
        
        
           oCharset.Assign(UTF16_BOM); 
        oCharsetSource= kCharsetFromByteOrderMark;
     }
   break;
   
   
   
   
 }  
 return !oCharset.IsEmpty();
}

inline const char
GetNextChar(nsACString::const_iterator& aStart,
            nsACString::const_iterator& aEnd)
{
  NS_ASSERTION(aStart != aEnd, "end of buffer");
  return (++aStart != aEnd) ? *aStart : '\0';
}

PRBool
nsParser::DetectMetaTag(const char* aBytes,
                        PRInt32 aLen,
                        nsCString& aCharset,
                        PRInt32& aCharsetSource)
{
  aCharsetSource= kCharsetFromMetaTag;
  aCharset.SetLength(0);

  
  
  if (!mParserContext->mMimeType.EqualsLiteral(kHTMLTextContentType)) {
    return PR_FALSE;
  }

  
  
  const nsASingleFragmentCString& str =
      Substring(aBytes, aBytes + PR_MIN(aLen, 2048));
  
  nsACString::const_iterator begin, end;

  str.BeginReading(begin);
  str.EndReading(end);
  nsACString::const_iterator currPos(begin);
  nsACString::const_iterator tokEnd;
  nsACString::const_iterator tagEnd(begin);

  while (currPos != end) {
    if (!FindCharInReadable('<', currPos, end))
      break; 

    if (GetNextChar(currPos, end) == '!') {
      if (GetNextChar(currPos, end) != '-' ||
          GetNextChar(currPos, end) != '-') {
        
        if (!FindCharInReadable('>', currPos, end)) {
          return PR_FALSE; 
        }

        
        ++currPos;
        continue;
      }

      
      PRBool foundMDC = PR_FALSE;
      PRBool foundMatch = PR_FALSE;
      while (!foundMDC) {
        if (GetNextChar(currPos, end) == '-' &&
            GetNextChar(currPos, end) == '-') {
          foundMatch = !foundMatch; 
        } else if (currPos == end) {
          return PR_FALSE; 
        } else if (foundMatch && *currPos == '>') {
          foundMDC = PR_TRUE; 
          ++currPos;
        }
      }
      continue; 
    }

    
    tagEnd = currPos;
    if (!FindCharInReadable('>', tagEnd, end))
      break;

    
    if ( (*currPos != 'm' && *currPos != 'M') ||
         (*(++currPos) != 'e' && *currPos != 'E') ||
         (*(++currPos) != 't' && *currPos != 'T') ||
         (*(++currPos) != 'a' && *currPos != 'A') ||
         !nsCRT::IsAsciiSpace(*(++currPos))) {
      currPos = tagEnd;
      continue;
    }

    
    tokEnd = tagEnd;
    if (!CaseInsensitiveFindInReadable(NS_LITERAL_CSTRING("CHARSET"),
                                       currPos, tokEnd)) {
      currPos = tagEnd;
      continue;
    }
    currPos = tokEnd;

    
    while (*currPos == kSpace || *currPos == kNewLine ||
           *currPos == kCR || *currPos == kTab) {
      ++currPos;
    }
    
    if (*currPos != '=') {
      currPos = tagEnd;
      continue;
    }
    ++currPos;
    
    while (*currPos == kSpace || *currPos == kNewLine ||
           *currPos == kCR || *currPos == kTab) {
      ++currPos;
    }

    
    if (*currPos == '\'' || *currPos == '\"')
      ++currPos;

    
    tokEnd = currPos;
    while (*tokEnd != '\'' && *tokEnd != '\"' && tokEnd != tagEnd)
      ++tokEnd;

    
    if (currPos != tokEnd) {
      aCharset.Assign(currPos.get(), tokEnd.get() - currPos.get());
      return PR_TRUE;
    }

    
    currPos = tagEnd;
  }

  return PR_FALSE;
}

typedef struct {
  PRBool mNeedCharsetCheck;
  nsParser* mParser;
  nsIParserFilter* mParserFilter;
  nsScanner* mScanner;
  nsIRequest* mRequest;
} ParserWriteStruct;







static NS_METHOD
ParserWriteFunc(nsIInputStream* in,
                void* closure,
                const char* fromRawSegment,
                PRUint32 toOffset,
                PRUint32 count,
                PRUint32 *writeCount)
{
  nsresult result;
  ParserWriteStruct* pws = static_cast<ParserWriteStruct*>(closure);
  const char* buf = fromRawSegment;
  PRUint32 theNumRead = count;

  if (!pws) {
    return NS_ERROR_FAILURE;
  }

  if (pws->mNeedCharsetCheck) {
    PRInt32 guessSource;
    nsCAutoString guess;
    nsCAutoString preferred;

    pws->mNeedCharsetCheck = PR_FALSE;
    if (pws->mParser->DetectMetaTag(buf, theNumRead, guess, guessSource) ||
        ((count >= 4) &&
         DetectByteOrderMark((const unsigned char*)buf,
                             theNumRead, guess, guessSource))) {
      nsCOMPtr<nsICharsetAlias> alias(do_GetService(NS_CHARSETALIAS_CONTRACTID));
      result = alias->GetPreferred(guess, preferred);
      
      
      if (NS_SUCCEEDED(result) &&
          ((kCharsetFromByteOrderMark == guessSource) ||
           (!preferred.EqualsLiteral("UTF-16") &&
            !preferred.EqualsLiteral("UTF-16BE") &&
            !preferred.EqualsLiteral("UTF-16LE") &&
            !preferred.EqualsLiteral("UTF-32") &&
            !preferred.EqualsLiteral("UTF-32BE") &&
            !preferred.EqualsLiteral("UTF-32LE")))) {
        guess = preferred;
        pws->mParser->SetDocumentCharset(guess, guessSource);
        pws->mParser->SetSinkCharset(preferred);
        nsCOMPtr<nsICachingChannel> channel(do_QueryInterface(pws->mRequest));
        if (channel) {
          nsCOMPtr<nsISupports> cacheToken;
          channel->GetCacheToken(getter_AddRefs(cacheToken));
          if (cacheToken) {
            nsCOMPtr<nsICacheEntryDescriptor> cacheDescriptor(do_QueryInterface(cacheToken));
            if (cacheDescriptor) {
#ifdef DEBUG
              nsresult rv =
#endif
                cacheDescriptor->SetMetaDataElement("charset",
                                                    guess.get());
              NS_ASSERTION(NS_SUCCEEDED(rv),"cannot SetMetaDataElement");
            }
          }
        }
      }
    }
  }

  if (pws->mParserFilter)
    pws->mParserFilter->RawBuffer(buf, &theNumRead);

  result = pws->mScanner->Append(buf, theNumRead, pws->mRequest);
  if (NS_SUCCEEDED(result)) {
    *writeCount = count;
  }

  return result;
}

nsresult
nsParser::OnDataAvailable(nsIRequest *request, nsISupports* aContext,
                          nsIInputStream *pIStream, PRUint32 sourceOffset,
                          PRUint32 aLength)
{
  NS_PRECONDITION((eOnStart == mParserContext->mStreamListenerState ||
                   eOnDataAvail == mParserContext->mStreamListenerState),
            "Error: OnStartRequest() must be called before OnDataAvailable()");
  NS_PRECONDITION(NS_InputStreamIsBuffered(pIStream),
                  "Must have a buffered input stream");

  nsresult rv = NS_OK;

  CParserContext *theContext = mParserContext;

  while (theContext && theContext->mRequest != request) {
    theContext = theContext->mPrevContext;
  }

  if (theContext) {
    theContext->mStreamListenerState = eOnDataAvail;

    if ((mFlags & NS_PARSER_FLAG_PARSER_ENABLED) &&
        mSpeculativeScriptThread) {
      mSpeculativeScriptThread->StopParsing(PR_FALSE);
    }

    if (eInvalidDetect == theContext->mAutoDetectStatus) {
      if (theContext->mScanner) {
        nsScannerIterator iter;
        theContext->mScanner->EndReading(iter);
        theContext->mScanner->SetPosition(iter, PR_TRUE);
      }
    }

    PRUint32 totalRead;
    ParserWriteStruct pws;
    pws.mNeedCharsetCheck =
      (0 == sourceOffset) && (mCharsetSource < kCharsetFromMetaTag);
    pws.mParser = this;
    pws.mParserFilter = mParserFilter;
    pws.mScanner = theContext->mScanner;
    pws.mRequest = request;

    rv = pIStream->ReadSegments(ParserWriteFunc, &pws, aLength, &totalRead);
    if (NS_FAILED(rv)) {
      return rv;
    }

    
    
    if (IsOkToProcessNetworkData() &&
        theContext->mScanner->FirstNonWhitespacePosition() >= 0) {
      mProcessingNetworkData = PR_TRUE;
      if (mSink) {
        mSink->WillParse();
      }
      rv = ResumeParse();
      mProcessingNetworkData = PR_FALSE;
    }
  } else {
    rv = NS_ERROR_UNEXPECTED;
  }

  return rv;
}





nsresult
nsParser::OnStopRequest(nsIRequest *request, nsISupports* aContext,
                        nsresult status)
{
  nsresult rv = NS_OK;

  if ((mFlags & NS_PARSER_FLAG_PARSER_ENABLED) &&
      mSpeculativeScriptThread) {
    mSpeculativeScriptThread->StopParsing(PR_FALSE);
  }

  CParserContext *pc = mParserContext;
  while (pc) {
    if (pc->mRequest == request) {
      pc->mStreamListenerState = eOnStop;
      pc->mScanner->SetIncremental(PR_FALSE);
      break;
    }

    pc = pc->mPrevContext;
  }

  mStreamStatus = status;

  if (mParserFilter)
    mParserFilter->Finish();

  if (IsOkToProcessNetworkData() && NS_SUCCEEDED(rv)) {
    mProcessingNetworkData = PR_TRUE;
    if (mSink) {
      mSink->WillParse();
    }
    rv = ResumeParse(PR_TRUE, PR_TRUE);
    mProcessingNetworkData = PR_FALSE;
  }

  
  


  
  
  if (mObserver) {
    mObserver->OnStopRequest(request, aContext, status);
  }

  if (sParserDataListeners && mSink) {
    nsISupports *ctx = mSink->GetTarget();
    PRInt32 count = sParserDataListeners->Count();

    while (count--) {
      rv |= sParserDataListeners->ObjectAt(count)->OnStopRequest(request, ctx,
                                                                 status);
    }
  }

  return rv;
}












PRBool
nsParser::WillTokenize(PRBool aIsFinalChunk)
{
  if (!mParserContext) {
    return PR_TRUE;
  }

  nsITokenizer* theTokenizer;
  nsresult result = mParserContext->GetTokenizer(mDTD, mSink, theTokenizer);
  NS_ENSURE_SUCCESS(result, PR_FALSE);
  return NS_SUCCEEDED(theTokenizer->WillTokenize(aIsFinalChunk,
                                                 &mTokenAllocator));
}







nsresult nsParser::Tokenize(PRBool aIsFinalChunk)
{
  nsITokenizer* theTokenizer;

  nsresult result = NS_ERROR_NOT_AVAILABLE;
  if (mParserContext) {
    result = mParserContext->GetTokenizer(mDTD, mSink, theTokenizer);
  }

  if (NS_SUCCEEDED(result)) {
    if (mFlags & NS_PARSER_FLAG_FLUSH_TOKENS) {
      
      
      
      if (theTokenizer->GetCount() != 0) {
        return result;
      }

      
      mFlags &= ~NS_PARSER_FLAG_FLUSH_TOKENS;
    }

    PRBool flushTokens = PR_FALSE;

    mParserContext->mNumConsumed = 0;

    PRBool killSink = PR_FALSE;

    WillTokenize(aIsFinalChunk);
    while (NS_SUCCEEDED(result)) {
      mParserContext->mNumConsumed += mParserContext->mScanner->Mark();
      result = theTokenizer->ConsumeToken(*mParserContext->mScanner,
                                          flushTokens);
      if (NS_FAILED(result)) {
        mParserContext->mScanner->RewindToMark();
        if (kEOF == result){
          break;
        }
        if (NS_ERROR_HTMLPARSER_STOPPARSING == result) {
          killSink = PR_TRUE;
          result = Terminate();
          break;
        }
      } else if (flushTokens && (mFlags & NS_PARSER_FLAG_OBSERVERS_ENABLED)) {
        
        
        
        mFlags |= NS_PARSER_FLAG_FLUSH_TOKENS;
        mParserContext->mNumConsumed += mParserContext->mScanner->Mark();
        break;
      }
    }
    DidTokenize(aIsFinalChunk);

    if (killSink) {
      mSink = nsnull;
    }
  } else {
    result = mInternalState = NS_ERROR_HTMLPARSER_BADTOKENIZER;
  }

  return result;
}






PRBool
nsParser::DidTokenize(PRBool aIsFinalChunk)
{
  if (!mParserContext) {
    return PR_TRUE;
  }

  nsITokenizer* theTokenizer;
  nsresult rv = mParserContext->GetTokenizer(mDTD, mSink, theTokenizer);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  rv = theTokenizer->DidTokenize(aIsFinalChunk);
  return NS_SUCCEEDED(rv);
}







NS_IMETHODIMP
nsParser::GetChannel(nsIChannel** aChannel)
{
  nsresult result = NS_ERROR_NOT_AVAILABLE;
  if (mParserContext && mParserContext->mRequest) {
    result = CallQueryInterface(mParserContext->mRequest, aChannel);
  }
  return result;
}




NS_IMETHODIMP
nsParser::GetDTD(nsIDTD** aDTD)
{
  if (mParserContext) {
    NS_IF_ADDREF(*aDTD = mDTD);
  }

  return NS_OK;
}




NS_IMETHODIMP
nsParser::GetStreamListener(nsIStreamListener** aListener)
{
  NS_ADDREF(*aListener = this);
  return NS_OK;
}
