






































#ifndef mozilla_dom_indexeddb_key_h__
#define mozilla_dom_indexeddb_key_h__

#include "mozilla/dom/indexedDB/IndexedDatabase.h"

BEGIN_INDEXEDDB_NAMESPACE

class Key
{
public:
  enum Type { UNSETKEY, STRINGKEY, INTKEY };

  Key()
  : mType(UNSETKEY), mInt(0)
  { }

  Key(const Key& aOther)
  {
    *this = aOther;
  }

  Key& operator=(const Key& aOther)
  {
    if (this != &aOther) {
      mType = aOther.mType;
      mString = aOther.mString;
      mInt = aOther.mInt;
    }
    return *this;
  }

  Key& operator=(Type aType)
  {
    NS_ASSERTION(aType == UNSETKEY ,
                 "Use one of the other operators to assign your value!");
    mType = aType;
    mString.Truncate();
    mInt = 0;
    return *this;
  }

  Key& operator=(const nsAString& aString)
  {
    mType = STRINGKEY;
    mString = aString;
    mInt = 0;
    return *this;
  }

  Key& operator=(PRInt64 aInt)
  {
    mType = INTKEY;
    mString.Truncate();
    mInt = aInt;
    return *this;
  }

  bool operator==(const Key& aOther) const
  {
    if (mType == aOther.mType) {
      switch (mType) {
        case UNSETKEY:
          return true;

        case STRINGKEY:
          return mString == aOther.mString;

        case INTKEY:
          return mInt == aOther.mInt;

        default:
          NS_NOTREACHED("Unknown type!");
      }
    }
    return false;
  }

  bool operator!=(const Key& aOther) const
  {
    return !(*this == aOther);
  }

  bool operator<(const Key& aOther) const
  {
    switch (mType) {
      case UNSETKEY:
        if (aOther.mType == UNSETKEY) {
          return false;
        }
        return true;

      case STRINGKEY:
        if (aOther.mType == UNSETKEY ||
            aOther.mType == INTKEY) {
          return false;
        }
        NS_ASSERTION(aOther.mType == STRINGKEY, "Unknown type!");
        return mString < aOther.mString;

      case INTKEY:
        if (aOther.mType == UNSETKEY) {
          return false;
        }
        if (aOther.mType == STRINGKEY) {
          return true;
        }
        NS_ASSERTION(aOther.mType == INTKEY, "Unknown type!");
        return mInt < aOther.mInt;

      default:
        NS_NOTREACHED("Unknown type!");
    }
    return false;
  }

  bool operator>(const Key& aOther) const
  {
    return !(*this == aOther || *this < aOther);
  }

  bool operator<=(const Key& aOther) const
  {
    return (*this == aOther || *this < aOther);
  }

  bool operator>=(const Key& aOther) const
  {
    return (*this == aOther || !(*this < aOther));
  }

  bool IsUnset() const { return mType == UNSETKEY; }
  bool IsString() const { return mType == STRINGKEY; }
  bool IsInt() const { return mType == INTKEY; }

  const nsString& StringValue() const {
    NS_ASSERTION(IsString(), "Wrong type!");
    return mString;
  }

  PRInt64 IntValue() const {
    NS_ASSERTION(IsInt(), "Wrong type!");
    return mInt;
  }

  nsAString& ToString() {
    mType = STRINGKEY;
    mInt = 0;
    return mString;
  }

  PRInt64* ToIntPtr() {
    mType = INTKEY;
    mString.Truncate();
    return &mInt;
  }

  static
  JSBool CanBeConstructedFromJSVal(jsval aVal) {
    return JSVAL_IS_VOID(aVal) || JSVAL_IS_NULL(aVal) || JSVAL_IS_INT(aVal) ||
           JSVAL_IS_DOUBLE(aVal) || JSVAL_IS_STRING(aVal);
  }

private:
  Type mType;
  nsString mString;
  PRInt64 mInt;
};

END_INDEXEDDB_NAMESPACE

#endif 
