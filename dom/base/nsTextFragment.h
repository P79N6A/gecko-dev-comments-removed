









#ifndef nsTextFragment_h___
#define nsTextFragment_h___

#include "mozilla/Attributes.h"
#include "mozilla/MemoryReporting.h"

#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsISupportsImpl.h"

class nsString;
class nsCString;














class nsTextFragment final {
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

  


  const char16_t *Get2b() const
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

  




  bool SetTo(const char16_t* aBuffer, int32_t aLength, bool aUpdateBidi);

  




  bool Append(const char16_t* aBuffer, uint32_t aLength, bool aUpdateBidi);

  


  void AppendTo(nsAString& aString) const {
    if (!AppendTo(aString, mozilla::fallible)) {
      aString.AllocFailed(aString.Length() + GetLength());
    }
  }

  



  MOZ_WARN_UNUSED_RESULT
  bool AppendTo(nsAString& aString,
                const mozilla::fallible_t& aFallible) const {
    if (mState.mIs2b) {
      bool ok = aString.Append(m2b, mState.mLength, aFallible);
      if (!ok) {
        return false;
      }

      return true;
    } else {
      return AppendASCIItoUTF16(Substring(m1b, mState.mLength), aString,
                                aFallible);
    }
  }

  




  void AppendTo(nsAString& aString, int32_t aOffset, int32_t aLength) const {
    if (!AppendTo(aString, aOffset, aLength, mozilla::fallible)) {
      aString.AllocFailed(aString.Length() + aLength);
    }
  }

  






  MOZ_WARN_UNUSED_RESULT
  bool AppendTo(nsAString& aString, int32_t aOffset, int32_t aLength,
                const mozilla::fallible_t& aFallible) const
  {
    if (mState.mIs2b) {
      bool ok = aString.Append(m2b + aOffset, aLength, aFallible);
      if (!ok) {
        return false;
      }

      return true;
    } else {
      return AppendASCIItoUTF16(Substring(m1b + aOffset, aLength), aString,
                                aFallible);
    }
  }

  





  void CopyTo(char16_t *aDest, int32_t aOffset, int32_t aCount);

  



  char16_t CharAt(int32_t aIndex) const
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

  size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

private:
  void ReleaseText();

  



  void UpdateBidiFlag(const char16_t* aBuffer, uint32_t aLength);
 
  union {
    char16_t *m2b;
    const char *m1b; 
  };

  union {
    uint32_t mAllBits;
    FragmentBits mState;
  };
};

#endif 

