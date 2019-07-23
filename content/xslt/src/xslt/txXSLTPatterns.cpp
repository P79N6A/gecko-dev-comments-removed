





































#include "nsReadableUtils.h"
#include "txExecutionState.h"
#include "txXSLTPatterns.h"
#include "txNodeSetContext.h"
#include "txForwardContext.h"
#include "txXMLUtils.h"
#include "txXSLTFunctions.h"
#include "nsWhitespaceTokenizer.h"
#ifndef TX_EXE
#include "nsIContent.h"
#endif






double txUnionPattern::getDefaultPriority()
{
    NS_ERROR("Don't call getDefaultPriority on txUnionPattern");
    return Double::NaN;
}







MBool txUnionPattern::matches(const txXPathNode& aNode, txIMatchContext* aContext)
{
    PRUint32 i, len = mLocPathPatterns.Length();
    for (i = 0; i < len; ++i) {
        if (mLocPathPatterns[i]->matches(aNode, aContext)) {
            return MB_TRUE;
        }
    }
    return MB_FALSE;
}

txPattern::Type
txUnionPattern::getType()
{
  return UNION_PATTERN;
}

TX_IMPL_PATTERN_STUBS_NO_SUB_EXPR(txUnionPattern)
txPattern*
txUnionPattern::getSubPatternAt(PRUint32 aPos)
{
    return mLocPathPatterns.SafeElementAt(aPos);
}

void
txUnionPattern::setSubPatternAt(PRUint32 aPos, txPattern* aPattern)
{
    NS_ASSERTION(aPos < mLocPathPatterns.Length(),
                 "setting bad subexpression index");
    mLocPathPatterns[aPos] = aPattern;
}


#ifdef TX_TO_STRING
void
txUnionPattern::toString(nsAString& aDest)
{
#ifdef DEBUG
    aDest.AppendLiteral("txUnionPattern{");
#endif
    for (PRUint32 i = 0; i < mLocPathPatterns.Length(); ++i) {
        if (i != 0)
            aDest.AppendLiteral(" | ");
        mLocPathPatterns[i]->toString(aDest);
    }
#ifdef DEBUG
    aDest.Append(PRUnichar('}'));
#endif
}
#endif









nsresult txLocPathPattern::addStep(txPattern* aPattern, PRBool isChild)
{
    Step* step = mSteps.AppendElement();
    if (!step)
        return NS_ERROR_OUT_OF_MEMORY;

    step->pattern = aPattern;
    step->isChild = isChild;

    return NS_OK;
}

MBool txLocPathPattern::matches(const txXPathNode& aNode, txIMatchContext* aContext)
{
    NS_ASSERTION(mSteps.Length() > 1, "Internal error");

    











    PRUint32 pos = mSteps.Length();
    Step* step = &mSteps[--pos];
    if (!step->pattern->matches(aNode, aContext))
        return MB_FALSE;

    txXPathTreeWalker walker(aNode);
    PRBool hasParent = walker.moveToParent();

    while (step->isChild) {
        if (!pos)
            return MB_TRUE; 
        step = &mSteps[--pos];
        if (!hasParent || !step->pattern->matches(walker.getCurrentPosition(), aContext))
            return MB_FALSE; 

        hasParent = walker.moveToParent();
    }

    
    txXPathTreeWalker blockWalker(walker);
    PRUint32 blockPos = pos;

    while (pos) {
        if (!hasParent)
            return MB_FALSE; 
                             

        step = &mSteps[--pos];
        if (!step->pattern->matches(walker.getCurrentPosition(), aContext)) {
            
            
            pos = blockPos;
            hasParent = blockWalker.moveToParent();
            walker.moveTo(blockWalker);
        }
        else {
            hasParent = walker.moveToParent();
            if (!step->isChild) {
                
                blockPos = pos;
                blockWalker.moveTo(walker);
            }
        }
    }

    return MB_TRUE;
} 

double txLocPathPattern::getDefaultPriority()
{
    NS_ASSERTION(mSteps.Length() > 1, "Internal error");

    return 0.5;
}

TX_IMPL_PATTERN_STUBS_NO_SUB_EXPR(txLocPathPattern)
txPattern*
txLocPathPattern::getSubPatternAt(PRUint32 aPos)
{
    return aPos < mSteps.Length() ? mSteps[aPos].pattern.get() : nsnull;
}

void
txLocPathPattern::setSubPatternAt(PRUint32 aPos, txPattern* aPattern)
{
    NS_ASSERTION(aPos < mSteps.Length(), "setting bad subexpression index");
    Step* step = &mSteps[aPos];
    step->pattern.forget();
    step->pattern = aPattern;
}

#ifdef TX_TO_STRING
void
txLocPathPattern::toString(nsAString& aDest)
{
#ifdef DEBUG
    aDest.AppendLiteral("txLocPathPattern{");
#endif
    for (PRUint32 i = 0; i < mSteps.Length(); ++i) {
        if (i != 0) {
            if (mSteps[i].isChild)
                aDest.Append(PRUnichar('/'));
            else
                aDest.AppendLiteral("//");
        }
        mSteps[i].pattern->toString(aDest);
    }
#ifdef DEBUG
    aDest.Append(PRUnichar('}'));
#endif
}
#endif







MBool txRootPattern::matches(const txXPathNode& aNode, txIMatchContext* aContext)
{
    return txXPathNodeUtils::isRoot(aNode);
}

double txRootPattern::getDefaultPriority()
{
    return 0.5;
}

TX_IMPL_PATTERN_STUBS_NO_SUB_EXPR(txRootPattern)
TX_IMPL_PATTERN_STUBS_NO_SUB_PATTERN(txRootPattern)

#ifdef TX_TO_STRING
void
txRootPattern::toString(nsAString& aDest)
{
#ifdef DEBUG
    aDest.AppendLiteral("txRootPattern{");
#endif
    if (mSerialize)
        aDest.Append(PRUnichar('/'));
#ifdef DEBUG
    aDest.Append(PRUnichar('}'));
#endif
}
#endif









txIdPattern::txIdPattern(const nsSubstring& aString)
{
    nsWhitespaceTokenizer tokenizer(aString);
    while (tokenizer.hasMoreTokens()) {
        
        nsCOMPtr<nsIAtom> atom = do_GetAtom(tokenizer.nextToken());
        mIds.AppendObject(atom);
    }
}

MBool txIdPattern::matches(const txXPathNode& aNode, txIMatchContext* aContext)
{
    if (!txXPathNodeUtils::isElement(aNode)) {
        return PR_FALSE;
    }

    
#ifdef TX_EXE
    Element* elem;
    nsresult rv = txXPathNativeNode::getElement(aNode, &elem);
    NS_ASSERTION(NS_SUCCEEDED(rv), "So why claim it's an element above?");

    nsAutoString value;
    if (!elem->getIDValue(value)) {
        return PR_FALSE;
    }
    nsCOMPtr<nsIAtom> id = do_GetAtom(value);
#else
    nsIContent* content = txXPathNativeNode::getContent(aNode);
    NS_ASSERTION(content, "a Element without nsIContent");

    nsIAtom* id = content->GetID();
#endif 
    return id && mIds.IndexOf(id) > -1;
}

double txIdPattern::getDefaultPriority()
{
    return 0.5;
}

TX_IMPL_PATTERN_STUBS_NO_SUB_EXPR(txIdPattern)
TX_IMPL_PATTERN_STUBS_NO_SUB_PATTERN(txIdPattern)

#ifdef TX_TO_STRING
void
txIdPattern::toString(nsAString& aDest)
{
#ifdef DEBUG
    aDest.AppendLiteral("txIdPattern{");
#endif
    aDest.AppendLiteral("id('");
    PRUint32 k, count = mIds.Count() - 1;
    for (k = 0; k < count; ++k) {
        nsAutoString str;
        mIds[k]->ToString(str);
        aDest.Append(str);
        aDest.Append(PRUnichar(' '));
    }
    nsAutoString str;
    mIds[count]->ToString(str);
    aDest.Append(str);
    aDest.Append(NS_LITERAL_STRING("')"));
#ifdef DEBUG
    aDest.Append(PRUnichar('}'));
#endif
}
#endif










MBool txKeyPattern::matches(const txXPathNode& aNode, txIMatchContext* aContext)
{
    txExecutionState* es = (txExecutionState*)aContext->getPrivateContext();
    nsAutoPtr<txXPathNode> contextDoc(txXPathNodeUtils::getOwnerDocument(aNode));
    NS_ENSURE_TRUE(contextDoc, PR_FALSE);

    nsRefPtr<txNodeSet> nodes;
    nsresult rv = es->getKeyNodes(mName, *contextDoc, mValue, PR_TRUE,
                                  getter_AddRefs(nodes));
    NS_ENSURE_SUCCESS(rv, PR_FALSE);

    return nodes->contains(aNode);
}

double txKeyPattern::getDefaultPriority()
{
    return 0.5;
}

TX_IMPL_PATTERN_STUBS_NO_SUB_EXPR(txKeyPattern)
TX_IMPL_PATTERN_STUBS_NO_SUB_PATTERN(txKeyPattern)

#ifdef TX_TO_STRING
void
txKeyPattern::toString(nsAString& aDest)
{
#ifdef DEBUG
    aDest.AppendLiteral("txKeyPattern{");
#endif
    aDest.AppendLiteral("key('");
    nsAutoString tmp;
    if (mPrefix) {
        mPrefix->ToString(tmp);
        aDest.Append(tmp);
        aDest.Append(PRUnichar(':'));
    }
    mName.mLocalName->ToString(tmp);
    aDest.Append(tmp);
    aDest.AppendLiteral(", ");
    aDest.Append(mValue);
    aDest.Append(NS_LITERAL_STRING("')"));
#ifdef DEBUG
    aDest.Append(PRUnichar('}'));
#endif
}
#endif







MBool txStepPattern::matches(const txXPathNode& aNode, txIMatchContext* aContext)
{
    NS_ASSERTION(mNodeTest, "Internal error");

    if (!mNodeTest->matches(aNode, aContext))
        return MB_FALSE;

    txXPathTreeWalker walker(aNode);
    if ((!mIsAttr &&
         txXPathNodeUtils::isAttribute(walker.getCurrentPosition())) ||
        !walker.moveToParent()) {
        return MB_FALSE;
    }
    if (isEmpty()) {
        return MB_TRUE;
    }

    

















    
    nsRefPtr<txNodeSet> nodes;
    nsresult rv = aContext->recycler()->getNodeSet(getter_AddRefs(nodes));
    NS_ENSURE_SUCCESS(rv, MB_FALSE);

    PRBool hasNext = mIsAttr ? walker.moveToFirstAttribute() :
                               walker.moveToFirstChild();
    while (hasNext) {
        if (mNodeTest->matches(walker.getCurrentPosition(), aContext)) {
            nodes->append(walker.getCurrentPosition());
        }
        hasNext = mIsAttr ? walker.moveToNextAttribute() :
                            walker.moveToNextSibling();
    }

    Expr* predicate = mPredicates[0];
    nsRefPtr<txNodeSet> newNodes;
    rv = aContext->recycler()->getNodeSet(getter_AddRefs(newNodes));
    NS_ENSURE_SUCCESS(rv, MB_FALSE);

    PRUint32 i, predLen = mPredicates.Length();
    for (i = 1; i < predLen; ++i) {
        newNodes->clear();
        MBool contextIsInPredicate = MB_FALSE;
        txNodeSetContext predContext(nodes, aContext);
        while (predContext.hasNext()) {
            predContext.next();
            nsRefPtr<txAExprResult> exprResult;
            rv = predicate->evaluate(&predContext, getter_AddRefs(exprResult));
            NS_ENSURE_SUCCESS(rv, PR_FALSE);

            switch(exprResult->getResultType()) {
                case txAExprResult::NUMBER:
                    
                    if ((double)predContext.position() ==
                        exprResult->numberValue()) {
                        const txXPathNode& tmp = predContext.getContextNode();
                        if (tmp == aNode)
                            contextIsInPredicate = MB_TRUE;
                        newNodes->append(tmp);
                    }
                    break;
                default:
                    if (exprResult->booleanValue()) {
                        const txXPathNode& tmp = predContext.getContextNode();
                        if (tmp == aNode)
                            contextIsInPredicate = MB_TRUE;
                        newNodes->append(tmp);
                    }
                    break;
            }
        }
        
        nodes->clear();
        nodes->append(*newNodes);
        if (!contextIsInPredicate) {
            return MB_FALSE;
        }
        predicate = mPredicates[i];
    }
    txForwardContext evalContext(aContext, aNode, nodes);
    nsRefPtr<txAExprResult> exprResult;
    rv = predicate->evaluate(&evalContext, getter_AddRefs(exprResult));
    NS_ENSURE_SUCCESS(rv, PR_FALSE);

    if (exprResult->getResultType() == txAExprResult::NUMBER)
        
        return ((double)evalContext.position() == exprResult->numberValue());

    return exprResult->booleanValue();
} 

double txStepPattern::getDefaultPriority()
{
    if (isEmpty())
        return mNodeTest->getDefaultPriority();
    return 0.5;
}

txPattern::Type
txStepPattern::getType()
{
  return STEP_PATTERN;
}

TX_IMPL_PATTERN_STUBS_NO_SUB_PATTERN(txStepPattern)
Expr*
txStepPattern::getSubExprAt(PRUint32 aPos)
{
    return PredicateList::getSubExprAt(aPos);
}

void
txStepPattern::setSubExprAt(PRUint32 aPos, Expr* aExpr)
{
    PredicateList::setSubExprAt(aPos, aExpr);
}

#ifdef TX_TO_STRING
void
txStepPattern::toString(nsAString& aDest)
{
#ifdef DEBUG
    aDest.AppendLiteral("txStepPattern{");
#endif
    if (mIsAttr)
        aDest.Append(PRUnichar('@'));
    if (mNodeTest)
        mNodeTest->toString(aDest);

    PredicateList::toString(aDest);
#ifdef DEBUG
    aDest.Append(PRUnichar('}'));
#endif
}
#endif
