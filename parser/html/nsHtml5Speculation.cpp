



#include "nsHtml5Speculation.h"

nsHtml5Speculation::nsHtml5Speculation(nsHtml5OwningUTF16Buffer* aBuffer,
                                       int32_t aStart, 
                                       int32_t aStartLineNumber, 
                                       nsAHtml5TreeBuilderState* aSnapshot)
  : mBuffer(aBuffer)
  , mStart(aStart)
  , mStartLineNumber(aStartLineNumber)
  , mSnapshot(aSnapshot)
{
  MOZ_COUNT_CTOR(nsHtml5Speculation);
}

nsHtml5Speculation::~nsHtml5Speculation()
{
  MOZ_COUNT_DTOR(nsHtml5Speculation);
}

void
nsHtml5Speculation::MoveOpsFrom(nsTArray<nsHtml5TreeOperation>& aOpQueue)
{
  if (mOpQueue.IsEmpty()) {
    mOpQueue.SwapElements(aOpQueue);
    return;
  }
  mOpQueue.MoveElementsFrom(aOpQueue);
}

void
nsHtml5Speculation::FlushToSink(nsAHtml5TreeOpSink* aSink)
{
  aSink->MoveOpsFrom(mOpQueue);
}
