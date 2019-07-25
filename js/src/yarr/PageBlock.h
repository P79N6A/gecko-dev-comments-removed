




























#ifndef PageBlock_h
#define PageBlock_h

#include <stdlib.h>
#include "jsstdint.h"
#include "assembler/wtf/Platform.h"

namespace WTF {

size_t pageSize();
inline bool isPageAligned(void* address) { return !(reinterpret_cast<intptr_t>(address) & (pageSize() - 1)); }
inline bool isPageAligned(size_t size) { return !(size & (pageSize() - 1)); }
inline bool isPowerOfTwo(size_t size) { return !(size & (size - 1)); }

class PageBlock {
public:
    PageBlock();
    PageBlock(const PageBlock&);
    PageBlock(void*, size_t);
    
    void* base() const { return m_base; }
    size_t size() const { return m_size; }

    operator bool() const { return !!m_base; }

    bool contains(void* containedBase, size_t containedSize)
    {
        return containedBase >= m_base
            && (static_cast<char*>(containedBase) + containedSize) <= (static_cast<char*>(m_base) + m_size);
    }

private:
    void* m_base;
    size_t m_size;
};

inline PageBlock::PageBlock()
    : m_base(0)
    , m_size(0)
{
}

inline PageBlock::PageBlock(const PageBlock& other)
    : m_base(other.m_base)
    , m_size(other.m_size)
{
}

inline PageBlock::PageBlock(void* base, size_t size)
    : m_base(base)
    , m_size(size)
{
}

} 

using WTF::pageSize;
using WTF::isPageAligned;
using WTF::isPageAligned;
using WTF::isPowerOfTwo;

#endif 
