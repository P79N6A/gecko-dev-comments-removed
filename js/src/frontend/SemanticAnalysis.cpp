







































#include "frontend/SemanticAnalysis.h"

#include "jsfun.h"

#include "frontend/Parser.h"
#include "frontend/TreeContext.h"

#include "jsobjinlines.h"
#include "jsfuninlines.h"

using namespace js;
using namespace js::frontend;

static void
FlagHeavyweights(Definition *dn, FunctionBox *funbox, uint32_t *tcflags, bool topInFunction)
{
    unsigned dnLevel = dn->frameLevel();

    while ((funbox = funbox->parent) != NULL) {
        





        if (funbox->level + 1U == dnLevel || (dnLevel == 0 && dn->isLet())) {
            funbox->tcflags |= TCF_FUN_HEAVYWEIGHT;
            break;
        }
    }

    if (!funbox && topInFunction)
        *tcflags |= TCF_FUN_HEAVYWEIGHT;
}

static void
SetFunctionKinds(FunctionBox *funbox, uint32_t *tcflags, bool topInFunction, bool isDirectEval)
{
    for (; funbox; funbox = funbox->siblings) {
        ParseNode *fn = funbox->node;
        if (!fn)
            continue;

        ParseNode *pn = fn->pn_body;
        if (!pn)
            continue;

        if (funbox->kids)
            SetFunctionKinds(funbox->kids, tcflags, topInFunction, isDirectEval);

        JSFunction *fun = funbox->function();

        JS_ASSERT(fun->kind() == JSFUN_INTERPRETED);

        if (funbox->tcflags & TCF_FUN_HEAVYWEIGHT) {
            
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
                    FlagHeavyweights(lexdep, funbox, tcflags, topInFunction);
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
                                                hasExtensibleParent || funbox->scopeIsExtensible())) {
                return false;
            }
        }
    }

    return true;
}

bool
frontend::AnalyzeFunctions(Parser *parser)
{
    SharedContext *sc = parser->tc->sc;
    if (!sc->functionList)
        return true;
    if (!MarkExtensibleScopeDescendants(sc->context, sc->functionList, false))
        return false;
    bool isDirectEval = !!parser->callerFrame;
    SetFunctionKinds(sc->functionList, &sc->flags, sc->inFunction, isDirectEval);
    return true;
}
