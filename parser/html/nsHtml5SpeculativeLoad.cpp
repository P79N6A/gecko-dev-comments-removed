




































#include "nsHtml5SpeculativeLoad.h"
#include "nsHtml5TreeOpExecutor.h"

nsHtml5SpeculativeLoad::nsHtml5SpeculativeLoad()
#ifdef DEBUG
 : mOpCode(eSpeculativeLoadUninitialized)
#endif
{
  MOZ_COUNT_CTOR(nsHtml5SpeculativeLoad);
}

nsHtml5SpeculativeLoad::~nsHtml5SpeculativeLoad()
{
  MOZ_COUNT_DTOR(nsHtml5SpeculativeLoad);
  NS_ASSERTION(mOpCode != eSpeculativeLoadUninitialized,
               "Uninitialized speculative load.");
}

void
nsHtml5SpeculativeLoad::Perform(nsHtml5TreeOpExecutor* aExecutor)
{
  switch (mOpCode) {
    case eSpeculativeLoadBase:
        aExecutor->SetSpeculationBase(mUrl);
      break;
    case eSpeculativeLoadImage:
        aExecutor->PreloadImage(mUrl);
      break;
    case eSpeculativeLoadScript:
        aExecutor->PreloadScript(mUrl, mCharset, mType);
      break;
    case eSpeculativeLoadStyle:
        aExecutor->PreloadStyle(mUrl, mCharset);
      break;
    case eSpeculativeLoadManifest:  
        aExecutor->ProcessOfflineManifest(mUrl);
      break;
    default:
      NS_NOTREACHED("Bogus speculative load.");
      break;
  }
}
