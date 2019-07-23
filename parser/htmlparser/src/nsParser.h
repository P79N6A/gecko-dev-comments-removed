



































 



































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
#include "nsIUnicharStreamListener.h"
#include "nsCycleCollectionParticipant.h"

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
                 public nsIStreamListener
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
                     PRBool aLastCall,
                     nsDTDMode aMode = eDTDMode_autodetect);

    NS_IMETHOD_(void *) GetRootContextKey();

    


    NS_IMETHOD ParseFragment(const nsAString& aSourceBuffer,
                             void* aKey,
                             nsTArray<nsString>& aTagStack,
                             PRBool aXMLMode,
                             const nsACString& aContentType,
                             nsDTDMode aMode = eDTDMode_autodetect);

    NS_IMETHOD ParseFragment(const nsAString& aSourceBuffer,
                             nsISupports* aTargetNode,
                             nsIAtom* aContextLocalName,
                             PRInt32 aContextNamespace,
                             PRBool aQuirks);
                             
    





    NS_IMETHOD BuildModel(void);

    








    NS_IMETHOD        ContinueParsing();
    NS_IMETHOD        ContinueInterruptedParsing();
    NS_IMETHOD_(void) BlockParser();
    NS_IMETHOD_(void) UnblockParser();
    NS_IMETHOD        Terminate(void);

    





    NS_IMETHOD_(PRBool) IsParserEnabled();

    





    NS_IMETHOD_(PRBool) IsComplete();

    









    void SetUnusedInput(nsString& aBuffer);

    




    virtual nsresult ResumeParse(PRBool allowIteration = PR_TRUE, 
                                 PRBool aIsFinalChunk = PR_FALSE,
                                 PRBool aCanInterrupt = PR_TRUE);

     
      
      
      
    
    NS_DECL_NSIREQUESTOBSERVER

    
    NS_DECL_NSISTREAMLISTENER

    void              PushContext(CParserContext& aContext);
    CParserContext*   PopContext();
    CParserContext*   PeekContext() {return mParserContext;}

    





    NS_IMETHOD GetChannel(nsIChannel** aChannel);

    





    NS_IMETHOD GetDTD(nsIDTD** aDTD);
  
    




    NS_IMETHOD GetStreamListener(nsIStreamListener** aListener);

    



    PRBool DetectMetaTag(const char* aBytes, 
                         PRInt32 aLen, 
                         nsCString& oCharset, 
                         PRInt32& oCharsetSource);

    void SetSinkCharset(nsACString& aCharset);

    




    NS_IMETHODIMP CancelParsingEvents();

    





    virtual PRBool CanInterrupt();

    




    void SetCanInterrupt(PRBool aCanInterrupt);

    







    nsresult PostContinueEvent();

    



    void HandleParserContinueEvent(class nsParserContinueEvent *);

    



    nsresult DataAdded(const nsSubstring& aData, nsIRequest *aRequest);

    static nsCOMArray<nsIUnicharStreamListener> *sParserDataListeners;

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

    PRBool IsScriptExecuting() {
      return mSink && mSink->IsScriptExecuting();
    }

    PRBool IsOkToProcessNetworkData() {
      return !IsScriptExecuting() && !mProcessingNetworkData;
    }

 protected:

    void Initialize(PRBool aConstructor = PR_FALSE);
    void Cleanup();

    





    nsresult WillBuildModel(nsString& aFilename);

    





    nsresult DidBuildModel(nsresult anErrorCode);

    void SpeculativelyParse();

private:

    



    








    PRBool WillTokenize(PRBool aIsFinalChunk = PR_FALSE);

   
    







    nsresult Tokenize(PRBool aIsFinalChunk = PR_FALSE);

    








    PRBool DidTokenize(PRBool aIsFinalChunk = PR_FALSE);

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

    PRBool              mProcessingNetworkData;

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

