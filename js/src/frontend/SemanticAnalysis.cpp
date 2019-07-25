







































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































static uintN
FindFunArgs(FunctionBox *funbox, int level, FunctionBoxQueue *queue)
{
    uintN allskipmin = UpvarCookie::FREE_LEVEL;

    do {
        ParseNode *fn = funbox->node;
        JS_ASSERT(fn->isArity(PN_FUNC));
        JSFunction *fun = funbox->function();
        int fnlevel = level;

        













        if (funbox->tcflags & (TCF_FUN_HEAVYWEIGHT | TCF_FUN_IS_GENERATOR)) {
            fn->setFunArg();
            for (FunctionBox *kid = funbox->kids; kid; kid = kid->siblings)
                kid->node->setFunArg();
        }

        




        uintN skipmin = UpvarCookie::FREE_LEVEL;
        ParseNode *pn = fn->pn_body;

        if (pn->isKind(PNK_UPVARS)) {
            AtomDefnMapPtr &upvars = pn->pn_names;
            JS_ASSERT(upvars->count() != 0);

            for (AtomDefnRange r = upvars->all(); !r.empty(); r.popFront()) {
                Definition *defn = r.front().value();
                Definition *lexdep = defn->resolve();

                if (!lexdep->isFreeVar()) {
                    uintN upvarLevel = lexdep->frameLevel();

                    if (int(upvarLevel) <= fnlevel)
                        fn->setFunArg();

                    uintN skip = (funbox->level + 1) - upvarLevel;
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
            uintN kidskipmin = FindFunArgs(funbox->kids, fnlevel, queue);

            JS_ASSERT(kidskipmin != 0);
            if (kidskipmin != UpvarCookie::FREE_LEVEL) {
                --kidskipmin;
                if (kidskipmin != 0 && kidskipmin < skipmin)
                    skipmin = kidskipmin;
            }
        }

        





        if (skipmin != UpvarCookie::FREE_LEVEL) {
            fun->u.i.skipmin = skipmin;
            if (skipmin < allskipmin)
                allskipmin = skipmin;
        }
    } while ((funbox = funbox->siblings) != NULL);

    return allskipmin;
}

static bool
MarkFunArgs(JSContext *cx, FunctionBox *funbox, uint32 functionCount)
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
                        uintN calleeLevel = lexdep->pn_cookie.level();
                        uintN staticLevel = afunbox->level + 1U;
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

static uint32
MinBlockId(ParseNode *fn, uint32 id)
{
    if (fn->pn_blockid < id)
        return false;
    if (fn->isDefn()) {
        for (ParseNode *pn = fn->dn_uses; pn; pn = pn->pn_link) {
            if (pn->pn_blockid < id)
                return false;
        }
    }
    return true;
}

static inline bool
CanFlattenUpvar(Definition *dn, FunctionBox *funbox, uint32 tcflags)
{
    

















    FunctionBox *afunbox = funbox;
    uintN dnLevel = dn->frameLevel();

    JS_ASSERT(dnLevel <= funbox->level);
    while (afunbox->level != dnLevel) {
        afunbox = afunbox->parent;

        







        JS_ASSERT(afunbox);

        





        if (!afunbox || afunbox->node->isFunArg())
            return false;

        




        if (afunbox->tcflags & TCF_FUN_IS_GENERATOR)
            return false;
    }

    





    if (afunbox->inLoop)
        return false;

    





    if ((afunbox->parent ? afunbox->parent->tcflags : tcflags) & TCF_FUN_HEAVYWEIGHT)
        return false;

    






    JSFunction *afun = afunbox->function();
    if (!(afun->flags & JSFUN_LAMBDA)) {
        if (dn->isBindingForm() || dn->pn_pos >= afunbox->node->pn_pos)
            return false;
    }

    if (!dn->isInitialized())
        return false;

    Definition::Kind dnKind = dn->kind();
    if (dnKind != Definition::CONST) {
        if (dn->isAssigned())
            return false;

        










        if (dnKind == Definition::ARG &&
            ((afunbox->parent ? afunbox->parent->tcflags : tcflags) & TCF_FUN_USES_ARGUMENTS)) {
            return false;
        }
    }

    




    if (dnKind != Definition::FUNCTION) {
        














        if (dn->pn_pos.end >= afunbox->node->pn_pos.end)
            return false;
        if (!MinBlockId(afunbox->node, dn->pn_blockid))
            return false;
    }
    return true;
}

static void
FlagHeavyweights(Definition *dn, FunctionBox *funbox, uint32 *tcflags)
{
    uintN dnLevel = dn->frameLevel();

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
ConsiderUnbranding(FunctionBox *funbox)
{
    









    bool returnsExpr = !!(funbox->tcflags & TCF_RETURN_EXPR);
#if JS_HAS_EXPR_CLOSURES
    {
        ParseNode *pn2 = funbox->node->pn_body;
        if (pn2->isKind(PNK_UPVARS))
            pn2 = pn2->pn_tree;
        if (pn2->isKind(PNK_ARGSBODY))
            pn2 = pn2->last();
        if (!pn2->isKind(PNK_LC))
            returnsExpr = true;
    }
#endif
    if (!returnsExpr) {
        uintN methodSets = 0, slowMethodSets = 0;

        for (ParseNode *method = funbox->methods; method; method = method->pn_link) {
            JS_ASSERT(method->isOp(JSOP_LAMBDA) || method->isOp(JSOP_LAMBDA_FC));
            ++methodSets;
            if (!method->pn_funbox->joinable())
                ++slowMethodSets;
        }

        if (funbox->shouldUnbrand(methodSets, slowMethodSets))
            funbox->tcflags |= TCF_FUN_UNBRAND_THIS;
    }
}

static void
SetFunctionKinds(FunctionBox *funbox, uint32 *tcflags, bool isDirectEval)
{
    for (; funbox; funbox = funbox->siblings) {
        ParseNode *fn = funbox->node;
        ParseNode *pn = fn->pn_body;

        if (funbox->kids) {
            SetFunctionKinds(funbox->kids, tcflags, isDirectEval);
            ConsiderUnbranding(funbox);
        }

        JSFunction *fun = funbox->function();

        JS_ASSERT(fun->kind() == JSFUN_INTERPRETED);

        if (funbox->tcflags & TCF_FUN_HEAVYWEIGHT) {
            
        } else if (isDirectEval || funbox->inAnyDynamicScope()) {
            









            JS_ASSERT(!fun->isNullClosure());
        } else {
            bool hasUpvars = false;
            bool canFlatten = true;

            if (pn->isKind(PNK_UPVARS)) {
                AtomDefnMapPtr upvars = pn->pn_names;
                JS_ASSERT(!upvars->empty());

                




                for (AtomDefnRange r = upvars->all(); !r.empty(); r.popFront()) {
                    Definition *defn = r.front().value();
                    Definition *lexdep = defn->resolve();

                    if (!lexdep->isFreeVar()) {
                        hasUpvars = true;
                        if (!CanFlattenUpvar(lexdep, funbox, *tcflags)) {
                            





                            canFlatten = false;
                            break;
                        }
                    }
                }
            }

            if (!hasUpvars) {
                
                fun->setKind(JSFUN_NULL_CLOSURE);
            } else if (canFlatten) {
                fun->setKind(JSFUN_FLAT_CLOSURE);
                switch (fn->getOp()) {
                  case JSOP_DEFFUN:
                    fn->setOp(JSOP_DEFFUN_FC);
                    break;
                  case JSOP_DEFLOCALFUN:
                    fn->setOp(JSOP_DEFLOCALFUN_FC);
                    break;
                  case JSOP_LAMBDA:
                    fn->setOp(JSOP_LAMBDA_FC);
                    break;
                  default:
                    
                    JS_ASSERT(fn->isOp(JSOP_NOP));
                }
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












static void
MarkExtensibleScopeDescendants(FunctionBox *funbox, bool hasExtensibleParent) 
{
    for (; funbox; funbox = funbox->siblings) {
        





        JS_ASSERT(!funbox->bindings.extensibleParents());
        if (hasExtensibleParent)
            funbox->bindings.setExtensibleParents();

        if (funbox->kids) {
            MarkExtensibleScopeDescendants(funbox->kids,
                                           hasExtensibleParent || funbox->scopeIsExtensible());
        }
    }
}

bool
frontend::AnalyzeFunctions(TreeContext *tc)
{
    CleanFunctionList(&tc->parser->allocator, &tc->functionList);
    if (!tc->functionList)
        return true;
    if (!MarkFunArgs(tc->parser->context, tc->functionList, tc->parser->functionCount))
        return false;
    MarkExtensibleScopeDescendants(tc->functionList, false);
    bool isDirectEval = !!tc->parser->callerFrame;
    SetFunctionKinds(tc->functionList, &tc->flags, isDirectEval);
    return true;
}
