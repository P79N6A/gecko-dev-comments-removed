






#include "frontend/SemanticAnalysis.h"

#include "jsfun.h"

#include "frontend/Parser.h"
#include "frontend/TreeContext.h"

#include "jsobjinlines.h"
#include "jsfuninlines.h"

using namespace js;
using namespace js::frontend;

static void
FlagHeavyweights(Definition *dn, FunctionBox *funbox, bool *isHeavyweight, bool topInFunction)
{
    unsigned dnLevel = dn->frameLevel();

    while ((funbox = funbox->parent) != NULL) {
        





        if (funbox->level + 1U == dnLevel || (dnLevel == 0 && dn->isLet())) {
            funbox->setFunIsHeavyweight();
            break;
        }
    }

    if (!funbox && topInFunction)
        *isHeavyweight = true;
}

static void
SetFunctionKinds(FunctionBox *funbox, bool *isHeavyweight, bool topInFunction, bool isDirectEval)
{
    for (; funbox; funbox = funbox->siblings) {
        ParseNode *fn = funbox->node;
        if (!fn)
            continue;

        ParseNode *pn = fn->pn_body;
        if (!pn)
            continue;

        if (funbox->kids)
            SetFunctionKinds(funbox->kids, isHeavyweight, topInFunction, isDirectEval);

        JSFunction *fun = funbox->function();

        JS_ASSERT(fun->kind() == JSFUN_INTERPRETED);

        if (funbox->funIsHeavyweight()) {
            
        } else if (isDirectEval || funbox->inAnyDynamicScope()) {
            









            JS_ASSERT(!fun->isNullClosure());
        } else {
            bool hasUpvars = false;

            if (pn->isKind(PNK_UPVARS)) {
                AtomDefnMapPtr upvars = pn->pn_names;
                JS_ASSERT(!upvars->empty());

                
                for (AtomDefnRange r = upvars->all(); !r.empty(); r.popFront()) {
                    if (!r.front().value()->resolve()->isFreeVar()) {
                        hasUpvars = true;
                        break;
                    }
                }
            }

            if (!hasUpvars) {
                
                fun->setKind(JSFUN_NULL_CLOSURE);
            }
        }

        if (fun->kind() == JSFUN_INTERPRETED && pn->isKind(PNK_UPVARS)) {
            







            AtomDefnMapPtr upvars = pn->pn_names;
            JS_ASSERT(!upvars->empty());

            for (AtomDefnRange r = upvars->all(); !r.empty(); r.popFront()) {
                Definition *defn = r.front().value();
                Definition *lexdep = defn->resolve();
                if (!lexdep->isFreeVar())
                    FlagHeavyweights(lexdep, funbox, isHeavyweight, topInFunction);
            }
        }
    }
}












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
    SharedContext *sc = tc->sc;
    if (!tc->functionList)
        return true;
    if (!MarkExtensibleScopeDescendants(sc->context, tc->functionList, false))
        return false;
    bool isDirectEval = !!parser->callerFrame;
    bool isHeavyweight = false;
    SetFunctionKinds(tc->functionList, &isHeavyweight, sc->inFunction(), isDirectEval);
    if (isHeavyweight)
        sc->setFunIsHeavyweight();
    return true;
}
