












#ifndef UVECTOR32_H
#define UVECTOR32_H

#include "unicode/utypes.h"
#include "unicode/uobject.h"
#include "uhash.h"
#include "uassert.h"

U_NAMESPACE_BEGIN




































class U_COMMON_API UVector32 : public UObject {
private:
    int32_t   count;

    int32_t   capacity;
    
    int32_t   maxCapacity;   

    int32_t*  elements;

public:
    UVector32(UErrorCode &status);

    UVector32(int32_t initialCapacity, UErrorCode &status);

    virtual ~UVector32();

    



    void assign(const UVector32& other, UErrorCode &ec);

    




    UBool operator==(const UVector32& other);

    


    inline UBool operator!=(const UVector32& other);

    
    
    

    void addElement(int32_t elem, UErrorCode &status);

    void setElementAt(int32_t elem, int32_t index);

    void insertElementAt(int32_t elem, int32_t index, UErrorCode &status);
    
    int32_t elementAti(int32_t index) const;

    UBool equals(const UVector32 &other) const;

    int32_t lastElementi(void) const;

    int32_t indexOf(int32_t elem, int32_t startIndex = 0) const;

    UBool contains(int32_t elem) const;

    UBool containsAll(const UVector32& other) const;

    UBool removeAll(const UVector32& other);

    UBool retainAll(const UVector32& other);

    void removeElementAt(int32_t index);

    void removeAllElements();

    int32_t size(void) const;

    UBool isEmpty(void) const;

    
    inline UBool ensureCapacity(int32_t minimumCapacity, UErrorCode &status);

    
    UBool expandCapacity(int32_t minimumCapacity, UErrorCode &status);

    





    void setSize(int32_t newSize);

    
    
    

    





    UBool containsNone(const UVector32& other) const;


    



    void sortedInsert(int32_t elem, UErrorCode& ec);

    


    int32_t *getBuffer() const;

    





    void setMaxCapacity(int32_t limit);

    


    static UClassID U_EXPORT2 getStaticClassID();

    


    virtual UClassID getDynamicClassID() const;

private:
    void _init(int32_t initialCapacity, UErrorCode &status);

    
    UVector32(const UVector32&);

    
    UVector32& operator=(const UVector32&);


    
    
    
public:
    UBool empty(void) const;   

    int32_t peeki(void) const;
    
    int32_t popi(void);
    
    int32_t push(int32_t i, UErrorCode &status);

    int32_t *reserveBlock(int32_t size, UErrorCode &status);
    int32_t *popFrame(int32_t size);
};




inline UBool UVector32::ensureCapacity(int32_t minimumCapacity, UErrorCode &status) {
    if ((minimumCapacity >= 0) && (capacity >= minimumCapacity)) {
        return TRUE;
    } else {
        return expandCapacity(minimumCapacity, status);
    }
}

inline int32_t UVector32::elementAti(int32_t index) const {
    return (index >= 0 && count > 0 && count - index > 0) ? elements[index] : 0;
}


inline void UVector32::addElement(int32_t elem, UErrorCode &status) {
    if (ensureCapacity(count + 1, status)) {
        elements[count] = elem;
        count++;
    }
}

inline int32_t *UVector32::reserveBlock(int32_t size, UErrorCode &status) {
    if (ensureCapacity(count+size, status) == FALSE) {
        return NULL;
    }
    int32_t  *rp = elements+count;
    count += size;
    return rp;
}

inline int32_t *UVector32::popFrame(int32_t size) {
    U_ASSERT(count >= size);
    count -= size;
    if (count < 0) {
        count = 0;
    }
    return elements+count-size;
}



inline int32_t UVector32::size(void) const {
    return count;
}

inline UBool UVector32::isEmpty(void) const {
    return count == 0;
}

inline UBool UVector32::contains(int32_t obj) const {
    return indexOf(obj) >= 0;
}

inline int32_t UVector32::lastElementi(void) const {
    return elementAti(count-1);
}

inline UBool UVector32::operator!=(const UVector32& other) {
    return !operator==(other);
}

inline int32_t *UVector32::getBuffer() const {
    return elements;
}




inline UBool UVector32::empty(void) const {
    return isEmpty();
}

inline int32_t UVector32::peeki(void) const {
    return lastElementi();
}

inline int32_t UVector32::push(int32_t i, UErrorCode &status) {
    addElement(i, status);
    return i;
}

inline int32_t UVector32::popi(void) {
    int32_t result = 0;
    if (count > 0) {
        count--;
        result = elements[count];
    }
    return result;
}

U_NAMESPACE_END

#endif
