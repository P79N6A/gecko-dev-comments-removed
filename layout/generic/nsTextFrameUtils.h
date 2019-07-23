




































#ifndef NSTEXTFRAMEUTILS_H_
#define NSTEXTFRAMEUTILS_H_

#include "gfxFont.h"
#include "gfxSkipChars.h"
#include "gfxTextRunCache.h"
#include "nsTextFragment.h"

#define BIG_TEXT_NODE_SIZE 4096

class nsTextFrameUtils {
public:
  
  enum {
    

    
    TEXT_HAS_TAB             = 0x010000,
    
    TEXT_HAS_SHY             = 0x020000,
    TEXT_WAS_TRANSFORMED     = 0x040000,

    

    TEXT_IS_SIMPLE_FLOW      = 0x100000,
    TEXT_INCOMING_WHITESPACE = 0x200000,
    TEXT_TRAILING_WHITESPACE = 0x400000,
    TEXT_COMPRESSED_LEADING_WHITESPACE = 0x800000,
    TEXT_IS_UNCACHED         = 0x1000000,
    TEXT_NO_BREAKS           = 0x2000000
  };

  static PRBool
  IsPunctuationMark(PRUnichar aChar);

  





  static PRBool
  IsSpaceCombiningSequenceTail(const PRUnichar* aChars, PRInt32 aLength) {
    return aLength > 0 && aChars[0] == 0x200D; 
  }

  












  static PRUnichar* TransformText(const PRUnichar* aText, PRUint32 aLength,
                                  PRUnichar* aOutput,
                                  PRBool aCompressWhitespace,
                                  PRPackedBool* aIncomingWhitespace,
                                  gfxSkipCharsBuilder* aSkipChars,
                                  PRUint32* aAnalysisFlags);

  static PRUint8* TransformText(const PRUint8* aText, PRUint32 aLength,
                                PRUint8* aOutput,
                                PRBool aCompressWhitespace,
                                PRPackedBool* aIncomingWhitespace,
                                gfxSkipCharsBuilder* aSkipChars,
                                PRUint32* aAnalysisFlags);
};

class nsSkipCharsRunIterator {
public:
  enum LengthMode {
    LENGTH_UNSKIPPED_ONLY   = PR_FALSE,
    LENGTH_INCLUDES_SKIPPED = PR_TRUE
  };
  nsSkipCharsRunIterator(const gfxSkipCharsIterator& aStart,
      LengthMode aLengthIncludesSkipped, PRUint32 aLength)
    : mIterator(aStart), mRemainingLength(aLength), mRunLength(0),
      mVisitSkipped(PR_FALSE),
      mLengthIncludesSkipped(aLengthIncludesSkipped) {
  }
  void SetVisitSkipped() { mVisitSkipped = PR_TRUE; }
  void SetOriginalOffset(PRInt32 aOffset) {
    mIterator.SetOriginalOffset(aOffset);
  }
  void SetSkippedOffset(PRUint32 aOffset) {
    mIterator.SetSkippedOffset(aOffset);
  }

  
  PRBool NextRun();
  PRBool IsSkipped() const { return mSkipped; }
  
  PRInt32 GetRunLength() const { return mRunLength; }
  const gfxSkipCharsIterator& GetPos() const { return mIterator; }
  PRInt32 GetOriginalOffset() const { return mIterator.GetOriginalOffset(); }
  PRUint32 GetSkippedOffset() const { return mIterator.GetSkippedOffset(); }

private:
  gfxSkipCharsIterator mIterator;
  PRInt32              mRemainingLength;
  PRInt32              mRunLength;
  PRPackedBool         mSkipped;
  PRPackedBool         mVisitSkipped;
  PRPackedBool         mLengthIncludesSkipped;
};

#endif
