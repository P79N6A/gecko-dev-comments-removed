




































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

  
  
  
  
  
  
  
  
  
  
  enum {
    


    BREAK_ALLOW_INITIAL = 0x01,
    


    BREAK_ALLOW_INSIDE = 0x02,
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
  PRPackedBool                mCurrentWordContainsCJK;

  
  PRPackedBool                mAfterSpace;
};

#endif 
