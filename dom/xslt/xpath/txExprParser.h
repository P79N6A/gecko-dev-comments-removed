










#ifndef MITREXSL_EXPRPARSER_H
#define MITREXSL_EXPRPARSER_H

#include "txCore.h"
#include "nsAutoPtr.h"
#include "nsString.h"

class Expr;
class txExprLexer;
class FunctionCall;
class LocationStep;
class nsIAtom;
class PredicateList;
class Token;
class txIParseContext;
class txNodeTest;

class txExprParser
{
public:

    static nsresult createExpr(const nsSubstring& aExpression,
                               txIParseContext* aContext, Expr** aExpr)
    {
        return createExprInternal(aExpression, 0, aContext, aExpr);
    }

    


    static nsresult createAVT(const nsSubstring& aAttrValue,
                              txIParseContext* aContext,
                              Expr** aResult);


protected:
    static nsresult createExprInternal(const nsSubstring& aExpression,
                                       uint32_t aSubStringPos,
                                       txIParseContext* aContext,
                                       Expr** aExpr);
    



    static nsresult createBinaryExpr(nsAutoPtr<Expr>& left,
                                     nsAutoPtr<Expr>& right, Token* op,
                                     Expr** aResult);
    static nsresult createExpr(txExprLexer& lexer, txIParseContext* aContext,
                               Expr** aResult);
    static nsresult createFilterOrStep(txExprLexer& lexer,
                                       txIParseContext* aContext,
                                       Expr** aResult);
    static nsresult createFunctionCall(txExprLexer& lexer,
                                       txIParseContext* aContext,
                                       Expr** aResult);
    static nsresult createLocationStep(txExprLexer& lexer,
                                       txIParseContext* aContext,
                                       Expr** aResult);
    static nsresult createNodeTypeTest(txExprLexer& lexer,
                                       txNodeTest** aResult);
    static nsresult createPathExpr(txExprLexer& lexer,
                                   txIParseContext* aContext,
                                   Expr** aResult);
    static nsresult createUnionExpr(txExprLexer& lexer,
                                    txIParseContext* aContext,
                                    Expr** aResult);
                  
    static bool isLocationStepToken(Token* aToken);
                  
    static short precedence(Token* aToken);

    



    static nsresult resolveQName(const nsAString& aQName, nsIAtom** aPrefix,
                                 txIParseContext* aContext,
                                 nsIAtom** aLocalName, int32_t& aNamespace,
                                 bool aIsNameTest = false);

    








    static nsresult parsePredicates(PredicateList* aPredicateList,
                                    txExprLexer& lexer,
                                    txIParseContext* aContext);
    static nsresult parseParameters(FunctionCall* aFnCall, txExprLexer& lexer,
                                    txIParseContext* aContext);

};

#endif
