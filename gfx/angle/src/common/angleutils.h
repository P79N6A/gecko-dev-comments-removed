







#ifndef COMMON_ANGLEUTILS_H_
#define COMMON_ANGLEUTILS_H_

#include "common/platform.h"

#include <stddef.h>
#include <string>
#include <set>



#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&);               \
  void operator=(const TypeName&)




#ifdef ANGLE_NO_EXCEPTIONS
#define ANGLE_TRY if (true)
#define ANGLE_CATCH_ALL else
#else
#define ANGLE_TRY try
#define ANGLE_CATCH_ALL catch(...)
#endif

template <typename T, unsigned int N>
inline unsigned int ArraySize(T(&)[N])
{
    return N;
}

template <typename T, unsigned int N>
void SafeRelease(T (&resourceBlock)[N])
{
    for (unsigned int i = 0; i < N; i++)
    {
        SafeRelease(resourceBlock[i]);
    }
}

template <typename T>
void SafeRelease(T& resource)
{
    if (resource)
    {
        resource->Release();
        resource = NULL;
    }
}

template <typename T>
void SafeDelete(T*& resource)
{
    delete resource;
    resource = NULL;
}

template <typename T>
void SafeDeleteContainer(T& resource)
{
    for (typename T::iterator i = resource.begin(); i != resource.end(); i++)
    {
        SafeDelete(*i);
    }
    resource.clear();
}

template <typename T>
void SafeDeleteArray(T*& resource)
{
    delete[] resource;
    resource = NULL;
}



template <typename T>
inline bool StructLessThan(const T &a, const T &b)
{
    return (memcmp(&a, &b, sizeof(T)) < 0);
}



template <typename T>
inline bool StructEquals(const T &a, const T &b)
{
    return (memcmp(&a, &b, sizeof(T)) == 0);
}

template <typename T>
inline void StructZero(T *obj)
{
    memset(obj, 0, sizeof(T));
}

inline const char* MakeStaticString(const std::string &str)
{
    static std::set<std::string> strings;
    std::set<std::string>::iterator it = strings.find(str);
    if (it != strings.end())
    {
        return it->c_str();
    }

    return strings.insert(str).first->c_str();
}

#if defined(_MSC_VER)
#define snprintf _snprintf
#endif

#define VENDOR_ID_AMD 0x1002
#define VENDOR_ID_INTEL 0x8086
#define VENDOR_ID_NVIDIA 0x10DE

#define GL_BGRA4_ANGLEX 0x6ABC
#define GL_BGR5_A1_ANGLEX 0x6ABD
#define GL_INT_64_ANGLEX 0x6ABE
#define GL_STRUCT_ANGLEX 0x6ABF

#endif 
