






#include "frontend/ParseNode.h"
#include "frontend/ParseContext.h"

#include "jsatominlines.h"

#include "frontend/ParseContext-inl.h"
#include "vm/ScopeObject-inl.h"
#include "vm/String-inl.h"

using namespace js;
using namespace js::frontend;

void
ParseContext::trace(JSTracer *trc)
{
    sc->bindings.trace(trc);
}

bool
frontend::GenerateBlockId(ParseContext *pc, uint32_t &blockid)
{
    if (pc->blockidGen == JS_BIT(20)) {
        JS_ReportErrorNumber(pc->sc->context, js_GetErrorMessage, NULL, JSMSG_NEED_DIET, "program");
        return false;
    }
    blockid = pc->blockidGen++;
    return true;
}

