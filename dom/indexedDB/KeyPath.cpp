





#include "KeyPath.h"
#include "IDBObjectStore.h"
#include "Key.h"
#include "ReportInternalError.h"

#include "nsCharSeparatedTokenizer.h"
#include "nsJSUtils.h"
#include "xpcpublic.h"

#include "mozilla/dom/BindingDeclarations.h"

USING_INDEXEDDB_NAMESPACE

namespace {

inline
bool
IgnoreWhitespace(char16_t c)
{
  return false;
}

typedef nsCharSeparatedTokenizerTemplate<IgnoreWhitespace> KeyPathTokenizer;

bool
IsValidKeyPathString(JSContext* aCx, const nsAString& aKeyPath)
{
  NS_ASSERTION(!aKeyPath.IsVoid(), "What?");

  KeyPathTokenizer tokenizer(aKeyPath, '.');

  while (tokenizer.hasMoreTokens()) {
    nsString token(tokenizer.nextToken());

    if (!token.Length()) {
      return false;
    }

    JS::Rooted<JS::Value> stringVal(aCx);
    if (!xpc::StringToJsval(aCx, token, &stringVal)) {
      return false;
    }

    NS_ASSERTION(stringVal.toString(), "This should never happen");
    JS::Rooted<JSString*> str(aCx, stringVal.toString());

    bool isIdentifier = false;
    if (!JS_IsIdentifier(aCx, str, &isIdentifier) || !isIdentifier) {
      return false;
    }
  }

  
  
  if (!aKeyPath.IsEmpty() &&
      aKeyPath.CharAt(aKeyPath.Length() - 1) == '.') {
    return false;
  }

  return true;
}

enum KeyExtractionOptions {
  DoNotCreateProperties,
  CreateProperties
};

nsresult
GetJSValFromKeyPathString(JSContext* aCx,
                          const JS::Value& aValue,
                          const nsAString& aKeyPathString,
                          JS::Value* aKeyJSVal,
                          KeyExtractionOptions aOptions,
                          KeyPath::ExtractOrCreateKeyCallback aCallback,
                          void* aClosure)
{
  NS_ASSERTION(aCx, "Null pointer!");
  NS_ASSERTION(IsValidKeyPathString(aCx, aKeyPathString),
               "This will explode!");
  NS_ASSERTION(!(aCallback || aClosure) || aOptions == CreateProperties,
               "This is not allowed!");
  NS_ASSERTION(aOptions != CreateProperties || aCallback,
               "If properties are created, there must be a callback!");

  nsresult rv = NS_OK;
  *aKeyJSVal = aValue;

  KeyPathTokenizer tokenizer(aKeyPathString, '.');

  nsString targetObjectPropName;
  JS::Rooted<JSObject*> targetObject(aCx, nullptr);
  JS::Rooted<JSObject*> obj(aCx,
    aValue.isPrimitive() ? nullptr : aValue.toObjectOrNull());

  while (tokenizer.hasMoreTokens()) {
    const nsDependentSubstring& token = tokenizer.nextToken();

    NS_ASSERTION(!token.IsEmpty(), "Should be a valid keypath");

    const char16_t* keyPathChars = token.BeginReading();
    const size_t keyPathLen = token.Length();

    bool hasProp;
    if (!targetObject) {
      
      if (!obj) {
        return NS_ERROR_DOM_INDEXEDDB_DATA_ERR;
      }

      bool ok = JS_HasUCProperty(aCx, obj, keyPathChars, keyPathLen,
                                 &hasProp);
      IDB_ENSURE_TRUE(ok, NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);

      if (hasProp) {
        
        JS::Rooted<JS::Value> intermediate(aCx);
        bool ok = JS_GetUCProperty(aCx, obj, keyPathChars, keyPathLen, &intermediate);
        IDB_ENSURE_TRUE(ok, NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);

        
        if (intermediate == JSVAL_VOID) {
          return NS_ERROR_DOM_INDEXEDDB_DATA_ERR;
        }
        if (tokenizer.hasMoreTokens()) {
          
          if (intermediate.isPrimitive()) {
            return NS_ERROR_DOM_INDEXEDDB_DATA_ERR;
          }
          obj = intermediate.toObjectOrNull();
        }
        else {
          
          *aKeyJSVal = intermediate;
        }
      }
      else {
        
        
        if (aOptions == DoNotCreateProperties) {
          return NS_ERROR_DOM_INDEXEDDB_DATA_ERR;
        }

        targetObject = obj;
        targetObjectPropName = token;
      }
    }

    if (targetObject) {
      
      

      *aKeyJSVal = JSVAL_VOID;

      if (tokenizer.hasMoreTokens()) {
        
        
        JS::Rooted<JSObject*> dummy(aCx, JS_NewObject(aCx, nullptr, JS::NullPtr(),
                                                      JS::NullPtr()));
        if (!dummy) {
          IDB_REPORT_INTERNAL_ERR();
          rv = NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
          break;
        }

        if (!JS_DefineUCProperty(aCx, obj, token.BeginReading(),
                                 token.Length(), dummy, JSPROP_ENUMERATE)) {
          IDB_REPORT_INTERNAL_ERR();
          rv = NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
          break;
        }

        obj = dummy;
      }
      else {
        JS::Rooted<JSObject*> dummy(aCx, JS_NewObject(aCx, &IDBObjectStore::sDummyPropJSClass,
                                                      JS::NullPtr(), JS::NullPtr()));
        if (!dummy) {
          IDB_REPORT_INTERNAL_ERR();
          rv = NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
          break;
        }

        if (!JS_DefineUCProperty(aCx, obj, token.BeginReading(),
                                 token.Length(), dummy, JSPROP_ENUMERATE)) {
          IDB_REPORT_INTERNAL_ERR();
          rv = NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
          break;
        }

        obj = dummy;
      }
    }
  }

  
  
  if (NS_SUCCEEDED(rv) && aCallback) {
    rv = (*aCallback)(aCx, aClosure);
  }

  if (targetObject) {
    
    
    bool succeeded;
    if (!JS_DeleteUCProperty2(aCx, targetObject,
                              targetObjectPropName.get(),
                              targetObjectPropName.Length(),
                              &succeeded)) {
      IDB_REPORT_INTERNAL_ERR();
      return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
    }
    IDB_ENSURE_TRUE(succeeded, NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);
  }

  NS_ENSURE_SUCCESS(rv, rv);
  return rv;
}

} 


nsresult
KeyPath::Parse(JSContext* aCx, const nsAString& aString, KeyPath* aKeyPath)
{
  KeyPath keyPath(0);
  keyPath.SetType(STRING);

  if (!keyPath.AppendStringWithValidation(aCx, aString)) {
    return NS_ERROR_FAILURE;
  }

  *aKeyPath = keyPath;
  return NS_OK;
}


nsresult
KeyPath::Parse(JSContext* aCx, const mozilla::dom::Sequence<nsString>& aStrings,
               KeyPath* aKeyPath)
{
  KeyPath keyPath(0);
  keyPath.SetType(ARRAY);

  for (uint32_t i = 0; i < aStrings.Length(); ++i) {
    if (!keyPath.AppendStringWithValidation(aCx, aStrings[i])) {
      return NS_ERROR_FAILURE;
    }
  }

  *aKeyPath = keyPath;
  return NS_OK;
}


nsresult
KeyPath::Parse(JSContext* aCx, const JS::Value& aValue_, KeyPath* aKeyPath)
{
  JS::Rooted<JS::Value> aValue(aCx, aValue_);
  KeyPath keyPath(0);

  aKeyPath->SetType(NONEXISTENT);

  
  if (JS_IsArrayObject(aCx, aValue)) {

    JS::Rooted<JSObject*> obj(aCx, aValue.toObjectOrNull());

    uint32_t length;
    if (!JS_GetArrayLength(aCx, obj, &length)) {
      return NS_ERROR_FAILURE;
    }

    if (!length) {
      return NS_ERROR_FAILURE;
    }

    keyPath.SetType(ARRAY);

    for (uint32_t index = 0; index < length; index++) {
      JS::Rooted<JS::Value> val(aCx);
      JSString* jsstr;
      nsAutoJSString str;
      if (!JS_GetElement(aCx, obj, index, &val) ||
          !(jsstr = JS::ToString(aCx, val)) ||
          !str.init(aCx, jsstr)) {
        return NS_ERROR_FAILURE;
      }

      if (!keyPath.AppendStringWithValidation(aCx, str)) {
        return NS_ERROR_FAILURE;
      }
    }
  }
  
  else if (!aValue.isNull() && !aValue.isUndefined()) {
    JSString* jsstr;
    nsAutoJSString str;
    if (!(jsstr = JS::ToString(aCx, aValue)) ||
        !str.init(aCx, jsstr)) {
      return NS_ERROR_FAILURE;
    }

    keyPath.SetType(STRING);

    if (!keyPath.AppendStringWithValidation(aCx, str)) {
      return NS_ERROR_FAILURE;
    }
  }

  *aKeyPath = keyPath;
  return NS_OK;
}

void
KeyPath::SetType(KeyPathType aType)
{
  mType = aType;
  mStrings.Clear();
}

bool
KeyPath::AppendStringWithValidation(JSContext* aCx, const nsAString& aString)
{
  if (!IsValidKeyPathString(aCx, aString)) {
    return false;
  }

  if (IsString()) {
    NS_ASSERTION(mStrings.Length() == 0, "Too many strings!");
    mStrings.AppendElement(aString);
    return true;
  }

  if (IsArray()) {
    mStrings.AppendElement(aString);
    return true;
  }

  NS_NOTREACHED("What?!");
  return false;
}

nsresult
KeyPath::ExtractKey(JSContext* aCx, const JS::Value& aValue, Key& aKey) const
{
  uint32_t len = mStrings.Length();
  JS::Rooted<JS::Value> value(aCx);

  aKey.Unset();

  for (uint32_t i = 0; i < len; ++i) {
    nsresult rv = GetJSValFromKeyPathString(aCx, aValue, mStrings[i],
                                            value.address(),
                                            DoNotCreateProperties, nullptr,
                                            nullptr);
    if (NS_FAILED(rv)) {
      return rv;
    }

    if (NS_FAILED(aKey.AppendItem(aCx, IsArray() && i == 0, value))) {
      NS_ASSERTION(aKey.IsUnset(), "Encoding error should unset");
      return NS_ERROR_DOM_INDEXEDDB_DATA_ERR;
    }
  }

  aKey.FinishArray();

  return NS_OK;
}

nsresult
KeyPath::ExtractKeyAsJSVal(JSContext* aCx, const JS::Value& aValue,
                           JS::Value* aOutVal) const
{
  NS_ASSERTION(IsValid(), "This doesn't make sense!");

  if (IsString()) {
    return GetJSValFromKeyPathString(aCx, aValue, mStrings[0], aOutVal,
                                     DoNotCreateProperties, nullptr, nullptr);
  }

  const uint32_t len = mStrings.Length();
  JS::Rooted<JSObject*> arrayObj(aCx, JS_NewArrayObject(aCx, len));
  if (!arrayObj) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  JS::Rooted<JS::Value> value(aCx);
  for (uint32_t i = 0; i < len; ++i) {
    nsresult rv = GetJSValFromKeyPathString(aCx, aValue, mStrings[i],
                                            value.address(),
                                            DoNotCreateProperties, nullptr,
                                            nullptr);
    if (NS_FAILED(rv)) {
      return rv;
    }

    if (!JS_SetElement(aCx, arrayObj, i, value)) {
      IDB_REPORT_INTERNAL_ERR();
      return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
    }
  }

  *aOutVal = OBJECT_TO_JSVAL(arrayObj);
  return NS_OK;
}

nsresult
KeyPath::ExtractOrCreateKey(JSContext* aCx, const JS::Value& aValue,
                            Key& aKey, ExtractOrCreateKeyCallback aCallback,
                            void* aClosure) const
{
  NS_ASSERTION(IsString(), "This doesn't make sense!");

  JS::Rooted<JS::Value> value(aCx);

  aKey.Unset();

  nsresult rv = GetJSValFromKeyPathString(aCx, aValue, mStrings[0],
                                          value.address(),
                                          CreateProperties, aCallback,
                                          aClosure);
  if (NS_FAILED(rv)) {
    return rv;
  }

  if (NS_FAILED(aKey.AppendItem(aCx, false, value))) {
    NS_ASSERTION(aKey.IsUnset(), "Should be unset");
    return value.isUndefined() ? NS_OK : NS_ERROR_DOM_INDEXEDDB_DATA_ERR;
  }

  aKey.FinishArray();

  return NS_OK;
}

void
KeyPath::SerializeToString(nsAString& aString) const
{
  NS_ASSERTION(IsValid(), "Check to see if I'm valid first!");

  if (IsString()) {
    aString = mStrings[0];
    return;
  }

  if (IsArray()) {
    
    
    
    
    uint32_t len = mStrings.Length();
    for (uint32_t i = 0; i < len; ++i) {
      aString.Append(',');
      aString.Append(mStrings[i]);
    }

    return;
  }

  NS_NOTREACHED("What?");
}


KeyPath
KeyPath::DeserializeFromString(const nsAString& aString)
{
  KeyPath keyPath(0);

  if (!aString.IsEmpty() && aString.First() == ',') {
    keyPath.SetType(ARRAY);

    
    
    
    nsCharSeparatedTokenizerTemplate<IgnoreWhitespace> tokenizer(aString, ',');
    tokenizer.nextToken();
    while (tokenizer.hasMoreTokens()) {
      keyPath.mStrings.AppendElement(tokenizer.nextToken());
    }

    return keyPath;
  }

  keyPath.SetType(STRING);
  keyPath.mStrings.AppendElement(aString);

  return keyPath;
}

nsresult
KeyPath::ToJSVal(JSContext* aCx, JS::MutableHandle<JS::Value> aValue) const
{
  if (IsArray()) {
    uint32_t len = mStrings.Length();
    JS::Rooted<JSObject*> array(aCx, JS_NewArrayObject(aCx, len));
    if (!array) {
      IDB_WARNING("Failed to make array!");
      return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
    }

    for (uint32_t i = 0; i < len; ++i) {
      JS::Rooted<JS::Value> val(aCx);
      nsString tmp(mStrings[i]);
      if (!xpc::StringToJsval(aCx, tmp, &val)) {
        IDB_REPORT_INTERNAL_ERR();
        return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
      }

      if (!JS_SetElement(aCx, array, i, val)) {
        IDB_REPORT_INTERNAL_ERR();
        return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
      }
    }

    aValue.setObject(*array);
    return NS_OK;
  }

  if (IsString()) {
    nsString tmp(mStrings[0]);
    if (!xpc::StringToJsval(aCx, tmp, aValue)) {
      IDB_REPORT_INTERNAL_ERR();
      return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
    }
    return NS_OK;
  }

  aValue.setNull();
  return NS_OK;
}

nsresult
KeyPath::ToJSVal(JSContext* aCx, JS::Heap<JS::Value>& aValue) const
{
  JS::Rooted<JS::Value> value(aCx);
  nsresult rv = ToJSVal(aCx, &value);
  if (NS_SUCCEEDED(rv)) {
    aValue = value;
  }
  return rv;
}

bool
KeyPath::IsAllowedForObjectStore(bool aAutoIncrement) const
{
  
  
  if (!aAutoIncrement) {
    return true;
  }

  
  if (IsArray()) {
    return false;
  }

  
  if (IsEmpty()) {
    return false;
  }

  
  return true;
}
