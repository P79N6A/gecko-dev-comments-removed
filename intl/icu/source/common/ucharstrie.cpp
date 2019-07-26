













#include "unicode/utypes.h"
#include "unicode/appendable.h"
#include "unicode/ucharstrie.h"
#include "unicode/uobject.h"
#include "unicode/utf16.h"
#include "cmemory.h"
#include "uassert.h"

U_NAMESPACE_BEGIN

UCharsTrie::~UCharsTrie() {
    uprv_free(ownedArray_);
}

UStringTrieResult
UCharsTrie::current() const {
    const UChar *pos=pos_;
    if(pos==NULL) {
        return USTRINGTRIE_NO_MATCH;
    } else {
        int32_t node;
        return (remainingMatchLength_<0 && (node=*pos)>=kMinValueLead) ?
                valueResult(node) : USTRINGTRIE_NO_VALUE;
    }
}

UStringTrieResult
UCharsTrie::firstForCodePoint(UChar32 cp) {
    return cp<=0xffff ?
        first(cp) :
        (USTRINGTRIE_HAS_NEXT(first(U16_LEAD(cp))) ?
            next(U16_TRAIL(cp)) :
            USTRINGTRIE_NO_MATCH);
}

UStringTrieResult
UCharsTrie::nextForCodePoint(UChar32 cp) {
    return cp<=0xffff ?
        next(cp) :
        (USTRINGTRIE_HAS_NEXT(next(U16_LEAD(cp))) ?
            next(U16_TRAIL(cp)) :
            USTRINGTRIE_NO_MATCH);
}

UStringTrieResult
UCharsTrie::branchNext(const UChar *pos, int32_t length, int32_t uchar) {
    
    if(length==0) {
        length=*pos++;
    }
    ++length;
    
    
    while(length>kMaxBranchLinearSubNodeLength) {
        if(uchar<*pos++) {
            length>>=1;
            pos=jumpByDelta(pos);
        } else {
            length=length-(length>>1);
            pos=skipDelta(pos);
        }
    }
    
    
    
    do {
        if(uchar==*pos++) {
            UStringTrieResult result;
            int32_t node=*pos;
            if(node&kValueIsFinal) {
                
                result=USTRINGTRIE_FINAL_VALUE;
            } else {
                
                ++pos;
                
                int32_t delta;
                if(node<kMinTwoUnitValueLead) {
                    delta=node;
                } else if(node<kThreeUnitValueLead) {
                    delta=((node-kMinTwoUnitValueLead)<<16)|*pos++;
                } else {
                    delta=(pos[0]<<16)|pos[1];
                    pos+=2;
                }
                
                pos+=delta;
                node=*pos;
                result= node>=kMinValueLead ? valueResult(node) : USTRINGTRIE_NO_VALUE;
            }
            pos_=pos;
            return result;
        }
        --length;
        pos=skipValue(pos);
    } while(length>1);
    if(uchar==*pos++) {
        pos_=pos;
        int32_t node=*pos;
        return node>=kMinValueLead ? valueResult(node) : USTRINGTRIE_NO_VALUE;
    } else {
        stop();
        return USTRINGTRIE_NO_MATCH;
    }
}

UStringTrieResult
UCharsTrie::nextImpl(const UChar *pos, int32_t uchar) {
    int32_t node=*pos++;
    for(;;) {
        if(node<kMinLinearMatch) {
            return branchNext(pos, node, uchar);
        } else if(node<kMinValueLead) {
            
            int32_t length=node-kMinLinearMatch;  
            if(uchar==*pos++) {
                remainingMatchLength_=--length;
                pos_=pos;
                return (length<0 && (node=*pos)>=kMinValueLead) ?
                        valueResult(node) : USTRINGTRIE_NO_VALUE;
            } else {
                
                break;
            }
        } else if(node&kValueIsFinal) {
            
            break;
        } else {
            
            pos=skipNodeValue(pos, node);
            node&=kNodeTypeMask;
        }
    }
    stop();
    return USTRINGTRIE_NO_MATCH;
}

UStringTrieResult
UCharsTrie::next(int32_t uchar) {
    const UChar *pos=pos_;
    if(pos==NULL) {
        return USTRINGTRIE_NO_MATCH;
    }
    int32_t length=remainingMatchLength_;  
    if(length>=0) {
        
        if(uchar==*pos++) {
            remainingMatchLength_=--length;
            pos_=pos;
            int32_t node;
            return (length<0 && (node=*pos)>=kMinValueLead) ?
                    valueResult(node) : USTRINGTRIE_NO_VALUE;
        } else {
            stop();
            return USTRINGTRIE_NO_MATCH;
        }
    }
    return nextImpl(pos, uchar);
}

UStringTrieResult
UCharsTrie::next(const UChar *s, int32_t sLength) {
    if(sLength<0 ? *s==0 : sLength==0) {
        
        return current();
    }
    const UChar *pos=pos_;
    if(pos==NULL) {
        return USTRINGTRIE_NO_MATCH;
    }
    int32_t length=remainingMatchLength_;  
    for(;;) {
        
        
        int32_t uchar;
        if(sLength<0) {
            for(;;) {
                if((uchar=*s++)==0) {
                    remainingMatchLength_=length;
                    pos_=pos;
                    int32_t node;
                    return (length<0 && (node=*pos)>=kMinValueLead) ?
                            valueResult(node) : USTRINGTRIE_NO_VALUE;
                }
                if(length<0) {
                    remainingMatchLength_=length;
                    break;
                }
                if(uchar!=*pos) {
                    stop();
                    return USTRINGTRIE_NO_MATCH;
                }
                ++pos;
                --length;
            }
        } else {
            for(;;) {
                if(sLength==0) {
                    remainingMatchLength_=length;
                    pos_=pos;
                    int32_t node;
                    return (length<0 && (node=*pos)>=kMinValueLead) ?
                            valueResult(node) : USTRINGTRIE_NO_VALUE;
                }
                uchar=*s++;
                --sLength;
                if(length<0) {
                    remainingMatchLength_=length;
                    break;
                }
                if(uchar!=*pos) {
                    stop();
                    return USTRINGTRIE_NO_MATCH;
                }
                ++pos;
                --length;
            }
        }
        int32_t node=*pos++;
        for(;;) {
            if(node<kMinLinearMatch) {
                UStringTrieResult result=branchNext(pos, node, uchar);
                if(result==USTRINGTRIE_NO_MATCH) {
                    return USTRINGTRIE_NO_MATCH;
                }
                
                if(sLength<0) {
                    if((uchar=*s++)==0) {
                        return result;
                    }
                } else {
                    if(sLength==0) {
                        return result;
                    }
                    uchar=*s++;
                    --sLength;
                }
                if(result==USTRINGTRIE_FINAL_VALUE) {
                    
                    stop();
                    return USTRINGTRIE_NO_MATCH;
                }
                pos=pos_;  
                node=*pos++;
            } else if(node<kMinValueLead) {
                
                length=node-kMinLinearMatch;  
                if(uchar!=*pos) {
                    stop();
                    return USTRINGTRIE_NO_MATCH;
                }
                ++pos;
                --length;
                break;
            } else if(node&kValueIsFinal) {
                
                stop();
                return USTRINGTRIE_NO_MATCH;
            } else {
                
                pos=skipNodeValue(pos, node);
                node&=kNodeTypeMask;
            }
        }
    }
}

const UChar *
UCharsTrie::findUniqueValueFromBranch(const UChar *pos, int32_t length,
                                      UBool haveUniqueValue, int32_t &uniqueValue) {
    while(length>kMaxBranchLinearSubNodeLength) {
        ++pos;  
        if(NULL==findUniqueValueFromBranch(jumpByDelta(pos), length>>1, haveUniqueValue, uniqueValue)) {
            return NULL;
        }
        length=length-(length>>1);
        pos=skipDelta(pos);
    }
    do {
        ++pos;  
        
        int32_t node=*pos++;
        UBool isFinal=(UBool)(node>>15);
        node&=0x7fff;
        int32_t value=readValue(pos, node);
        pos=skipValue(pos, node);
        if(isFinal) {
            if(haveUniqueValue) {
                if(value!=uniqueValue) {
                    return NULL;
                }
            } else {
                uniqueValue=value;
                haveUniqueValue=TRUE;
            }
        } else {
            if(!findUniqueValue(pos+value, haveUniqueValue, uniqueValue)) {
                return NULL;
            }
            haveUniqueValue=TRUE;
        }
    } while(--length>1);
    return pos+1;  
}

UBool
UCharsTrie::findUniqueValue(const UChar *pos, UBool haveUniqueValue, int32_t &uniqueValue) {
    int32_t node=*pos++;
    for(;;) {
        if(node<kMinLinearMatch) {
            if(node==0) {
                node=*pos++;
            }
            pos=findUniqueValueFromBranch(pos, node+1, haveUniqueValue, uniqueValue);
            if(pos==NULL) {
                return FALSE;
            }
            haveUniqueValue=TRUE;
            node=*pos++;
        } else if(node<kMinValueLead) {
            
            pos+=node-kMinLinearMatch+1;  
            node=*pos++;
        } else {
            UBool isFinal=(UBool)(node>>15);
            int32_t value;
            if(isFinal) {
                value=readValue(pos, node&0x7fff);
            } else {
                value=readNodeValue(pos, node);
            }
            if(haveUniqueValue) {
                if(value!=uniqueValue) {
                    return FALSE;
                }
            } else {
                uniqueValue=value;
                haveUniqueValue=TRUE;
            }
            if(isFinal) {
                return TRUE;
            }
            pos=skipNodeValue(pos, node);
            node&=kNodeTypeMask;
        }
    }
}

int32_t
UCharsTrie::getNextUChars(Appendable &out) const {
    const UChar *pos=pos_;
    if(pos==NULL) {
        return 0;
    }
    if(remainingMatchLength_>=0) {
        out.appendCodeUnit(*pos);  
        return 1;
    }
    int32_t node=*pos++;
    if(node>=kMinValueLead) {
        if(node&kValueIsFinal) {
            return 0;
        } else {
            pos=skipNodeValue(pos, node);
            node&=kNodeTypeMask;
        }
    }
    if(node<kMinLinearMatch) {
        if(node==0) {
            node=*pos++;
        }
        out.reserveAppendCapacity(++node);
        getNextBranchUChars(pos, node, out);
        return node;
    } else {
        
        out.appendCodeUnit(*pos);
        return 1;
    }
}

void
UCharsTrie::getNextBranchUChars(const UChar *pos, int32_t length, Appendable &out) {
    while(length>kMaxBranchLinearSubNodeLength) {
        ++pos;  
        getNextBranchUChars(jumpByDelta(pos), length>>1, out);
        length=length-(length>>1);
        pos=skipDelta(pos);
    }
    do {
        out.appendCodeUnit(*pos++);
        pos=skipValue(pos);
    } while(--length>1);
    out.appendCodeUnit(*pos);
}

U_NAMESPACE_END
