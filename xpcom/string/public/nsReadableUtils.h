




#ifndef nsReadableUtils_h___
#define nsReadableUtils_h___






#ifndef nsAString_h___
#include "nsAString.h"
#endif

template<class E> class nsTArray;

inline size_t Distance( const nsReadingIterator<PRUnichar>& start, const nsReadingIterator<PRUnichar>& end )
  {
    return end.get() - start.get();
  }
inline size_t Distance( const nsReadingIterator<char>& start, const nsReadingIterator<char>& end )
  {
    return end.get() - start.get();
  }

void LossyCopyUTF16toASCII( const nsAString& aSource, nsACString& aDest );
void CopyASCIItoUTF16( const nsACString& aSource, nsAString& aDest );

void LossyCopyUTF16toASCII( const PRUnichar* aSource, nsACString& aDest );
void CopyASCIItoUTF16( const char* aSource, nsAString& aDest );

void CopyUTF16toUTF8( const nsAString& aSource, nsACString& aDest );
void CopyUTF8toUTF16( const nsACString& aSource, nsAString& aDest );

void CopyUTF16toUTF8( const PRUnichar* aSource, nsACString& aDest );
void CopyUTF8toUTF16( const char* aSource, nsAString& aDest );

void LossyAppendUTF16toASCII( const nsAString& aSource, nsACString& aDest );
void AppendASCIItoUTF16( const nsACString& aSource, nsAString& aDest );

void LossyAppendUTF16toASCII( const PRUnichar* aSource, nsACString& aDest );
void AppendASCIItoUTF16( const char* aSource, nsAString& aDest );

void AppendUTF16toUTF8( const nsAString& aSource, nsACString& aDest );
void AppendUTF8toUTF16( const nsACString& aSource, nsAString& aDest );
bool AppendUTF8toUTF16( const nsACString& aSource, nsAString& aDest,
                        const mozilla::fallible_t& ) NS_WARN_UNUSED_RESULT;

void AppendUTF16toUTF8( const PRUnichar* aSource, nsACString& aDest );
void AppendUTF8toUTF16( const char* aSource, nsAString& aDest );

  










char* ToNewCString( const nsAString& aSource );


  








char* ToNewCString( const nsACString& aSource );

  














char* ToNewUTF8String( const nsAString& aSource, uint32_t *aUTF8Count = nullptr );


  











PRUnichar* ToNewUnicode( const nsAString& aSource );


  










PRUnichar* ToNewUnicode( const nsACString& aSource );

  







uint32_t CalcUTF8ToUnicodeLength( const nsACString& aSource );

  

















PRUnichar* UTF8ToUnicodeBuffer( const nsACString& aSource,
                                PRUnichar *aBuffer,
                                uint32_t *aUTF16Count = nullptr );

  














PRUnichar* UTF8ToNewUnicode( const nsACString& aSource, uint32_t *aUTF16Count = nullptr );

  











PRUnichar* CopyUnicodeTo( const nsAString& aSource,
                                 uint32_t aSrcOffset,
                                 PRUnichar* aDest,
                                 uint32_t aLength );


  










void CopyUnicodeTo( const nsAString::const_iterator& aSrcStart,
                           const nsAString::const_iterator& aSrcEnd,
                           nsAString& aDest );

  









void AppendUnicodeTo( const nsAString::const_iterator& aSrcStart,
                             const nsAString::const_iterator& aSrcEnd,
                             nsAString& aDest );

  




bool IsASCII( const nsAString& aString );

  




bool IsASCII( const nsACString& aString );

  






























bool IsUTF8( const nsACString& aString, bool aRejectNonChar = true );

bool ParseString(const nsACString& aAstring, char aDelimiter, 
                          nsTArray<nsCString>& aArray);

  


void ToUpperCase( nsACString& );

void ToLowerCase( nsACString& );

void ToUpperCase( nsCSubstring& );

void ToLowerCase( nsCSubstring& );

  


void ToUpperCase( const nsACString& aSource, nsACString& aDest );

void ToLowerCase( const nsACString& aSource, nsACString& aDest );

  









bool FindInReadable( const nsAString& aPattern, nsAString::const_iterator&, nsAString::const_iterator&, const nsStringComparator& = nsDefaultStringComparator() );
bool FindInReadable( const nsACString& aPattern, nsACString::const_iterator&, nsACString::const_iterator&, const nsCStringComparator& = nsDefaultCStringComparator() );



inline bool FindInReadable( const nsAString& aPattern, const nsAString& aSource, const nsStringComparator& compare = nsDefaultStringComparator() )
{
  nsAString::const_iterator start, end;
  aSource.BeginReading(start);
  aSource.EndReading(end);
  return FindInReadable(aPattern, start, end, compare);
}

inline bool FindInReadable( const nsACString& aPattern, const nsACString& aSource, const nsCStringComparator& compare = nsDefaultCStringComparator() )
{
  nsACString::const_iterator start, end;
  aSource.BeginReading(start);
  aSource.EndReading(end);
  return FindInReadable(aPattern, start, end, compare);
}


bool CaseInsensitiveFindInReadable( const nsACString& aPattern, nsACString::const_iterator&, nsACString::const_iterator& );

  





bool RFindInReadable( const nsAString& aPattern, nsAString::const_iterator&, nsAString::const_iterator&, const nsStringComparator& = nsDefaultStringComparator() );
bool RFindInReadable( const nsACString& aPattern, nsACString::const_iterator&, nsACString::const_iterator&, const nsCStringComparator& = nsDefaultCStringComparator() );

   







bool FindCharInReadable( PRUnichar aChar, nsAString::const_iterator& aSearchStart, const nsAString::const_iterator& aSearchEnd );
bool FindCharInReadable( char aChar, nsACString::const_iterator& aSearchStart, const nsACString::const_iterator& aSearchEnd );

    


uint32_t CountCharInReadable( const nsAString& aStr,
                                     PRUnichar aChar );
uint32_t CountCharInReadable( const nsACString& aStr,
                                     char aChar );

bool
StringBeginsWith( const nsAString& aSource, const nsAString& aSubstring,
                  const nsStringComparator& aComparator =
                                              nsDefaultStringComparator() );
bool
StringBeginsWith( const nsACString& aSource, const nsACString& aSubstring,
                  const nsCStringComparator& aComparator =
                                               nsDefaultCStringComparator() );
bool
StringEndsWith( const nsAString& aSource, const nsAString& aSubstring,
                const nsStringComparator& aComparator =
                                            nsDefaultStringComparator() );
bool
StringEndsWith( const nsACString& aSource, const nsACString& aSubstring,
                const nsCStringComparator& aComparator =
                                             nsDefaultCStringComparator() );

const nsAFlatString& EmptyString();
const nsAFlatCString& EmptyCString();

const nsAFlatString& NullString();
const nsAFlatCString& NullCString();

   







int32_t
CompareUTF8toUTF16(const nsASingleFragmentCString& aUTF8String,
                   const nsASingleFragmentString& aUTF16String);

void
AppendUCS4ToUTF16(const uint32_t aSource, nsAString& aDest);

template<class T>
inline bool EnsureStringLength(T& aStr, uint32_t aLen)
{
    aStr.SetLength(aLen);
    return (aStr.Length() == aLen);
}

#endif 
