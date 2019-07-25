




































#ifndef nsHtml5Speculation_h__
#define nsHtml5Speculation_h__

#include "nsHtml5OwningUTF16Buffer.h"
#include "nsAHtml5TreeBuilderState.h"
#include "nsHtml5TreeOperation.h"
#include "nsAHtml5TreeOpSink.h"
#include "nsTArray.h"
#include "nsAutoPtr.h"

class nsHtml5Speculation : public nsAHtml5TreeOpSink
{
  public:
    nsHtml5Speculation(nsHtml5OwningUTF16Buffer* aBuffer,
                       PRInt32 aStart, 
                       PRInt32 aStartLineNumber, 
                       nsAHtml5TreeBuilderState* aSnapshot);
    
    ~nsHtml5Speculation();

    nsHtml5OwningUTF16Buffer* GetBuffer() {
      return mBuffer;
    }
    
    PRInt32 GetStart() {
      return mStart;
    }

    PRInt32 GetStartLineNumber() {
      return mStartLineNumber;
    }
    
    nsAHtml5TreeBuilderState* GetSnapshot() {
      return mSnapshot;
    }

    



    virtual void MoveOpsFrom(nsTArray<nsHtml5TreeOperation>& aOpQueue);
    
    void FlushToSink(nsAHtml5TreeOpSink* aSink);

  private:
    


    nsRefPtr<nsHtml5OwningUTF16Buffer>  mBuffer;
    
    


    PRInt32                             mStart;

    


    PRInt32                             mStartLineNumber;
    
    nsAutoPtr<nsAHtml5TreeBuilderState> mSnapshot;

    nsTArray<nsHtml5TreeOperation>      mOpQueue;
};

#endif 
