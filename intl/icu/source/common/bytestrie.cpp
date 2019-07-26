













#include "unicode/utypes.h"
#include "unicode/bytestream.h"
#include "unicode/bytestrie.h"
#include "unicode/uobject.h"
#include "cmemory.h"
#include "uassert.h"

U_NAMESPACE_BEGIN

BytesTrie::~BytesTrie() {
    uprv_free(ownedArray_);
}


int32_t
BytesTrie::readValue(const uint8_t *pos, int32_t leadByte) {
    int32_t value;
    if(leadByte<kMinTwoByteValueLead) {
        value=leadByte-kMinOneByteValueLead;
    } else if(leadByte<kMinThreeByteValueLead) {
        value=((leadByte-kMinTwoByteValueLead)<<8)|*pos;
    } else if(leadByte<kFourByteValueLead) {
        value=((leadByte-kMinThreeByteValueLead)<<16)|(pos[0]<<8)|pos[1];
    } else if(leadByte==kFourByteValueLead) {
        value=(pos[0]<<16)|(pos[1]<<8)|pos[2];
    } else {
        value=(pos[0]<<24)|(pos[1]<<16)|(pos[2]<<8)|pos[3];
    }
    return value;
}

const uint8_t *
BytesTrie::jumpByDelta(const uint8_t *pos) {
    int32_t delta=*pos++;
    if(delta<kMinTwoByteDeltaLead) {
        
    } else if(delta<kMinThreeByteDeltaLead) {
        delta=((delta-kMinTwoByteDeltaLead)<<8)|*pos++;
    } else if(delta<kFourByteDeltaLead) {
        delta=((delta-kMinThreeByteDeltaLead)<<16)|(pos[0]<<8)|pos[1];
        pos+=2;
    } else if(delta==kFourByteDeltaLead) {
        delta=(pos[0]<<16)|(pos[1]<<8)|pos[2];
        pos+=3;
    } else {
        delta=(pos[0]<<24)|(pos[1]<<16)|(pos[2]<<8)|pos[3];
        pos+=4;
    }
    return pos+delta;
}

UStringTrieResult
BytesTrie::current() const {
    const uint8_t *pos=pos_;
    if(pos==NULL) {
        return USTRINGTRIE_NO_MATCH;
    } else {
        int32_t node;
        return (remainingMatchLength_<0 && (node=*pos)>=kMinValueLead) ?
                valueResult(node) : USTRINGTRIE_NO_VALUE;
    }
}

UStringTrieResult
BytesTrie::branchNext(const uint8_t *pos, int32_t length, int32_t inByte) {
    
    if(length==0) {
        length=*pos++;
    }
    ++length;
    
    
    while(length>kMaxBranchLinearSubNodeLength) {
        if(inByte<*pos++) {
            length>>=1;
            pos=jumpByDelta(pos);
        } else {
            length=length-(length>>1);
            pos=skipDelta(pos);
        }
    }
    
    
    
    do {
        if(inByte==*pos++) {
            UStringTrieResult result;
            int32_t node=*pos;
            U_ASSERT(node>=kMinValueLead);
            if(node&kValueIsFinal) {
                
                result=USTRINGTRIE_FINAL_VALUE;
            } else {
                
                ++pos;
                
                node>>=1;
                int32_t delta;
                if(node<kMinTwoByteValueLead) {
                    delta=node-kMinOneByteValueLead;
                } else if(node<kMinThreeByteValueLead) {
                    delta=((node-kMinTwoByteValueLead)<<8)|*pos++;
                } else if(node<kFourByteValueLead) {
                    delta=((node-kMinThreeByteValueLead)<<16)|(pos[0]<<8)|pos[1];
                    pos+=2;
                } else if(node==kFourByteValueLead) {
                    delta=(pos[0]<<16)|(pos[1]<<8)|pos[2];
                    pos+=3;
                } else {
                    delta=(pos[0]<<24)|(pos[1]<<16)|(pos[2]<<8)|pos[3];
                    pos+=4;
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
    if(inByte==*pos++) {
        pos_=pos;
        int32_t node=*pos;
        return node>=kMinValueLead ? valueResult(node) : USTRINGTRIE_NO_VALUE;
    } else {
        stop();
        return USTRINGTRIE_NO_MATCH;
    }
}

UStringTrieResult
BytesTrie::nextImpl(const uint8_t *pos, int32_t inByte) {
    for(;;) {
        int32_t node=*pos++;
        if(node<kMinLinearMatch) {
            return branchNext(pos, node, inByte);
        } else if(node<kMinValueLead) {
            
            int32_t length=node-kMinLinearMatch;  
            if(inByte==*pos++) {
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
            
            pos=skipValue(pos, node);
            
            U_ASSERT(*pos<kMinValueLead);
        }
    }
    stop();
    return USTRINGTRIE_NO_MATCH;
}

UStringTrieResult
BytesTrie::next(int32_t inByte) {
    const uint8_t *pos=pos_;
    if(pos==NULL) {
        return USTRINGTRIE_NO_MATCH;
    }
    if(inByte<0) {
        inByte+=0x100;
    }
    int32_t length=remainingMatchLength_;  
    if(length>=0) {
        
        if(inByte==*pos++) {
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
    return nextImpl(pos, inByte);
}

UStringTrieResult
BytesTrie::next(const char *s, int32_t sLength) {
    if(sLength<0 ? *s==0 : sLength==0) {
        
        return current();
    }
    const uint8_t *pos=pos_;
    if(pos==NULL) {
        return USTRINGTRIE_NO_MATCH;
    }
    int32_t length=remainingMatchLength_;  
    for(;;) {
        
        
        int32_t inByte;
        if(sLength<0) {
            for(;;) {
                if((inByte=*s++)==0) {
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
                if(inByte!=*pos) {
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
                inByte=*s++;
                --sLength;
                if(length<0) {
                    remainingMatchLength_=length;
                    break;
                }
                if(inByte!=*pos) {
                    stop();
                    return USTRINGTRIE_NO_MATCH;
                }
                ++pos;
                --length;
            }
        }
        for(;;) {
            int32_t node=*pos++;
            if(node<kMinLinearMatch) {
                UStringTrieResult result=branchNext(pos, node, inByte);
                if(result==USTRINGTRIE_NO_MATCH) {
                    return USTRINGTRIE_NO_MATCH;
                }
                
                if(sLength<0) {
                    if((inByte=*s++)==0) {
                        return result;
                    }
                } else {
                    if(sLength==0) {
                        return result;
                    }
                    inByte=*s++;
                    --sLength;
                }
                if(result==USTRINGTRIE_FINAL_VALUE) {
                    
                    stop();
                    return USTRINGTRIE_NO_MATCH;
                }
                pos=pos_;  
            } else if(node<kMinValueLead) {
                
                length=node-kMinLinearMatch;  
                if(inByte!=*pos) {
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
                
                pos=skipValue(pos, node);
                
                U_ASSERT(*pos<kMinValueLead);
            }
        }
    }
}

const uint8_t *
BytesTrie::findUniqueValueFromBranch(const uint8_t *pos, int32_t length,
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
        UBool isFinal=(UBool)(node&kValueIsFinal);
        int32_t value=readValue(pos, node>>1);
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
BytesTrie::findUniqueValue(const uint8_t *pos, UBool haveUniqueValue, int32_t &uniqueValue) {
    for(;;) {
        int32_t node=*pos++;
        if(node<kMinLinearMatch) {
            if(node==0) {
                node=*pos++;
            }
            pos=findUniqueValueFromBranch(pos, node+1, haveUniqueValue, uniqueValue);
            if(pos==NULL) {
                return FALSE;
            }
            haveUniqueValue=TRUE;
        } else if(node<kMinValueLead) {
            
            pos+=node-kMinLinearMatch+1;  
        } else {
            UBool isFinal=(UBool)(node&kValueIsFinal);
            int32_t value=readValue(pos, node>>1);
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
            pos=skipValue(pos, node);
        }
    }
}

int32_t
BytesTrie::getNextBytes(ByteSink &out) const {
    const uint8_t *pos=pos_;
    if(pos==NULL) {
        return 0;
    }
    if(remainingMatchLength_>=0) {
        append(out, *pos);  
        return 1;
    }
    int32_t node=*pos++;
    if(node>=kMinValueLead) {
        if(node&kValueIsFinal) {
            return 0;
        } else {
            pos=skipValue(pos, node);
            node=*pos++;
            U_ASSERT(node<kMinValueLead);
        }
    }
    if(node<kMinLinearMatch) {
        if(node==0) {
            node=*pos++;
        }
        getNextBranchBytes(pos, ++node, out);
        return node;
    } else {
        
        append(out, *pos);
        return 1;
    }
}

void
BytesTrie::getNextBranchBytes(const uint8_t *pos, int32_t length, ByteSink &out) {
    while(length>kMaxBranchLinearSubNodeLength) {
        ++pos;  
        getNextBranchBytes(jumpByDelta(pos), length>>1, out);
        length=length-(length>>1);
        pos=skipDelta(pos);
    }
    do {
        append(out, *pos++);
        pos=skipValue(pos);
    } while(--length>1);
    append(out, *pos);
}

void
BytesTrie::append(ByteSink &out, int c) {
    char ch=(char)c;
    out.Append(&ch, 1);
}

U_NAMESPACE_END
