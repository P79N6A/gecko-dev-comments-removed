










#ifndef __UTF16COLLATIONITERATOR_H__
#define __UTF16COLLATIONITERATOR_H__

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "cmemory.h"
#include "collation.h"
#include "collationdata.h"
#include "collationiterator.h"
#include "normalizer2impl.h"

U_NAMESPACE_BEGIN






class U_I18N_API UTF16CollationIterator : public CollationIterator {
public:
    UTF16CollationIterator(const CollationData *d, UBool numeric,
                           const UChar *s, const UChar *p, const UChar *lim)
            : CollationIterator(d, numeric),
              start(s), pos(p), limit(lim) {}

    UTF16CollationIterator(const UTF16CollationIterator &other, const UChar *newText);

    virtual ~UTF16CollationIterator();

    virtual UBool operator==(const CollationIterator &other) const;

    virtual void resetToOffset(int32_t newOffset);

    virtual int32_t getOffset() const;

    void setText(const UChar *s, const UChar *lim) {
        reset();
        start = pos = s;
        limit = lim;
    }

    virtual UChar32 nextCodePoint(UErrorCode &errorCode);

    virtual UChar32 previousCodePoint(UErrorCode &errorCode);

protected:
    
    UTF16CollationIterator(const UTF16CollationIterator &other)
            : CollationIterator(other),
              start(NULL), pos(NULL), limit(NULL) {}

    virtual uint32_t handleNextCE32(UChar32 &c, UErrorCode &errorCode);

    virtual UChar handleGetTrailSurrogate();

    virtual UBool foundNULTerminator();

    virtual void forwardNumCodePoints(int32_t num, UErrorCode &errorCode);

    virtual void backwardNumCodePoints(int32_t num, UErrorCode &errorCode);

    
    
    const UChar *start, *pos, *limit;
};




class U_I18N_API FCDUTF16CollationIterator : public UTF16CollationIterator {
public:
    FCDUTF16CollationIterator(const CollationData *data, UBool numeric,
                              const UChar *s, const UChar *p, const UChar *lim)
            : UTF16CollationIterator(data, numeric, s, p, lim),
              rawStart(s), segmentStart(p), segmentLimit(NULL), rawLimit(lim),
              nfcImpl(data->nfcImpl),
              checkDir(1) {}

    FCDUTF16CollationIterator(const FCDUTF16CollationIterator &other, const UChar *newText);

    virtual ~FCDUTF16CollationIterator();

    virtual UBool operator==(const CollationIterator &other) const;

    virtual void resetToOffset(int32_t newOffset);

    virtual int32_t getOffset() const;

    virtual UChar32 nextCodePoint(UErrorCode &errorCode);

    virtual UChar32 previousCodePoint(UErrorCode &errorCode);

protected:
    virtual uint32_t handleNextCE32(UChar32 &c, UErrorCode &errorCode);

    virtual UBool foundNULTerminator();

    virtual void forwardNumCodePoints(int32_t num, UErrorCode &errorCode);

    virtual void backwardNumCodePoints(int32_t num, UErrorCode &errorCode);

private:
    




    void switchToForward();

    




    UBool nextSegment(UErrorCode &errorCode);

    




    void switchToBackward();

    




    UBool previousSegment(UErrorCode &errorCode);

    UBool normalize(const UChar *from, const UChar *to, UErrorCode &errorCode);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    const UChar *rawStart;
    const UChar *segmentStart;
    const UChar *segmentLimit;
    
    const UChar *rawLimit;

    const Normalizer2Impl &nfcImpl;
    UnicodeString normalized;
    
    int8_t checkDir;
};

U_NAMESPACE_END

#endif  
#endif  
