





#ifndef ds_IdValuePair_h
#define ds_IdValuePair_h

#include "jsapi.h"

#include "NamespaceImports.h"
#include "js/Id.h"

namespace js {

struct IdValuePair
{
    jsid id;
    Value value;

    IdValuePair()
      : id(JSID_EMPTY), value(UndefinedValue())
    {}
    explicit IdValuePair(jsid idArg)
      : id(idArg), value(UndefinedValue())
    {}
    IdValuePair(jsid idArg, Value valueArg)
      : id(idArg), value(valueArg)
    {}
};

class MOZ_STACK_CLASS AutoIdValueVector : public AutoVectorRooter<IdValuePair>
{
  public:
    explicit AutoIdValueVector(ContextFriendFields* cx
                               MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
        : AutoVectorRooter<IdValuePair>(cx, IDVALVECTOR)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

} 

#endif 
