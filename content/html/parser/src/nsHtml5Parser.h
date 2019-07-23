




































 
#ifndef NS_HTML5_PARSER__
#define NS_HTML5_PARSER__

#include "nsTimer.h"
#include "nsIParser.h"
#include "nsDeque.h"
#include "nsIURL.h"
#include "nsParserCIID.h"
#include "nsITokenizer.h"
#include "nsThreadUtils.h"
#include "nsIContentSink.h"
#include "nsIParserFilter.h"
#include "nsIRequest.h"
#include "nsIChannel.h"
#include "nsCOMArray.h"
#include "nsContentSink.h"
#include "nsIHTMLDocument.h"
#include "nsIUnicharStreamListener.h"
#include "nsCycleCollectionParticipant.h"
#include "nsAutoPtr.h"
#include "nsIInputStream.h"
#include "nsIUnicodeDecoder.h"
#include "nsHtml5UTF16Buffer.h"

#define NS_HTML5_PARSER_CID \
  {0x3113adb0, 0xe56d, 0x459e, \
    {0xb9, 0x5b, 0xf1, 0xf2, 0x4a, 0xba, 0x2a, 0x80}}

#define NS_HTML5_PARSER_READ_BUFFER_SIZE 1024

enum eHtml5ParserLifecycle {
  NOT_STARTED = 0,
  PARSING = 1,
  STREAM_ENDING = 2,
  TERMINATED = 3,
};

class nsHtml5Parser : public nsIParser,
                      public nsIStreamListener,
                      public nsIContentSink,
                      public nsContentSink {

  
  public:
    NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW
    NS_DECL_ISUPPORTS
    
    nsHtml5Parser();

    virtual ~nsHtml5Parser();

    

    


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


    





    NS_IMETHOD GetChannel(nsIChannel** aChannel);
    
    


    NS_IMETHOD GetDTD(nsIDTD** aDTD);
    
    NS_IMETHOD        ContinueParsing();
    NS_IMETHOD        ContinueInterruptedParsing();
    NS_IMETHOD_(void) BlockParser();
    NS_IMETHOD_(void) UnblockParser();
    
    





    NS_IMETHOD_(PRBool) IsParserEnabled();

    





    NS_IMETHOD_(PRBool) IsComplete();
    
    






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

    NS_IMETHOD        Terminate(void);

    


    NS_IMETHOD ParseFragment(const nsAString& aSourceBuffer,
                             void* aKey,
                             nsTArray<nsString>& aTagStack,
                             PRBool aXMLMode,
                             const nsACString& aContentType,
                             nsDTDMode aMode = eDTDMode_autodetect);


    





    NS_IMETHOD BuildModel(void);

    





    NS_IMETHOD_(nsDTDMode) GetParseMode(void);

    




    NS_IMETHODIMP CancelParsingEvents();
    
    virtual void Reset();
    
    




    virtual void ScriptExecuting();

    



    virtual void ScriptDidExecute();
    
    

     
      
      
      
    
    NS_DECL_NSIREQUESTOBSERVER

    
    NS_DECL_NSISTREAMLISTENER
  
    



    void HandleParserContinueEvent(class nsHtml5ParserContinueEvent *);
    
    
    
    void internalEncodingDeclaration(nsString* aEncoding);
    
    

    void documentMode(nsHtml5DocumentMode m);

    

    NS_IMETHOD WillParse();
    NS_IMETHOD WillBuildModel();
    NS_IMETHOD DidBuildModel();
    NS_IMETHOD WillInterrupt();
    NS_IMETHOD WillResume();
    NS_IMETHOD SetParser(nsIParser* aParser);
    virtual void FlushPendingNotifications(mozFlushType aType);
    NS_IMETHOD SetDocumentCharset(nsACString& aCharset);
    virtual nsISupports *GetTarget();
  
    
  public:
    
    virtual nsresult Initialize(nsIDocument* aDoc,
                        nsIURI* aURI,
                        nsISupports* aContainer,
                        nsIChannel* aChannel);
    virtual nsresult ProcessBASETag(nsIContent* aContent);
    virtual void UpdateChildCounts();
    virtual nsresult FlushTags();
    
    
    NS_IMETHOD WriteStreamBytes(const char* aFromSegment,
                                PRUint32 aCount,
                                PRUint32* aWriteCount);
    void Suspend();
    void SetScriptElement(nsIContent* aScript);
    void UpdateStyleSheet(nsIContent* aElement);
    
    nsIDocument* GetDocument() {
      return mDocument;
    }
    nsNodeInfoManager* GetNodeInfoManager() {
      return mNodeInfoManager;
    }
    nsIDocShell* GetDocShell() {
      return mDocShell;
    }
  private:
    void ExecuteScript();
    void MaybePostContinueEvent();
    


    void ParseUntilSuspend();
    void Cleanup();
  
  private:
    
    PRBool                       mNeedsCharsetSwitch;
    PRBool                       mLastWasCR;
    PRBool                       mTerminated;
    PRBool                       mLayoutStarted;
    PRBool                       mFragmentMode;
    PRBool                       mBlocked;
    PRBool                       mSuspending;
    eHtml5ParserLifecycle        mLifeCycle;
    eStreamState                 mStreamListenerState;

    
    nsCOMPtr<nsIContent>         mScriptElement;
    PRUint32                     mScriptsExecuting;
     
    
    void*                        mRootContextKey;
    nsCOMPtr<nsIRequest>         mRequest; 
    nsCOMPtr<nsIRequestObserver> mObserver;
    nsIRunnable*                 mContinueEvent;  
 
    
    nsIContent*                  mDocElement; 
  
    
    PRInt32                      mCharsetSource;
    nsCString                    mCharset;
    nsCOMPtr<nsIUnicodeDecoder>  mUnicodeDecoder;
        
    
    nsHtml5UTF16Buffer*          mFirstBuffer; 
    nsHtml5UTF16Buffer*          mLastBuffer; 
                      
    nsHtml5TreeBuilder*          mTreeBuilder; 
    nsHtml5Tokenizer*            mTokenizer; 
};

#endif 

