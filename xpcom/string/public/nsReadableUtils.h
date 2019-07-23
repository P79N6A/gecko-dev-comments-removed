






































#ifndef nsReadableUtils_h___
#define nsReadableUtils_h___






#ifndef nsAString_h___
#include "nsAString.h"
#endif

inline size_t Distance( const nsReadingIterator<PRUnichar>& start, const nsReadingIterator<PRUnichar>& end )
  {
    return end.get() - start.get();
  }
inline size_t Distance( const nsReadingIterator<char>& start, const nsReadingIterator<char>& end )
  {
    return end.get() - start.get();
  }

NS_COM void LossyCopyUTF16toASCII( const nsAString& aSource, nsACString& aDest );
NS_COM void CopyASCIItoUTF16( const nsACString& aSource, nsAString& aDest );

NS_COM void LossyCopyUTF16toASCII( const PRUnichar* aSource, nsACString& aDest );
NS_COM void CopyASCIItoUTF16( const char* aSource, nsAString& aDest );

NS_COM void CopyUTF16toUTF8( const nsAString& aSource, nsACString& aDest );
NS_COM void CopyUTF8toUTF16( const nsACString& aSource, nsAString& aDest );

NS_COM void CopyUTF16toUTF8( const PRUnichar* aSource, nsACString& aDest );
NS_COM void CopyUTF8toUTF16( const char* aSource, nsAString& aDest );

NS_COM void LossyAppendUTF16toASCII( const nsAString& aSource, nsACString& aDest );
NS_COM void AppendASCIItoUTF16( const nsACString& aSource, nsAString& aDest );

NS_COM void LossyAppendUTF16toASCII( const PRUnichar* aSource, nsACString& aDest );
NS_COM void AppendASCIItoUTF16( const char* aSource, nsAString& aDest );

NS_COM void AppendUTF16toUTF8( const nsAString& aSource, nsACString& aDest );
NS_COM void AppendUTF8toUTF16( const nsACString& aSource, nsAString& aDest );

NS_COM void AppendUTF16toUTF8( const PRUnichar* aSource, nsACString& aDest );
NS_COM void AppendUTF8toUTF16( const char* aSource, nsAString& aDest );

  










NS_COM char* ToNewCString( const nsAString& aSource );


  








NS_COM char* ToNewCString( const nsACString& aSource );

  














NS_COM char* ToNewUTF8String( const nsAString& aSource, PRUint32 *aUTF8Count = nsnull );


  











NS_COM PRUnichar* ToNewUnicode( const nsAString& aSource );


  










NS_COM PRUnichar* ToNewUnicode( const nsACString& aSource );

  














NS_COM PRUnichar* UTF8ToNewUnicode( const nsACString& aSource, PRUint32 *aUTF16Count = nsnull );

  











NS_COM PRUnichar* CopyUnicodeTo( const nsAString& aSource,
                                 PRUint32 aSrcOffset,
                                 PRUnichar* aDest,
                                 PRUint32 aLength );


  










NS_COM void CopyUnicodeTo( const nsAString::const_iterator& aSrcStart,
                           const nsAString::const_iterator& aSrcEnd,
                           nsAString& aDest );

  









NS_COM void AppendUnicodeTo( const nsAString::const_iterator& aSrcStart,
                             const nsAString::const_iterator& aSrcEnd,
                             nsAString& aDest );

  




NS_COM PRBool IsASCII( const nsAString& aString );

  




NS_COM PRBool IsASCII( const nsACString& aString );


  

















NS_COM PRBool IsUTF8( const nsACString& aString );


  


NS_COM void ToUpperCase( nsACString& );

NS_COM void ToLowerCase( nsACString& );

NS_COM void ToUpperCase( nsCSubstring& );

NS_COM void ToLowerCase( nsCSubstring& );

  


NS_COM void ToUpperCase( const nsACString& aSource, nsACString& aDest );

NS_COM void ToLowerCase( const nsACString& aSource, nsACString& aDest );

  









NS_COM PRBool FindInReadable( const nsAString& aPattern, nsAString::const_iterator&, nsAString::const_iterator&, const nsStringComparator& = nsDefaultStringComparator() );
NS_COM PRBool FindInReadable( const nsACString& aPattern, nsACString::const_iterator&, nsACString::const_iterator&, const nsCStringComparator& = nsDefaultCStringComparator() );



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


NS_COM PRBool CaseInsensitiveFindInReadable( const nsACString& aPattern, nsACString::const_iterator&, nsACString::const_iterator& );

  







NS_COM PRBool RFindInReadable( const nsAString& aPattern, nsAString::const_iterator&, nsAString::const_iterator&, const nsStringComparator& = nsDefaultStringComparator() );
NS_COM PRBool RFindInReadable( const nsACString& aPattern, nsACString::const_iterator&, nsACString::const_iterator&, const nsCStringComparator& = nsDefaultCStringComparator() );

   







NS_COM PRBool FindCharInReadable( PRUnichar aChar, nsAString::const_iterator& aSearchStart, const nsAString::const_iterator& aSearchEnd );
NS_COM PRBool FindCharInReadable( char aChar, nsACString::const_iterator& aSearchStart, const nsACString::const_iterator& aSearchEnd );

    


NS_COM PRUint32 CountCharInReadable( const nsAString& aStr,
                                     PRUnichar aChar );
NS_COM PRUint32 CountCharInReadable( const nsACString& aStr,
                                     char aChar );

NS_COM PRBool
StringBeginsWith( const nsAString& aSource, const nsAString& aSubstring,
                  const nsStringComparator& aComparator =
                                              nsDefaultStringComparator() );
NS_COM PRBool
StringBeginsWith( const nsACString& aSource, const nsACString& aSubstring,
                  const nsCStringComparator& aComparator =
                                               nsDefaultCStringComparator() );
NS_COM PRBool
StringEndsWith( const nsAString& aSource, const nsAString& aSubstring,
                const nsStringComparator& aComparator =
                                            nsDefaultStringComparator() );
NS_COM PRBool
StringEndsWith( const nsACString& aSource, const nsACString& aSubstring,
                const nsCStringComparator& aComparator =
                                             nsDefaultCStringComparator() );

NS_COM const nsAFlatString& EmptyString();
NS_COM const nsAFlatCString& EmptyCString();


   







NS_COM PRInt32
CompareUTF8toUTF16(const nsASingleFragmentCString& aUTF8String,
                   const nsASingleFragmentString& aUTF16String);

NS_COM void
AppendUCS4ToUTF16(const PRUint32 aSource, nsAString& aDest);

template<class T>
inline PRBool EnsureStringLength(T& aStr, PRUint32 aLen)
{
    aStr.SetLength(aLen);
    return (aStr.Length() == aLen);
}

#endif 
