















#ifndef _UTILS_TOKENIZER_H
#define _UTILS_TOKENIZER_H

#include <assert.h>
#include <utils/Errors.h>
#include <utils/FileMap.h>
#include "String8.h"

namespace android {




class Tokenizer {
    Tokenizer(const String8& filename, FileMap* fileMap, char* buffer, size_t length);

public:
    ~Tokenizer();

    





    static status_t open(const String8& filename, Tokenizer** outTokenizer);

    


    inline bool isEof() const { return mCurrent == getEnd(); }

    


    inline bool isEol() const { return isEof() || *mCurrent == '\n'; }

    


    inline String8 getFilename() const { return mFilename; }

    


    inline int32_t getLineNumber() const { return mLineNumber; }

    



    String8 getLocation() const;

    



    inline char peekChar() const { return isEof() ? '\0' : *mCurrent; }

    


    String8 peekRemainderOfLine() const;

    



    inline char nextChar() { return isEof() ? '\0' : *(mCurrent++); }

    






    String8 nextToken(const char* delimiters);

    



    void nextLine();

    



    void skipDelimiters(const char* delimiters);

private:
    Tokenizer(const Tokenizer& other); 

    String8 mFilename;
    FileMap* mFileMap;
    char* mBuffer;
    size_t mLength;

    const char* mCurrent;
    int32_t mLineNumber;

    inline const char* getEnd() const { return mBuffer + mLength; }

};

} 

#endif 
