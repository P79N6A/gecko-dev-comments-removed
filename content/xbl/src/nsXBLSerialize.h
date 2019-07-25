




































#ifndef nsXBLSerialize_h__
#define nsXBLSerialize_h__

#include "jsapi.h"

#include "nsIScriptContext.h"
#include "nsIObjectInputStream.h"
#include "nsIObjectOutputStream.h"
#include "nsINameSpaceManager.h"

typedef PRUint8 XBLBindingSerializeDetails;



#define XBLBinding_Serialize_Version 0x00000001


#define XBLBinding_Serialize_IsFirstBinding 1


#define XBLBinding_Serialize_InheritStyle 2



#define XBLBinding_Serialize_NoMoreBindings 0x80





#define XBLBinding_Serialize_NoMoreItems 0 // appears at the end of the members list
#define XBLBinding_Serialize_Field 1
#define XBLBinding_Serialize_GetterProperty 2
#define XBLBinding_Serialize_SetterProperty 3
#define XBLBinding_Serialize_GetterSetterProperty 4
#define XBLBinding_Serialize_Method 5
#define XBLBinding_Serialize_Constructor 6
#define XBLBinding_Serialize_Destructor 7
#define XBLBinding_Serialize_Handler 8
#define XBLBinding_Serialize_Image 9
#define XBLBinding_Serialize_Stylesheet 10
#define XBLBinding_Serialize_Mask 0x0F
#define XBLBinding_Serialize_ReadOnly 0x80



#define XBLBinding_Serialize_NoMoreInsertionPoints 0xFFFFFFFF







#define XBLBinding_Serialize_CustomNamespace 0xFE


#define XBLBinding_Serialize_TextNode 0xFB
#define XBLBinding_Serialize_CDATANode 0xFC
#define XBLBinding_Serialize_CommentNode 0xFD


#define XBLBinding_Serialize_NoContent 0xFF



#define XBLBinding_Serialize_NoMoreAttributes 0xFF

PR_STATIC_ASSERT(XBLBinding_Serialize_CustomNamespace >= kNameSpaceID_LastBuiltin);

nsresult
XBL_SerializeFunction(nsIScriptContext* aContext,
                      nsIObjectOutputStream* aStream,
                      JSObject* aFunctionObject);

nsresult
XBL_DeserializeFunction(nsIScriptContext* aContext,
                        nsIObjectInputStream* aStream,
                        JSObject** aFunctionObject);

#endif 
