









































#ifndef xpcforwards_h___
#define xpcforwards_h___



class nsXPConnect;
class XPCJSRuntime;
class XPCContext;
class XPCCallContext;

class XPCPerThreadData;
class XPCJSThrower;
class XPCJSStack;

class nsXPCWrappedJS;
class nsXPCWrappedJSClass;

class XPCNativeMember;
class XPCNativeInterface;
class XPCNativeSet;

class XPCWrappedNative;
class XPCWrappedNativeProto;
class XPCWrappedNativeTearOff;
class XPCNativeScriptableShared;
class XPCNativeScriptableInfo;
class XPCNativeScriptableCreateInfo;

class JSObject2WrappedJSMap;
class Native2WrappedNativeMap;
class IID2WrappedJSClassMap;
class JSContext2XPCContextMap;
class IID2NativeInterfaceMap;
class ClassInfo2NativeSetMap;
class ClassInfo2WrappedNativeProtoMap;
class NativeSetMap;
class IID2ThisTranslatorMap;
class XPCNativeScriptableSharedMap;
class XPCWrappedNativeProtoMap;
class XPCNativeWrapperMap;

class nsXPCComponents;
class nsXPCComponents_Interfaces;
class nsXPCComponents_InterfacesByID;
class nsXPCComponents_Classes;
class nsXPCComponents_ClassesByID;
class nsXPCComponents_Results;
class nsXPCComponents_ID;
class nsXPCComponents_Exception;
class nsXPCComponents_Constructor;
class nsXPCComponents_Utils;
class nsXPCConstructor;

class AutoMarkingPtr;

class xpcProperty;
class xpcPropertyBagEnumerator;

#ifdef XPC_IDISPATCH_SUPPORT
class XPCDispInterface;
struct IDispatch;
class XPCDispParams;
class XPCDispJSPropertyInfo;
class nsIXPConnectWrappedJS;
class XPCIDispatchExtension;
#endif


#endif 
