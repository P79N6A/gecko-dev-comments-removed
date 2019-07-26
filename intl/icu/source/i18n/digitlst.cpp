

























#include "digitlst.h"

#if !UCONFIG_NO_FORMATTING
#include "unicode/putil.h"
#include "charstr.h"
#include "cmemory.h"
#include "cstring.h"
#include "mutex.h"
#include "putilimp.h"
#include "uassert.h"
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <limits>











#define kZero '0'







static const uint8_t DIGIT_HAVE_NONE=0;
static const uint8_t DIGIT_HAVE_DOUBLE=1;
static const uint8_t DIGIT_HAVE_INT64=2;

U_NAMESPACE_BEGIN




DigitList::DigitList()
{
    uprv_decContextDefault(&fContext, DEC_INIT_BASE);
    fContext.traps  = 0;
    uprv_decContextSetRounding(&fContext, DEC_ROUND_HALF_EVEN);
    fContext.digits = fStorage.getCapacity();

    fDecNumber = fStorage.getAlias();
    uprv_decNumberZero(fDecNumber);

    internalSetDouble(0.0);
}



DigitList::~DigitList()
{
}




DigitList::DigitList(const DigitList &other)
{
    fDecNumber = fStorage.getAlias();
    *this = other;
}





DigitList&
DigitList::operator=(const DigitList& other)
{
    if (this != &other)
    {
        uprv_memcpy(&fContext, &other.fContext, sizeof(decContext));

        if (other.fStorage.getCapacity() > fStorage.getCapacity()) {
            fDecNumber = fStorage.resize(other.fStorage.getCapacity());
        }
        
        
        fContext.digits = fStorage.getCapacity();
        uprv_decNumberCopy(fDecNumber, other.fDecNumber);

        {
            
            
            
            Mutex mutex;

            if(other.fHave==kDouble) {
              fUnion.fDouble = other.fUnion.fDouble;
            } else if(other.fHave==kInt64) {
              fUnion.fInt64 = other.fUnion.fInt64;
            }
            fHave = other.fHave;
        }
    }
    return *this;
}




UBool
DigitList::operator==(const DigitList& that) const
{
    if (this == &that) {
        return TRUE;
    }
    decNumber n;  
    decContext c;
    uprv_decContextDefault(&c, DEC_INIT_BASE);
    c.digits = 1;
    c.traps = 0;

    uprv_decNumberCompare(&n, this->fDecNumber, that.fDecNumber, &c);
    UBool result = decNumberIsZero(&n);
    return result;
}







int32_t DigitList::compare(const DigitList &other) {
    decNumber   result;
    int32_t     savedDigits = fContext.digits;
    fContext.digits = 1;
    uprv_decNumberCompare(&result, this->fDecNumber, other.fDecNumber, &fContext);
    fContext.digits = savedDigits;
    if (decNumberIsZero(&result)) {
        return 0;
    } else if (decNumberIsSpecial(&result)) {
        return -2;
    } else if (result.bits & DECNEG) {
        return -1;
    } else {
        return 1;
    }
}




void
DigitList::reduce() {
    uprv_decNumberReduce(fDecNumber, fDecNumber, &fContext);
}




void
DigitList::trim() {
    uprv_decNumberTrim(fDecNumber);
}




void
DigitList::clear()
{
    uprv_decNumberZero(fDecNumber);
    uprv_decContextSetRounding(&fContext, DEC_ROUND_HALF_EVEN);
    internalSetDouble(0.0);
}









static int32_t
formatBase10(int64_t number, char *outputStr) {
    
    
    

    const int32_t MAX_IDX = MAX_DIGITS+2;
    int32_t destIdx = MAX_IDX;
    outputStr[--destIdx] = 0; 

    int64_t  n = number;
    if (number < 0) {   
        outputStr[--destIdx] = (char)(-(n % 10) + kZero);
        n /= -10;
    }
    do { 
        outputStr[--destIdx] = (char)(n % 10 + kZero);
        n /= 10;
    } while (n > 0);
    
    if (number < 0) {
        outputStr[--destIdx] = '-';
    }

    
    U_ASSERT(destIdx >= 0);
    int32_t length = MAX_IDX - destIdx;
    uprv_memmove(outputStr, outputStr+MAX_IDX-length, length);

    return length;
}













void 
DigitList::setRoundingMode(DecimalFormat::ERoundingMode m) {
    enum rounding r;

    switch (m) {
      case  DecimalFormat::kRoundCeiling:  r = DEC_ROUND_CEILING;   break;
      case  DecimalFormat::kRoundFloor:    r = DEC_ROUND_FLOOR;     break;
      case  DecimalFormat::kRoundDown:     r = DEC_ROUND_DOWN;      break;
      case  DecimalFormat::kRoundUp:       r = DEC_ROUND_UP;        break;
      case  DecimalFormat::kRoundHalfEven: r = DEC_ROUND_HALF_EVEN; break;
      case  DecimalFormat::kRoundHalfDown: r = DEC_ROUND_HALF_DOWN; break;
      case  DecimalFormat::kRoundHalfUp:   r = DEC_ROUND_HALF_UP;   break;
      case  DecimalFormat::kRoundUnnecessary: r = DEC_ROUND_HALF_EVEN; break;
      default:
         
         
         r = uprv_decContextGetRounding(&fContext);
    }
    uprv_decContextSetRounding(&fContext, r);
  
}




void  
DigitList::setPositive(UBool s) {
    if (s) {
        fDecNumber->bits &= ~DECNEG; 
    } else {
        fDecNumber->bits |= DECNEG;
    }
    internalClear();
}


void     
DigitList::setDecimalAt(int32_t d) {
    U_ASSERT((fDecNumber->bits & DECSPECIAL) == 0);  
    U_ASSERT(d-1>-999999999);
    U_ASSERT(d-1< 999999999);
    int32_t adjustedDigits = fDecNumber->digits;
    if (decNumberIsZero(fDecNumber)) {
        
        adjustedDigits = 0;
    }
    fDecNumber->exponent = d - adjustedDigits;
    internalClear();
}

int32_t  
DigitList::getDecimalAt() {
    U_ASSERT((fDecNumber->bits & DECSPECIAL) == 0);  
    if (decNumberIsZero(fDecNumber) || ((fDecNumber->bits & DECSPECIAL) != 0)) {
        return fDecNumber->exponent;  
    }
    return fDecNumber->exponent + fDecNumber->digits;
}

void     
DigitList::setCount(int32_t c)  {
    U_ASSERT(c <= fContext.digits);
    if (c == 0) {
        
        
        c = 1;
        fDecNumber->lsu[0] = 0;
    }
    fDecNumber->digits = c;
    internalClear();
}

int32_t  
DigitList::getCount() const {
    if (decNumberIsZero(fDecNumber) && fDecNumber->exponent==0) {
       
       
       return 0;
    } else {
       return fDecNumber->digits;
    }
}
    
void     
DigitList::setDigit(int32_t i, char v) {
    int32_t count = fDecNumber->digits;
    U_ASSERT(i<count);
    U_ASSERT(v>='0' && v<='9');
    v &= 0x0f;
    fDecNumber->lsu[count-i-1] = v;
    internalClear();
}

char     
DigitList::getDigit(int32_t i) {
    int32_t count = fDecNumber->digits;
    U_ASSERT(i<count);
    return fDecNumber->lsu[count-i-1] + '0';
}


uint8_t
DigitList::getDigitValue(int32_t i) {
    int32_t count = fDecNumber->digits;
    U_ASSERT(i<count);
    return fDecNumber->lsu[count-i-1];
}









void
DigitList::append(char digit)
{
    U_ASSERT(digit>='0' && digit<='9');
    
    
    if (decNumberIsZero(fDecNumber)) {
        
        
        
        fDecNumber->lsu[0] = digit & 0x0f;
        fDecNumber->digits = 1;
        fDecNumber->exponent--;     
    } else {
        int32_t nDigits = fDecNumber->digits;
        if (nDigits < fContext.digits) {
            int i;
            for (i=nDigits; i>0; i--) {
                fDecNumber->lsu[i] = fDecNumber->lsu[i-1];
            }
            fDecNumber->lsu[0] = digit & 0x0f;
            fDecNumber->digits++;
            
            
            
            fDecNumber->exponent--;
        }
    }
    internalClear();
}










double
DigitList::getDouble() const
{
    static char gDecimal = 0;
    char decimalSeparator;
    {
        Mutex mutex;
        if (fHave == kDouble) {
            return fUnion.fDouble;
        } else if(fHave == kInt64) {
            return (double)fUnion.fInt64;
        }
        decimalSeparator = gDecimal;
    }

    if (decimalSeparator == 0) {
        
        
        
        
        char rep[MAX_DIGITS];
        sprintf(rep, "%+1.1f", 1.0);
        decimalSeparator = rep[2];
    }

    double tDouble = 0.0;
    if (isZero()) {
        tDouble = 0.0;
        if (decNumberIsNegative(fDecNumber)) {
            tDouble /= -1;
        }
    } else if (isInfinite()) {
        if (std::numeric_limits<double>::has_infinity) {
            tDouble = std::numeric_limits<double>::infinity();
        } else {
            tDouble = std::numeric_limits<double>::max();
        }
        if (!isPositive()) {
            tDouble = -tDouble; 
        } 
    } else {
        MaybeStackArray<char, MAX_DBL_DIGITS+18> s;
           
           
           
           

        
        
        if (getCount() > MAX_DBL_DIGITS + 3) {
            DigitList numToConvert(*this);
            numToConvert.reduce();    
            numToConvert.round(MAX_DBL_DIGITS+3);
            uprv_decNumberToString(numToConvert.fDecNumber, s.getAlias());
            
        } else {
            uprv_decNumberToString(this->fDecNumber, s.getAlias());
        }
        U_ASSERT(uprv_strlen(&s[0]) < MAX_DBL_DIGITS+18);
        
        if (decimalSeparator != '.') {
            char *decimalPt = strchr(s.getAlias(), '.');
            if (decimalPt != NULL) {
                *decimalPt = decimalSeparator;
            }
        }
        char *end = NULL;
        tDouble = uprv_strtod(s.getAlias(), &end);
    }
    {
        Mutex mutex;
        DigitList *nonConstThis = const_cast<DigitList *>(this);
        nonConstThis->internalSetDouble(tDouble);
        gDecimal = decimalSeparator;
    }
    return tDouble;
}







int32_t DigitList::getLong() 
{
    int32_t result = 0;
    if (fDecNumber->digits + fDecNumber->exponent > 10) {
        
        return result;
    }
    if (fDecNumber->exponent != 0) {
        
        
        DigitList copy(*this);
        DigitList zero;
        uprv_decNumberQuantize(copy.fDecNumber, copy.fDecNumber, zero.fDecNumber, &fContext);
        result = uprv_decNumberToInt32(copy.fDecNumber, &fContext);
    } else {
        result = uprv_decNumberToInt32(fDecNumber, &fContext);
    }
    return result;
}






int64_t DigitList::getInt64()  {
    if(fHave==kInt64) {
      return fUnion.fInt64;
    } 
    
    
    
    
    if (fDecNumber->digits + fDecNumber->exponent > 19) {
        
        return 0;
    }

    
    
    
    
    
    
    
    

    int32_t numIntDigits = fDecNumber->digits + fDecNumber->exponent;
    uint64_t value = 0;
    for (int32_t i = 0; i < numIntDigits; i++) {
        
        
        int32_t digitIndex = fDecNumber->digits - i - 1;
        int32_t v = (digitIndex >= 0) ? fDecNumber->lsu[digitIndex] : 0;
        value = value * (uint64_t)10 + (uint64_t)v;
    }

    if (decNumberIsNegative(fDecNumber)) {
        value = ~value;
        value += 1;
    }
    int64_t svalue = (int64_t)value;

    
    
    
    if (numIntDigits == 19) {
        if (( decNumberIsNegative(fDecNumber) && svalue>0) ||
            (!decNumberIsNegative(fDecNumber) && svalue<0)) {
            svalue = 0;
        }
    }
        
    return svalue;
}







void DigitList::getDecimal(CharString &str, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return;
    }

    
    
    int32_t maxLength = fDecNumber->digits + 14;
    int32_t capacity = 0;
    char *buffer = str.clear().getAppendBuffer(maxLength, 0, capacity, status);
    if (U_FAILURE(status)) {
        return;    
    }
    U_ASSERT(capacity >= maxLength);
    uprv_decNumberToString(this->fDecNumber, buffer);
    U_ASSERT((int32_t)uprv_strlen(buffer) <= maxLength);
    str.append(buffer, -1, status);
}





UBool
DigitList::fitsIntoLong(UBool ignoreNegativeZero) 
{
    if (decNumberIsSpecial(this->fDecNumber)) {
        
        return FALSE;
    }
    uprv_decNumberTrim(this->fDecNumber);
    if (fDecNumber->exponent < 0) {
        
        return FALSE;
    }
    if (decNumberIsZero(this->fDecNumber) && !ignoreNegativeZero &&
        (fDecNumber->bits & DECNEG) != 0) {
        
        return FALSE;
    }
    if (fDecNumber->digits + fDecNumber->exponent < 10) {
        
        
        
        return TRUE;
    }

    
    
    UErrorCode status = U_ZERO_ERROR;
    DigitList min32; min32.set("-2147483648", status);
    if (this->compare(min32) < 0) {
        return FALSE;
    }
    DigitList max32; max32.set("2147483647", status);
    if (this->compare(max32) > 0) {
        return FALSE;
    }
    if (U_FAILURE(status)) {
        return FALSE;
    }
    return true;
}







UBool
DigitList::fitsIntoInt64(UBool ignoreNegativeZero) 
{
    if (decNumberIsSpecial(this->fDecNumber)) {
        
        return FALSE;
    }
    uprv_decNumberTrim(this->fDecNumber);
    if (fDecNumber->exponent < 0) {
        
        return FALSE;
    }
    if (decNumberIsZero(this->fDecNumber) && !ignoreNegativeZero &&
        (fDecNumber->bits & DECNEG) != 0) {
        
        return FALSE;
    }
    if (fDecNumber->digits + fDecNumber->exponent < 19) {
        
        
        
        return TRUE;
    }

    
    
    UErrorCode status = U_ZERO_ERROR;
    DigitList min64; min64.set("-9223372036854775808", status);
    if (this->compare(min64) < 0) {
        return FALSE;
    }
    DigitList max64; max64.set("9223372036854775807", status);
    if (this->compare(max64) > 0) {
        return FALSE;
    }
    if (U_FAILURE(status)) {
        return FALSE;
    }
    return true;
}




void
DigitList::set(int32_t source)
{
    set((int64_t)source);
    internalSetDouble(source);
}





void
DigitList::set(int64_t source)
{
    char str[MAX_DIGITS+2];   
    formatBase10(source, str);
    U_ASSERT(uprv_strlen(str) < sizeof(str));

    uprv_decNumberFromString(fDecNumber, str, &fContext);
    internalSetDouble(source);
}




void
DigitList::setInteger(int64_t source)
{
  fDecNumber=NULL;
  internalSetInt64(source);
}











void
DigitList::set(const StringPiece &source, UErrorCode &status, uint32_t ) {
    if (U_FAILURE(status)) {
        return;
    }

#if 0    
    if(fastpathBits==(kFastpathOk|kNoDecimal)) {
      int32_t size = source.size();
      const char *data = source.data();
      int64_t r = 0;
      int64_t m = 1;
      
      while(size>0) {
        char ch = data[--size];
        if(ch=='+') {
          break;
        } else if(ch=='-') {
          r = -r;
          break;
        } else {
          int64_t d = ch-'0';
          
          r+=(d)*m;
          m *= 10;
        }
      }
      
      set(r);
    } else
#endif
        {
      
      
      int32_t numDigits = source.length();
      if (numDigits > fContext.digits) {
        
        decNumber *t = fStorage.resize(numDigits, fStorage.getCapacity());
        if (t == NULL) {
          status = U_MEMORY_ALLOCATION_ERROR;
          return;
        }
        fDecNumber = t;
        fContext.digits = numDigits;
      }

      fContext.status = 0;
      uprv_decNumberFromString(fDecNumber, source.data(), &fContext);
      if ((fContext.status & DEC_Conversion_syntax) != 0) {
        status = U_DECIMAL_NUMBER_SYNTAX_ERROR;
      }
    }
    internalClear();
}   






void
DigitList::set(double source)
{
    
    char rep[MAX_DIGITS + 8]; 

    
    
    
    
    if (uprv_isInfinite(source)) {
        if (uprv_isNegativeInfinity(source)) {
            uprv_strcpy(rep,"-inf"); 
        } else {
            uprv_strcpy(rep,"inf");
        }
    } else {
        sprintf(rep, "%+1.*e", MAX_DBL_DIGITS - 1, source);
    }
    U_ASSERT(uprv_strlen(rep) < sizeof(rep));

    
    
    
    char *decimalSeparator = strchr(rep, ',');
    if (decimalSeparator != NULL) {
        *decimalSeparator = '.';
    }

    
    uprv_decNumberFromString(fDecNumber, rep, &fContext);
    uprv_decNumberTrim(fDecNumber);
    internalSetDouble(source);
}









void
DigitList::mult(const DigitList &other, UErrorCode &status) {
    fContext.status = 0;
    int32_t requiredDigits = this->digits() + other.digits();
    if (requiredDigits > fContext.digits) {
        reduce();    
        int32_t requiredDigits = this->digits() + other.digits();
        ensureCapacity(requiredDigits, status);
    }
    uprv_decNumberMultiply(fDecNumber, fDecNumber, other.fDecNumber, &fContext);
    internalClear();
}









void
DigitList::div(const DigitList &other, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return;
    }
    uprv_decNumberDivide(fDecNumber, fDecNumber, other.fDecNumber, &fContext);
    internalClear();
}







void
DigitList::ensureCapacity(int32_t requestedCapacity, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return;
    }
    if (requestedCapacity <= 0) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    if (requestedCapacity > DEC_MAX_DIGITS) {
        
        
        
        requestedCapacity = DEC_MAX_DIGITS;
    }
    if (requestedCapacity > fContext.digits) {
        decNumber *newBuffer = fStorage.resize(requestedCapacity, fStorage.getCapacity());
        if (newBuffer == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
        fContext.digits = requestedCapacity;
        fDecNumber = newBuffer;
    }
}








void
DigitList::round(int32_t maximumDigits)
{
    int32_t savedDigits  = fContext.digits;
    fContext.digits = maximumDigits;
    uprv_decNumberPlus(fDecNumber, fDecNumber, &fContext);
    fContext.digits = savedDigits;
    uprv_decNumberTrim(fDecNumber);
    internalClear();
}


void
DigitList::roundFixedPoint(int32_t maximumFractionDigits) {
    trim();        
    if (fDecNumber->exponent >= -maximumFractionDigits) {
        return;
    }
    decNumber scale;   
    uprv_decNumberZero(&scale);    
    scale.exponent = -maximumFractionDigits;
    scale.lsu[0] = 1;
    
    uprv_decNumberQuantize(fDecNumber, fDecNumber, &scale, &fContext);
    trim();
    internalClear();
}



void
DigitList::toIntegralValue() {
    uprv_decNumberToIntegralValue(fDecNumber, fDecNumber, &fContext);
}



UBool
DigitList::isZero() const
{
    return decNumberIsZero(fDecNumber);
}

U_NAMESPACE_END
#endif 


