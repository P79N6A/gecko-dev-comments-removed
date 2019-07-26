





#ifndef mozilla_dom_indexeddb_keypath_h__
#define mozilla_dom_indexeddb_keypath_h__

#include "mozilla/dom/indexedDB/IndexedDatabase.h"

namespace IPC {

template <class T> struct ParamTraits;

} 

BEGIN_INDEXEDDB_NAMESPACE

class Key;

class KeyPath
{
  template<class T> friend struct IPC::ParamTraits;

  enum KeyPathType {
    NONEXISTENT,
    STRING,
    ARRAY,
    ENDGUARD
  };

  void SetType(KeyPathType aType);

  
  bool AppendStringWithValidation(JSContext* aCx, const nsAString& aString);

public:
  explicit KeyPath(int aDummy)
  : mType(NONEXISTENT)
  {
    MOZ_COUNT_CTOR(KeyPath);
  }

  KeyPath(const KeyPath& aOther)
  {
    MOZ_COUNT_CTOR(KeyPath);
    *this = aOther;
  }

  ~KeyPath()
  {
    MOZ_COUNT_DTOR(KeyPath);
  }

  static nsresult
  Parse(JSContext* aCx, const JS::Value& aValue, KeyPath* aKeyPath);

  nsresult
  ExtractKey(JSContext* aCx, const JS::Value& aValue, Key& aKey) const;

  nsresult
  ExtractKeyAsJSVal(JSContext* aCx, const JS::Value& aValue,
                    JS::Value* aOutVal) const;

  typedef nsresult
  (*ExtractOrCreateKeyCallback)(JSContext* aCx, void* aClosure);

  nsresult
  ExtractOrCreateKey(JSContext* aCx, const JS::Value& aValue, Key& aKey,
                     ExtractOrCreateKeyCallback aCallback,
                     void* aClosure) const;

  inline bool IsValid() const {
    return mType != NONEXISTENT;
  }

  inline bool IsArray() const {
    return mType == ARRAY;
  }

  inline bool IsString() const {
    return mType == STRING;
  }

  inline bool IsEmpty() const {
    return mType == STRING && mStrings[0].IsEmpty();
  }

  bool operator==(const KeyPath& aOther) const
  {
    return mType == aOther.mType && mStrings == aOther.mStrings;
  }

  void SerializeToString(nsAString& aString) const;
  static KeyPath DeserializeFromString(const nsAString& aString);

  nsresult ToJSVal(JSContext* aCx, JS::Value* aValue) const;

  bool IsAllowedForObjectStore(bool aAutoIncrement) const;

private:
  KeyPathType mType;

  nsTArray<nsString> mStrings;
};

END_INDEXEDDB_NAMESPACE

#endif 
