











































#include "txExprParser.h"
#include "txExprLexer.h"
#include "txExpr.h"
#include "txStack.h"
#include "txAtoms.h"
#include "txError.h"
#include "txIXPathContext.h"
#include "txStringUtils.h"
#include "txXPathNode.h"
#include "txXPathOptimizer.h"





nsresult
txExprParser::createAVT(const nsSubstring& aAttrValue,
                        txIParseContext* aContext,
                        Expr** aResult)
{
    *aResult = nsnull;
    nsresult rv = NS_OK;
    nsAutoPtr<Expr> expr;
    FunctionCall* concat = nsnull;

    nsAutoString literalString;
    PRBool inExpr = PR_FALSE;
    nsSubstring::const_char_iterator iter, start, end, avtStart;
    aAttrValue.BeginReading(iter);
    aAttrValue.EndReading(end);
    avtStart = iter;

    while (iter != end) {
        
        
        start = iter;
        nsAutoPtr<Expr> newExpr;
        if (!inExpr) {
            
            literalString.Truncate();
            while (iter != end) {
                PRUnichar q = *iter;
                if (q == '{' || q == '}') {
                    
                    
                    literalString.Append(Substring(start, iter));
                    start = ++iter;
                    
                    
                    
                    if (iter == end || *iter != q) {
                        if (q == '}') {
                            aContext->SetErrorOffset(iter - avtStart);
                            return NS_ERROR_XPATH_UNBALANCED_CURLY_BRACE;
                        }

                        inExpr = PR_TRUE;
                        break;
                    }
                    
                    
                }
                ++iter;
            }

            if (start == iter && literalString.IsEmpty()) {
                
                continue;
            }
            newExpr = new txLiteralExpr(literalString +
                                        Substring(start, iter));
            NS_ENSURE_TRUE(newExpr, NS_ERROR_OUT_OF_MEMORY);
        }
        else {
            
            
            while (iter != end) {
                if (*iter == '}') {
                    rv = createExprInternal(Substring(start, iter),
                                            start - avtStart, aContext,
                                            getter_Transfers(newExpr));
                    NS_ENSURE_SUCCESS(rv, rv);

                    inExpr = PR_FALSE;
                    ++iter; 
                    break;
                }
                else if (*iter == '\'' || *iter == '"') {
                    PRUnichar q = *iter;
                    while (++iter != end && *iter != q) {} 
                    if (iter == end) {
                        break;
                    }
                }
                ++iter;
            }

            if (inExpr) {
                aContext->SetErrorOffset(start - avtStart);
                return NS_ERROR_XPATH_UNBALANCED_CURLY_BRACE;
            }
        }
        
        
        if (!expr) {
            expr = newExpr;
        }
        else {
            if (!concat) {
                concat = new txCoreFunctionCall(txCoreFunctionCall::CONCAT);
                NS_ENSURE_TRUE(concat, NS_ERROR_OUT_OF_MEMORY);

                rv = concat->addParam(expr.forget());
                expr = concat;
                NS_ENSURE_SUCCESS(rv, rv);
            }

            rv = concat->addParam(newExpr.forget());
            NS_ENSURE_SUCCESS(rv, rv);
        }
    }

    if (inExpr) {
        aContext->SetErrorOffset(iter - avtStart);
        return NS_ERROR_XPATH_UNBALANCED_CURLY_BRACE;
    }

    if (!expr) {
        expr = new txLiteralExpr(EmptyString());
        NS_ENSURE_TRUE(expr, NS_ERROR_OUT_OF_MEMORY);
    }

    *aResult = expr.forget();

    return NS_OK;
}

nsresult
txExprParser::createExprInternal(const nsSubstring& aExpression,
                                 PRUint32 aSubStringPos,
                                 txIParseContext* aContext, Expr** aExpr)
{
    NS_ENSURE_ARG_POINTER(aExpr);
    *aExpr = nsnull;
    txExprLexer lexer;
    nsresult rv = lexer.parse(aExpression);
    if (NS_FAILED(rv)) {
        nsASingleFragmentString::const_char_iterator start;
        aExpression.BeginReading(start);
        aContext->SetErrorOffset(lexer.mPosition - start + aSubStringPos);
        return rv;
    }
    nsAutoPtr<Expr> expr;
    rv = createExpr(lexer, aContext, getter_Transfers(expr));
    if (NS_SUCCEEDED(rv) && lexer.peek()->mType != Token::END) {
        rv = NS_ERROR_XPATH_BINARY_EXPECTED;
    }
    if (NS_FAILED(rv)) {
        nsASingleFragmentString::const_char_iterator start;
        aExpression.BeginReading(start);
        aContext->SetErrorOffset(lexer.peek()->mStart - start + aSubStringPos);

        return rv;
    }

    txXPathOptimizer optimizer;
    Expr* newExpr = nsnull;
    rv = optimizer.optimize(expr, &newExpr);
    NS_ENSURE_SUCCESS(rv, rv);

    *aExpr = newExpr ? newExpr : expr.forget();

    return NS_OK;
}








nsresult
txExprParser::createBinaryExpr(nsAutoPtr<Expr>& left, nsAutoPtr<Expr>& right,
                               Token* op, Expr** aResult)
{
    NS_ASSERTION(op, "internal error");
    *aResult = nsnull;

    Expr* expr = nsnull;
    switch (op->mType) {
        
        case Token::ADDITION_OP :
            expr = new txNumberExpr(left, right, txNumberExpr::ADD);
            break;
        case Token::SUBTRACTION_OP:
            expr = new txNumberExpr(left, right, txNumberExpr::SUBTRACT);
            break;
        case Token::DIVIDE_OP :
            expr = new txNumberExpr(left, right, txNumberExpr::DIVIDE);
            break;
        case Token::MODULUS_OP :
            expr = new txNumberExpr(left, right, txNumberExpr::MODULUS);
            break;
        case Token::MULTIPLY_OP :
            expr = new txNumberExpr(left, right, txNumberExpr::MULTIPLY);
            break;

        
        case Token::AND_OP:
            expr = new BooleanExpr(left, right, BooleanExpr::AND);
            break;
        case Token::OR_OP:
            expr = new BooleanExpr(left, right, BooleanExpr::OR);
            break;

        
        case Token::EQUAL_OP :
            expr = new RelationalExpr(left, right, RelationalExpr::EQUAL);
            break;
        case Token::NOT_EQUAL_OP :
            expr = new RelationalExpr(left, right, RelationalExpr::NOT_EQUAL);
            break;

        
        case Token::LESS_THAN_OP:
            expr = new RelationalExpr(left, right, RelationalExpr::LESS_THAN);
            break;
        case Token::GREATER_THAN_OP:
            expr = new RelationalExpr(left, right,
                                      RelationalExpr::GREATER_THAN);
            break;
        case Token::LESS_OR_EQUAL_OP:
            expr = new RelationalExpr(left, right,
                                      RelationalExpr::LESS_OR_EQUAL);
            break;
        case Token::GREATER_OR_EQUAL_OP:
            expr = new RelationalExpr(left, right,
                                      RelationalExpr::GREATER_OR_EQUAL);
            break;

        default:
            NS_NOTREACHED("operator tokens should be already checked");
            return NS_ERROR_UNEXPECTED;
    }
    NS_ENSURE_TRUE(expr, NS_ERROR_OUT_OF_MEMORY);

    left.forget();
    right.forget();

    *aResult = expr;
    return NS_OK;
}


nsresult
txExprParser::createExpr(txExprLexer& lexer, txIParseContext* aContext,
                         Expr** aResult)
{
    *aResult = nsnull;

    nsresult rv = NS_OK;
    MBool done = MB_FALSE;

    nsAutoPtr<Expr> expr;

    txStack exprs;
    txStack ops;

    while (!done) {

        MBool unary = MB_FALSE;
        while (lexer.peek()->mType == Token::SUBTRACTION_OP) {
            unary = !unary;
            lexer.nextToken();
        }

        rv = createUnionExpr(lexer, aContext, getter_Transfers(expr));
        if (NS_FAILED(rv)) {
            break;
        }

        if (unary) {
            Expr* unaryExpr = new UnaryExpr(expr);
            if (!unaryExpr) {
                rv = NS_ERROR_OUT_OF_MEMORY;
                break;
            }
            
            expr.forget();
            expr = unaryExpr;
        }

        Token* tok = lexer.nextToken();
        short tokPrecedence = precedence(tok);
        if (tokPrecedence != 0) {
            while (!exprs.isEmpty() && tokPrecedence
                   <= precedence(static_cast<Token*>(ops.peek()))) {
                
                nsAutoPtr<Expr> left(static_cast<Expr*>(exprs.pop()));
                nsAutoPtr<Expr> right(expr);
                rv = createBinaryExpr(left, right,
                                      static_cast<Token*>(ops.pop()),
                                      getter_Transfers(expr));
                if (NS_FAILED(rv)) {
                    done = PR_TRUE;
                    break;
                }
            }
            exprs.push(expr.forget());
            ops.push(tok);
        }
        else {
            lexer.pushBack();
            done = PR_TRUE;
        }
    }

    while (NS_SUCCEEDED(rv) && !exprs.isEmpty()) {
        nsAutoPtr<Expr> left(static_cast<Expr*>(exprs.pop()));
        nsAutoPtr<Expr> right(expr);
        rv = createBinaryExpr(left, right, static_cast<Token*>(ops.pop()),
                              getter_Transfers(expr));
    }
    
    while (!exprs.isEmpty()) {
        delete static_cast<Expr*>(exprs.pop());
    }
    NS_ENSURE_SUCCESS(rv, rv);

    *aResult = expr.forget();
    return NS_OK;
}

nsresult
txExprParser::createFilterOrStep(txExprLexer& lexer, txIParseContext* aContext,
                                 Expr** aResult)
{
    *aResult = nsnull;

    nsresult rv = NS_OK;
    Token* tok = lexer.nextToken();

    nsAutoPtr<Expr> expr;
    switch (tok->mType) {
        case Token::FUNCTION_NAME_AND_PAREN:
            lexer.pushBack();
            rv = createFunctionCall(lexer, aContext, getter_Transfers(expr));
            NS_ENSURE_SUCCESS(rv, rv);
            break;
        case Token::VAR_REFERENCE :
            {
                nsCOMPtr<nsIAtom> prefix, lName;
                PRInt32 nspace;
                nsresult rv = resolveQName(tok->Value(), getter_AddRefs(prefix),
                                           aContext, getter_AddRefs(lName),
                                           nspace);
                NS_ENSURE_SUCCESS(rv, rv);
                expr = new VariableRefExpr(prefix, lName, nspace);
                NS_ENSURE_TRUE(expr, NS_ERROR_OUT_OF_MEMORY);
            }
            break;
        case Token::L_PAREN:
            rv = createExpr(lexer, aContext, getter_Transfers(expr));
            NS_ENSURE_SUCCESS(rv, rv);

            if (lexer.nextToken()->mType != Token::R_PAREN) {
                lexer.pushBack();
                return NS_ERROR_XPATH_PAREN_EXPECTED;
            }
            break;
        case Token::LITERAL :
            expr = new txLiteralExpr(tok->Value());
            NS_ENSURE_TRUE(expr, NS_ERROR_OUT_OF_MEMORY);
            break;
        case Token::NUMBER:
        {
            expr = new txLiteralExpr(Double::toDouble(tok->Value()));
            NS_ENSURE_TRUE(expr, NS_ERROR_OUT_OF_MEMORY);
            break;
        }
        default:
            lexer.pushBack();
            return createLocationStep(lexer, aContext, aResult);
    }

    if (lexer.peek()->mType == Token::L_BRACKET) {
        nsAutoPtr<FilterExpr> filterExpr(new FilterExpr(expr));
        NS_ENSURE_TRUE(filterExpr, NS_ERROR_OUT_OF_MEMORY);

        expr.forget();

        
        rv = parsePredicates(filterExpr, lexer, aContext);
        NS_ENSURE_SUCCESS(rv, rv);
        expr = filterExpr.forget();
    }

    *aResult = expr.forget();
    return NS_OK;
}

nsresult
txExprParser::createFunctionCall(txExprLexer& lexer, txIParseContext* aContext,
                                 Expr** aResult)
{
    *aResult = nsnull;

    nsAutoPtr<FunctionCall> fnCall;

    Token* tok = lexer.nextToken();
    NS_ASSERTION(tok->mType == Token::FUNCTION_NAME_AND_PAREN,
                 "FunctionCall expected");

    
    nsCOMPtr<nsIAtom> prefix, lName;
    PRInt32 namespaceID;
    nsresult rv = resolveQName(tok->Value(), getter_AddRefs(prefix), aContext,
                               getter_AddRefs(lName), namespaceID);
    NS_ENSURE_SUCCESS(rv, rv);

    txCoreFunctionCall::eType type;
    if (namespaceID == kNameSpaceID_None &&
        txCoreFunctionCall::getTypeFromAtom(lName, type)) {
        
        fnCall = new txCoreFunctionCall(type);
        NS_ENSURE_TRUE(fnCall, NS_ERROR_OUT_OF_MEMORY);
    }

    
    if (!fnCall) {
        rv = aContext->resolveFunctionCall(lName, namespaceID,
                                           getter_Transfers(fnCall));

        if (rv == NS_ERROR_NOT_IMPLEMENTED) {
            
            NS_ASSERTION(!fnCall, "Now is it implemented or not?");
            rv = parseParameters(0, lexer, aContext);
            NS_ENSURE_SUCCESS(rv, rv);

            *aResult = new txLiteralExpr(tok->Value() +
                                         NS_LITERAL_STRING(" not implemented."));
            NS_ENSURE_TRUE(*aResult, NS_ERROR_OUT_OF_MEMORY);

            return NS_OK;
        }

        NS_ENSURE_SUCCESS(rv, rv);
    }

    
    rv = parseParameters(fnCall, lexer, aContext);
    NS_ENSURE_SUCCESS(rv, rv);

    *aResult = fnCall.forget();
    return NS_OK;
}

nsresult
txExprParser::createLocationStep(txExprLexer& lexer, txIParseContext* aContext,
                                 Expr** aExpr)
{
    *aExpr = nsnull;

    
    LocationStep::LocationStepType axisIdentifier = LocationStep::CHILD_AXIS;
    nsAutoPtr<txNodeTest> nodeTest;

    
    Token* tok = lexer.peek();
    switch (tok->mType) {
        case Token::AXIS_IDENTIFIER:
        {
            
            lexer.nextToken();
            nsCOMPtr<nsIAtom> axis = do_GetAtom(tok->Value());
            if (axis == txXPathAtoms::ancestor) {
                axisIdentifier = LocationStep::ANCESTOR_AXIS;
            }
            else if (axis == txXPathAtoms::ancestorOrSelf) {
                axisIdentifier = LocationStep::ANCESTOR_OR_SELF_AXIS;
            }
            else if (axis == txXPathAtoms::attribute) {
                axisIdentifier = LocationStep::ATTRIBUTE_AXIS;
            }
            else if (axis == txXPathAtoms::child) {
                axisIdentifier = LocationStep::CHILD_AXIS;
            }
            else if (axis == txXPathAtoms::descendant) {
                axisIdentifier = LocationStep::DESCENDANT_AXIS;
            }
            else if (axis == txXPathAtoms::descendantOrSelf) {
                axisIdentifier = LocationStep::DESCENDANT_OR_SELF_AXIS;
            }
            else if (axis == txXPathAtoms::following) {
                axisIdentifier = LocationStep::FOLLOWING_AXIS;
            }
            else if (axis == txXPathAtoms::followingSibling) {
                axisIdentifier = LocationStep::FOLLOWING_SIBLING_AXIS;
            }
            else if (axis == txXPathAtoms::_namespace) {
                axisIdentifier = LocationStep::NAMESPACE_AXIS;
            }
            else if (axis == txXPathAtoms::parent) {
                axisIdentifier = LocationStep::PARENT_AXIS;
            }
            else if (axis == txXPathAtoms::preceding) {
                axisIdentifier = LocationStep::PRECEDING_AXIS;
            }
            else if (axis == txXPathAtoms::precedingSibling) {
                axisIdentifier = LocationStep::PRECEDING_SIBLING_AXIS;
            }
            else if (axis == txXPathAtoms::self) {
                axisIdentifier = LocationStep::SELF_AXIS;
            }
            else {
                return NS_ERROR_XPATH_INVALID_AXIS;
            }
            break;
        }
        case Token::AT_SIGN:
            
            lexer.nextToken();
            axisIdentifier = LocationStep::ATTRIBUTE_AXIS;
            break;
        case Token::PARENT_NODE :
            
            lexer.nextToken();
            axisIdentifier = LocationStep::PARENT_AXIS;
            nodeTest = new txNodeTypeTest(txNodeTypeTest::NODE_TYPE);
            NS_ENSURE_TRUE(nodeTest, NS_ERROR_OUT_OF_MEMORY);
            break;
        case Token::SELF_NODE :
            
            lexer.nextToken();
            axisIdentifier = LocationStep::SELF_AXIS;
            nodeTest = new txNodeTypeTest(txNodeTypeTest::NODE_TYPE);
            NS_ENSURE_TRUE(nodeTest, NS_ERROR_OUT_OF_MEMORY);
            break;
        default:
            break;
    }

    
    nsresult rv = NS_OK;
    if (!nodeTest) {
        tok = lexer.nextToken();

        if (tok->mType == Token::CNAME) {
            
            nsCOMPtr<nsIAtom> prefix, lName;
            PRInt32 nspace;
            rv = resolveQName(tok->Value(), getter_AddRefs(prefix),
                              aContext, getter_AddRefs(lName),
                              nspace, PR_TRUE);
            NS_ENSURE_SUCCESS(rv, rv);

            nodeTest =
              new txNameTest(prefix, lName, nspace,
                             axisIdentifier == LocationStep::ATTRIBUTE_AXIS ?
                             static_cast<PRUint16>(txXPathNodeType::ATTRIBUTE_NODE) :
                             static_cast<PRUint16>(txXPathNodeType::ELEMENT_NODE));
            NS_ENSURE_TRUE(nodeTest, NS_ERROR_OUT_OF_MEMORY);
        }
        else {
            lexer.pushBack();
            rv = createNodeTypeTest(lexer, getter_Transfers(nodeTest));
            NS_ENSURE_SUCCESS(rv, rv);
        }
    }
    
    nsAutoPtr<LocationStep> lstep(new LocationStep(nodeTest, axisIdentifier));
    NS_ENSURE_TRUE(lstep, NS_ERROR_OUT_OF_MEMORY);

    nodeTest.forget();

    
    rv = parsePredicates(lstep, lexer, aContext);
    NS_ENSURE_SUCCESS(rv, rv);

    *aExpr = lstep.forget();
    return NS_OK;
}





nsresult
txExprParser::createNodeTypeTest(txExprLexer& lexer, txNodeTest** aTest)
{
    *aTest = 0;
    nsAutoPtr<txNodeTypeTest> nodeTest;

    Token* nodeTok = lexer.nextToken();

    switch (nodeTok->mType) {
        case Token::COMMENT_AND_PAREN:
            nodeTest = new txNodeTypeTest(txNodeTypeTest::COMMENT_TYPE);
            break;
        case Token::NODE_AND_PAREN:
            nodeTest = new txNodeTypeTest(txNodeTypeTest::NODE_TYPE);
            break;
        case Token::PROC_INST_AND_PAREN:
            nodeTest = new txNodeTypeTest(txNodeTypeTest::PI_TYPE);
            break;
        case Token::TEXT_AND_PAREN:
            nodeTest = new txNodeTypeTest(txNodeTypeTest::TEXT_TYPE);
            break;
        default:
            lexer.pushBack();
            return NS_ERROR_XPATH_NO_NODE_TYPE_TEST;
    }
    NS_ENSURE_TRUE(nodeTest, NS_ERROR_OUT_OF_MEMORY);

    if (nodeTok->mType == Token::PROC_INST_AND_PAREN &&
        lexer.peek()->mType == Token::LITERAL) {
        Token* tok = lexer.nextToken();
        nodeTest->setNodeName(tok->Value());
    }
    if (lexer.nextToken()->mType != Token::R_PAREN) {
        lexer.pushBack();
        return NS_ERROR_XPATH_PAREN_EXPECTED;
    }

    *aTest = nodeTest.forget();
    return NS_OK;
}





nsresult
txExprParser::createPathExpr(txExprLexer& lexer, txIParseContext* aContext,
                             Expr** aResult)
{
    *aResult = nsnull;

    nsAutoPtr<Expr> expr;

    Token* tok = lexer.peek();

    
    if (tok->mType == Token::PARENT_OP) {
        lexer.nextToken();
        if (!isLocationStepToken(lexer.peek())) {
            *aResult = new RootExpr();
            NS_ENSURE_TRUE(*aResult, NS_ERROR_OUT_OF_MEMORY);
            return NS_OK;
        }
        lexer.pushBack();
    }

    
    nsresult rv = NS_OK;
    if (tok->mType != Token::PARENT_OP &&
        tok->mType != Token::ANCESTOR_OP) {
        rv = createFilterOrStep(lexer, aContext, getter_Transfers(expr));
        NS_ENSURE_SUCCESS(rv, rv);

        
        tok = lexer.peek();
        if (tok->mType != Token::PARENT_OP &&
            tok->mType != Token::ANCESTOR_OP) {
            *aResult = expr.forget();
            return NS_OK;
        }
    }
    else {
        expr = new RootExpr();
        NS_ENSURE_TRUE(expr, NS_ERROR_OUT_OF_MEMORY);

#ifdef TX_TO_STRING
        static_cast<RootExpr*>(expr.get())->setSerialize(PR_FALSE);
#endif
    }
    
    
    nsAutoPtr<PathExpr> pathExpr(new PathExpr());
    NS_ENSURE_TRUE(pathExpr, NS_ERROR_OUT_OF_MEMORY);

    rv = pathExpr->addExpr(expr, PathExpr::RELATIVE_OP);
    NS_ENSURE_SUCCESS(rv, rv);

    expr.forget();

    
    while (1) {
        PathExpr::PathOperator pathOp;
        tok = lexer.nextToken();
        switch (tok->mType) {
            case Token::ANCESTOR_OP :
                pathOp = PathExpr::DESCENDANT_OP;
                break;
            case Token::PARENT_OP :
                pathOp = PathExpr::RELATIVE_OP;
                break;
            default:
                lexer.pushBack();
                *aResult = pathExpr.forget();
                return NS_OK;
        }
        
        rv = createLocationStep(lexer, aContext, getter_Transfers(expr));
        NS_ENSURE_SUCCESS(rv, rv);

        rv = pathExpr->addExpr(expr, pathOp);
        NS_ENSURE_SUCCESS(rv, rv);

        expr.forget();
    }
    NS_NOTREACHED("internal xpath parser error");
    return NS_ERROR_UNEXPECTED;
}





nsresult
txExprParser::createUnionExpr(txExprLexer& lexer, txIParseContext* aContext,
                              Expr** aResult)
{
    *aResult = nsnull;

    nsAutoPtr<Expr> expr;
    nsresult rv = createPathExpr(lexer, aContext, getter_Transfers(expr));
    NS_ENSURE_SUCCESS(rv, rv);
    
    if (lexer.peek()->mType != Token::UNION_OP) {
        *aResult = expr.forget();
        return NS_OK;
    }

    nsAutoPtr<UnionExpr> unionExpr(new UnionExpr());
    NS_ENSURE_TRUE(unionExpr, NS_ERROR_OUT_OF_MEMORY);

    rv = unionExpr->addExpr(expr);
    NS_ENSURE_SUCCESS(rv, rv);

    expr.forget();

    while (lexer.peek()->mType == Token::UNION_OP) {
        lexer.nextToken(); 

        rv = createPathExpr(lexer, aContext, getter_Transfers(expr));
        NS_ENSURE_SUCCESS(rv, rv);

        rv = unionExpr->addExpr(expr.forget());
        NS_ENSURE_SUCCESS(rv, rv);
    }

    *aResult = unionExpr.forget();
    return NS_OK;
}

PRBool
txExprParser::isLocationStepToken(Token* aToken)
{
    
    return aToken->mType == Token::AXIS_IDENTIFIER ||
           aToken->mType == Token::AT_SIGN ||
           aToken->mType == Token::PARENT_NODE ||
           aToken->mType == Token::SELF_NODE ||
           aToken->mType == Token::CNAME ||
           aToken->mType == Token::COMMENT_AND_PAREN ||
           aToken->mType == Token::NODE_AND_PAREN ||
           aToken->mType == Token::PROC_INST_AND_PAREN ||
           aToken->mType == Token::TEXT_AND_PAREN;
}









nsresult
txExprParser::parsePredicates(PredicateList* aPredicateList,
                              txExprLexer& lexer, txIParseContext* aContext)
{
    nsAutoPtr<Expr> expr;
    nsresult rv = NS_OK;
    while (lexer.peek()->mType == Token::L_BRACKET) {
        
        lexer.nextToken();

        rv = createExpr(lexer, aContext, getter_Transfers(expr));
        NS_ENSURE_SUCCESS(rv, rv);

        rv = aPredicateList->add(expr);
        NS_ENSURE_SUCCESS(rv, rv);

        expr.forget();

        if (lexer.nextToken()->mType != Token::R_BRACKET) {
            lexer.pushBack();
            return NS_ERROR_XPATH_BRACKET_EXPECTED;
        }
    }
    return NS_OK;
}










nsresult
txExprParser::parseParameters(FunctionCall* aFnCall, txExprLexer& lexer,
                              txIParseContext* aContext)
{
    if (lexer.peek()->mType == Token::R_PAREN) {
        lexer.nextToken();
        return NS_OK;
    }

    nsAutoPtr<Expr> expr;
    nsresult rv = NS_OK;
    while (1) {
        rv = createExpr(lexer, aContext, getter_Transfers(expr));
        NS_ENSURE_SUCCESS(rv, rv);

        if (aFnCall) {
            rv = aFnCall->addParam(expr.forget());
            NS_ENSURE_SUCCESS(rv, rv);
        }
                    
        switch (lexer.nextToken()->mType) {
            case Token::R_PAREN :
                return NS_OK;
            case Token::COMMA: 
                break;
            default:
                lexer.pushBack();
                return NS_ERROR_XPATH_PAREN_EXPECTED;
        }
    }

    NS_NOTREACHED("internal xpath parser error");
    return NS_ERROR_UNEXPECTED;
}

short
txExprParser::precedence(Token* aToken)
{
    switch (aToken->mType) {
        case Token::OR_OP:
            return 1;
        case Token::AND_OP:
            return 2;
        
        case Token::EQUAL_OP:
        case Token::NOT_EQUAL_OP:
            return 3;
        
        case Token::LESS_THAN_OP:
        case Token::GREATER_THAN_OP:
        case Token::LESS_OR_EQUAL_OP:
        case Token::GREATER_OR_EQUAL_OP:
            return 4;
        
        case Token::ADDITION_OP:
        case Token::SUBTRACTION_OP:
            return 5;
        
        case Token::DIVIDE_OP:
        case Token::MULTIPLY_OP:
        case Token::MODULUS_OP:
            return 6;
        default:
            break;
    }
    return 0;
}

nsresult
txExprParser::resolveQName(const nsAString& aQName,
                           nsIAtom** aPrefix, txIParseContext* aContext,
                           nsIAtom** aLocalName, PRInt32& aNamespace,
                           PRBool aIsNameTest)
{
    aNamespace = kNameSpaceID_None;
    PRInt32 idx = aQName.FindChar(':');
    if (idx > 0) {
        *aPrefix = NS_NewAtom(StringHead(aQName, (PRUint32)idx));
        if (!*aPrefix) {
            return NS_ERROR_OUT_OF_MEMORY;
        }
        *aLocalName = NS_NewAtom(Substring(aQName, (PRUint32)idx + 1,
                                           aQName.Length() - (idx + 1)));
        if (!*aLocalName) {
            NS_RELEASE(*aPrefix);
            return NS_ERROR_OUT_OF_MEMORY;
        }
        return aContext->resolveNamespacePrefix(*aPrefix, aNamespace);
    }
    
    *aPrefix = 0;
    if (aIsNameTest && aContext->caseInsensitiveNameTests()) {
        nsAutoString lcname;
        TX_ToLowerCase(aQName, lcname);
        *aLocalName = NS_NewAtom(lcname);
    }
    else {
        *aLocalName = NS_NewAtom(aQName);
    }
    if (!*aLocalName) {
        return NS_ERROR_OUT_OF_MEMORY;
    }
    return NS_OK;
}
