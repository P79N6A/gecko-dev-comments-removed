




































  private:
#ifdef DEBUG_hsivonen
    static PRUint32    sInsertionBatchMaxLength;
    static PRUint32    sAppendBatchMaxSize;
    static PRUint32    sAppendBatchSlotsExamined;
    static PRUint32    sAppendBatchExaminations;
#endif
    static PRUint32    sTreeOpQueueMaxLength;
    PRBool             mNeedsFlush;
    nsCOMPtr<nsITimer> mFlushTimer;
    nsHtml5Parser*     mParser; 
    PRBool             mHasProcessedBase;
#ifdef DEBUG
    PRBool             mActive;
#endif
    nsTArray<nsHtml5TreeOperation>       mOpQueue;
    nsTArray<nsIContentPtr>              mElementsSeenInThisAppendBatch;
    nsTArray<nsHtml5PendingNotification> mPendingNotifications;
    
    inline void    MaybeSuspend() {
      if (!mNeedsFlush) {
        mNeedsFlush = !!(mOpQueue.Length() >= sTreeOpQueueMaxLength);
      }
      if (mParser->DidProcessATokenImpl() == NS_ERROR_HTMLPARSER_INTERRUPTED || mNeedsFlush) {
        
        
        mParser->Suspend();
        requestSuspension();
      }
    }
    
  public:
    nsHtml5TreeBuilder(nsHtml5Parser* aParser);
    ~nsHtml5TreeBuilder();
    void Flush();
    
    inline void MaybeFlush() {
      if (mNeedsFlush) {
        Flush();
      }
    }
    
    inline void DeferredTimerFlush() {
      if (!mOpQueue.IsEmpty()) {
        mNeedsFlush = PR_TRUE;
      }
    }
    
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
        iter->Fire();
      }
      mPendingNotifications.Clear();
#ifdef DEBUG_hsivonen
      if (mElementsSeenInThisAppendBatch.Length() > sAppendBatchMaxSize) {
        sAppendBatchMaxSize = mElementsSeenInThisAppendBatch.Length();
      }
#endif
      mElementsSeenInThisAppendBatch.Clear();
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
      nsIDocument* doc = GetDocument();
      if (doc) {
        FlushPendingAppendNotifications();
        mParser->StartLayout(PR_FALSE);
      }
    }
