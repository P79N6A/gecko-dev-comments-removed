






































#ifndef mozilla_dom_indexeddb_idbobjectstorerequest_h__
#define mozilla_dom_indexeddb_idbobjectstorerequest_h__

#include "mozilla/dom/indexedDB/IDBRequest.h"
#include "mozilla/dom/indexedDB/IDBDatabase.h"
#include "mozilla/dom/indexedDB/IDBTransaction.h"

#include "nsIIDBObjectStoreRequest.h"

struct JSContext;

BEGIN_INDEXEDDB_NAMESPACE

struct ObjectStoreInfo;
struct IndexInfo;
struct IndexUpdateInfo;

class Key
{
public:
  enum Type { UNSETKEY, NULLKEY, STRINGKEY, INTKEY };

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
    NS_ASSERTION(aType == UNSETKEY || aType == NULLKEY,
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
        case NULLKEY:
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

  bool operator<(const Key& aOther) const
  {
    switch (mType) {
      case UNSETKEY:
        if (aOther.mType == UNSETKEY) {
          return false;
        }
        return true;

      case NULLKEY:
        if (aOther.mType == UNSETKEY ||
            aOther.mType == NULLKEY) {
          return false;
        }
        return true;

      case STRINGKEY:
        if (aOther.mType == UNSETKEY ||
            aOther.mType == NULLKEY ||
            aOther.mType == INTKEY) {
          return false;
        }
        NS_ASSERTION(aOther.mType == STRINGKEY, "Unknown type!");
        return mString < aOther.mString;

      case INTKEY:
        if (aOther.mType == UNSETKEY ||
            aOther.mType == NULLKEY) {
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

  bool IsUnset() const { return mType == UNSETKEY; }
  bool IsNull() const { return mType == NULLKEY; }
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

private:
  Type mType;
  nsString mString;
  PRInt64 mInt;
};

class IDBObjectStoreRequest : public IDBRequest::Generator,
                              public nsIIDBObjectStoreRequest
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIIDBOBJECTSTORE
  NS_DECL_NSIIDBOBJECTSTOREREQUEST

  static already_AddRefed<IDBObjectStoreRequest>
  Create(IDBDatabase* aDatabase,
         IDBTransaction* aTransaction,
         const ObjectStoreInfo* aInfo,
         PRUint16 aMode);

  static nsresult
  GetKeyFromVariant(nsIVariant* aKeyVariant,
                    Key& aKey);

  static nsresult
  GetJSONFromArg0(
                  nsAString& aJSON);

  static nsresult
  GetKeyPathValueFromJSON(const nsAString& aJSON,
                          const nsAString& aKeyPath,
                          JSContext** aCx,
                          Key& aValue);

  static nsresult
  GetIndexUpdateInfo(ObjectStoreInfo* aObjectStoreInfo,
                     JSContext* aCx,
                     jsval aObject,
                     nsTArray<IndexUpdateInfo>& aUpdateInfoArray);

  static nsresult
  UpdateIndexes(IDBTransaction* aTransaction,
                PRInt64 aObjectStoreId,
                const Key& aObjectStoreKey,
                bool aAutoIncrement,
                bool aOverwrite,
                PRInt64 aObjectDataId,
                const nsTArray<IndexUpdateInfo>& aUpdateInfoArray);


  bool TransactionIsOpen() const
  {
    return mTransaction->TransactionIsOpen();
  }

  bool IsAutoIncrement() const
  {
    return mAutoIncrement;
  }

  bool IsWriteAllowed() const
  {
    return mTransaction->IsWriteAllowed();
  }

  PRInt64 Id() const
  {
    return mId;
  }

  const nsString& KeyPath() const
  {
    return mKeyPath;
  }

  IDBTransaction* Transaction()
  {
    return mTransaction;
  }

  ObjectStoreInfo* GetObjectStoreInfo();

protected:
  IDBObjectStoreRequest();
  ~IDBObjectStoreRequest();

  nsresult GetAddInfo(
                      nsIVariant* aKeyVariant,
                      nsString& aJSON,
                      Key& aKey,
                      nsTArray<IndexUpdateInfo>& aUpdateInfoArray);

private:
  nsRefPtr<IDBDatabase> mDatabase;
  nsRefPtr<IDBTransaction> mTransaction;

  PRInt64 mId;
  nsString mName;
  nsString mKeyPath;
  PRBool mAutoIncrement;
  PRUint32 mDatabaseId;
  PRUint16 mMode;
};

END_INDEXEDDB_NAMESPACE

#endif 
