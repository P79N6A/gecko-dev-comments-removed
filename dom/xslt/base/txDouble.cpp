




#include "mozilla/FloatingPoint.h"

#include "nsString.h"
#include "txCore.h"
#include "txXMLUtils.h"
#include <math.h>
#include <stdlib.h>
#include <algorithm>
#ifdef WIN32
#include <float.h>
#endif
#include "prdtoa.h"









class txStringToDouble
{
public:
    typedef char16_t input_type;
    typedef char16_t value_type;
    txStringToDouble(): mState(eWhitestart), mSign(ePositive) {}

    void
    write(const input_type* aSource, uint32_t aSourceLength)
    {
        if (mState == eIllegal) {
            return;
        }
        uint32_t i = 0;
        char16_t c;
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
            return mozilla::UnspecifiedNaN<double>();
        }
        return mSign*PR_strtod(mBuffer.get(), 0);
    }
private:
    nsAutoCString mBuffer;
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

double txDouble::toDouble(const nsAString& aSrc)
{
    txStringToDouble sink;
    nsAString::const_iterator fromBegin, fromEnd;
    copy_string(aSrc.BeginReading(fromBegin), aSrc.EndReading(fromEnd), sink);
    return sink.getDouble();
}






void txDouble::toString(double aValue, nsAString& aDest)
{

    

    if (mozilla::IsNaN(aValue)) {
        aDest.AppendLiteral("NaN");
        return;
    }
    if (mozilla::IsInfinite(aValue)) {
        if (aValue < 0)
            aDest.Append(char16_t('-'));
        aDest.AppendLiteral("Infinity");
        return;
    }

    
    const int buflen = 20;
    char buf[buflen];

    int intDigits, sign;
    char* endp;
    PR_dtoa(aValue, 0, 0, &intDigits, &sign, &endp, buf, buflen - 1);

    
    int32_t length = endp - buf;
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
    
    uint32_t oldlength = aDest.Length();
    if (!aDest.SetLength(oldlength + length, mozilla::fallible))
        return; 
    nsAString::iterator dest;
    aDest.BeginWriting(dest).advance(int32_t(oldlength));
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
    
    int firstlen = std::min<size_t>(intDigits, endp - buf);
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
