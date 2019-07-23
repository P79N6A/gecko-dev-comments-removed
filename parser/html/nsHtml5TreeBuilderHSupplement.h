




































  private:

    nsTArray<nsHtml5TreeOperation>       mOpQueue;
    nsHtml5TreeOpExecutor*               mExecutor;
#ifdef DEBUG
    PRBool                               mActive;
#endif

  public:

    nsHtml5TreeBuilder(nsHtml5TreeOpExecutor* aExec);

    ~nsHtml5TreeBuilder();

    void DoUnlink();

    void DoTraverse(nsCycleCollectionTraversalCallback &cb);

    
    


    void documentMode(nsHtml5DocumentMode m);
    
    inline PRUint32 GetOpQueueLength() {
      return mOpQueue.Length();
    }
    
    inline void SwapQueue(nsTArray<nsHtml5TreeOperation>& aOtherQueue) {
      mOpQueue.SwapElements(aOtherQueue);
    }
    
    inline void ReqSuspension() {
      requestSuspension();
    }
