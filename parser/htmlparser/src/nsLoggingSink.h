




































#ifndef NS_LOGGING_SINK_H__
#define NS_LOGGING_SINK_H__

#include "nsILoggingSink.h"
#include "nsIParser.h"

class nsLoggingSink : public nsILoggingSink {
public:
  nsLoggingSink();
  virtual ~nsLoggingSink();

  void SetProxySink(nsIHTMLContentSink *aSink) {
    mSink=aSink;    
  }

  void ReleaseProxySink() {
    NS_IF_RELEASE(mSink);
    mSink=0;
  }


  
  NS_DECL_ISUPPORTS

  
  NS_IMETHOD WillParse();
  NS_IMETHOD WillBuildModel(nsDTDMode aDTDMode);
  NS_IMETHOD DidBuildModel(bool aTerminated);
  NS_IMETHOD WillInterrupt();
  NS_IMETHOD WillResume();
  NS_IMETHOD SetParser(nsParserBase* aParser);
  NS_IMETHOD OpenContainer(const nsIParserNode& aNode);
  NS_IMETHOD CloseContainer(const nsHTMLTag aTag);
  NS_IMETHOD AddLeaf(const nsIParserNode& aNode);
  NS_IMETHOD AddComment(const nsIParserNode& aNode);
  NS_IMETHOD AddProcessingInstruction(const nsIParserNode& aNode);
  virtual void FlushPendingNotifications(mozFlushType aType) { }
  NS_IMETHOD SetDocumentCharset(nsACString& aCharset) { return NS_OK; }
  virtual nsISupports *GetTarget() { return nsnull; }

  
  NS_IMETHOD OpenHead();
  NS_IMETHOD IsEnabled(PRInt32 aTag, bool* aReturn) 
  
  { NS_ENSURE_ARG_POINTER(aReturn); *aReturn = true; return NS_OK; }
  NS_IMETHOD NotifyTagObservers(nsIParserNode* aNode) { return NS_OK; }

  NS_IMETHOD BeginContext(PRInt32 aPosition);
  NS_IMETHOD EndContext(PRInt32 aPosition);
  NS_IMETHOD DidProcessTokens(void) { return NS_OK; }
  NS_IMETHOD WillProcessAToken(void) { return NS_OK; }
  NS_IMETHOD DidProcessAToken(void) { return NS_OK; }

  
  NS_IMETHOD SetOutputStream(PRFileDesc *aStream,bool autoDelete=false);

  nsresult OpenNode(const char* aKind, const nsIParserNode& aNode);
  nsresult CloseNode(const char* aKind);
  nsresult LeafNode(const nsIParserNode& aNode);
  nsresult WriteAttributes(const nsIParserNode& aNode);
  nsresult QuoteText(const nsAString& aValue, nsString& aResult);
  nsresult GetNewCString(const nsAString& aValue, char** aResult);
  bool WillWriteAttributes(const nsIParserNode& aNode);

protected:
  PRFileDesc          *mOutput;
  int                  mLevel;
  nsIHTMLContentSink  *mSink;
  bool                 mAutoDeleteOutput;
};

#endif

