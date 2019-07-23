





































#define CharT                               char
#define CharT_is_char                       1
#define nsTObsoleteAString_CharT            nsObsoleteACString
#define nsTObsoleteAStringThunk_CharT       nsObsoleteACStringThunk
#ifdef MOZ_V1_STRING_ABI
#define nsTAString_CharT                    nsACString
#else
#define nsTAString_CharT                    nsCSubstring_base
#endif
#define nsTAString_IncompatibleCharT        nsAString
#define nsTString_CharT                     nsCString
#define nsTFixedString_CharT                nsFixedCString
#define nsTAutoString_CharT                 nsCAutoString
#ifdef MOZ_V1_STRING_ABI
#define nsTSubstring_CharT                  nsCSubstring
#else
#define nsTSubstring_CharT                  nsACString
#endif
#define nsTSubstringTuple_CharT             nsCSubstringTuple
#define nsTStringComparator_CharT           nsCStringComparator
#define nsTDefaultStringComparator_CharT    nsDefaultCStringComparator
#define nsTDependentString_CharT            nsDependentCString
#define nsTDependentSubstring_CharT         nsDependentCSubstring
#define nsTXPIDLString_CharT                nsXPIDLCString
#define nsTGetterCopies_CharT               nsCGetterCopies
#define nsTAdoptingString_CharT             nsAdoptingCString
#define nsTPromiseFlatString_CharT          nsPromiseFlatCString
#define TPromiseFlatString_CharT            PromiseFlatCString
