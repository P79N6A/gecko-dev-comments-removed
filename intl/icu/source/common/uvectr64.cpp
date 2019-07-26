






#include "uvectr64.h"
#include "cmemory.h"
#include "putilimp.h"

U_NAMESPACE_BEGIN

#define DEFAULT_CAPACITY 8






 
UOBJECT_DEFINE_RTTI_IMPLEMENTATION(UVector64)

UVector64::UVector64(UErrorCode &status) :
    count(0),
    capacity(0),
    maxCapacity(0),
    elements(NULL)
{
    _init(DEFAULT_CAPACITY, status);
}

UVector64::UVector64(int32_t initialCapacity, UErrorCode &status) :
    count(0),
    capacity(0),
    maxCapacity(0),
    elements(0)
{
    _init(initialCapacity, status);
}



void UVector64::_init(int32_t initialCapacity, UErrorCode &status) {
    
    if (initialCapacity < 1) {
        initialCapacity = DEFAULT_CAPACITY;
    }
    if (maxCapacity>0 && maxCapacity<initialCapacity) {
        initialCapacity = maxCapacity;
    }
    if (initialCapacity > (int32_t)(INT32_MAX / sizeof(int64_t))) {
        initialCapacity = uprv_min(DEFAULT_CAPACITY, maxCapacity);
    }
    elements = (int64_t *)uprv_malloc(sizeof(int64_t)*initialCapacity);
    if (elements == 0) {
        status = U_MEMORY_ALLOCATION_ERROR;
    } else {
        capacity = initialCapacity;
    }
}

UVector64::~UVector64() {
    uprv_free(elements);
    elements = 0;
}




void UVector64::assign(const UVector64& other, UErrorCode &ec) {
    if (ensureCapacity(other.count, ec)) {
        setSize(other.count);
        for (int32_t i=0; i<other.count; ++i) {
            elements[i] = other.elements[i];
        }
    }
}


UBool UVector64::operator==(const UVector64& other) {
    int32_t i;
    if (count != other.count) return FALSE;
    for (i=0; i<count; ++i) {
        if (elements[i] != other.elements[i]) {
            return FALSE;
        }
    }
    return TRUE;
}


void UVector64::setElementAt(int64_t elem, int32_t index) {
    if (0 <= index && index < count) {
        elements[index] = elem;
    }
    
}

void UVector64::insertElementAt(int64_t elem, int32_t index, UErrorCode &status) {
    
    if (0 <= index && index <= count && ensureCapacity(count + 1, status)) {
        for (int32_t i=count; i>index; --i) {
            elements[i] = elements[i-1];
        }
        elements[index] = elem;
        ++count;
    }
    
}

void UVector64::removeAllElements(void) {
    count = 0;
}

UBool UVector64::expandCapacity(int32_t minimumCapacity, UErrorCode &status) {
    if (minimumCapacity < 0) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return FALSE;
    }
    if (capacity >= minimumCapacity) {
        return TRUE;
    }
    if (maxCapacity>0 && minimumCapacity>maxCapacity) {
        status = U_BUFFER_OVERFLOW_ERROR;
        return FALSE;
    }
    if (capacity > (INT32_MAX - 1) / 2) {  
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return FALSE;
    }
    int32_t newCap = capacity * 2;
    if (newCap < minimumCapacity) {
        newCap = minimumCapacity;
    }
    if (maxCapacity > 0 && newCap > maxCapacity) {
        newCap = maxCapacity;
    }
    if (newCap > (int32_t)(INT32_MAX / sizeof(int64_t))) {  
        
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return FALSE;
    }
    int64_t* newElems = (int64_t *)uprv_realloc(elements, sizeof(int64_t)*newCap);
    if (newElems == NULL) {
        
        status = U_MEMORY_ALLOCATION_ERROR;
        return FALSE;
    }
    elements = newElems;
    capacity = newCap;
    return TRUE;
}

void UVector64::setMaxCapacity(int32_t limit) {
    U_ASSERT(limit >= 0);
    if (limit < 0) {
        limit = 0;
    }
    if (limit > (int32_t)(INT32_MAX / sizeof(int64_t))) {  
        
        return;
    }
    maxCapacity = limit;
    if (capacity <= maxCapacity || maxCapacity == 0) {
        
        return;
    }
    
    
    
    int64_t* newElems = (int64_t *)uprv_realloc(elements, sizeof(int64_t)*maxCapacity);
    if (newElems == NULL) {
        
        
        return;
    }
    elements = newElems;
    capacity = maxCapacity;
    if (count > capacity) {
        count = capacity;
    }
}







void UVector64::setSize(int32_t newSize) {
    int32_t i;
    if (newSize < 0) {
        return;
    }
    if (newSize > count) {
        UErrorCode ec = U_ZERO_ERROR;
        if (!ensureCapacity(newSize, ec)) {
            return;
        }
        for (i=count; i<newSize; ++i) {
            elements[i] = 0;
        }
    } 
    count = newSize;
}

U_NAMESPACE_END

