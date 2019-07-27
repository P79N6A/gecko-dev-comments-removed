





#ifndef gc_Rooting_h
#define gc_Rooting_h

#include "js/RootingAPI.h"

class JSAtom;
class JSLinearString;

namespace js {

class PropertyName;
class ScriptSourceObject;
class Shape;

namespace types { struct TypeObject; }



typedef JS::Handle<Shape*>             HandleShape;
typedef JS::Handle<types::TypeObject*> HandleTypeObject;
typedef JS::Handle<JSAtom*>            HandleAtom;
typedef JS::Handle<JSLinearString*>    HandleLinearString;
typedef JS::Handle<PropertyName*>      HandlePropertyName;
typedef JS::Handle<ScriptSourceObject*> HandleScriptSource;

typedef JS::MutableHandle<Shape*>      MutableHandleShape;
typedef JS::MutableHandle<JSAtom*>     MutableHandleAtom;

typedef JS::Rooted<Shape*>             RootedShape;
typedef JS::Rooted<types::TypeObject*> RootedTypeObject;
typedef JS::Rooted<JSAtom*>            RootedAtom;
typedef JS::Rooted<JSLinearString*>    RootedLinearString;
typedef JS::Rooted<PropertyName*>      RootedPropertyName;
typedef JS::Rooted<ScriptSourceObject*> RootedScriptSource;

} 

#endif 
