




































  private:
    nsHtml5Parser* mParser; 
    PRBool         mHasProcessedBase;
    PRBool         mFlushing;
    nsTArray<nsHtml5TreeOperation>       mOpQueue;
    nsTArray<nsIContentPtr>              mElementsSeenInThisAppendBatch;
    nsTArray<nsHtml5PendingNotification> mPendingNotifications;
    void           MaybeFlushAndMaybeSuspend();
  public:
    nsHtml5TreeBuilder(nsHtml5Parser* aParser);
    ~nsHtml5TreeBuilder();
    void Flush();
    
    inline void PostPendingAppendNotification(nsIContent* aParent, nsIContent* aChild) {
      nsIContent* parent = aParent; 
      nsIContent* child = aChild->IsNodeOfType(nsINode::eELEMENT) ? aChild : nsnull; 
      const nsIContentPtr* start = mElementsSeenInThisAppendBatch.Elements();
      const nsIContentPtr* end = start + mElementsSeenInThisAppendBatch.Length();
      
      for (const nsIContentPtr* iter = start; iter < end; ++iter) {
        if (*iter == parent) {
          parent = nsnull;
        }
        if (*iter == child) {
          child = nsnull;
        }
        if (!(parent || child)) {
          break;
        } 
      }
      if (child) {
        mElementsSeenInThisAppendBatch.AppendElement(child);
      }
      if (parent) {
        
        
        const nsHtml5PendingNotification* startNotifications = mPendingNotifications.Elements();
        const nsHtml5PendingNotification* endNotifications = startNotifications + mPendingNotifications.Length();
        
        for (nsHtml5PendingNotification* iter = (nsHtml5PendingNotification*)startNotifications; iter < endNotifications; ++iter) {
          if (iter->Contains(parent)) {
            return;
          }
        }
        mPendingNotifications.AppendElement(parent);
      }
    }
    
    inline void FlushPendingAppendNotifications() {
      const nsHtml5PendingNotification* start = mPendingNotifications.Elements();
      const nsHtml5PendingNotification* end = start + mPendingNotifications.Length();
      for (nsHtml5PendingNotification* iter = (nsHtml5PendingNotification*)start; iter < end; ++iter) {
        iter->Fire(this);
      }
      mPendingNotifications.Clear();
      mElementsSeenInThisAppendBatch.Clear();
    }
    
    inline void NotifyAppend(nsIContent* aParent, PRUint32 aChildCount) {
      mParser->NotifyAppend(aParent, aChildCount);
    }
    
    inline nsIDocument* GetDocument() {
      return mParser->GetDocument();
    }
    
    inline void SetScriptElement(nsIContent* aScript) {
      mParser->SetScriptElement(aScript);
    }
    
    inline void UpdateStyleSheet(nsIContent* aSheet) {
      mParser->UpdateStyleSheet(aSheet);
    }
    
    inline nsresult ProcessBase(nsIContent* aBase) {
      if (!mHasProcessedBase) {
        nsresult rv = mParser->ProcessBASETag(aBase);
        NS_ENSURE_SUCCESS(rv, rv);
        mHasProcessedBase = PR_TRUE;
      }
      return NS_OK;
    }
    
    inline void StartLayout() {
      mParser->StartLayout(PR_FALSE);
    }
