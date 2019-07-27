





#ifndef _COMMON_INCLUDED_
#define _COMMON_INCLUDED_

#include <map>
#include <sstream>
#include <string>
#include <vector>
#include <limits>
#include <stdio.h>

#include "compiler/translator/PoolAlloc.h"
#include "compiler/translator/compilerdebug.h"
#include "common/angleutils.h"

struct TSourceLoc {
    int first_file;
    int first_line;
    int last_file;
    int last_line;
};




#define POOL_ALLOCATOR_NEW_DELETE()                                                  \
    void* operator new(size_t s) { return GetGlobalPoolAllocator()->allocate(s); }   \
    void* operator new(size_t, void *_Where) { return (_Where); }                    \
    void operator delete(void*) { }                                                  \
    void operator delete(void *, void *) { }                                         \
    void* operator new[](size_t s) { return GetGlobalPoolAllocator()->allocate(s); } \
    void* operator new[](size_t, void *_Where) { return (_Where); }                  \
    void operator delete[](void*) { }                                                \
    void operator delete[](void *, void *) { }




typedef pool_allocator<char> TStringAllocator;
typedef std::basic_string <char, std::char_traits<char>, TStringAllocator> TString;
typedef std::basic_ostringstream<char, std::char_traits<char>, TStringAllocator> TStringStream;
inline TString* NewPoolTString(const char* s)
{
	void* memory = GetGlobalPoolAllocator()->allocate(sizeof(TString));
	return new(memory) TString(s);
}





#define TPersistString std::string
#define TPersistStringStream std::ostringstream




template <class T> class TVector : public std::vector<T, pool_allocator<T> > {
public:
    typedef typename std::vector<T, pool_allocator<T> >::size_type size_type;
    TVector() : std::vector<T, pool_allocator<T> >() {}
    TVector(const pool_allocator<T>& a) : std::vector<T, pool_allocator<T> >(a) {}
    TVector(size_type i): std::vector<T, pool_allocator<T> >(i) {}
};

template <class K, class D, class CMP = std::less<K> > 
class TMap : public std::map<K, D, CMP, pool_allocator<std::pair<const K, D> > > {
public:
    typedef pool_allocator<std::pair<const K, D> > tAllocator;

    TMap() : std::map<K, D, CMP, tAllocator>() {}
    
    TMap(const tAllocator& a) : std::map<K, D, CMP, tAllocator>(std::map<K, D, CMP, tAllocator>::key_compare(), a) {}
};


template <typename T>
inline TString str(T i)
{
    ASSERT(std::numeric_limits<T>::is_integer);
    char buffer[((8 * sizeof(T)) / 3) + 3];
    const char *formatStr = std::numeric_limits<T>::is_signed ? "%d" : "%u";
    snprintf(buffer, sizeof(buffer), formatStr, i);
    return buffer;
}

#endif 
