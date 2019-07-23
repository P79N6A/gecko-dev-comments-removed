




































#define NS_HTML5_TREE_BUILDER_HANDLE_ARRAY_LENGTH 512

  private:

    nsTArray<nsHtml5TreeOperation>         mOpQueue;
    nsAHtml5TreeOpSink*                    mOpSink;
    nsAutoArrayPtr<nsIContent*>            mHandles;
    PRInt32                                mHandlesUsed;
    nsTArray<nsAutoArrayPtr<nsIContent*> > mOldHandles;
    nsRefPtr<nsHtml5SpeculativeLoader>     mSpeculativeLoader;
    PRBool                                 mCurrentHtmlScriptIsAsyncOrDefer;
#ifdef DEBUG
    PRBool                                 mActive;
#endif

    
    


    void documentMode(nsHtml5DocumentMode m);

    nsIContent** AllocateContentHandle();
    
  public:

    nsHtml5TreeBuilder(nsAHtml5TreeOpSink* aOpSink, 
                       nsHtml5SpeculativeLoader* aSpeculativeLoader);

    ~nsHtml5TreeBuilder();
    
    PRBool IsDiscretionaryFlushSafe();

    PRBool HasScript();
    
    void SetOpSink(nsAHtml5TreeOpSink* aOpSink) {
      mOpSink = aOpSink;
    }

    void ClearOps() {
      mOpQueue.Clear();
    }
    
    PRBool HasSpeculativeLoader() {
      return !!mSpeculativeLoader;
    }
    
    void DropSpeculativeLoader();

    PRBool Flush();
    
    void SetDocumentCharset(nsACString& aCharset, PRInt32 aCharsetSource);

    void StreamEnded();

    void NeedsCharsetSwitchTo(const nsACString& aEncoding);

    void AddSnapshotToScript(nsAHtml5TreeBuilderState* aSnapshot, PRInt32 aLine);

    inline void Dispatch(nsIRunnable* aEvent) {
      if (NS_FAILED(NS_DispatchToMainThread(aEvent))) {
        NS_WARNING("Failed to dispatch speculative load runnable.");
      }
    }
