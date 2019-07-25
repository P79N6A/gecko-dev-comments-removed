






































#ifndef nsReadableUtils_h___
#define nsReadableUtils_h___






#ifndef nsAString_h___
#include "nsAString.h"
#endif
#include "nsTArray.h"

inline size_t Distance( const nsReadingIterator<PRUnichar>& start, const nsReadingIterator<PRUnichar>& end )
  {
    return end.get() - start.get();
  }
inline size_t Distance( const nsReadingIterator<char>& start, const nsReadingIterator<char>& end )
  {
    return end.get() - start.get();
  }

void LossyCopyUTF16toASCII( const nsAString& aSource, nsACString& aDest NS_OUTPARAM );
void CopyASCIItoUTF16( const nsACString& aSource, nsAString& aDest NS_OUTPARAM );

void LossyCopyUTF16toASCII( const PRUnichar* aSource, nsACString& aDest NS_OUTPARAM );
void CopyASCIItoUTF16( const char* aSource, nsAString& aDest NS_OUTPARAM );

void CopyUTF16toUTF8( const nsAString& aSource, nsACString& aDest NS_OUTPARAM );
void CopyUTF8toUTF16( const nsACString& aSource, nsAString& aDest NS_OUTPARAM );

void CopyUTF16toUTF8( const PRUnichar* aSource, nsACString& aDest NS_OUTPARAM );
void CopyUTF8toUTF16( const char* aSource, nsAString& aDest NS_OUTPARAM );

void LossyAppendUTF16toASCII( const nsAString& aSource, nsACString& aDest );
void AppendASCIItoUTF16( const nsACString& aSource, nsAString& aDest );

void LossyAppendUTF16toASCII( const PRUnichar* aSource, nsACString& aDest );
void AppendASCIItoUTF16( const char* aSource, nsAString& aDest );

void AppendUTF16toUTF8( const nsAString& aSource, nsACString& aDest );
void AppendUTF8toUTF16( const nsACString& aSource, nsAString& aDest );

void AppendUTF16toUTF8( const PRUnichar* aSource, nsACString& aDest );
void AppendUTF8toUTF16( const char* aSource, nsAString& aDest );

  










char* ToNewCString( const nsAString& aSource );


  








char* ToNewCString( const nsACString& aSource );

  














char* ToNewUTF8String( const nsAString& aSource, PRUint32 *aUTF8Count = nsnull );


  











PRUnichar* ToNewUnicode( const nsAString& aSource );


  










PRUnichar* ToNewUnicode( const nsACString& aSource );

  














PRUnichar* UTF8ToNewUnicode( const nsACString& aSource, PRUint32 *aUTF16Count = nsnull );

  











PRUnichar* CopyUnicodeTo( const nsAString& aSource,
                                 PRUint32 aSrcOffset,
                                 PRUnichar* aDest,
                                 PRUint32 aLength );


  










void CopyUnicodeTo( const nsAString::const_iterator& aSrcStart,
                           const nsAString::const_iterator& aSrcEnd,
                           nsAString& aDest );

  









void AppendUnicodeTo( const nsAString::const_iterator& aSrcStart,
                             const nsAString::const_iterator& aSrcEnd,
                             nsAString& aDest );

  




PRBool IsASCII( const nsAString& aString );

  




PRBool IsASCII( const nsACString& aString );

  






























PRBool IsUTF8( const nsACString& aString, PRBool aRejectNonChar = PR_TRUE );

PRBool ParseString(const nsACString& aAstring, char aDelimiter, 
                          nsTArray<nsCString>& aArray);

  


void ToUpperCase( nsACString& );

void ToLowerCase( nsACString& );

void ToUpperCase( nsCSubstring& );

void ToLowerCase( nsCSubstring& );

  


void ToUpperCase( const nsACString& aSource, nsACString& aDest );

void ToLowerCase( const nsACString& aSource, nsACString& aDest );

  









PRBool FindInReadable( const nsAString& aPattern, nsAString::const_iterator&, nsAString::const_iterator&, const nsStringComparator& = nsDefaultStringComparator() );
PRBool FindInReadable( const nsACString& aPattern, nsACString::const_iterator&, nsACString::const_iterator&, const nsCStringComparator& = nsDefaultCStringComparator() );



inline PRBool FindInReadable( const nsAString& aPattern, const nsAString& aSource, const nsStringComparator& compare = nsDefaultStringComparator() )
{
  nsAString::const_iterator start, end;
  aSource.BeginReading(start);
  aSource.EndReading(end);
  return FindInReadable(aPattern, start, end, compare);
}

inline PRBool FindInReadable( const nsACString& aPattern, const nsACString& aSource, const nsCStringComparator& compare = nsDefaultCStringComparator() )
{
  nsACString::const_iterator start, end;
  aSource.BeginReading(start);
  aSource.EndReading(end);
  return FindInReadable(aPattern, start, end, compare);
}


PRBool CaseInsensitiveFindInReadable( const nsACString& aPattern, nsACString::const_iterator&, nsACString::const_iterator& );

  





PRBool RFindInReadable( const nsAString& aPattern, nsAString::const_iterator&, nsAString::const_iterator&, const nsStringComparator& = nsDefaultStringComparator() );
PRBool RFindInReadable( const nsACString& aPattern, nsACString::const_iterator&, nsACString::const_iterator&, const nsCStringComparator& = nsDefaultCStringComparator() );

   







PRBool FindCharInReadable( PRUnichar aChar, nsAString::const_iterator& aSearchStart, const nsAString::const_iterator& aSearchEnd );
PRBool FindCharInReadable( char aChar, nsACString::const_iterator& aSearchStart, const nsACString::const_iterator& aSearchEnd );

    


PRUint32 CountCharInReadable( const nsAString& aStr,
                                     PRUnichar aChar );
PRUint32 CountCharInReadable( const nsACString& aStr,
                                     char aChar );

PRBool
StringBeginsWith( const nsAString& aSource, const nsAString& aSubstring,
                  const nsStringComparator& aComparator =
                                              nsDefaultStringComparator() );
PRBool
StringBeginsWith( const nsACString& aSource, const nsACString& aSubstring,
                  const nsCStringComparator& aComparator =
                                               nsDefaultCStringComparator() );
PRBool
StringEndsWith( const nsAString& aSource, const nsAString& aSubstring,
                const nsStringComparator& aComparator =
                                            nsDefaultStringComparator() );
PRBool
StringEndsWith( const nsACString& aSource, const nsACString& aSubstring,
                const nsCStringComparator& aComparator =
                                             nsDefaultCStringComparator() );

const nsAFlatString& EmptyString();
const nsAFlatCString& EmptyCString();

   







PRInt32
CompareUTF8toUTF16(const nsASingleFragmentCString& aUTF8String,
                   const nsASingleFragmentString& aUTF16String);

void
AppendUCS4ToUTF16(const PRUint32 aSource, nsAString& aDest);

template<class T>
inline PRBool EnsureStringLength(T& aStr, PRUint32 aLen)
{
    aStr.SetLength(aLen);
    return (aStr.Length() == aLen);
}

#endif 
