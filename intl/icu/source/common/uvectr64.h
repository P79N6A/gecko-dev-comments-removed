












#ifndef UVECTOR64_H
#define UVECTOR64_H

#include "unicode/utypes.h"
#include "unicode/uobject.h"
#include "uhash.h"
#include "uassert.h"

U_NAMESPACE_BEGIN



































class U_COMMON_API UVector64 : public UObject {
private:
    int32_t   count;

    int32_t   capacity;
    
    int32_t   maxCapacity;   

    int64_t*  elements;

public:
    UVector64(UErrorCode &status);

    UVector64(int32_t initialCapacity, UErrorCode &status);

    virtual ~UVector64();

    



    void assign(const UVector64& other, UErrorCode &ec);

    




    UBool operator==(const UVector64& other);

    


    inline UBool operator!=(const UVector64& other);

    
    
    

    void addElement(int64_t elem, UErrorCode &status);

    void setElementAt(int64_t elem, int32_t index);

    void insertElementAt(int64_t elem, int32_t index, UErrorCode &status);
    
    int64_t elementAti(int32_t index) const;

    

    int64_t lastElementi(void) const;

    

    

    

    

    

    

    void removeAllElements();

    int32_t size(void) const;

    

    
    inline UBool ensureCapacity(int32_t minimumCapacity, UErrorCode &status);

    
    UBool expandCapacity(int32_t minimumCapacity, UErrorCode &status);

    





    void setSize(int32_t newSize);

    
    
    

    


    

    


    int64_t *getBuffer() const;

    





    void setMaxCapacity(int32_t limit);

    


    static UClassID U_EXPORT2 getStaticClassID();

    


    virtual UClassID getDynamicClassID() const;

private:
    void _init(int32_t initialCapacity, UErrorCode &status);

    
    UVector64(const UVector64&);

    
    UVector64& operator=(const UVector64&);


    
    
    
public:
    

    
    
    int64_t popi(void);
    
    int64_t push(int64_t i, UErrorCode &status);

    int64_t *reserveBlock(int32_t size, UErrorCode &status);
    int64_t *popFrame(int32_t size);
};




inline UBool UVector64::ensureCapacity(int32_t minimumCapacity, UErrorCode &status) {
    if ((minimumCapacity >= 0) && (capacity >= minimumCapacity)) {
        return TRUE;
    } else {
        return expandCapacity(minimumCapacity, status);
    }
}

inline int64_t UVector64::elementAti(int32_t index) const {
    return (0 <= index && index < count) ? elements[index] : 0;
}


inline void UVector64::addElement(int64_t elem, UErrorCode &status) {
    if (ensureCapacity(count + 1, status)) {
        elements[count] = elem;
        count++;
    }
}

inline int64_t *UVector64::reserveBlock(int32_t size, UErrorCode &status) {
    if (ensureCapacity(count+size, status) == FALSE) {
        return NULL;
    }
    int64_t  *rp = elements+count;
    count += size;
    return rp;
}

inline int64_t *UVector64::popFrame(int32_t size) {
    U_ASSERT(count >= size);
    count -= size;
    if (count < 0) {
        count = 0;
    }
    return elements+count-size;
}



inline int32_t UVector64::size(void) const {
    return count;
}

inline int64_t UVector64::lastElementi(void) const {
    return elementAti(count-1);
}

inline UBool UVector64::operator!=(const UVector64& other) {
    return !operator==(other);
}

inline int64_t *UVector64::getBuffer() const {
    return elements;
}




inline int64_t UVector64::push(int64_t i, UErrorCode &status) {
    addElement(i, status);
    return i;
}

inline int64_t UVector64::popi(void) {
    int64_t result = 0;
    if (count > 0) {
        count--;
        result = elements[count];
    }
    return result;
}

U_NAMESPACE_END

#endif
