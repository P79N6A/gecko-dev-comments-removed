



































 



































#ifndef NS_PARSER__
#define NS_PARSER__

#include "nsIParser.h"
#include "nsDeque.h"
#include "nsParserNode.h"
#include "nsIURL.h"
#include "CParserContext.h"
#include "nsParserCIID.h"
#include "nsITokenizer.h"
#include "nsHTMLTags.h"
#include "nsDTDUtils.h"
#include "nsThreadUtils.h"
#include "nsIContentSink.h"
#include "nsIParserFilter.h"
#include "nsCOMArray.h"
#include "nsCycleCollectionParticipant.h"
#include "nsWeakReference.h"

class nsICharsetConverterManager;
class nsICharsetAlias;
class nsIDTD;
class nsScanner;
class nsSpeculativeScriptThread;
class nsIThreadPool;

#ifdef _MSC_VER
#pragma warning( disable : 4275 )
#endif


class nsParser : public nsIParser,
                 public nsIStreamListener,
                 public nsSupportsWeakReference
{
  public:
    


    static nsresult Init();

    


    static void Shutdown();

    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsParser, nsIParser)

    



    nsParser();

    



    virtual ~nsParser();

    





    NS_IMETHOD_(void) SetContentSink(nsIContentSink* aSink);

    





    NS_IMETHOD_(nsIContentSink*) GetContentSink(void);
    
    








    NS_IMETHOD_(void) GetCommand(nsCString& aCommand);
    NS_IMETHOD_(void) SetCommand(const char* aCommand);
    NS_IMETHOD_(void) SetCommand(eParserCommands aParserCommand);

    








    NS_IMETHOD_(void) SetDocumentCharset(const nsACString& aCharset, PRInt32 aSource);

    NS_IMETHOD_(void) GetDocumentCharset(nsACString& aCharset, PRInt32& aSource)
    {
         aCharset = mCharset;
         aSource = mCharsetSource;
    }


    NS_IMETHOD_(void) SetParserFilter(nsIParserFilter* aFilter);

    






    NS_IMETHOD Parse(nsIURI* aURL,
                     nsIRequestObserver* aListener = nsnull,
                     void* aKey = 0,
                     nsDTDMode aMode = eDTDMode_autodetect);

    





    NS_IMETHOD Parse(const nsAString& aSourceBuffer,
                     void* aKey,
                     const nsACString& aContentType,
                     bool aLastCall,
                     nsDTDMode aMode = eDTDMode_autodetect);

    NS_IMETHOD_(void *) GetRootContextKey();

    


    NS_IMETHOD ParseFragment(const nsAString& aSourceBuffer,
                             nsTArray<nsString>& aTagStack);
                             
    





    NS_IMETHOD BuildModel(void);

    NS_IMETHOD        ContinueInterruptedParsing();
    NS_IMETHOD_(void) BlockParser();
    NS_IMETHOD_(void) UnblockParser();
    NS_IMETHOD        Terminate(void);

    





    NS_IMETHOD_(bool) IsParserEnabled();

    





    NS_IMETHOD_(bool) IsComplete();

    









    void SetUnusedInput(nsString& aBuffer);

    




    virtual nsresult ResumeParse(bool allowIteration = true, 
                                 bool aIsFinalChunk = false,
                                 bool aCanInterrupt = true);

     
      
      
      
    
    NS_DECL_NSIREQUESTOBSERVER

    
    NS_DECL_NSISTREAMLISTENER

    void              PushContext(CParserContext& aContext);
    CParserContext*   PopContext();
    CParserContext*   PeekContext() {return mParserContext;}

    





    NS_IMETHOD GetChannel(nsIChannel** aChannel);

    





    NS_IMETHOD GetDTD(nsIDTD** aDTD);
  
    




    NS_IMETHOD GetStreamListener(nsIStreamListener** aListener);

    



    bool DetectMetaTag(const char* aBytes, 
                         PRInt32 aLen, 
                         nsCString& oCharset, 
                         PRInt32& oCharsetSource);

    void SetSinkCharset(nsACString& aCharset);

    




    NS_IMETHODIMP CancelParsingEvents();

    





    virtual bool CanInterrupt();

    


    virtual bool IsInsertionPointDefined();

    


    virtual void BeginEvaluatingParserInsertedScript();

    


    virtual void EndEvaluatingParserInsertedScript();

    


    virtual void MarkAsNotScriptCreated(const char* aCommand);

    


    virtual bool IsScriptCreated();

    




    void SetCanInterrupt(bool aCanInterrupt);

    







    nsresult PostContinueEvent();

    



    void HandleParserContinueEvent(class nsParserContinueEvent *);

    static nsICharsetAlias* GetCharsetAliasService() {
      return sCharsetAliasService;
    }

    static nsICharsetConverterManager* GetCharsetConverterManager() {
      return sCharsetConverterManager;
    }

    virtual void Reset() {
      Cleanup();
      Initialize();
    }

    nsIThreadPool* ThreadPool() {
      return sSpeculativeThreadPool;
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

    void SpeculativelyParse();

private:

    



    








    bool WillTokenize(bool aIsFinalChunk = false);

   
    







    nsresult Tokenize(bool aIsFinalChunk = false);

    








    bool DidTokenize(bool aIsFinalChunk = false);

protected:
    
    
    
    
      
    CParserContext*              mParserContext;
    nsCOMPtr<nsIDTD>             mDTD;
    nsCOMPtr<nsIRequestObserver> mObserver;
    nsCOMPtr<nsIContentSink>     mSink;
    nsIRunnable*                 mContinueEvent;  
    nsRefPtr<nsSpeculativeScriptThread> mSpeculativeScriptThread;
   
    nsCOMPtr<nsIParserFilter> mParserFilter;
    nsTokenAllocator          mTokenAllocator;
    
    eParserCommands     mCommand;
    nsresult            mInternalState;
    PRInt32             mStreamStatus;
    PRInt32             mCharsetSource;
    
    PRUint16            mFlags;

    nsString            mUnusedInput;
    nsCString           mCharset;
    nsCString           mCommandStr;

    bool                mProcessingNetworkData;

    static nsICharsetAlias*            sCharsetAliasService;
    static nsICharsetConverterManager* sCharsetConverterManager;
    static nsIThreadPool*              sSpeculativeThreadPool;

    enum {
      kSpeculativeThreadLimit = 15,
      kIdleThreadLimit = 0,
      kIdleThreadTimeout = 50
    };
};

#endif 

