




#ifndef nsStaticAtom_h__
#define nsStaticAtom_h__

#include "nsIAtom.h"
#include "nsStringBuffer.h"
#include "prlog.h"

#if defined(HAVE_CPP_CHAR16_T)
#define NS_STATIC_ATOM_USE_WIDE_STRINGS
typedef char16_t nsStaticAtomStringType;
#elif defined(HAVE_CPP_2BYTE_WCHAR_T)
#define NS_STATIC_ATOM_USE_WIDE_STRINGS
typedef wchar_t nsStaticAtomStringType;
#else
typedef char nsStaticAtomStringType;
#endif

#define NS_STATIC_ATOM(buffer_name, atom_ptr)  { (nsStringBuffer*) &buffer_name, atom_ptr }
#define NS_STATIC_ATOM_BUFFER(buffer_name, str_data) static nsFakeStringBuffer< sizeof(str_data) > buffer_name = { 1, sizeof(str_data) * sizeof(nsStaticAtomStringType), NS_L(str_data) };






struct nsStaticAtom {
    nsStringBuffer* mStringBuffer;
    nsIAtom ** mAtom;
};




template <uint32_t size>
struct nsFakeStringBuffer {
    int32_t mRefCnt;
    uint32_t mSize;
    nsStaticAtomStringType mStringData[size];
};


template<uint32_t N>
nsresult
NS_RegisterStaticAtoms(const nsStaticAtom (&atoms)[N])
{
    extern nsresult RegisterStaticAtoms(const nsStaticAtom*, uint32_t aAtomCount);
    return RegisterStaticAtoms(atoms, N);
}

#endif
