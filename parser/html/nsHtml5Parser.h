




#ifndef NS_HTML5_PARSER
#define NS_HTML5_PARSER

#include "nsAutoPtr.h"
#include "nsIParser.h"
#include "nsDeque.h"
#include "nsIURL.h"
#include "nsParserCIID.h"
#include "nsITokenizer.h"
#include "nsIContentSink.h"
#include "nsIRequest.h"
#include "nsIChannel.h"
#include "nsCOMArray.h"
#include "nsContentSink.h"
#include "nsCycleCollectionParticipant.h"
#include "nsIInputStream.h"
#include "nsDetectionConfident.h"
#include "nsHtml5OwningUTF16Buffer.h"
#include "nsHtml5TreeOpExecutor.h"
#include "nsHtml5StreamParser.h"
#include "nsHtml5AtomTable.h"
#include "nsWeakReference.h"
#include "nsHtml5StreamListener.h"

class nsHtml5Parser MOZ_FINAL : public nsIParser,
                                public nsSupportsWeakReference
{
  public:
    NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS

    NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsHtml5Parser, nsIParser)

    nsHtml5Parser();

    
    


    NS_IMETHOD_(void) SetContentSink(nsIContentSink* aSink);

    


    NS_IMETHOD_(nsIContentSink*) GetContentSink();

    


    NS_IMETHOD_(void) GetCommand(nsCString& aCommand);

    


    NS_IMETHOD_(void) SetCommand(const char* aCommand);

    


    NS_IMETHOD_(void) SetCommand(eParserCommands aParserCommand);

    






    NS_IMETHOD_(void) SetDocumentCharset(const nsACString& aCharset, int32_t aSource);

    


    NS_IMETHOD_(void) GetDocumentCharset(nsACString& aCharset, int32_t& aSource)
    {
      NS_NOTREACHED("No one should call this.");
    }

    




    NS_IMETHOD GetChannel(nsIChannel** aChannel);

    


    NS_IMETHOD GetDTD(nsIDTD** aDTD);

    


    virtual nsIStreamListener* GetStreamListener();

    


    NS_IMETHOD ContinueInterruptedParsing();

    


    NS_IMETHOD_(void) BlockParser();

    


    NS_IMETHOD_(void) UnblockParser();

    


    NS_IMETHOD_(void) ContinueInterruptedParsingAsync();

    


    NS_IMETHOD_(bool) IsParserEnabled();

    


    NS_IMETHOD_(bool) IsComplete();

    







    NS_IMETHOD Parse(nsIURI* aURL,
                     nsIRequestObserver* aListener = nullptr,
                     void* aKey = 0,
                     nsDTDMode aMode = eDTDMode_autodetect);

    








    NS_IMETHOD Parse(const nsAString& aSourceBuffer,
                     void* aKey,
                     const nsACString& aContentType,
                     bool aLastCall,
                     nsDTDMode aMode = eDTDMode_autodetect);

    


    NS_IMETHOD Terminate();

    


    NS_IMETHOD ParseFragment(const nsAString& aSourceBuffer,
                             nsTArray<nsString>& aTagStack);

    


    NS_IMETHOD BuildModel();

    


    NS_IMETHODIMP CancelParsingEvents();

    


    virtual void Reset();
    
    


    virtual bool CanInterrupt();

    


    virtual bool IsInsertionPointDefined();

    


    virtual void BeginEvaluatingParserInsertedScript();

    


    virtual void EndEvaluatingParserInsertedScript();

    






    virtual void MarkAsNotScriptCreated(const char* aCommand);

    


    virtual bool IsScriptCreated();

    

    
    

  public:

    


    virtual nsresult Initialize(nsIDocument* aDoc,
                        nsIURI* aURI,
                        nsISupports* aContainer,
                        nsIChannel* aChannel);

    inline nsHtml5Tokenizer* GetTokenizer() {
      return mTokenizer;
    }

    void InitializeDocWriteParserState(nsAHtml5TreeBuilderState* aState, int32_t aLine);

    void DropStreamParser()
    {
      if (GetStreamParser()) {
        GetStreamParser()->DropTimer();
        mStreamListener->DropDelegate();
        mStreamListener = nullptr;
      }
    }
    
    void StartTokenizer(bool aScriptingEnabled);
    
    void ContinueAfterFailedCharsetSwitch();

    nsHtml5StreamParser* GetStreamParser()
    {
      if (!mStreamListener) {
        return nullptr;
      }
      return mStreamListener->GetDelegate();
    }

    


    void ParseUntilBlocked();

  private:

    virtual ~nsHtml5Parser();

    

    


    bool                          mLastWasCR;

    



    bool                          mDocWriteSpeculativeLastWasCR;

    


    bool                          mBlocked;

    


    bool                          mDocWriteSpeculatorActive;
    
    


    int32_t                       mParserInsertedScriptsBeingEvaluated;

    


    bool                          mDocumentClosed;

    bool                          mInDocumentWrite;

    
    


    nsRefPtr<nsHtml5OwningUTF16Buffer>  mFirstBuffer;

    



    nsHtml5OwningUTF16Buffer* mLastBuffer; 

    


    nsRefPtr<nsHtml5TreeOpExecutor>     mExecutor;

    


    const nsAutoPtr<nsHtml5TreeBuilder> mTreeBuilder;

    


    const nsAutoPtr<nsHtml5Tokenizer>   mTokenizer;

    


    nsAutoPtr<nsHtml5TreeBuilder> mDocWriteSpeculativeTreeBuilder;

    


    nsAutoPtr<nsHtml5Tokenizer>   mDocWriteSpeculativeTokenizer;

    


    nsRefPtr<nsHtml5StreamListener>     mStreamListener;

    


    int32_t                             mRootContextLineNumber;
    
    


    bool                                mReturnToStreamParserPermitted;

    


    nsHtml5AtomTable                    mAtomTable;

};
#endif
