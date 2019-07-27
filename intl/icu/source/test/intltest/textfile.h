









#ifndef __ICU_INTLTEST_TEXTFILE__
#define __ICU_INTLTEST_TEXTFILE__

#include "intltest.h"
#include "filestrm.h"





class TextFile {
 public:
    




    TextFile(const char* name, const char* encoding, UErrorCode& ec);

    virtual ~TextFile();

    






    UBool readLine(UnicodeString& line, UErrorCode& ec);

    






    UBool readLineSkippingComments(UnicodeString& line, UErrorCode& ec,
                                   UBool trim = FALSE);

    


    inline int32_t getLineNumber() const;

 private:
    UBool ensureCapacity(int32_t capacity);
    UBool setBuffer(int32_t index, char c, UErrorCode& ec);

    FileStream* file;
    char* name;
    char* encoding;
    char* buffer;
    int32_t capacity;
    int32_t lineNo;
};

inline int32_t TextFile::getLineNumber() const {
    return lineNo;
}

#endif
