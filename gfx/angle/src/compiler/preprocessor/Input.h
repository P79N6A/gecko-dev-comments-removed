





#ifndef COMPILER_PREPROCESSOR_INPUT_H_
#define COMPILER_PREPROCESSOR_INPUT_H_

#include <stddef.h>
#include <vector>

namespace pp
{


class Input
{
  public:
    Input();
    Input(size_t count, const char *const string[], const int length[]);

    size_t count() const
    {
        return mCount;
    }
    const char *string(size_t index) const
    {
        return mString[index];
    }
    size_t length(size_t index) const
    {
        return mLength[index];
    }

    size_t read(char *buf, size_t maxSize);

    struct Location
    {
        size_t sIndex;  
        size_t cIndex;  

        Location()
            : sIndex(0),
              cIndex(0)
        {
        }
    };
    const Location &readLoc() const { return mReadLoc; }

  private:
    
    size_t mCount;
    const char * const *mString;
    std::vector<size_t> mLength;

    Location mReadLoc;
};

}  
#endif  

