





































#include "txStylesheetCompiler.h"
#include "txStylesheetCompileHandlers.h"
#include "txTokenizer.h"
#include "txInstructions.h"
#include "txAtoms.h"
#include "txCore.h"
#include "txStringUtils.h"
#include "txStylesheet.h"
#include "txToplevelItems.h"
#include "txPatternParser.h"
#include "txNamespaceMap.h"
#include "txURIUtils.h"
#include "txXSLTFunctions.h"

txHandlerTable* gTxIgnoreHandler = 0;
txHandlerTable* gTxRootHandler = 0;
txHandlerTable* gTxEmbedHandler = 0;
txHandlerTable* gTxTopHandler = 0;
txHandlerTable* gTxTemplateHandler = 0;
txHandlerTable* gTxTextHandler = 0;
txHandlerTable* gTxApplyTemplatesHandler = 0;
txHandlerTable* gTxCallTemplateHandler = 0;
txHandlerTable* gTxVariableHandler = 0;
txHandlerTable* gTxForEachHandler = 0;
txHandlerTable* gTxTopVariableHandler = 0;
txHandlerTable* gTxChooseHandler = 0;
txHandlerTable* gTxParamHandler = 0;
txHandlerTable* gTxImportHandler = 0;
txHandlerTable* gTxAttributeSetHandler = 0;
txHandlerTable* gTxFallbackHandler = 0;

nsresult
txFnStartLRE(PRInt32 aNamespaceID,
             nsIAtom* aLocalName,
             nsIAtom* aPrefix,
             txStylesheetAttr* aAttributes,
             PRInt32 aAttrCount,
             txStylesheetCompilerState& aState);
nsresult
txFnEndLRE(txStylesheetCompilerState& aState);


#define TX_RETURN_IF_WHITESPACE(_str, _state)                               \
    do {                                                                    \
      if (!_state.mElementContext->mPreserveWhitespace &&                   \
          XMLUtils::isWhitespace(PromiseFlatString(_str))) {                \
          return NS_OK;                                                     \
      }                                                                     \
    } while(0)


nsresult
getStyleAttr(txStylesheetAttr* aAttributes,
             PRInt32 aAttrCount,
             PRInt32 aNamespace,
             nsIAtom* aName,
             PRBool aRequired,
             txStylesheetAttr** aAttr)
{
    PRInt32 i;
    for (i = 0; i < aAttrCount; ++i) {
        txStylesheetAttr* attr = aAttributes + i;
        if (attr->mNamespaceID == aNamespace &&
            attr->mLocalName == aName) {
            attr->mLocalName = nsnull;
            *aAttr = attr;

            return NS_OK;
        }
    }
    *aAttr = nsnull;
    
    if (aRequired) {
        
        return NS_ERROR_XSLT_PARSE_FAILURE;
    }
    
    return NS_OK;
}

nsresult
parseUseAttrSets(txStylesheetAttr* aAttributes,
                 PRInt32 aAttrCount,
                 PRBool aInXSLTNS,
                 txStylesheetCompilerState& aState)
{
    txStylesheetAttr* attr = nsnull;
    nsresult rv = getStyleAttr(aAttributes, aAttrCount,
                               aInXSLTNS ? kNameSpaceID_XSLT
                                         : kNameSpaceID_None,
                               txXSLTAtoms::useAttributeSets, PR_FALSE,
                               &attr);
    if (!attr) {
        return rv;
    }

    txTokenizer tok(attr->mValue);
    while (tok.hasMoreTokens()) {
        txExpandedName name;
        rv = name.init(tok.nextToken(), aState.mElementContext->mMappings,
                       PR_FALSE);
        NS_ENSURE_SUCCESS(rv, rv);

        nsAutoPtr<txInstruction> instr(new txInsertAttrSet(name));
        NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

        rv = aState.addInstruction(instr);
        NS_ENSURE_SUCCESS(rv, rv);
    }
    return NS_OK;
}

nsresult
parseExcludeResultPrefixes(txStylesheetAttr* aAttributes,
                           PRInt32 aAttrCount,
                           PRInt32 aNamespaceID)
{
    txStylesheetAttr* attr = nsnull;
    nsresult rv = getStyleAttr(aAttributes, aAttrCount, aNamespaceID,
                               txXSLTAtoms::excludeResultPrefixes, PR_FALSE,
                               &attr);
    if (!attr) {
        return rv;
    }

    

    return NS_OK;
}

nsresult
getQNameAttr(txStylesheetAttr* aAttributes,
             PRInt32 aAttrCount,
             nsIAtom* aName,
             PRBool aRequired,
             txStylesheetCompilerState& aState,
             txExpandedName& aExpName)
{
    aExpName.reset();
    txStylesheetAttr* attr = nsnull;
    nsresult rv = getStyleAttr(aAttributes, aAttrCount, kNameSpaceID_None,
                               aName, aRequired, &attr);
    if (!attr) {
        return rv;
    }

    rv = aExpName.init(attr->mValue, aState.mElementContext->mMappings,
                       PR_FALSE);
    if (!aRequired && NS_FAILED(rv) && aState.fcp()) {
        aExpName.reset();
        rv = NS_OK;
    }

    return rv;
}

nsresult
getExprAttr(txStylesheetAttr* aAttributes,
            PRInt32 aAttrCount,
            nsIAtom* aName,
            PRBool aRequired,
            txStylesheetCompilerState& aState,
            nsAutoPtr<Expr>& aExpr)
{
    aExpr = nsnull;
    txStylesheetAttr* attr = nsnull;
    nsresult rv = getStyleAttr(aAttributes, aAttrCount, kNameSpaceID_None,
                               aName, aRequired, &attr);
    if (!attr) {
        return rv;
    }

    rv = txExprParser::createExpr(attr->mValue, &aState,
                                  getter_Transfers(aExpr));
    if (NS_FAILED(rv) && aState.fcp()) {
        
        if (aRequired) {
            aExpr = new txErrorExpr(
#ifdef TX_TO_STRING
                                    attr->mValue
#endif
                                    );
            NS_ENSURE_TRUE(aExpr, NS_ERROR_OUT_OF_MEMORY);
        }
        else {
            aExpr = nsnull;
        }
        return NS_OK;
    }

    return rv;
}

nsresult
getAVTAttr(txStylesheetAttr* aAttributes,
           PRInt32 aAttrCount,
           nsIAtom* aName,
           PRBool aRequired,
           txStylesheetCompilerState& aState,
           nsAutoPtr<Expr>& aAVT)
{
    aAVT = nsnull;
    txStylesheetAttr* attr = nsnull;
    nsresult rv = getStyleAttr(aAttributes, aAttrCount, kNameSpaceID_None,
                               aName, aRequired, &attr);
    if (!attr) {
        return rv;
    }

    rv = txExprParser::createAVT(attr->mValue, &aState,
                                 getter_Transfers(aAVT));
    if (NS_FAILED(rv) && aState.fcp()) {
        
        if (aRequired) {
            aAVT = new txErrorExpr(
#ifdef TX_TO_STRING
                                   attr->mValue
#endif
                                   );
            NS_ENSURE_TRUE(aAVT, NS_ERROR_OUT_OF_MEMORY);
        }
        else {
            aAVT = nsnull;
        }
        return NS_OK;
    }

    return rv;
}

nsresult
getPatternAttr(txStylesheetAttr* aAttributes,
               PRInt32 aAttrCount,
               nsIAtom* aName,
               PRBool aRequired,
               txStylesheetCompilerState& aState,
               nsAutoPtr<txPattern>& aPattern)
{
    aPattern = nsnull;
    txStylesheetAttr* attr = nsnull;
    nsresult rv = getStyleAttr(aAttributes, aAttrCount, kNameSpaceID_None,
                               aName, aRequired, &attr);
    if (!attr) {
        return rv;
    }

    aPattern = txPatternParser::createPattern(attr->mValue, &aState);
    if (!aPattern && (aRequired || !aState.fcp())) {
        
        return NS_ERROR_XPATH_PARSE_FAILURE;
    }

    return NS_OK;
}

nsresult
getNumberAttr(txStylesheetAttr* aAttributes,
              PRInt32 aAttrCount,
              nsIAtom* aName,
              PRBool aRequired,
              txStylesheetCompilerState& aState,
              double& aNumber)
{
    aNumber = Double::NaN;
    txStylesheetAttr* attr = nsnull;
    nsresult rv = getStyleAttr(aAttributes, aAttrCount, kNameSpaceID_None,
                               aName, aRequired, &attr);
    if (!attr) {
        return rv;
    }

    aNumber = Double::toDouble(attr->mValue);
    if (Double::isNaN(aNumber) && (aRequired || !aState.fcp())) {
        
        return NS_ERROR_XSLT_PARSE_FAILURE;
    }

    return NS_OK;
}

nsresult
getAtomAttr(txStylesheetAttr* aAttributes,
            PRInt32 aAttrCount,
            nsIAtom* aName,
            PRBool aRequired,
            txStylesheetCompilerState& aState,
            nsIAtom** aAtom)
{
    *aAtom = nsnull;
    txStylesheetAttr* attr = nsnull;
    nsresult rv = getStyleAttr(aAttributes, aAttrCount, kNameSpaceID_None,
                               aName, aRequired, &attr);
    if (!attr) {
        return rv;
    }

    *aAtom = NS_NewAtom(attr->mValue);
    NS_ENSURE_TRUE(*aAtom, NS_ERROR_OUT_OF_MEMORY);

    return NS_OK;
}

nsresult
getYesNoAttr(txStylesheetAttr* aAttributes,
             PRInt32 aAttrCount,
             nsIAtom* aName,
             PRBool aRequired,
             txStylesheetCompilerState& aState,
             txThreeState& aRes)
{
    aRes = eNotSet;
    nsCOMPtr<nsIAtom> atom;
    nsresult rv = getAtomAttr(aAttributes, aAttrCount, aName, aRequired,
                              aState, getter_AddRefs(atom));
    if (!atom) {
        return rv;
    }

    if (atom == txXSLTAtoms::yes) {
        aRes = eTrue;
    }
    else if (atom == txXSLTAtoms::no) {
        aRes = eFalse;
    }
    else if (aRequired || !aState.fcp()) {
        
        return NS_ERROR_XSLT_PARSE_FAILURE;
    }

    return NS_OK;
}

nsresult
getCharAttr(txStylesheetAttr* aAttributes,
            PRInt32 aAttrCount,
            nsIAtom* aName,
            PRBool aRequired,
            txStylesheetCompilerState& aState,
            PRUnichar& aChar)
{
    
    txStylesheetAttr* attr = nsnull;
    nsresult rv = getStyleAttr(aAttributes, aAttrCount, kNameSpaceID_None,
                               aName, aRequired, &attr);
    if (!attr) {
        return rv;
    }

    if (attr->mValue.Length() == 1) {
        aChar = attr->mValue.CharAt(0);
    }
    else if (aRequired || !aState.fcp()) {
        
        return NS_ERROR_XSLT_PARSE_FAILURE;
    }

    return NS_OK;
}





nsresult
txFnTextIgnore(const nsAString& aStr, txStylesheetCompilerState& aState)
{
    return NS_OK;
}

nsresult
txFnTextError(const nsAString& aStr, txStylesheetCompilerState& aState)
{
    TX_RETURN_IF_WHITESPACE(aStr, aState);

    return NS_ERROR_XSLT_PARSE_FAILURE;
}

void
clearAttributes(txStylesheetAttr* aAttributes,
                     PRInt32 aAttrCount)
{
    PRInt32 i;
    for (i = 0; i < aAttrCount; ++i) {
        aAttributes[i].mLocalName = nsnull;
    }
}

nsresult
txFnStartElementIgnore(PRInt32 aNamespaceID,
                       nsIAtom* aLocalName,
                       nsIAtom* aPrefix,
                       txStylesheetAttr* aAttributes,
                       PRInt32 aAttrCount,
                       txStylesheetCompilerState& aState)
{
    if (!aState.fcp()) {
        clearAttributes(aAttributes, aAttrCount);
    }

    return NS_OK;
}

nsresult
txFnEndElementIgnore(txStylesheetCompilerState& aState)
{
    return NS_OK;
}

nsresult
txFnStartElementSetIgnore(PRInt32 aNamespaceID,
                          nsIAtom* aLocalName,
                          nsIAtom* aPrefix,
                          txStylesheetAttr* aAttributes,
                          PRInt32 aAttrCount,
                          txStylesheetCompilerState& aState)
{
    if (!aState.fcp()) {
        clearAttributes(aAttributes, aAttrCount);
    }

    return aState.pushHandlerTable(gTxIgnoreHandler);
}

nsresult
txFnEndElementSetIgnore(txStylesheetCompilerState& aState)
{
    aState.popHandlerTable();
    return NS_OK;
}

nsresult
txFnStartElementError(PRInt32 aNamespaceID,
                      nsIAtom* aLocalName,
                      nsIAtom* aPrefix,
                      txStylesheetAttr* aAttributes,
                      PRInt32 aAttrCount,
                      txStylesheetCompilerState& aState)
{
    return NS_ERROR_XSLT_PARSE_FAILURE;
}

nsresult
txFnEndElementError(txStylesheetCompilerState& aState)
{
    NS_ERROR("txFnEndElementError shouldn't be called"); 
    return NS_ERROR_XSLT_PARSE_FAILURE;
}





nsresult
txFnStartStylesheet(PRInt32 aNamespaceID,
                    nsIAtom* aLocalName,
                    nsIAtom* aPrefix,
                    txStylesheetAttr* aAttributes,
                    PRInt32 aAttrCount,
                    txStylesheetCompilerState& aState)
{
    
    

    txStylesheetAttr* attr;
    nsresult rv = getStyleAttr(aAttributes, aAttrCount, kNameSpaceID_None,
                               txXSLTAtoms::id, PR_FALSE, &attr);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = parseExcludeResultPrefixes(aAttributes, aAttrCount, kNameSpaceID_None);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = getStyleAttr(aAttributes, aAttrCount, kNameSpaceID_None,
                      txXSLTAtoms::version, PR_TRUE, &attr);
    NS_ENSURE_SUCCESS(rv, rv);

    return aState.pushHandlerTable(gTxImportHandler);
}

nsresult
txFnEndStylesheet(txStylesheetCompilerState& aState)
{
    aState.popHandlerTable();
    return NS_OK;
}

nsresult
txFnStartElementContinueTopLevel(PRInt32 aNamespaceID,
                                nsIAtom* aLocalName,
                                nsIAtom* aPrefix,
                                txStylesheetAttr* aAttributes,
                                PRInt32 aAttrCount,
                                txStylesheetCompilerState& aState)
{
    aState.mHandlerTable = gTxTopHandler;

    return NS_XSLT_GET_NEW_HANDLER;
}

nsresult
txFnStartLREStylesheet(PRInt32 aNamespaceID,
                       nsIAtom* aLocalName,
                       nsIAtom* aPrefix,
                       txStylesheetAttr* aAttributes,
                       PRInt32 aAttrCount,
                       txStylesheetCompilerState& aState)
{
    txStylesheetAttr* attr;
    nsresult rv = getStyleAttr(aAttributes, aAttrCount, kNameSpaceID_XSLT,
                               txXSLTAtoms::version, PR_TRUE, &attr);
    NS_ENSURE_SUCCESS(rv, rv);

    txExpandedName nullExpr;
    double prio = Double::NaN;

    nsAutoPtr<txPattern> match(new txRootPattern());
    NS_ENSURE_TRUE(match, NS_ERROR_OUT_OF_MEMORY);

    nsAutoPtr<txTemplateItem> templ(new txTemplateItem(match, nullExpr,
                                                       nullExpr, prio));
    NS_ENSURE_TRUE(templ, NS_ERROR_OUT_OF_MEMORY);

    aState.openInstructionContainer(templ);
    rv = aState.addToplevelItem(templ);
    NS_ENSURE_SUCCESS(rv, rv);

    templ.forget();

    rv = aState.pushHandlerTable(gTxTemplateHandler);
    NS_ENSURE_SUCCESS(rv, rv);
    
    return txFnStartLRE(aNamespaceID, aLocalName, aPrefix, aAttributes,
                        aAttrCount, aState);
}

nsresult
txFnEndLREStylesheet(txStylesheetCompilerState& aState)
{
    nsresult rv = txFnEndLRE(aState);
    NS_ENSURE_SUCCESS(rv, rv);

    aState.popHandlerTable();

    nsAutoPtr<txInstruction> instr(new txReturn());
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

    rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    aState.closeInstructionContainer();

    return NS_OK;
}

nsresult
txFnStartEmbed(PRInt32 aNamespaceID,
               nsIAtom* aLocalName,
               nsIAtom* aPrefix,
               txStylesheetAttr* aAttributes,
               PRInt32 aAttrCount,
               txStylesheetCompilerState& aState)
{
    if (!aState.handleEmbeddedSheet()) {
        return NS_OK;
    }
    if (aNamespaceID != kNameSpaceID_XSLT ||
        (aLocalName != txXSLTAtoms::stylesheet &&
         aLocalName != txXSLTAtoms::transform)) {
        return NS_ERROR_XSLT_PARSE_FAILURE;
    }
    return txFnStartStylesheet(aNamespaceID, aLocalName, aPrefix,
                               aAttributes, aAttrCount, aState);
}

nsresult
txFnEndEmbed(txStylesheetCompilerState& aState)
{
    if (!aState.handleEmbeddedSheet()) {
        return NS_OK;
    }
    nsresult rv = txFnEndStylesheet(aState);
    aState.doneEmbedding();
    return rv;
}





nsresult
txFnStartOtherTop(PRInt32 aNamespaceID,
                  nsIAtom* aLocalName,
                  nsIAtom* aPrefix,
                  txStylesheetAttr* aAttributes,
                  PRInt32 aAttrCount,
                  txStylesheetCompilerState& aState)
{
    if (aNamespaceID == kNameSpaceID_None ||
        (aNamespaceID == kNameSpaceID_XSLT && !aState.fcp())) {
        return NS_ERROR_XSLT_PARSE_FAILURE;
    }

    return aState.pushHandlerTable(gTxIgnoreHandler);
}

nsresult
txFnEndOtherTop(txStylesheetCompilerState& aState)
{
    aState.popHandlerTable();
    return NS_OK;
}



nsresult
txFnStartAttributeSet(PRInt32 aNamespaceID,
                      nsIAtom* aLocalName,
                      nsIAtom* aPrefix,
                      txStylesheetAttr* aAttributes,
                      PRInt32 aAttrCount,
                      txStylesheetCompilerState& aState)
{
    nsresult rv = NS_OK;
    txExpandedName name;
    rv = getQNameAttr(aAttributes, aAttrCount, txXSLTAtoms::name, PR_TRUE,
                      aState, name);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoPtr<txAttributeSetItem> attrSet(new txAttributeSetItem(name));
    NS_ENSURE_TRUE(attrSet, NS_ERROR_OUT_OF_MEMORY);

    aState.openInstructionContainer(attrSet);

    rv = aState.addToplevelItem(attrSet);
    NS_ENSURE_SUCCESS(rv, rv);

    attrSet.forget();

    rv = parseUseAttrSets(aAttributes, aAttrCount, PR_FALSE, aState);
    NS_ENSURE_SUCCESS(rv, rv);

    return aState.pushHandlerTable(gTxAttributeSetHandler);
}

nsresult
txFnEndAttributeSet(txStylesheetCompilerState& aState)
{
    aState.popHandlerTable();

    nsAutoPtr<txInstruction> instr(new txReturn());
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

    nsresult rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    aState.closeInstructionContainer();

    return NS_OK;
}



nsresult
txFnStartDecimalFormat(PRInt32 aNamespaceID,
                       nsIAtom* aLocalName,
                       nsIAtom* aPrefix,
                       txStylesheetAttr* aAttributes,
                       PRInt32 aAttrCount,
                       txStylesheetCompilerState& aState)
{
    nsresult rv = NS_OK;
    txExpandedName name;
    rv = getQNameAttr(aAttributes, aAttrCount, txXSLTAtoms::name, PR_FALSE,
                      aState, name);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoPtr<txDecimalFormat> format(new txDecimalFormat);
    NS_ENSURE_TRUE(format, NS_ERROR_OUT_OF_MEMORY);

    rv = getCharAttr(aAttributes, aAttrCount, txXSLTAtoms::decimalSeparator,
                     PR_FALSE, aState, format->mDecimalSeparator);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = getCharAttr(aAttributes, aAttrCount, txXSLTAtoms::groupingSeparator,
                     PR_FALSE, aState, format->mGroupingSeparator);
    NS_ENSURE_SUCCESS(rv, rv);

    txStylesheetAttr* attr = nsnull;
    rv = getStyleAttr(aAttributes, aAttrCount, kNameSpaceID_None,
                      txXSLTAtoms::infinity, PR_FALSE, &attr);
    NS_ENSURE_SUCCESS(rv, rv);

    if (attr) {
        format->mInfinity = attr->mValue;
    }

    rv = getCharAttr(aAttributes, aAttrCount, txXSLTAtoms::minusSign,
                     PR_FALSE, aState, format->mMinusSign);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = getStyleAttr(aAttributes, aAttrCount, kNameSpaceID_None,
                      txXSLTAtoms::NaN, PR_FALSE, &attr);
    NS_ENSURE_SUCCESS(rv, rv);

    if (attr) {
        format->mNaN = attr->mValue;
    }

    rv = getCharAttr(aAttributes, aAttrCount, txXSLTAtoms::percent,
                     PR_FALSE, aState, format->mPercent);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = getCharAttr(aAttributes, aAttrCount, txXSLTAtoms::perMille,
                     PR_FALSE, aState, format->mPerMille);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = getCharAttr(aAttributes, aAttrCount, txXSLTAtoms::zeroDigit,
                     PR_FALSE, aState, format->mZeroDigit);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = getCharAttr(aAttributes, aAttrCount, txXSLTAtoms::digit,
                     PR_FALSE, aState, format->mDigit);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = getCharAttr(aAttributes, aAttrCount, txXSLTAtoms::patternSeparator,
                     PR_FALSE, aState, format->mPatternSeparator);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = aState.mStylesheet->addDecimalFormat(name, format);
    NS_ENSURE_SUCCESS(rv, rv);
    
    return aState.pushHandlerTable(gTxIgnoreHandler);
}

nsresult
txFnEndDecimalFormat(txStylesheetCompilerState& aState)
{
    aState.popHandlerTable();

    return NS_OK;
}


nsresult
txFnStartImport(PRInt32 aNamespaceID,
                nsIAtom* aLocalName,
                nsIAtom* aPrefix,
                txStylesheetAttr* aAttributes,
                PRInt32 aAttrCount,
                txStylesheetCompilerState& aState)
{
    nsAutoPtr<txImportItem> import(new txImportItem);
    NS_ENSURE_TRUE(import, NS_ERROR_OUT_OF_MEMORY);
    
    import->mFrame = new txStylesheet::ImportFrame;
    NS_ENSURE_TRUE(import->mFrame, NS_ERROR_OUT_OF_MEMORY);

    nsresult rv = aState.addToplevelItem(import);
    NS_ENSURE_SUCCESS(rv, rv);
    
    txImportItem* importPtr = import.forget();
    
    txStylesheetAttr* attr = nsnull;
    rv = getStyleAttr(aAttributes, aAttrCount, kNameSpaceID_None,
                      txXSLTAtoms::href, PR_TRUE, &attr);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoString absUri;
    URIUtils::resolveHref(attr->mValue, aState.mElementContext->mBaseURI,
                          absUri);
    rv = aState.loadImportedStylesheet(absUri, importPtr->mFrame);
    NS_ENSURE_SUCCESS(rv, rv);

    return aState.pushHandlerTable(gTxIgnoreHandler);
}

nsresult
txFnEndImport(txStylesheetCompilerState& aState)
{
    aState.popHandlerTable();

    return NS_OK;
}


nsresult
txFnStartInclude(PRInt32 aNamespaceID,
                 nsIAtom* aLocalName,
                 nsIAtom* aPrefix,
                 txStylesheetAttr* aAttributes,
                 PRInt32 aAttrCount,
                 txStylesheetCompilerState& aState)
{
    txStylesheetAttr* attr = nsnull;
    nsresult rv = getStyleAttr(aAttributes, aAttrCount, kNameSpaceID_None,
                               txXSLTAtoms::href, PR_TRUE, &attr);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoString absUri;
    URIUtils::resolveHref(attr->mValue, aState.mElementContext->mBaseURI,
                          absUri);
    rv = aState.loadIncludedStylesheet(absUri);
    NS_ENSURE_SUCCESS(rv, rv);

    return aState.pushHandlerTable(gTxIgnoreHandler);
}

nsresult
txFnEndInclude(txStylesheetCompilerState& aState)
{
    aState.popHandlerTable();

    return NS_OK;
}


nsresult
txFnStartKey(PRInt32 aNamespaceID,
             nsIAtom* aLocalName,
             nsIAtom* aPrefix,
             txStylesheetAttr* aAttributes,
             PRInt32 aAttrCount,
             txStylesheetCompilerState& aState)
{
    nsresult rv = NS_OK;
    txExpandedName name;
    rv = getQNameAttr(aAttributes, aAttrCount, txXSLTAtoms::name, PR_TRUE,
                      aState, name);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoPtr<txPattern> match;
    rv = getPatternAttr(aAttributes, aAttrCount, txXSLTAtoms::match, PR_TRUE,
                        aState, match);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoPtr<Expr> use;
    rv = getExprAttr(aAttributes, aAttrCount, txXSLTAtoms::use, PR_TRUE,
                     aState, use);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = aState.mStylesheet->addKey(name, match, use);
    NS_ENSURE_SUCCESS(rv, rv);
    
    return aState.pushHandlerTable(gTxIgnoreHandler);
}

nsresult
txFnEndKey(txStylesheetCompilerState& aState)
{
    aState.popHandlerTable();

    return NS_OK;
}


nsresult
txFnStartNamespaceAlias(PRInt32 aNamespaceID,
             nsIAtom* aLocalName,
             nsIAtom* aPrefix,
             txStylesheetAttr* aAttributes,
             PRInt32 aAttrCount,
             txStylesheetCompilerState& aState)
{
    txStylesheetAttr* attr = nsnull;
    nsresult rv = getStyleAttr(aAttributes, aAttrCount, kNameSpaceID_None,
                               txXSLTAtoms::stylesheetPrefix, PR_TRUE, &attr);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = getStyleAttr(aAttributes, aAttrCount, kNameSpaceID_None,
                      txXSLTAtoms::resultPrefix, PR_TRUE, &attr);
    NS_ENSURE_SUCCESS(rv, rv);

    

    return aState.pushHandlerTable(gTxIgnoreHandler);
}

nsresult
txFnEndNamespaceAlias(txStylesheetCompilerState& aState)
{
    aState.popHandlerTable();

    return NS_OK;
}


nsresult
txFnStartOutput(PRInt32 aNamespaceID,
                nsIAtom* aLocalName,
                nsIAtom* aPrefix,
                txStylesheetAttr* aAttributes,
                PRInt32 aAttrCount,
                txStylesheetCompilerState& aState)
{
    nsresult rv = NS_OK;

    nsAutoPtr<txOutputItem> item(new txOutputItem);
    NS_ENSURE_TRUE(item, NS_ERROR_OUT_OF_MEMORY);

    txExpandedName methodExpName;
    rv = getQNameAttr(aAttributes, aAttrCount, txXSLTAtoms::method, PR_FALSE,
                      aState, methodExpName);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!methodExpName.isNull()) {
        if (methodExpName.mNamespaceID != kNameSpaceID_None) {
            
            
        }
        else if (methodExpName.mLocalName == txXSLTAtoms::html) {
            item->mFormat.mMethod = eHTMLOutput;
        }
        else if (methodExpName.mLocalName == txXSLTAtoms::text) {
            item->mFormat.mMethod = eTextOutput;
        }
        else if (methodExpName.mLocalName == txXSLTAtoms::xml) {
            item->mFormat.mMethod = eXMLOutput;
        }
        else {
            return NS_ERROR_XSLT_PARSE_FAILURE;
        }
    }

    txStylesheetAttr* attr = nsnull;
    getStyleAttr(aAttributes, aAttrCount, kNameSpaceID_None,
                 txXSLTAtoms::version, PR_FALSE, &attr);
    if (attr) {
        item->mFormat.mVersion = attr->mValue;
    }

    getStyleAttr(aAttributes, aAttrCount, kNameSpaceID_None,
                 txXSLTAtoms::encoding, PR_FALSE, &attr);
    if (attr) {
        item->mFormat.mEncoding = attr->mValue;
    }

    rv = getYesNoAttr(aAttributes, aAttrCount,
                      txXSLTAtoms::omitXmlDeclaration, PR_FALSE, aState,
                      item->mFormat.mOmitXMLDeclaration);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = getYesNoAttr(aAttributes, aAttrCount,
                      txXSLTAtoms::standalone, PR_FALSE, aState,
                      item->mFormat.mStandalone);
    NS_ENSURE_SUCCESS(rv, rv);

    getStyleAttr(aAttributes, aAttrCount, kNameSpaceID_None,
                 txXSLTAtoms::doctypePublic, PR_FALSE, &attr);
    if (attr) {
        item->mFormat.mPublicId = attr->mValue;
    }

    getStyleAttr(aAttributes, aAttrCount, kNameSpaceID_None,
                 txXSLTAtoms::doctypeSystem, PR_FALSE, &attr);
    if (attr) {
        item->mFormat.mSystemId = attr->mValue;
    }

    getStyleAttr(aAttributes, aAttrCount, kNameSpaceID_None,
                 txXSLTAtoms::cdataSectionElements, PR_FALSE, &attr);
    if (attr) {
        txTokenizer tokens(attr->mValue);
        while (tokens.hasMoreTokens()) {
            txExpandedName* qname = new txExpandedName();
            NS_ENSURE_TRUE(qname, NS_ERROR_OUT_OF_MEMORY);

            rv = qname->init(tokens.nextToken(),
                             aState.mElementContext->mMappings, PR_FALSE);
            NS_ENSURE_SUCCESS(rv, rv);

            rv = item->mFormat.mCDATASectionElements.add(qname);
            NS_ENSURE_SUCCESS(rv, rv);
        }
    }

    rv = getYesNoAttr(aAttributes, aAttrCount,
                      txXSLTAtoms::indent, PR_FALSE, aState,
                      item->mFormat.mIndent);
    NS_ENSURE_SUCCESS(rv, rv);

    getStyleAttr(aAttributes, aAttrCount, kNameSpaceID_None,
                 txXSLTAtoms::mediaType, PR_FALSE, &attr);
    if (attr) {
        item->mFormat.mMediaType = attr->mValue;
    }

    rv = aState.addToplevelItem(item);
    NS_ENSURE_SUCCESS(rv, rv);
    
    item.forget();

    return aState.pushHandlerTable(gTxIgnoreHandler);
}

nsresult
txFnEndOutput(txStylesheetCompilerState& aState)
{
    aState.popHandlerTable();

    return NS_OK;
}


nsresult
txFnStartStripSpace(PRInt32 aNamespaceID,
                    nsIAtom* aLocalName,
                    nsIAtom* aPrefix,
                    txStylesheetAttr* aAttributes,
                    PRInt32 aAttrCount,
                    txStylesheetCompilerState& aState)
{
    txStylesheetAttr* attr = nsnull;
    nsresult rv = getStyleAttr(aAttributes, aAttrCount, kNameSpaceID_None,
                               txXSLTAtoms::elements, PR_TRUE, &attr);
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool strip = aLocalName == txXSLTAtoms::stripSpace;

    nsAutoPtr<txStripSpaceItem> stripItem(new txStripSpaceItem);
    NS_ENSURE_TRUE(stripItem, NS_ERROR_OUT_OF_MEMORY);

    txTokenizer tokenizer(attr->mValue);
    while (tokenizer.hasMoreTokens()) {
        const nsASingleFragmentString& name = tokenizer.nextToken();
        PRInt32 ns = kNameSpaceID_None;
        nsCOMPtr<nsIAtom> prefix, localName;
        rv = XMLUtils::splitQName(name, getter_AddRefs(prefix),
                                  getter_AddRefs(localName));
        if (NS_FAILED(rv)) {
            
            PRUint32 length = name.Length();
            const PRUnichar* c;
            name.BeginReading(c);
            if (length == 2 || c[length-1] != '*') {
                
                return NS_ERROR_XSLT_PARSE_FAILURE;
            }
            if (length > 1) {
                
                
                
                if (c[length-2] != ':') {
                    return NS_ERROR_XSLT_PARSE_FAILURE;
                }
                rv = XMLUtils::splitQName(StringHead(name, length - 2), 
                                          getter_AddRefs(prefix),
                                          getter_AddRefs(localName));
                if (NS_FAILED(rv) || prefix) {
                    
                    return NS_ERROR_XSLT_PARSE_FAILURE;
                }
                prefix = localName;
            }
            localName = txXPathAtoms::_asterix;
        }
        if (prefix) {
            ns = aState.mElementContext->mMappings->lookupNamespace(prefix);
            NS_ENSURE_TRUE(ns != kNameSpaceID_Unknown, NS_ERROR_FAILURE);
        }
        nsAutoPtr<txStripSpaceTest> sst(new txStripSpaceTest(prefix, localName,
                                                             ns, strip));
        NS_ENSURE_TRUE(sst, NS_ERROR_OUT_OF_MEMORY);

        rv = stripItem->addStripSpaceTest(sst);
        NS_ENSURE_SUCCESS(rv, rv);
        
        sst.forget();
    }

    rv = aState.addToplevelItem(stripItem);
    NS_ENSURE_SUCCESS(rv, rv);

    stripItem.forget();

    return aState.pushHandlerTable(gTxIgnoreHandler);
}

nsresult
txFnEndStripSpace(txStylesheetCompilerState& aState)
{
    aState.popHandlerTable();

    return NS_OK;
}


nsresult
txFnStartTemplate(PRInt32 aNamespaceID,
                  nsIAtom* aLocalName,
                  nsIAtom* aPrefix,
                  txStylesheetAttr* aAttributes,
                  PRInt32 aAttrCount,
                  txStylesheetCompilerState& aState)
{
    nsresult rv = NS_OK;
    txExpandedName name;
    rv = getQNameAttr(aAttributes, aAttrCount, txXSLTAtoms::name, PR_FALSE,
                      aState, name);
    NS_ENSURE_SUCCESS(rv, rv);

    txExpandedName mode;
    rv = getQNameAttr(aAttributes, aAttrCount, txXSLTAtoms::mode, PR_FALSE,
                      aState, mode);
    NS_ENSURE_SUCCESS(rv, rv);

    double prio = Double::NaN;
    rv = getNumberAttr(aAttributes, aAttrCount, txXSLTAtoms::priority,
                       PR_FALSE, aState, prio);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoPtr<txPattern> match;
    rv = getPatternAttr(aAttributes, aAttrCount, txXSLTAtoms::match,
                        name.isNull(), aState, match);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoPtr<txTemplateItem> templ(new txTemplateItem(match, name, mode, prio));
    NS_ENSURE_TRUE(templ, NS_ERROR_OUT_OF_MEMORY);

    aState.openInstructionContainer(templ);
    rv = aState.addToplevelItem(templ);
    NS_ENSURE_SUCCESS(rv, rv);
    
    templ.forget();

    return aState.pushHandlerTable(gTxParamHandler);
}

nsresult
txFnEndTemplate(txStylesheetCompilerState& aState)
{
    aState.popHandlerTable();

    nsAutoPtr<txInstruction> instr(new txReturn());
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

    nsresult rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    aState.closeInstructionContainer();

    return NS_OK;
}


nsresult
txFnStartTopVariable(PRInt32 aNamespaceID,
                     nsIAtom* aLocalName,
                     nsIAtom* aPrefix,
                     txStylesheetAttr* aAttributes,
                     PRInt32 aAttrCount,
                     txStylesheetCompilerState& aState)
{
    nsresult rv = NS_OK;
    txExpandedName name;
    rv = getQNameAttr(aAttributes, aAttrCount, txXSLTAtoms::name, PR_TRUE,
                      aState, name);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoPtr<Expr> select;
    rv = getExprAttr(aAttributes, aAttrCount, txXSLTAtoms::select, PR_FALSE,
                     aState, select);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoPtr<txVariableItem> var(
        new txVariableItem(name, select, aLocalName == txXSLTAtoms::param));
    NS_ENSURE_TRUE(var, NS_ERROR_OUT_OF_MEMORY);

    aState.openInstructionContainer(var);
    rv = aState.pushPtr(var);
    NS_ENSURE_SUCCESS(rv, rv);

    if (var->mValue) {
        
        rv = aState.pushHandlerTable(gTxIgnoreHandler);
        NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
        rv = aState.pushHandlerTable(gTxTopVariableHandler);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    rv = aState.addToplevelItem(var);
    NS_ENSURE_SUCCESS(rv, rv);
    
    var.forget();

    return NS_OK;
}

nsresult
txFnEndTopVariable(txStylesheetCompilerState& aState)
{
    txHandlerTable* prev = aState.mHandlerTable;
    aState.popHandlerTable();
    txVariableItem* var = static_cast<txVariableItem*>(aState.popPtr());

    if (prev == gTxTopVariableHandler) {
        
        NS_ASSERTION(!var->mValue,
                     "There shouldn't be a select-expression here");
        var->mValue = new txLiteralExpr(EmptyString());
        NS_ENSURE_TRUE(var->mValue, NS_ERROR_OUT_OF_MEMORY);
    }
    else if (!var->mValue) {
        
        nsAutoPtr<txInstruction> instr(new txReturn());
        NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

        nsresult rv = aState.addInstruction(instr);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    aState.closeInstructionContainer();

    return NS_OK;
}

nsresult
txFnStartElementStartTopVar(PRInt32 aNamespaceID,
                            nsIAtom* aLocalName,
                            nsIAtom* aPrefix,
                            txStylesheetAttr* aAttributes,
                            PRInt32 aAttrCount,
                            txStylesheetCompilerState& aState)
{
    aState.mHandlerTable = gTxTemplateHandler;

    return NS_XSLT_GET_NEW_HANDLER;
}

nsresult
txFnTextStartTopVar(const nsAString& aStr, txStylesheetCompilerState& aState)
{
    TX_RETURN_IF_WHITESPACE(aStr, aState);

    aState.mHandlerTable = gTxTemplateHandler;

    return NS_XSLT_GET_NEW_HANDLER;
}














nsresult
txFnStartLRE(PRInt32 aNamespaceID,
             nsIAtom* aLocalName,
             nsIAtom* aPrefix,
             txStylesheetAttr* aAttributes,
             PRInt32 aAttrCount,
             txStylesheetCompilerState& aState)
{
    nsresult rv = NS_OK;

    nsAutoPtr<txInstruction> instr(new txStartLREElement(aNamespaceID,
                                                         aLocalName, aPrefix));
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

    rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);
    
    rv = parseExcludeResultPrefixes(aAttributes, aAttrCount, kNameSpaceID_XSLT);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = parseUseAttrSets(aAttributes, aAttrCount, PR_TRUE, aState);
    NS_ENSURE_SUCCESS(rv, rv);

    txStylesheetAttr* attr = nsnull;
    PRInt32 i;
    for (i = 0; i < aAttrCount; ++i) {
        attr = aAttributes + i;
        
        if (attr->mNamespaceID == kNameSpaceID_XSLT) {
            continue;
        }

        nsAutoPtr<Expr> avt;
        rv = txExprParser::createAVT(attr->mValue, &aState,
                                     getter_Transfers(avt));
        NS_ENSURE_SUCCESS(rv, rv);

        instr = new txLREAttribute(attr->mNamespaceID, attr->mLocalName,
                                   attr->mPrefix, avt);
        NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);
        
        rv = aState.addInstruction(instr);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    return NS_OK;
}

nsresult
txFnEndLRE(txStylesheetCompilerState& aState)
{
    nsAutoPtr<txInstruction> instr(new txEndElement);
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

    nsresult rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}






nsresult
txFnText(const nsAString& aStr, txStylesheetCompilerState& aState)
{
    TX_RETURN_IF_WHITESPACE(aStr, aState);

    nsAutoPtr<txInstruction> instr(new txText(aStr, MB_FALSE));
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

    nsresult rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}







nsresult
txFnStartApplyImports(PRInt32 aNamespaceID,
                      nsIAtom* aLocalName,
                      nsIAtom* aPrefix,
                      txStylesheetAttr* aAttributes,
                      PRInt32 aAttrCount,
                      txStylesheetCompilerState& aState)
{
    nsresult rv = NS_OK;

    nsAutoPtr<txInstruction> instr(new txApplyImportsStart);
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

    rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    instr = new txApplyImportsEnd;
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

    rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    return aState.pushHandlerTable(gTxIgnoreHandler);
}

nsresult
txFnEndApplyImports(txStylesheetCompilerState& aState)
{
    aState.popHandlerTable();

    return NS_OK;
}











nsresult
txFnStartApplyTemplates(PRInt32 aNamespaceID,
                        nsIAtom* aLocalName,
                        nsIAtom* aPrefix,
                        txStylesheetAttr* aAttributes,
                        PRInt32 aAttrCount,
                        txStylesheetCompilerState& aState)
{
    nsresult rv = NS_OK;

    nsAutoPtr<txInstruction> instr(new txPushParams);
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

    rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    txExpandedName mode;
    rv = getQNameAttr(aAttributes, aAttrCount, txXSLTAtoms::mode, PR_FALSE,
                      aState, mode);
    NS_ENSURE_SUCCESS(rv, rv);

    instr = new txApplyTemplates(mode);
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

    rv = aState.pushObject(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    instr.forget();

    nsAutoPtr<Expr> select;
    rv = getExprAttr(aAttributes, aAttrCount, txXSLTAtoms::select, PR_FALSE,
                     aState, select);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!select) {
        nsAutoPtr<txNodeTest> nt(
            new txNodeTypeTest(txNodeTypeTest::NODE_TYPE));
        NS_ENSURE_TRUE(nt, NS_ERROR_OUT_OF_MEMORY);

        select = new LocationStep(nt, LocationStep::CHILD_AXIS);
        NS_ENSURE_TRUE(select, NS_ERROR_OUT_OF_MEMORY);

        nt.forget();
    }

    nsAutoPtr<txPushNewContext> pushcontext(new txPushNewContext(select));
    NS_ENSURE_TRUE(pushcontext, NS_ERROR_OUT_OF_MEMORY);

    rv = aState.pushSorter(pushcontext);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = aState.pushObject(pushcontext);
    NS_ENSURE_SUCCESS(rv, rv);
    
    pushcontext.forget();

    return aState.pushHandlerTable(gTxApplyTemplatesHandler);
}

nsresult
txFnEndApplyTemplates(txStylesheetCompilerState& aState)
{
    aState.popHandlerTable();

    txPushNewContext* pushcontext =
        static_cast<txPushNewContext*>(aState.popObject());
    nsAutoPtr<txInstruction> instr(pushcontext); 
    nsresult rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    aState.popSorter();

    instr = static_cast<txInstruction*>(aState.popObject()); 
    nsAutoPtr<txLoopNodeSet> loop(new txLoopNodeSet(instr));
    NS_ENSURE_TRUE(loop, NS_ERROR_OUT_OF_MEMORY);

    rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    instr = loop.forget();
    rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    instr = new txPopParams;
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);
    
    pushcontext->mBailTarget = instr;
    rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}








nsresult
txFnStartAttribute(PRInt32 aNamespaceID,
                   nsIAtom* aLocalName,
                   nsIAtom* aPrefix,
                   txStylesheetAttr* aAttributes,
                   PRInt32 aAttrCount,
                   txStylesheetCompilerState& aState)
{
    nsresult rv = NS_OK;

    nsAutoPtr<txInstruction> instr(new txPushStringHandler(PR_TRUE));
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

    rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoPtr<Expr> name;
    rv = getAVTAttr(aAttributes, aAttrCount, txXSLTAtoms::name, PR_TRUE,
                    aState, name);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoPtr<Expr> nspace;
    rv = getAVTAttr(aAttributes, aAttrCount, txXSLTAtoms::_namespace, PR_FALSE,
                    aState, nspace);
    NS_ENSURE_SUCCESS(rv, rv);

    instr = new txAttribute(name, nspace, aState.mElementContext->mMappings);
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

    rv = aState.pushObject(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    instr.forget();

    
    
    return aState.pushHandlerTable(gTxTemplateHandler);
}

nsresult
txFnEndAttribute(txStylesheetCompilerState& aState)
{
    aState.popHandlerTable();
    nsAutoPtr<txInstruction> instr(static_cast<txInstruction*>
                                              (aState.popObject()));
    nsresult rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}









nsresult
txFnStartCallTemplate(PRInt32 aNamespaceID,
                      nsIAtom* aLocalName,
                      nsIAtom* aPrefix,
                      txStylesheetAttr* aAttributes,
                      PRInt32 aAttrCount,
                      txStylesheetCompilerState& aState)
{
    nsresult rv = NS_OK;

    nsAutoPtr<txInstruction> instr(new txPushParams);
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

    rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    txExpandedName name;
    rv = getQNameAttr(aAttributes, aAttrCount, txXSLTAtoms::name, PR_TRUE,
                      aState, name);
    NS_ENSURE_SUCCESS(rv, rv);

    instr = new txCallTemplate(name);
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

    rv = aState.pushObject(instr);
    NS_ENSURE_SUCCESS(rv, rv);
    
    instr.forget();

    return aState.pushHandlerTable(gTxCallTemplateHandler);
}

nsresult
txFnEndCallTemplate(txStylesheetCompilerState& aState)
{
    aState.popHandlerTable();

    
    nsAutoPtr<txInstruction> instr(static_cast<txInstruction*>(aState.popObject()));
    nsresult rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    instr = new txPopParams;
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

    rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}















nsresult
txFnStartChoose(PRInt32 aNamespaceID,
                 nsIAtom* aLocalName,
                 nsIAtom* aPrefix,
                 txStylesheetAttr* aAttributes,
                 PRInt32 aAttrCount,
                 txStylesheetCompilerState& aState)
{
    nsresult rv = aState.pushChooseGotoList();
    NS_ENSURE_SUCCESS(rv, rv);
    
    return aState.pushHandlerTable(gTxChooseHandler);
}

nsresult
txFnEndChoose(txStylesheetCompilerState& aState)
{
    nsresult rv = NS_OK;
    aState.popHandlerTable();
    txListIterator iter(aState.mChooseGotoList);
    txGoTo* gotoinstr;
    while ((gotoinstr = static_cast<txGoTo*>(iter.next()))) {
        rv = aState.addGotoTarget(&gotoinstr->mTarget);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    aState.popChooseGotoList();

    return NS_OK;
}








nsresult
txFnStartComment(PRInt32 aNamespaceID,
                 nsIAtom* aLocalName,
                 nsIAtom* aPrefix,
                 txStylesheetAttr* aAttributes,
                 PRInt32 aAttrCount,
                 txStylesheetCompilerState& aState)
{
    nsAutoPtr<txInstruction> instr(new txPushStringHandler(PR_TRUE));
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

    nsresult rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}

nsresult
txFnEndComment(txStylesheetCompilerState& aState)
{
    nsAutoPtr<txInstruction> instr(new txComment);
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

    nsresult rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}










nsresult
txFnStartCopy(PRInt32 aNamespaceID,
              nsIAtom* aLocalName,
              nsIAtom* aPrefix,
              txStylesheetAttr* aAttributes,
              PRInt32 aAttrCount,
              txStylesheetCompilerState& aState)
{
    nsAutoPtr<txCopy> copy(new txCopy);
    NS_ENSURE_TRUE(copy, NS_ERROR_OUT_OF_MEMORY);

    nsresult rv = aState.pushPtr(copy);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoPtr<txInstruction> instr(copy.forget());
    rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = parseUseAttrSets(aAttributes, aAttrCount, PR_FALSE, aState);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}

nsresult
txFnEndCopy(txStylesheetCompilerState& aState)
{
    nsAutoPtr<txInstruction> instr(new txEndElement);
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

    nsresult rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    txCopy* copy = static_cast<txCopy*>(aState.popPtr());
    rv = aState.addGotoTarget(&copy->mBailTarget);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}






nsresult
txFnStartCopyOf(PRInt32 aNamespaceID,
                nsIAtom* aLocalName,
                nsIAtom* aPrefix,
                txStylesheetAttr* aAttributes,
                PRInt32 aAttrCount,
                txStylesheetCompilerState& aState)
{
    nsresult rv = NS_OK;

    nsAutoPtr<Expr> select;
    rv = getExprAttr(aAttributes, aAttrCount, txXSLTAtoms::select, PR_TRUE,
                    aState, select);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoPtr<txInstruction> instr(new txCopyOf(select));
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);
    
    rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    return aState.pushHandlerTable(gTxIgnoreHandler);
}

nsresult
txFnEndCopyOf(txStylesheetCompilerState& aState)
{
    aState.popHandlerTable();
    return NS_OK;
}









nsresult
txFnStartElement(PRInt32 aNamespaceID,
                 nsIAtom* aLocalName,
                 nsIAtom* aPrefix,
                 txStylesheetAttr* aAttributes,
                 PRInt32 aAttrCount,
                 txStylesheetCompilerState& aState)
{
    nsresult rv = NS_OK;

    nsAutoPtr<Expr> name;
    rv = getAVTAttr(aAttributes, aAttrCount, txXSLTAtoms::name, PR_TRUE,
                    aState, name);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoPtr<Expr> nspace;
    rv = getAVTAttr(aAttributes, aAttrCount, txXSLTAtoms::_namespace, PR_FALSE,
                    aState, nspace);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoPtr<txInstruction> instr(
        new txStartElement(name, nspace, aState.mElementContext->mMappings));
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

    rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = parseUseAttrSets(aAttributes, aAttrCount, PR_FALSE, aState);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}

nsresult
txFnEndElement(txStylesheetCompilerState& aState)
{
    nsAutoPtr<txInstruction> instr(new txEndElement);
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

    nsresult rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}






nsresult
txFnStartFallback(PRInt32 aNamespaceID,
                  nsIAtom* aLocalName,
                  nsIAtom* aPrefix,
                  txStylesheetAttr* aAttributes,
                  PRInt32 aAttrCount,
                  txStylesheetCompilerState& aState)
{
    aState.mSearchingForFallback = PR_FALSE;

    return aState.pushHandlerTable(gTxTemplateHandler);
}

nsresult
txFnEndFallback(txStylesheetCompilerState& aState)
{
    aState.popHandlerTable();

    NS_ASSERTION(!aState.mSearchingForFallback,
                 "bad nesting of unknown-instruction and fallback handlers");
    return NS_OK;
}










nsresult
txFnStartForEach(PRInt32 aNamespaceID,
                 nsIAtom* aLocalName,
                 nsIAtom* aPrefix,
                 txStylesheetAttr* aAttributes,
                 PRInt32 aAttrCount,
                 txStylesheetCompilerState& aState)
{
    nsresult rv = NS_OK;

    nsAutoPtr<Expr> select;
    rv = getExprAttr(aAttributes, aAttrCount, txXSLTAtoms::select, PR_TRUE,
                     aState, select);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoPtr<txPushNewContext> pushcontext(new txPushNewContext(select));
    NS_ENSURE_TRUE(pushcontext, NS_ERROR_OUT_OF_MEMORY);

    rv = aState.pushPtr(pushcontext);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = aState.pushSorter(pushcontext);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoPtr<txInstruction> instr(pushcontext.forget());
    rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);
    
    instr = new txPushNullTemplateRule;
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

    rv = aState.pushPtr(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    return aState.pushHandlerTable(gTxForEachHandler);
}

nsresult
txFnEndForEach(txStylesheetCompilerState& aState)
{
    aState.popHandlerTable();

    
    txInstruction* pnullrule =
        static_cast<txInstruction*>(aState.popPtr());

    nsAutoPtr<txInstruction> instr(new txLoopNodeSet(pnullrule));
    nsresult rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    aState.popSorter();
    txPushNewContext* pushcontext =
        static_cast<txPushNewContext*>(aState.popPtr());
    aState.addGotoTarget(&pushcontext->mBailTarget);

    return NS_OK;
}

nsresult
txFnStartElementContinueTemplate(PRInt32 aNamespaceID,
                                nsIAtom* aLocalName,
                                nsIAtom* aPrefix,
                                txStylesheetAttr* aAttributes,
                                PRInt32 aAttrCount,
                                txStylesheetCompilerState& aState)
{
    aState.mHandlerTable = gTxTemplateHandler;

    return NS_XSLT_GET_NEW_HANDLER;
}

nsresult
txFnTextContinueTemplate(const nsAString& aStr,
                        txStylesheetCompilerState& aState)
{
    TX_RETURN_IF_WHITESPACE(aStr, aState);

    aState.mHandlerTable = gTxTemplateHandler;

    return NS_XSLT_GET_NEW_HANDLER;
}








nsresult
txFnStartIf(PRInt32 aNamespaceID,
            nsIAtom* aLocalName,
            nsIAtom* aPrefix,
            txStylesheetAttr* aAttributes,
            PRInt32 aAttrCount,
            txStylesheetCompilerState& aState)
{
    nsresult rv = NS_OK;

    nsAutoPtr<Expr> test;
    rv = getExprAttr(aAttributes, aAttrCount, txXSLTAtoms::test, PR_TRUE,
                     aState, test);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoPtr<txConditionalGoto> condGoto(new txConditionalGoto(test, nsnull));
    NS_ENSURE_TRUE(condGoto, NS_ERROR_OUT_OF_MEMORY);

    rv = aState.pushPtr(condGoto);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoPtr<txInstruction> instr(condGoto.forget());
    rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}

nsresult
txFnEndIf(txStylesheetCompilerState& aState)
{
    txConditionalGoto* condGoto =
        static_cast<txConditionalGoto*>(aState.popPtr());
    return aState.addGotoTarget(&condGoto->mTarget);
}








nsresult
txFnStartMessage(PRInt32 aNamespaceID,
                 nsIAtom* aLocalName,
                 nsIAtom* aPrefix,
                 txStylesheetAttr* aAttributes,
                 PRInt32 aAttrCount,
                 txStylesheetCompilerState& aState)
{
    nsAutoPtr<txInstruction> instr(new txPushStringHandler(PR_FALSE));
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

    nsresult rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    txThreeState term;
    rv = getYesNoAttr(aAttributes, aAttrCount, txXSLTAtoms::terminate,
                      PR_FALSE, aState, term);
    NS_ENSURE_SUCCESS(rv, rv);

    instr = new txMessage(term == eTrue);
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

    rv = aState.pushObject(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    instr.forget();

    return NS_OK;
}

nsresult
txFnEndMessage(txStylesheetCompilerState& aState)
{
    nsAutoPtr<txInstruction> instr(static_cast<txInstruction*>(aState.popObject()));
    nsresult rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}






nsresult
txFnStartNumber(PRInt32 aNamespaceID,
                nsIAtom* aLocalName,
                nsIAtom* aPrefix,
                txStylesheetAttr* aAttributes,
                PRInt32 aAttrCount,
                txStylesheetCompilerState& aState)
{
    nsresult rv = NS_OK;

    nsCOMPtr<nsIAtom> levelAtom;
    rv = getAtomAttr(aAttributes, aAttrCount, txXSLTAtoms::level, PR_FALSE,
                     aState, getter_AddRefs(levelAtom));
    NS_ENSURE_SUCCESS(rv, rv);
    
    txXSLTNumber::LevelType level = txXSLTNumber::eLevelSingle;
    if (levelAtom == txXSLTAtoms::multiple) {
        level = txXSLTNumber::eLevelMultiple;
    }
    else if (levelAtom == txXSLTAtoms::any) {
        level = txXSLTNumber::eLevelAny;
    }
    else if (levelAtom && levelAtom != txXSLTAtoms::single && !aState.fcp()) {
        return NS_ERROR_XSLT_PARSE_FAILURE;
    }
    
    nsAutoPtr<txPattern> count;
    rv = getPatternAttr(aAttributes, aAttrCount, txXSLTAtoms::count, PR_FALSE,
                        aState, count);
    NS_ENSURE_SUCCESS(rv, rv);
    
    nsAutoPtr<txPattern> from;
    rv = getPatternAttr(aAttributes, aAttrCount, txXSLTAtoms::from, PR_FALSE,
                        aState, from);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoPtr<Expr> value;
    rv = getExprAttr(aAttributes, aAttrCount, txXSLTAtoms::value, PR_FALSE,
                     aState, value);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoPtr<Expr> format;
    rv = getAVTAttr(aAttributes, aAttrCount, txXSLTAtoms::format, PR_FALSE,
                    aState, format);
    NS_ENSURE_SUCCESS(rv, rv);
    
    nsAutoPtr<Expr> lang;
    rv = getAVTAttr(aAttributes, aAttrCount, txXSLTAtoms::lang, PR_FALSE,
                      aState, lang);
    NS_ENSURE_SUCCESS(rv, rv);
    
    nsAutoPtr<Expr> letterValue;
    rv = getAVTAttr(aAttributes, aAttrCount, txXSLTAtoms::letterValue, PR_FALSE,
                    aState, letterValue);
    NS_ENSURE_SUCCESS(rv, rv);
    
    nsAutoPtr<Expr> groupingSeparator;
    rv = getAVTAttr(aAttributes, aAttrCount, txXSLTAtoms::groupingSeparator,
                    PR_FALSE, aState, groupingSeparator);
    NS_ENSURE_SUCCESS(rv, rv);
    
    nsAutoPtr<Expr> groupingSize;
    rv = getAVTAttr(aAttributes, aAttrCount, txXSLTAtoms::groupingSize,
                    PR_FALSE, aState, groupingSize);
    NS_ENSURE_SUCCESS(rv, rv);
    
    nsAutoPtr<txInstruction> instr(new txNumber(level, count, from, value,
                                                format,groupingSeparator,
                                                groupingSize));
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);
    
    rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    return aState.pushHandlerTable(gTxIgnoreHandler);
}

nsresult
txFnEndNumber(txStylesheetCompilerState& aState)
{
    aState.popHandlerTable();

    return NS_OK;
}






nsresult
txFnStartOtherwise(PRInt32 aNamespaceID,
                   nsIAtom* aLocalName,
                   nsIAtom* aPrefix,
                   txStylesheetAttr* aAttributes,
                   PRInt32 aAttrCount,
                   txStylesheetCompilerState& aState)
{
    return aState.pushHandlerTable(gTxTemplateHandler);
}

nsresult
txFnEndOtherwise(txStylesheetCompilerState& aState)
{
    aState.popHandlerTable();
    aState.mHandlerTable = gTxIgnoreHandler; 

    return NS_OK;
}










nsresult
txFnStartParam(PRInt32 aNamespaceID,
               nsIAtom* aLocalName,
               nsIAtom* aPrefix,
               txStylesheetAttr* aAttributes,
               PRInt32 aAttrCount,
               txStylesheetCompilerState& aState)
{
    nsresult rv = NS_OK;

    txExpandedName name;
    rv = getQNameAttr(aAttributes, aAttrCount, txXSLTAtoms::name, PR_TRUE,
                      aState, name);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoPtr<txCheckParam> checkParam(new txCheckParam(name));
    NS_ENSURE_SUCCESS(rv, rv);
    
    rv = aState.pushPtr(checkParam);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoPtr<txInstruction> instr(checkParam.forget());
    rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoPtr<Expr> select;
    rv = getExprAttr(aAttributes, aAttrCount, txXSLTAtoms::select, PR_FALSE,
                     aState, select);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoPtr<txSetVariable> var(new txSetVariable(name, select));
    NS_ENSURE_TRUE(var, NS_ERROR_OUT_OF_MEMORY);

    if (var->mValue) {
        
        rv = aState.pushHandlerTable(gTxIgnoreHandler);
        NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
        rv = aState.pushHandlerTable(gTxVariableHandler);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    rv = aState.pushObject(var);
    NS_ENSURE_SUCCESS(rv, rv);
    
    var.forget();

    return NS_OK;
}

nsresult
txFnEndParam(txStylesheetCompilerState& aState)
{
    nsAutoPtr<txSetVariable> var(static_cast<txSetVariable*>
                                            (aState.popObject()));
    txHandlerTable* prev = aState.mHandlerTable;
    aState.popHandlerTable();

    if (prev == gTxVariableHandler) {
        
        NS_ASSERTION(!var->mValue,
                     "There shouldn't be a select-expression here");
        var->mValue = new txLiteralExpr(EmptyString());
        NS_ENSURE_TRUE(var->mValue, NS_ERROR_OUT_OF_MEMORY);
    }

    nsresult rv = aState.addVariable(var->mName);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoPtr<txInstruction> instr(var.forget());
    rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    txCheckParam* checkParam = static_cast<txCheckParam*>(aState.popPtr());
    aState.addGotoTarget(&checkParam->mBailTarget);

    return NS_OK;
}








nsresult
txFnStartPI(PRInt32 aNamespaceID,
            nsIAtom* aLocalName,
            nsIAtom* aPrefix,
            txStylesheetAttr* aAttributes,
            PRInt32 aAttrCount,
            txStylesheetCompilerState& aState)
{
    nsAutoPtr<txInstruction> instr(new txPushStringHandler(PR_TRUE));
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

    nsresult rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoPtr<Expr> name;
    rv = getAVTAttr(aAttributes, aAttrCount, txXSLTAtoms::name, PR_TRUE,
                    aState, name);
    NS_ENSURE_SUCCESS(rv, rv);

    instr = new txProcessingInstruction(name);
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

    rv = aState.pushObject(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    instr.forget();

    return NS_OK;
}

nsresult
txFnEndPI(txStylesheetCompilerState& aState)
{
    nsAutoPtr<txInstruction> instr(static_cast<txInstruction*>
                                              (aState.popObject()));
    nsresult rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}






nsresult
txFnStartSort(PRInt32 aNamespaceID,
              nsIAtom* aLocalName,
              nsIAtom* aPrefix,
              txStylesheetAttr* aAttributes,
              PRInt32 aAttrCount,
              txStylesheetCompilerState& aState)
{
    nsresult rv = NS_OK;

    nsAutoPtr<Expr> select;
    rv = getExprAttr(aAttributes, aAttrCount, txXSLTAtoms::select, PR_FALSE,
                     aState, select);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!select) {
        nsAutoPtr<txNodeTest> nt(
              new txNodeTypeTest(txNodeTypeTest::NODE_TYPE));
        NS_ENSURE_TRUE(nt, NS_ERROR_OUT_OF_MEMORY);

        select = new LocationStep(nt, LocationStep::SELF_AXIS);
        NS_ENSURE_TRUE(select, NS_ERROR_OUT_OF_MEMORY);

        nt.forget();
    }

    nsAutoPtr<Expr> lang;
    rv = getAVTAttr(aAttributes, aAttrCount, txXSLTAtoms::lang, PR_FALSE,
                    aState, lang);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoPtr<Expr> dataType;
    rv = getAVTAttr(aAttributes, aAttrCount, txXSLTAtoms::dataType, PR_FALSE,
                    aState, dataType);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoPtr<Expr> order;
    rv = getAVTAttr(aAttributes, aAttrCount, txXSLTAtoms::order, PR_FALSE,
                    aState, order);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoPtr<Expr> caseOrder;
    rv = getAVTAttr(aAttributes, aAttrCount, txXSLTAtoms::caseOrder, PR_FALSE,
                    aState, caseOrder);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = aState.mSorter->addSort(select, lang, dataType, order, caseOrder);
    NS_ENSURE_SUCCESS(rv, rv);

    return aState.pushHandlerTable(gTxIgnoreHandler);
}

nsresult
txFnEndSort(txStylesheetCompilerState& aState)
{
    aState.popHandlerTable();

    return NS_OK;
}






nsresult
txFnStartText(PRInt32 aNamespaceID,
              nsIAtom* aLocalName,
              nsIAtom* aPrefix,
              txStylesheetAttr* aAttributes,
              PRInt32 aAttrCount,
              txStylesheetCompilerState& aState)
{
    NS_ASSERTION(!aState.mDOE, "nested d-o-e elements should not happen");

    nsresult rv = NS_OK;
    txThreeState doe;
    rv = getYesNoAttr(aAttributes, aAttrCount,
                      txXSLTAtoms::disableOutputEscaping, PR_FALSE, aState,
                      doe);
    NS_ENSURE_SUCCESS(rv, rv);

    aState.mDOE = doe == eTrue;

    return aState.pushHandlerTable(gTxTextHandler);
}

nsresult
txFnEndText(txStylesheetCompilerState& aState)
{
    aState.mDOE = MB_FALSE;
    aState.popHandlerTable();
    return NS_OK;
}

nsresult
txFnTextText(const nsAString& aStr, txStylesheetCompilerState& aState)
{
    nsAutoPtr<txInstruction> instr(new txText(aStr, aState.mDOE));
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

    nsresult rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}






nsresult
txFnStartValueOf(PRInt32 aNamespaceID,
                 nsIAtom* aLocalName,
                 nsIAtom* aPrefix,
                 txStylesheetAttr* aAttributes,
                 PRInt32 aAttrCount,
                 txStylesheetCompilerState& aState)
{
    nsresult rv = NS_OK;

    txThreeState doe;
    rv = getYesNoAttr(aAttributes, aAttrCount,
                     txXSLTAtoms::disableOutputEscaping, PR_FALSE, aState,
                     doe);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoPtr<Expr> select;
    rv = getExprAttr(aAttributes, aAttrCount, txXSLTAtoms::select, PR_TRUE,
                     aState, select);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoPtr<txInstruction> instr(new txValueOf(select, doe == eTrue));
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

    rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    return aState.pushHandlerTable(gTxIgnoreHandler);
}

nsresult
txFnEndValueOf(txStylesheetCompilerState& aState)
{
    aState.popHandlerTable();
    return NS_OK;
}








nsresult
txFnStartVariable(PRInt32 aNamespaceID,
                  nsIAtom* aLocalName,
                  nsIAtom* aPrefix,
                  txStylesheetAttr* aAttributes,
                  PRInt32 aAttrCount,
                  txStylesheetCompilerState& aState)
{
    nsresult rv = NS_OK;

    txExpandedName name;
    rv = getQNameAttr(aAttributes, aAttrCount, txXSLTAtoms::name, PR_TRUE,
                      aState, name);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoPtr<Expr> select;
    rv = getExprAttr(aAttributes, aAttrCount, txXSLTAtoms::select, PR_FALSE,
                     aState, select);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoPtr<txSetVariable> var(new txSetVariable(name, select));
    NS_ENSURE_TRUE(var, NS_ERROR_OUT_OF_MEMORY);

    if (var->mValue) {
        
        rv = aState.pushHandlerTable(gTxIgnoreHandler);
        NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
        rv = aState.pushHandlerTable(gTxVariableHandler);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    rv = aState.pushObject(var);
    NS_ENSURE_SUCCESS(rv, rv);

    var.forget();

    return NS_OK;
}

nsresult
txFnEndVariable(txStylesheetCompilerState& aState)
{
    nsAutoPtr<txSetVariable> var(static_cast<txSetVariable*>
                                            (aState.popObject()));

    txHandlerTable* prev = aState.mHandlerTable;
    aState.popHandlerTable();

    if (prev == gTxVariableHandler) {
        
        NS_ASSERTION(!var->mValue,
                     "There shouldn't be a select-expression here");
        var->mValue = new txLiteralExpr(EmptyString());
        NS_ENSURE_TRUE(var->mValue, NS_ERROR_OUT_OF_MEMORY);
    }

    nsresult rv = aState.addVariable(var->mName);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoPtr<txInstruction> instr(var.forget());
    rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}

nsresult
txFnStartElementStartRTF(PRInt32 aNamespaceID,
                         nsIAtom* aLocalName,
                         nsIAtom* aPrefix,
                         txStylesheetAttr* aAttributes,
                         PRInt32 aAttrCount,
                         txStylesheetCompilerState& aState)
{
    nsAutoPtr<txInstruction> instr(new txPushRTFHandler);
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

    nsresult rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    aState.mHandlerTable = gTxTemplateHandler;

    return NS_XSLT_GET_NEW_HANDLER;
}

nsresult
txFnTextStartRTF(const nsAString& aStr, txStylesheetCompilerState& aState)
{
    TX_RETURN_IF_WHITESPACE(aStr, aState);

    nsAutoPtr<txInstruction> instr(new txPushRTFHandler);
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

    nsresult rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    aState.mHandlerTable = gTxTemplateHandler;

    return NS_XSLT_GET_NEW_HANDLER;
}






nsresult
txFnStartWhen(PRInt32 aNamespaceID,
              nsIAtom* aLocalName,
              nsIAtom* aPrefix,
              txStylesheetAttr* aAttributes,
              PRInt32 aAttrCount,
              txStylesheetCompilerState& aState)
{
    nsresult rv = NS_OK;

    nsAutoPtr<Expr> test;
    rv = getExprAttr(aAttributes, aAttrCount, txXSLTAtoms::test, PR_TRUE,
                     aState, test);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoPtr<txConditionalGoto> condGoto(new txConditionalGoto(test, nsnull));
    NS_ENSURE_TRUE(condGoto, NS_ERROR_OUT_OF_MEMORY);

    rv = aState.pushPtr(condGoto);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoPtr<txInstruction> instr(condGoto.forget());
    rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    return aState.pushHandlerTable(gTxTemplateHandler);
}

nsresult
txFnEndWhen(txStylesheetCompilerState& aState)
{
    aState.popHandlerTable();
    nsAutoPtr<txGoTo> gotoinstr(new txGoTo(nsnull));
    NS_ENSURE_TRUE(gotoinstr, NS_ERROR_OUT_OF_MEMORY);
    
    nsresult rv = aState.mChooseGotoList->add(gotoinstr);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoPtr<txInstruction> instr(gotoinstr.forget());
    rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    txConditionalGoto* condGoto =
        static_cast<txConditionalGoto*>(aState.popPtr());
    rv = aState.addGotoTarget(&condGoto->mTarget);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}








nsresult
txFnStartWithParam(PRInt32 aNamespaceID,
                   nsIAtom* aLocalName,
                   nsIAtom* aPrefix,
                   txStylesheetAttr* aAttributes,
                   PRInt32 aAttrCount,
                   txStylesheetCompilerState& aState)
{
    nsresult rv = NS_OK;

    txExpandedName name;
    rv = getQNameAttr(aAttributes, aAttrCount, txXSLTAtoms::name, PR_TRUE,
                      aState, name);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoPtr<Expr> select;
    rv = getExprAttr(aAttributes, aAttrCount, txXSLTAtoms::select, PR_FALSE,
                     aState, select);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoPtr<txSetParam> var(new txSetParam(name, select));
    NS_ENSURE_TRUE(var, NS_ERROR_OUT_OF_MEMORY);

    if (var->mValue) {
        
        rv = aState.pushHandlerTable(gTxIgnoreHandler);
        NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
        rv = aState.pushHandlerTable(gTxVariableHandler);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    rv = aState.pushObject(var);
    NS_ENSURE_SUCCESS(rv, rv);

    var.forget();

    return NS_OK;
}

nsresult
txFnEndWithParam(txStylesheetCompilerState& aState)
{
    nsAutoPtr<txSetParam> var(static_cast<txSetParam*>(aState.popObject()));
    txHandlerTable* prev = aState.mHandlerTable;
    aState.popHandlerTable();

    if (prev == gTxVariableHandler) {
        
        NS_ASSERTION(!var->mValue,
                     "There shouldn't be a select-expression here");
        var->mValue = new txLiteralExpr(EmptyString());
        NS_ENSURE_TRUE(var->mValue, NS_ERROR_OUT_OF_MEMORY);
    }

    nsAutoPtr<txInstruction> instr(var.forget());
    nsresult rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}








nsresult
txFnStartUnknownInstruction(PRInt32 aNamespaceID,
                            nsIAtom* aLocalName,
                            nsIAtom* aPrefix,
                            txStylesheetAttr* aAttributes,
                            PRInt32 aAttrCount,
                            txStylesheetCompilerState& aState)
{
    NS_ASSERTION(!aState.mSearchingForFallback,
                 "bad nesting of unknown-instruction and fallback handlers");

    if (aNamespaceID == kNameSpaceID_XSLT && !aState.fcp()) {
        return NS_ERROR_XSLT_PARSE_FAILURE;
    }

    aState.mSearchingForFallback = PR_TRUE;

    return aState.pushHandlerTable(gTxFallbackHandler);
}

nsresult
txFnEndUnknownInstruction(txStylesheetCompilerState& aState)
{
    aState.popHandlerTable();

    if (aState.mSearchingForFallback) {
        nsAutoPtr<txInstruction> instr(new txErrorInstruction());
        NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

        nsresult rv = aState.addInstruction(instr);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    aState.mSearchingForFallback = PR_FALSE;

    return NS_OK;
}





struct txHandlerTableData {
    txElementHandler mOtherHandler;
    txElementHandler mLREHandler;
    HandleTextFn mTextHandler;
};

const txHandlerTableData gTxIgnoreTableData = {
  
  { 0, 0, txFnStartElementIgnore, txFnEndElementIgnore },
  
  { 0, 0, txFnStartElementIgnore, txFnEndElementIgnore },
  
  txFnTextIgnore
};

const txElementHandler gTxRootElementHandlers[] = {
  { kNameSpaceID_XSLT, "stylesheet", txFnStartStylesheet, txFnEndStylesheet },
  { kNameSpaceID_XSLT, "transform", txFnStartStylesheet, txFnEndStylesheet }
};

const txHandlerTableData gTxRootTableData = {
  
  { 0, 0, txFnStartElementError, txFnEndElementError },
  
  { 0, 0, txFnStartLREStylesheet, txFnEndLREStylesheet },
  
  txFnTextError
};

const txHandlerTableData gTxEmbedTableData = {
  
  { 0, 0, txFnStartEmbed, txFnEndEmbed },
  
  { 0, 0, txFnStartElementIgnore, txFnEndElementIgnore },
  
  txFnTextIgnore
};

const txElementHandler gTxTopElementHandlers[] = {
  { kNameSpaceID_XSLT, "attribute-set", txFnStartAttributeSet, txFnEndAttributeSet },
  { kNameSpaceID_XSLT, "decimal-format", txFnStartDecimalFormat, txFnEndDecimalFormat },
  { kNameSpaceID_XSLT, "include", txFnStartInclude, txFnEndInclude },
  { kNameSpaceID_XSLT, "key", txFnStartKey, txFnEndKey },
  { kNameSpaceID_XSLT, "namespace-alias", txFnStartNamespaceAlias,
    txFnEndNamespaceAlias },
  { kNameSpaceID_XSLT, "output", txFnStartOutput, txFnEndOutput },
  { kNameSpaceID_XSLT, "param", txFnStartTopVariable, txFnEndTopVariable },
  { kNameSpaceID_XSLT, "preserve-space", txFnStartStripSpace, txFnEndStripSpace },
  { kNameSpaceID_XSLT, "strip-space", txFnStartStripSpace, txFnEndStripSpace },
  { kNameSpaceID_XSLT, "template", txFnStartTemplate, txFnEndTemplate },
  { kNameSpaceID_XSLT, "variable", txFnStartTopVariable, txFnEndTopVariable }
};

const txHandlerTableData gTxTopTableData = {
  
  { 0, 0, txFnStartOtherTop, txFnEndOtherTop },
  
  { 0, 0, txFnStartOtherTop, txFnEndOtherTop },
  
  txFnTextIgnore
};

const txElementHandler gTxTemplateElementHandlers[] = {
  { kNameSpaceID_XSLT, "apply-imports", txFnStartApplyImports, txFnEndApplyImports },
  { kNameSpaceID_XSLT, "apply-templates", txFnStartApplyTemplates, txFnEndApplyTemplates },
  { kNameSpaceID_XSLT, "attribute", txFnStartAttribute, txFnEndAttribute },
  { kNameSpaceID_XSLT, "call-template", txFnStartCallTemplate, txFnEndCallTemplate },
  { kNameSpaceID_XSLT, "choose", txFnStartChoose, txFnEndChoose },
  { kNameSpaceID_XSLT, "comment", txFnStartComment, txFnEndComment },
  { kNameSpaceID_XSLT, "copy", txFnStartCopy, txFnEndCopy },
  { kNameSpaceID_XSLT, "copy-of", txFnStartCopyOf, txFnEndCopyOf },
  { kNameSpaceID_XSLT, "element", txFnStartElement, txFnEndElement },
  { kNameSpaceID_XSLT, "fallback", txFnStartElementSetIgnore, txFnEndElementSetIgnore },
  { kNameSpaceID_XSLT, "for-each", txFnStartForEach, txFnEndForEach },
  { kNameSpaceID_XSLT, "if", txFnStartIf, txFnEndIf },
  { kNameSpaceID_XSLT, "message", txFnStartMessage, txFnEndMessage },
  { kNameSpaceID_XSLT, "number", txFnStartNumber, txFnEndNumber },
  { kNameSpaceID_XSLT, "processing-instruction", txFnStartPI, txFnEndPI },
  { kNameSpaceID_XSLT, "text", txFnStartText, txFnEndText },
  { kNameSpaceID_XSLT, "value-of", txFnStartValueOf, txFnEndValueOf },
  { kNameSpaceID_XSLT, "variable", txFnStartVariable, txFnEndVariable }
};

const txHandlerTableData gTxTemplateTableData = {
  
  { 0, 0, txFnStartUnknownInstruction, txFnEndUnknownInstruction },
  
  { 0, 0, txFnStartLRE, txFnEndLRE },
  
  txFnText
};

const txHandlerTableData gTxTextTableData = {
  
  { 0, 0, txFnStartElementError, txFnEndElementError },
  
  { 0, 0, txFnStartElementError, txFnEndElementError },
  
  txFnTextText
};

const txElementHandler gTxApplyTemplatesElementHandlers[] = {
  { kNameSpaceID_XSLT, "sort", txFnStartSort, txFnEndSort },
  { kNameSpaceID_XSLT, "with-param", txFnStartWithParam, txFnEndWithParam }
};

const txHandlerTableData gTxApplyTemplatesTableData = {
  
  { 0, 0, txFnStartElementSetIgnore, txFnEndElementSetIgnore }, 
  
  { 0, 0, txFnStartElementSetIgnore, txFnEndElementSetIgnore },
  
  txFnTextIgnore
};

const txElementHandler gTxCallTemplateElementHandlers[] = {
  { kNameSpaceID_XSLT, "with-param", txFnStartWithParam, txFnEndWithParam }
};

const txHandlerTableData gTxCallTemplateTableData = {
  
  { 0, 0, txFnStartElementSetIgnore, txFnEndElementSetIgnore }, 
  
  { 0, 0, txFnStartElementSetIgnore, txFnEndElementSetIgnore },
  
  txFnTextIgnore
};

const txHandlerTableData gTxVariableTableData = {
  
  { 0, 0, txFnStartElementStartRTF, 0 },
  
  { 0, 0, txFnStartElementStartRTF, 0 },
  
  txFnTextStartRTF
};

const txElementHandler gTxForEachElementHandlers[] = {
  { kNameSpaceID_XSLT, "sort", txFnStartSort, txFnEndSort }
};

const txHandlerTableData gTxForEachTableData = {
  
  { 0, 0, txFnStartElementContinueTemplate, 0 },
  
  { 0, 0, txFnStartElementContinueTemplate, 0 },
  
  txFnTextContinueTemplate
};

const txHandlerTableData gTxTopVariableTableData = {
  
  { 0, 0, txFnStartElementStartTopVar, 0 },
  
  { 0, 0, txFnStartElementStartTopVar, 0 },
  
  txFnTextStartTopVar
};

const txElementHandler gTxChooseElementHandlers[] = {
  { kNameSpaceID_XSLT, "otherwise", txFnStartOtherwise, txFnEndOtherwise },
  { kNameSpaceID_XSLT, "when", txFnStartWhen, txFnEndWhen }
};

const txHandlerTableData gTxChooseTableData = {
  
  { 0, 0, txFnStartElementError, 0 },
  
  { 0, 0, txFnStartElementError, 0 },
  
  txFnTextError
};

const txElementHandler gTxParamElementHandlers[] = {
  { kNameSpaceID_XSLT, "param", txFnStartParam, txFnEndParam }
};

const txHandlerTableData gTxParamTableData = {
  
  { 0, 0, txFnStartElementContinueTemplate, 0 },
  
  { 0, 0, txFnStartElementContinueTemplate, 0 },
  
  txFnTextContinueTemplate
};

const txElementHandler gTxImportElementHandlers[] = {
  { kNameSpaceID_XSLT, "import", txFnStartImport, txFnEndImport }
};

const txHandlerTableData gTxImportTableData = {
  
  { 0, 0, txFnStartElementContinueTopLevel, 0 },
  
  { 0, 0, txFnStartOtherTop, txFnEndOtherTop }, 
  
  txFnTextIgnore  
};

const txElementHandler gTxAttributeSetElementHandlers[] = {
  { kNameSpaceID_XSLT, "attribute", txFnStartAttribute, txFnEndAttribute }
};

const txHandlerTableData gTxAttributeSetTableData = {
  
  { 0, 0, txFnStartElementError, 0 },
  
  { 0, 0, txFnStartElementError, 0 },
  
  txFnTextError
};

const txElementHandler gTxFallbackElementHandlers[] = {
  { kNameSpaceID_XSLT, "fallback", txFnStartFallback, txFnEndFallback }
};

const txHandlerTableData gTxFallbackTableData = {
  
  { 0, 0, txFnStartElementSetIgnore, txFnEndElementSetIgnore },
  
  { 0, 0, txFnStartElementSetIgnore, txFnEndElementSetIgnore },
  
  txFnTextIgnore
};






txHandlerTable::txHandlerTable(const HandleTextFn aTextHandler,
                               const txElementHandler* aLREHandler,
                               const txElementHandler* aOtherHandler)
  : mTextHandler(aTextHandler),
    mLREHandler(aLREHandler),
    mOtherHandler(aOtherHandler)
{
}

nsresult
txHandlerTable::init(const txElementHandler* aHandlers, PRUint32 aCount)
{
    nsresult rv = NS_OK;

    PRUint32 i;
    for (i = 0; i < aCount; ++i) {
        nsCOMPtr<nsIAtom> nameAtom = do_GetAtom(aHandlers->mLocalName);
        txExpandedName name(aHandlers->mNamespaceID, nameAtom);
        rv = mHandlers.add(name, aHandlers);
        NS_ENSURE_SUCCESS(rv, rv);

        ++aHandlers;
    }
    return NS_OK;
}

const txElementHandler*
txHandlerTable::find(PRInt32 aNamespaceID, nsIAtom* aLocalName)
{
    txExpandedName name(aNamespaceID, aLocalName);
    const txElementHandler* handler = mHandlers.get(name);
    if (!handler) {
        handler = mOtherHandler;
    }
    return handler;
}

#define INIT_HANDLER(_name)                                          \
    gTx##_name##Handler =                                            \
        new txHandlerTable(gTx##_name##TableData.mTextHandler,       \
                           &gTx##_name##TableData.mLREHandler,       \
                           &gTx##_name##TableData.mOtherHandler);    \
    if (!gTx##_name##Handler)                                        \
        return PR_FALSE

#define INIT_HANDLER_WITH_ELEMENT_HANDLERS(_name)                    \
    INIT_HANDLER(_name);                                             \
                                                                     \
    rv = gTx##_name##Handler->init(gTx##_name##ElementHandlers,      \
                                   NS_ARRAY_LENGTH(gTx##_name##ElementHandlers)); \
    if (NS_FAILED(rv))                                               \
        return PR_FALSE

#define SHUTDOWN_HANDLER(_name)                                      \
    delete gTx##_name##Handler;                                      \
    gTx##_name##Handler = nsnull


MBool
txHandlerTable::init()
{
    nsresult rv = NS_OK;

    INIT_HANDLER_WITH_ELEMENT_HANDLERS(Root);
    INIT_HANDLER(Embed);
    INIT_HANDLER_WITH_ELEMENT_HANDLERS(Top);
    INIT_HANDLER(Ignore);
    INIT_HANDLER_WITH_ELEMENT_HANDLERS(Template);
    INIT_HANDLER(Text);
    INIT_HANDLER_WITH_ELEMENT_HANDLERS(ApplyTemplates);
    INIT_HANDLER_WITH_ELEMENT_HANDLERS(CallTemplate);
    INIT_HANDLER(Variable);
    INIT_HANDLER_WITH_ELEMENT_HANDLERS(ForEach);
    INIT_HANDLER(TopVariable);
    INIT_HANDLER_WITH_ELEMENT_HANDLERS(Choose);
    INIT_HANDLER_WITH_ELEMENT_HANDLERS(Param);
    INIT_HANDLER_WITH_ELEMENT_HANDLERS(Import);
    INIT_HANDLER_WITH_ELEMENT_HANDLERS(AttributeSet);
    INIT_HANDLER_WITH_ELEMENT_HANDLERS(Fallback);

    return MB_TRUE;
}


void
txHandlerTable::shutdown()
{
    SHUTDOWN_HANDLER(Root);
    SHUTDOWN_HANDLER(Embed);
    SHUTDOWN_HANDLER(Top);
    SHUTDOWN_HANDLER(Ignore);
    SHUTDOWN_HANDLER(Template);
    SHUTDOWN_HANDLER(Text);
    SHUTDOWN_HANDLER(ApplyTemplates);
    SHUTDOWN_HANDLER(CallTemplate);
    SHUTDOWN_HANDLER(Variable);
    SHUTDOWN_HANDLER(ForEach);
    SHUTDOWN_HANDLER(TopVariable);
    SHUTDOWN_HANDLER(Choose);
    SHUTDOWN_HANDLER(Param);
    SHUTDOWN_HANDLER(Import);
    SHUTDOWN_HANDLER(AttributeSet);
    SHUTDOWN_HANDLER(Fallback);
}
