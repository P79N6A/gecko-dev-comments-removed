




































#include "prdtoa.h"

#ifdef XPCOM_STRING_CONSTRUCTOR_OUT_OF_LINE
nsTSubstring_CharT::nsTSubstring_CharT( char_type *data, size_type length,
                                        PRUint32 flags)
  : mData(data),
    mLength(length),
    mFlags(flags)
  {
    if (flags & F_OWNED) {
      STRING_STAT_INCREMENT(Adopt);
#ifdef NS_BUILD_REFCNT_LOGGING
      NS_LogCtor(mData, "StringAdopt", 1);
#endif
    }
  }
#endif 

  


inline const nsTFixedString_CharT*
AsFixedString( const nsTSubstring_CharT* s )
  {
    return static_cast<const nsTFixedString_CharT*>(s);
  }


  






PRBool
nsTSubstring_CharT::MutatePrep( size_type capacity, char_type** oldData, PRUint32* oldFlags )
  {
    
    *oldData = nsnull;
    *oldFlags = 0;

    size_type curCapacity = Capacity();

    
    
    
    PR_STATIC_ASSERT((sizeof(nsStringBuffer) & 0x1) == 0);
    const size_type kMaxCapacity =
      (size_type(-1)/2 - sizeof(nsStringBuffer)) / sizeof(char_type) - 2;
    if (capacity > kMaxCapacity) {
      
      
      NS_ASSERTION(capacity != size_type(-1), "Bogus capacity");
      return PR_FALSE;
    }

    
    
    

    if (curCapacity != 0)
      {
        if (capacity <= curCapacity) {
          mFlags &= ~F_VOIDED;  
          return PR_TRUE;
        }

        
        size_type temp = curCapacity;
        while (temp < capacity)
          temp <<= 1;
        NS_ASSERTION(NS_MIN(temp, kMaxCapacity) >= capacity,
                     "should have hit the early return at the top");
        capacity = NS_MIN(temp, kMaxCapacity);
      }

    
    
    
    
    
    
    
    
    
    
    

    size_type storageSize = (capacity + 1) * sizeof(char_type);

    
    if (mFlags & F_SHARED)
      {
        nsStringBuffer* hdr = nsStringBuffer::FromData(mData);
        if (!hdr->IsReadonly())
          {
            nsStringBuffer *newHdr = nsStringBuffer::Realloc(hdr, storageSize);
            if (!newHdr)
              return PR_FALSE; 

            hdr = newHdr;
            mData = (char_type*) hdr->Data();
            mFlags &= ~F_VOIDED;  
            return PR_TRUE;
          }
      }

    char_type* newData;
    PRUint32 newDataFlags;

      
      
    if ((mFlags & F_CLASS_FIXED) && (capacity < AsFixedString(this)->mFixedCapacity))
      {
        newData = AsFixedString(this)->mFixedBuf;
        newDataFlags = F_TERMINATED | F_FIXED;
      }
    else
      {
        
        
        

        nsStringBuffer* newHdr = nsStringBuffer::Alloc(storageSize);
        if (!newHdr)
          return PR_FALSE; 

        newData = (char_type*) newHdr->Data();
        newDataFlags = F_TERMINATED | F_SHARED;
      }

    
    *oldData = mData;
    *oldFlags = mFlags;

    mData = newData;
    SetDataFlags(newDataFlags);

    

    
    

    return PR_TRUE;
  }

void
nsTSubstring_CharT::Finalize()
  {
    ::ReleaseData(mData, mFlags);
    
  }

PRBool
nsTSubstring_CharT::ReplacePrepInternal(index_type cutStart, size_type cutLen,
                                        size_type fragLen, size_type newLen)
  {
    char_type* oldData;
    PRUint32 oldFlags;
    if (!MutatePrep(newLen, &oldData, &oldFlags))
      return PR_FALSE; 

    if (oldData)
      {
        
        

        if (cutStart > 0)
          {
            
            char_traits::copy(mData, oldData, cutStart);
          }

        if (cutStart + cutLen < mLength)
          {
            
            size_type from = cutStart + cutLen;
            size_type fromLen = mLength - from;
            PRUint32 to = cutStart + fragLen;
            char_traits::copy(mData + to, oldData + from, fromLen);
          }

        ::ReleaseData(oldData, oldFlags);
      }
    else
      {
        

        
        
        if (fragLen != cutLen && cutStart + cutLen < mLength)
          {
            PRUint32 from = cutStart + cutLen;
            PRUint32 fromLen = mLength - from;
            PRUint32 to = cutStart + fragLen;
            char_traits::move(mData + to, mData + from, fromLen);
          }
      }

    
    
    mData[newLen] = char_type(0);
    mLength = newLen;

    return PR_TRUE;
  }

nsTSubstring_CharT::size_type
nsTSubstring_CharT::Capacity() const
  {
    

    size_type capacity;
    if (mFlags & F_SHARED)
      {
        
        nsStringBuffer* hdr = nsStringBuffer::FromData(mData);
        if (hdr->IsReadonly())
          capacity = 0;
        else {
          capacity = (hdr->StorageSize() / sizeof(char_type)) - 1;
        }
      }
    else if (mFlags & F_FIXED)
      {
        capacity = AsFixedString(this)->mFixedCapacity;
      }
    else if (mFlags & F_OWNED)
      {
        
        
        
        
        capacity = mLength;
      }
    else
      {
        capacity = 0;
      }

    return capacity;
  }

PRBool
nsTSubstring_CharT::EnsureMutable( size_type newLen )
  {
    if (newLen == size_type(-1) || newLen == mLength)
      {
        if (mFlags & (F_FIXED | F_OWNED))
          return PR_TRUE;
        if ((mFlags & F_SHARED) && !nsStringBuffer::FromData(mData)->IsReadonly())
          return PR_TRUE;

        
        char_type* prevData = mData;
        Assign(mData, mLength);
        return mData != prevData;
      }
    else
      {
        SetLength(newLen);
        return mLength == newLen;
      }
  }



  
void
nsTSubstring_CharT::Assign( char_type c )
  {
    if (ReplacePrep(0, mLength, 1))
      *mData = c;
  }


void
nsTSubstring_CharT::Assign( const char_type* data, size_type length )
  {
      
    if (!data)
      {
        Truncate();
        return;
      }

    if (length == size_type(-1))
      length = char_traits::length(data);

    if (IsDependentOn(data, data + length))
      {
        
        Assign(string_type(data, length));
        return;
      }

    if (ReplacePrep(0, mLength, length))
      char_traits::copy(mData, data, length);
  }

void
nsTSubstring_CharT::AssignASCII( const char* data, size_type length )
  {
    
    
#ifdef CharT_is_char
    if (IsDependentOn(data, data + length))
      {
        
        Assign(string_type(data, length));
        return;
      }
#endif

    if (ReplacePrep(0, mLength, length))
      char_traits::copyASCII(mData, data, length);
  }

void
nsTSubstring_CharT::AssignASCII( const char* data )
  {
    AssignASCII(data, strlen(data));
  }

void
nsTSubstring_CharT::Assign( const self_type& str )
  {
    
    

    if (&str == this)
      return;

    if (!str.mLength)
      {
        Truncate();
        mFlags |= str.mFlags & F_VOIDED;
      }
    else if (str.mFlags & F_SHARED)
      {
        

        
        NS_ASSERTION(str.mFlags & F_TERMINATED, "shared, but not terminated");

        ::ReleaseData(mData, mFlags);

        mData = str.mData;
        mLength = str.mLength;
        SetDataFlags(F_TERMINATED | F_SHARED);

        
        nsStringBuffer::FromData(mData)->AddRef();
      }
    else
      {
        
        Assign(str.Data(), str.Length());
      }
  }

void
nsTSubstring_CharT::Assign( const substring_tuple_type& tuple )
  {
    if (tuple.IsDependentOn(mData, mData + mLength))
      {
        
        Assign(string_type(tuple));
        return;
      }

    size_type length = tuple.Length();

    
    char_type* oldData;
    PRUint32 oldFlags;
    if (MutatePrep(length, &oldData, &oldFlags)) {
      if (oldData)
        ::ReleaseData(oldData, oldFlags);

      tuple.WriteTo(mData, length);
      mData[length] = 0;
      mLength = length;
    }
  }

void
nsTSubstring_CharT::Adopt( char_type* data, size_type length )
  {
    if (data)
      {
        ::ReleaseData(mData, mFlags);

        if (length == size_type(-1))
          length = char_traits::length(data);

        mData = data;
        mLength = length;
        SetDataFlags(F_TERMINATED | F_OWNED);

        STRING_STAT_INCREMENT(Adopt);
#ifdef NS_BUILD_REFCNT_LOGGING
        
        
        NS_LogCtor(mData, "StringAdopt", 1);
#endif 
      }
    else
      {
        SetIsVoid(PR_TRUE);
      }
  }


  
void
nsTSubstring_CharT::Replace( index_type cutStart, size_type cutLength, char_type c )
  {
    cutStart = NS_MIN(cutStart, Length());

    if (ReplacePrep(cutStart, cutLength, 1))
      mData[cutStart] = c;
  }


void
nsTSubstring_CharT::Replace( index_type cutStart, size_type cutLength, const char_type* data, size_type length )
  {
      
    if (!data)
      {
        length = 0;
      }
    else
      {
        if (length == size_type(-1))
          length = char_traits::length(data);

        if (IsDependentOn(data, data + length))
          {
            nsTAutoString_CharT temp(data, length);
            Replace(cutStart, cutLength, temp);
            return;
          }
      }

    cutStart = NS_MIN(cutStart, Length());

    if (ReplacePrep(cutStart, cutLength, length) && length > 0)
      char_traits::copy(mData + cutStart, data, length);
  }

void
nsTSubstring_CharT::ReplaceASCII( index_type cutStart, size_type cutLength, const char* data, size_type length )
  {
    if (length == size_type(-1))
      length = strlen(data);
    
    
    
#ifdef CharT_is_char
    if (IsDependentOn(data, data + length))
      {
        nsTAutoString_CharT temp(data, length);
        Replace(cutStart, cutLength, temp);
        return;
      }
#endif

    cutStart = NS_MIN(cutStart, Length());

    if (ReplacePrep(cutStart, cutLength, length) && length > 0)
      char_traits::copyASCII(mData + cutStart, data, length);
  }

void
nsTSubstring_CharT::Replace( index_type cutStart, size_type cutLength, const substring_tuple_type& tuple )
  {
    if (tuple.IsDependentOn(mData, mData + mLength))
      {
        nsTAutoString_CharT temp(tuple);
        Replace(cutStart, cutLength, temp);
        return;
      }

    size_type length = tuple.Length();

    cutStart = NS_MIN(cutStart, Length());

    if (ReplacePrep(cutStart, cutLength, length) && length > 0)
      tuple.WriteTo(mData + cutStart, length);
  }

PRBool
nsTSubstring_CharT::SetCapacity( size_type capacity )
  {
    

    
    if (capacity == 0)
      {
        ::ReleaseData(mData, mFlags);
        mData = char_traits::sEmptyBuffer;
        mLength = 0;
        SetDataFlags(F_TERMINATED);
      }
    else
      {
        char_type* oldData;
        PRUint32 oldFlags;
        if (!MutatePrep(capacity, &oldData, &oldFlags))
          return PR_FALSE; 

        
        size_type newLen = NS_MIN(mLength, capacity);

        if (oldData)
          {
            
            if (mLength > 0)
              char_traits::copy(mData, oldData, newLen);

            ::ReleaseData(oldData, oldFlags);
          }

        
        if (newLen < mLength)
          mLength = newLen;

        
        
        mData[capacity] = char_type(0);
      }

    return PR_TRUE;
  }

void
nsTSubstring_CharT::SetLength( size_type length )
  {
    if (SetCapacity(length))
      mLength = length;
  }

void
nsTSubstring_CharT::SetIsVoid( PRBool val )
  {
    if (val)
      {
        Truncate();
        mFlags |= F_VOIDED;
      }
    else
      {
        mFlags &= ~F_VOIDED;
      }
  }

PRBool
nsTSubstring_CharT::Equals( const self_type& str ) const
  {
    return mLength == str.mLength && char_traits::compare(mData, str.mData, mLength) == 0;
  }

PRBool
nsTSubstring_CharT::Equals( const self_type& str, const comparator_type& comp ) const
  {
    return mLength == str.mLength && comp(mData, str.mData, mLength, str.mLength) == 0;
  }

PRBool
nsTSubstring_CharT::Equals( const char_type* data ) const
  {
    
    if (!data)
      {
        NS_NOTREACHED("null data pointer");
        return mLength == 0;
      }

    
    size_type length = char_traits::length(data);
    return mLength == length && char_traits::compare(mData, data, mLength) == 0;
  }

PRBool
nsTSubstring_CharT::Equals( const char_type* data, const comparator_type& comp ) const
  {
    
    if (!data)
      {
        NS_NOTREACHED("null data pointer");
        return mLength == 0;
      }

    
    size_type length = char_traits::length(data);
    return mLength == length && comp(mData, data, mLength, length) == 0;
  }

PRBool
nsTSubstring_CharT::EqualsASCII( const char* data, size_type len ) const
  {
    return mLength == len && char_traits::compareASCII(mData, data, len) == 0;
  }

PRBool
nsTSubstring_CharT::EqualsASCII( const char* data ) const
  {
    return char_traits::compareASCIINullTerminated(mData, mLength, data) == 0;
  }

PRBool
nsTSubstring_CharT::LowerCaseEqualsASCII( const char* data, size_type len ) const
  {
    return mLength == len && char_traits::compareLowerCaseToASCII(mData, data, len) == 0;
  }

PRBool
nsTSubstring_CharT::LowerCaseEqualsASCII( const char* data ) const
  {
    return char_traits::compareLowerCaseToASCIINullTerminated(mData, mLength, data) == 0;
  }

nsTSubstring_CharT::size_type
nsTSubstring_CharT::CountChar( char_type c ) const
  {
    const char_type *start = mData;
    const char_type *end   = mData + mLength;

    return NS_COUNT(start, end, c);
  }

PRInt32
nsTSubstring_CharT::FindChar( char_type c, index_type offset ) const
  {
    if (offset < mLength)
      {
        const char_type* result = char_traits::find(mData + offset, mLength - offset, c);
        if (result)
          return result - mData;
      }
    return -1;
  }

void
nsTSubstring_CharT::StripChar( char_type aChar, PRInt32 aOffset )
  {
    if (mLength == 0 || aOffset >= PRInt32(mLength))
      return;

    EnsureMutable(); 

    

    char_type* to   = mData + aOffset;
    char_type* from = mData + aOffset;
    char_type* end  = mData + mLength;

    while (from < end)
      {
        char_type theChar = *from++;
        if (aChar != theChar)
          *to++ = theChar;
      }
    *to = char_type(0); 
    mLength = to - mData;
  }

void
nsTSubstring_CharT::StripChars( const char_type* aChars, PRUint32 aOffset )
  {
    if (aOffset >= PRUint32(mLength))
      return;

    EnsureMutable(); 

    

    char_type* to   = mData + aOffset;
    char_type* from = mData + aOffset;
    char_type* end  = mData + mLength;

    while (from < end)
      {
        char_type theChar = *from++;
        const char_type* test = aChars;

        for (; *test && *test != theChar; ++test);

        if (!*test) {
          
          *to++ = theChar;
        }
      }
    *to = char_type(0); 
    mLength = to - mData;
  }

void nsTSubstring_CharT::AppendPrintf( const char* format, ...)
  {
    char buf[32];
    va_list ap;
    va_start(ap, format);
    PRUint32 len = PR_vsnprintf(buf, sizeof(buf), format, ap);
    AppendASCII(buf, len);
    va_end(ap);
  }



#ifdef CharT_is_PRUnichar






static void 
Modified_cnvtf(char *buf, int bufsz, int prcsn, double fval)
{
  PRIntn decpt, sign, numdigits;
  char *num, *nump;
  char *bufp = buf;
  char *endnum;

  
  num = (char*)malloc(bufsz);
  if (num == NULL) {
    buf[0] = '\0';
    return;
  }
  if (PR_dtoa(fval, 2, prcsn, &decpt, &sign, &endnum, num, bufsz)
      == PR_FAILURE) {
    buf[0] = '\0';
    goto done;
  }
  numdigits = endnum - num;
  nump = num;

  





  if (sign && fval < 0.0f) {
    *bufp++ = '-';
  }

  if (decpt == 9999) {
    while ((*bufp++ = *nump++) != 0) {} 
    goto done;
  }

  if (decpt > (prcsn+1) || decpt < -(prcsn-1) || decpt < -5) {
    *bufp++ = *nump++;
    if (numdigits != 1) {
      *bufp++ = '.';
    }

    while (*nump != '\0') {
      *bufp++ = *nump++;
    }
    *bufp++ = 'e';
    PR_snprintf(bufp, bufsz - (bufp - buf), "%+d", decpt-1);
  }
  else if (decpt >= 0) {
    if (decpt == 0) {
      *bufp++ = '0';
    }
    else {
      while (decpt--) {
        if (*nump != '\0') {
          *bufp++ = *nump++;
        }
        else {
          *bufp++ = '0';
        }
      }
    }
    if (*nump != '\0') {
      *bufp++ = '.';
      while (*nump != '\0') {
        *bufp++ = *nump++;
      }
    }
    *bufp++ = '\0';
  }
  else if (decpt < 0) {
    *bufp++ = '0';
    *bufp++ = '.';
    while (decpt++) {
      *bufp++ = '0';
    }

    while (*nump != '\0') {
      *bufp++ = *nump++;
    }
    *bufp++ = '\0';
  }
done:
  free(num);
}
#endif 

void
nsTSubstring_CharT::DoAppendFloat( double aFloat, int digits )
{
  char buf[40];
  
  
  Modified_cnvtf(buf, sizeof(buf), digits, aFloat);
  AppendASCII(buf);
}

