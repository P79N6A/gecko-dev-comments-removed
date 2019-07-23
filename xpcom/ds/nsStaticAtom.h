





































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




template <PRUint32 size>
struct nsFakeStringBuffer {
    PRInt32 mRefCnt;
    PRUint32 mSize;
    nsStaticAtomStringType mStringData[size];
};


NS_COM nsresult
NS_RegisterStaticAtoms(const nsStaticAtom*, PRUint32 aAtomCount);

#endif
