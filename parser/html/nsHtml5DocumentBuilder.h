





#ifndef nsHtml5DocumentBuilder_h
#define nsHtml5DocumentBuilder_h

#include "nsHtml5PendingNotification.h"
#include "nsContentSink.h"
#include "nsHtml5DocumentMode.h"
#include "nsIDocument.h"

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

  inline void HoldElement(nsIContent* aContent) {
    mOwnedElements.AppendElement(aContent);
  }

  inline bool HaveNotified(nsIContent* aNode) {
    NS_PRECONDITION(aNode, "HaveNotified called with null argument.");
    const nsHtml5PendingNotification* start = mPendingNotifications.Elements();
    const nsHtml5PendingNotification* end = start + mPendingNotifications.Length();
    for (;;) {
      nsIContent* parent = aNode->GetParent();
      if (!parent) {
        return true;
      }
      for (nsHtml5PendingNotification* iter = (nsHtml5PendingNotification*)start; iter < end; ++iter) {
        if (iter->Contains(parent)) {
          return iter->HaveNotifiedIndex(parent->IndexOf(aNode));
        }
      }
      aNode = parent;
    }
  }

  void PostPendingAppendNotification(nsIContent* aParent, nsIContent* aChild) {
    bool newParent = true;
    const nsIContentPtr* first = mElementsSeenInThisAppendBatch.Elements();
    const nsIContentPtr* last = first + mElementsSeenInThisAppendBatch.Length() - 1;
    for (const nsIContentPtr* iter = last; iter >= first; --iter) {
#ifdef DEBUG_NS_HTML5_TREE_OP_EXECUTOR_FLUSH
      sAppendBatchSlotsExamined++;
#endif
      if (*iter == aParent) {
        newParent = false;
        break;
      }
    }
    if (aChild->IsElement()) {
      mElementsSeenInThisAppendBatch.AppendElement(aChild);
    }
    mElementsSeenInThisAppendBatch.AppendElement(aParent);
    if (newParent) {
      mPendingNotifications.AppendElement(aParent);
    }
#ifdef DEBUG_NS_HTML5_TREE_OP_EXECUTOR_FLUSH
    sAppendBatchExaminations++;
#endif
  }

  void FlushPendingAppendNotifications() {
    NS_PRECONDITION(mFlushState == eInDocUpdate, "Notifications flushed outside update");
    mFlushState = eNotifying;
    const nsHtml5PendingNotification* start = mPendingNotifications.Elements();
    const nsHtml5PendingNotification* end = start + mPendingNotifications.Length();
    for (nsHtml5PendingNotification* iter = (nsHtml5PendingNotification*)start; iter < end; ++iter) {
      iter->Fire();
    }
    mPendingNotifications.Clear();
#ifdef DEBUG_NS_HTML5_TREE_OP_EXECUTOR_FLUSH
    if (mElementsSeenInThisAppendBatch.Length() > sAppendBatchMaxSize) {
      sAppendBatchMaxSize = mElementsSeenInThisAppendBatch.Length();
    }
#endif
    mElementsSeenInThisAppendBatch.Clear();
    NS_ASSERTION(mFlushState == eNotifying, "mFlushState out of sync");
    mFlushState = eInDocUpdate;
  }

  nsresult Init(nsIDocument* aDoc, nsIURI* aURI,
                nsISupports* aContainer, nsIChannel* aChannel);

  
  nsIDocument* GetDocument() {
    return mDocument;
  }
  nsNodeInfoManager* GetNodeInfoManager() {
    return mNodeInfoManager;
  }

  





  virtual nsresult MarkAsBroken(nsresult aReason);

  



  inline nsresult IsBroken() {
    return mBroken;
  }

  inline void BeginDocUpdate() {
    NS_PRECONDITION(mFlushState == eInFlush, "Tried to double-open update.");
    NS_PRECONDITION(mParser, "Started update without parser.");
    mFlushState = eInDocUpdate;
    mDocument->BeginUpdate(UPDATE_CONTENT_MODEL);
  }

  inline void EndDocUpdate() {
    NS_PRECONDITION(mFlushState != eNotifying, "mFlushState out of sync");
    if (mFlushState == eInDocUpdate) {
      FlushPendingAppendNotifications();
      mFlushState = eInFlush;
      mDocument->EndUpdate(UPDATE_CONTENT_MODEL);
    }
  }

  void SetDocumentCharsetAndSource(nsACString& aCharset, int32_t aCharsetSource);

  


  void UpdateStyleSheet(nsIContent* aElement);

  void SetDocumentMode(nsHtml5DocumentMode m);

  void SetNodeInfoManager(nsNodeInfoManager* aManager) {
    mNodeInfoManager = aManager;
  }

  
  virtual void UpdateChildCounts();
  virtual nsresult FlushTags();

protected:
  inline void SetAppendBatchCapacity(uint32_t aCapacity)
  {
    mElementsSeenInThisAppendBatch.SetCapacity(aCapacity);
  }

  nsHtml5DocumentBuilder(bool aRunsToCompletion);
  virtual ~nsHtml5DocumentBuilder();

private:
  nsTArray<nsHtml5PendingNotification> mPendingNotifications;
  nsTArray<nsIContentPtr>              mElementsSeenInThisAppendBatch;
protected:
  nsTArray<nsCOMPtr<nsIContent> >      mOwnedElements;
  










  nsresult                             mBroken;
  eHtml5FlushState                     mFlushState;
};

#endif 
