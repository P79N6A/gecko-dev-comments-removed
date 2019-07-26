















#ifndef LOCAL_ARRAY_H_included
#define LOCAL_ARRAY_H_included

#include <cstddef>
#include <new>









template <size_t STACK_BYTE_COUNT>
class LocalArray {
public:
    





    LocalArray(size_t desiredByteCount) : mSize(desiredByteCount) {
        if (desiredByteCount > STACK_BYTE_COUNT) {
            mPtr = new char[mSize];
        } else {
            mPtr = &mOnStackBuffer[0];
        }
    }

    


    ~LocalArray() {
        if (mPtr != &mOnStackBuffer[0]) {
            delete[] mPtr;
        }
    }

    
    size_t size() { return mSize; }
    bool empty() { return mSize == 0; }

    
    char& operator[](size_t n) { return mPtr[n]; }
    const char& operator[](size_t n) const { return mPtr[n]; }

private:
    char mOnStackBuffer[STACK_BYTE_COUNT];
    char* mPtr;
    size_t mSize;

    
    LocalArray(const LocalArray&);
    void operator=(const LocalArray&);
};

#endif 
