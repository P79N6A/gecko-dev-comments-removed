










#ifndef UVECTOR_H
#define UVECTOR_H

#include "unicode/utypes.h"
#include "unicode/uobject.h"
#include "cmemory.h"
#include "uarrsort.h"
#include "uelement.h"

U_NAMESPACE_BEGIN



















































class U_COMMON_API UVector : public UObject {
    
    
    
    
    
    

private:
    int32_t count;

    int32_t capacity;

    UElement* elements;

    UObjectDeleter *deleter;

    UElementsAreEqual *comparer;

public:
    UVector(UErrorCode &status);

    UVector(int32_t initialCapacity, UErrorCode &status);

    UVector(UObjectDeleter *d, UElementsAreEqual *c, UErrorCode &status);

    UVector(UObjectDeleter *d, UElementsAreEqual *c, int32_t initialCapacity, UErrorCode &status);

    virtual ~UVector();

    



    void assign(const UVector& other, UElementAssigner *assign, UErrorCode &ec);

    




    UBool operator==(const UVector& other);

    


    inline UBool operator!=(const UVector& other);

    
    
    

    void addElement(void* obj, UErrorCode &status);

    void addElement(int32_t elem, UErrorCode &status);

    void setElementAt(void* obj, int32_t index);

    void setElementAt(int32_t elem, int32_t index);

    void insertElementAt(void* obj, int32_t index, UErrorCode &status);

    void insertElementAt(int32_t elem, int32_t index, UErrorCode &status);
    
    void* elementAt(int32_t index) const;

    int32_t elementAti(int32_t index) const;

    UBool equals(const UVector &other) const;

    void* firstElement(void) const;

    void* lastElement(void) const;

    int32_t lastElementi(void) const;

    int32_t indexOf(void* obj, int32_t startIndex = 0) const;

    int32_t indexOf(int32_t obj, int32_t startIndex = 0) const;

    UBool contains(void* obj) const;

    UBool contains(int32_t obj) const;

    UBool containsAll(const UVector& other) const;

    UBool removeAll(const UVector& other);

    UBool retainAll(const UVector& other);

    void removeElementAt(int32_t index);

    UBool removeElement(void* obj);

    void removeAllElements();

    int32_t size(void) const;

    UBool isEmpty(void) const;

    UBool ensureCapacity(int32_t minimumCapacity, UErrorCode &status);

    





    void setSize(int32_t newSize, UErrorCode &status);

    


    void** toArray(void** result) const;

    
    
    

    UObjectDeleter *setDeleter(UObjectDeleter *d);

    UElementsAreEqual *setComparer(UElementsAreEqual *c);

    void* operator[](int32_t index) const;

    








    void* orphanElementAt(int32_t index);

    





    UBool containsNone(const UVector& other) const;

    




    void sortedInsert(void* obj, UElementComparator *compare, UErrorCode& ec);

    




    void sortedInsert(int32_t obj, UElementComparator *compare, UErrorCode& ec);

    



    void sorti(UErrorCode &ec);

    





    void sort(UElementComparator *compare, UErrorCode &ec);

    





    void sortWithUComparator(UComparator *compare, const void *context, UErrorCode &ec);

    


    static UClassID U_EXPORT2 getStaticClassID();

    


    virtual UClassID getDynamicClassID() const;

private:
    void _init(int32_t initialCapacity, UErrorCode &status);

    int32_t indexOf(UElement key, int32_t startIndex = 0, int8_t hint = 0) const;

    void sortedInsert(UElement e, UElementComparator *compare, UErrorCode& ec);

    
    UVector(const UVector&);

    
    UVector& operator=(const UVector&);

};


















class U_COMMON_API UStack : public UVector {
public:
    UStack(UErrorCode &status);

    UStack(int32_t initialCapacity, UErrorCode &status);

    UStack(UObjectDeleter *d, UElementsAreEqual *c, UErrorCode &status);

    UStack(UObjectDeleter *d, UElementsAreEqual *c, int32_t initialCapacity, UErrorCode &status);

    virtual ~UStack();

    
    

    UBool empty(void) const;

    void* peek(void) const;

    int32_t peeki(void) const;
    
    void* pop(void);
    
    int32_t popi(void);
    
    void* push(void* obj, UErrorCode &status);

    int32_t push(int32_t i, UErrorCode &status);

    



    int32_t search(void* obj) const;

    


    static UClassID U_EXPORT2 getStaticClassID();

    


    virtual UClassID getDynamicClassID() const;

private:
    
    UStack(const UStack&);

    
    UStack& operator=(const UStack&);
};




inline int32_t UVector::size(void) const {
    return count;
}

inline UBool UVector::isEmpty(void) const {
    return count == 0;
}

inline UBool UVector::contains(void* obj) const {
    return indexOf(obj) >= 0;
}

inline UBool UVector::contains(int32_t obj) const {
    return indexOf(obj) >= 0;
}

inline void* UVector::firstElement(void) const {
    return elementAt(0);
}

inline void* UVector::lastElement(void) const {
    return elementAt(count-1);
}

inline int32_t UVector::lastElementi(void) const {
    return elementAti(count-1);
}

inline void* UVector::operator[](int32_t index) const {
    return elementAt(index);
}

inline UBool UVector::operator!=(const UVector& other) {
    return !operator==(other);
}



inline UBool UStack::empty(void) const {
    return isEmpty();
}

inline void* UStack::peek(void) const {
    return lastElement();
}

inline int32_t UStack::peeki(void) const {
    return lastElementi();
}

inline void* UStack::push(void* obj, UErrorCode &status) {
    addElement(obj, status);
    return obj;
}

inline int32_t UStack::push(int32_t i, UErrorCode &status) {
    addElement(i, status);
    return i;
}

U_NAMESPACE_END

#endif
