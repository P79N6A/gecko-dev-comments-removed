













#ifndef __BYTESTRIE_H__
#define __BYTESTRIE_H__






#include "unicode/utypes.h"
#include "unicode/stringpiece.h"
#include "unicode/uobject.h"
#include "unicode/ustringtrie.h"

U_NAMESPACE_BEGIN

class ByteSink;
class BytesTrieBuilder;
class CharString;
class UVector32;














class U_COMMON_API BytesTrie : public UMemory {
public:
    













    BytesTrie(const void *trieBytes)
            : ownedArray_(NULL), bytes_(static_cast<const uint8_t *>(trieBytes)),
              pos_(bytes_), remainingMatchLength_(-1) {}

    



    ~BytesTrie();

    





    BytesTrie(const BytesTrie &other)
            : ownedArray_(NULL), bytes_(other.bytes_),
              pos_(other.pos_), remainingMatchLength_(other.remainingMatchLength_) {}

    




    BytesTrie &reset() {
        pos_=bytes_;
        remainingMatchLength_=-1;
        return *this;
    }

    




    class State : public UMemory {
    public:
        



        State() { bytes=NULL; }
    private:
        friend class BytesTrie;

        const uint8_t *bytes;
        const uint8_t *pos;
        int32_t remainingMatchLength;
    };

    






    const BytesTrie &saveState(State &state) const {
        state.bytes=bytes_;
        state.pos=pos_;
        state.remainingMatchLength=remainingMatchLength_;
        return *this;
    }

    









    BytesTrie &resetToState(const State &state) {
        if(bytes_==state.bytes && bytes_!=NULL) {
            pos_=state.pos;
            remainingMatchLength_=state.remainingMatchLength;
        }
        return *this;
    }

    





    UStringTrieResult current() const;

    







    inline UStringTrieResult first(int32_t inByte) {
        remainingMatchLength_=-1;
        if(inByte<0) {
            inByte+=0x100;
        }
        return nextImpl(bytes_, inByte);
    }

    






    UStringTrieResult next(int32_t inByte);

    














    UStringTrieResult next(const char *s, int32_t length);

    








    inline int32_t getValue() const {
        const uint8_t *pos=pos_;
        int32_t leadByte=*pos++;
        
        return readValue(pos, leadByte>>1);
    }

    








    inline UBool hasUniqueValue(int32_t &uniqueValue) const {
        const uint8_t *pos=pos_;
        
        return pos!=NULL && findUniqueValue(pos+remainingMatchLength_+1, FALSE, uniqueValue);
    }

    







    int32_t getNextBytes(ByteSink &out) const;

    



    class U_COMMON_API Iterator : public UMemory {
    public:
        










        Iterator(const void *trieBytes, int32_t maxStringLength, UErrorCode &errorCode);

        










        Iterator(const BytesTrie &trie, int32_t maxStringLength, UErrorCode &errorCode);

        



        ~Iterator();

        




        Iterator &reset();

        



        UBool hasNext() const;

        













        UBool next(UErrorCode &errorCode);

        



        const StringPiece &getString() const { return sp_; }
        



        int32_t getValue() const { return value_; }

    private:
        UBool truncateAndStop();

        const uint8_t *branchNext(const uint8_t *pos, int32_t length, UErrorCode &errorCode);

        const uint8_t *bytes_;
        const uint8_t *pos_;
        const uint8_t *initialPos_;
        int32_t remainingMatchLength_;
        int32_t initialRemainingMatchLength_;

        CharString *str_;
        StringPiece sp_;
        int32_t maxLength_;
        int32_t value_;

        
        
        
        
        
        
        
        UVector32 *stack_;
    };

private:
    friend class BytesTrieBuilder;

    





    BytesTrie(void *adoptBytes, const void *trieBytes)
            : ownedArray_(static_cast<uint8_t *>(adoptBytes)),
              bytes_(static_cast<const uint8_t *>(trieBytes)),
              pos_(bytes_), remainingMatchLength_(-1) {}

    
    BytesTrie &operator=(const BytesTrie &other);

    inline void stop() {
        pos_=NULL;
    }

    
    
    static int32_t readValue(const uint8_t *pos, int32_t leadByte);
    static inline const uint8_t *skipValue(const uint8_t *pos, int32_t leadByte) {
        
        if(leadByte>=(kMinTwoByteValueLead<<1)) {
            if(leadByte<(kMinThreeByteValueLead<<1)) {
                ++pos;
            } else if(leadByte<(kFourByteValueLead<<1)) {
                pos+=2;
            } else {
                pos+=3+((leadByte>>1)&1);
            }
        }
        return pos;
    }
    static inline const uint8_t *skipValue(const uint8_t *pos) {
        int32_t leadByte=*pos++;
        return skipValue(pos, leadByte);
    }

    
    static const uint8_t *jumpByDelta(const uint8_t *pos);

    static inline const uint8_t *skipDelta(const uint8_t *pos) {
        int32_t delta=*pos++;
        if(delta>=kMinTwoByteDeltaLead) {
            if(delta<kMinThreeByteDeltaLead) {
                ++pos;
            } else if(delta<kFourByteDeltaLead) {
                pos+=2;
            } else {
                pos+=3+(delta&1);
            }
        }
        return pos;
    }

    static inline UStringTrieResult valueResult(int32_t node) {
        return (UStringTrieResult)(USTRINGTRIE_INTERMEDIATE_VALUE-(node&kValueIsFinal));
    }

    
    UStringTrieResult branchNext(const uint8_t *pos, int32_t length, int32_t inByte);

    
    UStringTrieResult nextImpl(const uint8_t *pos, int32_t inByte);

    
    
    
    static const uint8_t *findUniqueValueFromBranch(const uint8_t *pos, int32_t length,
                                                    UBool haveUniqueValue, int32_t &uniqueValue);
    
    
    static UBool findUniqueValue(const uint8_t *pos, UBool haveUniqueValue, int32_t &uniqueValue);

    
    
    static void getNextBranchBytes(const uint8_t *pos, int32_t length, ByteSink &out);
    static void append(ByteSink &out, int c);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    

    
    

    
    
    static const int32_t kMaxBranchLinearSubNodeLength=5;

    
    static const int32_t kMinLinearMatch=0x10;
    static const int32_t kMaxLinearMatchLength=0x10;

    
    
    
    
    
    static const int32_t kMinValueLead=kMinLinearMatch+kMaxLinearMatchLength;  
    
    static const int32_t kValueIsFinal=1;

    
    static const int32_t kMinOneByteValueLead=kMinValueLead/2;  
    static const int32_t kMaxOneByteValue=0x40;  

    static const int32_t kMinTwoByteValueLead=kMinOneByteValueLead+kMaxOneByteValue+1;  
    static const int32_t kMaxTwoByteValue=0x1aff;

    static const int32_t kMinThreeByteValueLead=kMinTwoByteValueLead+(kMaxTwoByteValue>>8)+1;  
    static const int32_t kFourByteValueLead=0x7e;

    
    static const int32_t kMaxThreeByteValue=((kFourByteValueLead-kMinThreeByteValueLead)<<16)-1;

    static const int32_t kFiveByteValueLead=0x7f;

    
    static const int32_t kMaxOneByteDelta=0xbf;
    static const int32_t kMinTwoByteDeltaLead=kMaxOneByteDelta+1;  
    static const int32_t kMinThreeByteDeltaLead=0xf0;
    static const int32_t kFourByteDeltaLead=0xfe;
    static const int32_t kFiveByteDeltaLead=0xff;

    static const int32_t kMaxTwoByteDelta=((kMinThreeByteDeltaLead-kMinTwoByteDeltaLead)<<8)-1;  
    static const int32_t kMaxThreeByteDelta=((kFourByteDeltaLead-kMinThreeByteDeltaLead)<<16)-1;  

    uint8_t *ownedArray_;

    
    const uint8_t *bytes_;

    

    
    const uint8_t *pos_;
    
    int32_t remainingMatchLength_;
};

U_NAMESPACE_END

#endif  
