




































#define NS_HTML5_TREE_BUILDER_HANDLE_ARRAY_LENGTH 512

  private:

    nsTArray<nsHtml5TreeOperation>         mOpQueue;
    nsTArray<nsHtml5SpeculativeLoad>       mSpeculativeLoadQueue;
    nsAHtml5TreeOpSink*                    mOpSink;
    nsAutoArrayPtr<nsIContent*>            mHandles;
    PRInt32                                mHandlesUsed;
    nsTArray<nsAutoArrayPtr<nsIContent*> > mOldHandles;
    nsHtml5TreeOpStage*                    mSpeculativeLoadStage;
    nsIContent**                           mDeepTreeSurrogateParent;
    PRBool                                 mCurrentHtmlScriptIsAsyncOrDefer;
#ifdef DEBUG
    PRBool                                 mActive;
#endif

    
    


    void documentMode(nsHtml5DocumentMode m);

    nsIContent** AllocateContentHandle();
    
    void accumulateCharactersForced(const PRUnichar* aBuf, PRInt32 aStart, PRInt32 aLength)
    {
      accumulateCharacters(aBuf, aStart, aLength);
    }

  public:

    nsHtml5TreeBuilder(nsAHtml5TreeOpSink* aOpSink,
                       nsHtml5TreeOpStage* aStage);

    ~nsHtml5TreeBuilder();
    
    PRBool HasScript();
    
    void SetOpSink(nsAHtml5TreeOpSink* aOpSink) {
      mOpSink = aOpSink;
    }

    void ClearOps() {
      mOpQueue.Clear();
    }
    
    PRBool Flush(PRBool aDiscretionary = PR_FALSE);
    
    void FlushLoads();

    void SetDocumentCharset(nsACString& aCharset, PRInt32 aCharsetSource);

    void StreamEnded();

    void NeedsCharsetSwitchTo(const nsACString& aEncoding, PRInt32 aSource);

    void AddSnapshotToScript(nsAHtml5TreeBuilderState* aSnapshot, PRInt32 aLine);

    void DropHandles();
