






#include "frontend/ParseNode.h"
#include "frontend/TreeContext.h"

#include "jsatominlines.h"

#include "frontend/TreeContext-inl.h"
#include "vm/ScopeObject-inl.h"
#include "vm/String-inl.h"

using namespace js;
using namespace js::frontend;

void
TreeContext::trace(JSTracer *trc)
{
    sc->bindings.trace(trc);
}

bool
frontend::GenerateBlockId(TreeContext *tc, uint32_t &blockid)
{
    if (tc->blockidGen == JS_BIT(20)) {
        JS_ReportErrorNumber(tc->sc->context, js_GetErrorMessage, NULL, JSMSG_NEED_DIET, "program");
        return false;
    }
    JS_ASSERT(tc->blockidGen < JS_BIT(20));
    blockid = tc->blockidGen++;
    return true;
}

