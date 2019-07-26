








#ifndef NamespaceImports_h
#define NamespaceImports_h



#include "js/CallNonGenericMethod.h"
#include "js/TypeDecls.h"
#include "js/Value.h"



namespace JS {

class Latin1CharsZ;
class StableCharPtr;
class TwoByteChars;

class AutoFunctionVector;
class AutoIdVector;
class AutoObjectVector;
class AutoScriptVector;
class AutoValueVector;

class AutoIdArray;

class AutoGCRooter;
class AutoArrayRooter;
template <typename T> class AutoVectorRooter;
template<typename K, typename V> class AutoHashMapRooter;
template<typename T> class AutoHashSetRooter;

}


namespace js {

using JS::Value;
using JS::BooleanValue;
using JS::DoubleValue;
using JS::Float32Value;
using JS::Int32Value;
using JS::IsPoisonedValue;
using JS::MagicValue;
using JS::NullValue;
using JS::NumberValue;
using JS::ObjectOrNullValue;
using JS::ObjectValue;
using JS::PrivateUint32Value;
using JS::PrivateValue;
using JS::StringValue;
using JS::UndefinedValue;

using JS::IsPoisonedPtr;

using JS::Latin1CharsZ;
using JS::StableCharPtr;
using JS::TwoByteChars;

using JS::AutoFunctionVector;
using JS::AutoIdVector;
using JS::AutoObjectVector;
using JS::AutoScriptVector;
using JS::AutoValueVector;

using JS::AutoIdArray;

using JS::AutoGCRooter;
using JS::AutoArrayRooter;
using JS::AutoHashMapRooter;
using JS::AutoHashSetRooter;
using JS::AutoVectorRooter;

using JS::CallArgs;
using JS::CallNonGenericMethod;
using JS::CallReceiver;
using JS::CompileOptions;
using JS::IsAcceptableThis;
using JS::NativeImpl;

using JS::Rooted;
using JS::RootedFunction;
using JS::RootedId;
using JS::RootedObject;
using JS::RootedScript;
using JS::RootedString;
using JS::RootedValue;

using JS::Handle;
using JS::HandleFunction;
using JS::HandleId;
using JS::HandleObject;
using JS::HandleScript;
using JS::HandleString;
using JS::HandleValue;

using JS::MutableHandle;
using JS::MutableHandleFunction;
using JS::MutableHandleId;
using JS::MutableHandleObject;
using JS::MutableHandleScript;
using JS::MutableHandleString;
using JS::MutableHandleValue;

using JS::Zone;

} 

#endif 
