




#ifndef nsUniversalDetector_h__
#define nsUniversalDetector_h__

class nsCharSetProber;

#define NUM_OF_CHARSET_PROBERS  3

typedef enum {
  ePureAscii = 0,
  eEscAscii  = 1,
  eHighbyte  = 2
} nsInputState;

class nsUniversalDetector {
public:
   nsUniversalDetector();
   virtual ~nsUniversalDetector();
   virtual nsresult HandleData(const char* aBuf, uint32_t aLen);
   virtual void DataEnd(void);

protected:
   virtual void Report(const char* aCharset) = 0;
   virtual void Reset();
   nsInputState  mInputState;
   bool    mDone;
   bool    mInTag;
   bool    mStart;
   bool    mGotData;
   char    mLastChar;
   const char *  mDetectedCharset;
   int32_t mBestGuess;
   uint32_t mLanguageFilter;

   nsCharSetProber  *mCharSetProbers[NUM_OF_CHARSET_PROBERS];
   nsCharSetProber  *mEscCharSetProber;
};

#endif

