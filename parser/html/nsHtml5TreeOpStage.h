




































#ifndef nsHtml5TreeOpStage_h___
#define nsHtml5TreeOpStage_h___

#include "mozilla/Mutex.h"
#include "nsHtml5TreeOperation.h"
#include "nsTArray.h"
#include "nsAHtml5TreeOpSink.h"

class nsHtml5TreeOpStage : public nsAHtml5TreeOpSink {
  public:
  
    nsHtml5TreeOpStage();
    
    ~nsHtml5TreeOpStage();
  
    



    virtual void MaybeFlush(nsTArray<nsHtml5TreeOperation>& aOpQueue);

    



    virtual void ForcedFlush(nsTArray<nsHtml5TreeOperation>& aOpQueue);
    
    


    void RetrieveOperations(nsTArray<nsHtml5TreeOperation>& aOpQueue);

#ifdef DEBUG
    void AssertEmpty();
#endif

  private:
    nsTArray<nsHtml5TreeOperation> mOpQueue;
    mozilla::Mutex                 mMutex;
    
};

#endif 
