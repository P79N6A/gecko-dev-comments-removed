















#ifndef __NORMALIZER2_H__
#define __NORMALIZER2_H__






#include "unicode/utypes.h"

#if !UCONFIG_NO_NORMALIZATION

#include "unicode/uniset.h"
#include "unicode/unistr.h"
#include "unicode/unorm2.h"

U_NAMESPACE_BEGIN












































class U_COMMON_API Normalizer2 : public UObject {
public:
    



    ~Normalizer2();

#ifndef U_HIDE_DRAFT_API
    










    static const Normalizer2 *
    getNFCInstance(UErrorCode &errorCode);

    










    static const Normalizer2 *
    getNFDInstance(UErrorCode &errorCode);

    










    static const Normalizer2 *
    getNFKCInstance(UErrorCode &errorCode);

    










    static const Normalizer2 *
    getNFKDInstance(UErrorCode &errorCode);

    










    static const Normalizer2 *
    getNFKCCasefoldInstance(UErrorCode &errorCode);
#endif  

    




















    static const Normalizer2 *
    getInstance(const char *packageName,
                const char *name,
                UNormalization2Mode mode,
                UErrorCode &errorCode);

    









    UnicodeString
    normalize(const UnicodeString &src, UErrorCode &errorCode) const {
        UnicodeString result;
        normalize(src, result, errorCode);
        return result;
    }
    












    virtual UnicodeString &
    normalize(const UnicodeString &src,
              UnicodeString &dest,
              UErrorCode &errorCode) const = 0;
    













    virtual UnicodeString &
    normalizeSecondAndAppend(UnicodeString &first,
                             const UnicodeString &second,
                             UErrorCode &errorCode) const = 0;
    













    virtual UnicodeString &
    append(UnicodeString &first,
           const UnicodeString &second,
           UErrorCode &errorCode) const = 0;

    












    virtual UBool
    getDecomposition(UChar32 c, UnicodeString &decomposition) const = 0;

    























    virtual UBool
    getRawDecomposition(UChar32 c, UnicodeString &decomposition) const;

    














    virtual UChar32
    composePair(UChar32 a, UChar32 b) const;

    







    virtual uint8_t
    getCombiningClass(UChar32 c) const;

    













    virtual UBool
    isNormalized(const UnicodeString &s, UErrorCode &errorCode) const = 0;

    














    virtual UNormalizationCheckResult
    quickCheck(const UnicodeString &s, UErrorCode &errorCode) const = 0;

    





















    virtual int32_t
    spanQuickCheckYes(const UnicodeString &s, UErrorCode &errorCode) const = 0;

    












    virtual UBool hasBoundaryBefore(UChar32 c) const = 0;

    













    virtual UBool hasBoundaryAfter(UChar32 c) const = 0;

    












    virtual UBool isInert(UChar32 c) const = 0;

private:
    
    virtual UClassID getDynamicClassID() const;
};












class U_COMMON_API FilteredNormalizer2 : public Normalizer2 {
public:
    









    FilteredNormalizer2(const Normalizer2 &n2, const UnicodeSet &filterSet) :
            norm2(n2), set(filterSet) {}

    



    ~FilteredNormalizer2();

    












    virtual UnicodeString &
    normalize(const UnicodeString &src,
              UnicodeString &dest,
              UErrorCode &errorCode) const;
    













    virtual UnicodeString &
    normalizeSecondAndAppend(UnicodeString &first,
                             const UnicodeString &second,
                             UErrorCode &errorCode) const;
    













    virtual UnicodeString &
    append(UnicodeString &first,
           const UnicodeString &second,
           UErrorCode &errorCode) const;

    










    virtual UBool
    getDecomposition(UChar32 c, UnicodeString &decomposition) const;

    










    virtual UBool
    getRawDecomposition(UChar32 c, UnicodeString &decomposition) const;

    









    virtual UChar32
    composePair(UChar32 a, UChar32 b) const;

    







    virtual uint8_t
    getCombiningClass(UChar32 c) const;

    










    virtual UBool
    isNormalized(const UnicodeString &s, UErrorCode &errorCode) const;
    










    virtual UNormalizationCheckResult
    quickCheck(const UnicodeString &s, UErrorCode &errorCode) const;
    










    virtual int32_t
    spanQuickCheckYes(const UnicodeString &s, UErrorCode &errorCode) const;

    







    virtual UBool hasBoundaryBefore(UChar32 c) const;

    







    virtual UBool hasBoundaryAfter(UChar32 c) const;

    






    virtual UBool isInert(UChar32 c) const;
private:
    UnicodeString &
    normalize(const UnicodeString &src,
              UnicodeString &dest,
              USetSpanCondition spanCondition,
              UErrorCode &errorCode) const;

    UnicodeString &
    normalizeSecondAndAppend(UnicodeString &first,
                             const UnicodeString &second,
                             UBool doNormalize,
                             UErrorCode &errorCode) const;

    const Normalizer2 &norm2;
    const UnicodeSet &set;
};

U_NAMESPACE_END

#endif  
#endif  
