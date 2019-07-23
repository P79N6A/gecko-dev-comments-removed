





































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
#include "nsIInputStream.h"
#include "nsDetectionConfident.h"
#include "nsHtml5UTF16Buffer.h"
#include "nsHtml5TreeOpExecutor.h"
#include "nsHtml5StreamParser.h"
#include "nsHtml5AtomTable.h"
#include "nsWeakReference.h"

class nsHtml5Parser : public nsIParser,
                      public nsSupportsWeakReference
{
  public:
    NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS

    NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsHtml5Parser, nsIParser)

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
      NS_NOTREACHED("No one should call this.");
    }

    


    NS_IMETHOD_(void) SetParserFilter(nsIParserFilter* aFilter);

    




    NS_IMETHOD GetChannel(nsIChannel** aChannel);

    


    NS_IMETHOD GetDTD(nsIDTD** aDTD);

    


    NS_IMETHOD GetStreamListener(nsIStreamListener** aListener);

    


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

    


    virtual PRBool IsInsertionPointDefined();

    


    virtual void BeginEvaluatingParserInsertedScript();

    


    virtual void EndEvaluatingParserInsertedScript();

    



    virtual void MarkAsNotScriptCreated();

    


    virtual PRBool IsScriptCreated();

    

    


    void HandleParserContinueEvent(class nsHtml5ParserContinueEvent *);

    
    

  public:

    


    virtual nsresult Initialize(nsIDocument* aDoc,
                        nsIURI* aURI,
                        nsISupports* aContainer,
                        nsIChannel* aChannel);

    inline nsHtml5Tokenizer* GetTokenizer() {
      return mTokenizer;
    }

    void InitializeDocWriteParserState(nsAHtml5TreeBuilderState* aState);

    


    void MaybePostContinueEvent();
    
    void DropStreamParser() {
      mStreamParser = nsnull;
    }
    
    void StartTokenizer(PRBool aScriptingEnabled);

#ifdef DEBUG
    PRBool HasStreamParser() {
      return !!mStreamParser;
    }
#endif

  private:

    


    void ParseUntilScript();

    

    


    PRBool                        mLastWasCR;

    


    PRBool                        mFragmentMode;

    


    PRBool                        mBlocked;
    
    


    PRInt32                       mParserInsertedScriptsBeingEvaluated;

    


    PRBool                        mDocumentClosed;

    
    void*                         mRootContextKey;
    nsIRunnable*                  mContinueEvent;  

    
    


    nsRefPtr<nsHtml5UTF16Buffer>  mFirstBuffer;

    


    nsHtml5UTF16Buffer*           mLastBuffer; 
                      

    


    nsRefPtr<nsHtml5TreeOpExecutor>     mExecutor;

    


    const nsAutoPtr<nsHtml5TreeBuilder> mTreeBuilder;

    


    const nsAutoPtr<nsHtml5Tokenizer>   mTokenizer;

    


    nsRefPtr<nsHtml5StreamParser>       mStreamParser;
    
    


    PRBool                              mReturnToStreamParserPermitted;

    


    nsHtml5AtomTable                    mAtomTable;

};
#endif
