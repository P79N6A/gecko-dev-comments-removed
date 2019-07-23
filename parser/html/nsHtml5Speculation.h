




































#ifndef nsHtml5Speculation_h__
#define nsHtml5Speculation_h__

#include "nsHtml5UTF16Buffer.h"
#include "nsAHtml5TreeBuilderState.h"
#include "nsHtml5TreeOperation.h"
#include "nsAHtml5TreeOpSink.h"
#include "nsTArray.h"
#include "nsAutoPtr.h"

class nsHtml5Speculation : public nsAHtml5TreeOpSink
{
  public:
    nsHtml5Speculation(nsHtml5UTF16Buffer* aBuffer, 
                       PRInt32 aStart, 
                       nsAHtml5TreeBuilderState* aSnapshot);
    
    ~nsHtml5Speculation();

    nsHtml5UTF16Buffer* GetBuffer() {
      return mBuffer;
    }
    
    PRInt32 GetStart() {
      return mStart;
    }
    
    nsAHtml5TreeBuilderState* GetSnapshot() {
      return mSnapshot;
    }

    


    virtual void MaybeFlush(nsTArray<nsHtml5TreeOperation>& aOpQueue);

    



    virtual void ForcedFlush(nsTArray<nsHtml5TreeOperation>& aOpQueue);
    
    void FlushToSink(nsAHtml5TreeOpSink* aSink);

  private:
    


    nsRefPtr<nsHtml5UTF16Buffer>        mBuffer;
    
    


    PRInt32                             mStart;
    
    nsAutoPtr<nsAHtml5TreeBuilderState> mSnapshot;

    nsTArray<nsHtml5TreeOperation>      mOpQueue;
};

#endif 