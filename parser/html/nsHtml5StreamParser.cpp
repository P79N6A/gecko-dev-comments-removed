





#include "mozilla/DebugOnly.h"

#include "nsHtml5StreamParser.h"
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
#include "nsHtml5Highlighter.h"
#include "expat_config.h"
#include "expat.h"
#include "nsINestedURI.h"
#include "nsCharsetSource.h"
#include "nsIWyciwygChannel.h"
#include "nsIThreadRetargetableRequest.h"
#include "nsPrintfCString.h"
#include "nsNetUtil.h"
#include "nsXULAppAPI.h"

#include "mozilla/dom/EncodingUtils.h"

using namespace mozilla;
using mozilla::dom::EncodingUtils;

int32_t nsHtml5StreamParser::sTimerInitialDelay = 120;
int32_t nsHtml5StreamParser::sTimerSubsequentDelay = 120;


void
nsHtml5StreamParser::InitializeStatics()
{
  Preferences::AddIntVarCache(&sTimerInitialDelay,
                              "html5.flushtimer.initialdelay");
  Preferences::AddIntVarCache(&sTimerSubsequentDelay,
                              "html5.flushtimer.subsequentdelay");
}

























NS_IMPL_CYCLE_COLLECTING_ADDREF(nsHtml5StreamParser)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsHtml5StreamParser)

NS_INTERFACE_TABLE_HEAD(nsHtml5StreamParser)
  NS_INTERFACE_TABLE(nsHtml5StreamParser,
                     nsICharsetDetectionObserver)
  NS_INTERFACE_TABLE_TO_MAP_SEGUE_CYCLE_COLLECTION(nsHtml5StreamParser)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTION_CLASS(nsHtml5StreamParser)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsHtml5StreamParser)
  tmp->DropTimer();
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mObserver)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mRequest)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mOwner)
  tmp->mExecutorFlusher = nullptr;
  tmp->mLoadFlusher = nullptr;
  tmp->mExecutor = nullptr;
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mChardet)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsHtml5StreamParser)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mObserver)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mRequest)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mOwner)
  
  if (tmp->mExecutorFlusher) {
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mExecutorFlusher->mExecutor");
    cb.NoteXPCOMChild(static_cast<nsIContentSink*> (tmp->mExecutor));
  }
  
  if (tmp->mLoadFlusher) {
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mLoadFlusher->mExecutor");
    cb.NoteXPCOMChild(static_cast<nsIContentSink*> (tmp->mExecutor));
  }
  
  if (tmp->mChardet) {
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mChardet->mObserver");
    cb.NoteXPCOMChild(static_cast<nsICharsetDetectionObserver*>(tmp));
  }
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

class nsHtml5ExecutorFlusher : public nsRunnable
{
  private:
    nsRefPtr<nsHtml5TreeOpExecutor> mExecutor;
  public:
    explicit nsHtml5ExecutorFlusher(nsHtml5TreeOpExecutor* aExecutor)
      : mExecutor(aExecutor)
    {}
    NS_IMETHODIMP Run()
    {
      if (!mExecutor->isInList()) {
        mExecutor->RunFlushLoop();
      }
      return NS_OK;
    }
};

class nsHtml5LoadFlusher : public nsRunnable
{
  private:
    nsRefPtr<nsHtml5TreeOpExecutor> mExecutor;
  public:
    explicit nsHtml5LoadFlusher(nsHtml5TreeOpExecutor* aExecutor)
      : mExecutor(aExecutor)
    {}
    NS_IMETHODIMP Run()
    {
      mExecutor->FlushSpeculativeLoads();
      return NS_OK;
    }
};

nsHtml5StreamParser::nsHtml5StreamParser(nsHtml5TreeOpExecutor* aExecutor,
                                         nsHtml5Parser* aOwner,
                                         eParserMode aMode)
  : mFirstBuffer(nullptr) 
  , mLastBuffer(nullptr) 
  , mExecutor(aExecutor)
  , mTreeBuilder(new nsHtml5TreeBuilder((aMode == VIEW_SOURCE_HTML ||
                                         aMode == VIEW_SOURCE_XML) ?
                                             nullptr : mExecutor->GetStage(),
                                         aMode == NORMAL ?
                                             mExecutor->GetStage() : nullptr))
  , mTokenizer(new nsHtml5Tokenizer(mTreeBuilder, aMode == VIEW_SOURCE_XML))
  , mTokenizerMutex("nsHtml5StreamParser mTokenizerMutex")
  , mOwner(aOwner)
  , mSpeculationMutex("nsHtml5StreamParser mSpeculationMutex")
  , mTerminatedMutex("nsHtml5StreamParser mTerminatedMutex")
  , mThread(nsHtml5Module::GetStreamParserThread())
  , mExecutorFlusher(new nsHtml5ExecutorFlusher(aExecutor))
  , mLoadFlusher(new nsHtml5LoadFlusher(aExecutor))
  , mFlushTimer(do_CreateInstance("@mozilla.org/timer;1"))
  , mMode(aMode)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  mFlushTimer->SetTarget(mThread);
#ifdef DEBUG
  mAtomTable.SetPermittedLookupThread(mThread);
#endif
  mTokenizer->setInterner(&mAtomTable);
  mTokenizer->setEncodingDeclarationHandler(this);

  if (aMode == VIEW_SOURCE_HTML || aMode == VIEW_SOURCE_XML) {
    nsHtml5Highlighter* highlighter =
      new nsHtml5Highlighter(mExecutor->GetStage());
    mTokenizer->EnableViewSource(highlighter); 
    mTreeBuilder->EnableViewSource(highlighter); 
  }

  
  
  
  
  const nsAdoptingCString& detectorName =
    Preferences::GetLocalizedCString("intl.charset.detector");
  if (!detectorName.IsEmpty()) {
    nsAutoCString detectorContractID;
    detectorContractID.AssignLiteral(NS_CHARSET_DETECTOR_CONTRACTID_BASE);
    detectorContractID += detectorName;
    if ((mChardet = do_CreateInstance(detectorContractID.get()))) {
      (void) mChardet->Init(this);
      mFeedChardet = true;
    }
  }

  
}

nsHtml5StreamParser::~nsHtml5StreamParser()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  mTokenizer->end();
  NS_ASSERTION(!mFlushTimer, "Flush timer was not dropped before dtor!");
#ifdef DEBUG
  mRequest = nullptr;
  mObserver = nullptr;
  mUnicodeDecoder = nullptr;
  mSniffingBuffer = nullptr;
  mMetaScanner = nullptr;
  mFirstBuffer = nullptr;
  mExecutor = nullptr;
  mTreeBuilder = nullptr;
  mTokenizer = nullptr;
  mOwner = nullptr;
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
    mFeedChardet = false; 
    nsAutoCString encoding;
    if (!EncodingUtils::FindEncodingForLabelNoReplacement(
        nsDependentCString(aCharset), encoding)) {
      return NS_OK;
    }
    if (HasDecoder()) {
      if (mCharset.Equals(encoding)) {
        NS_ASSERTION(mCharsetSource < kCharsetFromAutoDetection,
            "Why are we running chardet at all?");
        mCharsetSource = kCharsetFromAutoDetection;
        mTreeBuilder->SetDocumentCharset(mCharset, mCharsetSource);
      } else {
        
        
        mTreeBuilder->NeedsCharsetSwitchTo(encoding,
                                           kCharsetFromAutoDetection,
                                           0);
        FlushTreeOpsAndDisarmTimer();
        Interrupt();
      }
    } else {
      
      
      mCharset.Assign(encoding);
      mCharsetSource = kCharsetFromAutoDetection;
      mTreeBuilder->SetDocumentCharset(mCharset, mCharsetSource);
    }
  }
  return NS_OK;
}

void
nsHtml5StreamParser::SetViewSourceTitle(nsIURI* aURL)
{
  if (aURL) {
    nsCOMPtr<nsIURI> temp;
    bool isViewSource;
    aURL->SchemeIs("view-source", &isViewSource);
    if (isViewSource) {
      nsCOMPtr<nsINestedURI> nested = do_QueryInterface(aURL);
      nested->GetInnerURI(getter_AddRefs(temp));
    } else {
      temp = aURL;
    }
    bool isData;
    temp->SchemeIs("data", &isData);
    if (isData) {
      
      
      mViewSourceTitle.AssignLiteral("data:\xE2\x80\xA6");
    } else {
      temp->GetSpec(mViewSourceTitle);
    }
  }
}

nsresult
nsHtml5StreamParser::SetupDecodingAndWriteSniffingBufferAndCurrentSegment(const uint8_t* aFromSegment, 
                                                                          uint32_t aCount,
                                                                          uint32_t* aWriteCount)
{
  NS_ASSERTION(IsParserThread(), "Wrong thread!");
  nsresult rv = NS_OK;
  mUnicodeDecoder = EncodingUtils::DecoderForEncoding(mCharset);
  if (mSniffingBuffer) {
    uint32_t writeCount;
    rv = WriteStreamBytes(mSniffingBuffer, mSniffingLength, &writeCount);
    NS_ENSURE_SUCCESS(rv, rv);
    mSniffingBuffer = nullptr;
  }
  mMetaScanner = nullptr;
  if (aFromSegment) {
    rv = WriteStreamBytes(aFromSegment, aCount, aWriteCount);
  }
  return rv;
}

nsresult
nsHtml5StreamParser::SetupDecodingFromBom(const char* aDecoderCharsetName)
{
  NS_ASSERTION(IsParserThread(), "Wrong thread!");
  mCharset.Assign(aDecoderCharsetName);
  mUnicodeDecoder = EncodingUtils::DecoderForEncoding(mCharset);
  mCharsetSource = kCharsetFromByteOrderMark;
  mFeedChardet = false;
  mTreeBuilder->SetDocumentCharset(mCharset, mCharsetSource);
  mSniffingBuffer = nullptr;
  mMetaScanner = nullptr;
  mBomState = BOM_SNIFFING_OVER;
  return NS_OK;
}

void
nsHtml5StreamParser::SniffBOMlessUTF16BasicLatin(const uint8_t* aFromSegment,
                                                 uint32_t aCountToSniffingLimit)
{
  
  if (mMode == LOAD_AS_DATA) {
    return;
  }
  
  if (mSniffingLength + aCountToSniffingLimit < 30) {
    return;
  }
  
  bool byteZero[2] = { false, false };
  bool byteNonZero[2] = { false, false };
  uint32_t i = 0;
  if (mSniffingBuffer) {
    for (; i < mSniffingLength; ++i) {
      if (mSniffingBuffer[i]) {
        if (byteNonZero[1 - (i % 2)]) {
          return;
        }
        byteNonZero[i % 2] = true;
      } else {
        if (byteZero[1 - (i % 2)]) {
          return;
        }
        byteZero[i % 2] = true;
      }
    }
  }
  if (aFromSegment) {
    for (uint32_t j = 0; j < aCountToSniffingLimit; ++j) {
      if (aFromSegment[j]) {
        if (byteNonZero[1 - ((i + j) % 2)]) {
          return;
        }
        byteNonZero[(i + j) % 2] = true;
      } else {
        if (byteZero[1 - ((i + j) % 2)]) {
          return;
        }
        byteZero[(i + j) % 2] = true;
      }
    }
  }

  if (byteNonZero[0]) {
    mCharset.AssignLiteral("UTF-16LE");
  } else {
    mCharset.AssignLiteral("UTF-16BE");
  }
  mCharsetSource = kCharsetFromIrreversibleAutoDetection;
  mTreeBuilder->SetDocumentCharset(mCharset, mCharsetSource);
  mFeedChardet = false;
  mTreeBuilder->MaybeComplainAboutCharset("EncBomlessUtf16",
                                          true,
                                          0);

}

void
nsHtml5StreamParser::SetEncodingFromExpat(const char16_t* aEncoding)
{
  if (aEncoding) {
    nsDependentString utf16(aEncoding);
    nsAutoCString utf8;
    CopyUTF16toUTF8(utf16, utf8);
    if (PreferredForInternalEncodingDecl(utf8)) {
      mCharset.Assign(utf8);
      mCharsetSource = kCharsetFromMetaTag; 
      return;
    }
    
    
    
    
  }
  mCharset.AssignLiteral("UTF-8"); 
  mCharsetSource = kCharsetFromMetaTag; 
}






struct UserData {
  XML_Parser mExpat;
  nsHtml5StreamParser* mStreamParser;
};



static void
HandleXMLDeclaration(void* aUserData,
                     const XML_Char* aVersion,
                     const XML_Char* aEncoding,
                     int aStandalone)
{
  UserData* ud = static_cast<UserData*>(aUserData);
  ud->mStreamParser->SetEncodingFromExpat(
      reinterpret_cast<const char16_t*>(aEncoding));
  XML_StopParser(ud->mExpat, false);
}

static void
HandleStartElement(void* aUserData,
                   const XML_Char* aName,
                   const XML_Char **aAtts)
{
  UserData* ud = static_cast<UserData*>(aUserData);
  XML_StopParser(ud->mExpat, false);
}

static void
HandleEndElement(void* aUserData,
                 const XML_Char* aName)
{
  UserData* ud = static_cast<UserData*>(aUserData);
  XML_StopParser(ud->mExpat, false);
}

static void
HandleComment(void* aUserData,
              const XML_Char* aName)
{
  UserData* ud = static_cast<UserData*>(aUserData);
  XML_StopParser(ud->mExpat, false);
}

static void
HandleProcessingInstruction(void* aUserData,
                            const XML_Char* aTarget,
                            const XML_Char* aData)
{
  UserData* ud = static_cast<UserData*>(aUserData);
  XML_StopParser(ud->mExpat, false);
}

nsresult
nsHtml5StreamParser::FinalizeSniffing(const uint8_t* aFromSegment, 
                                      uint32_t aCount,
                                      uint32_t* aWriteCount,
                                      uint32_t aCountToSniffingLimit)
{
  NS_ASSERTION(IsParserThread(), "Wrong thread!");
  NS_ASSERTION(mCharsetSource < kCharsetFromParentForced,
      "Should not finalize sniffing when using forced charset.");
  if (mMode == VIEW_SOURCE_XML) {
    static const XML_Memory_Handling_Suite memsuite =
      {
        (void *(*)(size_t))moz_xmalloc,
        (void *(*)(void *, size_t))moz_xrealloc,
        free
      };

    static const char16_t kExpatSeparator[] = { 0xFFFF, '\0' };

    static const char16_t kISO88591[] =
        { 'I', 'S', 'O', '-', '8', '8', '5', '9', '-', '1', '\0' };

    UserData ud;
    ud.mStreamParser = this;

    
    
    
    
    
    
    
    
    ud.mExpat = XML_ParserCreate_MM(kISO88591, &memsuite, kExpatSeparator);
    XML_SetXmlDeclHandler(ud.mExpat, HandleXMLDeclaration);
    XML_SetElementHandler(ud.mExpat, HandleStartElement, HandleEndElement);
    XML_SetCommentHandler(ud.mExpat, HandleComment);
    XML_SetProcessingInstructionHandler(ud.mExpat, HandleProcessingInstruction);
    XML_SetUserData(ud.mExpat, static_cast<void*>(&ud));

    XML_Status status = XML_STATUS_OK;

    
    
    
    
    
    
    
    if (mSniffingBuffer) {
      status = XML_Parse(ud.mExpat,
                         reinterpret_cast<const char*>(mSniffingBuffer.get()),
                         mSniffingLength,
                         false);
    }
    if (status == XML_STATUS_OK &&
        mCharsetSource < kCharsetFromMetaTag &&
        aFromSegment) {
      status = XML_Parse(ud.mExpat,
                         reinterpret_cast<const char*>(aFromSegment),
                         aCountToSniffingLimit,
                         false);
    }
    XML_ParserFree(ud.mExpat);

    if (mCharsetSource < kCharsetFromMetaTag) {
      
      
      
      
      mCharset.AssignLiteral("UTF-8");
      mCharsetSource = kCharsetFromMetaTag; 
    }

    return SetupDecodingAndWriteSniffingBufferAndCurrentSegment(aFromSegment,
                                                                aCount,
                                                                aWriteCount);
  }

  
  if (mCharsetSource >= kCharsetFromHintPrevDoc) {
    mFeedChardet = false;
    return SetupDecodingAndWriteSniffingBufferAndCurrentSegment(aFromSegment, aCount, aWriteCount);
  }
  
  
  SniffBOMlessUTF16BasicLatin(aFromSegment, aCountToSniffingLimit);
  
  
  if (mFeedChardet) {
    bool dontFeed;
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
      
      
      
      mFeedChardet = false;
      rv = mChardet->Done();
      NS_ENSURE_SUCCESS(rv, rv);
    }
    
  }
  if (mCharsetSource == kCharsetUninitialized) {
    
    mCharset.AssignLiteral("windows-1252");
    mCharsetSource = kCharsetFromFallback;
    mTreeBuilder->SetDocumentCharset(mCharset, mCharsetSource);
  } else if (mMode == LOAD_AS_DATA &&
             mCharsetSource == kCharsetFromFallback) {
    NS_ASSERTION(mReparseForbidden, "Reparse should be forbidden for XHR");
    NS_ASSERTION(!mFeedChardet, "Should not feed chardet for XHR");
    NS_ASSERTION(mCharset.EqualsLiteral("UTF-8"),
                 "XHR should default to UTF-8");
    
    mCharsetSource = kCharsetFromDocTypeDefault;
    mTreeBuilder->SetDocumentCharset(mCharset, mCharsetSource);
  }
  return SetupDecodingAndWriteSniffingBufferAndCurrentSegment(aFromSegment, aCount, aWriteCount);
}

nsresult
nsHtml5StreamParser::SniffStreamBytes(const uint8_t* aFromSegment,
                                      uint32_t aCount,
                                      uint32_t* aWriteCount)
{
  NS_ASSERTION(IsParserThread(), "Wrong thread!");
  nsresult rv = NS_OK;
  uint32_t writeCount;

  
  
  
  
  for (uint32_t i = 0; i < aCount && mBomState != BOM_SNIFFING_OVER; i++) {
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
          rv = SetupDecodingFromBom("UTF-16LE"); 
          NS_ENSURE_SUCCESS(rv, rv);
          uint32_t count = aCount - (i + 1);
          rv = WriteStreamBytes(aFromSegment + (i + 1), count, &writeCount);
          NS_ENSURE_SUCCESS(rv, rv);
          *aWriteCount = writeCount + (i + 1);
          return rv;
        }
        mBomState = BOM_SNIFFING_OVER;
        break;
      case SEEN_UTF_16_BE_FIRST_BYTE:
        if (aFromSegment[i] == 0xFF) {
          rv = SetupDecodingFromBom("UTF-16BE"); 
          NS_ENSURE_SUCCESS(rv, rv);
          uint32_t count = aCount - (i + 1);
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
          rv = SetupDecodingFromBom("UTF-8"); 
          NS_ENSURE_SUCCESS(rv, rv);
          uint32_t count = aCount - (i + 1);
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
  
  
  
  MOZ_ASSERT(mCharsetSource != kCharsetFromByteOrderMark,
             "Should not come here if BOM was found.");
  MOZ_ASSERT(mCharsetSource != kCharsetFromOtherComponent,
             "kCharsetFromOtherComponent is for XSLT.");

  if (mBomState == BOM_SNIFFING_OVER &&
    mCharsetSource == kCharsetFromChannel) {
    
    
    
    
    
    mFeedChardet = false;
    mTreeBuilder->SetDocumentCharset(mCharset, mCharsetSource);
    return SetupDecodingAndWriteSniffingBufferAndCurrentSegment(aFromSegment,
      aCount, aWriteCount);
  }

  if (!mMetaScanner && (mMode == NORMAL ||
                        mMode == VIEW_SOURCE_HTML ||
                        mMode == LOAD_AS_DATA)) {
    mMetaScanner = new nsHtml5MetaScanner();
  }
  
  if (mSniffingLength + aCount >= NS_HTML5_STREAM_PARSER_SNIFFING_BUFFER_SIZE) {
    
    uint32_t countToSniffingLimit =
        NS_HTML5_STREAM_PARSER_SNIFFING_BUFFER_SIZE - mSniffingLength;
    if (mMode == NORMAL || mMode == VIEW_SOURCE_HTML || mMode == LOAD_AS_DATA) {
      nsHtml5ByteReadable readable(aFromSegment, aFromSegment +
          countToSniffingLimit);
      nsAutoCString encoding;
      mMetaScanner->sniff(&readable, encoding);
      if (!encoding.IsEmpty()) {
        
        if ((mCharsetSource == kCharsetFromParentForced ||
             mCharsetSource == kCharsetFromUserForced) &&
            EncodingUtils::IsAsciiCompatible(encoding)) {
          
          return SetupDecodingAndWriteSniffingBufferAndCurrentSegment(
            aFromSegment, aCount, aWriteCount);
        }
        mCharset.Assign(encoding);
        mCharsetSource = kCharsetFromMetaPrescan;
        mFeedChardet = false;
        mTreeBuilder->SetDocumentCharset(mCharset, mCharsetSource);
        return SetupDecodingAndWriteSniffingBufferAndCurrentSegment(
          aFromSegment, aCount, aWriteCount);
      }
    }
    if (mCharsetSource == kCharsetFromParentForced ||
        mCharsetSource == kCharsetFromUserForced) {
      
      return SetupDecodingAndWriteSniffingBufferAndCurrentSegment(
        aFromSegment, aCount, aWriteCount);
    }
    return FinalizeSniffing(aFromSegment, aCount, aWriteCount,
        countToSniffingLimit);
  }

  
  if (mMode == NORMAL || mMode == VIEW_SOURCE_HTML || mMode == LOAD_AS_DATA) {
    nsHtml5ByteReadable readable(aFromSegment, aFromSegment + aCount);
    nsAutoCString encoding;
    mMetaScanner->sniff(&readable, encoding);
    if (!encoding.IsEmpty()) {
      
      if ((mCharsetSource == kCharsetFromParentForced ||
           mCharsetSource == kCharsetFromUserForced) &&
          EncodingUtils::IsAsciiCompatible(encoding)) {
        
        return SetupDecodingAndWriteSniffingBufferAndCurrentSegment(aFromSegment,
            aCount, aWriteCount);
      }
      mCharset.Assign(encoding);
      mCharsetSource = kCharsetFromMetaPrescan;
      mFeedChardet = false;
      mTreeBuilder->SetDocumentCharset(mCharset, mCharsetSource);
      return SetupDecodingAndWriteSniffingBufferAndCurrentSegment(aFromSegment,
        aCount, aWriteCount);
    }
  }

  if (!mSniffingBuffer) {
    mSniffingBuffer = new (mozilla::fallible)
      uint8_t[NS_HTML5_STREAM_PARSER_SNIFFING_BUFFER_SIZE];
    if (!mSniffingBuffer) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }
  memcpy(mSniffingBuffer + mSniffingLength, aFromSegment, aCount);
  mSniffingLength += aCount;
  *aWriteCount = aCount;
  return NS_OK;
}

nsresult
nsHtml5StreamParser::WriteStreamBytes(const uint8_t* aFromSegment,
                                      uint32_t aCount,
                                      uint32_t* aWriteCount)
{
  NS_ASSERTION(IsParserThread(), "Wrong thread!");
  
  
  if (!mLastBuffer) {
    NS_WARNING("mLastBuffer should not be null!");
    MarkAsBroken(NS_ERROR_NULL_POINTER);
    return NS_ERROR_NULL_POINTER;
  }
  if (mLastBuffer->getEnd() == NS_HTML5_STREAM_PARSER_READ_BUFFER_SIZE) {
    nsRefPtr<nsHtml5OwningUTF16Buffer> newBuf =
      nsHtml5OwningUTF16Buffer::FalliblyCreate(
        NS_HTML5_STREAM_PARSER_READ_BUFFER_SIZE);
    if (!newBuf) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    mLastBuffer = (mLastBuffer->next = newBuf.forget());
  }
  int32_t totalByteCount = 0;
  for (;;) {
    int32_t end = mLastBuffer->getEnd();
    int32_t byteCount = aCount - totalByteCount;
    int32_t utf16Count = NS_HTML5_STREAM_PARSER_READ_BUFFER_SIZE - end;

    NS_ASSERTION(utf16Count, "Trying to convert into a buffer with no free space!");
    
    

    nsresult convResult = mUnicodeDecoder->Convert((const char*)aFromSegment, &byteCount, mLastBuffer->getBuffer() + end, &utf16Count);
    MOZ_ASSERT(NS_SUCCEEDED(convResult));

    end += utf16Count;
    mLastBuffer->setEnd(end);
    totalByteCount += byteCount;
    aFromSegment += byteCount;

    NS_ASSERTION(end <= NS_HTML5_STREAM_PARSER_READ_BUFFER_SIZE,
        "The Unicode decoder wrote too much data.");
    NS_ASSERTION(byteCount >= -1, "The decoder consumed fewer than -1 bytes.");

    if (convResult == NS_PARTIAL_MORE_OUTPUT) {
      nsRefPtr<nsHtml5OwningUTF16Buffer> newBuf =
        nsHtml5OwningUTF16Buffer::FalliblyCreate(
          NS_HTML5_STREAM_PARSER_READ_BUFFER_SIZE);
      if (!newBuf) {
        return NS_ERROR_OUT_OF_MEMORY;
      }
      mLastBuffer = (mLastBuffer->next = newBuf.forget());
      
      
      
    } else {
      NS_ASSERTION(totalByteCount == (int32_t)aCount,
          "The Unicode decoder consumed the wrong number of bytes.");
      *aWriteCount = (uint32_t)totalByteCount;
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

  if (mMode == VIEW_SOURCE_HTML || mMode == VIEW_SOURCE_XML) {
    mTokenizer->StartViewSource(NS_ConvertUTF8toUTF16(mViewSourceTitle));
  }

  
  
  bool scriptingEnabled = mMode == LOAD_AS_DATA ?
                                   false : mExecutor->IsScriptEnabled();
  mOwner->StartTokenizer(scriptingEnabled);

  bool isSrcdoc = false;
  nsCOMPtr<nsIChannel> channel;
  nsresult rv = GetChannel(getter_AddRefs(channel));
  if (NS_SUCCEEDED(rv)) {
    isSrcdoc = NS_IsSrcdocChannel(channel);
  }
  mTreeBuilder->setIsSrcdocDocument(isSrcdoc);
  mTreeBuilder->setScriptingEnabled(scriptingEnabled);
  mTreeBuilder->SetPreventScriptExecution(!((mMode == NORMAL) &&
                                            scriptingEnabled));
  mTokenizer->start();
  mExecutor->Start();
  mExecutor->StartReadingFromStage();

  if (mMode == PLAIN_TEXT) {
    mTreeBuilder->StartPlainText();
    mTokenizer->StartPlainText();
  } else if (mMode == VIEW_SOURCE_PLAIN) {
    mTreeBuilder->StartPlainTextViewSource(NS_ConvertUTF8toUTF16(mViewSourceTitle));
    mTokenizer->StartPlainText();
  }

  




  rv = mExecutor->WillBuildModel(eDTDMode_unknown);
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsRefPtr<nsHtml5OwningUTF16Buffer> newBuf =
    nsHtml5OwningUTF16Buffer::FalliblyCreate(
      NS_HTML5_STREAM_PARSER_READ_BUFFER_SIZE);
  if (!newBuf) {
    
    
    
    return mExecutor->MarkAsBroken(NS_ERROR_OUT_OF_MEMORY);
  }
  NS_ASSERTION(!mFirstBuffer, "How come we have the first buffer set?");
  NS_ASSERTION(!mLastBuffer, "How come we have the last buffer set?");
  mFirstBuffer = mLastBuffer = newBuf;

  rv = NS_OK;

  
  
  
  mReparseForbidden = !(mMode == NORMAL || mMode == PLAIN_TEXT);

  nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(mRequest, &rv));
  if (NS_SUCCEEDED(rv)) {
    nsAutoCString method;
    httpChannel->GetRequestMethod(method);
    
    
    if (!method.EqualsLiteral("GET")) {
      
      
      mReparseForbidden = true;
      mFeedChardet = false; 
    }
  }

  
  
  nsCOMPtr<nsIThreadRetargetableRequest> threadRetargetableRequest =
    do_QueryInterface(mRequest, &rv);
  if (threadRetargetableRequest) {
    rv = threadRetargetableRequest->RetargetDeliveryTo(mThread);
  }

  if (NS_FAILED(rv)) {
    
    
    if (XRE_GetProcessType() != GeckoProcessType_Content) {
      NS_WARNING("Failed to retarget HTML data delivery to the parser thread.");
    }
  }

  if (mCharsetSource == kCharsetFromParentFrame) {
    
    mInitialEncodingWasFromParentFrame = true;
  }

  if (mCharsetSource >= kCharsetFromAutoDetection) {
    mFeedChardet = false;
  }
  
  nsCOMPtr<nsIWyciwygChannel> wyciwygChannel(do_QueryInterface(mRequest));
  if (!wyciwygChannel) {
    
    
    return NS_OK;
  }

  
  mReparseForbidden = true;
  mFeedChardet = false;

  
  mUnicodeDecoder = EncodingUtils::DecoderForEncoding(mCharset);
  return NS_OK;
}

nsresult
nsHtml5StreamParser::CheckListenerChain()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on the main thread!");
  if (!mObserver) {
    return NS_OK;
  }
  nsresult rv;
  nsCOMPtr<nsIThreadRetargetableStreamListener> retargetable =
    do_QueryInterface(mObserver, &rv);
  if (NS_SUCCEEDED(rv) && retargetable) {
    rv = retargetable->CheckListenerChain();
  }
  return rv;
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

  mStreamState = STREAM_ENDED;

  if (!mUnicodeDecoder) {
    uint32_t writeCount;
    nsresult rv;
    if (NS_FAILED(rv = FinalizeSniffing(nullptr, 0, &writeCount, 0))) {
      MarkAsBroken(rv);
      return;
    }
  } else if (mFeedChardet) {
    mChardet->Done();
  }

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
    explicit nsHtml5RequestStopper(nsHtml5StreamParser* aStreamParser)
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
nsHtml5StreamParser::DoDataAvailable(const uint8_t* aBuffer, uint32_t aLength)
{
  NS_ASSERTION(IsParserThread(), "Wrong thread!");
  NS_PRECONDITION(STREAM_BEING_READ == mStreamState,
                  "DoDataAvailable called when stream not open.");
  mTokenizerMutex.AssertCurrentThreadOwns();

  if (IsTerminated()) {
    return;
  }

  uint32_t writeCount;
  nsresult rv;
  if (HasDecoder()) {
    if (mFeedChardet) {
      bool dontFeed;
      mChardet->DoIt((const char*)aBuffer, aLength, &dontFeed);
      mFeedChardet = !dontFeed;
    }
    rv = WriteStreamBytes(aBuffer, aLength, &writeCount);
  } else {
    rv = SniffStreamBytes(aBuffer, aLength, &writeCount);
  }
  if (NS_FAILED(rv)) {
    MarkAsBroken(rv);
    return;
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
  mFlushTimerArmed = true;
}

class nsHtml5DataAvailable : public nsRunnable
{
  private:
    nsHtml5RefPtr<nsHtml5StreamParser> mStreamParser;
    nsAutoArrayPtr<uint8_t>            mData;
    uint32_t                           mLength;
  public:
    nsHtml5DataAvailable(nsHtml5StreamParser* aStreamParser,
                         uint8_t*             aData,
                         uint32_t             aLength)
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
                                     uint64_t aSourceOffset,
                                     uint32_t aLength)
{
  nsresult rv;
  if (NS_FAILED(rv = mExecutor->IsBroken())) {
    return rv;
  }

  NS_ASSERTION(mRequest == aRequest, "Got data on wrong stream.");
  uint32_t totalRead;
  
  if (NS_IsMainThread()) {
    nsAutoArrayPtr<uint8_t> data(new (mozilla::fallible) uint8_t[aLength]);
    if (!data) {
      return mExecutor->MarkAsBroken(NS_ERROR_OUT_OF_MEMORY);
    }
    rv = aInStream->Read(reinterpret_cast<char*>(data.get()),
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
  } else {
    NS_ASSERTION(IsParserThread(), "Wrong thread!");
    mozilla::MutexAutoLock autoLock(mTokenizerMutex);

    
    rv = aInStream->ReadSegments(CopySegmentsToParser, this, aLength,
                                 &totalRead);
    if (NS_FAILED(rv)) {
      NS_WARNING("Failed reading response data to parser");
      return rv;
    }
    return NS_OK;
  }
}


NS_METHOD
nsHtml5StreamParser::CopySegmentsToParser(nsIInputStream *aInStream,
                                          void *aClosure,
                                          const char *aFromSegment,
                                          uint32_t aToOffset,
                                          uint32_t aCount,
                                          uint32_t *aWriteCount)
{
  nsHtml5StreamParser* parser = static_cast<nsHtml5StreamParser*>(aClosure);

  parser->DoDataAvailable((const uint8_t*)aFromSegment, aCount);
  
  *aWriteCount = aCount;
  return NS_OK;
}

bool
nsHtml5StreamParser::PreferredForInternalEncodingDecl(nsACString& aEncoding)
{
  nsAutoCString newEncoding;
  if (!EncodingUtils::FindEncodingForLabel(aEncoding, newEncoding)) {
    
    mTreeBuilder->MaybeComplainAboutCharset("EncMetaUnsupported",
                                            true,
                                            mTokenizer->getLineNumber());
    return false;
  }

  if (newEncoding.EqualsLiteral("UTF-16BE") ||
      newEncoding.EqualsLiteral("UTF-16LE")) {
    mTreeBuilder->MaybeComplainAboutCharset("EncMetaUtf16",
                                            true,
                                            mTokenizer->getLineNumber());
    newEncoding.AssignLiteral("UTF-8");
  }

  if (newEncoding.EqualsLiteral("x-user-defined")) {
    
    mTreeBuilder->MaybeComplainAboutCharset("EncMetaUserDefined",
                                            true,
                                            mTokenizer->getLineNumber());
    newEncoding.AssignLiteral("windows-1252");
  }

  if (newEncoding.Equals(mCharset)) {
    if (mCharsetSource < kCharsetFromMetaPrescan) {
      if (mInitialEncodingWasFromParentFrame) {
        mTreeBuilder->MaybeComplainAboutCharset("EncLateMetaFrame",
                                                false,
                                                mTokenizer->getLineNumber());
      } else {
        mTreeBuilder->MaybeComplainAboutCharset("EncLateMeta",
                                                false,
                                                mTokenizer->getLineNumber());
      }
    }
    mCharsetSource = kCharsetFromMetaTag; 
    mFeedChardet = false; 
    return false;
  }

  aEncoding.Assign(newEncoding);
  return true;
}

bool
nsHtml5StreamParser::internalEncodingDeclaration(nsString* aEncoding)
{
  
  
  
  NS_ASSERTION(IsParserThread(), "Wrong thread!");
  if (mCharsetSource >= kCharsetFromMetaTag) { 
    return false;
  }

  nsAutoCString newEncoding;
  CopyUTF16toUTF8(*aEncoding, newEncoding);

  if (!PreferredForInternalEncodingDecl(newEncoding)) {
    return false;
  }

  if (mReparseForbidden) {
    
    
    
    
    mTreeBuilder->MaybeComplainAboutCharset("EncLateMetaTooLate",
                                            true,
                                            mTokenizer->getLineNumber());
    return false; 
  }

  
  
  mFeedChardet = false;
  mTreeBuilder->NeedsCharsetSwitchTo(newEncoding,
                                     kCharsetFromMetaTag,
                                     mTokenizer->getLineNumber());
  FlushTreeOpsAndDisarmTimer();
  Interrupt();
  
  
  
  
  
  return true;
}

void
nsHtml5StreamParser::FlushTreeOpsAndDisarmTimer()
{
  NS_ASSERTION(IsParserThread(), "Wrong thread!");
  if (mFlushTimerArmed) {
    
    
    mFlushTimer->Cancel();
    mFlushTimerArmed = false;
  }
  if (mMode == VIEW_SOURCE_HTML || mMode == VIEW_SOURCE_XML) {
    mTokenizer->FlushViewSource();
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

  if (mSpeculating && !IsSpeculationEnabled()) {
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
            mAtEOF = true;
            if (mCharsetSource < kCharsetFromMetaTag) {
              if (mInitialEncodingWasFromParentFrame) {
                
                
                
                
                
                mTreeBuilder->MaybeComplainAboutCharset("EncNoDeclarationFrame",
                                                        false,
                                                        0);
              } else if (mMode == NORMAL) {
                mTreeBuilder->MaybeComplainAboutCharset("EncNoDeclaration",
                                                        true,
                                                        0);
              } else if (mMode == PLAIN_TEXT) {
                mTreeBuilder->MaybeComplainAboutCharset("EncNoDeclarationPlain",
                                                        true,
                                                        0);
              }
            }
            mTokenizer->eof();
            mTreeBuilder->StreamEnded();
            if (mMode == VIEW_SOURCE_HTML || mMode == VIEW_SOURCE_XML) {
              mTokenizer->EndViewSource();
            }
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
    mLastWasCR = false;
    if (mFirstBuffer->hasMore()) {
      mLastWasCR = mTokenizer->tokenizeBuffer(mFirstBuffer);
      
      
      
      
      if (mTreeBuilder->HasScript()) {
        
        
        MOZ_ASSERT(mMode == NORMAL);
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
        mSpeculating = true;
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
  explicit nsHtml5StreamParserContinuation(nsHtml5StreamParser* aStreamParser)
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
                                          bool aLastWasCR)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(!(mMode == VIEW_SOURCE_HTML || mMode == VIEW_SOURCE_XML),
      "ContinueAfterScripts called in view source mode!");
  if (NS_FAILED(mExecutor->IsBroken())) {
    return;
  }
  #ifdef DEBUG
    mExecutor->AssertStageEmpty();
  #endif
  bool speculationFailed = false;
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
      speculationFailed = true;
      
      MaybeDisableFutureSpeculation();
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
      
      mAtEOF = false;
      nsHtml5Speculation* speculation = mSpeculations.ElementAt(0);
      mFirstBuffer = speculation->GetBuffer();
      mFirstBuffer->setStart(speculation->GetStart());
      mTokenizer->setLineNumber(speculation->GetStartLineNumber());

      nsContentUtils::ReportToConsole(nsIScriptError::warningFlag,
                                      NS_LITERAL_CSTRING("DOM Events"),
                                      mExecutor->GetDocument(),
                                      nsContentUtils::eDOM_PROPERTIES,
                                      "SpeculationFailed",
                                      nullptr, 0,
                                      nullptr,
                                      EmptyString(),
                                      speculation->GetStartLineNumber());

      nsHtml5OwningUTF16Buffer* buffer = mFirstBuffer->next;
      while (buffer) {
        buffer->setStart(0);
        buffer = buffer->next;
      }
      
      mSpeculations.Clear(); 
                             

      mTreeBuilder->flushCharacters(); 
      mTreeBuilder->ClearOps(); 

      mTreeBuilder->SetOpSink(mExecutor->GetStage());
      mExecutor->StartReadingFromStage();
      mSpeculating = false;

      
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
        mTreeBuilder->Flush(true);
        mTreeBuilder->SetOpSink(mExecutor->GetStage());
        mExecutor->StartReadingFromStage();
        mSpeculating = false;
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
  explicit nsHtml5TimerKungFu(nsHtml5StreamParser* aStreamParser)
    : mStreamParser(aStreamParser)
  {}
  NS_IMETHODIMP Run()
  {
    if (mStreamParser->mFlushTimer) {
      mStreamParser->mFlushTimer->Cancel();
      mStreamParser->mFlushTimer = nullptr;
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

  
  
  mFlushTimerArmed = false;

  mFlushTimerEverFired = true;

  if (IsTerminatedOrInterrupted()) {
    return;
  }

  if (mMode == VIEW_SOURCE_HTML || mMode == VIEW_SOURCE_XML) {
    mTreeBuilder->Flush(); 
    if (mTokenizer->FlushViewSource()) {
       if (NS_FAILED(NS_DispatchToMainThread(mExecutorFlusher))) {
         NS_WARNING("failed to dispatch executor flush event");
       }
     }
  } else {
    
    
    if (mTreeBuilder->Flush(true)) {
      if (NS_FAILED(NS_DispatchToMainThread(mExecutorFlusher))) {
        NS_WARNING("failed to dispatch executor flush event");
      }
    }
  }
}

void
nsHtml5StreamParser::MarkAsBroken(nsresult aRv)
{
  NS_ASSERTION(IsParserThread(), "Wrong thread!");
  mTokenizerMutex.AssertCurrentThreadOwns();

  Terminate();
  mTreeBuilder->MarkAsBroken(aRv);
  mozilla::DebugOnly<bool> hadOps = mTreeBuilder->Flush(false);
  NS_ASSERTION(hadOps, "Should have had the markAsBroken op!");
  if (NS_FAILED(NS_DispatchToMainThread(mExecutorFlusher))) {
    NS_WARNING("failed to dispatch executor flush event");
  }
}
