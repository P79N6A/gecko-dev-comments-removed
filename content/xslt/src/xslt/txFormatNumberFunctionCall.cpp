





































#include "txXSLTFunctions.h"
#include "txAtoms.h"
#include "txIXPathContext.h"
#include "txStylesheet.h"
#include <math.h>
#include "txNamespaceMap.h"

#include "prdtoa.h"

#define INVALID_PARAM_VALUE \
    NS_LITERAL_STRING("invalid parameter value for function")

const PRUnichar txFormatNumberFunctionCall::FORMAT_QUOTE = '\'';









txFormatNumberFunctionCall::txFormatNumberFunctionCall(txStylesheet* aStylesheet,
                                                       txNamespaceMap* aMappings)
    : mStylesheet(aStylesheet),
      mMappings(aMappings)
{
}








nsresult
txFormatNumberFunctionCall::evaluate(txIEvalContext* aContext,
                                     txAExprResult** aResult)
{
    *aResult = nsnull;
    if (!requireParams(2, 3, aContext))
        return NS_ERROR_XPATH_BAD_ARGUMENT_COUNT;

    
    double value;
    txExpandedName formatName;

    nsresult rv = evaluateToNumber(mParams[0], aContext, &value);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoString formatStr;
    rv = mParams[1]->evaluateToString(aContext, formatStr);
    NS_ENSURE_SUCCESS(rv, rv);

    if (mParams.Length() == 3) {
        nsAutoString formatQName;
        rv = mParams[2]->evaluateToString(aContext, formatQName);
        NS_ENSURE_SUCCESS(rv, rv);

        rv = formatName.init(formatQName, mMappings, MB_FALSE);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    txDecimalFormat* format = mStylesheet->getDecimalFormat(formatName);
    if (!format) {
        nsAutoString err(NS_LITERAL_STRING("unknown decimal format"));
#ifdef TX_TO_STRING
        err.AppendLiteral(" for: ");
        toString(err);
#endif
        aContext->receiveError(err, NS_ERROR_XPATH_INVALID_ARG);
        return NS_ERROR_XPATH_INVALID_ARG;
    }

    
    if (Double::isNaN(value)) {
        return aContext->recycler()->getStringResult(format->mNaN, aResult);
    }

    if (value == Double::POSITIVE_INFINITY) {
        return aContext->recycler()->getStringResult(format->mInfinity,
                                                     aResult);
    }

    if (value == Double::NEGATIVE_INFINITY) {
        nsAutoString res;
        res.Append(format->mMinusSign);
        res.Append(format->mInfinity);
        return aContext->recycler()->getStringResult(res, aResult);
    }
    
    
    nsAutoString prefix;
    nsAutoString suffix;
    int minIntegerSize=0;
    int minFractionSize=0;
    int maxFractionSize=0;
    int multiplier=1;
    int groupSize=-1;

    PRUint32 pos = 0;
    PRUint32 formatLen = formatStr.Length();
    MBool inQuote;

    
    inQuote = MB_FALSE;
    if (Double::isNeg(value)) {
        while (pos < formatLen &&
               (inQuote ||
                formatStr.CharAt(pos) != format->mPatternSeparator)) {
            if (formatStr.CharAt(pos) == FORMAT_QUOTE)
                inQuote = !inQuote;
            pos++;
        }

        if (pos == formatLen) {
            pos = 0;
            prefix.Append(format->mMinusSign);
        }
        else
            pos++;
    }

    
    FormatParseState pState = Prefix;
    inQuote = MB_FALSE;

    PRUnichar c = 0;
    while (pos < formatLen && pState != Finished) {
        c=formatStr.CharAt(pos++);

        switch (pState) {

        case Prefix:
        case Suffix:
            if (!inQuote) {
                if (c == format->mPercent) {
                    if (multiplier == 1)
                        multiplier = 100;
                    else {
                        nsAutoString err(INVALID_PARAM_VALUE);
#ifdef TX_TO_STRING
                        err.AppendLiteral(": ");
                        toString(err);
#endif
                        aContext->receiveError(err,
                                               NS_ERROR_XPATH_INVALID_ARG);
                        return NS_ERROR_XPATH_INVALID_ARG;
                    }
                }
                else if (c == format->mPerMille) {
                    if (multiplier == 1)
                        multiplier = 1000;
                    else {
                        nsAutoString err(INVALID_PARAM_VALUE);
#ifdef TX_TO_STRING
                        err.AppendLiteral(": ");
                        toString(err);
#endif
                        aContext->receiveError(err,
                                               NS_ERROR_XPATH_INVALID_ARG);
                        return NS_ERROR_XPATH_INVALID_ARG;
                    }
                }
                else if (c == format->mDecimalSeparator ||
                         c == format->mGroupingSeparator ||
                         c == format->mZeroDigit ||
                         c == format->mDigit ||
                         c == format->mPatternSeparator) {
                    pState = pState == Prefix ? IntDigit : Finished;
                    pos--;
                    break;
                }
            }

            if (c == FORMAT_QUOTE)
                inQuote = !inQuote;
            else if (pState == Prefix)
                prefix.Append(c);
            else
                suffix.Append(c);
            break;

        case IntDigit:
            if (c == format->mGroupingSeparator)
                groupSize=0;
            else if (c == format->mDigit) {
                if (groupSize >= 0)
                    groupSize++;
            }
            else {
                pState = IntZero;
                pos--;
            }
            break;

        case IntZero:
            if (c == format->mGroupingSeparator)
                groupSize = 0;
            else if (c == format->mZeroDigit) {
                if (groupSize >= 0)
                    groupSize++;
                minIntegerSize++;
            }
            else if (c == format->mDecimalSeparator) {
                pState = FracZero;
            }
            else {
                pState = Suffix;
                pos--;
            }
            break;

        case FracZero:
            if (c == format->mZeroDigit) {
                maxFractionSize++;
                minFractionSize++;
            }
            else {
                pState = FracDigit;
                pos--;
            }
            break;

        case FracDigit:
            if (c == format->mDigit)
                maxFractionSize++;
            else {
                pState = Suffix;
                pos--;
            }
            break;

        case Finished:
            break;
        }
    }

    
    if ((c != format->mPatternSeparator && pos < formatLen) ||
        inQuote ||
        groupSize == 0) {
        nsAutoString err(INVALID_PARAM_VALUE);
#ifdef TX_TO_STRING
        err.AppendLiteral(": ");
        toString(err);
#endif
        aContext->receiveError(err, NS_ERROR_XPATH_INVALID_ARG);
        return NS_ERROR_XPATH_INVALID_ARG;
    }


    




    value = fabs(value) * multiplier;

    
    nsAutoString res(prefix);

    int bufsize;
    if (value > 1)
        bufsize = (int)log10(value) + 30;
    else
        bufsize = 1 + 30;

    char* buf = new char[bufsize];
    NS_ENSURE_TRUE(buf, NS_ERROR_OUT_OF_MEMORY);

    PRIntn bufIntDigits, sign;
    char* endp;
    PR_dtoa(value, 0, 0, &bufIntDigits, &sign, &endp, buf, bufsize-1);

    int buflen = endp - buf;
    int intDigits;
    intDigits = bufIntDigits > minIntegerSize ? bufIntDigits : minIntegerSize;

    if (groupSize < 0)
        groupSize = intDigits + 10; 

    
    res.SetLength(res.Length() +
                  intDigits +               
                  1 +                       
                  maxFractionSize +         
                  (intDigits-1)/groupSize); 

    PRInt32 i = bufIntDigits + maxFractionSize - 1;
    MBool carry = (i+1 < buflen) && (buf[i+1] >= '5');
    MBool hasFraction = MB_FALSE;

    PRUint32 resPos = res.Length()-1;

    
    for (; i >= bufIntDigits; --i) {
        int digit;
        if (i >= buflen || i < 0) {
            digit = 0;
        }
        else {
            digit = buf[i] - '0';
        }
        
        if (carry) {
            digit = (digit + 1) % 10;
            carry = digit == 0;
        }

        if (hasFraction || digit != 0 || i < bufIntDigits+minFractionSize) {
            hasFraction = MB_TRUE;
            res.SetCharAt((PRUnichar)(digit + format->mZeroDigit),
                          resPos--);
        }
        else {
            res.Truncate(resPos--);
        }
    }

    
    if (hasFraction) {
        res.SetCharAt(format->mDecimalSeparator, resPos--);
    }
    else {
        res.Truncate(resPos--);
    }

    
    for (i = 0; i < intDigits; ++i) {
        int digit;
        if (bufIntDigits-i-1 >= buflen || bufIntDigits-i-1 < 0) {
            digit = 0;
        }
        else {
            digit = buf[bufIntDigits-i-1] - '0';
        }
        
        if (carry) {
            digit = (digit + 1) % 10;
            carry = digit == 0;
        }

        if (i != 0 && i%groupSize == 0) {
            res.SetCharAt(format->mGroupingSeparator, resPos--);
        }

        res.SetCharAt((PRUnichar)(digit + format->mZeroDigit), resPos--);
    }

    if (carry) {
        if (i%groupSize == 0) {
            res.Insert(format->mGroupingSeparator, resPos + 1);
        }
        res.Insert((PRUnichar)(1 + format->mZeroDigit), resPos + 1);
    }
    
    if (!hasFraction && !intDigits && !carry) {
        
        
        res.Append(format->mZeroDigit);
    }

    delete [] buf;

    
    res.Append(suffix);

    return aContext->recycler()->getStringResult(res, aResult);
} 

Expr::ResultType
txFormatNumberFunctionCall::getReturnType()
{
    return STRING_RESULT;
}

PRBool
txFormatNumberFunctionCall::isSensitiveTo(ContextSensitivity aContext)
{
    return argsSensitiveTo(aContext);
}

#ifdef TX_TO_STRING
nsresult
txFormatNumberFunctionCall::getNameAtom(nsIAtom** aAtom)
{
    *aAtom = txXSLTAtoms::formatNumber;
    NS_ADDREF(*aAtom);
    return NS_OK;
}
#endif






txDecimalFormat::txDecimalFormat() : mInfinity(NS_LITERAL_STRING("Infinity")),
                                     mNaN(NS_LITERAL_STRING("NaN"))
{
    mDecimalSeparator = '.';
    mGroupingSeparator = ',';
    mMinusSign = '-';
    mPercent = '%';
    mPerMille = 0x2030;
    mZeroDigit = '0';
    mDigit = '#';
    mPatternSeparator = ';';
}

MBool txDecimalFormat::isEqual(txDecimalFormat* other)
{
    return mDecimalSeparator == other->mDecimalSeparator &&
           mGroupingSeparator == other->mGroupingSeparator &&
           mInfinity.Equals(other->mInfinity) &&
           mMinusSign == other->mMinusSign &&
           mNaN.Equals(other->mNaN) &&
           mPercent == other->mPercent &&
           mPerMille == other->mPerMille &&
           mZeroDigit == other->mZeroDigit &&
           mDigit == other->mDigit &&
           mPatternSeparator  == other->mPatternSeparator;
}
