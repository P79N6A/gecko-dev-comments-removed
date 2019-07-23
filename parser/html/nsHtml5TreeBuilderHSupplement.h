




































#define NS_HTML5_TREE_BUILDER_HANDLE_ARRAY_LENGTH 512

  private:

    nsTArray<nsHtml5TreeOperation>         mOpQueue;
    nsHtml5TreeOpExecutor*                 mExecutor;
    nsAutoArrayPtr<nsIContent*>            mHandles;
    PRInt32                                mHandlesUsed;
    nsTArray<nsAutoArrayPtr<nsIContent*> > mOldHandles;              
#ifdef DEBUG
    PRBool                                 mActive;
#endif

    
    


    void documentMode(nsHtml5DocumentMode m);

    nsIContent** AllocateContentHandle();
    
  public:

    nsHtml5TreeBuilder(nsHtml5TreeOpExecutor* aExec);

    ~nsHtml5TreeBuilder();

    inline PRUint32 GetOpQueueLength() {
      return mOpQueue.Length();
    }
    
    inline void SwapQueue(nsTArray<nsHtml5TreeOperation>& aOtherQueue) {
      mOpQueue.SwapElements(aOtherQueue);
    }
    
    inline void ReqSuspension() {
      requestSuspension();
    }
    
    PRBool HasScript();
    
