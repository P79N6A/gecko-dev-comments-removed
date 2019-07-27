










#ifndef __UTF8COLLATIONITERATOR_H__
#define __UTF8COLLATIONITERATOR_H__

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "cmemory.h"
#include "collation.h"
#include "collationdata.h"
#include "normalizer2impl.h"

U_NAMESPACE_BEGIN






class U_I18N_API UTF8CollationIterator : public CollationIterator {
public:
    UTF8CollationIterator(const CollationData *d, UBool numeric,
                          const uint8_t *s, int32_t p, int32_t len)
            : CollationIterator(d, numeric),
              u8(s), pos(p), length(len) {}

    virtual ~UTF8CollationIterator();

    virtual void resetToOffset(int32_t newOffset);

    virtual int32_t getOffset() const;

    virtual UChar32 nextCodePoint(UErrorCode &errorCode);

    virtual UChar32 previousCodePoint(UErrorCode &errorCode);

protected:
    









    virtual uint32_t handleNextCE32(UChar32 &c, UErrorCode &errorCode);

    virtual UBool foundNULTerminator();

    virtual UBool forbidSurrogateCodePoints() const;

    virtual void forwardNumCodePoints(int32_t num, UErrorCode &errorCode);

    virtual void backwardNumCodePoints(int32_t num, UErrorCode &errorCode);

    const uint8_t *u8;
    int32_t pos;
    int32_t length;  
};




class U_I18N_API FCDUTF8CollationIterator : public UTF8CollationIterator {
public:
    FCDUTF8CollationIterator(const CollationData *data, UBool numeric,
                             const uint8_t *s, int32_t p, int32_t len)
            : UTF8CollationIterator(data, numeric, s, p, len),
              state(CHECK_FWD), start(p),
              nfcImpl(data->nfcImpl) {}

    virtual ~FCDUTF8CollationIterator();

    virtual void resetToOffset(int32_t newOffset);

    virtual int32_t getOffset() const;

    virtual UChar32 nextCodePoint(UErrorCode &errorCode);

    virtual UChar32 previousCodePoint(UErrorCode &errorCode);

protected:
    virtual uint32_t handleNextCE32(UChar32 &c, UErrorCode &errorCode);

    virtual UChar handleGetTrailSurrogate();

    virtual UBool foundNULTerminator();

    virtual void forwardNumCodePoints(int32_t num, UErrorCode &errorCode);

    virtual void backwardNumCodePoints(int32_t num, UErrorCode &errorCode);

private:
    UBool nextHasLccc() const;
    UBool previousHasTccc() const;

    


    void switchToForward();

    



    UBool nextSegment(UErrorCode &errorCode);

    


    void switchToBackward();

    



    UBool previousSegment(UErrorCode &errorCode);

    UBool normalize(const UnicodeString &s, UErrorCode &errorCode);

    enum State {
        




        CHECK_FWD,
        




        CHECK_BWD,
        



        IN_FCD_SEGMENT,
        



        IN_NORMALIZED
    };

    State state;

    int32_t start;
    int32_t limit;

    const Normalizer2Impl &nfcImpl;
    UnicodeString normalized;
};

U_NAMESPACE_END

#endif  
#endif  
