













#ifndef __UCHARSTRIE_H__
#define __UCHARSTRIE_H__







#include "unicode/utypes.h"
#include "unicode/unistr.h"
#include "unicode/uobject.h"
#include "unicode/ustringtrie.h"

U_NAMESPACE_BEGIN

class Appendable;
class UCharsTrieBuilder;
class UVector32;














class U_COMMON_API UCharsTrie : public UMemory {
public:
    













    UCharsTrie(const UChar *trieUChars)
            : ownedArray_(NULL), uchars_(trieUChars),
              pos_(uchars_), remainingMatchLength_(-1) {}

    



    ~UCharsTrie();

    





    UCharsTrie(const UCharsTrie &other)
            : ownedArray_(NULL), uchars_(other.uchars_),
              pos_(other.pos_), remainingMatchLength_(other.remainingMatchLength_) {}

    




    UCharsTrie &reset() {
        pos_=uchars_;
        remainingMatchLength_=-1;
        return *this;
    }

    




    class State : public UMemory {
    public:
        



        State() { uchars=NULL; }
    private:
        friend class UCharsTrie;

        const UChar *uchars;
        const UChar *pos;
        int32_t remainingMatchLength;
    };

    






    const UCharsTrie &saveState(State &state) const {
        state.uchars=uchars_;
        state.pos=pos_;
        state.remainingMatchLength=remainingMatchLength_;
        return *this;
    }

    









    UCharsTrie &resetToState(const State &state) {
        if(uchars_==state.uchars && uchars_!=NULL) {
            pos_=state.pos;
            remainingMatchLength_=state.remainingMatchLength;
        }
        return *this;
    }

    





    UStringTrieResult current() const;

    






    inline UStringTrieResult first(int32_t uchar) {
        remainingMatchLength_=-1;
        return nextImpl(uchars_, uchar);
    }

    







    UStringTrieResult firstForCodePoint(UChar32 cp);

    





    UStringTrieResult next(int32_t uchar);

    






    UStringTrieResult nextForCodePoint(UChar32 cp);

    














    UStringTrieResult next(const UChar *s, int32_t length);

    








    inline int32_t getValue() const {
        const UChar *pos=pos_;
        int32_t leadUnit=*pos++;
        
        return leadUnit&kValueIsFinal ?
            readValue(pos, leadUnit&0x7fff) : readNodeValue(pos, leadUnit);
    }

    








    inline UBool hasUniqueValue(int32_t &uniqueValue) const {
        const UChar *pos=pos_;
        
        return pos!=NULL && findUniqueValue(pos+remainingMatchLength_+1, FALSE, uniqueValue);
    }

    






    int32_t getNextUChars(Appendable &out) const;

    



    class U_COMMON_API Iterator : public UMemory {
    public:
        










        Iterator(const UChar *trieUChars, int32_t maxStringLength, UErrorCode &errorCode);

        










        Iterator(const UCharsTrie &trie, int32_t maxStringLength, UErrorCode &errorCode);

        



        ~Iterator();

        




        Iterator &reset();

        



        UBool hasNext() const;

        













        UBool next(UErrorCode &errorCode);

        



        const UnicodeString &getString() const { return str_; }
        



        int32_t getValue() const { return value_; }

    private:
        UBool truncateAndStop() {
            pos_=NULL;
            value_=-1;  
            return TRUE;
        }

        const UChar *branchNext(const UChar *pos, int32_t length, UErrorCode &errorCode);

        const UChar *uchars_;
        const UChar *pos_;
        const UChar *initialPos_;
        int32_t remainingMatchLength_;
        int32_t initialRemainingMatchLength_;
        UBool skipValue_;  

        UnicodeString str_;
        int32_t maxLength_;
        int32_t value_;

        
        
        
        
        
        
        
        UVector32 *stack_;
    };

private:
    friend class UCharsTrieBuilder;

    





    UCharsTrie(UChar *adoptUChars, const UChar *trieUChars)
            : ownedArray_(adoptUChars), uchars_(trieUChars),
              pos_(uchars_), remainingMatchLength_(-1) {}

    
    UCharsTrie &operator=(const UCharsTrie &other);

    inline void stop() {
        pos_=NULL;
    }

    
    
    static inline int32_t readValue(const UChar *pos, int32_t leadUnit) {
        int32_t value;
        if(leadUnit<kMinTwoUnitValueLead) {
            value=leadUnit;
        } else if(leadUnit<kThreeUnitValueLead) {
            value=((leadUnit-kMinTwoUnitValueLead)<<16)|*pos;
        } else {
            value=(pos[0]<<16)|pos[1];
        }
        return value;
    }
    static inline const UChar *skipValue(const UChar *pos, int32_t leadUnit) {
        if(leadUnit>=kMinTwoUnitValueLead) {
            if(leadUnit<kThreeUnitValueLead) {
                ++pos;
            } else {
                pos+=2;
            }
        }
        return pos;
    }
    static inline const UChar *skipValue(const UChar *pos) {
        int32_t leadUnit=*pos++;
        return skipValue(pos, leadUnit&0x7fff);
    }

    static inline int32_t readNodeValue(const UChar *pos, int32_t leadUnit) {
        
        int32_t value;
        if(leadUnit<kMinTwoUnitNodeValueLead) {
            value=(leadUnit>>6)-1;
        } else if(leadUnit<kThreeUnitNodeValueLead) {
            value=(((leadUnit&0x7fc0)-kMinTwoUnitNodeValueLead)<<10)|*pos;
        } else {
            value=(pos[0]<<16)|pos[1];
        }
        return value;
    }
    static inline const UChar *skipNodeValue(const UChar *pos, int32_t leadUnit) {
        
        if(leadUnit>=kMinTwoUnitNodeValueLead) {
            if(leadUnit<kThreeUnitNodeValueLead) {
                ++pos;
            } else {
                pos+=2;
            }
        }
        return pos;
    }

    static inline const UChar *jumpByDelta(const UChar *pos) {
        int32_t delta=*pos++;
        if(delta>=kMinTwoUnitDeltaLead) {
            if(delta==kThreeUnitDeltaLead) {
                delta=(pos[0]<<16)|pos[1];
                pos+=2;
            } else {
                delta=((delta-kMinTwoUnitDeltaLead)<<16)|*pos++;
            }
        }
        return pos+delta;
    }

    static const UChar *skipDelta(const UChar *pos) {
        int32_t delta=*pos++;
        if(delta>=kMinTwoUnitDeltaLead) {
            if(delta==kThreeUnitDeltaLead) {
                pos+=2;
            } else {
                ++pos;
            }
        }
        return pos;
    }

    static inline UStringTrieResult valueResult(int32_t node) {
        return (UStringTrieResult)(USTRINGTRIE_INTERMEDIATE_VALUE-(node>>15));
    }

    
    UStringTrieResult branchNext(const UChar *pos, int32_t length, int32_t uchar);

    
    UStringTrieResult nextImpl(const UChar *pos, int32_t uchar);

    
    
    
    static const UChar *findUniqueValueFromBranch(const UChar *pos, int32_t length,
                                                  UBool haveUniqueValue, int32_t &uniqueValue);
    
    
    static UBool findUniqueValue(const UChar *pos, UBool haveUniqueValue, int32_t &uniqueValue);

    
    
    static void getNextBranchUChars(const UChar *pos, int32_t length, Appendable &out);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    

    
    

    
    
    static const int32_t kMaxBranchLinearSubNodeLength=5;

    
    static const int32_t kMinLinearMatch=0x30;
    static const int32_t kMaxLinearMatchLength=0x10;

    
    
    
    static const int32_t kMinValueLead=kMinLinearMatch+kMaxLinearMatchLength;  
    static const int32_t kNodeTypeMask=kMinValueLead-1;  

    
    static const int32_t kValueIsFinal=0x8000;

    
    static const int32_t kMaxOneUnitValue=0x3fff;

    static const int32_t kMinTwoUnitValueLead=kMaxOneUnitValue+1;  
    static const int32_t kThreeUnitValueLead=0x7fff;

    static const int32_t kMaxTwoUnitValue=((kThreeUnitValueLead-kMinTwoUnitValueLead)<<16)-1;  

    
    static const int32_t kMaxOneUnitNodeValue=0xff;
    static const int32_t kMinTwoUnitNodeValueLead=kMinValueLead+((kMaxOneUnitNodeValue+1)<<6);  
    static const int32_t kThreeUnitNodeValueLead=0x7fc0;

    static const int32_t kMaxTwoUnitNodeValue=
        ((kThreeUnitNodeValueLead-kMinTwoUnitNodeValueLead)<<10)-1;  

    
    static const int32_t kMaxOneUnitDelta=0xfbff;
    static const int32_t kMinTwoUnitDeltaLead=kMaxOneUnitDelta+1;  
    static const int32_t kThreeUnitDeltaLead=0xffff;

    static const int32_t kMaxTwoUnitDelta=((kThreeUnitDeltaLead-kMinTwoUnitDeltaLead)<<16)-1;  

    UChar *ownedArray_;

    
    const UChar *uchars_;

    

    
    const UChar *pos_;
    
    int32_t remainingMatchLength_;
};

U_NAMESPACE_END

#endif  
