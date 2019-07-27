




#ifndef nsXMLContentSink_h__
#define nsXMLContentSink_h__

#include "mozilla/Attributes.h"
#include "nsContentSink.h"
#include "nsIXMLContentSink.h"
#include "nsIExpatSink.h"
#include "nsIDocumentTransformer.h"
#include "nsTArray.h"
#include "nsCOMPtr.h"
#include "nsCRT.h"
#include "nsCycleCollectionParticipant.h"
#include "nsIDTD.h"
#include "mozilla/dom/FromParser.h"

class nsIDocument;
class nsIURI;
class nsIContent;
class nsIParser;

namespace mozilla {
namespace dom {
class NodeInfo;
}
}

typedef enum {
  eXMLContentSinkState_InProlog,
  eXMLContentSinkState_InDocumentElement,
  eXMLContentSinkState_InEpilog
} XMLContentSinkState;

struct StackNode {
  nsCOMPtr<nsIContent> mContent;
  uint32_t mNumFlushed;
};

class nsXMLContentSink : public nsContentSink,
                         public nsIXMLContentSink,
                         public nsITransformObserver,
                         public nsIExpatSink
{
public:
  nsXMLContentSink();

  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

  nsresult Init(nsIDocument* aDoc,
                nsIURI* aURL,
                nsISupports* aContainer,
                nsIChannel* aChannel);

  
  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED_NO_UNLINK(nsXMLContentSink,
                                                     nsContentSink)

  NS_DECL_NSIEXPATSINK

  
  NS_IMETHOD WillParse(void) override;
  NS_IMETHOD WillBuildModel(nsDTDMode aDTDMode) override;
  NS_IMETHOD DidBuildModel(bool aTerminated) override;
  NS_IMETHOD WillInterrupt(void) override;
  NS_IMETHOD WillResume(void) override;
  NS_IMETHOD SetParser(nsParserBase* aParser) override;
  virtual void FlushPendingNotifications(mozFlushType aType) override;
  NS_IMETHOD SetDocumentCharset(nsACString& aCharset) override;
  virtual nsISupports *GetTarget() override;
  virtual bool IsScriptExecuting() override;
  virtual void ContinueInterruptedParsingAsync() override;

  
  NS_IMETHOD OnDocumentCreated(nsIDocument *aResultDocument) override;
  NS_IMETHOD OnTransformDone(nsresult aResult, nsIDocument *aResultDocument) override;

  
  NS_IMETHOD StyleSheetLoaded(mozilla::CSSStyleSheet* aSheet,
                              bool aWasAlternate,
                              nsresult aStatus) override;
  static bool ParsePIData(const nsString &aData, nsString &aHref,
                          nsString &aTitle, nsString &aMedia,
                          bool &aIsAlternate);

protected:
  virtual ~nsXMLContentSink();

  nsIParser* GetParser();

  void ContinueInterruptedParsingIfEnabled();

  
  
  
  virtual void MaybeStartLayout(bool aIgnorePendingSheets);

  virtual nsresult AddAttributes(const char16_t** aNode, nsIContent* aContent);
  nsresult AddText(const char16_t* aString, int32_t aLength);

  virtual bool OnOpenContainer(const char16_t **aAtts, 
                                 uint32_t aAttsCount, 
                                 int32_t aNameSpaceID, 
                                 nsIAtom* aTagName,
                                 uint32_t aLineNumber) { return true; }
  
  
  
  virtual bool SetDocElement(int32_t aNameSpaceID, 
                               nsIAtom *aTagName,
                               nsIContent *aContent);
  virtual bool NotifyForDocElement() { return true; }
  virtual nsresult CreateElement(const char16_t** aAtts, uint32_t aAttsCount,
                                 mozilla::dom::NodeInfo* aNodeInfo, uint32_t aLineNumber,
                                 nsIContent** aResult, bool* aAppendContent,
                                 mozilla::dom::FromParser aFromParser);

  
  
  virtual nsresult CloseElement(nsIContent* aContent);

  virtual nsresult FlushText(bool aReleaseTextNode = true);

  nsresult AddContentAsLeaf(nsIContent *aContent);

  nsIContent* GetCurrentContent();
  StackNode* GetCurrentStackNode();
  nsresult PushContent(nsIContent *aContent);
  void PopContent();
  bool HaveNotifiedForCurrentContent() const;

  nsresult FlushTags() override;

  void UpdateChildCounts() override;

  void DidAddContent()
  {
    if (IsTimeToNotify()) {
      FlushTags();	
    }
  }
  
  
  virtual nsresult ProcessStyleLink(nsIContent* aElement,
                                    const nsSubstring& aHref,
                                    bool aAlternate,
                                    const nsSubstring& aTitle,
                                    const nsSubstring& aType,
                                    const nsSubstring& aMedia) override;

  nsresult LoadXSLStyleSheet(nsIURI* aUrl);

  bool CanStillPrettyPrint();

  nsresult MaybePrettyPrint();
  
  bool IsMonolithicContainer(mozilla::dom::NodeInfo* aNodeInfo);

  nsresult HandleStartElement(const char16_t *aName, const char16_t **aAtts, 
                              uint32_t aAttsCount, uint32_t aLineNumber,
                              bool aInterruptable);
  nsresult HandleEndElement(const char16_t *aName, bool aInterruptable);
  nsresult HandleCharacterData(const char16_t *aData, uint32_t aLength,
                               bool aInterruptable);

  nsCOMPtr<nsIContent> mDocElement;
  nsCOMPtr<nsIContent> mCurrentHead;  
  char16_t*       mText;

  XMLContentSinkState mState;

  int32_t mTextLength;
  int32_t mTextSize;
  
  int32_t mNotifyLevel;
  nsCOMPtr<nsIContent> mLastTextNode;
  int32_t mLastTextNodeSize;

  uint8_t mConstrainSize : 1;
  uint8_t mPrettyPrintXML : 1;
  uint8_t mPrettyPrintHasSpecialRoot : 1;
  uint8_t mPrettyPrintHasFactoredElements : 1;
  uint8_t mPrettyPrinting : 1;  
                                
  
  uint8_t mPreventScriptExecution : 1;
  
  nsTArray<StackNode>              mContentStack;

  nsCOMPtr<nsIDocumentTransformer> mXSLTProcessor;
};

#endif 
