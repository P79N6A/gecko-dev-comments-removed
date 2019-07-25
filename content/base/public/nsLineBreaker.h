




































#ifndef NSLINEBREAKER_H_
#define NSLINEBREAKER_H_

#include "nsString.h"
#include "nsTArray.h"
#include "nsILineBreaker.h"

class nsIAtom;
class nsHyphenator;




class nsILineBreakSink {
public:
  










  virtual void SetBreaks(PRUint32 aStart, PRUint32 aLength, PRUint8* aBreakBefore) = 0;
  
  



  virtual void SetCapitalization(PRUint32 aStart, PRUint32 aLength, PRPackedBool* aCapitalize) = 0;
};























class nsLineBreaker {
public:
  nsLineBreaker();
  ~nsLineBreaker();
  
  static inline PRBool IsSpace(PRUnichar u) { return NS_IsSpace(u); }

  static inline PRBool IsComplexASCIIChar(PRUnichar u)
  {
    return !((0x0030 <= u && u <= 0x0039) ||
             (0x0041 <= u && u <= 0x005A) ||
             (0x0061 <= u && u <= 0x007A) ||
             (0x000a == u));
  }

  static inline PRBool IsComplexChar(PRUnichar u)
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

  




  nsresult AppendInvisibleWhitespace(PRUint32 aFlags);

  





  nsresult AppendText(nsIAtom* aLangGroup, const PRUnichar* aText, PRUint32 aLength,
                      PRUint32 aFlags, nsILineBreakSink* aSink);
  




  nsresult AppendText(nsIAtom* aLangGroup, const PRUint8* aText, PRUint32 aLength,
                      PRUint32 aFlags, nsILineBreakSink* aSink);
  










  nsresult Reset(PRBool* aTrailingBreak);

private:
  
  
  
  struct TextItem {
    TextItem(nsILineBreakSink* aSink, PRUint32 aSinkOffset, PRUint32 aLength,
             PRUint32 aFlags)
      : mSink(aSink), mSinkOffset(aSinkOffset), mLength(aLength), mFlags(aFlags) {}

    nsILineBreakSink* mSink;
    PRUint32          mSinkOffset;
    PRUint32          mLength;
    PRUint32          mFlags;
  };

  
  

  
  
  
  nsresult FlushCurrentWord();

  void UpdateCurrentWordLangGroup(nsIAtom *aLangGroup);

  void FindHyphenationPoints(nsHyphenator *aHyphenator,
                             const PRUnichar *aTextStart,
                             const PRUnichar *aTextLimit,
                             PRPackedBool *aBreakState);

  nsAutoTArray<PRUnichar,100> mCurrentWord;
  
  nsAutoTArray<TextItem,2>    mTextItems;
  nsIAtom*                    mCurrentWordLangGroup;
  PRPackedBool                mCurrentWordContainsMixedLang;
  PRPackedBool                mCurrentWordContainsComplexChar;

  
  PRPackedBool                mAfterBreakableSpace;
  
  
  PRPackedBool                mBreakHere;
};

#endif 
