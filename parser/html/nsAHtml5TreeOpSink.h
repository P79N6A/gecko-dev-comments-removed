




































#ifndef nsAHtml5TreeOpSink_h___
#define nsAHtml5TreeOpSink_h___






class nsAHtml5TreeOpSink {
  public:
  
    



    virtual void MoveOpsFrom(nsTArray<nsHtml5TreeOperation>& aOpQueue) = 0;
    
};

#endif 
