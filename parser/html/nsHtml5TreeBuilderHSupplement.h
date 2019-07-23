




































#define NS_HTML5_TREE_BUILDER_HANDLE_ARRAY_LENGTH 512

  private:

    nsTArray<nsHtml5TreeOperation>         mOpQueue;
    nsAHtml5TreeOpSink*                    mOpSink;
    nsAutoArrayPtr<nsIContent*>            mHandles;
    PRInt32                                mHandlesUsed;
    nsTArray<nsAutoArrayPtr<nsIContent*> > mOldHandles;
    nsRefPtr<nsHtml5SpeculativeLoader>     mSpeculativeLoader;
#ifdef DEBUG
    PRBool                                 mActive;
#endif

    
    


    void documentMode(nsHtml5DocumentMode m);

    nsIContent** AllocateContentHandle();
    
  public:

    nsHtml5TreeBuilder(nsAHtml5TreeOpSink* aOpSink);

    ~nsHtml5TreeBuilder();
    
    PRBool HasScript();
    
    void SetOpSink(nsAHtml5TreeOpSink* aOpSink) {
      mOpSink = aOpSink;
    }
    
    void SetSpeculativeLoaderWithDocument(nsIDocument* aDocument);

    void DropSpeculativeLoader();

    void Flush();
    
    void MaybeFlush();
    
    void SetDocumentCharset(nsACString& aCharset);

    void StreamEnded();

    void NeedsCharsetSwitchTo(const nsACString& aEncoding);

    void AddSnapshotToScript(nsAHtml5TreeBuilderState* aSnapshot);

    inline void Dispatch(nsIRunnable* aEvent) {
      if (NS_FAILED(NS_DispatchToMainThread(aEvent))) {
        NS_WARNING("Failed to dispatch speculative load runnable.");
      }
    }
