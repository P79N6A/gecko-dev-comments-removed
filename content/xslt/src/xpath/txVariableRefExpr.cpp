





































#include "txExpr.h"
#include "nsIAtom.h"
#include "txNodeSet.h"
#include "txAtoms.h"
#include "txIXPathContext.h"

  
 





VariableRefExpr::VariableRefExpr(nsIAtom* aPrefix, nsIAtom* aLocalName,
                                 PRInt32 aNSID)
    : mPrefix(aPrefix), mLocalName(aLocalName), mNamespace(aNSID)
{
    NS_ASSERTION(mLocalName, "VariableRefExpr without local name?");
    if (mPrefix == txXMLAtoms::_empty)
        mPrefix = 0;
}








nsresult
VariableRefExpr::evaluate(txIEvalContext* aContext, txAExprResult** aResult)
{
    nsresult rv = aContext->getVariable(mNamespace, mLocalName, *aResult);
    if (NS_FAILED(rv)) {
      
      return rv;
    }
    return NS_OK;
}

TX_IMPL_EXPR_STUBS_0(VariableRefExpr, ANY_RESULT)

PRBool
VariableRefExpr::isSensitiveTo(ContextSensitivity aContext)
{
    return !!(aContext & VARIABLES_CONTEXT);
}

#ifdef TX_TO_STRING
void
VariableRefExpr::toString(nsAString& aDest)
{
    aDest.Append(PRUnichar('$'));
    if (mPrefix) {
        nsAutoString prefix;
        mPrefix->ToString(prefix);
        aDest.Append(prefix);
        aDest.Append(PRUnichar(':'));
    }
    nsAutoString lname;
    mLocalName->ToString(lname);
    aDest.Append(lname);
}
#endif
