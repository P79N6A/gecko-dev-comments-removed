









#ifndef nsTextFragment_h___
#define nsTextFragment_h___

#include "mozilla/Attributes.h"

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










class nsTextFragment MOZ_FINAL {
public:
  static nsresult Init();
  static void Shutdown();

  


  nsTextFragment()
    : m1b(nullptr), mAllBits(0)
  {
    MOZ_COUNT_CTOR(nsTextFragment);
    NS_ASSERTION(sizeof(FragmentBits) == 4, "Bad field packing!");
  }

  ~nsTextFragment();

  



  nsTextFragment& operator=(const nsTextFragment& aOther);

  


  bool Is2b() const
  {
    return mState.mIs2b;
  }

  




  bool IsBidi() const
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

  



  uint32_t GetLength() const
  {
    return mState.mLength;
  }

  bool CanGrowBy(size_t n) const
  {
    return n < (1 << 29) && mState.mLength + n < (1 << 29);
  }

  




  void SetTo(const PRUnichar* aBuffer, int32_t aLength, bool aUpdateBidi);

  




  void Append(const PRUnichar* aBuffer, uint32_t aLength, bool aUpdateBidi);

  


  void AppendTo(nsAString& aString) const {
    if (mState.mIs2b) {
      aString.Append(m2b, mState.mLength);
    } else {
      AppendASCIItoUTF16(Substring(m1b, mState.mLength), aString);
    }
  }

  




  void AppendTo(nsAString& aString, int32_t aOffset, int32_t aLength) const {
    if (mState.mIs2b) {
      aString.Append(m2b + aOffset, aLength);
    } else {
      AppendASCIItoUTF16(Substring(m1b + aOffset, aLength), aString);
    }
  }

  





  void CopyTo(PRUnichar *aDest, int32_t aOffset, int32_t aCount);

  



  PRUnichar CharAt(int32_t aIndex) const
  {
    NS_ASSERTION(uint32_t(aIndex) < mState.mLength, "bad index");
    return mState.mIs2b ? m2b[aIndex] : static_cast<unsigned char>(m1b[aIndex]);
  }

  struct FragmentBits {
    
    
    
    
    
    uint32_t mInHeap : 1;
    uint32_t mIs2b : 1;
    uint32_t mIsBidi : 1;
    uint32_t mLength : 29;
  };

  size_t SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf) const;

private:
  void ReleaseText();

  



  void UpdateBidiFlag(const PRUnichar* aBuffer, uint32_t aLength);
 
  union {
    PRUnichar *m2b;
    const char *m1b; 
  };

  union {
    uint32_t mAllBits;
    FragmentBits mState;
  };
};

#endif 

