






#ifndef nsStringFwd_h___
#define nsStringFwd_h___

#ifndef nscore_h___
#include "nscore.h"
#endif

#ifndef MOZILLA_INTERNAL_API
#error Internal string headers are not available from external-linkage code.
#endif

  



class nsAString;
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
class nsCSubstringTuple;
class nsCString;
class nsAutoCString;
class nsDependentCString;
class nsDependentCSubstring;
class nsPromiseFlatCString;
class nsCStringComparator;
class nsDefaultCStringComparator;
class nsXPIDLCString;


  



typedef nsAString             nsSubstring;
typedef nsACString            nsCSubstring;

typedef nsString              nsAFlatString;
typedef nsSubstring           nsASingleFragmentString;

typedef nsCString             nsAFlatCString;
typedef nsCSubstring          nsASingleFragmentCString;

  
#endif 
