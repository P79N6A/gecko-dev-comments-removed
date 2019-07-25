










































#ifndef nsTextFragment_h___
#define nsTextFragment_h___

#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsTraceRefcnt.h"
class nsString;
class nsCString;






#define XP_IS_SPACE(_ch) \
  (((_ch) == ' ') || ((_ch) == '\t') || ((_ch) == '\n') || ((_ch) == '\r'))

#define XP_IS_UPPERCASE(_ch) \
  (((_ch) >= 'A') && ((_ch) <= 'Z'))

#define XP_IS_LOWERCASE(_ch) \
  (((_ch) >= 'a') && ((_ch) <= 'z'))

#define XP_TO_LOWER(_ch) ((_ch) | 32)

#define XP_TO_UPPER(_ch) ((_ch) & ~32)

#define XP_IS_SPACE_W XP_IS_SPACE










class nsTextFragment {
public:
  static nsresult Init();
  static void Shutdown();

  


  nsTextFragment()
    : m1b(nsnull), mAllBits(0)
  {
    MOZ_COUNT_CTOR(nsTextFragment);
    NS_ASSERTION(sizeof(FragmentBits) == 4, "Bad field packing!");
  }

  ~nsTextFragment();

  



  nsTextFragment& operator=(const nsTextFragment& aOther);

  


  PRBool Is2b() const
  {
    return mState.mIs2b;
  }

  




  PRBool IsBidi() const
  {
    return mState.mIsBidi;
  }

  


  const PRUnichar *Get2b() const
  {
    NS_ASSERTION(Is2b(), "not 2b text"); 
    return m2b;
  }

  


  const char *Get1b() const
  {
    NS_ASSERTION(!Is2b(), "not 1b text"); 
    return (const char *)m1b;
  }

  



  PRUint32 GetLength() const
  {
    return mState.mLength;
  }

  PRBool CanGrowBy(size_t n) const
  {
    return n < (1 << 29) && mState.mLength + n < (1 << 29);
  }

  



  void SetTo(const PRUnichar* aBuffer, PRInt32 aLength);

  


  void Append(const PRUnichar* aBuffer, PRUint32 aLength);

  


  void AppendTo(nsAString& aString) const {
    if (mState.mIs2b) {
      aString.Append(m2b, mState.mLength);
    } else {
      AppendASCIItoUTF16(Substring(m1b, m1b + mState.mLength),
                         aString);
    }
  }

  




  void AppendTo(nsAString& aString, PRInt32 aOffset, PRInt32 aLength) const {
    if (mState.mIs2b) {
      aString.Append(m2b + aOffset, aLength);
    } else {
      AppendASCIItoUTF16(Substring(m1b + aOffset, m1b + aOffset + aLength), aString);
    }
  }

  





  void CopyTo(PRUnichar *aDest, PRInt32 aOffset, PRInt32 aCount);

  



  PRUnichar CharAt(PRInt32 aIndex) const
  {
    NS_ASSERTION(PRUint32(aIndex) < mState.mLength, "bad index");
    return mState.mIs2b ? m2b[aIndex] : static_cast<unsigned char>(m1b[aIndex]);
  }

  



  void UpdateBidiFlag(const PRUnichar* aBuffer, PRUint32 aLength);

  struct FragmentBits {
    
    
    
    
    
    PRUint32 mInHeap : 1;
    PRUint32 mIs2b : 1;
    PRUint32 mIsBidi : 1;
    PRUint32 mLength : 29;
  };

private:
  void ReleaseText();

  union {
    PRUnichar *m2b;
    const char *m1b; 
  };

  union {
    PRUint32 mAllBits;
    FragmentBits mState;
  };
};

#endif 

