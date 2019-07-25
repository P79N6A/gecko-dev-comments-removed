




































#ifndef nsUniversalDetector_h__
#define nsUniversalDetector_h__

class nsCharSetProber;

#define NUM_OF_CHARSET_PROBERS  3

typedef enum {
  ePureAscii = 0,
  eEscAscii  = 1,
  eHighbyte  = 2
} nsInputState;

#define NS_FILTER_CHINESE_SIMPLIFIED  0x01
#define NS_FILTER_CHINESE_TRADITIONAL 0x02
#define NS_FILTER_JAPANESE            0x04
#define NS_FILTER_KOREAN              0x08
#define NS_FILTER_NON_CJK             0x10
#define NS_FILTER_ALL                 0x1F
#define NS_FILTER_CHINESE (NS_FILTER_CHINESE_SIMPLIFIED | \
                           NS_FILTER_CHINESE_TRADITIONAL)
#define NS_FILTER_CJK (NS_FILTER_CHINESE_SIMPLIFIED | \
                       NS_FILTER_CHINESE_TRADITIONAL | \
                       NS_FILTER_JAPANESE | \
                       NS_FILTER_KOREAN)

class nsUniversalDetector {
public:
   nsUniversalDetector(PRUint32 aLanguageFilter);
   virtual ~nsUniversalDetector();
   virtual nsresult HandleData(const char* aBuf, PRUint32 aLen);
   virtual void DataEnd(void);

protected:
   virtual void Report(const char* aCharset) = 0;
   virtual void Reset();
   nsInputState  mInputState;
   PRBool  mDone;
   PRBool  mInTag;
   PRBool  mStart;
   PRBool  mGotData;
   char    mLastChar;
   const char *  mDetectedCharset;
   PRInt32 mBestGuess;
   PRUint32 mLanguageFilter;

   nsCharSetProber  *mCharSetProbers[NUM_OF_CHARSET_PROBERS];
   nsCharSetProber  *mEscCharSetProber;
};

#endif

