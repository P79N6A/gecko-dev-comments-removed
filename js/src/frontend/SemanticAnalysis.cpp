







































#include "frontend/SemanticAnalysis.h"

#include "jsfun.h"

#include "frontend/BytecodeEmitter.h"
#include "frontend/Parser.h"

#include "jsobjinlines.h"
#include "jsfuninlines.h"

using namespace js;
using namespace js::frontend;






























static void
CleanFunctionList(ParseNodeAllocator *allocator, FunctionBox **funboxHead)
{
    FunctionBox **link = funboxHead;
    while (FunctionBox *box = *link) {
        if (!box->node) {
            



            *link = box->siblings;
        } else if (!box->node->pn_funbox) {
            



            *link = box->siblings;
            allocator->freeNode(box->node);
        } else {
            

            
            {
                ParseNode **methodLink = &box->methods;
                while (ParseNode *method = *methodLink) {
                    
                    JS_ASSERT(method->isArity(PN_FUNC));
                    if (!method->pn_funbox) {
                        
                        *methodLink = method->pn_link;
                    } else {
                        
                        methodLink = &method->pn_link;
                    }
                }
            }

            
            CleanFunctionList(allocator, &box->kids);

            
            link = &box->siblings;
        }
    }
}































static unsigned
FindFunArgs(FunctionBox *funbox, int level, FunctionBoxQueue *queue)
{
    unsigned allskipmin = UpvarCookie::FREE_LEVEL;

    do {
        ParseNode *fn = funbox->node;
        JS_ASSERT(fn->isArity(PN_FUNC));
        int fnlevel = level;

        













        if (funbox->tcflags & (TCF_FUN_HEAVYWEIGHT | TCF_FUN_IS_GENERATOR)) {
            fn->setFunArg();
            for (FunctionBox *kid = funbox->kids; kid; kid = kid->siblings)
                kid->node->setFunArg();
        }

        




        unsigned skipmin = UpvarCookie::FREE_LEVEL;
        ParseNode *pn = fn->pn_body;

        if (pn->isKind(PNK_UPVARS)) {
            AtomDefnMapPtr &upvars = pn->pn_names;
            JS_ASSERT(upvars->count() != 0);

            for (AtomDefnRange r = upvars->all(); !r.empty(); r.popFront()) {
                Definition *defn = r.front().value();
                Definition *lexdep = defn->resolve();

                if (!lexdep->isFreeVar()) {
                    unsigned upvarLevel = lexdep->frameLevel();

                    if (int(upvarLevel) <= fnlevel)
                        fn->setFunArg();

                    unsigned skip = (funbox->level + 1) - upvarLevel;
                    if (skip < skipmin)
                        skipmin = skip;
                }
            }
        }

        






        if (fn->isFunArg()) {
            queue->push(funbox);
            fnlevel = int(funbox->level);
        }

        



        if (funbox->kids) {
            unsigned kidskipmin = FindFunArgs(funbox->kids, fnlevel, queue);

            JS_ASSERT(kidskipmin != 0);
            if (kidskipmin != UpvarCookie::FREE_LEVEL) {
                --kidskipmin;
                if (kidskipmin != 0 && kidskipmin < skipmin)
                    skipmin = kidskipmin;
            }
        }

        




        if (skipmin != UpvarCookie::FREE_LEVEL) {
            if (skipmin < allskipmin)
                allskipmin = skipmin;
        }
    } while ((funbox = funbox->siblings) != NULL);

    return allskipmin;
}

static bool
MarkFunArgs(JSContext *cx, FunctionBox *funbox, uint32_t functionCount)
{
    FunctionBoxQueue queue;
    if (!queue.init(functionCount)) {
        js_ReportOutOfMemory(cx);
        return false;
    }

    FindFunArgs(funbox, -1, &queue);
    while ((funbox = queue.pull()) != NULL) {
        ParseNode *fn = funbox->node;
        JS_ASSERT(fn->isFunArg());

        ParseNode *pn = fn->pn_body;
        if (pn->isKind(PNK_UPVARS)) {
            AtomDefnMapPtr upvars = pn->pn_names;
            JS_ASSERT(!upvars->empty());

            for (AtomDefnRange r = upvars->all(); !r.empty(); r.popFront()) {
                Definition *defn = r.front().value();
                Definition *lexdep = defn->resolve();

                if (!lexdep->isFreeVar() &&
                    !lexdep->isFunArg() &&
                    (lexdep->kind() == Definition::FUNCTION ||
                     lexdep->isOp(JSOP_CALLEE))) {
                    








                    lexdep->setFunArg();

                    FunctionBox *afunbox;
                    if (lexdep->isOp(JSOP_CALLEE)) {
                        






                        afunbox = funbox;
                        unsigned calleeLevel = lexdep->pn_cookie.level();
                        unsigned staticLevel = afunbox->level + 1U;
                        while (staticLevel != calleeLevel) {
                            afunbox = afunbox->parent;
                            --staticLevel;
                        }
                        JS_ASSERT(afunbox->level + 1U == calleeLevel);
                        afunbox->node->setFunArg();
                    } else {
                       afunbox = lexdep->pn_funbox;
                    }
                    queue.push(afunbox);

                    




                    if (afunbox->kids)
                        FindFunArgs(afunbox->kids, afunbox->level, &queue);
                }
            }
        }
    }
    return true;
}

static void
FlagHeavyweights(Definition *dn, FunctionBox *funbox, uint32_t *tcflags)
{
    unsigned dnLevel = dn->frameLevel();

    while ((funbox = funbox->parent) != NULL) {
        





        if (funbox->level + 1U == dnLevel || (dnLevel == 0 && dn->isLet())) {
            funbox->tcflags |= TCF_FUN_HEAVYWEIGHT;
            break;
        }
        funbox->tcflags |= TCF_FUN_ENTRAINS_SCOPES;
    }

    if (!funbox && (*tcflags & TCF_IN_FUNCTION))
        *tcflags |= TCF_FUN_HEAVYWEIGHT;
}

static void
SetFunctionKinds(FunctionBox *funbox, uint32_t *tcflags, bool isDirectEval)
{
    for (; funbox; funbox = funbox->siblings) {
        ParseNode *fn = funbox->node;
        ParseNode *pn = fn->pn_body;

        if (funbox->kids)
            SetFunctionKinds(funbox->kids, tcflags, isDirectEval);

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
                    FlagHeavyweights(lexdep, funbox, tcflags);
            }
        }

        if (funbox->joinable())
            fun->setJoinable();
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
frontend::AnalyzeFunctions(TreeContext *tc)
{
    CleanFunctionList(&tc->parser->allocator, &tc->functionList);
    if (!tc->functionList)
        return true;
    if (!MarkFunArgs(tc->parser->context, tc->functionList, tc->parser->functionCount))
        return false;
    if (!MarkExtensibleScopeDescendants(tc->parser->context, tc->functionList, false))
        return false;
    bool isDirectEval = !!tc->parser->callerFrame;
    SetFunctionKinds(tc->functionList, &tc->flags, isDirectEval);
    return true;
}
