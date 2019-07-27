










#ifndef __UITERCOLLATIONITERATOR_H__
#define __UITERCOLLATIONITERATOR_H__

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/uiter.h"
#include "cmemory.h"
#include "collation.h"
#include "collationdata.h"
#include "normalizer2impl.h"

U_NAMESPACE_BEGIN






class U_I18N_API UIterCollationIterator : public CollationIterator {
public:
    UIterCollationIterator(const CollationData *d, UBool numeric, UCharIterator &ui)
            : CollationIterator(d, numeric), iter(ui) {}

    virtual ~UIterCollationIterator();

    virtual void resetToOffset(int32_t newOffset);

    virtual int32_t getOffset() const;

    virtual UChar32 nextCodePoint(UErrorCode &errorCode);

    virtual UChar32 previousCodePoint(UErrorCode &errorCode);

protected:
    virtual uint32_t handleNextCE32(UChar32 &c, UErrorCode &errorCode);

    virtual UChar handleGetTrailSurrogate();

    virtual void forwardNumCodePoints(int32_t num, UErrorCode &errorCode);

    virtual void backwardNumCodePoints(int32_t num, UErrorCode &errorCode);

    UCharIterator &iter;
};




class U_I18N_API FCDUIterCollationIterator : public UIterCollationIterator {
public:
    FCDUIterCollationIterator(const CollationData *data, UBool numeric, UCharIterator &ui, int32_t startIndex)
            : UIterCollationIterator(data, numeric, ui),
              state(ITER_CHECK_FWD), start(startIndex),
              nfcImpl(data->nfcImpl) {}

    virtual ~FCDUIterCollationIterator();

    virtual void resetToOffset(int32_t newOffset);

    virtual int32_t getOffset() const;

    virtual UChar32 nextCodePoint(UErrorCode &errorCode);

    virtual UChar32 previousCodePoint(UErrorCode &errorCode);

protected:
    virtual uint32_t handleNextCE32(UChar32 &c, UErrorCode &errorCode);

    virtual UChar handleGetTrailSurrogate();

    virtual void forwardNumCodePoints(int32_t num, UErrorCode &errorCode);

    virtual void backwardNumCodePoints(int32_t num, UErrorCode &errorCode);

private:
    


    void switchToForward();

    



    UBool nextSegment(UErrorCode &errorCode);

    


    void switchToBackward();

    



    UBool previousSegment(UErrorCode &errorCode);

    UBool normalize(const UnicodeString &s, UErrorCode &errorCode);

    enum State {
        




        ITER_CHECK_FWD,
        




        ITER_CHECK_BWD,
        



        ITER_IN_FCD_SEGMENT,
        




        IN_NORM_ITER_AT_LIMIT,
        




        IN_NORM_ITER_AT_START
    };

    State state;

    int32_t start;
    int32_t pos;
    int32_t limit;

    const Normalizer2Impl &nfcImpl;
    UnicodeString normalized;
};

U_NAMESPACE_END

#endif  
#endif  
