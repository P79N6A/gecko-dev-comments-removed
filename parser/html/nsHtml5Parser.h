





































#ifndef NS_HTML5_PARSER__
#define NS_HTML5_PARSER__

#include "nsAutoPtr.h"
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
#include "nsICharsetDetectionObserver.h"
#include "nsDetectionConfident.h"
#include "nsHtml5UTF16Buffer.h"
#include "nsHtml5MetaScanner.h"

#define NS_HTML5_PARSER_READ_BUFFER_SIZE 1024
#define NS_HTML5_PARSER_SNIFFING_BUFFER_SIZE 512

enum eHtml5ParserLifecycle {
  


  NOT_STARTED = 0,

  


  PARSING = 1,

  



  STREAM_ENDING = 2,

  


  TERMINATED = 3
};

enum eBomState {
  


  BOM_SNIFFING_NOT_STARTED = 0,

  



  SEEN_UTF_16_LE_FIRST_BYTE = 1,

  



  SEEN_UTF_16_BE_FIRST_BYTE = 2,

  



  SEEN_UTF_8_FIRST_BYTE = 3,

  



  SEEN_UTF_8_SECOND_BYTE = 4,

  


  BOM_SNIFFING_OVER = 5
};

class nsHtml5Parser : public nsIParser,
                      public nsIStreamListener,
                      public nsICharsetDetectionObserver,
                      public nsIContentSink,
                      public nsContentSink {
  public:
    NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW
    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsHtml5Parser, nsContentSink)

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

    








    NS_IMETHOD ParseFragment(const nsAString& aSourceBuffer,
                             nsISupports* aTargetNode,
                             nsIAtom* aContextLocalName,
                             PRInt32 aContextNamespace,
                             PRBool aQuirks);

    


    NS_IMETHOD BuildModel(void);

    


    NS_IMETHODIMP CancelParsingEvents();

    


    virtual void Reset();
    
    


    virtual PRBool CanInterrupt();
    
    
     
      
      
      
    
    NS_DECL_NSIREQUESTOBSERVER
    
    NS_DECL_NSISTREAMLISTENER

    


    void HandleParserContinueEvent(class nsHtml5ParserContinueEvent *);

    
    


    void internalEncodingDeclaration(nsString* aEncoding);

    
    


    void documentMode(nsHtml5DocumentMode m);

    
    


    NS_IMETHOD Notify(const char* aCharset, nsDetectionConfident aConf);

    

    


    NS_IMETHOD WillParse();

    


    NS_IMETHOD WillBuildModel(nsDTDMode aDTDMode);

    


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
    virtual void PostEvaluateScript(nsIScriptElement *aElement);
    using nsContentSink::Notify;
    

    












    nsresult FinalizeSniffing(const PRUint8* aFromSegment,
                              PRUint32 aCount,
                              PRUint32* aWriteCount,
                              PRUint32 aCountToSniffingLimit);

    










    nsresult SetupDecodingAndWriteSniffingBufferAndCurrentSegment(const PRUint8* aFromSegment,
                                                                  PRUint32 aCount,
                                                                  PRUint32* aWriteCount);

    










    nsresult WriteSniffingBufferAndCurrentSegment(const PRUint8* aFromSegment,
                                                  PRUint32 aCount,
                                                  PRUint32* aWriteCount);

    









    nsresult SetupDecodingFromBom(const char* aCharsetName,
                                  const char* aDecoderCharsetName);

    


    PRBool HasDecoder() {
      return !!mUnicodeDecoder;
    }

    


    nsresult SniffStreamBytes(const PRUint8* aFromSegment,
                              PRUint32 aCount,
                              PRUint32* aWriteCount);

    


    nsresult WriteStreamBytes(const PRUint8* aFromSegment,
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

    


    nsresult PerformCharsetSwitch();

    


    void ParseUntilSuspend();

  private:
    
    


    PRBool                        mNeedsCharsetSwitch;

    


    PRBool                        mLastWasCR;

    


    PRBool                        mFragmentMode;

    


    PRBool                        mBlocked;

    


    PRBool                        mSuspending;
    
    


    PRBool                        mSuppressEOF;

    


    eHtml5ParserLifecycle         mLifeCycle;

    
    


    nsCOMPtr<nsIContent>          mScriptElement;

    


    PRBool                        mUninterruptibleDocWrite;

    
    void*                         mRootContextKey;
    nsCOMPtr<nsIRequest>          mRequest;
    nsCOMPtr<nsIRequestObserver>  mObserver;
    nsIRunnable*                  mContinueEvent;  

    
    


    PRInt32                       mCharsetSource;

    


    nsCString                     mCharset;

    


    nsCString                     mPendingCharset;

    


    nsCOMPtr<nsIUnicodeDecoder>   mUnicodeDecoder;

    


    nsAutoArrayPtr<PRUint8>       mSniffingBuffer;

    


    PRUint32                      mSniffingLength;

    


    eBomState                     mBomState;

    


    nsAutoPtr<nsHtml5MetaScanner> mMetaScanner;

    
    


    nsHtml5UTF16Buffer*           mFirstBuffer; 

    


    nsHtml5UTF16Buffer*           mLastBuffer; 
                      

    


    nsAutoPtr<nsHtml5TreeBuilder> mTreeBuilder;

    


    nsAutoPtr<nsHtml5Tokenizer>   mTokenizer;

#ifdef DEBUG
    


    eStreamState                  mStreamListenerState;
#endif

#ifdef GATHER_DOCWRITE_STATISTICS
    nsHtml5StateSnapshot*         mSnapshot;
    static PRUint32               sUnsafeDocWrites;
    static PRUint32               sTokenSafeDocWrites;
    static PRUint32               sTreeSafeDocWrites;
#endif
};
#endif
