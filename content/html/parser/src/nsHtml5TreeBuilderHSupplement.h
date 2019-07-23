




































  private:
    nsHtml5Parser* mParser; 
    PRBool         mHasProcessedBase;
    PRBool         mFlushing;
    nsTArray<nsHtml5TreeOperation> mOpQueue;
    void           MaybeFlushAndMaybeSuspend();
  public:
    nsHtml5TreeBuilder(nsHtml5Parser* aParser);
    ~nsHtml5TreeBuilder();
    void Flush();
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
