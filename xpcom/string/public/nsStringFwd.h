







































#ifndef nsStringFwd_h___
#define nsStringFwd_h___

#ifndef nscore_h___
#include "nscore.h"
#endif

#ifndef MOZILLA_INTERNAL_API
#error Internal string headers are not available from external-linkage code.
#endif

  



class nsAString;
class nsObsoleteAString;
#ifdef MOZ_V1_STRING_ABI
class nsSubstring;
#endif
class nsSubstringTuple;
class nsString;
class nsAutoString;
class nsDependentString;
class nsDependentSubstring;
class nsPromiseFlatString;
class nsStringComparator;
class nsDefaultStringComparator;
class nsXPIDLString;


  



class nsACString;
class nsObsoleteACString;
#ifdef MOZ_V1_STRING_ABI
class nsCSubstring;
#endif
class nsCSubstringTuple;
class nsCString;
class nsCAutoString;
class nsDependentCString;
class nsDependentCSubstring;
class nsPromiseFlatCString;
class nsCStringComparator;
class nsDefaultCStringComparator;
class nsXPIDLCString;


  



#ifndef MOZ_V1_STRING_ABI
typedef nsAString             nsSubstring;
typedef nsACString            nsCSubstring;
#endif

typedef nsString              nsAFlatString;
typedef nsSubstring           nsASingleFragmentString;

typedef nsCString             nsAFlatCString;
typedef nsCSubstring          nsASingleFragmentCString;

  
#endif 
