




































#ifndef nsXBLSerialize_h__
#define nsXBLSerialize_h__

#include "jsapi.h"

#include "nsIScriptContext.h"
#include "nsIObjectInputStream.h"
#include "nsIObjectOutputStream.h"

nsresult
XBL_SerializeFunction(nsIScriptContext* aContext,
                      nsIObjectOutputStream* aStream,
                      JSObject* aFunctionObject);

nsresult
XBL_DeserializeFunction(nsIScriptContext* aContext,
                        nsIObjectInputStream* aStream,
                        void* aHolder,
                        PRUint32* aLineNumber,
                        void **aScriptObject);

#endif 
