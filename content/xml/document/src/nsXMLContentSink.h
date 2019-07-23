




































#ifndef nsXMLContentSink_h__
#define nsXMLContentSink_h__

#include "nsContentSink.h"
#include "nsIXMLContentSink.h"
#include "nsIExpatSink.h"
#include "nsIDocumentTransformer.h"
#include "nsTArray.h"
#include "nsCOMPtr.h"
#include "nsCRT.h"
#include "nsCycleCollectionParticipant.h"
#include "nsIDTD.h"

class nsIDocument;
class nsIURI;
class nsIContent;
class nsINodeInfo;
class nsIParser;
class nsIViewManager;

typedef enum {
  eXMLContentSinkState_InProlog,
  eXMLContentSinkState_InDocumentElement,
  eXMLContentSinkState_InEpilog
} XMLContentSinkState;

struct StackNode {
  nsCOMPtr<nsIContent> mContent;
  PRUint32 mNumFlushed;
};

class nsXMLContentSink : public nsContentSink,
                         public nsIXMLContentSink,
                         public nsITransformObserver,
                         public nsIExpatSink
{
public:
  nsXMLContentSink();
  virtual ~nsXMLContentSink();

  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

  nsresult Init(nsIDocument* aDoc,
                nsIURI* aURL,
                nsISupports* aContainer,
                nsIChannel* aChannel);

  
  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED_NO_UNLINK(nsXMLContentSink,
                                                     nsContentSink)

  NS_DECL_NSIEXPATSINK

  
  NS_IMETHOD WillParse(void);
  NS_IMETHOD WillBuildModel(nsDTDMode aDTDMode);
  NS_IMETHOD DidBuildModel(PRBool aTerminated);
  NS_IMETHOD WillInterrupt(void);
  NS_IMETHOD WillResume(void);
  NS_IMETHOD SetParser(nsIParser* aParser);  
  virtual void FlushPendingNotifications(mozFlushType aType);
  NS_IMETHOD SetDocumentCharset(nsACString& aCharset);
  virtual nsISupports *GetTarget();
  virtual PRBool IsScriptExecuting();

  
  NS_IMETHOD OnDocumentCreated(nsIDocument *aResultDocument);
  NS_IMETHOD OnTransformDone(nsresult aResult, nsIDocument *aResultDocument);

  
  NS_IMETHOD StyleSheetLoaded(nsICSSStyleSheet* aSheet, PRBool aWasAlternate,
                              nsresult aStatus);
  static PRBool ParsePIData(const nsString &aData, nsString &aHref,
                          nsString &aTitle, nsString &aMedia,
                          PRBool &aIsAlternate);

protected:
  
  
  
  virtual void MaybeStartLayout(PRBool aIgnorePendingSheets);

  virtual nsresult AddAttributes(const PRUnichar** aNode, nsIContent* aContent);
  nsresult AddText(const PRUnichar* aString, PRInt32 aLength);

  virtual PRBool OnOpenContainer(const PRUnichar **aAtts, 
                                 PRUint32 aAttsCount, 
                                 PRInt32 aNameSpaceID, 
                                 nsIAtom* aTagName,
                                 PRUint32 aLineNumber) { return PR_TRUE; }
  
  
  
  virtual PRBool SetDocElement(PRInt32 aNameSpaceID, 
                               nsIAtom *aTagName,
                               nsIContent *aContent);
  virtual nsresult CreateElement(const PRUnichar** aAtts, PRUint32 aAttsCount,
                                 nsINodeInfo* aNodeInfo, PRUint32 aLineNumber,
                                 nsIContent** aResult, PRBool* aAppendContent,
                                 PRBool aFromParser);

  
  
  virtual nsresult CloseElement(nsIContent* aContent);

  virtual nsresult FlushText(PRBool aReleaseTextNode = PR_TRUE);

  nsresult AddContentAsLeaf(nsIContent *aContent);

  nsIContent* GetCurrentContent();
  StackNode & GetCurrentStackNode();
  nsresult PushContent(nsIContent *aContent);
  void PopContent();
  PRBool HaveNotifiedForCurrentContent() const;

  void ProcessBASETag(nsIContent* aContent);

  nsresult FlushTags();

  void UpdateChildCounts();

  void DidAddContent()
  {
    if (IsTimeToNotify()) {
      FlushTags();	
    }
  }
  
  
  virtual nsresult ProcessStyleLink(nsIContent* aElement,
                                    const nsSubstring& aHref,
                                    PRBool aAlternate,
                                    const nsSubstring& aTitle,
                                    const nsSubstring& aType,
                                    const nsSubstring& aMedia);

  nsresult LoadXSLStyleSheet(nsIURI* aUrl);

  PRBool CanStillPrettyPrint();

  nsresult MaybePrettyPrint();
  
  PRBool IsMonolithicContainer(nsINodeInfo* aNodeInfo);

  nsresult HandleStartElement(const PRUnichar *aName, const PRUnichar **aAtts, 
                              PRUint32 aAttsCount, PRInt32 aIndex, 
                              PRUint32 aLineNumber,
                              PRBool aInterruptable);
  nsresult HandleEndElement(const PRUnichar *aName, PRBool aInterruptable);
  nsresult HandleCharacterData(const PRUnichar *aData, PRUint32 aLength,
                               PRBool aInterruptable);

  nsIContent*      mDocElement;
  nsCOMPtr<nsIContent> mCurrentHead;  
  PRUnichar*       mText;

  XMLContentSinkState mState;

  PRInt32 mTextLength;
  PRInt32 mTextSize;
  
  PRInt32 mNotifyLevel;
  nsCOMPtr<nsIContent> mLastTextNode;
  PRInt32 mLastTextNodeSize;

  PRUint8 mConstrainSize : 1;
  PRUint8 mPrettyPrintXML : 1;
  PRUint8 mPrettyPrintHasSpecialRoot : 1;
  PRUint8 mPrettyPrintHasFactoredElements : 1;
  PRUint8 mHasProcessedBase : 1;
  PRUint8 mPrettyPrinting : 1;  
                                
  
  nsTArray<StackNode>              mContentStack;

  nsCOMPtr<nsIDocumentTransformer> mXSLTProcessor;
};

#endif 
