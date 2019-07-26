





#include "Input.h"

#include <algorithm>
#include <cassert>
#include <cstring>

namespace pp
{

Input::Input() : mCount(0), mString(0)
{
}

Input::Input(int count, const char* const string[], const int length[]) :
    mCount(count),
    mString(string)
{
    assert(mCount >= 0);
    mLength.reserve(mCount);
    for (int i = 0; i < mCount; ++i)
    {
        int len = length ? length[i] : -1;
        mLength.push_back(len < 0 ? std::strlen(mString[i]) : len);
    }
}

int Input::read(char* buf, int maxSize)
{
    int nRead = 0;
    while ((nRead < maxSize) && (mReadLoc.sIndex < mCount))
    {
        int size = mLength[mReadLoc.sIndex] - mReadLoc.cIndex;
        size = std::min(size, maxSize);
        std::memcpy(buf + nRead, mString[mReadLoc.sIndex] + mReadLoc.cIndex, size);
        nRead += size;
        mReadLoc.cIndex += size;

        
        if (mReadLoc.cIndex == mLength[mReadLoc.sIndex])
        {
            ++mReadLoc.sIndex;
            mReadLoc.cIndex = 0;
        }
    }
    return nRead;
}

}  

