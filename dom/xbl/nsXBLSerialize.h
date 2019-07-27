




#ifndef nsXBLSerialize_h__
#define nsXBLSerialize_h__

#include "nsIObjectInputStream.h"
#include "nsIObjectOutputStream.h"
#include "mozilla/dom/NameSpaceConstants.h"
#include "js/TypeDecls.h"

typedef uint8_t XBLBindingSerializeDetails;



#define XBLBinding_Serialize_Version 0x00000004


#define XBLBinding_Serialize_IsFirstBinding 1


#define XBLBinding_Serialize_InheritStyle 2


#define XBLBinding_Serialize_ChromeOnlyContent 4


#define XBLBinding_Serialize_BindToUntrustedContent 8



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
#define XBLBinding_Serialize_Attribute 0xA
#define XBLBinding_Serialize_Mask 0x0F
#define XBLBinding_Serialize_ReadOnly 0x80



#define XBLBinding_Serialize_NoMoreInsertionPoints 0xFFFFFFFF







#define XBLBinding_Serialize_CustomNamespace 0xFE


#define XBLBinding_Serialize_TextNode 0xFB
#define XBLBinding_Serialize_CDATANode 0xFC
#define XBLBinding_Serialize_CommentNode 0xFD


#define XBLBinding_Serialize_NoContent 0xFF



#define XBLBinding_Serialize_NoMoreAttributes 0xFF

static_assert(XBLBinding_Serialize_CustomNamespace >= kNameSpaceID_LastBuiltin,
              "The custom namespace should not be in use as a real namespace");

nsresult
XBL_SerializeFunction(nsIObjectOutputStream* aStream,
                      JS::Handle<JSObject*> aFunctionObject);

nsresult
XBL_DeserializeFunction(nsIObjectInputStream* aStream,
                        JS::MutableHandle<JSObject*> aFunctionObject);

#endif 
