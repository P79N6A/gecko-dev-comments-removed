

























#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/coleitr.h"
#include "unicode/ustring.h"
#include "ucol_imp.h"
#include "uassert.h"
#include "cmemory.h"




U_NAMESPACE_BEGIN

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(CollationElementIterator)



CollationElementIterator::CollationElementIterator(
                                         const CollationElementIterator& other) 
                                         : UObject(other), isDataOwned_(TRUE)
{
    UErrorCode status = U_ZERO_ERROR;
    m_data_ = ucol_openElements(other.m_data_->iteratordata_.coll, NULL, 0, 
                                &status);

    *this = other;
}

CollationElementIterator::~CollationElementIterator()
{
    if (isDataOwned_) {
        ucol_closeElements(m_data_);
    }
}



int32_t CollationElementIterator::getOffset() const
{
    return ucol_getOffset(m_data_);
}






int32_t CollationElementIterator::next(UErrorCode& status)
{
    return ucol_next(m_data_, &status);
}

UBool CollationElementIterator::operator!=(
                                  const CollationElementIterator& other) const
{
    return !(*this == other);
}

UBool CollationElementIterator::operator==(
                                    const CollationElementIterator& that) const
{
    if (this == &that || m_data_ == that.m_data_) {
        return TRUE;
    }

    
    if (m_data_->iteratordata_.coll != that.m_data_->iteratordata_.coll)
    {
        return FALSE;
    }

    
    
    
    int thislength = (int)(m_data_->iteratordata_.endp - m_data_->iteratordata_.string);
    int thatlength = (int)(that.m_data_->iteratordata_.endp - that.m_data_->iteratordata_.string);
    
    if (thislength != thatlength) {
        return FALSE;
    }

    if (uprv_memcmp(m_data_->iteratordata_.string, 
                    that.m_data_->iteratordata_.string, 
                    thislength * U_SIZEOF_UCHAR) != 0) {
        return FALSE;
    }
    if (getOffset() != that.getOffset()) {
        return FALSE;
    }

    
    if ((m_data_->iteratordata_.flags & UCOL_ITER_HASLEN) == 0) {
        if ((that.m_data_->iteratordata_.flags & UCOL_ITER_HASLEN) != 0) {
            return FALSE;
        }
        
        if (m_data_->iteratordata_.pos 
            - m_data_->iteratordata_.writableBuffer.getBuffer()
            != that.m_data_->iteratordata_.pos 
            - that.m_data_->iteratordata_.writableBuffer.getBuffer()) {
            
            return FALSE;
        }
    }
    else if ((that.m_data_->iteratordata_.flags & UCOL_ITER_HASLEN) == 0) {
        return FALSE;
    }
    
    return (m_data_->iteratordata_.CEpos - m_data_->iteratordata_.CEs)
            == (that.m_data_->iteratordata_.CEpos 
                                        - that.m_data_->iteratordata_.CEs);
}







int32_t CollationElementIterator::previous(UErrorCode& status)
{
    return ucol_previous(m_data_, &status);
}




void CollationElementIterator::reset()
{
    ucol_reset(m_data_);
}

void CollationElementIterator::setOffset(int32_t newOffset, 
                                         UErrorCode& status)
{
    ucol_setOffset(m_data_, newOffset, &status);
}




void CollationElementIterator::setText(const UnicodeString& source,
                                       UErrorCode& status)
{
    if (U_FAILURE(status)) {
        return;
    }

    int32_t length = source.length();
    UChar *string = NULL;
    if (m_data_->isWritable && m_data_->iteratordata_.string != NULL) {
        uprv_free((UChar *)m_data_->iteratordata_.string);
    }
    m_data_->isWritable = TRUE;
    if (length > 0) {
        string = (UChar *)uprv_malloc(U_SIZEOF_UCHAR * length);
        
        if (string == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
        u_memcpy(string, source.getBuffer(), length);
    }
    else {
        string = (UChar *)uprv_malloc(U_SIZEOF_UCHAR);
        
        if (string == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
        *string = 0;
    }
    
    ucol_freeOffsetBuffer(&(m_data_->iteratordata_));
    uprv_init_collIterate(m_data_->iteratordata_.coll, string, length, 
        &m_data_->iteratordata_, &status);

    m_data_->reset_   = TRUE;
}


void CollationElementIterator::setText(CharacterIterator& source, 
                                       UErrorCode& status)
{
    if (U_FAILURE(status)) 
        return;

    int32_t length = source.getLength();
    UChar *buffer = NULL;

    if (length == 0) {
        buffer = (UChar *)uprv_malloc(U_SIZEOF_UCHAR);
        
        if (buffer == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
        *buffer = 0;
    }
    else {
        buffer = (UChar *)uprv_malloc(U_SIZEOF_UCHAR * length);
        
        if (buffer == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
        



        UnicodeString string;
        source.getText(string);
        u_memcpy(buffer, string.getBuffer(), length);
    }

    if (m_data_->isWritable && m_data_->iteratordata_.string != NULL) {
        uprv_free((UChar *)m_data_->iteratordata_.string);
    }
    m_data_->isWritable = TRUE;
    
    ucol_freeOffsetBuffer(&(m_data_->iteratordata_));
    uprv_init_collIterate(m_data_->iteratordata_.coll, buffer, length, 
        &m_data_->iteratordata_, &status);
    m_data_->reset_   = TRUE;
}

int32_t CollationElementIterator::strengthOrder(int32_t order) const
{
    UCollationStrength s = ucol_getStrength(m_data_->iteratordata_.coll);
    
    if (s == UCOL_PRIMARY) {
        order &= RuleBasedCollator::PRIMARYDIFFERENCEONLY;
    }
    else if (s == UCOL_SECONDARY) {
        order &= RuleBasedCollator::SECONDARYDIFFERENCEONLY;
    }

    return order;
}







CollationElementIterator::CollationElementIterator(
                                               const UnicodeString& sourceText,
                                               const RuleBasedCollator* order,
                                               UErrorCode& status)
                                               : isDataOwned_(TRUE)
{
    if (U_FAILURE(status)) {
        return;
    }

    int32_t length = sourceText.length();
    UChar *string = NULL;

    if (length > 0) {
        string = (UChar *)uprv_malloc(U_SIZEOF_UCHAR * length);
        
        if (string == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
        



        u_memcpy(string, sourceText.getBuffer(), length);
    }
    else {
        string = (UChar *)uprv_malloc(U_SIZEOF_UCHAR);
        
        if (string == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
        *string = 0;
    }
    m_data_ = ucol_openElements(order->ucollator, string, length, &status);

    
    if (U_FAILURE(status)) {
        return;
    }
    m_data_->isWritable = TRUE;
}





CollationElementIterator::CollationElementIterator(
                                           const CharacterIterator& sourceText,
                                           const RuleBasedCollator* order,
                                           UErrorCode& status)
                                           : isDataOwned_(TRUE)
{
    if (U_FAILURE(status))
        return;

    
    

















    int32_t length = sourceText.getLength();
    UChar *buffer;
    if (length > 0) {
        buffer = (UChar *)uprv_malloc(U_SIZEOF_UCHAR * length);
        
        if (buffer == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
        



        UnicodeString string(buffer, length, length);
        ((CharacterIterator &)sourceText).getText(string);
        const UChar *temp = string.getBuffer();
        u_memcpy(buffer, temp, length);
    }
    else {
        buffer = (UChar *)uprv_malloc(U_SIZEOF_UCHAR);
        
        if (buffer == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
        *buffer = 0;
    }
    m_data_ = ucol_openElements(order->ucollator, buffer, length, &status);

    
    if (U_FAILURE(status)) {
        return;
    }
    m_data_->isWritable = TRUE;
}



const CollationElementIterator& CollationElementIterator::operator=(
                                         const CollationElementIterator& other)
{
    if (this != &other)
    {
        UCollationElements *ucolelem      = this->m_data_;
        UCollationElements *otherucolelem = other.m_data_;
        collIterate        *coliter       = &(ucolelem->iteratordata_);
        collIterate        *othercoliter  = &(otherucolelem->iteratordata_);
        int                length         = 0;

        
        
        length = (int)(othercoliter->endp - othercoliter->string);

        ucolelem->reset_         = otherucolelem->reset_;
        ucolelem->isWritable     = TRUE;

        
        if (length > 0) {
            coliter->string = (UChar *)uprv_malloc(length * U_SIZEOF_UCHAR);
            if(coliter->string != NULL) {
                uprv_memcpy((UChar *)coliter->string, othercoliter->string,
                    length * U_SIZEOF_UCHAR);
            } else { 
                length = 0;
            }
        }
        else {
            coliter->string = NULL;
        }

        
        coliter->endp = coliter->string == NULL ? NULL : coliter->string + length;

        

        if (othercoliter->flags & UCOL_ITER_INNORMBUF) {
            coliter->writableBuffer = othercoliter->writableBuffer;
            coliter->writableBuffer.getTerminatedBuffer();
        }

        
        if (othercoliter->pos >= othercoliter->string && 
            othercoliter->pos <= othercoliter->endp)
        {
            U_ASSERT(coliter->string != NULL);
            coliter->pos = coliter->string + 
                (othercoliter->pos - othercoliter->string);
        }
        else {
            coliter->pos = coliter->writableBuffer.getTerminatedBuffer() + 
                (othercoliter->pos - othercoliter->writableBuffer.getBuffer());
        }

        
        int32_t CEsize;
        if (coliter->extendCEs) {
            uprv_memcpy(coliter->CEs, othercoliter->CEs, sizeof(uint32_t) * UCOL_EXPAND_CE_BUFFER_SIZE);
            CEsize = sizeof(othercoliter->extendCEs);
            if (CEsize > 0) {
                othercoliter->extendCEs = (uint32_t *)uprv_malloc(CEsize);
                uprv_memcpy(coliter->extendCEs, othercoliter->extendCEs, CEsize);
            }
            coliter->toReturn = coliter->extendCEs + 
                (othercoliter->toReturn - othercoliter->extendCEs);
            coliter->CEpos    = coliter->extendCEs + CEsize;
        } else {
            CEsize = (int32_t)(othercoliter->CEpos - othercoliter->CEs);
            if (CEsize > 0) {
                uprv_memcpy(coliter->CEs, othercoliter->CEs, CEsize);
            }
            coliter->toReturn = coliter->CEs + 
                (othercoliter->toReturn - othercoliter->CEs);
            coliter->CEpos    = coliter->CEs + CEsize;
        }

        if (othercoliter->fcdPosition != NULL) {
            U_ASSERT(coliter->string != NULL);
            coliter->fcdPosition = coliter->string + 
                (othercoliter->fcdPosition 
                - othercoliter->string);
        }
        else {
            coliter->fcdPosition = NULL;
        }
        coliter->flags       = othercoliter->flags;
        coliter->origFlags   = othercoliter->origFlags;
        coliter->coll = othercoliter->coll;
        this->isDataOwned_ = TRUE;
    }

    return *this;
}

U_NAMESPACE_END

#endif 


