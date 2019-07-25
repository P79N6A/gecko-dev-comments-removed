





#ifndef COMPILER_PREPROCESSOR_INPUT_H_
#define COMPILER_PREPROCESSOR_INPUT_H_

namespace pp
{



class Input
{
  public:
    Input(int count, const char* const string[], const int length[]);

    enum Error
    {
        kErrorNone,
        kErrorUnexpectedEOF
    };
    Error error() const { return mError; }

    
    int stringIndex() const { return mIndex; }
    
    bool eof() const;

    
    
    
    
    
    int read(char* buf, int bufSize);

private:
    enum State
    {
        kStateInitial,
        kStateLineComment,
        kStateBlockComment
    };

    int getChar();
    int peekChar();
    
    
    void switchToNextString();
    
    bool isStringEmpty(int index);
    
    
    int stringLength(int index);

    
    int mCount;
    const char* const* mString;
    const int* mLength;

    
    int mIndex;   
    int mSize;    

    
    Error mError;
    State mState;
};

}  
#endif

