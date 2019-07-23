






































#ifndef jspropertytree_h___
#define jspropertytree_h___

#include "jsarena.h"
#include "jsdhash.h"
#include "jsprvtd.h"

struct JSScope;

namespace js {

JSDHashOperator RemoveNodeIfDead(JSDHashTable *table, JSDHashEntryHdr *hdr,
                                 uint32 number, void *arg);

void SweepScopeProperties(JSContext *cx);

class PropertyTree
{
    friend struct ::JSScope;
    friend void js::SweepScopeProperties(JSContext *cx);

    JSDHashTable        hash;
    JSScopeProperty     *freeList;
    JSArenaPool         arenaPool;
    uint32              emptyShapeChanges;

    bool insertChild(JSContext *cx, JSScopeProperty *parent, JSScopeProperty *child);
    void removeChild(JSContext *cx, JSScopeProperty *child);
    void emptyShapeChange(uint32 oldEmptyShape, uint32 newEmptyShape);

  public:
    bool init();
    void finish();

    JSScopeProperty *newScopeProperty(JSContext *cx, bool gcLocked = false);

    JSScopeProperty *getChild(JSContext *cx, JSScopeProperty *parent, uint32 shape,
                              const JSScopeProperty &child);
};

} 

#endif 
