






#include "frontend/SemanticAnalysis.h"

#include "jsfun.h"

#include "frontend/Parser.h"
#include "frontend/SharedContext.h"

#include "jsobjinlines.h"
#include "jsfuninlines.h"

using namespace js;
using namespace js::frontend;












static bool
MarkExtensibleScopeDescendants(JSContext *context, FunctionBox *funbox, bool hasExtensibleParent)
{
    for (; funbox; funbox = funbox->siblings) {
        





        JS_ASSERT(!funbox->bindings.extensibleParents());
        if (hasExtensibleParent) {
            if (!funbox->bindings.setExtensibleParents(context))
                return false;
        }

        if (funbox->kids) {
            if (!MarkExtensibleScopeDescendants(context, funbox->kids,
                                                hasExtensibleParent ||
                                                funbox->funHasExtensibleScope()))
            {
                return false;
            }
        }
    }

    return true;
}

bool
frontend::AnalyzeFunctions(Parser *parser)
{
    TreeContext *tc = parser->tc;
    if (!tc->functionList)
        return true;
    if (!MarkExtensibleScopeDescendants(tc->sc->context, tc->functionList, false))
        return false;
    return true;
}
