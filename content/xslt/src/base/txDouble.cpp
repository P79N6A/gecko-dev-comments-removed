




































#include "nsString.h"
#include "txCore.h"
#include "txXMLUtils.h"
#include <math.h>
#include <stdlib.h>
#ifdef WIN32
#include <float.h>
#endif
#include "prdtoa.h"






const dpun Double::NaN = DOUBLE_NaN;
#ifdef IS_BIG_ENDIAN
const dpun Double::POSITIVE_INFINITY = {{DOUBLE_HI32_EXPMASK, 0}};
const dpun Double::NEGATIVE_INFINITY = {{DOUBLE_HI32_EXPMASK | DOUBLE_HI32_SIGNBIT, 0}};
#else
const dpun Double::POSITIVE_INFINITY = {{0, DOUBLE_HI32_EXPMASK}};
const dpun Double::NEGATIVE_INFINITY = {{0, DOUBLE_HI32_EXPMASK | DOUBLE_HI32_SIGNBIT}};
#endif





MBool Double::isInfinite(double aDbl)
{
    return ((DOUBLE_HI32(aDbl) & ~DOUBLE_HI32_SIGNBIT) == DOUBLE_HI32_EXPMASK &&
            !DOUBLE_LO32(aDbl));
}




MBool Double::isNaN(double aDbl)
{
    return DOUBLE_IS_NaN(aDbl);
}




MBool Double::isNeg(double aDbl)
{
    return (DOUBLE_HI32(aDbl) & DOUBLE_HI32_SIGNBIT) != 0;
}





class txStringToDouble
{
public:
    typedef PRUnichar input_type;
    typedef PRUnichar value_type;
    txStringToDouble(): mState(eWhitestart), mSign(ePositive) {}

    void
    write(const input_type* aSource, PRUint32 aSourceLength)
    {
        if (mState == eIllegal) {
            return;
        }
        PRUint32 i = 0;
        PRUnichar c;
        for ( ; i < aSourceLength; ++i) {
            c = aSource[i];
            switch (mState) {
                case eWhitestart:
                    if (c == '-') {
                        mState = eDecimal;
                        mSign = eNegative;
                    }
                    else if (c >= '0' && c <= '9') {
                        mState = eDecimal;
                        mBuffer.Append((char)c);
                    }
                    else if (c == '.') {
                        mState = eMantissa;
                        mBuffer.Append((char)c);
                    }
                    else if (!XMLUtils::isWhitespace(c)) {
                        mState = eIllegal;
                        return;
                    }
                    break;
                case eDecimal:
                    if (c >= '0' && c <= '9') {
                        mBuffer.Append((char)c);
                    }
                    else if (c == '.') {
                        mState = eMantissa;
                        mBuffer.Append((char)c);
                    }
                    else if (XMLUtils::isWhitespace(c)) {
                        mState = eWhiteend;
                    }
                    else {
                        mState = eIllegal;
                        return;
                    }
                    break;
                case eMantissa:
                    if (c >= '0' && c <= '9') {
                        mBuffer.Append((char)c);
                    }
                    else if (XMLUtils::isWhitespace(c)) {
                        mState = eWhiteend;
                    }
                    else {
                        mState = eIllegal;
                        return;
                    }
                    break;
                case eWhiteend:
                    if (!XMLUtils::isWhitespace(c)) {
                        mState = eIllegal;
                        return;
                    }
                    break;
                default:
                    break;
            }
        }
    }

    double
    getDouble()
    {
        if (mState == eIllegal || mBuffer.IsEmpty() ||
            (mBuffer.Length() == 1 && mBuffer[0] == '.')) {
            return Double::NaN;
        }
        return mSign*PR_strtod(mBuffer.get(), 0);
    }
private:
    nsCAutoString mBuffer;
    enum {
        eWhitestart,
        eDecimal,
        eMantissa,
        eWhiteend,
        eIllegal
    } mState;
    enum {
        eNegative = -1,
        ePositive = 1
    } mSign;
};

double Double::toDouble(const nsAString& aSrc)
{
    txStringToDouble sink;
    nsAString::const_iterator fromBegin, fromEnd;
    copy_string(aSrc.BeginReading(fromBegin), aSrc.EndReading(fromEnd), sink);
    return sink.getDouble();
}






void Double::toString(double aValue, nsAString& aDest)
{

    

    if (isNaN(aValue)) {
        aDest.AppendLiteral("NaN");
        return;
    }
    if (isInfinite(aValue)) {
        if (aValue < 0)
            aDest.Append(PRUnichar('-'));
        aDest.AppendLiteral("Infinity");
        return;
    }

    
    const int buflen = 20;
    char buf[buflen];

    PRIntn intDigits, sign;
    char* endp;
    PR_dtoa(aValue, 0, 0, &intDigits, &sign, &endp, buf, buflen - 1);

    
    PRInt32 length = endp - buf;
    if (length > intDigits) {
        
        ++length;
        if (intDigits < 1) {
            
            length += 1 - intDigits;
        }
    }
    else {
        
        length = intDigits;
    }
    if (aValue < 0)
        ++length;
    
    PRUint32 oldlength = aDest.Length();
    if (!EnsureStringLength(aDest, oldlength + length))
        return; 
    nsAString::iterator dest;
    aDest.BeginWriting(dest).advance(PRInt32(oldlength));
    if (aValue < 0) {
        *dest = '-'; ++dest;
    }
    int i;
    
    if (intDigits < 1) {
        *dest = '0'; ++dest;
        *dest = '.'; ++dest;
        for (i = 0; i > intDigits; --i) {
            *dest = '0'; ++dest;
        }
    }
    
    int firstlen = NS_MIN<size_t>(intDigits, endp - buf);
    for (i = 0; i < firstlen; i++) {
        *dest = buf[i]; ++dest;
    }
    if (i < endp - buf) {
        if (i > 0) {
            *dest = '.'; ++dest;
        }
        for (; i < endp - buf; i++) {
            *dest = buf[i]; ++dest;
        }
    }
    
    for (; i < intDigits; i++) {
        *dest = '0'; ++dest;
    }
}
