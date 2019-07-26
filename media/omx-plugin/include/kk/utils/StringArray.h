


















#ifndef _LIBS_UTILS_STRING_ARRAY_H
#define _LIBS_UTILS_STRING_ARRAY_H

#include <stdlib.h>
#include <string.h>

namespace android {




class StringArray {
public:
    StringArray();
    virtual ~StringArray();

    
    
    
    bool push_back(const char* str);

    
    
    
    void erase(int idx);

    
    
    
    void sort(int (*compare)(const void*, const void*));
    
    
    
    
    static int cmpAscendingAlpha(const void* pstr1, const void* pstr2);
    
    
    
    
    inline int size(void) const { return mCurrent; }

    
    
    
    
    const char* getEntry(int idx) const {
        return (unsigned(idx) >= unsigned(mCurrent)) ? NULL : mArray[idx];
    }

    
    
    
    
    void setEntry(int idx, const char* str);

private:
    int     mMax;
    int     mCurrent;
    char**  mArray;
};

}; 

#endif 
