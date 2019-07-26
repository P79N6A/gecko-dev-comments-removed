



#ifndef nsAHtml5TreeOpSink_h
#define nsAHtml5TreeOpSink_h






class nsAHtml5TreeOpSink {
  public:
  
    



    virtual void MoveOpsFrom(nsTArray<nsHtml5TreeOperation>& aOpQueue) = 0;
    
};

#endif 
