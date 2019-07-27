




#ifndef mozilla_dom_DOMString_h
#define mozilla_dom_DOMString_h

#include "nsStringGlue.h"
#include "nsStringBuffer.h"
#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"
#include "mozilla/Maybe.h"
#include "nsDOMString.h"
#include "nsIAtom.h"

namespace mozilla {
namespace dom {





















class MOZ_STACK_CLASS DOMString {
public:
  DOMString()
    : mStringBuffer(nullptr)
    , mLength(0)
    , mIsNull(false)
  {}
  ~DOMString()
  {
    MOZ_ASSERT(!mString || !mStringBuffer,
               "Shouldn't have both present!");
  }

  operator nsString&()
  {
    return AsAString();
  }

  nsString& AsAString()
  {
    MOZ_ASSERT(!mStringBuffer, "We already have a stringbuffer?");
    MOZ_ASSERT(!mIsNull, "We're already set as null");
    if (!mString) {
      mString.emplace();
    }
    return *mString;
  }

  bool HasStringBuffer() const
  {
    MOZ_ASSERT(!mString || !mStringBuffer,
               "Shouldn't have both present!");
    MOZ_ASSERT(!mIsNull, "Caller should have checked IsNull() first");
    return !mString;
  }

  
  
  
  nsStringBuffer* StringBuffer() const
  {
    MOZ_ASSERT(!mIsNull, "Caller should have checked IsNull() first");
    MOZ_ASSERT(HasStringBuffer(),
               "Don't ask for the stringbuffer if we don't have it");
    MOZ_ASSERT(StringBufferLength() != 0, "Why are you asking for this?");
    MOZ_ASSERT(mStringBuffer,
               "If our length is nonzero, we better have a stringbuffer.");
    return mStringBuffer;
  }

  
  
  uint32_t StringBufferLength() const
  {
    MOZ_ASSERT(HasStringBuffer(), "Don't call this if there is no stringbuffer");
    return mLength;
  }

  void SetStringBuffer(nsStringBuffer* aStringBuffer, uint32_t aLength)
  {
    MOZ_ASSERT(mString.isNothing(), "We already have a string?");
    MOZ_ASSERT(!mIsNull, "We're already set as null");
    MOZ_ASSERT(!mStringBuffer, "Setting stringbuffer twice?");
    MOZ_ASSERT(aStringBuffer, "Why are we getting null?");
    mStringBuffer = aStringBuffer;
    mLength = aLength;
  }

  void SetOwnedString(const nsAString& aString)
  {
    MOZ_ASSERT(mString.isNothing(), "We already have a string?");
    MOZ_ASSERT(!mIsNull, "We're already set as null");
    MOZ_ASSERT(!mStringBuffer, "Setting stringbuffer twice?");
    nsStringBuffer* buf = nsStringBuffer::FromString(aString);
    if (buf) {
      SetStringBuffer(buf, aString.Length());
    } else if (aString.IsVoid()) {
      SetNull();
    } else if (!aString.IsEmpty()) {
      AsAString() = aString;
    }
  }

  enum NullHandling
  {
    eTreatNullAsNull,
    eTreatNullAsEmpty,
    eNullNotExpected
  };

  void SetOwnedAtom(nsIAtom* aAtom, NullHandling aNullHandling)
  {
    MOZ_ASSERT(mString.isNothing(), "We already have a string?");
    MOZ_ASSERT(!mIsNull, "We're already set as null");
    MOZ_ASSERT(!mStringBuffer, "Setting stringbuffer twice?");
    MOZ_ASSERT(aAtom || aNullHandling != eNullNotExpected);
    if (aNullHandling == eNullNotExpected || aAtom) {
      SetStringBuffer(aAtom->GetStringBuffer(), aAtom->GetLength());
    } else if (aNullHandling == eTreatNullAsNull) {
      SetNull();
    }
  }

  void SetNull()
  {
    MOZ_ASSERT(!mStringBuffer, "Should have no stringbuffer if null");
    MOZ_ASSERT(mString.isNothing(), "Should have no string if null");
    mIsNull = true;
  }

  bool IsNull() const
  {
    MOZ_ASSERT(!mStringBuffer || mString.isNothing(),
               "How could we have a stringbuffer and a nonempty string?");
    return mIsNull || (mString && mString->IsVoid());
  }

  void ToString(nsAString& aString)
  {
    if (IsNull()) {
      SetDOMStringToNull(aString);
    } else if (HasStringBuffer()) {
      if (StringBufferLength() == 0) {
        aString.Truncate();
      } else {
        StringBuffer()->ToString(StringBufferLength(), aString);
      }
    } else {
      aString = AsAString();
    }
  }

private:
  
  Maybe<nsAutoString> mString;

  
  
  nsStringBuffer* mStringBuffer;
  uint32_t mLength;
  bool mIsNull;
};

} 
} 

#endif 
