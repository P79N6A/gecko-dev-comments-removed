













#include "unicode/utypes.h"
#include "unicode/ucharstrie.h"
#include "unicode/unistr.h"
#include "uvectr32.h"

U_NAMESPACE_BEGIN

UCharsTrie::Iterator::Iterator(const UChar *trieUChars, int32_t maxStringLength,
                               UErrorCode &errorCode)
        : uchars_(trieUChars),
          pos_(uchars_), initialPos_(uchars_),
          remainingMatchLength_(-1), initialRemainingMatchLength_(-1),
          skipValue_(FALSE),
          maxLength_(maxStringLength), value_(0), stack_(NULL) {
    if(U_FAILURE(errorCode)) {
        return;
    }
    
    
    
    
    
    
    stack_=new UVector32(errorCode);
    if(stack_==NULL) {
        errorCode=U_MEMORY_ALLOCATION_ERROR;
    }
}

UCharsTrie::Iterator::Iterator(const UCharsTrie &trie, int32_t maxStringLength,
                               UErrorCode &errorCode)
        : uchars_(trie.uchars_), pos_(trie.pos_), initialPos_(trie.pos_),
          remainingMatchLength_(trie.remainingMatchLength_),
          initialRemainingMatchLength_(trie.remainingMatchLength_),
          skipValue_(FALSE),
          maxLength_(maxStringLength), value_(0), stack_(NULL) {
    if(U_FAILURE(errorCode)) {
        return;
    }
    stack_=new UVector32(errorCode);
    if(U_FAILURE(errorCode)) {
        return;
    }
    if(stack_==NULL) {
        errorCode=U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    int32_t length=remainingMatchLength_;  
    if(length>=0) {
        
        ++length;
        if(maxLength_>0 && length>maxLength_) {
            length=maxLength_;  
        }
        str_.append(pos_, length);
        pos_+=length;
        remainingMatchLength_-=length;
    }
}

UCharsTrie::Iterator::~Iterator() {
    delete stack_;
}

UCharsTrie::Iterator &
UCharsTrie::Iterator::reset() {
    pos_=initialPos_;
    remainingMatchLength_=initialRemainingMatchLength_;
    skipValue_=FALSE;
    int32_t length=remainingMatchLength_+1;  
    if(maxLength_>0 && length>maxLength_) {
        length=maxLength_;
    }
    str_.truncate(length);
    pos_+=length;
    remainingMatchLength_-=length;
    stack_->setSize(0);
    return *this;
}

UBool
UCharsTrie::Iterator::hasNext() const { return pos_!=NULL || !stack_->isEmpty(); }

UBool
UCharsTrie::Iterator::next(UErrorCode &errorCode) {
    if(U_FAILURE(errorCode)) {
        return FALSE;
    }
    const UChar *pos=pos_;
    if(pos==NULL) {
        if(stack_->isEmpty()) {
            return FALSE;
        }
        
        
        int32_t stackSize=stack_->size();
        int32_t length=stack_->elementAti(stackSize-1);
        pos=uchars_+stack_->elementAti(stackSize-2);
        stack_->setSize(stackSize-2);
        str_.truncate(length&0xffff);
        length=(int32_t)((uint32_t)length>>16);
        if(length>1) {
            pos=branchNext(pos, length, errorCode);
            if(pos==NULL) {
                return TRUE;  
            }
        } else {
            str_.append(*pos++);
        }
    }
    if(remainingMatchLength_>=0) {
        
        
        return truncateAndStop();
    }
    for(;;) {
        int32_t node=*pos++;
        if(node>=kMinValueLead) {
            if(skipValue_) {
                pos=skipNodeValue(pos, node);
                node&=kNodeTypeMask;
                skipValue_=FALSE;
            } else {
                
                UBool isFinal=(UBool)(node>>15);
                if(isFinal) {
                    value_=readValue(pos, node&0x7fff);
                } else {
                    value_=readNodeValue(pos, node);
                }
                if(isFinal || (maxLength_>0 && str_.length()==maxLength_)) {
                    pos_=NULL;
                } else {
                    
                    
                    
                    
                    pos_=pos-1;
                    skipValue_=TRUE;
                }
                return TRUE;
            }
        }
        if(maxLength_>0 && str_.length()==maxLength_) {
            return truncateAndStop();
        }
        if(node<kMinLinearMatch) {
            if(node==0) {
                node=*pos++;
            }
            pos=branchNext(pos, node+1, errorCode);
            if(pos==NULL) {
                return TRUE;  
            }
        } else {
            
            int32_t length=node-kMinLinearMatch+1;
            if(maxLength_>0 && str_.length()+length>maxLength_) {
                str_.append(pos, maxLength_-str_.length());
                return truncateAndStop();
            }
            str_.append(pos, length);
            pos+=length;
        }
    }
}


const UChar *
UCharsTrie::Iterator::branchNext(const UChar *pos, int32_t length, UErrorCode &errorCode) {
    while(length>kMaxBranchLinearSubNodeLength) {
        ++pos;  
        
        stack_->addElement((int32_t)(skipDelta(pos)-uchars_), errorCode);
        stack_->addElement(((length-(length>>1))<<16)|str_.length(), errorCode);
        
        length>>=1;
        pos=jumpByDelta(pos);
    }
    
    
    UChar trieUnit=*pos++;
    int32_t node=*pos++;
    UBool isFinal=(UBool)(node>>15);
    int32_t value=readValue(pos, node&=0x7fff);
    pos=skipValue(pos, node);
    stack_->addElement((int32_t)(pos-uchars_), errorCode);
    stack_->addElement(((length-1)<<16)|str_.length(), errorCode);
    str_.append(trieUnit);
    if(isFinal) {
        pos_=NULL;
        value_=value;
        return NULL;
    } else {
        return pos+value;
    }
}

U_NAMESPACE_END
