
























#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/coleitr.h"
#include "unicode/tblcoll.h"
#include "unicode/ustring.h"
#include "cmemory.h"
#include "collation.h"
#include "collationdata.h"
#include "collationiterator.h"
#include "collationsets.h"
#include "collationtailoring.h"
#include "uassert.h"
#include "uhash.h"
#include "utf16collationiterator.h"
#include "uvectr32.h"



U_NAMESPACE_BEGIN

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(CollationElementIterator)



CollationElementIterator::CollationElementIterator(
                                         const CollationElementIterator& other) 
        : UObject(other), iter_(NULL), rbc_(NULL), otherHalf_(0), dir_(0), offsets_(NULL) {
    *this = other;
}

CollationElementIterator::~CollationElementIterator()
{
    delete iter_;
    delete offsets_;
}



namespace {

uint32_t getFirstHalf(uint32_t p, uint32_t lower32) {
    return (p & 0xffff0000) | ((lower32 >> 16) & 0xff00) | ((lower32 >> 8) & 0xff);
}
uint32_t getSecondHalf(uint32_t p, uint32_t lower32) {
    return (p << 16) | ((lower32 >> 8) & 0xff00) | (lower32 & 0x3f);
}
UBool ceNeedsTwoParts(int64_t ce) {
    return (ce & INT64_C(0xffff00ff003f)) != 0;
}

}  

int32_t CollationElementIterator::getOffset() const
{
    if (dir_ < 0 && offsets_ != NULL && !offsets_->isEmpty()) {
        
        
        int32_t i = iter_->getCEsLength();
        if (otherHalf_ != 0) {
            
            ++i;
        }
        U_ASSERT(i < offsets_->size());
        return offsets_->elementAti(i);
    }
    return iter_->getOffset();
}






int32_t CollationElementIterator::next(UErrorCode& status)
{
    if (U_FAILURE(status)) { return NULLORDER; }
    if (dir_ > 1) {
        
        if (otherHalf_ != 0) {
            uint32_t oh = otherHalf_;
            otherHalf_ = 0;
            return oh;
        }
    } else if (dir_ == 1) {
        
        dir_ = 2;
    } else if (dir_ == 0) {
        
        dir_ = 2;
    } else  {
        
        status = U_INVALID_STATE_ERROR;
        return NULLORDER;
    }
    
    iter_->clearCEsIfNoneRemaining();
    int64_t ce = iter_->nextCE(status);
    if (ce == Collation::NO_CE) { return NULLORDER; }
    
    uint32_t p = (uint32_t)(ce >> 32);
    uint32_t lower32 = (uint32_t)ce;
    uint32_t firstHalf = getFirstHalf(p, lower32);
    uint32_t secondHalf = getSecondHalf(p, lower32);
    if (secondHalf != 0) {
        otherHalf_ = secondHalf | 0xc0;  
    }
    return firstHalf;
}

UBool CollationElementIterator::operator!=(
                                  const CollationElementIterator& other) const
{
    return !(*this == other);
}

UBool CollationElementIterator::operator==(
                                    const CollationElementIterator& that) const
{
    if (this == &that) {
        return TRUE;
    }

    return
        (rbc_ == that.rbc_ || *rbc_ == *that.rbc_) &&
        otherHalf_ == that.otherHalf_ &&
        normalizeDir() == that.normalizeDir() &&
        string_ == that.string_ &&
        *iter_ == *that.iter_;
}







int32_t CollationElementIterator::previous(UErrorCode& status)
{
    if (U_FAILURE(status)) { return NULLORDER; }
    if (dir_ < 0) {
        
        if (otherHalf_ != 0) {
            uint32_t oh = otherHalf_;
            otherHalf_ = 0;
            return oh;
        }
    } else if (dir_ == 0) {
        iter_->resetToOffset(string_.length());
        dir_ = -1;
    } else if (dir_ == 1) {
        
        dir_ = -1;
    } else  {
        
        status = U_INVALID_STATE_ERROR;
        return NULLORDER;
    }
    if (offsets_ == NULL) {
        offsets_ = new UVector32(status);
        if (offsets_ == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return NULLORDER;
        }
    }
    
    
    
    int32_t limitOffset = iter_->getCEsLength() == 0 ? iter_->getOffset() : 0;
    int64_t ce = iter_->previousCE(*offsets_, status);
    if (ce == Collation::NO_CE) { return NULLORDER; }
    
    uint32_t p = (uint32_t)(ce >> 32);
    uint32_t lower32 = (uint32_t)ce;
    uint32_t firstHalf = getFirstHalf(p, lower32);
    uint32_t secondHalf = getSecondHalf(p, lower32);
    if (secondHalf != 0) {
        if (offsets_->isEmpty()) {
            
            
            
            offsets_->addElement(iter_->getOffset(), status);
            offsets_->addElement(limitOffset, status);
        }
        otherHalf_ = firstHalf;
        return secondHalf | 0xc0;  
    }
    return firstHalf;
}




void CollationElementIterator::reset()
{
    iter_ ->resetToOffset(0);
    otherHalf_ = 0;
    dir_ = 0;
}

void CollationElementIterator::setOffset(int32_t newOffset, 
                                         UErrorCode& status)
{
    if (U_FAILURE(status)) { return; }
    if (0 < newOffset && newOffset < string_.length()) {
        int32_t offset = newOffset;
        do {
            UChar c = string_.charAt(offset);
            if (!rbc_->isUnsafe(c) ||
                    (U16_IS_LEAD(c) && !rbc_->isUnsafe(string_.char32At(offset)))) {
                break;
            }
            
            --offset;
        } while (offset > 0);
        if (offset < newOffset) {
            
            
            
            
            
            int32_t lastSafeOffset = offset;
            do {
                iter_->resetToOffset(lastSafeOffset);
                do {
                    iter_->nextCE(status);
                    if (U_FAILURE(status)) { return; }
                } while ((offset = iter_->getOffset()) == lastSafeOffset);
                if (offset <= newOffset) {
                    lastSafeOffset = offset;
                }
            } while (offset < newOffset);
            newOffset = lastSafeOffset;
        }
    }
    iter_->resetToOffset(newOffset);
    otherHalf_ = 0;
    dir_ = 1;
}




void CollationElementIterator::setText(const UnicodeString& source,
                                       UErrorCode& status)
{
    if (U_FAILURE(status)) {
        return;
    }

    string_ = source;
    const UChar *s = string_.getBuffer();
    CollationIterator *newIter;
    UBool numeric = rbc_->settings->isNumeric();
    if (rbc_->settings->dontCheckFCD()) {
        newIter = new UTF16CollationIterator(rbc_->data, numeric, s, s, s + string_.length());
    } else {
        newIter = new FCDUTF16CollationIterator(rbc_->data, numeric, s, s, s + string_.length());
    }
    if (newIter == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    delete iter_;
    iter_ = newIter;
    otherHalf_ = 0;
    dir_ = 0;
}


void CollationElementIterator::setText(CharacterIterator& source, 
                                       UErrorCode& status)
{
    if (U_FAILURE(status)) 
        return;

    source.getText(string_);
    setText(string_, status);
}

int32_t CollationElementIterator::strengthOrder(int32_t order) const
{
    UColAttributeValue s = (UColAttributeValue)rbc_->settings->getStrength();
    
    if (s == UCOL_PRIMARY) {
        order &= 0xffff0000;
    }
    else if (s == UCOL_SECONDARY) {
        order &= 0xffffff00;
    }

    return order;
}







CollationElementIterator::CollationElementIterator(
                                               const UnicodeString &source,
                                               const RuleBasedCollator *coll,
                                               UErrorCode &status)
        : iter_(NULL), rbc_(coll), otherHalf_(0), dir_(0), offsets_(NULL) {
    setText(source, status);
}





CollationElementIterator::CollationElementIterator(
                                           const CharacterIterator &source,
                                           const RuleBasedCollator *coll,
                                           UErrorCode &status)
        : iter_(NULL), rbc_(coll), otherHalf_(0), dir_(0), offsets_(NULL) {
    
    setText(const_cast<CharacterIterator &>(source), status);
}



const CollationElementIterator& CollationElementIterator::operator=(
                                         const CollationElementIterator& other)
{
    if (this == &other) {
        return *this;
    }

    CollationIterator *newIter;
    const FCDUTF16CollationIterator *otherFCDIter =
            dynamic_cast<const FCDUTF16CollationIterator *>(other.iter_);
    if(otherFCDIter != NULL) {
        newIter = new FCDUTF16CollationIterator(*otherFCDIter, string_.getBuffer());
    } else {
        const UTF16CollationIterator *otherIter =
                dynamic_cast<const UTF16CollationIterator *>(other.iter_);
        if(otherIter != NULL) {
            newIter = new UTF16CollationIterator(*otherIter, string_.getBuffer());
        } else {
            newIter = NULL;
        }
    }
    if(newIter != NULL) {
        delete iter_;
        iter_ = newIter;
        rbc_ = other.rbc_;
        otherHalf_ = other.otherHalf_;
        dir_ = other.dir_;

        string_ = other.string_;
    }
    if(other.dir_ < 0 && other.offsets_ != NULL && !other.offsets_->isEmpty()) {
        UErrorCode errorCode = U_ZERO_ERROR;
        if(offsets_ == NULL) {
            offsets_ = new UVector32(other.offsets_->size(), errorCode);
        }
        if(offsets_ != NULL) {
            offsets_->assign(*other.offsets_, errorCode);
        }
    }
    return *this;
}

namespace {

class MaxExpSink : public ContractionsAndExpansions::CESink {
public:
    MaxExpSink(UHashtable *h, UErrorCode &ec) : maxExpansions(h), errorCode(ec) {}
    virtual ~MaxExpSink();
    virtual void handleCE(int64_t ) {}
    virtual void handleExpansion(const int64_t ces[], int32_t length) {
        if (length <= 1) {
            
            return;
        }
        int32_t count = 0;  
        for (int32_t i = 0; i < length; ++i) {
            count += ceNeedsTwoParts(ces[i]) ? 2 : 1;
        }
        
        int64_t ce = ces[length - 1];
        uint32_t p = (uint32_t)(ce >> 32);
        uint32_t lower32 = (uint32_t)ce;
        uint32_t lastHalf = getSecondHalf(p, lower32);
        if (lastHalf == 0) {
            lastHalf = getFirstHalf(p, lower32);
            U_ASSERT(lastHalf != 0);
        } else {
            lastHalf |= 0xc0;  
        }
        if (count > uhash_igeti(maxExpansions, (int32_t)lastHalf)) {
            uhash_iputi(maxExpansions, (int32_t)lastHalf, count, &errorCode);
        }
    }

private:
    UHashtable *maxExpansions;
    UErrorCode &errorCode;
};

MaxExpSink::~MaxExpSink() {}

}  

UHashtable *
CollationElementIterator::computeMaxExpansions(const CollationData *data, UErrorCode &errorCode) {
    if (U_FAILURE(errorCode)) { return NULL; }
    UHashtable *maxExpansions = uhash_open(uhash_hashLong, uhash_compareLong,
                                           uhash_compareLong, &errorCode);
    if (U_FAILURE(errorCode)) { return NULL; }
    MaxExpSink sink(maxExpansions, errorCode);
    ContractionsAndExpansions(NULL, NULL, &sink, TRUE).forData(data, errorCode);
    if (U_FAILURE(errorCode)) {
        uhash_close(maxExpansions);
        return NULL;
    }
    return maxExpansions;
}

int32_t
CollationElementIterator::getMaxExpansion(int32_t order) const {
    return getMaxExpansion(rbc_->tailoring->maxExpansions, order);
}

int32_t
CollationElementIterator::getMaxExpansion(const UHashtable *maxExpansions, int32_t order) {
    if (order == 0) { return 1; }
    int32_t max;
    if(maxExpansions != NULL && (max = uhash_igeti(maxExpansions, order)) != 0) {
        return max;
    }
    if ((order & 0xc0) == 0xc0) {
        
        return 2;
    } else {
        return 1;
    }
}

U_NAMESPACE_END

#endif 
