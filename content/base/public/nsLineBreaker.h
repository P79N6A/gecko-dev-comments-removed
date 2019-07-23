




































#ifndef NSLINEBREAKER_H_
#define NSLINEBREAKER_H_

#include "nsString.h"
#include "nsTArray.h"

class nsIAtom;




class nsILineBreakSink {
public:
  






  virtual void SetBreaks(PRUint32 aStart, PRUint32 aLength, PRPackedBool* aBreakBefore) = 0;
};



















class nsLineBreaker {
public:
  nsLineBreaker();
  ~nsLineBreaker();
  
  static inline PRBool IsSpace(PRUnichar u)
  {
    return u == 0x0020 || u == 0x200b || u == '\n' || u == '\t';
  }

  static inline PRBool IsComplexASCIIChar(PRUnichar u)
  {
    return !((0x0030 <= u && u <= 0x0039) ||
             (0x0041 <= u && u <= 0x005A) ||
             (0x0061 <= u && u <= 0x007A));
  }

  static inline PRBool IsComplexChar(PRUnichar u)
  {
    return IsComplexASCIIChar(u) ||
           (0x0e01 <= u && u <= 0x0edf) || 
           (0x1100 <= u && u <= 0x11ff) || 
           (0x2000 <= u && u <= 0x21ff) || 
           (0x2e80 <= u && u <= 0xd7ff) || 
           (0xf900 <= u && u <= 0xfaff) || 
           (0xff00 <= u && u <= 0xffef);   
  }

  
  
  
  
  
  
  
  
  
  
  
  
  enum {
    


    BREAK_ALLOW_INITIAL = 0x01,
    


    BREAK_ALLOW_INSIDE = 0x02
  };

  



  nsresult AppendInvisibleWhitespace();

  





  nsresult AppendText(nsIAtom* aLangGroup, const PRUnichar* aText, PRUint32 aLength,
                      PRUint32 aFlags, nsILineBreakSink* aSink);
  




  nsresult AppendText(nsIAtom* aLangGroup, const PRUint8* aText, PRUint32 aLength,
                      PRUint32 aFlags, nsILineBreakSink* aSink);
  







  nsresult Reset() { return FlushCurrentWord(); }

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

  nsAutoTArray<PRUnichar,100> mCurrentWord;
  
  nsAutoTArray<TextItem,2>    mTextItems;
  PRPackedBool                mCurrentWordContainsComplexChar;

  
  PRPackedBool                mAfterSpace;
};

#endif 
