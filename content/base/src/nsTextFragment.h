










































#ifndef nsTextFragment_h___
#define nsTextFragment_h___

#include "nsAString.h"
class nsString;
class nsCString;






#define XP_IS_SPACE(_ch) \
  (((_ch) == ' ') || ((_ch) == '\t') || ((_ch) == '\n'))

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

  



  void SetTo(const PRUnichar* aBuffer, PRInt32 aLength);

  


  void Append(const PRUnichar* aBuffer, PRUint32 aLength);

  


  void AppendTo(nsAString& aString) const;

  




  void AppendTo(nsAString& aString, PRInt32 aOffset, PRInt32 aLength) const;

  





  void CopyTo(PRUnichar *aDest, PRInt32 aOffset, PRInt32 aCount);

  



  PRUnichar CharAt(PRInt32 aIndex) const
  {
    NS_ASSERTION(PRUint32(aIndex) < mState.mLength, "bad index");
    return mState.mIs2b ? m2b[aIndex] : NS_STATIC_CAST(unsigned char, m1b[aIndex]);
  }

  



  void SetBidiFlag();

  struct FragmentBits {
    PRBool mInHeap : 1;
    PRBool mIs2b : 1;
    PRBool mIsBidi : 1;
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

