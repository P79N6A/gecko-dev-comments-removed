










#ifndef __COLLATIONITERATOR_H__
#define __COLLATIONITERATOR_H__

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "cmemory.h"
#include "collation.h"
#include "collationdata.h"

U_NAMESPACE_BEGIN

class SkippedState;
class UCharsTrie;
class UVector32;







class U_I18N_API CollationIterator : public UObject {
private:
    class CEBuffer {
    private:
        
        static const int32_t INITIAL_CAPACITY = 40;
    public:
        CEBuffer() : length(0) {}
        ~CEBuffer();

        inline void append(int64_t ce, UErrorCode &errorCode) {
            if(length < INITIAL_CAPACITY || ensureAppendCapacity(1, errorCode)) {
                buffer[length++] = ce;
            }
        }

        inline void appendUnsafe(int64_t ce) {
            buffer[length++] = ce;
        }

        UBool ensureAppendCapacity(int32_t appCap, UErrorCode &errorCode);

        inline UBool incLength(UErrorCode &errorCode) {
            
            
            if(length < INITIAL_CAPACITY || ensureAppendCapacity(1, errorCode)) {
                ++length;
                return TRUE;
            } else {
                return FALSE;
            }
        }

        inline int64_t set(int32_t i, int64_t ce) {
            return buffer[i] = ce;
        }
        inline int64_t get(int32_t i) const { return buffer[i]; }

        const int64_t *getCEs() const { return buffer.getAlias(); }

        int32_t length;

    private:
        CEBuffer(const CEBuffer &);
        void operator=(const CEBuffer &);

        MaybeStackArray<int64_t, INITIAL_CAPACITY> buffer;
    };

public:
    CollationIterator(const CollationData *d, UBool numeric)
            : trie(d->trie),
              data(d),
              cesIndex(0),
              skipped(NULL),
              numCpFwd(-1),
              isNumeric(numeric) {}

    virtual ~CollationIterator();

    virtual UBool operator==(const CollationIterator &other) const;
    inline UBool operator!=(const CollationIterator &other) const {
        return !operator==(other);
    }

    




    virtual void resetToOffset(int32_t newOffset) = 0;

    virtual int32_t getOffset() const = 0;

    


    inline int64_t nextCE(UErrorCode &errorCode) {
        if(cesIndex < ceBuffer.length) {
            
            return ceBuffer.get(cesIndex++);
        }
        
        if(!ceBuffer.incLength(errorCode)) {
            return Collation::NO_CE;
        }
        UChar32 c;
        uint32_t ce32 = handleNextCE32(c, errorCode);
        uint32_t t = ce32 & 0xff;
        if(t < Collation::SPECIAL_CE32_LOW_BYTE) {  
            
            
            return ceBuffer.set(cesIndex++,
                    ((int64_t)(ce32 & 0xffff0000) << 32) | ((ce32 & 0xff00) << 16) | (t << 8));
        }
        const CollationData *d;
        
        
        if(t == Collation::SPECIAL_CE32_LOW_BYTE) {
            if(c < 0) {
                return ceBuffer.set(cesIndex++, Collation::NO_CE);
            }
            d = data->base;
            ce32 = d->getCE32(c);
            t = ce32 & 0xff;
            if(t < Collation::SPECIAL_CE32_LOW_BYTE) {
                
                return ceBuffer.set(cesIndex++,
                        ((int64_t)(ce32 & 0xffff0000) << 32) | ((ce32 & 0xff00) << 16) | (t << 8));
            }
        } else {
            d = data;
        }
        if(t == Collation::LONG_PRIMARY_CE32_LOW_BYTE) {
            
            return ceBuffer.set(cesIndex++,
                    ((int64_t)(ce32 - t) << 32) | Collation::COMMON_SEC_AND_TER_CE);
        }
        return nextCEFromCE32(d, c, ce32, errorCode);
    }

    



    int32_t fetchCEs(UErrorCode &errorCode);

    


    void setCurrentCE(int64_t ce) {
        
        ceBuffer.set(cesIndex - 1, ce);
    }

    


    int64_t previousCE(UVector32 &offsets, UErrorCode &errorCode);

    inline int32_t getCEsLength() const {
        return ceBuffer.length;
    }

    inline int64_t getCE(int32_t i) const {
        return ceBuffer.get(i);
    }

    const int64_t *getCEs() const {
        return ceBuffer.getCEs();
    }

    void clearCEs() {
        cesIndex = ceBuffer.length = 0;
    }

    void clearCEsIfNoneRemaining() {
        if(cesIndex == ceBuffer.length) { clearCEs(); }
    }

    



    virtual UChar32 nextCodePoint(UErrorCode &errorCode) = 0;

    



    virtual UChar32 previousCodePoint(UErrorCode &errorCode) = 0;

protected:
    CollationIterator(const CollationIterator &other);

    void reset();

    







    virtual uint32_t handleNextCE32(UChar32 &c, UErrorCode &errorCode);

    





    virtual UChar handleGetTrailSurrogate();

    



    virtual UBool foundNULTerminator();

    




    virtual UBool forbidSurrogateCodePoints() const;

    virtual void forwardNumCodePoints(int32_t num, UErrorCode &errorCode) = 0;

    virtual void backwardNumCodePoints(int32_t num, UErrorCode &errorCode) = 0;

    




    virtual uint32_t getDataCE32(UChar32 c) const;

    virtual uint32_t getCE32FromBuilderData(uint32_t ce32, UErrorCode &errorCode);

    void appendCEsFromCE32(const CollationData *d, UChar32 c, uint32_t ce32,
                           UBool forward, UErrorCode &errorCode);

    
    const UTrie2 *trie;
    const CollationData *data;

private:
    int64_t nextCEFromCE32(const CollationData *d, UChar32 c, uint32_t ce32,
                           UErrorCode &errorCode);

    uint32_t getCE32FromPrefix(const CollationData *d, uint32_t ce32,
                               UErrorCode &errorCode);

    UChar32 nextSkippedCodePoint(UErrorCode &errorCode);

    void backwardNumSkipped(int32_t n, UErrorCode &errorCode);

    uint32_t nextCE32FromContraction(
            const CollationData *d, uint32_t contractionCE32,
            const UChar *p, uint32_t ce32, UChar32 c,
            UErrorCode &errorCode);

    uint32_t nextCE32FromDiscontiguousContraction(
            const CollationData *d, UCharsTrie &suffixes, uint32_t ce32,
            int32_t lookAhead, UChar32 c,
            UErrorCode &errorCode);

    


    int64_t previousCEUnsafe(UChar32 c, UVector32 &offsets, UErrorCode &errorCode);

    






    void appendNumericCEs(uint32_t ce32, UBool forward, UErrorCode &errorCode);

    



    void appendNumericSegmentCEs(const char *digits, int32_t length, UErrorCode &errorCode);

    CEBuffer ceBuffer;
    int32_t cesIndex;

    SkippedState *skipped;

    
    
    int32_t numCpFwd;
    
    UBool isNumeric;
};

U_NAMESPACE_END

#endif  
#endif  
