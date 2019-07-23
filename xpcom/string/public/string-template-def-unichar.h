





































#define CharT                               PRUnichar
#define CharT_is_PRUnichar                  1
#define nsTObsoleteAString_CharT            nsObsoleteAString
#define nsTObsoleteAStringThunk_CharT       nsObsoleteAStringThunk
#ifdef MOZ_V1_STRING_ABI
#define nsTAString_CharT                    nsAString
#else
#define nsTAString_CharT                    nsSubstring_base
#endif
#define nsTAString_IncompatibleCharT        nsACString
#define nsTString_CharT                     nsString
#define nsTFixedString_CharT                nsFixedString
#define nsTAutoString_CharT                 nsAutoString
#ifdef MOZ_V1_STRING_ABI
#define nsTSubstring_CharT                  nsSubstring
#else
#define nsTSubstring_CharT                  nsAString
#endif
#define nsTSubstringTuple_CharT             nsSubstringTuple
#define nsTStringComparator_CharT           nsStringComparator
#define nsTDefaultStringComparator_CharT    nsDefaultStringComparator
#define nsTDependentString_CharT            nsDependentString
#define nsTDependentSubstring_CharT         nsDependentSubstring
#define nsTXPIDLString_CharT                nsXPIDLString
#define nsTGetterCopies_CharT               nsGetterCopies
#define nsTAdoptingString_CharT             nsAdoptingString
#define nsTPromiseFlatString_CharT          nsPromiseFlatString
#define TPromiseFlatString_CharT            PromiseFlatString
