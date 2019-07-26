








#ifndef NamespaceImports_h
#define NamespaceImports_h



#include "js/CallNonGenericMethod.h"
#include "js/TypeDecls.h"
#include "js/Value.h"



namespace JS {

class Latin1CharsZ;
class ConstTwoByteChars;
class TwoByteChars;

class AutoFunctionVector;
class AutoIdVector;
class AutoObjectVector;
class AutoScriptVector;
class AutoValueVector;

class AutoIdArray;

class AutoGCRooter;
template <typename T> class AutoVectorRooter;
template<typename K, typename V> class AutoHashMapRooter;
template<typename T> class AutoHashSetRooter;

class SourceBufferHolder;

class HandleValueArray;

class JS_PUBLIC_API(AutoCheckCannotGC);

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

using JS::Latin1Char;
using JS::Latin1CharsZ;
using JS::ConstTwoByteChars;
using JS::TwoByteChars;

using JS::AutoFunctionVector;
using JS::AutoIdVector;
using JS::AutoObjectVector;
using JS::AutoScriptVector;
using JS::AutoValueVector;

using JS::AutoIdArray;

using JS::AutoGCRooter;
using JS::AutoHashMapRooter;
using JS::AutoHashSetRooter;
using JS::AutoVectorRooter;

using JS::CallArgs;
using JS::CallNonGenericMethod;
using JS::CallReceiver;
using JS::CompileOptions;
using JS::IsAcceptableThis;
using JS::NativeImpl;
using JS::OwningCompileOptions;
using JS::ReadOnlyCompileOptions;
using JS::SourceBufferHolder;

using JS::Rooted;
using JS::RootedFunction;
using JS::RootedId;
using JS::RootedObject;
using JS::RootedScript;
using JS::RootedString;
using JS::RootedValue;

using JS::PersistentRooted;
using JS::PersistentRootedFunction;
using JS::PersistentRootedId;
using JS::PersistentRootedObject;
using JS::PersistentRootedScript;
using JS::PersistentRootedString;
using JS::PersistentRootedValue;

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

using JS::NullHandleValue;
using JS::UndefinedHandleValue;

using JS::HandleValueArray;

using JS::Zone;

using JS::AutoCheckCannotGC;

} 

#endif 
