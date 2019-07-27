



 



































#ifndef NS_PARSER__
#define NS_PARSER__

#include "nsIParser.h"
#include "nsDeque.h"
#include "nsIURL.h"
#include "CParserContext.h"
#include "nsParserCIID.h"
#include "nsITokenizer.h"
#include "nsHTMLTags.h"
#include "nsIContentSink.h"
#include "nsCOMArray.h"
#include "nsCycleCollectionParticipant.h"
#include "nsWeakReference.h"

class nsIDTD;
class nsIRunnable;

#ifdef _MSC_VER
#pragma warning( disable : 4275 )
#endif


class nsParser final : public nsIParser,
                       public nsIStreamListener,
                       public nsSupportsWeakReference
{
    



    virtual ~nsParser();

  public:
    


    static nsresult Init();

    


    static void Shutdown();

    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsParser, nsIParser)

    



    nsParser();

    





    NS_IMETHOD_(void) SetContentSink(nsIContentSink* aSink) override;

    





    NS_IMETHOD_(nsIContentSink*) GetContentSink(void) override;
    
    








    NS_IMETHOD_(void) GetCommand(nsCString& aCommand) override;
    NS_IMETHOD_(void) SetCommand(const char* aCommand) override;
    NS_IMETHOD_(void) SetCommand(eParserCommands aParserCommand) override;

    








    NS_IMETHOD_(void) SetDocumentCharset(const nsACString& aCharset, int32_t aSource) override;

    NS_IMETHOD_(void) GetDocumentCharset(nsACString& aCharset, int32_t& aSource) override
    {
         aCharset = mCharset;
         aSource = mCharsetSource;
    }

    






    NS_IMETHOD Parse(nsIURI* aURL,
                     nsIRequestObserver* aListener = nullptr,
                     void* aKey = 0,
                     nsDTDMode aMode = eDTDMode_autodetect) override;

    


    NS_IMETHOD ParseFragment(const nsAString& aSourceBuffer,
                             nsTArray<nsString>& aTagStack) override;
                             
    





    NS_IMETHOD BuildModel(void) override;

    NS_IMETHOD        ContinueInterruptedParsing() override;
    NS_IMETHOD_(void) BlockParser() override;
    NS_IMETHOD_(void) UnblockParser() override;
    NS_IMETHOD_(void) ContinueInterruptedParsingAsync() override;
    NS_IMETHOD        Terminate(void) override;

    





    NS_IMETHOD_(bool) IsParserEnabled() override;

    





    NS_IMETHOD_(bool) IsComplete() override;

    









    void SetUnusedInput(nsString& aBuffer);

    




    virtual nsresult ResumeParse(bool allowIteration = true, 
                                 bool aIsFinalChunk = false,
                                 bool aCanInterrupt = true);

     
      
      
      
    
    NS_DECL_NSIREQUESTOBSERVER

    
    NS_DECL_NSISTREAMLISTENER

    void              PushContext(CParserContext& aContext);
    CParserContext*   PopContext();
    CParserContext*   PeekContext() {return mParserContext;}

    





    NS_IMETHOD GetChannel(nsIChannel** aChannel) override;

    





    NS_IMETHOD GetDTD(nsIDTD** aDTD) override;
  
    


    virtual nsIStreamListener* GetStreamListener() override;

    void SetSinkCharset(nsACString& aCharset);

    




    NS_IMETHODIMP CancelParsingEvents() override;

    


    virtual bool IsInsertionPointDefined() override;

    


    virtual void BeginEvaluatingParserInsertedScript() override;

    


    virtual void EndEvaluatingParserInsertedScript() override;

    


    virtual void MarkAsNotScriptCreated(const char* aCommand) override;

    


    virtual bool IsScriptCreated() override;

    




    void SetCanInterrupt(bool aCanInterrupt);

    







    nsresult PostContinueEvent();

    



    void HandleParserContinueEvent(class nsParserContinueEvent *);

    virtual void Reset() override {
      Cleanup();
      Initialize();
    }

    bool IsScriptExecuting() {
      return mSink && mSink->IsScriptExecuting();
    }

    bool IsOkToProcessNetworkData() {
      return !IsScriptExecuting() && !mProcessingNetworkData;
    }

 protected:

    void Initialize(bool aConstructor = false);
    void Cleanup();

    





    nsresult WillBuildModel(nsString& aFilename);

    





    nsresult DidBuildModel(nsresult anErrorCode);

private:

    



    








    bool WillTokenize(bool aIsFinalChunk = false);

   
    







    nsresult Tokenize(bool aIsFinalChunk = false);

    


    nsresult Parse(const nsAString& aSourceBuffer,
                   void* aKey,
                   bool aLastCall);

protected:
    
    
    
    
      
    CParserContext*              mParserContext;
    nsCOMPtr<nsIDTD>             mDTD;
    nsCOMPtr<nsIRequestObserver> mObserver;
    nsCOMPtr<nsIContentSink>     mSink;
    nsIRunnable*                 mContinueEvent;  

    eParserCommands     mCommand;
    nsresult            mInternalState;
    nsresult            mStreamStatus;
    int32_t             mCharsetSource;
    
    uint16_t            mFlags;

    nsString            mUnusedInput;
    nsCString           mCharset;
    nsCString           mCommandStr;

    bool                mProcessingNetworkData;
    bool                mIsAboutBlank;
};

#endif 

