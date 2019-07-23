





































#include "nsSubstringTuple.h"

#if 0
  
#define TO_SUBSTRING(_v)                                        \
    ( (ptrdiff_t(_v) & 0x1)                                     \
        ? reinterpret_cast<const abstract_string_type*>(        \
            ((unsigned long)_v & ~0x1))->ToSubstring()          \
        : *reinterpret_cast<const substring_type*>((_v)) )
#endif

  
#ifdef MOZ_V1_STRING_ABI
#define TO_SUBSTRING(_v)                                        \
    ( (_v)->mVTable == obsolete_string_type::sCanonicalVTable   \
        ? *(_v)->AsSubstring()                                   \
        :  (_v)->ToSubstring() )
#else
#define TO_SUBSTRING(_v) (*(_v))
#endif

  
#include "string-template-def-unichar.h"
#include "nsTSubstringTuple.cpp"
#include "string-template-undef.h"

  
#include "string-template-def-char.h"
#include "nsTSubstringTuple.cpp"
#include "string-template-undef.h"
