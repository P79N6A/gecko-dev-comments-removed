




































  private:
#ifdef DEBUG_hsivonen
    static PRUint32 sInsertionBatchMaxLength;
    static PRUint32 sAppendBatchMaxSize;
    static PRUint32 sAppendBatchSlotsExamined;
    static PRUint32 sAppendBatchExaminations;
#endif
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
      PRBool newParent = PR_TRUE;
      const nsIContentPtr* first = mElementsSeenInThisAppendBatch.Elements();
      const nsIContentPtr* last = first + (mElementsSeenInThisAppendBatch.Length() - 1);
      for (const nsIContentPtr* iter = last; iter >= first; --iter) {
#ifdef DEBUG_hsivonen
        sAppendBatchSlotsExamined++;
#endif
        if (*iter == aParent) {
          newParent = PR_FALSE;
          break;
        }
      }
      if (aChild->IsNodeOfType(nsINode::eELEMENT)) {
        mElementsSeenInThisAppendBatch.AppendElement(aChild);
      }
      mElementsSeenInThisAppendBatch.AppendElement(aParent);
      if (newParent) {
        mPendingNotifications.AppendElement(aParent);
      }

#ifdef DEBUG_hsivonen
      sAppendBatchExaminations++;
#endif

    }
    
    inline void FlushPendingAppendNotifications() {
      const nsHtml5PendingNotification* start = mPendingNotifications.Elements();
      const nsHtml5PendingNotification* end = start + mPendingNotifications.Length();
      for (nsHtml5PendingNotification* iter = (nsHtml5PendingNotification*)start; iter < end; ++iter) {
        iter->Fire(this);
      }
      mPendingNotifications.Clear();
#ifdef DEBUG_hsivonen
      if (mElementsSeenInThisAppendBatch.Length() > sAppendBatchMaxSize) {
        sAppendBatchMaxSize = mElementsSeenInThisAppendBatch.Length();
      }
#endif
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
