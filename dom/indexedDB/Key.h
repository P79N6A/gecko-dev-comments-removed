






































#ifndef mozilla_dom_indexeddb_key_h__
#define mozilla_dom_indexeddb_key_h__

#include "mozilla/dom/indexedDB/IndexedDatabase.h"

#include "mozIStorageStatement.h"

#include "xpcprivate.h"
#include "XPCQuickStubs.h"

BEGIN_INDEXEDDB_NAMESPACE

class Key
{
public:
  Key()
  {
    Unset();
  }

  Key& operator=(const nsAString& aString)
  {
    SetFromString(aString);
    return *this;
  }

  Key& operator=(PRInt64 aInt)
  {
    SetFromInteger(aInt);
    return *this;
  }

  bool operator==(const Key& aOther) const
  {
     NS_ASSERTION(mType != KEYTYPE_VOID && aOther.mType != KEYTYPE_VOID,
                 "Don't compare unset keys!");

    if (mType == aOther.mType) {
      switch (mType) {
        case KEYTYPE_STRING:
          return ToString() == aOther.ToString();

        case KEYTYPE_INTEGER:
          return ToInteger() == aOther.ToInteger();

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
    NS_ASSERTION(mType != KEYTYPE_VOID && aOther.mType != KEYTYPE_VOID,
                 "Don't compare unset keys!");

    switch (mType) {
      case KEYTYPE_STRING: {
        if (aOther.mType == KEYTYPE_INTEGER) {
          return false;
        }
        NS_ASSERTION(aOther.mType == KEYTYPE_STRING, "Unknown type!");
        return ToString() < aOther.ToString();
      }

      case KEYTYPE_INTEGER:
        if (aOther.mType == KEYTYPE_STRING) {
          return true;
        }
        NS_ASSERTION(aOther.mType == KEYTYPE_INTEGER, "Unknown type!");
        return ToInteger() < aOther.ToInteger();

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

  void
  Unset()
  {
    mType = KEYTYPE_VOID;
    mStringKey.SetIsVoid(true);
    mIntKey = 0;
  }

  bool IsUnset() const { return mType == KEYTYPE_VOID; }
  bool IsString() const { return mType == KEYTYPE_STRING; }
  bool IsInteger() const { return mType == KEYTYPE_INTEGER; }

  nsresult SetFromString(const nsAString& aString)
  {
    mType = KEYTYPE_STRING;
    mStringKey = aString;
    mIntKey = 0;
    return NS_OK;
  }

  nsresult SetFromInteger(PRInt64 aInt)
  {
    mType = KEYTYPE_INTEGER;
    mStringKey.SetIsVoid(true);
    mIntKey = aInt;
    return NS_OK;
  }

  nsresult SetFromJSVal(JSContext* aCx,
                        jsval aVal)
  {
    if (JSVAL_IS_STRING(aVal)) {
      jsval tempRoot = JSVAL_VOID;
      SetFromString(xpc_qsAString(aCx, aVal, &tempRoot));
      return NS_OK;
    }

    if (JSVAL_IS_INT(aVal)) {
      SetFromInteger(JSVAL_TO_INT(aVal));
      return NS_OK;
    }

    if (JSVAL_IS_DOUBLE(aVal)) {
      jsdouble doubleActual = JSVAL_TO_DOUBLE(aVal);
      int64 doubleAsInt = static_cast<int64>(doubleActual);
      if (doubleActual == doubleAsInt) {
        SetFromInteger(doubleAsInt);
        return NS_OK;
      }
    }

    if (JSVAL_IS_NULL(aVal) || JSVAL_IS_VOID(aVal)) {
      Unset();
      return NS_OK;
    }

    return NS_ERROR_DOM_INDEXEDDB_DATA_ERR;
  }

  nsresult ToJSVal(JSContext* aCx,
                   jsval* aVal) const
  {
    if (IsString()) {
      if (!xpc_qsStringToJsval(aCx, nsString(ToString()), aVal)) {
        return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
      }
    }
    else if (IsInteger()) {
      if (!JS_NewNumberValue(aCx, static_cast<jsdouble>(ToInteger()), aVal)) {
        return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
      }
    }
    else if (IsUnset()) {
      *aVal = JSVAL_VOID;
    }
    else {
      NS_NOTREACHED("Unknown key type!");
    }
    return NS_OK;
  }

  PRInt64 ToInteger() const
  {
    NS_ASSERTION(IsInteger(), "Don't call me!");
    return mIntKey;
  }

  const nsAString& ToString() const
  {
    NS_ASSERTION(IsString(), "Don't call me!");
    return mStringKey;
  }

  nsresult BindToStatement(mozIStorageStatement* aStatement,
                           const nsACString& aParamName) const
  {
    nsresult rv;

    if (IsString()) {
      rv = aStatement->BindStringByName(aParamName, ToString());
    }
    else if (IsInteger()) {
      rv = aStatement->BindInt64ByName(aParamName, ToInteger());
    }
    else {
      NS_NOTREACHED("Bad key!");
    }

    return NS_SUCCEEDED(rv) ? NS_OK : NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
  }

  nsresult BindToStatementAllowUnset(mozIStorageStatement* aStatement,
                                     const nsACString& aParamName) const
  {
    nsresult rv;

    if (IsUnset()) {
      rv = aStatement->BindStringByName(aParamName, EmptyString());
      return NS_SUCCEEDED(rv) ? NS_OK : NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
    }

    return BindToStatement(aStatement, aParamName);
  }

  nsresult SetFromStatement(mozIStorageStatement* aStatement,
                            PRUint32 aIndex)
  {
    PRInt32 columnType;
    nsresult rv = aStatement->GetTypeOfIndex(aIndex, &columnType);
    NS_ENSURE_SUCCESS(rv, NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);

    NS_ASSERTION(columnType == mozIStorageStatement::VALUE_TYPE_INTEGER ||
                 columnType == mozIStorageStatement::VALUE_TYPE_TEXT,
                 "Unsupported column type!");

    return SetFromStatement(aStatement, aIndex, columnType);
  }

  nsresult SetFromStatement(mozIStorageStatement* aStatement,
                            PRUint32 aIndex,
                            PRInt32 aColumnType)
  {
    if (aColumnType == mozIStorageStatement::VALUE_TYPE_INTEGER) {
      return SetFromInteger(aStatement->AsInt64(aIndex));
    }

    if (aColumnType == mozIStorageStatement::VALUE_TYPE_TEXT) {
      nsString keyString;
      nsresult rv = aStatement->GetString(aIndex, keyString);
      NS_ENSURE_SUCCESS(rv, NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);

      return SetFromString(keyString);
    }

    NS_NOTREACHED("Unsupported column type!");
    return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
  }

  static
  bool CanBeConstructedFromJSVal(jsval aVal)
  {
    return JSVAL_IS_INT(aVal) || JSVAL_IS_DOUBLE(aVal) || JSVAL_IS_STRING(aVal);
  }

private:
  
  
  enum Type {
    KEYTYPE_VOID,
    KEYTYPE_STRING,
    KEYTYPE_INTEGER
  };

  
  Type mType;

  
  nsString mStringKey;

  
  int64 mIntKey;
};

END_INDEXEDDB_NAMESPACE

#endif 
