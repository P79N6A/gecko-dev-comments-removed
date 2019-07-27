





#ifndef nsStaticAtom_h__
#define nsStaticAtom_h__

#include "nsIAtom.h"
#include "nsStringBuffer.h"
#include "prlog.h"

typedef char16_t nsStaticAtomStringType;

#define NS_STATIC_ATOM(buffer_name, atom_ptr)  { (nsStringBuffer*) &buffer_name, atom_ptr }
#define NS_STATIC_ATOM_BUFFER(buffer_name, str_data) static nsFakeStringBuffer< sizeof(str_data) > buffer_name = { 1, sizeof(str_data) * sizeof(nsStaticAtomStringType), MOZ_UTF16(str_data) };






struct nsStaticAtom
{
  
  
  nsStringBuffer* MOZ_NON_OWNING_REF mStringBuffer;
  nsIAtom** mAtom;
};




template<uint32_t size>
struct nsFakeStringBuffer
{
  int32_t mRefCnt;
  uint32_t mSize;
  nsStaticAtomStringType mStringData[size];
};


template<uint32_t N>
nsresult
NS_RegisterStaticAtoms(const nsStaticAtom (&aAtoms)[N])
{
  extern nsresult RegisterStaticAtoms(const nsStaticAtom*, uint32_t aAtomCount);
  return RegisterStaticAtoms(aAtoms, N);
}

#endif
