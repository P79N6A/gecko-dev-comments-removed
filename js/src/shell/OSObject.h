







#ifndef shell_OSObject_h
#define shell_OSObject_h

#include "jsapi.h"

namespace js {
namespace shell {


bool
DefineOS(JSContext* cx, JS::HandleObject global);

enum PathResolutionMode {
    RootRelative,
    ScriptRelative
};

JSString*
ResolvePath(JSContext* cx, JS::HandleString filenameStr, PathResolutionMode resolveMode);

}
}

#endif 
