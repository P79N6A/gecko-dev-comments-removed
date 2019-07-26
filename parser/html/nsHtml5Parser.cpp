





#include "nsHtml5Parser.h"

#include "mozilla/AutoRestore.h"
#include "nsContentUtils.h" 
#include "nsHtml5Tokenizer.h"
#include "nsHtml5TreeBuilder.h"
#include "nsHtml5AtomTable.h"
#include "nsHtml5DependentUTF16Buffer.h"
#include "nsNetUtil.h"

NS_INTERFACE_TABLE_HEAD(nsHtml5Parser)
  NS_INTERFACE_TABLE(nsHtml5Parser, nsIParser, nsISupportsWeakReference)
  NS_INTERFACE_TABLE_TO_MAP_SEGUE_CYCLE_COLLECTION(nsHtml5Parser)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsHtml5Parser)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsHtml5Parser)

NS_IMPL_CYCLE_COLLECTION_CLASS(nsHtml5Parser)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsHtml5Parser)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mExecutor)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_RAWPTR(GetStreamParser())
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsHtml5Parser)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mExecutor)
  tmp->DropStreamParser();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

nsHtml5Parser::nsHtml5Parser()
  : mFirstBuffer(new nsHtml5OwningUTF16Buffer((void*)nullptr))
  , mLastBuffer(mFirstBuffer)
  , mExecutor(new nsHtml5TreeOpExecutor())
  , mTreeBuilder(new nsHtml5TreeBuilder(mExecutor, nullptr))
  , mTokenizer(new nsHtml5Tokenizer(mTreeBuilder, false))
  , mRootContextLineNumber(1)
{
  mTokenizer->setInterner(&mAtomTable);
  
}

nsHtml5Parser::~nsHtml5Parser()
{
  mTokenizer->end();
  if (mDocWriteSpeculativeTokenizer) {
    mDocWriteSpeculativeTokenizer->end();
  }
}

NS_IMETHODIMP_(void)
nsHtml5Parser::SetContentSink(nsIContentSink* aSink)
{
  NS_ASSERTION(aSink == static_cast<nsIContentSink*> (mExecutor), 
               "Attempt to set a foreign sink.");
}

NS_IMETHODIMP_(nsIContentSink*)
nsHtml5Parser::GetContentSink()
{
  return static_cast<nsIContentSink*> (mExecutor);
}

NS_IMETHODIMP_(void)
nsHtml5Parser::GetCommand(nsCString& aCommand)
{
  aCommand.Assign("view");
}

NS_IMETHODIMP_(void)
nsHtml5Parser::SetCommand(const char* aCommand)
{
  NS_ASSERTION(!strcmp(aCommand, "view") ||
               !strcmp(aCommand, "view-source") ||
               !strcmp(aCommand, "external-resource") ||
               !strcmp(aCommand, "import") ||
               !strcmp(aCommand, kLoadAsData),
               "Unsupported parser command");
}

NS_IMETHODIMP_(void)
nsHtml5Parser::SetCommand(eParserCommands aParserCommand)
{
  NS_ASSERTION(aParserCommand == eViewNormal, 
               "Parser command was not eViewNormal.");
}

NS_IMETHODIMP_(void)
nsHtml5Parser::SetDocumentCharset(const nsACString& aCharset,
                                  int32_t aCharsetSource)
{
  NS_PRECONDITION(!mExecutor->HasStarted(),
                  "Document charset set too late.");
  NS_PRECONDITION(GetStreamParser(), "Setting charset on a script-only parser.");
  nsAutoCString trimmed;
  trimmed.Assign(aCharset);
  trimmed.Trim(" \t\r\n\f");
  GetStreamParser()->SetDocumentCharset(trimmed, aCharsetSource);
  mExecutor->SetDocumentCharsetAndSource(trimmed,
                                         aCharsetSource);
}

NS_IMETHODIMP
nsHtml5Parser::GetChannel(nsIChannel** aChannel)
{
  if (GetStreamParser()) {
    return GetStreamParser()->GetChannel(aChannel);
  } else {
    return NS_ERROR_NOT_AVAILABLE;
  }
}

NS_IMETHODIMP
nsHtml5Parser::GetDTD(nsIDTD** aDTD)
{
  *aDTD = nullptr;
  return NS_OK;
}

nsIStreamListener*
nsHtml5Parser::GetStreamListener()
{
  return mStreamListener;
}

NS_IMETHODIMP
nsHtml5Parser::ContinueInterruptedParsing()
{
  NS_NOTREACHED("Don't call. For interface compat only.");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP_(void)
nsHtml5Parser::BlockParser()
{
  mBlocked = true;
}

NS_IMETHODIMP_(void)
nsHtml5Parser::UnblockParser()
{
  mBlocked = false;
  mExecutor->ContinueInterruptedParsingAsync();
}

NS_IMETHODIMP_(void)
nsHtml5Parser::ContinueInterruptedParsingAsync()
{
  mExecutor->ContinueInterruptedParsingAsync();
}

NS_IMETHODIMP_(bool)
nsHtml5Parser::IsParserEnabled()
{
  return !mBlocked;
}

NS_IMETHODIMP_(bool)
nsHtml5Parser::IsComplete()
{
  return mExecutor->IsComplete();
}

NS_IMETHODIMP
nsHtml5Parser::Parse(nsIURI* aURL,
                     nsIRequestObserver* aObserver,
                     void* aKey, 
                     nsDTDMode aMode) 
{
  



  NS_PRECONDITION(!mExecutor->HasStarted(), 
                  "Tried to start parse without initializing the parser.");
  NS_PRECONDITION(GetStreamParser(),
                  "Can't call this Parse() variant on script-created parser");
  GetStreamParser()->SetObserver(aObserver);
  GetStreamParser()->SetViewSourceTitle(aURL); 
  mExecutor->SetStreamParser(GetStreamParser());
  mExecutor->SetParser(this);
  return NS_OK;
}

NS_IMETHODIMP
nsHtml5Parser::Parse(const nsAString& aSourceBuffer,
                     void* aKey,
                     const nsACString& aContentType,
                     bool aLastCall,
                     nsDTDMode aMode) 
{
  nsresult rv;
  if (NS_FAILED(rv = mExecutor->IsBroken())) {
    return rv;
  }
  if (aSourceBuffer.Length() > INT32_MAX) {
    return mExecutor->MarkAsBroken(NS_ERROR_OUT_OF_MEMORY);
  }

  
  
  nsCOMPtr<nsIParser> kungFuDeathGrip(this);
  
  
  
  nsRefPtr<nsHtml5StreamParser> streamKungFuDeathGrip(GetStreamParser());
  nsRefPtr<nsHtml5TreeOpExecutor> treeOpKungFuDeathGrip(mExecutor);

  if (!mExecutor->HasStarted()) {
    NS_ASSERTION(!GetStreamParser(),
                 "Had stream parser but document.write started life cycle.");
    
    mExecutor->SetParser(this);
    mTreeBuilder->setScriptingEnabled(mExecutor->IsScriptEnabled());

    bool isSrcdoc = false;
    nsCOMPtr<nsIChannel> channel;
    rv = GetChannel(getter_AddRefs(channel));
    if (NS_SUCCEEDED(rv)) {
      isSrcdoc = NS_IsSrcdocChannel(channel);
    }
    mTreeBuilder->setIsSrcdocDocument(isSrcdoc);

    mTokenizer->start();
    mExecutor->Start();
    if (!aContentType.EqualsLiteral("text/html")) {
      mTreeBuilder->StartPlainText();
      mTokenizer->StartPlainText();
    }
    




    mExecutor->WillBuildModel(eDTDMode_unknown);
  }

  
  if (mExecutor->IsComplete()) {
    return NS_OK;
  }

  if (aLastCall && aSourceBuffer.IsEmpty() && !aKey) {
    
    NS_ASSERTION(!GetStreamParser(),
                 "Had stream parser but got document.close().");
    if (mDocumentClosed) {
      
      return NS_OK;
    }
    mDocumentClosed = true;
    if (!mBlocked && !mInDocumentWrite) {
      ParseUntilBlocked();
    }
    return NS_OK;
  }

  
  

  NS_ASSERTION(IsInsertionPointDefined(),
               "Doc.write reached parser with undefined insertion point.");

  NS_ASSERTION(!(GetStreamParser() && !aKey),
               "Got a null key in a non-script-created parser");

  
  if (aSourceBuffer.IsEmpty()) {
    return NS_OK;
  }

  
  
  
  mozilla::AutoRestore<bool> guard(mInDocumentWrite);
  mInDocumentWrite = true;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  
  
  
  
  
  
  
  nsRefPtr<nsHtml5OwningUTF16Buffer> prevSearchBuf;
  nsRefPtr<nsHtml5OwningUTF16Buffer> firstLevelMarker;

  if (aKey) {
    if (mFirstBuffer == mLastBuffer) {
      nsHtml5OwningUTF16Buffer* keyHolder = new nsHtml5OwningUTF16Buffer(aKey);
      keyHolder->next = mLastBuffer;
      mFirstBuffer = keyHolder;
    } else if (mFirstBuffer->key != aKey) {
      prevSearchBuf = mFirstBuffer;
      for (;;) {
        if (prevSearchBuf->next == mLastBuffer) {
          
          nsHtml5OwningUTF16Buffer* keyHolder =
            new nsHtml5OwningUTF16Buffer(aKey);
          keyHolder->next = mFirstBuffer;
          mFirstBuffer = keyHolder;
          prevSearchBuf = nullptr;
          break;
        }
        if (prevSearchBuf->next->key == aKey) {
          
          break;
        }
        prevSearchBuf = prevSearchBuf->next;
      }
    } 

    
    
  } else {
    
    
    
    
    
    mLastBuffer->next = new nsHtml5OwningUTF16Buffer((void*)nullptr);
    firstLevelMarker = mLastBuffer;
    mLastBuffer = mLastBuffer->next;
  }

  nsHtml5DependentUTF16Buffer stackBuffer(aSourceBuffer);

  while (!mBlocked && stackBuffer.hasMore()) {
    stackBuffer.adjust(mLastWasCR);
    mLastWasCR = false;
    if (stackBuffer.hasMore()) {
      int32_t lineNumberSave;
      bool inRootContext = (!GetStreamParser() && !aKey);
      if (inRootContext) {
        mTokenizer->setLineNumber(mRootContextLineNumber);
      } else {
        
        
        lineNumberSave = mTokenizer->getLineNumber();
      }

      mLastWasCR = mTokenizer->tokenizeBuffer(&stackBuffer);

      if (inRootContext) {
        mRootContextLineNumber = mTokenizer->getLineNumber();
      } else {
        mTokenizer->setLineNumber(lineNumberSave);
      }

      if (mTreeBuilder->HasScript()) {
        mTreeBuilder->Flush(); 
        mExecutor->FlushDocumentWrite(); 
        
        
        if (mExecutor->IsComplete()) {
          return NS_OK;
        }
      }
      
    }
  }

  nsRefPtr<nsHtml5OwningUTF16Buffer> heapBuffer;
  if (stackBuffer.hasMore()) {
    
    
    heapBuffer = stackBuffer.FalliblyCopyAsOwningBuffer();
    if (!heapBuffer) {
      
      return mExecutor->MarkAsBroken(NS_ERROR_OUT_OF_MEMORY);
    }
  }

  if (heapBuffer) {
    
    
    
    if (aKey) {
      NS_ASSERTION(mFirstBuffer != mLastBuffer,
        "Where's the keyholder?");
      
      
      if (mFirstBuffer->key == aKey) {
        NS_ASSERTION(!prevSearchBuf,
          "Non-null prevSearchBuf when mFirstBuffer is the key holder?");
        heapBuffer->next = mFirstBuffer;
        mFirstBuffer = heapBuffer;
      } else {
        if (!prevSearchBuf) {
          prevSearchBuf = mFirstBuffer;
        }
        
        
        while (prevSearchBuf->next->key != aKey) {
          prevSearchBuf = prevSearchBuf->next;
        }
        heapBuffer->next = prevSearchBuf->next;
        prevSearchBuf->next = heapBuffer;
      }
    } else {
      NS_ASSERTION(firstLevelMarker, "How come we don't have a marker.");
      firstLevelMarker->Swap(heapBuffer);
    }
  }

  if (!mBlocked) { 
    NS_ASSERTION(!stackBuffer.hasMore(),
      "Buffer wasn't tokenized to completion?");
    
    mTreeBuilder->Flush(); 
    mExecutor->FlushDocumentWrite(); 
  } else if (stackBuffer.hasMore()) {
    
    
    
    if (!mDocWriteSpeculatorActive) {
      mDocWriteSpeculatorActive = true;
      if (!mDocWriteSpeculativeTreeBuilder) {
        
        mDocWriteSpeculativeTreeBuilder =
            new nsHtml5TreeBuilder(nullptr, mExecutor->GetStage());
        mDocWriteSpeculativeTreeBuilder->setScriptingEnabled(
            mTreeBuilder->isScriptingEnabled());
        mDocWriteSpeculativeTokenizer =
            new nsHtml5Tokenizer(mDocWriteSpeculativeTreeBuilder, false);
        mDocWriteSpeculativeTokenizer->setInterner(&mAtomTable);
        mDocWriteSpeculativeTokenizer->start();
      }
      mDocWriteSpeculativeTokenizer->resetToDataState();
      mDocWriteSpeculativeTreeBuilder->loadState(mTreeBuilder, &mAtomTable);
      mDocWriteSpeculativeLastWasCR = false;
    }

    
    
    
    
    

    
    
    
    while (stackBuffer.hasMore()) {
      stackBuffer.adjust(mDocWriteSpeculativeLastWasCR);
      if (stackBuffer.hasMore()) {
        mDocWriteSpeculativeLastWasCR =
            mDocWriteSpeculativeTokenizer->tokenizeBuffer(&stackBuffer);
      }
    }

    mDocWriteSpeculativeTreeBuilder->Flush();
    mDocWriteSpeculativeTreeBuilder->DropHandles();
    mExecutor->FlushSpeculativeLoads();
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHtml5Parser::Terminate()
{
  
  
  if (mExecutor->IsComplete()) {
    return NS_OK;
  }
  
  
  nsCOMPtr<nsIParser> kungFuDeathGrip(this);
  nsRefPtr<nsHtml5StreamParser> streamKungFuDeathGrip(GetStreamParser());
  nsRefPtr<nsHtml5TreeOpExecutor> treeOpKungFuDeathGrip(mExecutor);
  if (GetStreamParser()) {
    GetStreamParser()->Terminate();
  }
  return mExecutor->DidBuildModel(true);
}

NS_IMETHODIMP
nsHtml5Parser::ParseFragment(const nsAString& aSourceBuffer,
                             nsTArray<nsString>& aTagStack)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHtml5Parser::BuildModel()
{
  NS_NOTREACHED("Don't call this!");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHtml5Parser::CancelParsingEvents()
{
  NS_NOTREACHED("Don't call this!");
  return NS_ERROR_NOT_IMPLEMENTED;
}

void
nsHtml5Parser::Reset()
{
  NS_NOTREACHED("Don't call this!");
}

bool
nsHtml5Parser::CanInterrupt()
{
  
  
  return true;
}

bool
nsHtml5Parser::IsInsertionPointDefined()
{
  return !mExecutor->IsFlushing() &&
    (!GetStreamParser() || mParserInsertedScriptsBeingEvaluated);
}

void
nsHtml5Parser::BeginEvaluatingParserInsertedScript()
{
  ++mParserInsertedScriptsBeingEvaluated;
}

void
nsHtml5Parser::EndEvaluatingParserInsertedScript()
{
  --mParserInsertedScriptsBeingEvaluated;
}

void
nsHtml5Parser::MarkAsNotScriptCreated(const char* aCommand)
{
  NS_PRECONDITION(!mStreamListener, "Must not call this twice.");
  eParserMode mode = NORMAL;
  if (!nsCRT::strcmp(aCommand, "view-source")) {
    mode = VIEW_SOURCE_HTML;
  } else if (!nsCRT::strcmp(aCommand, "view-source-xml")) {
    mode = VIEW_SOURCE_XML;
  } else if (!nsCRT::strcmp(aCommand, "view-source-plain")) {
    mode = VIEW_SOURCE_PLAIN;
  } else if (!nsCRT::strcmp(aCommand, "plain-text")) {
    mode = PLAIN_TEXT;
  } else if (!nsCRT::strcmp(aCommand, kLoadAsData)) {
    mode = LOAD_AS_DATA;
  }
#ifdef DEBUG
  else {
    NS_ASSERTION(!nsCRT::strcmp(aCommand, "view") ||
                 !nsCRT::strcmp(aCommand, "external-resource") ||
                 !nsCRT::strcmp(aCommand, "import"),
                 "Unsupported parser command!");
  }
#endif
  mStreamListener =
    new nsHtml5StreamListener(new nsHtml5StreamParser(mExecutor, this, mode));
}

bool
nsHtml5Parser::IsScriptCreated()
{
  return !GetStreamParser();
}




void
nsHtml5Parser::ParseUntilBlocked()
{
  if (mBlocked || mExecutor->IsComplete() || NS_FAILED(mExecutor->IsBroken())) {
    return;
  }
  NS_ASSERTION(mExecutor->HasStarted(), "Bad life cycle.");
  NS_ASSERTION(!mInDocumentWrite,
    "ParseUntilBlocked entered while in doc.write!");

  mDocWriteSpeculatorActive = false;

  for (;;) {
    if (!mFirstBuffer->hasMore()) {
      if (mFirstBuffer == mLastBuffer) {
        if (mExecutor->IsComplete()) {
          
          return;
        }
        if (mDocumentClosed) {
          NS_ASSERTION(!GetStreamParser(),
                       "This should only happen with script-created parser.");
          mTokenizer->eof();
          mTreeBuilder->StreamEnded();
          mTreeBuilder->Flush();
          mExecutor->FlushDocumentWrite();
          mTokenizer->end();
          return;            
        }
        
        NS_ASSERTION(!mLastBuffer->getStart() && !mLastBuffer->getEnd(),
                     "Sentinel buffer had its indeces changed.");
        if (GetStreamParser()) {
          if (mReturnToStreamParserPermitted &&
              !mExecutor->IsScriptExecuting()) {
            mTreeBuilder->Flush();
            mReturnToStreamParserPermitted = false;
            GetStreamParser()->ContinueAfterScripts(mTokenizer,
                                                mTreeBuilder,
                                                mLastWasCR);
          }
        } else {
          
          mTreeBuilder->Flush();
          
          
          NS_ASSERTION(mExecutor->IsInFlushLoop(),
              "How did we come here without being in the flush loop?");
        }
        return; 
      }
      mFirstBuffer = mFirstBuffer->next;
      continue;
    }

    if (mBlocked || mExecutor->IsComplete()) {
      return;
    }

    
    mFirstBuffer->adjust(mLastWasCR);
    mLastWasCR = false;
    if (mFirstBuffer->hasMore()) {
      bool inRootContext = (!GetStreamParser() && !mFirstBuffer->key);
      if (inRootContext) {
        mTokenizer->setLineNumber(mRootContextLineNumber);
      }
      mLastWasCR = mTokenizer->tokenizeBuffer(mFirstBuffer);
      if (inRootContext) {
        mRootContextLineNumber = mTokenizer->getLineNumber();
      }
      if (mTreeBuilder->HasScript()) {
        mTreeBuilder->Flush();
        mExecutor->FlushDocumentWrite();
      }
      if (mBlocked) {
        return;
      }
    }
    continue;
  }
}

nsresult
nsHtml5Parser::Initialize(nsIDocument* aDoc,
                          nsIURI* aURI,
                          nsISupports* aContainer,
                          nsIChannel* aChannel)
{
  return mExecutor->Init(aDoc, aURI, aContainer, aChannel);
}

void
nsHtml5Parser::StartTokenizer(bool aScriptingEnabled) {

  bool isSrcdoc = false;
  nsCOMPtr<nsIChannel> channel;
  nsresult rv = GetChannel(getter_AddRefs(channel));
  if (NS_SUCCEEDED(rv)) {
    isSrcdoc = NS_IsSrcdocChannel(channel);
  }
  mTreeBuilder->setIsSrcdocDocument(isSrcdoc);

  mTreeBuilder->SetPreventScriptExecution(!aScriptingEnabled);
  mTreeBuilder->setScriptingEnabled(aScriptingEnabled);
  mTokenizer->start();
}

void
nsHtml5Parser::InitializeDocWriteParserState(nsAHtml5TreeBuilderState* aState,
                                             int32_t aLine)
{
  mTokenizer->resetToDataState();
  mTokenizer->setLineNumber(aLine);
  mTreeBuilder->loadState(aState, &mAtomTable);
  mLastWasCR = false;
  mReturnToStreamParserPermitted = true;
}

void
nsHtml5Parser::ContinueAfterFailedCharsetSwitch()
{
  NS_PRECONDITION(GetStreamParser(),
    "Tried to continue after failed charset switch without a stream parser");
  GetStreamParser()->ContinueAfterFailedCharsetSwitch();
}

