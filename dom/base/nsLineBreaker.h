





#ifndef NSLINEBREAKER_H_
#define NSLINEBREAKER_H_

#include "nsString.h"
#include "nsTArray.h"
#include "nsILineBreaker.h"

class nsIAtom;
class nsHyphenator;




class nsILineBreakSink {
public:
  










  virtual void SetBreaks(uint32_t aStart, uint32_t aLength, uint8_t* aBreakBefore) = 0;
  
  



  virtual void SetCapitalization(uint32_t aStart, uint32_t aLength, bool* aCapitalize) = 0;
};























class nsLineBreaker {
public:
  nsLineBreaker();
  ~nsLineBreaker();
  
  static inline bool IsSpace(char16_t u) { return NS_IsSpace(u); }

  static inline bool IsComplexASCIIChar(char16_t u)
  {
    return !((0x0030 <= u && u <= 0x0039) ||
             (0x0041 <= u && u <= 0x005A) ||
             (0x0061 <= u && u <= 0x007A) ||
             (0x000a == u));
  }

  static inline bool IsComplexChar(char16_t u)
  {
    return IsComplexASCIIChar(u) ||
           NS_NeedsPlatformNativeHandling(u) ||
           (0x1100 <= u && u <= 0x11ff) || 
           (0x2000 <= u && u <= 0x21ff) || 
           (0x2e80 <= u && u <= 0xd7ff) || 
           (0xf900 <= u && u <= 0xfaff) || 
           (0xff00 <= u && u <= 0xffef);   
  }

  
  
  
  
  
  
  
  
  
  
  
  

  


  enum {
    


    BREAK_SUPPRESS_INITIAL = 0x01,
    



    BREAK_SUPPRESS_INSIDE = 0x02,
    








    BREAK_SKIP_SETTING_NO_BREAKS = 0x04,
    



    BREAK_NEED_CAPITALIZATION = 0x08,
    



    BREAK_USE_AUTO_HYPHENATION = 0x10
  };

  




  nsresult AppendInvisibleWhitespace(uint32_t aFlags);

  





  nsresult AppendText(nsIAtom* aHyphenationLanguage, const char16_t* aText, uint32_t aLength,
                      uint32_t aFlags, nsILineBreakSink* aSink);
  




  nsresult AppendText(nsIAtom* aHyphenationLanguage, const uint8_t* aText, uint32_t aLength,
                      uint32_t aFlags, nsILineBreakSink* aSink);
  










  nsresult Reset(bool* aTrailingBreak);

  



  void SetWordBreak(uint8_t aMode) { mWordBreak = aMode; }

private:
  
  
  
  struct TextItem {
    TextItem(nsILineBreakSink* aSink, uint32_t aSinkOffset, uint32_t aLength,
             uint32_t aFlags)
      : mSink(aSink), mSinkOffset(aSinkOffset), mLength(aLength), mFlags(aFlags) {}

    nsILineBreakSink* mSink;
    uint32_t          mSinkOffset;
    uint32_t          mLength;
    uint32_t          mFlags;
  };

  
  

  
  
  
  nsresult FlushCurrentWord();

  void UpdateCurrentWordLanguage(nsIAtom *aHyphenationLanguage);

  void FindHyphenationPoints(nsHyphenator *aHyphenator,
                             const char16_t *aTextStart,
                             const char16_t *aTextLimit,
                             uint8_t *aBreakState);

  nsAutoTArray<char16_t,100> mCurrentWord;
  
  nsAutoTArray<TextItem,2>    mTextItems;
  nsIAtom*                    mCurrentWordLanguage;
  bool                        mCurrentWordContainsMixedLang;
  bool                        mCurrentWordContainsComplexChar;

  
  bool                        mAfterBreakableSpace;
  
  
  bool                        mBreakHere;
  
  uint8_t                     mWordBreak;
};

#endif 
