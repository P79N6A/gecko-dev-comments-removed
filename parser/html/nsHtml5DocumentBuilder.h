





#ifndef nsHtml5DocumentBuilder_h
#define nsHtml5DocumentBuilder_h

#include "nsContentSink.h"
#include "nsHtml5DocumentMode.h"
#include "nsIDocument.h"
#include "nsIContent.h"

typedef nsIContent* nsIContentPtr;

enum eHtml5FlushState {
  eNotFlushing = 0,  
  eInFlush = 1,      
  eInDocUpdate = 2,  
  eNotifying = 3     
};

class nsHtml5DocumentBuilder : public nsContentSink
{
public:
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsHtml5DocumentBuilder,
                                           nsContentSink)

  NS_DECL_ISUPPORTS_INHERITED

  inline void HoldElement(already_AddRefed<nsIContent> aContent)
  {
    *(mOwnedElements.AppendElement()) = aContent;
  }

  nsresult Init(nsIDocument* aDoc, nsIURI* aURI,
                nsISupports* aContainer, nsIChannel* aChannel);

  
  nsIDocument* GetDocument()
  {
    return mDocument;
  }

  nsNodeInfoManager* GetNodeInfoManager()
  {
    return mNodeInfoManager;
  }

  





  virtual nsresult MarkAsBroken(nsresult aReason);

  



  inline nsresult IsBroken()
  {
    return mBroken;
  }

  inline void BeginDocUpdate()
  {
    NS_PRECONDITION(mFlushState == eInFlush, "Tried to double-open update.");
    NS_PRECONDITION(mParser, "Started update without parser.");
    mFlushState = eInDocUpdate;
    mDocument->BeginUpdate(UPDATE_CONTENT_MODEL);
  }

  inline void EndDocUpdate()
  {
    NS_PRECONDITION(mFlushState != eNotifying, "mFlushState out of sync");
    if (mFlushState == eInDocUpdate) {
      mFlushState = eInFlush;
      mDocument->EndUpdate(UPDATE_CONTENT_MODEL);
    }
  }

  bool IsInDocUpdate()
  {
    return mFlushState == eInDocUpdate;
  }

  void SetDocumentCharsetAndSource(nsACString& aCharset, int32_t aCharsetSource);

  


  void UpdateStyleSheet(nsIContent* aElement);

  void SetDocumentMode(nsHtml5DocumentMode m);

  void SetNodeInfoManager(nsNodeInfoManager* aManager)
  {
    mNodeInfoManager = aManager;
  }

  
  virtual void UpdateChildCounts() override;
  virtual nsresult FlushTags() override;

protected:

  explicit nsHtml5DocumentBuilder(bool aRunsToCompletion);
  virtual ~nsHtml5DocumentBuilder();

protected:
  nsAutoTArray<nsCOMPtr<nsIContent>, 32> mOwnedElements;
  










  nsresult                             mBroken;
  eHtml5FlushState                     mFlushState;
};

#endif 
