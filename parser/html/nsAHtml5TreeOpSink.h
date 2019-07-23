




































#ifndef nsAHtml5TreeOpSink_h___
#define nsAHtml5TreeOpSink_h___






class nsAHtml5TreeOpSink {
  public:
  
    



    virtual void MaybeFlush(nsTArray<nsHtml5TreeOperation>& aOpQueue) = 0;

    



    virtual void ForcedFlush(nsTArray<nsHtml5TreeOperation>& aOpQueue) = 0;
    
};

#endif 
