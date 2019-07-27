






#ifndef nsReadableUtils_h___
#define nsReadableUtils_h___






#include "mozilla/Assertions.h"
#include "nsAString.h"

#include "nsTArrayForwardDeclare.h"

inline size_t
Distance(const nsReadingIterator<char16_t>& aStart,
         const nsReadingIterator<char16_t>& aEnd)
{
  MOZ_ASSERT(aStart.get() <= aEnd.get());
  return static_cast<size_t>(aEnd.get() - aStart.get());
}
inline size_t
Distance(const nsReadingIterator<char>& aStart,
         const nsReadingIterator<char>& aEnd)
{
  MOZ_ASSERT(aStart.get() <= aEnd.get());
  return static_cast<size_t>(aEnd.get() - aStart.get());
}

void LossyCopyUTF16toASCII(const nsAString& aSource, nsACString& aDest);
void CopyASCIItoUTF16(const nsACString& aSource, nsAString& aDest);

void LossyCopyUTF16toASCII(const char16_t* aSource, nsACString& aDest);
void CopyASCIItoUTF16(const char* aSource, nsAString& aDest);

void CopyUTF16toUTF8(const nsAString& aSource, nsACString& aDest);
MOZ_WARN_UNUSED_RESULT bool CopyUTF16toUTF8(const nsAString& aSource,
                                            nsACString& aDest,
                                            const mozilla::fallible_t&);
void CopyUTF8toUTF16(const nsACString& aSource, nsAString& aDest);

void CopyUTF16toUTF8(const char16_t* aSource, nsACString& aDest);
void CopyUTF8toUTF16(const char* aSource, nsAString& aDest);

void LossyAppendUTF16toASCII(const nsAString& aSource, nsACString& aDest);
void AppendASCIItoUTF16(const nsACString& aSource, nsAString& aDest);
MOZ_WARN_UNUSED_RESULT bool AppendASCIItoUTF16(const nsACString& aSource,
                                               nsAString& aDest,
                                               const mozilla::fallible_t&);

void LossyAppendUTF16toASCII(const char16_t* aSource, nsACString& aDest);
MOZ_WARN_UNUSED_RESULT     bool AppendASCIItoUTF16(const char* aSource,
                                              nsAString& aDest,
                                              const mozilla::fallible_t&);
void AppendASCIItoUTF16(const char* aSource, nsAString& aDest);

void AppendUTF16toUTF8(const nsAString& aSource, nsACString& aDest);
MOZ_WARN_UNUSED_RESULT bool AppendUTF16toUTF8(const nsAString& aSource,
                                              nsACString& aDest,
                                              const mozilla::fallible_t&);
void AppendUTF8toUTF16(const nsACString& aSource, nsAString& aDest);
MOZ_WARN_UNUSED_RESULT bool AppendUTF8toUTF16(const nsACString& aSource,
                                              nsAString& aDest,
                                              const mozilla::fallible_t&);

void AppendUTF16toUTF8(const char16_t* aSource, nsACString& aDest);
void AppendUTF8toUTF16(const char* aSource, nsAString& aDest);

#ifdef MOZ_USE_CHAR16_WRAPPER
inline void AppendUTF16toUTF8(char16ptr_t aSource, nsACString& aDest)
{
  return AppendUTF16toUTF8(static_cast<const char16_t*>(aSource), aDest);
}
#endif












char* ToNewCString(const nsAString& aSource);











char* ToNewCString(const nsACString& aSource);
















char* ToNewUTF8String(const nsAString& aSource, uint32_t* aUTF8Count = nullptr);














char16_t* ToNewUnicode(const nsAString& aSource);













char16_t* ToNewUnicode(const nsACString& aSource);









uint32_t CalcUTF8ToUnicodeLength(const nsACString& aSource);



















char16_t* UTF8ToUnicodeBuffer(const nsACString& aSource,
                              char16_t* aBuffer,
                              uint32_t* aUTF16Count = nullptr);
















char16_t* UTF8ToNewUnicode(const nsACString& aSource,
                           uint32_t* aUTF16Count = nullptr);













char16_t* CopyUnicodeTo(const nsAString& aSource,
                        uint32_t aSrcOffset,
                        char16_t* aDest,
                        uint32_t aLength);













void CopyUnicodeTo(const nsAString::const_iterator& aSrcStart,
                   const nsAString::const_iterator& aSrcEnd,
                   nsAString& aDest);











void AppendUnicodeTo(const nsAString::const_iterator& aSrcStart,
                     const nsAString::const_iterator& aSrcEnd,
                     nsAString& aDest);






bool IsASCII(const nsAString& aString);






bool IsASCII(const nsACString& aString);
































bool IsUTF8(const nsACString& aString, bool aRejectNonChar = true);

bool ParseString(const nsACString& aAstring, char aDelimiter,
                 nsTArray<nsCString>& aArray);




void ToUpperCase(nsACString&);

void ToLowerCase(nsACString&);

void ToUpperCase(nsCSubstring&);

void ToLowerCase(nsCSubstring&);




void ToUpperCase(const nsACString& aSource, nsACString& aDest);

void ToLowerCase(const nsACString& aSource, nsACString& aDest);











bool FindInReadable(const nsAString& aPattern, nsAString::const_iterator&,
                    nsAString::const_iterator&,
                    const nsStringComparator& = nsDefaultStringComparator());
bool FindInReadable(const nsACString& aPattern, nsACString::const_iterator&,
                    nsACString::const_iterator&,
                    const nsCStringComparator& = nsDefaultCStringComparator());



inline bool
FindInReadable(const nsAString& aPattern, const nsAString& aSource,
               const nsStringComparator& aCompare = nsDefaultStringComparator())
{
  nsAString::const_iterator start, end;
  aSource.BeginReading(start);
  aSource.EndReading(end);
  return FindInReadable(aPattern, start, end, aCompare);
}

inline bool
FindInReadable(const nsACString& aPattern, const nsACString& aSource,
               const nsCStringComparator& aCompare = nsDefaultCStringComparator())
{
  nsACString::const_iterator start, end;
  aSource.BeginReading(start);
  aSource.EndReading(end);
  return FindInReadable(aPattern, start, end, aCompare);
}


bool CaseInsensitiveFindInReadable(const nsACString& aPattern,
                                   nsACString::const_iterator&,
                                   nsACString::const_iterator&);







bool RFindInReadable(const nsAString& aPattern, nsAString::const_iterator&,
                     nsAString::const_iterator&,
                     const nsStringComparator& = nsDefaultStringComparator());
bool RFindInReadable(const nsACString& aPattern, nsACString::const_iterator&,
                     nsACString::const_iterator&,
                     const nsCStringComparator& = nsDefaultCStringComparator());









bool FindCharInReadable(char16_t aChar, nsAString::const_iterator& aSearchStart,
                        const nsAString::const_iterator& aSearchEnd);
bool FindCharInReadable(char aChar, nsACString::const_iterator& aSearchStart,
                        const nsACString::const_iterator& aSearchEnd);




uint32_t CountCharInReadable(const nsAString& aStr,
                             char16_t aChar);
uint32_t CountCharInReadable(const nsACString& aStr,
                             char aChar);

bool StringBeginsWith(const nsAString& aSource, const nsAString& aSubstring,
                      const nsStringComparator& aComparator =
                        nsDefaultStringComparator());
bool StringBeginsWith(const nsACString& aSource, const nsACString& aSubstring,
                      const nsCStringComparator& aComparator =
                        nsDefaultCStringComparator());
bool StringEndsWith(const nsAString& aSource, const nsAString& aSubstring,
                    const nsStringComparator& aComparator =
                      nsDefaultStringComparator());
bool StringEndsWith(const nsACString& aSource, const nsACString& aSubstring,
                    const nsCStringComparator& aComparator =
                      nsDefaultCStringComparator());

const nsAFlatString& EmptyString();
const nsAFlatCString& EmptyCString();

const nsAFlatString& NullString();
const nsAFlatCString& NullCString();









int32_t CompareUTF8toUTF16(const nsASingleFragmentCString& aUTF8String,
                           const nsASingleFragmentString& aUTF16String);

void AppendUCS4ToUTF16(const uint32_t aSource, nsAString& aDest);

template<class T>
inline bool
EnsureStringLength(T& aStr, uint32_t aLen)
{
  aStr.SetLength(aLen);
  return (aStr.Length() == aLen);
}

#endif 
