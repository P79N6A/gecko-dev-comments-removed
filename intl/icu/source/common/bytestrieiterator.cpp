













#include "unicode/utypes.h"
#include "unicode/bytestrie.h"
#include "unicode/stringpiece.h"
#include "charstr.h"
#include "uvectr32.h"

U_NAMESPACE_BEGIN

BytesTrie::Iterator::Iterator(const void *trieBytes, int32_t maxStringLength,
                              UErrorCode &errorCode)
        : bytes_(static_cast<const uint8_t *>(trieBytes)),
          pos_(bytes_), initialPos_(bytes_),
          remainingMatchLength_(-1), initialRemainingMatchLength_(-1),
          str_(NULL), maxLength_(maxStringLength), value_(0), stack_(NULL) {
    if(U_FAILURE(errorCode)) {
        return;
    }
    
    
    
    
    
    
    str_=new CharString();
    stack_=new UVector32(errorCode);
    if(U_SUCCESS(errorCode) && (str_==NULL || stack_==NULL)) {
        errorCode=U_MEMORY_ALLOCATION_ERROR;
    }
}

BytesTrie::Iterator::Iterator(const BytesTrie &trie, int32_t maxStringLength,
                              UErrorCode &errorCode)
        : bytes_(trie.bytes_), pos_(trie.pos_), initialPos_(trie.pos_),
          remainingMatchLength_(trie.remainingMatchLength_),
          initialRemainingMatchLength_(trie.remainingMatchLength_),
          str_(NULL), maxLength_(maxStringLength), value_(0), stack_(NULL) {
    if(U_FAILURE(errorCode)) {
        return;
    }
    str_=new CharString();
    stack_=new UVector32(errorCode);
    if(U_FAILURE(errorCode)) {
        return;
    }
    if(str_==NULL || stack_==NULL) {
        errorCode=U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    int32_t length=remainingMatchLength_;  
    if(length>=0) {
        
        ++length;
        if(maxLength_>0 && length>maxLength_) {
            length=maxLength_;  
        }
        str_->append(reinterpret_cast<const char *>(pos_), length, errorCode);
        pos_+=length;
        remainingMatchLength_-=length;
    }
}

BytesTrie::Iterator::~Iterator() {
    delete str_;
    delete stack_;
}

BytesTrie::Iterator &
BytesTrie::Iterator::reset() {
    pos_=initialPos_;
    remainingMatchLength_=initialRemainingMatchLength_;
    int32_t length=remainingMatchLength_+1;  
    if(maxLength_>0 && length>maxLength_) {
        length=maxLength_;
    }
    str_->truncate(length);
    pos_+=length;
    remainingMatchLength_-=length;
    stack_->setSize(0);
    return *this;
}

UBool
BytesTrie::Iterator::hasNext() const { return pos_!=NULL || !stack_->isEmpty(); }

UBool
BytesTrie::Iterator::next(UErrorCode &errorCode) {
    if(U_FAILURE(errorCode)) {
        return FALSE;
    }
    const uint8_t *pos=pos_;
    if(pos==NULL) {
        if(stack_->isEmpty()) {
            return FALSE;
        }
        
        
        int32_t stackSize=stack_->size();
        int32_t length=stack_->elementAti(stackSize-1);
        pos=bytes_+stack_->elementAti(stackSize-2);
        stack_->setSize(stackSize-2);
        str_->truncate(length&0xffff);
        length=(int32_t)((uint32_t)length>>16);
        if(length>1) {
            pos=branchNext(pos, length, errorCode);
            if(pos==NULL) {
                return TRUE;  
            }
        } else {
            str_->append((char)*pos++, errorCode);
        }
    }
    if(remainingMatchLength_>=0) {
        
        
        return truncateAndStop();
    }
    for(;;) {
        int32_t node=*pos++;
        if(node>=kMinValueLead) {
            
            UBool isFinal=(UBool)(node&kValueIsFinal);
            value_=readValue(pos, node>>1);
            if(isFinal || (maxLength_>0 && str_->length()==maxLength_)) {
                pos_=NULL;
            } else {
                pos_=skipValue(pos, node);
            }
            sp_.set(str_->data(), str_->length());
            return TRUE;
        }
        if(maxLength_>0 && str_->length()==maxLength_) {
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
            if(maxLength_>0 && str_->length()+length>maxLength_) {
                str_->append(reinterpret_cast<const char *>(pos),
                            maxLength_-str_->length(), errorCode);
                return truncateAndStop();
            }
            str_->append(reinterpret_cast<const char *>(pos), length, errorCode);
            pos+=length;
        }
    }
}

UBool
BytesTrie::Iterator::truncateAndStop() {
    pos_=NULL;
    sp_.set(str_->data(), str_->length());
    value_=-1;  
    return TRUE;
}


const uint8_t *
BytesTrie::Iterator::branchNext(const uint8_t *pos, int32_t length, UErrorCode &errorCode) {
    while(length>kMaxBranchLinearSubNodeLength) {
        ++pos;  
        
        stack_->addElement((int32_t)(skipDelta(pos)-bytes_), errorCode);
        stack_->addElement(((length-(length>>1))<<16)|str_->length(), errorCode);
        
        length>>=1;
        pos=jumpByDelta(pos);
    }
    
    
    uint8_t trieByte=*pos++;
    int32_t node=*pos++;
    UBool isFinal=(UBool)(node&kValueIsFinal);
    int32_t value=readValue(pos, node>>1);
    pos=skipValue(pos, node);
    stack_->addElement((int32_t)(pos-bytes_), errorCode);
    stack_->addElement(((length-1)<<16)|str_->length(), errorCode);
    str_->append((char)trieByte, errorCode);
    if(isFinal) {
        pos_=NULL;
        sp_.set(str_->data(), str_->length());
        value_=value;
        return NULL;
    } else {
        return pos+value;
    }
}

U_NAMESPACE_END
