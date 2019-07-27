









#include "textfile.h"
#include "cmemory.h"
#include "cstring.h"
#include "intltest.h"
#include "util.h"





TextFile::TextFile(const char* _name, const char* _encoding, UErrorCode& ec) :
    file(0),
    name(0), encoding(0),
    buffer(0),
    capacity(0),
    lineNo(0)
{
    if (U_FAILURE(ec) || _name == 0 || _encoding == 0) {
        if (U_SUCCESS(ec)) {
            ec = U_ILLEGAL_ARGUMENT_ERROR; 
        }
        return;
    }

#ifdef CCP
    name = uprv_malloc(uprv_strlen(_name) + 1);
    encoding = uprv_malloc(uprv_strlen(_encoding) + 1);
    if (name == 0 || encoding == 0) {
        ec = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    uprv_strcpy(name, _name);
    uprv_strcpy(encoding, _encoding);
#else
    name = (char*) _name;
    encoding = (char*) _encoding;
#endif

    const char* testDir = IntlTest::getSourceTestData(ec);
    if (U_FAILURE(ec)) {
        return;
    }
    if (!ensureCapacity((int32_t)(uprv_strlen(testDir) + uprv_strlen(name) + 1))) {
        ec = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    uprv_strcpy(buffer, testDir);
    uprv_strcat(buffer, name);

    file = T_FileStream_open(buffer, "rb");
    if (file == 0) {
        ec = U_ILLEGAL_ARGUMENT_ERROR; 
        return;        
    }
}

TextFile::~TextFile() {
    if (file != 0) T_FileStream_close(file);
    if (buffer != 0) uprv_free(buffer);
#ifdef CCP
    uprv_free(name);
    uprv_free(encoding);
#endif
}

UBool TextFile::readLine(UnicodeString& line, UErrorCode& ec) {
    if (T_FileStream_eof(file)) {
        return FALSE;
    }
    
    
    
    
    
    int32_t n = 0;
    for (;;) {
        int c = T_FileStream_getc(file); 
        if (c < 0 || c == 0xD || c == 0xA) {
            
            if (c == 0xD) {
                c = T_FileStream_getc(file);
                if (c != 0xA && c >= 0) {
                    T_FileStream_ungetc(c, file);
                }
            }
            break;
        }
        if (!setBuffer(n++, c, ec)) return FALSE;
    }
    if (!setBuffer(n++, 0, ec)) return FALSE;
    UnicodeString str(buffer, encoding);
    
    if (lineNo == 0 && str[0] == 0xFEFF) {
        str.remove(0, 1);
    }
    ++lineNo;
    line = str.unescape();
    return TRUE;
}

UBool TextFile::readLineSkippingComments(UnicodeString& line, UErrorCode& ec,
                                         UBool trim) {
    for (;;) {
        if (!readLine(line, ec)) return FALSE;
        
        int32_t pos = 0;
        ICU_Utility::skipWhitespace(line, pos, TRUE);
        
        if (pos == line.length() || line.charAt(pos) == 0x23) {
            continue;
        }
        
        if (trim) line.remove(0, pos);
        return TRUE;
    }
}





UBool TextFile::setBuffer(int32_t index, char c, UErrorCode& ec) {
    if (capacity <= index) {
        if (!ensureCapacity(index+1)) {
            ec = U_MEMORY_ALLOCATION_ERROR;
            return FALSE;
        }
    }
    buffer[index] = c;
    return TRUE;
}






 #define LOWEST_MIN_CAPACITY 64
UBool TextFile::ensureCapacity(int32_t mincapacity) {
    if (capacity >= mincapacity) {
        return TRUE;
    }

    
    
    int32_t i = (capacity < LOWEST_MIN_CAPACITY)? LOWEST_MIN_CAPACITY: capacity;
    while (i < mincapacity) {
        i <<= 1;
        if (i < 0) {
            i = 0x7FFFFFFF;
            break;
        }
    }
    mincapacity = i;

    
    
    char* newbuffer = (char*) uprv_malloc(mincapacity);
    if (newbuffer == 0) {
        return FALSE;
    }
    if (buffer != 0) {
        uprv_strncpy(newbuffer, buffer, capacity);
        uprv_free(buffer);
    }
    buffer = newbuffer;
    capacity = mincapacity;
    return TRUE;
}

