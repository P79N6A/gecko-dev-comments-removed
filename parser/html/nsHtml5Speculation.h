



#ifndef nsHtml5Speculation_h__
#define nsHtml5Speculation_h__

#include "nsHtml5OwningUTF16Buffer.h"
#include "nsAHtml5TreeBuilderState.h"
#include "nsHtml5TreeOperation.h"
#include "nsAHtml5TreeOpSink.h"
#include "nsTArray.h"
#include "nsAutoPtr.h"
#include "mozilla/Attributes.h"

class nsHtml5Speculation MOZ_FINAL : public nsAHtml5TreeOpSink
{
  public:
    nsHtml5Speculation(nsHtml5OwningUTF16Buffer* aBuffer,
                       int32_t aStart, 
                       int32_t aStartLineNumber, 
                       nsAHtml5TreeBuilderState* aSnapshot);
    
    ~nsHtml5Speculation();

    nsHtml5OwningUTF16Buffer* GetBuffer() {
      return mBuffer;
    }
    
    int32_t GetStart() {
      return mStart;
    }

    int32_t GetStartLineNumber() {
      return mStartLineNumber;
    }
    
    nsAHtml5TreeBuilderState* GetSnapshot() {
      return mSnapshot;
    }

    



    virtual void MoveOpsFrom(nsTArray<nsHtml5TreeOperation>& aOpQueue);
    
    void FlushToSink(nsAHtml5TreeOpSink* aSink);

  private:
    


    nsRefPtr<nsHtml5OwningUTF16Buffer>  mBuffer;
    
    


    int32_t                             mStart;

    


    int32_t                             mStartLineNumber;
    
    nsAutoPtr<nsAHtml5TreeBuilderState> mSnapshot;

    nsTArray<nsHtml5TreeOperation>      mOpQueue;
};

#endif 
