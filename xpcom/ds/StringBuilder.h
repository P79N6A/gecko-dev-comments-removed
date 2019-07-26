













#include <stdlib.h>
#include <string.h>
#include "nsAlgorithm.h"















namespace mozilla {

class StringBuilder
{
public:
    StringBuilder() {
        mCapacity = 16;
        mLength = 0;
        mBuffer = static_cast<char*>(malloc(sizeof(char)*mCapacity));
        mBuffer[0] = '\0';
    }

    void Append(const char *s) {
        size_t newLength = strlen(s);

        EnsureCapacity(mLength + newLength+1);

        
        memcpy(&mBuffer[mLength], s, newLength+1);
        mLength += newLength;
    }

    char *Buffer() {
        return mBuffer;
    }

    size_t Length() {
        return mLength;
    }

    size_t EnsureCapacity(size_t capacity) {
        if (capacity > mCapacity) {
            
            mCapacity = XPCOM_MAX(capacity, mCapacity*2);
            mBuffer = static_cast<char*>(realloc(mBuffer, mCapacity));
            mCapacity = moz_malloc_usable_size(mBuffer);
        }
        return mCapacity;
    }

    ~StringBuilder()
    {
        free(mBuffer);
    }

private:
    char *mBuffer;
    size_t mLength; 
    size_t mCapacity; 
};

}
