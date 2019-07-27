





#ifndef mozilla_dom_indexeddb_keypath_h__
#define mozilla_dom_indexeddb_keypath_h__

#include "mozilla/dom/BindingDeclarations.h"
#include "mozilla/dom/Nullable.h"

namespace mozilla {
namespace dom {

class OwningStringOrStringSequence;

namespace indexedDB {

class IndexMetadata;
class Key;
class ObjectStoreMetadata;

class KeyPath
{
  
  friend class IndexMetadata;
  friend class ObjectStoreMetadata;

  KeyPath()
  : mType(NONEXISTENT)
  {
    MOZ_COUNT_CTOR(KeyPath);
  }

public:
  enum KeyPathType {
    NONEXISTENT,
    STRING,
    ARRAY,
    ENDGUARD
  };

  void SetType(KeyPathType aType);

  bool AppendStringWithValidation(const nsAString& aString);

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
  Parse(const nsAString& aString, KeyPath* aKeyPath);

  static nsresult
  Parse(const Sequence<nsString>& aStrings, KeyPath* aKeyPath);

  static nsresult
  Parse(const Nullable<OwningStringOrStringSequence>& aValue, KeyPath* aKeyPath);

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

  nsresult ToJSVal(JSContext* aCx, JS::MutableHandle<JS::Value> aValue) const;
  nsresult ToJSVal(JSContext* aCx, JS::Heap<JS::Value>& aValue) const;

  bool IsAllowedForObjectStore(bool aAutoIncrement) const;

  KeyPathType mType;

  nsTArray<nsString> mStrings;
};

} 
} 
} 

#endif
