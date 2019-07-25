







































#include "nsHtml5StreamParser.h"
#include "nsICharsetConverterManager.h"
#include "nsICharsetAlias.h"
#include "nsServiceManagerUtils.h"
#include "nsEncoderDecoderUtils.h"
#include "nsContentUtils.h"
#include "nsHtml5Tokenizer.h"
#include "nsIHttpChannel.h"
#include "nsHtml5Parser.h"
#include "nsHtml5TreeBuilder.h"
#include "nsHtml5AtomTable.h"
#include "nsHtml5Module.h"
#include "nsHtml5RefPtr.h"
#include "nsIScriptError.h"
#include "mozilla/Preferences.h"

using namespace mozilla;

static NS_DEFINE_CID(kCharsetAliasCID, NS_CHARSETALIAS_CID);

PRInt32 nsHtml5StreamParser::sTimerInitialDelay = 120;
PRInt32 nsHtml5StreamParser::sTimerSubsequentDelay = 120;


void
nsHtml5StreamParser::InitializeStatics()
{
  nsContentUtils::AddIntPrefVarCache("html5.flushtimer.initialdelay",
                                     &sTimerInitialDelay);
  nsContentUtils::AddIntPrefVarCache("html5.flushtimer.subsequentdelay",
                                     &sTimerSubsequentDelay);
}

























NS_IMPL_CYCLE_COLLECTING_ADDREF(nsHtml5StreamParser)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsHtml5StreamParser)

NS_INTERFACE_TABLE_HEAD(nsHtml5StreamParser)
  NS_INTERFACE_TABLE2(nsHtml5StreamParser, 
                      nsIStreamListener, 
                      nsICharsetDetectionObserver)
  NS_INTERFACE_TABLE_TO_MAP_SEGUE_CYCLE_COLLECTION(nsHtml5StreamParser)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTION_CLASS(nsHtml5StreamParser)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsHtml5StreamParser)
  tmp->DropTimer();
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mObserver)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mRequest)
  tmp->mOwner = nsnull;
  tmp->mExecutorFlusher = nsnull;
  tmp->mLoadFlusher = nsnull;
  tmp->mExecutor = nsnull;
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mChardet)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsHtml5StreamParser)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mObserver)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mRequest)
  if (tmp->mOwner) {
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mOwner");
    cb.NoteXPCOMChild(static_cast<nsIParser*> (tmp->mOwner));
  }
  
  if (tmp->mExecutorFlusher) {
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mExecutorFlusher->mExecutor");
    cb.NoteXPCOMChild(static_cast<nsIContentSink*> (tmp->mExecutor));
  }
  
  if (tmp->mLoadFlusher) {
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mLoadFlusher->mExecutor");
    cb.NoteXPCOMChild(static_cast<nsIContentSink*> (tmp->mExecutor));
  }
  
  if (tmp->mChardet) {
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, 
      "mChardet->mObserver");
    cb.NoteXPCOMChild(static_cast<nsIStreamListener*>(tmp));
  }
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

class nsHtml5ExecutorFlusher : public nsRunnable
{
  private:
    nsRefPtr<nsHtml5TreeOpExecutor> mExecutor;
  public:
    nsHtml5ExecutorFlusher(nsHtml5TreeOpExecutor* aExecutor)
      : mExecutor(aExecutor)
    {}
    NS_IMETHODIMP Run()
    {
      mExecutor->RunFlushLoop();
      return NS_OK;
    }
};

class nsHtml5LoadFlusher : public nsRunnable
{
  private:
    nsRefPtr<nsHtml5TreeOpExecutor> mExecutor;
  public:
    nsHtml5LoadFlusher(nsHtml5TreeOpExecutor* aExecutor)
      : mExecutor(aExecutor)
    {}
    NS_IMETHODIMP Run()
    {
      mExecutor->FlushSpeculativeLoads();
      return NS_OK;
    }
};

nsHtml5StreamParser::nsHtml5StreamParser(nsHtml5TreeOpExecutor* aExecutor,
                                         nsHtml5Parser* aOwner)
  : mFirstBuffer(new nsHtml5UTF16Buffer(NS_HTML5_STREAM_PARSER_READ_BUFFER_SIZE))
  , mLastBuffer(mFirstBuffer)
  , mExecutor(aExecutor)
  , mTreeBuilder(new nsHtml5TreeBuilder(mExecutor->GetStage(),
                                        mExecutor->GetStage()))
  , mTokenizer(new nsHtml5Tokenizer(mTreeBuilder))
  , mTokenizerMutex("nsHtml5StreamParser mTokenizerMutex")
  , mOwner(aOwner)
  , mSpeculationMutex("nsHtml5StreamParser mSpeculationMutex")
  , mTerminatedMutex("nsHtml5StreamParser mTerminatedMutex")
  , mThread(nsHtml5Module::GetStreamParserThread())
  , mExecutorFlusher(new nsHtml5ExecutorFlusher(aExecutor))
  , mLoadFlusher(new nsHtml5LoadFlusher(aExecutor))
  , mFlushTimer(do_CreateInstance("@mozilla.org/timer;1"))
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  mFlushTimer->SetTarget(mThread);
  mAtomTable.Init(); 
#ifdef DEBUG
  mAtomTable.SetPermittedLookupThread(mThread);
#endif
  mTokenizer->setInterner(&mAtomTable);
  mTokenizer->setEncodingDeclarationHandler(this);

  
  
  
  
  const nsAdoptingCString& detectorName =
    Preferences::GetLocalizedCString("intl.charset.detector");
  if (!detectorName.IsEmpty()) {
    nsCAutoString detectorContractID;
    detectorContractID.AssignLiteral(NS_CHARSET_DETECTOR_CONTRACTID_BASE);
    detectorContractID += detectorName;
    if ((mChardet = do_CreateInstance(detectorContractID.get()))) {
      (void) mChardet->Init(this);
      mFeedChardet = PR_TRUE;
    }
  }

  
}

nsHtml5StreamParser::~nsHtml5StreamParser()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  mTokenizer->end();
  NS_ASSERTION(!mFlushTimer, "Flush timer was not dropped before dtor!");
#ifdef DEBUG
  mRequest = nsnull;
  mObserver = nsnull;
  mUnicodeDecoder = nsnull;
  mSniffingBuffer = nsnull;
  mMetaScanner = nsnull;
  mFirstBuffer = nsnull;
  mExecutor = nsnull;
  mTreeBuilder = nsnull;
  mTokenizer = nsnull;
  mOwner = nsnull;
#endif
}

nsresult
nsHtml5StreamParser::GetChannel(nsIChannel** aChannel)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  return mRequest ? CallQueryInterface(mRequest, aChannel) :
                    NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
nsHtml5StreamParser::Notify(const char* aCharset, nsDetectionConfident aConf)
{
  NS_ASSERTION(IsParserThread(), "Wrong thread!");
  if (aConf == eBestAnswer || aConf == eSureAnswer) {
    mFeedChardet = PR_FALSE; 
    if (HasDecoder()) {
      if (mCharset.Equals(aCharset)) {
        NS_ASSERTION(mCharsetSource < kCharsetFromAutoDetection,
            "Why are we running chardet at all?");
        mCharsetSource = kCharsetFromAutoDetection;
        mTreeBuilder->SetDocumentCharset(mCharset, mCharsetSource);
      } else {
        
        
        nsCAutoString charset(aCharset);
        mTreeBuilder->NeedsCharsetSwitchTo(charset, kCharsetFromAutoDetection);
        FlushTreeOpsAndDisarmTimer();
        Interrupt();
      }
    } else {
      
      
      mCharset.Assign(aCharset);
      mCharsetSource = kCharsetFromAutoDetection;
      mTreeBuilder->SetDocumentCharset(mCharset, mCharsetSource);
    }
  }
  return NS_OK;
}

nsresult
nsHtml5StreamParser::SetupDecodingAndWriteSniffingBufferAndCurrentSegment(const PRUint8* aFromSegment, 
                                                                          PRUint32 aCount,
                                                                          PRUint32* aWriteCount)
{
  NS_ASSERTION(IsParserThread(), "Wrong thread!");
  nsresult rv = NS_OK;
  nsCOMPtr<nsICharsetConverterManager> convManager = do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = convManager->GetUnicodeDecoder(mCharset.get(), getter_AddRefs(mUnicodeDecoder));
  if (rv == NS_ERROR_UCONV_NOCONV) {
    mCharset.AssignLiteral("windows-1252"); 
    mCharsetSource = kCharsetFromWeakDocTypeDefault;
    rv = convManager->GetUnicodeDecoderRaw(mCharset.get(), getter_AddRefs(mUnicodeDecoder));
    mTreeBuilder->SetDocumentCharset(mCharset, mCharsetSource);
  }
  NS_ENSURE_SUCCESS(rv, rv);
  mUnicodeDecoder->SetInputErrorBehavior(nsIUnicodeDecoder::kOnError_Recover);
  return WriteSniffingBufferAndCurrentSegment(aFromSegment, aCount, aWriteCount);
}

nsresult
nsHtml5StreamParser::WriteSniffingBufferAndCurrentSegment(const PRUint8* aFromSegment, 
                                                          PRUint32 aCount,
                                                          PRUint32* aWriteCount)
{
  NS_ASSERTION(IsParserThread(), "Wrong thread!");
  nsresult rv = NS_OK;
  if (mSniffingBuffer) {
    PRUint32 writeCount;
    rv = WriteStreamBytes(mSniffingBuffer, mSniffingLength, &writeCount);
    NS_ENSURE_SUCCESS(rv, rv);
    mSniffingBuffer = nsnull;
  }
  mMetaScanner = nsnull;
  if (aFromSegment) {
    rv = WriteStreamBytes(aFromSegment, aCount, aWriteCount);
  }
  return rv;
}

nsresult
nsHtml5StreamParser::SetupDecodingFromBom(const char* aCharsetName, const char* aDecoderCharsetName)
{
  NS_ASSERTION(IsParserThread(), "Wrong thread!");
  nsresult rv = NS_OK;
  nsCOMPtr<nsICharsetConverterManager> convManager = do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = convManager->GetUnicodeDecoderRaw(aDecoderCharsetName, getter_AddRefs(mUnicodeDecoder));
  NS_ENSURE_SUCCESS(rv, rv);
  mUnicodeDecoder->SetInputErrorBehavior(nsIUnicodeDecoder::kOnError_Recover);
  mCharset.Assign(aCharsetName);
  mCharsetSource = kCharsetFromByteOrderMark;
  mFeedChardet = PR_FALSE;
  mTreeBuilder->SetDocumentCharset(mCharset, mCharsetSource);
  mSniffingBuffer = nsnull;
  mMetaScanner = nsnull;
  mBomState = BOM_SNIFFING_OVER;
  return rv;
}

void
nsHtml5StreamParser::SniffBOMlessUTF16BasicLatin(const PRUint8* aFromSegment,
                                                 PRUint32 aCountToSniffingLimit)
{
  
  if (mSniffingLength + aCountToSniffingLimit < 30) {
    return;
  }
  
  PRBool byteZero[2] = { PR_FALSE, PR_FALSE };
  PRBool byteNonZero[2] = { PR_FALSE, PR_FALSE };
  PRUint32 i = 0;
  if (mSniffingBuffer) {
    for (; i < mSniffingLength; ++i) {
      if (mSniffingBuffer[i]) {
        if (byteNonZero[1 - (i % 2)]) {
          return;
        }
        byteNonZero[i % 2] = PR_TRUE;
      } else {
        if (byteZero[1 - (i % 2)]) {
          return;
        }
        byteZero[i % 2] = PR_TRUE;
      }
    }
  }
  if (aFromSegment) {
    for (PRUint32 j = 0; j < aCountToSniffingLimit; ++j) {
      if (aFromSegment[j]) {
        if (byteNonZero[1 - ((i + j) % 2)]) {
          return;
        }
        byteNonZero[(i + j) % 2] = PR_TRUE;
      } else {
        if (byteZero[1 - ((i + j) % 2)]) {
          return;
        }
        byteZero[(i + j) % 2] = PR_TRUE;
      }
    }
  }

  if (byteNonZero[0]) {
    mCharset.Assign("UTF-16LE");
  } else {
    mCharset.Assign("UTF-16BE");
  }
  mCharsetSource = kCharsetFromIrreversibleAutoDetection;
  mTreeBuilder->SetDocumentCharset(mCharset, mCharsetSource);
  mFeedChardet = PR_FALSE;
}

nsresult
nsHtml5StreamParser::FinalizeSniffing(const PRUint8* aFromSegment, 
                                      PRUint32 aCount,
                                      PRUint32* aWriteCount,
                                      PRUint32 aCountToSniffingLimit)
{
  NS_ASSERTION(IsParserThread(), "Wrong thread!");
  
  if (mCharsetSource >= kCharsetFromHintPrevDoc) {
    mFeedChardet = PR_FALSE;
    return SetupDecodingAndWriteSniffingBufferAndCurrentSegment(aFromSegment, aCount, aWriteCount);
  }
  
  
  SniffBOMlessUTF16BasicLatin(aFromSegment, aCountToSniffingLimit);
  
  
  if (mFeedChardet) {
    PRBool dontFeed;
    nsresult rv;
    if (mSniffingBuffer) {
      rv = mChardet->DoIt((const char*)mSniffingBuffer.get(), mSniffingLength, &dontFeed);
      mFeedChardet = !dontFeed;
      NS_ENSURE_SUCCESS(rv, rv);
    }
    if (mFeedChardet && aFromSegment) {
      rv = mChardet->DoIt((const char*)aFromSegment,
                          
                          
                          
                          
                          
                          
                          mReparseForbidden ? aCountToSniffingLimit : aCount,
                          &dontFeed);
      mFeedChardet = !dontFeed;
      NS_ENSURE_SUCCESS(rv, rv);
    }
    if (mFeedChardet && (!aFromSegment || mReparseForbidden)) {
      
      
      
      mFeedChardet = PR_FALSE;
      rv = mChardet->Done();
      NS_ENSURE_SUCCESS(rv, rv);
    }
    
  }
  if (mCharsetSource == kCharsetUninitialized) {
    
    mCharset.AssignLiteral("windows-1252");
    mCharsetSource = kCharsetFromWeakDocTypeDefault;
    mTreeBuilder->SetDocumentCharset(mCharset, mCharsetSource);
  }
  return SetupDecodingAndWriteSniffingBufferAndCurrentSegment(aFromSegment, aCount, aWriteCount);
}

nsresult
nsHtml5StreamParser::SniffStreamBytes(const PRUint8* aFromSegment,
                                      PRUint32 aCount,
                                      PRUint32* aWriteCount)
{
  NS_ASSERTION(IsParserThread(), "Wrong thread!");
  nsresult rv = NS_OK;
  PRUint32 writeCount;
  for (PRUint32 i = 0; i < aCount && mBomState != BOM_SNIFFING_OVER; i++) {
    switch (mBomState) {
      case BOM_SNIFFING_NOT_STARTED:
        NS_ASSERTION(i == 0, "Bad BOM sniffing state.");
        switch (*aFromSegment) {
          case 0xEF:
            mBomState = SEEN_UTF_8_FIRST_BYTE;
            break;
          case 0xFF:
            mBomState = SEEN_UTF_16_LE_FIRST_BYTE;
            break;
          case 0xFE:
            mBomState = SEEN_UTF_16_BE_FIRST_BYTE;
            break;
          default:
            mBomState = BOM_SNIFFING_OVER;
            break;
        }
        break;
      case SEEN_UTF_16_LE_FIRST_BYTE:
        if (aFromSegment[i] == 0xFE) {
          rv = SetupDecodingFromBom("UTF-16", "UTF-16LE"); 
          NS_ENSURE_SUCCESS(rv, rv);
          PRUint32 count = aCount - (i + 1);
          rv = WriteStreamBytes(aFromSegment + (i + 1), count, &writeCount);
          NS_ENSURE_SUCCESS(rv, rv);
          *aWriteCount = writeCount + (i + 1);
          return rv;
        }
        mBomState = BOM_SNIFFING_OVER;
        break;
      case SEEN_UTF_16_BE_FIRST_BYTE:
        if (aFromSegment[i] == 0xFF) {
          rv = SetupDecodingFromBom("UTF-16", "UTF-16BE"); 
          NS_ENSURE_SUCCESS(rv, rv);
          PRUint32 count = aCount - (i + 1);
          rv = WriteStreamBytes(aFromSegment + (i + 1), count, &writeCount);
          NS_ENSURE_SUCCESS(rv, rv);
          *aWriteCount = writeCount + (i + 1);
          return rv;
        }
        mBomState = BOM_SNIFFING_OVER;
        break;
      case SEEN_UTF_8_FIRST_BYTE:
        if (aFromSegment[i] == 0xBB) {
          mBomState = SEEN_UTF_8_SECOND_BYTE;
        } else {
          mBomState = BOM_SNIFFING_OVER;
        }
        break;
      case SEEN_UTF_8_SECOND_BYTE:
        if (aFromSegment[i] == 0xBF) {
          rv = SetupDecodingFromBom("UTF-8", "UTF-8"); 
          NS_ENSURE_SUCCESS(rv, rv);
          PRUint32 count = aCount - (i + 1);
          rv = WriteStreamBytes(aFromSegment + (i + 1), count, &writeCount);
          NS_ENSURE_SUCCESS(rv, rv);
          *aWriteCount = writeCount + (i + 1);
          return rv;
        }
        mBomState = BOM_SNIFFING_OVER;
        break;
      default:
        mBomState = BOM_SNIFFING_OVER;
        break;
    }
  }
  
  
  if (!mMetaScanner) {
    mMetaScanner = new nsHtml5MetaScanner();
  }
  
  if (mSniffingLength + aCount >= NS_HTML5_STREAM_PARSER_SNIFFING_BUFFER_SIZE) {
    
    PRUint32 countToSniffingLimit = NS_HTML5_STREAM_PARSER_SNIFFING_BUFFER_SIZE - mSniffingLength;
    nsHtml5ByteReadable readable(aFromSegment, aFromSegment + countToSniffingLimit);
    mMetaScanner->sniff(&readable, getter_AddRefs(mUnicodeDecoder), mCharset);
    if (mUnicodeDecoder) {
      mUnicodeDecoder->SetInputErrorBehavior(nsIUnicodeDecoder::kOnError_Recover);
      
      mCharsetSource = kCharsetFromMetaPrescan;
      mFeedChardet = PR_FALSE;
      mTreeBuilder->SetDocumentCharset(mCharset, mCharsetSource);
      mMetaScanner = nsnull;
      return WriteSniffingBufferAndCurrentSegment(aFromSegment, aCount, aWriteCount);
    }
    return FinalizeSniffing(aFromSegment, aCount, aWriteCount, countToSniffingLimit);
  }

  
  nsHtml5ByteReadable readable(aFromSegment, aFromSegment + aCount);
  mMetaScanner->sniff(&readable, getter_AddRefs(mUnicodeDecoder), mCharset);
  if (mUnicodeDecoder) {
    
    mUnicodeDecoder->SetInputErrorBehavior(nsIUnicodeDecoder::kOnError_Recover);
    mCharsetSource = kCharsetFromMetaPrescan;
    mFeedChardet = PR_FALSE;
    mTreeBuilder->SetDocumentCharset(mCharset, mCharsetSource);
    mMetaScanner = nsnull;
    return WriteSniffingBufferAndCurrentSegment(aFromSegment, aCount, aWriteCount);
  }
  if (!mSniffingBuffer) {
    mSniffingBuffer = new PRUint8[NS_HTML5_STREAM_PARSER_SNIFFING_BUFFER_SIZE];
  }
  memcpy(mSniffingBuffer + mSniffingLength, aFromSegment, aCount);
  mSniffingLength += aCount;
  *aWriteCount = aCount;
  return NS_OK;
}

nsresult
nsHtml5StreamParser::WriteStreamBytes(const PRUint8* aFromSegment,
                                      PRUint32 aCount,
                                      PRUint32* aWriteCount)
{
  NS_ASSERTION(IsParserThread(), "Wrong thread!");
  
  if (mLastBuffer->getEnd() == NS_HTML5_STREAM_PARSER_READ_BUFFER_SIZE) {
    mLastBuffer = (mLastBuffer->next = new nsHtml5UTF16Buffer(NS_HTML5_STREAM_PARSER_READ_BUFFER_SIZE));
  }
  PRInt32 totalByteCount = 0;
  for (;;) {
    PRInt32 end = mLastBuffer->getEnd();
    PRInt32 byteCount = aCount - totalByteCount;
    PRInt32 utf16Count = NS_HTML5_STREAM_PARSER_READ_BUFFER_SIZE - end;

    NS_ASSERTION(utf16Count, "Trying to convert into a buffer with no free space!");
    
    

    nsresult convResult = mUnicodeDecoder->Convert((const char*)aFromSegment, &byteCount, mLastBuffer->getBuffer() + end, &utf16Count);

    end += utf16Count;
    mLastBuffer->setEnd(end);
    totalByteCount += byteCount;
    aFromSegment += byteCount;

    NS_ASSERTION(end <= NS_HTML5_STREAM_PARSER_READ_BUFFER_SIZE,
        "The Unicode decoder wrote too much data.");
    NS_ASSERTION(byteCount >= -1, "The decoder consumed fewer than -1 bytes.");

    if (NS_FAILED(convResult)) {
      
      
      NS_ASSERTION(convResult == NS_ERROR_ILLEGAL_INPUT,
          "The decoder signaled an error other than NS_ERROR_ILLEGAL_INPUT.");

      
      
      

      if (totalByteCount < (PRInt32)aCount) {
        
        ++totalByteCount;
        ++aFromSegment;
      } else {
        NS_NOTREACHED("The decoder signaled an error but consumed all input.");
        
        
        totalByteCount = (PRInt32)aCount;
      }

      
      mLastBuffer->getBuffer()[end] = 0xFFFD;
      ++end;
      mLastBuffer->setEnd(end);
      if (end == NS_HTML5_STREAM_PARSER_READ_BUFFER_SIZE) {
          mLastBuffer = mLastBuffer->next = new nsHtml5UTF16Buffer(NS_HTML5_STREAM_PARSER_READ_BUFFER_SIZE);
      }

      mUnicodeDecoder->Reset();
      if (totalByteCount == (PRInt32)aCount) {
        *aWriteCount = (PRUint32)totalByteCount;
        return NS_OK;
      }
    } else if (convResult == NS_PARTIAL_MORE_OUTPUT) {
      mLastBuffer = mLastBuffer->next = new nsHtml5UTF16Buffer(NS_HTML5_STREAM_PARSER_READ_BUFFER_SIZE);
      
      
      
    } else {
      NS_ASSERTION(totalByteCount == (PRInt32)aCount,
          "The Unicode decoder consumed the wrong number of bytes.");
      *aWriteCount = (PRUint32)totalByteCount;
      return NS_OK;
    }
  }
}


nsresult
nsHtml5StreamParser::OnStartRequest(nsIRequest* aRequest, nsISupports* aContext)
{
  NS_PRECONDITION(STREAM_NOT_STARTED == mStreamState,
                  "Got OnStartRequest when the stream had already started.");
  NS_PRECONDITION(!mExecutor->HasStarted(), 
                  "Got OnStartRequest at the wrong stage in the executor life cycle.");
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  if (mObserver) {
    mObserver->OnStartRequest(aRequest, aContext);
  }
  mRequest = aRequest;

  mStreamState = STREAM_BEING_READ;

  PRBool scriptingEnabled = mExecutor->IsScriptEnabled();
  mOwner->StartTokenizer(scriptingEnabled);
  mTreeBuilder->setScriptingEnabled(scriptingEnabled);
  mTokenizer->start();
  mExecutor->Start();
  mExecutor->StartReadingFromStage();
  




  mExecutor->WillBuildModel(eDTDMode_unknown);
  
  nsresult rv = NS_OK;

  mReparseForbidden = PR_FALSE;
  nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(mRequest, &rv));
  if (NS_SUCCEEDED(rv)) {
    nsCAutoString method;
    httpChannel->GetRequestMethod(method);
    
    
    if (!method.EqualsLiteral("GET")) {
      
      
      mReparseForbidden = PR_TRUE;
    }
  }

  if (mCharsetSource >= kCharsetFromAutoDetection) {
    mFeedChardet = PR_FALSE;
  }
  
  if (mCharsetSource <= kCharsetFromMetaPrescan) {
    
    
    return NS_OK;
  }
  
  nsCOMPtr<nsICharsetConverterManager> convManager = do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = convManager->GetUnicodeDecoder(mCharset.get(), getter_AddRefs(mUnicodeDecoder));
  
  
  if (NS_SUCCEEDED(rv)) {
    mUnicodeDecoder->SetInputErrorBehavior(nsIUnicodeDecoder::kOnError_Recover);
  } else {
    mCharsetSource = kCharsetFromWeakDocTypeDefault;
  }
  return NS_OK;
}

void
nsHtml5StreamParser::DoStopRequest()
{
  NS_ASSERTION(IsParserThread(), "Wrong thread!");
  NS_PRECONDITION(STREAM_BEING_READ == mStreamState,
                  "Stream ended without being open.");
  mTokenizerMutex.AssertCurrentThreadOwns();

  if (IsTerminated()) {
    return;
  }

  if (!mUnicodeDecoder) {
    PRUint32 writeCount;
    FinalizeSniffing(nsnull, 0, &writeCount, 0);
    
  } else if (mFeedChardet) {
    mChardet->Done();
  }

  mStreamState = STREAM_ENDED;

  if (IsTerminatedOrInterrupted()) {
    return;
  }

  ParseAvailableData(); 
}

class nsHtml5RequestStopper : public nsRunnable
{
  private:
    nsHtml5RefPtr<nsHtml5StreamParser> mStreamParser;
  public:
    nsHtml5RequestStopper(nsHtml5StreamParser* aStreamParser)
      : mStreamParser(aStreamParser)
    {}
    NS_IMETHODIMP Run()
    {
      mozilla::MutexAutoLock autoLock(mStreamParser->mTokenizerMutex);
      mStreamParser->DoStopRequest();
      return NS_OK;
    }
};

nsresult
nsHtml5StreamParser::OnStopRequest(nsIRequest* aRequest,
                             nsISupports* aContext,
                             nsresult status)
{
  NS_ASSERTION(mRequest == aRequest, "Got Stop on wrong stream.");
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  if (mObserver) {
    mObserver->OnStopRequest(aRequest, aContext, status);
  }
  nsCOMPtr<nsIRunnable> stopper = new nsHtml5RequestStopper(this);
  if (NS_FAILED(mThread->Dispatch(stopper, nsIThread::DISPATCH_NORMAL))) {
    NS_WARNING("Dispatching StopRequest event failed.");
  }
  return NS_OK;
}

void
nsHtml5StreamParser::DoDataAvailable(PRUint8* aBuffer, PRUint32 aLength)
{
  NS_ASSERTION(IsParserThread(), "Wrong thread!");
  NS_PRECONDITION(STREAM_BEING_READ == mStreamState,
                  "DoDataAvailable called when stream not open.");
  mTokenizerMutex.AssertCurrentThreadOwns();

  if (IsTerminated()) {
    return;
  }

  PRUint32 writeCount;
  if (HasDecoder()) {
    if (mFeedChardet) {
      PRBool dontFeed;
      mChardet->DoIt((const char*)aBuffer, aLength, &dontFeed);
      mFeedChardet = !dontFeed;
    }
    WriteStreamBytes(aBuffer, aLength, &writeCount);
  } else {
    SniffStreamBytes(aBuffer, aLength, &writeCount);
  }
  
  NS_ASSERTION(writeCount == aLength, "Wrong number of stream bytes written/sniffed.");

  if (IsTerminatedOrInterrupted()) {
    return;
  }

  ParseAvailableData();

  if (mFlushTimerArmed || mSpeculating) {
    return;
  }

  mFlushTimer->InitWithFuncCallback(nsHtml5StreamParser::TimerCallback,
                                    static_cast<void*> (this),
                                    mFlushTimerEverFired ?
                                        sTimerInitialDelay :
                                        sTimerSubsequentDelay,
                                    nsITimer::TYPE_ONE_SHOT);
  mFlushTimerArmed = PR_TRUE;
}

class nsHtml5DataAvailable : public nsRunnable
{
  private:
    nsHtml5RefPtr<nsHtml5StreamParser> mStreamParser;
    nsAutoArrayPtr<PRUint8>            mData;
    PRUint32                           mLength;
  public:
    nsHtml5DataAvailable(nsHtml5StreamParser* aStreamParser,
                         PRUint8*             aData,
                         PRUint32             aLength)
      : mStreamParser(aStreamParser)
      , mData(aData)
      , mLength(aLength)
    {}
    NS_IMETHODIMP Run()
    {
      mozilla::MutexAutoLock autoLock(mStreamParser->mTokenizerMutex);
      mStreamParser->DoDataAvailable(mData, mLength);
      return NS_OK;
    }
};


nsresult
nsHtml5StreamParser::OnDataAvailable(nsIRequest* aRequest,
                               nsISupports* aContext,
                               nsIInputStream* aInStream,
                               PRUint32 aSourceOffset,
                               PRUint32 aLength)
{
  NS_ASSERTION(mRequest == aRequest, "Got data on wrong stream.");
  PRUint32 totalRead;
  nsAutoArrayPtr<PRUint8> data(new PRUint8[aLength]);
  nsresult rv = aInStream->Read(reinterpret_cast<char*>(data.get()),
  aLength, &totalRead);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ASSERTION(totalRead <= aLength, "Read more bytes than were available?");
  nsCOMPtr<nsIRunnable> dataAvailable = new nsHtml5DataAvailable(this,
                                                                data.forget(),
                                                                totalRead);
  if (NS_FAILED(mThread->Dispatch(dataAvailable, nsIThread::DISPATCH_NORMAL))) {
    NS_WARNING("Dispatching DataAvailable event failed.");
  }
  return rv;
}

PRBool
nsHtml5StreamParser::internalEncodingDeclaration(nsString* aEncoding)
{
  
  
  
  NS_ASSERTION(IsParserThread(), "Wrong thread!");
  if (mCharsetSource >= kCharsetFromMetaTag) { 
    return PR_FALSE;
  }

  if (mReparseForbidden) {
    return PR_FALSE; 
  }

  nsCAutoString newEncoding;
  CopyUTF16toUTF8(*aEncoding, newEncoding);
  newEncoding.Trim(" \t\r\n\f");
  if (newEncoding.LowerCaseEqualsLiteral("utf-16") ||
      newEncoding.LowerCaseEqualsLiteral("utf-16be") ||
      newEncoding.LowerCaseEqualsLiteral("utf-16le")) {
    newEncoding.Assign("UTF-8");
  }

  nsresult rv = NS_OK;
  nsCOMPtr<nsICharsetAlias> calias(do_GetService(kCharsetAliasCID, &rv));
  if (NS_FAILED(rv)) {
    NS_NOTREACHED("Charset alias service not available.");
    return PR_FALSE;
  }
  PRBool eq;
  rv = calias->Equals(newEncoding, mCharset, &eq);
  if (NS_FAILED(rv)) {
    NS_NOTREACHED("Charset name equality check failed.");
    return PR_FALSE;
  }
  if (eq) {
    mCharsetSource = kCharsetFromMetaTag; 
    return PR_FALSE;
  }
  
  
  
  nsCAutoString preferred;
  
  rv = calias->GetPreferred(newEncoding, preferred);
  if (NS_FAILED(rv)) {
    
    return PR_FALSE;
  }
  
  if (preferred.LowerCaseEqualsLiteral("utf-16") ||
      preferred.LowerCaseEqualsLiteral("utf-16be") ||
      preferred.LowerCaseEqualsLiteral("utf-16le") ||
      preferred.LowerCaseEqualsLiteral("utf-7") ||
      preferred.LowerCaseEqualsLiteral("jis_x0212-1990") ||
      preferred.LowerCaseEqualsLiteral("x-jis0208") ||
      preferred.LowerCaseEqualsLiteral("x-imap4-modified-utf7") ||
      preferred.LowerCaseEqualsLiteral("x-user-defined")) {
    
    return PR_FALSE;
  }

  mTreeBuilder->NeedsCharsetSwitchTo(preferred, kCharsetFromMetaTag);
  FlushTreeOpsAndDisarmTimer();
  Interrupt();
  
  
  
  return PR_TRUE;
}

void
nsHtml5StreamParser::FlushTreeOpsAndDisarmTimer()
{
  NS_ASSERTION(IsParserThread(), "Wrong thread!");
  if (mFlushTimerArmed) {
    
    
    mFlushTimer->Cancel();
    mFlushTimerArmed = PR_FALSE;
  }
  mTreeBuilder->Flush();
  if (NS_FAILED(NS_DispatchToMainThread(mExecutorFlusher))) {
    NS_WARNING("failed to dispatch executor flush event");
  }
}

void
nsHtml5StreamParser::ParseAvailableData()
{
  NS_ASSERTION(IsParserThread(), "Wrong thread!");
  mTokenizerMutex.AssertCurrentThreadOwns();

  if (IsTerminatedOrInterrupted()) {
    return;
  }
  
  for (;;) {
    if (!mFirstBuffer->hasMore()) {
      if (mFirstBuffer == mLastBuffer) {
        switch (mStreamState) {
          case STREAM_BEING_READ:
            
            if (!mSpeculating) {
              
              mFirstBuffer->setStart(0);
              mFirstBuffer->setEnd(0);
            }
            mTreeBuilder->FlushLoads();
            
            
            
            if (NS_FAILED(NS_DispatchToMainThread(mLoadFlusher))) {
              NS_WARNING("failed to dispatch load flush event");
            }
            return; 
          case STREAM_ENDED:
            if (mAtEOF) {
              return;
            }
            mAtEOF = PR_TRUE;
            mTokenizer->eof();
            mTreeBuilder->StreamEnded();
            FlushTreeOpsAndDisarmTimer();
            return; 
          default:
            NS_NOTREACHED("It should be impossible to reach this.");
            return;
        }
      }
      mFirstBuffer = mFirstBuffer->next;
      continue;
    }

    
    mFirstBuffer->adjust(mLastWasCR);
    mLastWasCR = PR_FALSE;
    if (mFirstBuffer->hasMore()) {
      mLastWasCR = mTokenizer->tokenizeBuffer(mFirstBuffer);
      
      
      
      
      if (mTreeBuilder->HasScript()) {
        mozilla::MutexAutoLock speculationAutoLock(mSpeculationMutex);
        nsHtml5Speculation* speculation = 
          new nsHtml5Speculation(mFirstBuffer,
                                 mFirstBuffer->getStart(),
                                 mTokenizer->getLineNumber(),
                                 mTreeBuilder->newSnapshot());
        mTreeBuilder->AddSnapshotToScript(speculation->GetSnapshot(), 
                                          speculation->GetStartLineNumber());
        FlushTreeOpsAndDisarmTimer();
        mTreeBuilder->SetOpSink(speculation);
        mSpeculations.AppendElement(speculation); 
        mSpeculating = PR_TRUE;
      }
      if (IsTerminatedOrInterrupted()) {
        return;
      }
    }
    continue;
  }
}

class nsHtml5StreamParserContinuation : public nsRunnable
{
private:
  nsHtml5RefPtr<nsHtml5StreamParser> mStreamParser;
public:
  nsHtml5StreamParserContinuation(nsHtml5StreamParser* aStreamParser)
    : mStreamParser(aStreamParser)
  {}
  NS_IMETHODIMP Run()
  {
    mozilla::MutexAutoLock autoLock(mStreamParser->mTokenizerMutex);
    mStreamParser->Uninterrupt();
    mStreamParser->ParseAvailableData();
    return NS_OK;
  }
};

void
nsHtml5StreamParser::ContinueAfterScripts(nsHtml5Tokenizer* aTokenizer, 
                                          nsHtml5TreeBuilder* aTreeBuilder,
                                          PRBool aLastWasCR)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  #ifdef DEBUG
    mExecutor->AssertStageEmpty();
  #endif
  PRBool speculationFailed = PR_FALSE;
  {
    mozilla::MutexAutoLock speculationAutoLock(mSpeculationMutex);
    if (mSpeculations.IsEmpty()) {
      NS_NOTREACHED("ContinueAfterScripts called without speculations.");
      return;
    }
    nsHtml5Speculation* speculation = mSpeculations.ElementAt(0);
    if (aLastWasCR || 
        !aTokenizer->isInDataState() || 
        !aTreeBuilder->snapshotMatches(speculation->GetSnapshot())) {
      speculationFailed = PR_TRUE;
      
      Interrupt(); 
      
    } else {
      
      if (mSpeculations.Length() > 1) {
        
        
        speculation->FlushToSink(mExecutor);
        NS_ASSERTION(!mExecutor->IsScriptExecuting(),
          "ParseUntilBlocked() was supposed to ensure we don't come "
          "here when scripts are executing.");
        NS_ASSERTION(mExecutor->IsInFlushLoop(), "How are we here if "
          "RunFlushLoop() didn't call ParseUntilBlocked() which is the "
          "only caller of this method?");
        mSpeculations.RemoveElementAt(0);
        return;
      }
      
      Interrupt(); 
      
      
      
      
      
    }
  }
  {
    mozilla::MutexAutoLock tokenizerAutoLock(mTokenizerMutex);
    #ifdef DEBUG
    {
      nsCOMPtr<nsIThread> mainThread;
      NS_GetMainThread(getter_AddRefs(mainThread));
      mAtomTable.SetPermittedLookupThread(mainThread);
    }
    #endif
    
    
    
    
    if (speculationFailed) {
      
      mAtEOF = PR_FALSE;
      nsHtml5Speculation* speculation = mSpeculations.ElementAt(0);
      mFirstBuffer = speculation->GetBuffer();
      mFirstBuffer->setStart(speculation->GetStart());
      mTokenizer->setLineNumber(speculation->GetStartLineNumber());

      nsContentUtils::ReportToConsole(nsContentUtils::eDOM_PROPERTIES,
                                      "SpeculationFailed",
                                      nsnull, 0,
                                      nsnull,
                                      EmptyString(),
                                      speculation->GetStartLineNumber(),
                                      0,
                                      nsIScriptError::warningFlag,
                                      "DOM Events",
                                      mExecutor->GetDocument());

      nsHtml5UTF16Buffer* buffer = mFirstBuffer->next;
      while (buffer) {
        buffer->setStart(0);
        buffer = buffer->next;
      }
      
      mSpeculations.Clear(); 
                             

      mTreeBuilder->flushCharacters(); 
      mTreeBuilder->ClearOps(); 

      mTreeBuilder->SetOpSink(mExecutor->GetStage());
      mExecutor->StartReadingFromStage();
      mSpeculating = PR_FALSE;

      
      mLastWasCR = aLastWasCR;
      mTokenizer->loadState(aTokenizer);
      mTreeBuilder->loadState(aTreeBuilder, &mAtomTable);
    } else {    
      
      
      mSpeculations.ElementAt(0)->FlushToSink(mExecutor);
      NS_ASSERTION(!mExecutor->IsScriptExecuting(),
        "ParseUntilBlocked() was supposed to ensure we don't come "
        "here when scripts are executing.");
      NS_ASSERTION(mExecutor->IsInFlushLoop(), "How are we here if "
        "RunFlushLoop() didn't call ParseUntilBlocked() which is the "
        "only caller of this method?");
      mSpeculations.RemoveElementAt(0);
      if (mSpeculations.IsEmpty()) {
        
        
        
        
        mTreeBuilder->SetOpSink(mExecutor);
        mTreeBuilder->Flush(PR_TRUE);
        mTreeBuilder->SetOpSink(mExecutor->GetStage());
        mExecutor->StartReadingFromStage();
        mSpeculating = PR_FALSE;
      }
    }
    nsCOMPtr<nsIRunnable> event = new nsHtml5StreamParserContinuation(this);
    if (NS_FAILED(mThread->Dispatch(event, nsIThread::DISPATCH_NORMAL))) {
      NS_WARNING("Failed to dispatch nsHtml5StreamParserContinuation");
    }
    
    #ifdef DEBUG
      mAtomTable.SetPermittedLookupThread(mThread);
    #endif
  }
}

void
nsHtml5StreamParser::ContinueAfterFailedCharsetSwitch()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  nsCOMPtr<nsIRunnable> event = new nsHtml5StreamParserContinuation(this);
  if (NS_FAILED(mThread->Dispatch(event, nsIThread::DISPATCH_NORMAL))) {
    NS_WARNING("Failed to dispatch nsHtml5StreamParserContinuation");
  }
}

class nsHtml5TimerKungFu : public nsRunnable
{
private:
  nsHtml5RefPtr<nsHtml5StreamParser> mStreamParser;
public:
  nsHtml5TimerKungFu(nsHtml5StreamParser* aStreamParser)
    : mStreamParser(aStreamParser)
  {}
  NS_IMETHODIMP Run()
  {
    if (mStreamParser->mFlushTimer) {
      mStreamParser->mFlushTimer->Cancel();
      mStreamParser->mFlushTimer = nsnull;
    }
    return NS_OK;
  }
};

void
nsHtml5StreamParser::DropTimer()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  

















  if (mFlushTimer) {
    nsCOMPtr<nsIRunnable> event = new nsHtml5TimerKungFu(this);
    if (NS_FAILED(mThread->Dispatch(event, nsIThread::DISPATCH_NORMAL))) {
      NS_WARNING("Failed to dispatch TimerKungFu event");
    }
  }
}



void
nsHtml5StreamParser::TimerCallback(nsITimer* aTimer, void* aClosure)
{
  (static_cast<nsHtml5StreamParser*> (aClosure))->TimerFlush();
}

void
nsHtml5StreamParser::TimerFlush()
{
  NS_ASSERTION(IsParserThread(), "Wrong thread!");
  mozilla::MutexAutoLock autoLock(mTokenizerMutex);

  NS_ASSERTION(!mSpeculating, "Flush timer fired while speculating.");

  
  
  mFlushTimerArmed = PR_FALSE;

  mFlushTimerEverFired = PR_TRUE;

  if (IsTerminatedOrInterrupted()) {
    return;
  }

  
  
  if (mTreeBuilder->Flush(PR_TRUE)) {
    if (NS_FAILED(NS_DispatchToMainThread(mExecutorFlusher))) {
      NS_WARNING("failed to dispatch executor flush event");
    }
  }
}
