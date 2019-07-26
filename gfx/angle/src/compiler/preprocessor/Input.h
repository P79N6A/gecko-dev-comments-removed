





#ifndef COMPILER_PREPROCESSOR_INPUT_H_
#define COMPILER_PREPROCESSOR_INPUT_H_

#include <vector>

namespace pp
{


class Input
{
  public:
    Input();
    Input(int count, const char* const string[], const int length[]);

    int count() const { return mCount; }
    const char* string(int index) const { return mString[index]; }
    int length(int index) const { return mLength[index]; }

    int read(char* buf, int maxSize);

    struct Location
    {
        int sIndex;  
        int cIndex;  

        Location() : sIndex(0), cIndex(0) { }
    };
    const Location& readLoc() const { return mReadLoc; }

  private:
    
    int mCount;
    const char* const* mString;
    std::vector<int> mLength;

    Location mReadLoc;
};

}  
#endif  

