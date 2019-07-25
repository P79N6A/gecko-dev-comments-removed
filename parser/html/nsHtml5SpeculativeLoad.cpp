




































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
      aExecutor->PreloadImage(mUrl, mCharsetOrCrossOrigin);
      break;
    case eSpeculativeLoadScript:
      aExecutor->PreloadScript(mUrl, mCharsetOrCrossOrigin, mTypeOrCharsetSource);
      break;
    case eSpeculativeLoadStyle:
      aExecutor->PreloadStyle(mUrl, mCharsetOrCrossOrigin);
      break;
    case eSpeculativeLoadManifest:  
      aExecutor->ProcessOfflineManifest(mUrl);
      break;
    case eSpeculativeLoadSetDocumentCharset: {
        nsCAutoString narrowName;
        CopyUTF16toUTF8(mCharsetOrCrossOrigin, narrowName);
        NS_ASSERTION(mTypeOrCharsetSource.Length() == 1,
            "Unexpected charset source string");
        PRInt32 intSource = (PRInt32)mTypeOrCharsetSource.First();
        aExecutor->SetDocumentCharsetAndSource(narrowName,
                                               intSource);
      }
      break;
    default:
      NS_NOTREACHED("Bogus speculative load.");
      break;
  }
}
