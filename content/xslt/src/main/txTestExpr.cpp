





































#include "nsXPCOM.h"
#include "txStandaloneXSLTProcessor.h"
#include "nsString.h"
#include "txExprParser.h"
#include "txIXPathContext.h"





static const char* kTokens[] = {"(", "concat", "(", "foo", ",", "'", "bar",
                                "'",")", "//", ".", "[", "preceding-sibling",
                                "::", "bar", "]", "/", "*", "[", "23", "]",
                                "|", "node", "(", ")", ")", "<", "3"};
static const PRUint8 kCount = sizeof(kTokens)/sizeof(char*);

class ParseContextImpl : public txIParseContext
{
public:
    nsresult
    resolveNamespacePrefix(nsIAtom* aPrefix, PRInt32& aID)
    {
        return NS_ERROR_FAILURE;
    }
    nsresult
    resolveFunctionCall(nsIAtom* aName, PRInt32 aID, FunctionCall** aFunction)
    {
        return NS_ERROR_XPATH_UNKNOWN_FUNCTION;
    }
    PRBool
    caseInsensitiveNameTests()
    {
        return PR_FALSE;
    }
    void
    SetErrorOffset(PRUint32 aOffset)
    {
        mOff = aOffset;
    }
    PRUint32 mOff;
};

static void doTest(const nsASingleFragmentString& aExpr)
{
    ParseContextImpl ct;
    nsAutoPtr<Expr> expression;
    cout << NS_LossyConvertUTF16toASCII(aExpr).get() << endl;
    ct.mOff = 0;
    nsresult rv = txExprParser::createExpr(aExpr, &ct,
                                           getter_Transfers(expression));

    cout << "createExpr returned " << ios::hex << rv  << ios::dec;
    cout << " at " << ct.mOff << endl;
    if (NS_FAILED(rv)) {
        NS_LossyConvertUTF16toASCII cstring(aExpr);
        cout << NS_LossyConvertUTF16toASCII(StringHead(aExpr, ct.mOff)).get();
        cout << " ^ ";
        cout << NS_LossyConvertUTF16toASCII(StringTail(aExpr, aExpr.Length()-ct.mOff)).get();
        cout << endl << endl;
    }
#ifdef TX_TO_STRING
    else {
        nsAutoString expr;
        expression->toString(expr);
        cout << "parsed expression: ";
        cout << NS_LossyConvertUTF16toASCII(expr).get() << endl << endl;
    }
#endif
}

int main(int argc, char** argv)
{
    using namespace std;
    nsresult rv = NS_InitXPCOM2(nsnull, nsnull, nsnull);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!txXSLTProcessor::init())
        return 1;

    nsAutoString exprOrig, expr;
    nsStringArray exprHead, exprTail;
    PRUint8 i, dropStart, dropEnd;
    exprHead.AppendString(NS_ConvertASCIItoUTF16(kTokens[0]));
    exprTail.AppendString(NS_ConvertASCIItoUTF16(kTokens[kCount - 1]));
    for (i = 2; i < kCount; ++i) {
        exprHead.AppendString(*exprHead[i - 2] +
                              NS_ConvertASCIItoUTF16(kTokens[i - 1]));
        exprTail.AppendString(NS_ConvertASCIItoUTF16(kTokens[kCount - i]) +
                              *exprTail[i - 2]);
    }
    exprOrig = NS_ConvertASCIItoUTF16(kTokens[0]) + *exprTail[kCount - 2];
    cout << NS_LossyConvertUTF16toASCII(exprOrig).get() << endl << endl;
    for (dropStart = 0; dropStart < kCount - 2; ++dropStart) {
        doTest(*exprTail[kCount - 2 - dropStart]);
        for (dropEnd = kCount - 3 - dropStart; dropEnd > 0; --dropEnd) {
            expr = *exprHead[dropStart] + *exprTail[dropEnd];
            doTest(expr);
        }
        doTest(*exprHead[dropStart]);
    }
    doTest(*exprHead[kCount - 2]);

    txXSLTProcessor::shutdown();
    rv = NS_ShutdownXPCOM(nsnull);
    NS_ENSURE_SUCCESS(rv, rv);
    return 0;
}
