





#include "KeyPath.h"
#include "IDBObjectStore.h"
#include "Key.h"

#include "nsCharSeparatedTokenizer.h"
#include "nsJSUtils.h"
#include "xpcpublic.h"

USING_INDEXEDDB_NAMESPACE

namespace {

inline
bool
IgnoreWhitespace(PRUnichar c)
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

    jsval stringVal;
    if (!xpc::StringToJsval(aCx, token, &stringVal)) {
      return false;
    }

    NS_ASSERTION(JSVAL_IS_STRING(stringVal), "This should never happen");
    JSString* str = JSVAL_TO_STRING(stringVal);

    JSBool isIdentifier = JS_FALSE;
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
  JSObject* targetObject = nullptr;
  JSObject* obj = JSVAL_IS_PRIMITIVE(aValue) ? nullptr : 
                                               JSVAL_TO_OBJECT(aValue);

  while (tokenizer.hasMoreTokens()) {
    const nsDependentSubstring& token = tokenizer.nextToken();

    NS_ASSERTION(!token.IsEmpty(), "Should be a valid keypath");

    const jschar* keyPathChars = token.BeginReading();
    const size_t keyPathLen = token.Length();

    JSBool hasProp;
    if (!targetObject) {
      
      if (!obj) {
        return NS_ERROR_DOM_INDEXEDDB_DATA_ERR;
      }

      JSBool ok = JS_HasUCProperty(aCx, obj, keyPathChars, keyPathLen,
                                   &hasProp);
      NS_ENSURE_TRUE(ok, NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);

      if (hasProp) {
        
        jsval intermediate;
        JSBool ok = JS_GetUCProperty(aCx, obj, keyPathChars, keyPathLen,
                                     &intermediate);
        NS_ENSURE_TRUE(ok, NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);

        
        if (intermediate == JSVAL_VOID) {
          return NS_ERROR_DOM_INDEXEDDB_DATA_ERR;
        }
        if (tokenizer.hasMoreTokens()) {
          
          if (JSVAL_IS_PRIMITIVE(intermediate)) {
            return NS_ERROR_DOM_INDEXEDDB_DATA_ERR;
          }
          obj = JSVAL_TO_OBJECT(intermediate);
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
        
        
        JSObject* dummy = JS_NewObject(aCx, nullptr, nullptr, nullptr);
        if (!dummy) {
          rv = NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
          break;
        }

        if (!JS_DefineUCProperty(aCx, obj, token.BeginReading(),
                                 token.Length(),
                                 OBJECT_TO_JSVAL(dummy), nullptr, nullptr,
                                 JSPROP_ENUMERATE)) {
          rv = NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
          break;
        }

        obj = dummy;
      }
      else {
        JSObject* dummy = JS_NewObject(aCx, &IDBObjectStore::sDummyPropJSClass,
                                       nullptr, nullptr);
        if (!dummy) {
          rv = NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
          break;
        }

        if (!JS_DefineUCProperty(aCx, obj, token.BeginReading(),
                                 token.Length(), OBJECT_TO_JSVAL(dummy),
                                 nullptr, nullptr, JSPROP_ENUMERATE)) {
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
    
    
    jsval succeeded;
    if (!JS_DeleteUCProperty2(aCx, targetObject,
                              targetObjectPropName.get(),
                              targetObjectPropName.Length(), &succeeded)) {
      return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
    }
    NS_ASSERTION(JSVAL_IS_BOOLEAN(succeeded), "Wtf?");
    NS_ENSURE_TRUE(JSVAL_TO_BOOLEAN(succeeded),
                   NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);
  }

  NS_ENSURE_SUCCESS(rv, rv);
  return rv;
}

} 


nsresult
KeyPath::Parse(JSContext* aCx, const JS::Value& aValue, KeyPath* aKeyPath)
{
  KeyPath keyPath(0);

  aKeyPath->SetType(NONEXISTENT);

  
  if (!JSVAL_IS_PRIMITIVE(aValue) &&
      JS_IsArrayObject(aCx, JSVAL_TO_OBJECT(aValue))) {

    JSObject* obj = JSVAL_TO_OBJECT(aValue);

    uint32_t length;
    if (!JS_GetArrayLength(aCx, obj, &length)) {
      return NS_ERROR_FAILURE;
    }

    if (!length) {
      return NS_ERROR_FAILURE;
    }

    keyPath.SetType(ARRAY);

    for (uint32_t index = 0; index < length; index++) {
      jsval val;
      JSString* jsstr;
      nsDependentJSString str;
      if (!JS_GetElement(aCx, obj, index, &val) ||
          !(jsstr = JS_ValueToString(aCx, val)) ||
          !str.init(aCx, jsstr)) {
        return NS_ERROR_FAILURE;
      }

      if (!keyPath.AppendStringWithValidation(aCx, str)) {
        return NS_ERROR_FAILURE;
      }
    }
  }
  
  else if (!JSVAL_IS_NULL(aValue) && !JSVAL_IS_VOID(aValue)) {
    JSString* jsstr;
    nsDependentJSString str;
    if (!(jsstr = JS_ValueToString(aCx, aValue)) ||
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
  JS::Value value;

  aKey.Unset();

  for (uint32_t i = 0; i < len; ++i) {
    nsresult rv = GetJSValFromKeyPathString(aCx, aValue, mStrings[i], &value,
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
  JS::RootedObject arrayObj(aCx, JS_NewArrayObject(aCx, len, nullptr));
  if (!arrayObj) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  JS::Value value;
  for (uint32_t i = 0; i < len; ++i) {
    nsresult rv = GetJSValFromKeyPathString(aCx, aValue, mStrings[i], &value,
                                            DoNotCreateProperties, nullptr,
                                            nullptr);
    if (NS_FAILED(rv)) {
      return rv;
    }

    if (!JS_SetElement(aCx, arrayObj, i, &value)) {
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

  JS::Value value;

  aKey.Unset();

  nsresult rv = GetJSValFromKeyPathString(aCx, aValue, mStrings[0], &value,
                                          CreateProperties, aCallback,
                                          aClosure);
  if (NS_FAILED(rv)) {
    return rv;
  }

  if (NS_FAILED(aKey.AppendItem(aCx, false, value))) {
    NS_ASSERTION(aKey.IsUnset(), "Should be unset");
    return JSVAL_IS_VOID(value) ? NS_OK : NS_ERROR_DOM_INDEXEDDB_DATA_ERR;
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
      aString.Append(NS_LITERAL_STRING(",") + mStrings[i]);
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
KeyPath::ToJSVal(JSContext* aCx, JS::Value* aValue) const
{
  if (IsArray()) {
    uint32_t len = mStrings.Length();
    JSObject* array = JS_NewArrayObject(aCx, len, nullptr);
    if (!array) {
      NS_WARNING("Failed to make array!");
      return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
    }

    for (uint32_t i = 0; i < len; ++i) {
      jsval val;
      nsString tmp(mStrings[i]);
      if (!xpc::StringToJsval(aCx, tmp, &val)) {
        return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
      }

      if (!JS_SetElement(aCx, array, i, &val)) {
        return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
      }
    }

    *aValue = OBJECT_TO_JSVAL(array);
    return NS_OK;
  }

  if (IsString()) {
    nsString tmp(mStrings[0]);
    if (!xpc::StringToJsval(aCx, tmp, aValue)) {
      return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
    }
    return NS_OK;
  }

  *aValue = JSVAL_NULL;
  return NS_OK;
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
