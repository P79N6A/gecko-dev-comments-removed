






#ifndef __UNICODEREADER_H
#define __UNICODEREADER_H

#include "unicode/utypes.h"

#include "GUISupport.h"

class UnicodeReader
{
public:
    UnicodeReader()
    {
        
    }

    ~UnicodeReader()
    {
        
    }

    static const UChar *readFile(const char *fileName, GUISupport *guiSupport, int32_t &charCount);
};

#endif

