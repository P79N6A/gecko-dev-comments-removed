




































#ifndef nsHtml5TreeOpStage_h___
#define nsHtml5TreeOpStage_h___

#include "mozilla/Mutex.h"
#include "nsHtml5TreeOperation.h"
#include "nsTArray.h"
#include "nsAHtml5TreeOpSink.h"
#include "nsHtml5SpeculativeLoad.h"

class nsHtml5TreeOpStage : public nsAHtml5TreeOpSink {
  public:
  
    nsHtml5TreeOpStage();
    
    virtual ~nsHtml5TreeOpStage();
  
    



    virtual void MoveOpsFrom(nsTArray<nsHtml5TreeOperation>& aOpQueue);
    
    


    void MoveOpsAndSpeculativeLoadsTo(nsTArray<nsHtml5TreeOperation>& aOpQueue,
        nsTArray<nsHtml5SpeculativeLoad>& aSpeculativeLoadQueue);

    


    void MoveSpeculativeLoadsFrom(nsTArray<nsHtml5SpeculativeLoad>& aSpeculativeLoadQueue);

    


    void MoveSpeculativeLoadsTo(nsTArray<nsHtml5SpeculativeLoad>& aSpeculativeLoadQueue);

#ifdef DEBUG
    void AssertEmpty();
#endif

  private:
    nsTArray<nsHtml5TreeOperation> mOpQueue;
    nsTArray<nsHtml5SpeculativeLoad> mSpeculativeLoadQueue;
    mozilla::Mutex mMutex;
    
};

#endif 
