






































#ifndef mozilla_dom_indexeddb_idbobjectstorerequest_h__
#define mozilla_dom_indexeddb_idbobjectstorerequest_h__

#include "mozilla/dom/indexedDB/IDBRequest.h"
#include "mozilla/dom/indexedDB/IDBDatabaseRequest.h"

#include "nsIIDBObjectStoreRequest.h"

BEGIN_INDEXEDDB_NAMESPACE

class IDBTransactionRequest;

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
  Create(IDBDatabaseRequest* aDatabase,
         IDBTransactionRequest* aTransaction,
         const ObjectStoreInfo& aStoreInfo,
         PRUint16 aMode);

protected:
  IDBObjectStoreRequest();
  ~IDBObjectStoreRequest();

  nsresult GetJSONAndKeyForAdd(
                               nsIVariant* aKeyVariant,
                               nsString& aJSON,
                               Key& aKey);

private:
  nsRefPtr<IDBDatabaseRequest> mDatabase;
  nsRefPtr<IDBTransactionRequest> mTransaction;

  nsString mName;
  PRInt64 mId;
  nsString mKeyPath;
  PRBool mAutoIncrement;
  PRUint16 mMode;

  nsTArray<nsString> mIndexes;
};

END_INDEXEDDB_NAMESPACE

#endif 
