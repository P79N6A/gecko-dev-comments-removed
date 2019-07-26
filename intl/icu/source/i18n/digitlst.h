





















 
#ifndef DIGITLST_H
#define DIGITLST_H
 
#include "unicode/uobject.h"

#if !UCONFIG_NO_FORMATTING
#include "unicode/decimfmt.h"
#include <float.h>
#include "decContext.h"
#include "decNumber.h"
#include "cmemory.h"


#define INT64_DIGITS 19

typedef enum EDigitListValues {
    MAX_DBL_DIGITS = DBL_DIG,
    MAX_I64_DIGITS = INT64_DIGITS,
    MAX_DIGITS = MAX_I64_DIGITS,
    MAX_EXPONENT = DBL_DIG,
    DIGIT_PADDING = 3,
    DEFAULT_DIGITS = 40,   

     
    MAX_DEC_DIGITS = MAX_DIGITS + DIGIT_PADDING + MAX_EXPONENT
} EDigitListValues;

U_NAMESPACE_BEGIN

class CharString;










#if !U_PLATFORM_IS_DARWIN_BASED
template class U_I18N_API MaybeStackHeaderAndArray<decNumber, char, DEFAULT_DIGITS>;
#endif


enum EStackMode { kOnStack };

enum EFastpathBits { kFastpathOk = 1, kNoDecimal = 2 };






















































class U_I18N_API DigitList : public UMemory { 
public:

    DigitList();
    ~DigitList();

    



    DigitList(const DigitList&); 

    



    DigitList& operator=(const DigitList&);  

    





    UBool operator==(const DigitList& other) const;

    int32_t  compare(const DigitList& other);


    inline UBool operator!=(const DigitList& other) const { return !operator==(other); }

    






    void clear(void);

    



    void toIntegralValue();
    
    











    void append(char digit);

    




    double getDouble(void) const;

    





    int32_t getLong(void) ;

    





    int64_t getInt64(void) ;

    


    void getDecimal(CharString &str, UErrorCode &status);

    






    UBool fitsIntoLong(UBool ignoreNegativeZero) ;

    






    UBool fitsIntoInt64(UBool ignoreNegativeZero) ;

    



    void set(double source);

    





    void set(int32_t source);

    





    void set(int64_t source);

    






    void setInteger(int64_t source);

   





    void set(const StringPiece &source, UErrorCode &status, uint32_t fastpathBits = 0);

    




    void mult(const DigitList &arg, UErrorCode &status);

    


    void div(const DigitList &arg, UErrorCode &status);

    
    

    void setRoundingMode(DecimalFormat::ERoundingMode m); 

    


    UBool isZero(void) const;

    


    UBool isNaN(void) const {return decNumberIsNaN(fDecNumber);}

    UBool isInfinite() const {return decNumberIsInfinite(fDecNumber);}

    
    void     reduce();

    
    void     trim();

    
    void     setToZero() {uprv_decNumberZero(fDecNumber);}

    
    int32_t  digits() const {return fDecNumber->digits;}

    




    void round(int32_t maximumDigits);

    void roundFixedPoint(int32_t maximumFractionDigits);

    



    void  ensureCapacity(int32_t  requestedSize, UErrorCode &status); 

    UBool    isPositive(void) const { return decNumberIsNegative(fDecNumber) == 0;}
    void     setPositive(UBool s); 

    void     setDecimalAt(int32_t d);
    int32_t  getDecimalAt();

    void     setCount(int32_t c);
    int32_t  getCount() const;
    
    




    void     setDigit(int32_t i, char v);

    




    char     getDigit(int32_t i);


    





    uint8_t     getDigitValue(int32_t i);


private:
    



























public:
    decContext    fContext;   

private:
    decNumber     *fDecNumber;
    MaybeStackHeaderAndArray<decNumber, char, DEFAULT_DIGITS>  fStorage;

    



    union DoubleOrInt64 {
      double        fDouble;
      int64_t       fInt64;
    } fUnion;
    enum EHave {
      kNone=0,
      kDouble,
      kInt64
    } fHave;



    UBool shouldRoundUp(int32_t maximumDigits) const;

 public:

    using UMemory::operator new;
    using UMemory::operator delete;

    



    static inline void * U_EXPORT2 operator new(size_t , void * onStack, EStackMode  ) U_NO_THROW { return onStack; }

    



    static inline void U_EXPORT2 operator delete(void * , void * , EStackMode )  U_NO_THROW {}

 private:
    inline void internalSetDouble(double d) {
      fHave = kDouble;
      fUnion.fDouble=d;
    }
    inline void internalSetInt64(int64_t d) {
      fHave = kInt64;
      fUnion.fInt64=d;
    }
    inline void internalClear() {
      fHave = kNone;
    }
};


U_NAMESPACE_END

#endif 
#endif 


