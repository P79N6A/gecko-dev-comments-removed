









#include "uvectr32.h"
#include "cmemory.h"
#include "putilimp.h"

U_NAMESPACE_BEGIN

#define DEFAULT_CAPACITY 8






 
UOBJECT_DEFINE_RTTI_IMPLEMENTATION(UVector32)

UVector32::UVector32(UErrorCode &status) :
    count(0),
    capacity(0),
    maxCapacity(0),
    elements(NULL)
{
    _init(DEFAULT_CAPACITY, status);
}

UVector32::UVector32(int32_t initialCapacity, UErrorCode &status) :
    count(0),
    capacity(0),
    maxCapacity(0),
    elements(0)
{
    _init(initialCapacity, status);
}



void UVector32::_init(int32_t initialCapacity, UErrorCode &status) {
    
    if (initialCapacity < 1) {
        initialCapacity = DEFAULT_CAPACITY;
    }
    if (maxCapacity>0 && maxCapacity<initialCapacity) {
        initialCapacity = maxCapacity;
    }
    if (initialCapacity > (int32_t)(INT32_MAX / sizeof(int32_t))) {
        initialCapacity = uprv_min(DEFAULT_CAPACITY, maxCapacity);
    }
    elements = (int32_t *)uprv_malloc(sizeof(int32_t)*initialCapacity);
    if (elements == 0) {
        status = U_MEMORY_ALLOCATION_ERROR;
    } else {
        capacity = initialCapacity;
    }
}

UVector32::~UVector32() {
    uprv_free(elements);
    elements = 0;
}




void UVector32::assign(const UVector32& other, UErrorCode &ec) {
    if (ensureCapacity(other.count, ec)) {
        setSize(other.count);
        for (int32_t i=0; i<other.count; ++i) {
            elements[i] = other.elements[i];
        }
    }
}


UBool UVector32::operator==(const UVector32& other) {
    int32_t i;
    if (count != other.count) return FALSE;
    for (i=0; i<count; ++i) {
        if (elements[i] != other.elements[i]) {
            return FALSE;
        }
    }
    return TRUE;
}


void UVector32::setElementAt(int32_t elem, int32_t index) {
    if (0 <= index && index < count) {
        elements[index] = elem;
    }
    
}

void UVector32::insertElementAt(int32_t elem, int32_t index, UErrorCode &status) {
    
    if (0 <= index && index <= count && ensureCapacity(count + 1, status)) {
        for (int32_t i=count; i>index; --i) {
            elements[i] = elements[i-1];
        }
        elements[index] = elem;
        ++count;
    }
    
}

UBool UVector32::containsAll(const UVector32& other) const {
    for (int32_t i=0; i<other.size(); ++i) {
        if (indexOf(other.elements[i]) < 0) {
            return FALSE;
        }
    }
    return TRUE;
}

UBool UVector32::containsNone(const UVector32& other) const {
    for (int32_t i=0; i<other.size(); ++i) {
        if (indexOf(other.elements[i]) >= 0) {
            return FALSE;
        }
    }
    return TRUE;
}

UBool UVector32::removeAll(const UVector32& other) {
    UBool changed = FALSE;
    for (int32_t i=0; i<other.size(); ++i) {
        int32_t j = indexOf(other.elements[i]);
        if (j >= 0) {
            removeElementAt(j);
            changed = TRUE;
        }
    }
    return changed;
}

UBool UVector32::retainAll(const UVector32& other) {
    UBool changed = FALSE;
    for (int32_t j=size()-1; j>=0; --j) {
        int32_t i = other.indexOf(elements[j]);
        if (i < 0) {
            removeElementAt(j);
            changed = TRUE;
        }
    }
    return changed;
}

void UVector32::removeElementAt(int32_t index) {
    if (index >= 0) {
        for (int32_t i=index; i<count-1; ++i) {
            elements[i] = elements[i+1];
        }
        --count;
    }
}

void UVector32::removeAllElements(void) {
    count = 0;
}

UBool   UVector32::equals(const UVector32 &other) const {
    int      i;

    if (this->count != other.count) {
        return FALSE;
    }
    for (i=0; i<count; i++) {
        if (elements[i] != other.elements[i]) {
            return FALSE;
        }
    }
    return TRUE;
}




int32_t UVector32::indexOf(int32_t key, int32_t startIndex) const {
    int32_t i;
    for (i=startIndex; i<count; ++i) {
        if (key == elements[i]) {
            return i;
        }
    }
    return -1;
}


UBool UVector32::expandCapacity(int32_t minimumCapacity, UErrorCode &status) {
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
    if (newCap > (int32_t)(INT32_MAX / sizeof(int32_t))) {  
        
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return FALSE;
    }
    int32_t* newElems = (int32_t *)uprv_realloc(elements, sizeof(int32_t)*newCap);
    if (newElems == NULL) {
        
        status = U_MEMORY_ALLOCATION_ERROR;
        return FALSE;
    }
    elements = newElems;
    capacity = newCap;
    return TRUE;
}

void UVector32::setMaxCapacity(int32_t limit) {
    U_ASSERT(limit >= 0);
    if (limit < 0) {
        limit = 0;
    }
    if (limit > (int32_t)(INT32_MAX / sizeof(int32_t))) {  
        
        return;
    }
    maxCapacity = limit;
    if (capacity <= maxCapacity || maxCapacity == 0) {
        
        return;
    }
    
    
    
    int32_t* newElems = (int32_t *)uprv_realloc(elements, sizeof(int32_t)*maxCapacity);
    if (newElems == NULL) {
        
        
        return;
    }
    elements = newElems;
    capacity = maxCapacity;
    if (count > capacity) {
        count = capacity;
    }
}







void UVector32::setSize(int32_t newSize) {
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









void UVector32::sortedInsert(int32_t tok, UErrorCode& ec) {
    
    
    
    
    
    int32_t min = 0, max = count;
    while (min != max) {
        int32_t probe = (min + max) / 2;
        
        
        if (elements[probe] > tok) {
            max = probe;
        } else {
            
            min = probe + 1;
        }
    }
    if (ensureCapacity(count + 1, ec)) {
        for (int32_t i=count; i>min; --i) {
            elements[i] = elements[i-1];
        }
        elements[min] = tok;
        ++count;
    }
}





U_NAMESPACE_END

