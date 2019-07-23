






































#include "txIXPathContext.h"
#include "txAtoms.h"
#include "txError.h"
#include "txXMLUtils.h"
#include "txXSLTFunctions.h"
#include "txNamespaceMap.h"

nsresult
txXSLTEnvironmentFunctionCall::evaluate(txIEvalContext* aContext,
                                        txAExprResult** aResult)
{
    *aResult = nsnull;

    if (!requireParams(1, 1, aContext)) {
        return NS_ERROR_XPATH_BAD_ARGUMENT_COUNT;
    }

    nsAutoString property;
    nsresult rv = mParams[0]->evaluateToString(aContext, property);
    NS_ENSURE_SUCCESS(rv, rv);

    txExpandedName qname;
    rv = qname.init(property, mMappings, mType != FUNCTION_AVAILABLE);
    NS_ENSURE_SUCCESS(rv, rv);

    switch (mType) {
        case SYSTEM_PROPERTY:
        {
            if (qname.mNamespaceID == kNameSpaceID_XSLT) {
                if (qname.mLocalName == txXSLTAtoms::version) {
                    return aContext->recycler()->getNumberResult(1.0, aResult);
                }
                if (qname.mLocalName == txXSLTAtoms::vendor) {
                    return aContext->recycler()->getStringResult(
                          NS_LITERAL_STRING("Transformiix"), aResult);
                }
                if (qname.mLocalName == txXSLTAtoms::vendorUrl) {
                    return aContext->recycler()->getStringResult(
                          NS_LITERAL_STRING("http://www.mozilla.org/projects/xslt/"),
                          aResult);
                }
            }
            aContext->recycler()->getEmptyStringResult(aResult);
            break;
        }
        case ELEMENT_AVAILABLE:
        {
            PRBool val = qname.mNamespaceID == kNameSpaceID_XSLT &&
                         (qname.mLocalName == txXSLTAtoms::applyImports ||
                          qname.mLocalName == txXSLTAtoms::applyTemplates ||
                          qname.mLocalName == txXSLTAtoms::attribute ||
                          qname.mLocalName == txXSLTAtoms::attributeSet ||
                          qname.mLocalName == txXSLTAtoms::callTemplate ||
                          qname.mLocalName == txXSLTAtoms::choose ||
                          qname.mLocalName == txXSLTAtoms::comment ||
                          qname.mLocalName == txXSLTAtoms::copy ||
                          qname.mLocalName == txXSLTAtoms::copyOf ||
                          qname.mLocalName == txXSLTAtoms::decimalFormat ||
                          qname.mLocalName == txXSLTAtoms::element ||
                          qname.mLocalName == txXSLTAtoms::fallback ||
                          qname.mLocalName == txXSLTAtoms::forEach ||
                          qname.mLocalName == txXSLTAtoms::_if ||
                          qname.mLocalName == txXSLTAtoms::import ||
                          qname.mLocalName == txXSLTAtoms::include ||
                          qname.mLocalName == txXSLTAtoms::key ||
                          qname.mLocalName == txXSLTAtoms::message ||
                          
                          qname.mLocalName == txXSLTAtoms::number ||
                          qname.mLocalName == txXSLTAtoms::otherwise ||
                          qname.mLocalName == txXSLTAtoms::output ||
                          qname.mLocalName == txXSLTAtoms::param ||
                          qname.mLocalName == txXSLTAtoms::preserveSpace ||
                          qname.mLocalName == txXSLTAtoms::processingInstruction ||
                          qname.mLocalName == txXSLTAtoms::sort ||
                          qname.mLocalName == txXSLTAtoms::stripSpace ||
                          qname.mLocalName == txXSLTAtoms::stylesheet ||
                          qname.mLocalName == txXSLTAtoms::_template ||
                          qname.mLocalName == txXSLTAtoms::text ||
                          qname.mLocalName == txXSLTAtoms::transform ||
                          qname.mLocalName == txXSLTAtoms::valueOf ||
                          qname.mLocalName == txXSLTAtoms::variable ||
                          qname.mLocalName == txXSLTAtoms::when ||
                          qname.mLocalName == txXSLTAtoms::withParam);

            aContext->recycler()->getBoolResult(val, aResult);
            break;
        }
        case FUNCTION_AVAILABLE:
        {
            extern PRBool TX_XSLTFunctionAvailable(nsIAtom* aName,
                                                   PRInt32 aNameSpaceID);

            txCoreFunctionCall::eType type;
            PRBool val = (qname.mNamespaceID == kNameSpaceID_None &&
                          txCoreFunctionCall::getTypeFromAtom(qname.mLocalName,
                                                              type)) ||
                         TX_XSLTFunctionAvailable(qname.mLocalName,
                                                  qname.mNamespaceID);

            aContext->recycler()->getBoolResult(val, aResult);
            break;
        }
    }

    return NS_OK;
}

Expr::ResultType
txXSLTEnvironmentFunctionCall::getReturnType()
{
    return mType == SYSTEM_PROPERTY ? (STRING_RESULT | NUMBER_RESULT) :
                                      BOOLEAN_RESULT;
}

PRBool
txXSLTEnvironmentFunctionCall::isSensitiveTo(ContextSensitivity aContext)
{
    return argsSensitiveTo(aContext);
}

#ifdef TX_TO_STRING
nsresult
txXSLTEnvironmentFunctionCall::getNameAtom(nsIAtom** aAtom)
{
    *aAtom = mType == SYSTEM_PROPERTY ? txXSLTAtoms::systemProperty :
             mType == ELEMENT_AVAILABLE ? txXSLTAtoms::elementAvailable :
             txXSLTAtoms::functionAvailable;
    NS_ADDREF(*aAtom);

    return NS_OK;
}
#endif
