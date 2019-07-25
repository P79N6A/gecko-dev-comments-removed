




#include "nsReadableUtils.h"
#include "nsMemory.h"
#include "nsString.h"
#include "nsUTF8Utils.h"

void
LossyCopyUTF16toASCII( const nsAString& aSource, nsACString& aDest )
  {
    aDest.Truncate();
    LossyAppendUTF16toASCII(aSource, aDest);
  }

void
CopyASCIItoUTF16( const nsACString& aSource, nsAString& aDest )
  {
    aDest.Truncate();
    AppendASCIItoUTF16(aSource, aDest);
  }

void
LossyCopyUTF16toASCII( const PRUnichar* aSource, nsACString& aDest )
  {
    aDest.Truncate();
    if (aSource) {
      LossyAppendUTF16toASCII(nsDependentString(aSource), aDest);
    }
  }

void
CopyASCIItoUTF16( const char* aSource, nsAString& aDest )
  {
    aDest.Truncate();
    if (aSource) {
      AppendASCIItoUTF16(nsDependentCString(aSource), aDest);
    }
  }

void
CopyUTF16toUTF8( const nsAString& aSource, nsACString& aDest )
  {
    aDest.Truncate();
    AppendUTF16toUTF8(aSource, aDest);
  }

void
CopyUTF8toUTF16( const nsACString& aSource, nsAString& aDest )
  {
    aDest.Truncate();
    AppendUTF8toUTF16(aSource, aDest);
  }

void
CopyUTF16toUTF8( const PRUnichar* aSource, nsACString& aDest )
  {
    aDest.Truncate();
    AppendUTF16toUTF8(aSource, aDest);
  }

void
CopyUTF8toUTF16( const char* aSource, nsAString& aDest )
  {
    aDest.Truncate();
    AppendUTF8toUTF16(aSource, aDest);
  }





bool
SetLengthForWriting(nsAString& aDest, uint32_t aDesiredLength)
  {
    PRUnichar* dummy;
    uint32_t len = aDest.GetMutableData(&dummy, aDesiredLength);
    return (len >= aDesiredLength);
  }

bool
SetLengthForWritingC(nsACString& aDest, uint32_t aDesiredLength)
  {
    char* dummy;
    uint32_t len = aDest.GetMutableData(&dummy, aDesiredLength);
    return (len >= aDesiredLength);
  }


void
LossyAppendUTF16toASCII( const nsAString& aSource, nsACString& aDest )
  {
    uint32_t old_dest_length = aDest.Length();
    if (!SetLengthForWritingC(aDest, old_dest_length + aSource.Length()))
        return;

    nsAString::const_iterator fromBegin, fromEnd;

    nsACString::iterator dest;
    aDest.BeginWriting(dest);

    dest.advance(old_dest_length);

    
    LossyConvertEncoding16to8 converter(dest.get());

    copy_string(aSource.BeginReading(fromBegin), aSource.EndReading(fromEnd), converter);
  }

void
AppendASCIItoUTF16( const nsACString& aSource, nsAString& aDest )
  {
    uint32_t old_dest_length = aDest.Length();
    if (!SetLengthForWriting(aDest, old_dest_length + aSource.Length()))
        return;

    nsACString::const_iterator fromBegin, fromEnd;

    nsAString::iterator dest;
    aDest.BeginWriting(dest);

    dest.advance(old_dest_length);

      
    LossyConvertEncoding8to16 converter(dest.get());

    copy_string(aSource.BeginReading(fromBegin), aSource.EndReading(fromEnd), converter);
  }

void
LossyAppendUTF16toASCII( const PRUnichar* aSource, nsACString& aDest )
  {
    if (aSource) {
      LossyAppendUTF16toASCII(nsDependentString(aSource), aDest);
    }
  }

void
AppendASCIItoUTF16( const char* aSource, nsAString& aDest )
  {
    if (aSource) {
      AppendASCIItoUTF16(nsDependentCString(aSource), aDest);
    }
  }

void
AppendUTF16toUTF8( const nsAString& aSource, nsACString& aDest )
  {
    nsAString::const_iterator source_start, source_end;
    CalculateUTF8Size calculator;
    copy_string(aSource.BeginReading(source_start),
                aSource.EndReading(source_end), calculator);

    uint32_t count = calculator.Size();

    if (count)
      {
        uint32_t old_dest_length = aDest.Length();

        
        if(!SetLengthForWritingC(aDest, old_dest_length + count))
            return;

        

        ConvertUTF16toUTF8 converter(aDest.BeginWriting() + old_dest_length);
        copy_string(aSource.BeginReading(source_start),
                    aSource.EndReading(source_end), converter);

        NS_ASSERTION(converter.Size() == count,
                     "Unexpected disparity between CalculateUTF8Size and "
                     "ConvertUTF16toUTF8");
      }
  }

void
AppendUTF8toUTF16( const nsACString& aSource, nsAString& aDest )
  {
    nsACString::const_iterator source_start, source_end;
    CalculateUTF8Length calculator;
    copy_string(aSource.BeginReading(source_start),
                aSource.EndReading(source_end), calculator);

    uint32_t count = calculator.Length();

    
    if (count)
      {
        uint32_t old_dest_length = aDest.Length();

        
        if(!SetLengthForWriting(aDest, old_dest_length + count))
          return;

        

        ConvertUTF8toUTF16 converter(aDest.BeginWriting() + old_dest_length);
        copy_string(aSource.BeginReading(source_start),
                    aSource.EndReading(source_end), converter);

        NS_ASSERTION(converter.ErrorEncountered() ||
                     converter.Length() == count,
                     "CalculateUTF8Length produced the wrong length");

        if (converter.ErrorEncountered())
          {
            NS_ERROR("Input wasn't UTF8 or incorrect length was calculated");
            aDest.SetLength(old_dest_length);
          }
      }
  }

void
AppendUTF16toUTF8( const PRUnichar* aSource, nsACString& aDest )
  {
    if (aSource) {
      AppendUTF16toUTF8(nsDependentString(aSource), aDest);
    }
  }

void
AppendUTF8toUTF16( const char* aSource, nsAString& aDest )
  {
    if (aSource) {
      AppendUTF8toUTF16(nsDependentCString(aSource), aDest);
    }
  }


  






template <class FromStringT, class ToCharT>
inline
ToCharT*
AllocateStringCopy( const FromStringT& aSource, ToCharT* )
  {
    return static_cast<ToCharT*>(nsMemory::Alloc((aSource.Length()+1) * sizeof(ToCharT)));
  }


char*
ToNewCString( const nsAString& aSource )
  {
    char* result = AllocateStringCopy(aSource, (char*)0);
    if (!result)
      return nullptr;

    nsAString::const_iterator fromBegin, fromEnd;
    LossyConvertEncoding16to8 converter(result);
    copy_string(aSource.BeginReading(fromBegin), aSource.EndReading(fromEnd), converter).write_terminator();
    return result;
  }

char*
ToNewUTF8String( const nsAString& aSource, uint32_t *aUTF8Count )
  {
    nsAString::const_iterator start, end;
    CalculateUTF8Size calculator;
    copy_string(aSource.BeginReading(start), aSource.EndReading(end),
                calculator);

    if (aUTF8Count)
      *aUTF8Count = calculator.Size();

    char *result = static_cast<char*>
                              (nsMemory::Alloc(calculator.Size() + 1));
    if (!result)
      return nullptr;

    ConvertUTF16toUTF8 converter(result);
    copy_string(aSource.BeginReading(start), aSource.EndReading(end),
                converter).write_terminator();
    NS_ASSERTION(calculator.Size() == converter.Size(), "length mismatch");

    return result;
  }

char*
ToNewCString( const nsACString& aSource )
  {
    

    char* result = AllocateStringCopy(aSource, (char*)0);
    if (!result)
      return nullptr;

    nsACString::const_iterator fromBegin, fromEnd;
    char* toBegin = result;
    *copy_string(aSource.BeginReading(fromBegin), aSource.EndReading(fromEnd), toBegin) = char(0);
    return result;
  }

PRUnichar*
ToNewUnicode( const nsAString& aSource )
  {
    

    PRUnichar* result = AllocateStringCopy(aSource, (PRUnichar*)0);
    if (!result)
      return nullptr;

    nsAString::const_iterator fromBegin, fromEnd;
    PRUnichar* toBegin = result;
    *copy_string(aSource.BeginReading(fromBegin), aSource.EndReading(fromEnd), toBegin) = PRUnichar(0);
    return result;
  }

PRUnichar*
ToNewUnicode( const nsACString& aSource )
  {
    PRUnichar* result = AllocateStringCopy(aSource, (PRUnichar*)0);
    if (!result)
      return nullptr;

    nsACString::const_iterator fromBegin, fromEnd;
    LossyConvertEncoding8to16 converter(result);
    copy_string(aSource.BeginReading(fromBegin), aSource.EndReading(fromEnd), converter).write_terminator();
    return result;
  }

PRUnichar*
UTF8ToNewUnicode( const nsACString& aSource, uint32_t *aUTF16Count )
  {
    nsACString::const_iterator start, end;
    CalculateUTF8Length calculator;
    copy_string(aSource.BeginReading(start), aSource.EndReading(end),
                calculator);

    if (aUTF16Count)
      *aUTF16Count = calculator.Length();

    PRUnichar *result = static_cast<PRUnichar*>
                                   (nsMemory::Alloc(sizeof(PRUnichar) * (calculator.Length() + 1)));
    if (!result)
      return nullptr;

    ConvertUTF8toUTF16 converter(result);
    copy_string(aSource.BeginReading(start), aSource.EndReading(end),
                converter).write_terminator();
    NS_ASSERTION(calculator.Length() == converter.Length(), "length mismatch");

    return result;
  }

PRUnichar*
CopyUnicodeTo( const nsAString& aSource, uint32_t aSrcOffset, PRUnichar* aDest, uint32_t aLength )
  {
    nsAString::const_iterator fromBegin, fromEnd;
    PRUnichar* toBegin = aDest;    
    copy_string(aSource.BeginReading(fromBegin).advance( int32_t(aSrcOffset) ), aSource.BeginReading(fromEnd).advance( int32_t(aSrcOffset+aLength) ), toBegin);
    return aDest;
  }

void
CopyUnicodeTo( const nsAString::const_iterator& aSrcStart,
               const nsAString::const_iterator& aSrcEnd,
               nsAString& aDest )
  {
    nsAString::iterator writer;
    if (!SetLengthForWriting(aDest, Distance(aSrcStart, aSrcEnd)))
        return;

    aDest.BeginWriting(writer);
    nsAString::const_iterator fromBegin(aSrcStart);
    
    copy_string(fromBegin, aSrcEnd, writer);
  }

void
AppendUnicodeTo( const nsAString::const_iterator& aSrcStart,
                 const nsAString::const_iterator& aSrcEnd,
                 nsAString& aDest )
  {
    nsAString::iterator writer;
    uint32_t oldLength = aDest.Length();
    if(!SetLengthForWriting(aDest, oldLength + Distance(aSrcStart, aSrcEnd)))
        return;

    aDest.BeginWriting(writer).advance(oldLength);
    nsAString::const_iterator fromBegin(aSrcStart);
    
    copy_string(fromBegin, aSrcEnd, writer);
  }

bool
IsASCII( const nsAString& aString )
  {
    static const PRUnichar NOT_ASCII = PRUnichar(~0x007F);


    

    nsAString::const_iterator iter, done_reading;
    aString.BeginReading(iter);
    aString.EndReading(done_reading);

    const PRUnichar* c = iter.get();
    const PRUnichar* end = done_reading.get();
    
    while ( c < end )
      {
        if ( *c++ & NOT_ASCII )
          return false;
      }

    return true;
  }

bool
IsASCII( const nsACString& aString )
  {
    static const char NOT_ASCII = char(~0x7F);


    

    nsACString::const_iterator iter, done_reading;
    aString.BeginReading(iter);
    aString.EndReading(done_reading);

    const char* c = iter.get();
    const char* end = done_reading.get();
    
    while ( c < end )
      {
        if ( *c++ & NOT_ASCII )
          return false;
      }

    return true;
  }

bool
IsUTF8( const nsACString& aString, bool aRejectNonChar )
  {
    nsReadingIterator<char> done_reading;
    aString.EndReading(done_reading);

    int32_t state = 0;
    bool overlong = false;
    bool surrogate = false;
    bool nonchar = false;
    uint16_t olupper = 0; 
    uint16_t slower = 0;  

    nsReadingIterator<char> iter;
    aString.BeginReading(iter);

    const char* ptr = iter.get();
    const char* end = done_reading.get();
    while ( ptr < end )
      {
        uint8_t c;
        
        if (0 == state)
          {
            c = *ptr++;

            if ( UTF8traits::isASCII(c) ) 
              continue;

            if ( c <= 0xC1 ) 
              return false;
            else if ( UTF8traits::is2byte(c) ) 
                state = 1;
            else if ( UTF8traits::is3byte(c) ) 
              {
                state = 2;
                if ( c == 0xE0 ) 
                  {
                    overlong = true;
                    olupper = 0x9F;
                  }
                else if ( c == 0xED ) 
                  {
                    surrogate = true;
                    slower = 0xA0;
                  }
                else if ( c == 0xEF ) 
                  nonchar = true;
              }
            else if ( c <= 0xF4 ) 
              {
                state = 3;
                nonchar = true;
                if ( c == 0xF0 ) 
                  {
                    overlong = true;
                    olupper = 0x8F;
                  }
                else if ( c == 0xF4 ) 
                  {
                    
                    surrogate = true;
                    slower = 0x90;
                  }
              }
            else
              return false; 
          }
          
        if (nonchar && !aRejectNonChar)
          nonchar = false;

        while ( ptr < end && state )
          {
            c = *ptr++;
            --state;

            
            if ( nonchar &&  
                 ( ( !state && c < 0xBE ) ||
                   ( state == 1 && c != 0xBF )  ||
                   ( state == 2 && 0x0F != (0x0F & c) )))
              nonchar = false;

            if ( !UTF8traits::isInSeq(c) || ( overlong && c <= olupper ) || 
                 ( surrogate && slower <= c ) || ( nonchar && !state ))
              return false; 

            overlong = surrogate = false;
          }
        }
    return !state; 
  }

  


class ConvertToUpperCase
  {
    public:
      typedef char value_type;

      uint32_t
      write( const char* aSource, uint32_t aSourceLength )
        {
          char* cp = const_cast<char*>(aSource);
          const char* end = aSource + aSourceLength;
          while (cp != end) {
            char ch = *cp;
            if ((ch >= 'a') && (ch <= 'z'))
              *cp = ch - ('a' - 'A');
            ++cp;
          }
          return aSourceLength;
        }
  };

void
ToUpperCase( nsCSubstring& aCString )
  {
    ConvertToUpperCase converter;
    char* start;
    converter.write(aCString.BeginWriting(start), aCString.Length());
  }

  


class CopyToUpperCase
  {
    public:
      typedef char value_type;

      CopyToUpperCase( nsACString::iterator& aDestIter )
        : mIter(aDestIter)
        {
        }

      uint32_t
      write( const char* aSource, uint32_t aSourceLength )
        {
          uint32_t len = NS_MIN(uint32_t(mIter.size_forward()), aSourceLength);
          char* cp = mIter.get();
          const char* end = aSource + len;
          while (aSource != end) {
            char ch = *aSource;
            if ((ch >= 'a') && (ch <= 'z'))
              *cp = ch - ('a' - 'A');
            else
              *cp = ch;
            ++aSource;
            ++cp;
          }
          mIter.advance(len);
          return len;
        }

    protected:
      nsACString::iterator& mIter;
  };

void
ToUpperCase( const nsACString& aSource, nsACString& aDest )
  {
    nsACString::const_iterator fromBegin, fromEnd;
    nsACString::iterator toBegin;
    if (!SetLengthForWritingC(aDest, aSource.Length()))
        return;

    CopyToUpperCase converter(aDest.BeginWriting(toBegin));
    copy_string(aSource.BeginReading(fromBegin), aSource.EndReading(fromEnd), converter);
  }

  


class ConvertToLowerCase
  {
    public:
      typedef char value_type;

      uint32_t
      write( const char* aSource, uint32_t aSourceLength )
        {
          char* cp = const_cast<char*>(aSource);
          const char* end = aSource + aSourceLength;
          while (cp != end) {
            char ch = *cp;
            if ((ch >= 'A') && (ch <= 'Z'))
              *cp = ch + ('a' - 'A');
            ++cp;
          }
          return aSourceLength;
        }
  };

void
ToLowerCase( nsCSubstring& aCString )
  {
    ConvertToLowerCase converter;
    char* start;
    converter.write(aCString.BeginWriting(start), aCString.Length());
  }

  


class CopyToLowerCase
  {
    public:
      typedef char value_type;

      CopyToLowerCase( nsACString::iterator& aDestIter )
        : mIter(aDestIter)
        {
        }

      uint32_t
      write( const char* aSource, uint32_t aSourceLength )
        {
          uint32_t len = NS_MIN(uint32_t(mIter.size_forward()), aSourceLength);
          char* cp = mIter.get();
          const char* end = aSource + len;
          while (aSource != end) {
            char ch = *aSource;
            if ((ch >= 'A') && (ch <= 'Z'))
              *cp = ch + ('a' - 'A');
            else
              *cp = ch;
            ++aSource;
            ++cp;
          }
          mIter.advance(len);
          return len;
        }

    protected:
      nsACString::iterator& mIter;
  };

void
ToLowerCase( const nsACString& aSource, nsACString& aDest )
  {
    nsACString::const_iterator fromBegin, fromEnd;
    nsACString::iterator toBegin;
    if (!SetLengthForWritingC(aDest, aSource.Length()))
        return;

    CopyToLowerCase converter(aDest.BeginWriting(toBegin));
    copy_string(aSource.BeginReading(fromBegin), aSource.EndReading(fromEnd), converter);
  }

bool
ParseString(const nsACString& aSource, char aDelimiter, 
            nsTArray<nsCString>& aArray)
  {
    nsACString::const_iterator start, end;
    aSource.BeginReading(start);
    aSource.EndReading(end);

    uint32_t oldLength = aArray.Length();

    for (;;)
      {
        nsACString::const_iterator delimiter = start;
        FindCharInReadable(aDelimiter, delimiter, end);

        if (delimiter != start)
          {
            if (!aArray.AppendElement(Substring(start, delimiter)))
              {
                aArray.RemoveElementsAt(oldLength, aArray.Length() - oldLength);
                return false;
              }
          }

        if (delimiter == end)
          break;
        start = ++delimiter;
        if (start == end)
          break;
      }

    return true;
  }

template <class StringT, class IteratorT, class Comparator>
bool
FindInReadable_Impl( const StringT& aPattern, IteratorT& aSearchStart, IteratorT& aSearchEnd, const Comparator& compare )
  {
    bool found_it = false;

      
    if ( aSearchStart != aSearchEnd )
      {
        IteratorT aPatternStart, aPatternEnd;
        aPattern.BeginReading(aPatternStart);
        aPattern.EndReading(aPatternEnd);

          
        while ( !found_it )
          {
              
            while ( aSearchStart != aSearchEnd &&
                    compare(aPatternStart.get(), aSearchStart.get(), 1, 1) )
              ++aSearchStart;

              
            if ( aSearchStart == aSearchEnd )
              break;

              
            IteratorT testPattern(aPatternStart);
            IteratorT testSearch(aSearchStart);

              
            for(;;)
              {
                  
                  
                ++testPattern;
                ++testSearch;

                  
                if ( testPattern == aPatternEnd )
                  {
                    found_it = true;
                    aSearchEnd = testSearch; 
                    break;
                  }

                  
                  
                if ( testSearch == aSearchEnd )
                  {
                    aSearchStart = aSearchEnd;
                    break;
                  }

                  
                  
                if ( compare(testPattern.get(), testSearch.get(), 1, 1) )
                  {
                    ++aSearchStart;
                    break;
                  }
              }
          }
      }

    return found_it;
  }

  


template <class StringT, class IteratorT, class Comparator>
bool
RFindInReadable_Impl( const StringT& aPattern, IteratorT& aSearchStart, IteratorT& aSearchEnd, const Comparator& compare )
  {
    IteratorT patternStart, patternEnd, searchEnd = aSearchEnd;
    aPattern.BeginReading(patternStart);
    aPattern.EndReading(patternEnd);

      
    --patternEnd;
      
    while ( aSearchStart != searchEnd )
      {
          
        --searchEnd;
    
          
        if ( compare(patternEnd.get(), searchEnd.get(), 1, 1) == 0 )
          {  
              
            IteratorT testPattern(patternEnd);
            IteratorT testSearch(searchEnd);

              
            do
              {
                  
                if ( testPattern == patternStart )
                  {
                    aSearchStart = testSearch;  
                    aSearchEnd = ++searchEnd;   
                    return true;
                  }
    
                  
                  
                if ( testSearch == aSearchStart )
                  {
                    aSearchStart = aSearchEnd;
                    return false;
                  }
    
                  
                --testPattern;
                --testSearch;
              }
            while ( compare(testPattern.get(), testSearch.get(), 1, 1) == 0 );
          }
      }

    aSearchStart = aSearchEnd;
    return false;
  }

bool
FindInReadable( const nsAString& aPattern, nsAString::const_iterator& aSearchStart, nsAString::const_iterator& aSearchEnd, const nsStringComparator& aComparator )
  {
    return FindInReadable_Impl(aPattern, aSearchStart, aSearchEnd, aComparator);
  }

bool
FindInReadable( const nsACString& aPattern, nsACString::const_iterator& aSearchStart, nsACString::const_iterator& aSearchEnd, const nsCStringComparator& aComparator)
  {
    return FindInReadable_Impl(aPattern, aSearchStart, aSearchEnd, aComparator);
  }

bool
CaseInsensitiveFindInReadable( const nsACString& aPattern, nsACString::const_iterator& aSearchStart, nsACString::const_iterator& aSearchEnd )
  {
    return FindInReadable_Impl(aPattern, aSearchStart, aSearchEnd, nsCaseInsensitiveCStringComparator());
  }

bool
RFindInReadable( const nsAString& aPattern, nsAString::const_iterator& aSearchStart, nsAString::const_iterator& aSearchEnd, const nsStringComparator& aComparator)
  {
    return RFindInReadable_Impl(aPattern, aSearchStart, aSearchEnd, aComparator);
  }

bool
RFindInReadable( const nsACString& aPattern, nsACString::const_iterator& aSearchStart, nsACString::const_iterator& aSearchEnd, const nsCStringComparator& aComparator)
  {
    return RFindInReadable_Impl(aPattern, aSearchStart, aSearchEnd, aComparator);
  }

bool
FindCharInReadable( PRUnichar aChar, nsAString::const_iterator& aSearchStart, const nsAString::const_iterator& aSearchEnd )
  {
    int32_t fragmentLength = aSearchEnd.get() - aSearchStart.get();

    const PRUnichar* charFoundAt = nsCharTraits<PRUnichar>::find(aSearchStart.get(), fragmentLength, aChar);
    if ( charFoundAt ) {
      aSearchStart.advance( charFoundAt - aSearchStart.get() );
      return true;
    }

    aSearchStart.advance(fragmentLength);
    return false;
  }

bool
FindCharInReadable( char aChar, nsACString::const_iterator& aSearchStart, const nsACString::const_iterator& aSearchEnd )
  {
    int32_t fragmentLength = aSearchEnd.get() - aSearchStart.get();

    const char* charFoundAt = nsCharTraits<char>::find(aSearchStart.get(), fragmentLength, aChar);
    if ( charFoundAt ) {
      aSearchStart.advance( charFoundAt - aSearchStart.get() );
      return true;
    }

    aSearchStart.advance(fragmentLength);
    return false;
  }

uint32_t
CountCharInReadable( const nsAString& aStr,
                     PRUnichar aChar )
{
  uint32_t count = 0;
  nsAString::const_iterator begin, end;
  
  aStr.BeginReading(begin);
  aStr.EndReading(end);
  
  while (begin != end) {
    if (*begin == aChar) {
      ++count;
    }
    ++begin;
  }

  return count;
}

uint32_t
CountCharInReadable( const nsACString& aStr,
                     char aChar )
{
  uint32_t count = 0;
  nsACString::const_iterator begin, end;
  
  aStr.BeginReading(begin);
  aStr.EndReading(end);
  
  while (begin != end) {
    if (*begin == aChar) {
      ++count;
    }
    ++begin;
  }

  return count;
}

bool
StringBeginsWith( const nsAString& aSource, const nsAString& aSubstring,
                  const nsStringComparator& aComparator )
  {
    nsAString::size_type src_len = aSource.Length(),
                         sub_len = aSubstring.Length();
    if (sub_len > src_len)
      return false;
    return Substring(aSource, 0, sub_len).Equals(aSubstring, aComparator);
  }

bool
StringBeginsWith( const nsACString& aSource, const nsACString& aSubstring,
                  const nsCStringComparator& aComparator )
  {
    nsACString::size_type src_len = aSource.Length(),
                          sub_len = aSubstring.Length();
    if (sub_len > src_len)
      return false;
    return Substring(aSource, 0, sub_len).Equals(aSubstring, aComparator);
  }

bool
StringEndsWith( const nsAString& aSource, const nsAString& aSubstring,
                const nsStringComparator& aComparator )
  {
    nsAString::size_type src_len = aSource.Length(),
                         sub_len = aSubstring.Length();
    if (sub_len > src_len)
      return false;
    return Substring(aSource, src_len - sub_len, sub_len).Equals(aSubstring,
                                                                 aComparator);
  }

bool
StringEndsWith( const nsACString& aSource, const nsACString& aSubstring,
                const nsCStringComparator& aComparator )
  {
    nsACString::size_type src_len = aSource.Length(),
                          sub_len = aSubstring.Length();
    if (sub_len > src_len)
      return false;
    return Substring(aSource, src_len - sub_len, sub_len).Equals(aSubstring,
                                                                 aComparator);
  }



static const PRUnichar empty_buffer[1] = { '\0' };

const nsAFlatString&
EmptyString()
  {
    static const nsDependentString sEmpty(empty_buffer);

    return sEmpty;
  }

const nsAFlatCString&
EmptyCString()
  {
    static const nsDependentCString sEmpty((const char *)empty_buffer);

    return sEmpty;
  }

const nsAFlatString&
NullString()
  {
    static const nsXPIDLString sNull;

    return sNull;
  }

const nsAFlatCString&
NullCString()
  {
    static const nsXPIDLCString sNull;

    return sNull;
  }

int32_t
CompareUTF8toUTF16(const nsASingleFragmentCString& aUTF8String,
                   const nsASingleFragmentString& aUTF16String)
  {
    static const uint32_t NOT_ASCII = uint32_t(~0x7F);

    const char *u8, *u8end;
    aUTF8String.BeginReading(u8);
    aUTF8String.EndReading(u8end);

    const PRUnichar *u16, *u16end;
    aUTF16String.BeginReading(u16);
    aUTF16String.EndReading(u16end);

    while (u8 != u8end && u16 != u16end)
      {
        
        
        uint32_t c8_32 = (uint8_t)*u8;

        if (c8_32 & NOT_ASCII)
          {
            bool err;
            c8_32 = UTF8CharEnumerator::NextChar(&u8, u8end, &err);
            if (err)
              return PR_INT32_MIN;

            uint32_t c16_32 = UTF16CharEnumerator::NextChar(&u16, u16end);
            
            
            
            
            
            
            
            
            
            
            
            
            
            if (c8_32 != c16_32)
              return c8_32 < c16_32 ? -1 : 1;
          }
        else
          {
            if (c8_32 != *u16)
              return c8_32 > *u16 ? 1 : -1;

            ++u8;
            ++u16;
          }
      }

    if (u8 != u8end)
      {
        
        
        

        return 1;
      }

    if (u16 != u16end)
      {
        
        
        

        return -1;
      }

    

    return 0;
  }

void
AppendUCS4ToUTF16(const uint32_t aSource, nsAString& aDest)
  {
    NS_ASSERTION(IS_VALID_CHAR(aSource), "Invalid UCS4 char");
    if (IS_IN_BMP(aSource))
      {
        aDest.Append(PRUnichar(aSource));
      }
    else
      {
        aDest.Append(H_SURROGATE(aSource));
        aDest.Append(L_SURROGATE(aSource));
      }
  }
