









#include "uvector.h"
#include "cmemory.h"
#include "uarrsort.h"
#include "uelement.h"

U_NAMESPACE_BEGIN

#define DEFAULT_CAPACITY 8






#define HINT_KEY_POINTER   (1)
#define HINT_KEY_INTEGER   (0)
 
UOBJECT_DEFINE_RTTI_IMPLEMENTATION(UVector)

UVector::UVector(UErrorCode &status) :
    count(0),
    capacity(0),
    elements(0),
    deleter(0),
    comparer(0)
{
    _init(DEFAULT_CAPACITY, status);
}

UVector::UVector(int32_t initialCapacity, UErrorCode &status) :
    count(0),
    capacity(0),
    elements(0),
    deleter(0),
    comparer(0)
{
    _init(initialCapacity, status);
}

UVector::UVector(UObjectDeleter *d, UElementsAreEqual *c, UErrorCode &status) :
    count(0),
    capacity(0),
    elements(0),
    deleter(d),
    comparer(c)
{
    _init(DEFAULT_CAPACITY, status);
}

UVector::UVector(UObjectDeleter *d, UElementsAreEqual *c, int32_t initialCapacity, UErrorCode &status) :
    count(0),
    capacity(0),
    elements(0),
    deleter(d),
    comparer(c)
{
    _init(initialCapacity, status);
}

void UVector::_init(int32_t initialCapacity, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return;
    }
    
    if ((initialCapacity < 1) || (initialCapacity > (int32_t)(INT32_MAX / sizeof(UElement)))) {
        initialCapacity = DEFAULT_CAPACITY;
    }
    elements = (UElement *)uprv_malloc(sizeof(UElement)*initialCapacity);
    if (elements == 0) {
        status = U_MEMORY_ALLOCATION_ERROR;
    } else {
        capacity = initialCapacity;
    }
}

UVector::~UVector() {
    removeAllElements();
    uprv_free(elements);
    elements = 0;
}





void UVector::assign(const UVector& other, UElementAssigner *assign, UErrorCode &ec) {
    if (ensureCapacity(other.count, ec)) {
        setSize(other.count, ec);
        if (U_SUCCESS(ec)) {
            for (int32_t i=0; i<other.count; ++i) {
                if (elements[i].pointer != 0 && deleter != 0) {
                    (*deleter)(elements[i].pointer);
                }
                (*assign)(&elements[i], &other.elements[i]);
            }
        }
    }
}


UBool UVector::operator==(const UVector& other) {
    int32_t i;
    if (count != other.count) return FALSE;
    if (comparer != NULL) {
        
        for (i=0; i<count; ++i) {
            if (!(*comparer)(elements[i], other.elements[i])) {
                return FALSE;
            }
        }
    }
    return TRUE;
}

void UVector::addElement(void* obj, UErrorCode &status) {
    if (ensureCapacity(count + 1, status)) {
        elements[count++].pointer = obj;
    }
}

void UVector::addElement(int32_t elem, UErrorCode &status) {
    if (ensureCapacity(count + 1, status)) {
        elements[count].pointer = NULL;     
        elements[count].integer = elem;
        count++;
    }
}

void UVector::setElementAt(void* obj, int32_t index) {
    if (0 <= index && index < count) {
        if (elements[index].pointer != 0 && deleter != 0) {
            (*deleter)(elements[index].pointer);
        }
        elements[index].pointer = obj;
    }
    
}

void UVector::setElementAt(int32_t elem, int32_t index) {
    if (0 <= index && index < count) {
        if (elements[index].pointer != 0 && deleter != 0) {
            
            (*deleter)(elements[index].pointer);
        }
        elements[index].pointer = NULL;
        elements[index].integer = elem;
    }
    
}

void UVector::insertElementAt(void* obj, int32_t index, UErrorCode &status) {
    
    if (0 <= index && index <= count && ensureCapacity(count + 1, status)) {
        for (int32_t i=count; i>index; --i) {
            elements[i] = elements[i-1];
        }
        elements[index].pointer = obj;
        ++count;
    }
    
}

void UVector::insertElementAt(int32_t elem, int32_t index, UErrorCode &status) {
    
    if (0 <= index && index <= count && ensureCapacity(count + 1, status)) {
        for (int32_t i=count; i>index; --i) {
            elements[i] = elements[i-1];
        }
        elements[index].pointer = NULL;
        elements[index].integer = elem;
        ++count;
    }
    
}

void* UVector::elementAt(int32_t index) const {
    return (0 <= index && index < count) ? elements[index].pointer : 0;
}

int32_t UVector::elementAti(int32_t index) const {
    return (0 <= index && index < count) ? elements[index].integer : 0;
}

UBool UVector::containsAll(const UVector& other) const {
    for (int32_t i=0; i<other.size(); ++i) {
        if (indexOf(other.elements[i]) < 0) {
            return FALSE;
        }
    }
    return TRUE;
}

UBool UVector::containsNone(const UVector& other) const {
    for (int32_t i=0; i<other.size(); ++i) {
        if (indexOf(other.elements[i]) >= 0) {
            return FALSE;
        }
    }
    return TRUE;
}

UBool UVector::removeAll(const UVector& other) {
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

UBool UVector::retainAll(const UVector& other) {
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

void UVector::removeElementAt(int32_t index) {
    void* e = orphanElementAt(index);
    if (e != 0 && deleter != 0) {
        (*deleter)(e);
    }
}

UBool UVector::removeElement(void* obj) {
    int32_t i = indexOf(obj);
    if (i >= 0) {
        removeElementAt(i);
        return TRUE;
    }
    return FALSE;
}

void UVector::removeAllElements(void) {
    if (deleter != 0) {
        for (int32_t i=0; i<count; ++i) {
            if (elements[i].pointer != 0) {
                (*deleter)(elements[i].pointer);
            }
        }
    }
    count = 0;
}

UBool   UVector::equals(const UVector &other) const {
    int      i;

    if (this->count != other.count) {
        return FALSE;
    }
    if (comparer == 0) {
        for (i=0; i<count; i++) {
            if (elements[i].pointer != other.elements[i].pointer) {
                return FALSE;
            }
        }
    } else {
        UElement key;
        for (i=0; i<count; i++) {
            key.pointer = &other.elements[i];
            if (!(*comparer)(key, elements[i])) {
                return FALSE;
            }
        }
    }
    return TRUE;
}



int32_t UVector::indexOf(void* obj, int32_t startIndex) const {
    UElement key;
    key.pointer = obj;
    return indexOf(key, startIndex, HINT_KEY_POINTER);
}

int32_t UVector::indexOf(int32_t obj, int32_t startIndex) const {
    UElement key;
    key.integer = obj;
    return indexOf(key, startIndex, HINT_KEY_INTEGER);
}


int32_t UVector::indexOf(UElement key, int32_t startIndex, int8_t hint) const {
    int32_t i;
    if (comparer != 0) {
        for (i=startIndex; i<count; ++i) {
            if ((*comparer)(key, elements[i])) {
                return i;
            }
        }
    } else {
        for (i=startIndex; i<count; ++i) {
            


            if (hint & HINT_KEY_POINTER) {
                if (key.pointer == elements[i].pointer) {
                    return i;
                }
            } else {
                if (key.integer == elements[i].integer) {
                    return i;
                }
            }
        }
    }
    return -1;
}

UBool UVector::ensureCapacity(int32_t minimumCapacity, UErrorCode &status) {
	if (minimumCapacity < 0) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return FALSE;
	}
    if (capacity < minimumCapacity) {
        if (capacity > (INT32_MAX - 1) / 2) {        	
        	status = U_ILLEGAL_ARGUMENT_ERROR;
        	return FALSE;
        }
        int32_t newCap = capacity * 2;
        if (newCap < minimumCapacity) {
            newCap = minimumCapacity;
        }
        if (newCap > (int32_t)(INT32_MAX / sizeof(UElement))) {	
        	
        	status = U_ILLEGAL_ARGUMENT_ERROR;
        	return FALSE;
        }
        UElement* newElems = (UElement *)uprv_realloc(elements, sizeof(UElement)*newCap);
        if (newElems == NULL) {
            
            status = U_MEMORY_ALLOCATION_ERROR;
            return FALSE;
        }
        elements = newElems;
        capacity = newCap;
    }
    return TRUE;
}







void UVector::setSize(int32_t newSize, UErrorCode &status) {
    int32_t i;
    if (newSize < 0) {
        return;
    }
    if (newSize > count) {
        if (!ensureCapacity(newSize, status)) {
            return;
        }
        UElement empty;
        empty.pointer = NULL;
        empty.integer = 0;
        for (i=count; i<newSize; ++i) {
            elements[i] = empty;
        }
    } else {
        
        for (i=count-1; i>=newSize; --i) {
            removeElementAt(i);
        }
    }
    count = newSize;
}




void** UVector::toArray(void** result) const {
    void** a = result;
    for (int i=0; i<count; ++i) {
        *a++ = elements[i].pointer;
    }
    return result;
}

UObjectDeleter *UVector::setDeleter(UObjectDeleter *d) {
    UObjectDeleter *old = deleter;
    deleter = d;
    return old;
}

UElementsAreEqual *UVector::setComparer(UElementsAreEqual *d) {
    UElementsAreEqual *old = comparer;
    comparer = d;
    return old;
}










void* UVector::orphanElementAt(int32_t index) {
    void* e = 0;
    if (0 <= index && index < count) {
        e = elements[index].pointer;
        for (int32_t i=index; i<count-1; ++i) {
            elements[i] = elements[i+1];
        }
        --count;
    }
    
    return e;
}






void UVector::sortedInsert(void* obj, UElementComparator *compare, UErrorCode& ec) {
    UElement e;
    e.pointer = obj;
    sortedInsert(e, compare, ec);
}






void UVector::sortedInsert(int32_t obj, UElementComparator *compare, UErrorCode& ec) {
    UElement e;
    e.integer = obj;
    sortedInsert(e, compare, ec);
}


void UVector::sortedInsert(UElement e, UElementComparator *compare, UErrorCode& ec) {
    
    
    
    
    
    int32_t min = 0, max = count;
    while (min != max) {
        int32_t probe = (min + max) / 2;
        int8_t c = (*compare)(elements[probe], e);
        if (c > 0) {
            max = probe;
        } else {
            
            min = probe + 1;
        }
    }
    if (ensureCapacity(count + 1, ec)) {
        for (int32_t i=count; i>min; --i) {
            elements[i] = elements[i-1];
        }
        elements[min] = e;
        ++count;
    }
}












static int32_t U_CALLCONV
sortComparator(const void *context, const void *left, const void *right) {
    UElementComparator *compare = *static_cast<UElementComparator * const *>(context);
    UElement e1 = *static_cast<const UElement *>(left);
    UElement e2 = *static_cast<const UElement *>(right);
    int32_t result = (*compare)(e1, e2);
    return result;
}






static int32_t U_CALLCONV
sortiComparator(const void * , const void *left, const void *right) {
    const UElement *e1 = static_cast<const UElement *>(left);
    const UElement *e2 = static_cast<const UElement *>(right);
    int32_t result = e1->integer < e2->integer? -1 :
                     e1->integer == e2->integer? 0 : 1;
    return result;
}







void UVector::sorti(UErrorCode &ec) {
    if (U_SUCCESS(ec)) {
        uprv_sortArray(elements, count, sizeof(UElement),
                       sortiComparator, NULL,  FALSE, &ec);
    }
}
















void UVector::sort(UElementComparator *compare, UErrorCode &ec) {
    if (U_SUCCESS(ec)) {
        uprv_sortArray(elements, count, sizeof(UElement),
                       sortComparator, &compare, FALSE, &ec);
    }
}





void UVector::sortWithUComparator(UComparator *compare, const void *context, UErrorCode &ec) {
    if (U_SUCCESS(ec)) {
        uprv_sortArray(elements, count, sizeof(UElement),
                       compare, context, FALSE, &ec);
    }
}

U_NAMESPACE_END

