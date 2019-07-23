




































#ifndef nsUTF8Utils_h_
#define nsUTF8Utils_h_





#include "nscore.h"

#include "nsCharTraits.h"

class UTF8traits
  {
    public:
      static PRBool isASCII(char c) { return (c & 0x80) == 0x00; }
      static PRBool isInSeq(char c) { return (c & 0xC0) == 0x80; }
      static PRBool is2byte(char c) { return (c & 0xE0) == 0xC0; }
      static PRBool is3byte(char c) { return (c & 0xF0) == 0xE0; }
      static PRBool is4byte(char c) { return (c & 0xF8) == 0xF0; }
      static PRBool is5byte(char c) { return (c & 0xFC) == 0xF8; }
      static PRBool is6byte(char c) { return (c & 0xFE) == 0xFC; }
  };









class UTF8CharEnumerator
{
public:
  static PRUint32 NextChar(const char **buffer, const char *end,
                           PRBool *err = nsnull, PRBool* overlong = nsnull)
  {
    NS_ASSERTION(buffer && *buffer, "null buffer!");

    const char *p = *buffer;

    if (p >= end)
      {
        if (err)
          *err = PR_TRUE;

        return 0;
      }

    char c = *p++;

    if ( UTF8traits::isASCII(c) )
      {
        if (err)
          *err = PR_FALSE;
        if (overlong)
          *overlong = PR_FALSE;
        *buffer = p;
        return c;
      }

    PRUint32 ucs4;
    PRUint32 minUcs4;
    PRInt32 state = 0;

    if (!CalcState(c, ucs4, minUcs4, state)) {
        NS_ERROR("Not a UTF-8 string. This code should only be used for converting from known UTF-8 strings.");
        if (err)
          *err = PR_TRUE;
        return 0;
    }

    while ( state-- )
      {
        if (p == end)
          {
            if (err)
              *err = PR_TRUE;

            return 0;
          }

        c = *p++;

        if (!AddByte(c, state, ucs4))
          {
            NS_ERROR("not a UTF8 string");
            if (err)
              *err = PR_TRUE;
            return 0;
          }
      }

    if (err)
      *err = PR_FALSE;
    if (overlong)
      *overlong = ucs4 < minUcs4;
    *buffer = p;
    return ucs4;
  }

#ifdef MOZILLA_INTERNAL_API

  static PRUint32 NextChar(nsACString::const_iterator& iter,
                           const nsACString::const_iterator& end,
                           PRBool *err = nsnull, PRBool *overlong = nsnull)
  {
    if ( iter == end )
      {
        NS_ERROR("No input to work with");
        if (err)
          *err = PR_TRUE;

        return 0;
      }

    char c = *iter++;

    if ( UTF8traits::isASCII(c) )
      {
        if (err)
          *err = PR_FALSE;
        if (overlong)
          *overlong = PR_FALSE;
        return c;
      }

    PRUint32 ucs4;
    PRUint32 minUcs4;
    PRInt32 state = 0;

    if (!CalcState(c, ucs4, minUcs4, state)) {
        NS_ERROR("Not a UTF-8 string. This code should only be used for converting from known UTF-8 strings.");
        if (err)
          *err = PR_TRUE;
        return 0;
    }

    while ( state-- )
      {
        if (iter == end)
          {
            NS_ERROR("Buffer ended in the middle of a multibyte sequence");
            if (err)
              *err = PR_TRUE;

            return 0;
          }

        c = *iter++;

        if (!AddByte(c, state, ucs4))
          {
            NS_ERROR("not a UTF8 string");
            if (err)
              *err = PR_TRUE;
            return 0;
          }
      }

    if (err)
      *err = PR_FALSE;
    if (overlong)
      *overlong = ucs4 < minUcs4;
    return ucs4;
  }
#endif 

private:
  static PRBool CalcState(char c, PRUint32& ucs4, PRUint32& minUcs4,
                          PRInt32& state)
  {
    if ( UTF8traits::is2byte(c) )
      {
        ucs4 = (PRUint32(c) << 6) & 0x000007C0L;
        state = 1;
        minUcs4 = 0x00000080;
      }
    else if ( UTF8traits::is3byte(c) )
      {
        ucs4 = (PRUint32(c) << 12) & 0x0000F000L;
        state = 2;
        minUcs4 = 0x00000800;
      }
    else if ( UTF8traits::is4byte(c) )
      {
        ucs4 = (PRUint32(c) << 18) & 0x001F0000L;
        state = 3;
        minUcs4 = 0x00010000;
      }
    else if ( UTF8traits::is5byte(c) )
      {
        ucs4 = (PRUint32(c) << 24) & 0x03000000L;
        state = 4;
        minUcs4 = 0x00200000;
      }
    else if ( UTF8traits::is6byte(c) )
      {
        ucs4 = (PRUint32(c) << 30) & 0x40000000L;
        state = 5;
        minUcs4 = 0x04000000;
      }
    else
      {
        return PR_FALSE;
      }

    return PR_TRUE;
  }

  static PRBool AddByte(char c, PRInt32 state, PRUint32& ucs4)
  {
    if ( UTF8traits::isInSeq(c) )
      {
        PRInt32 shift = state * 6;
        ucs4 |= (PRUint32(c) & 0x3F) << shift;
        return PR_TRUE;
      }

    return PR_FALSE;
  }
};









class UTF16CharEnumerator
{
public:
  static PRUint32 NextChar(const PRUnichar **buffer, const PRUnichar *end,
                           PRBool *err = nsnull)
  {
    NS_ASSERTION(buffer && *buffer, "null buffer!");

    const PRUnichar *p = *buffer;

    if (p >= end)
      {
        NS_ERROR("No input to work with");
        if (err)
          *err = PR_TRUE;

        return 0;
      }

    PRUnichar c = *p++;

    if (!IS_SURROGATE(c)) 
      {
        if (err)
          *err = PR_FALSE;
        *buffer = p;
        return c;
      }
    else if (NS_IS_HIGH_SURROGATE(c)) 
      {
        if (p == end)
          {
            
            
            

            NS_WARNING("Unexpected end of buffer after high surrogate");

            if (err)
              *err = PR_TRUE;
            *buffer = p;
            return 0xFFFD;
          }

        
        PRUnichar h = c;

        c = *p++;

        if (NS_IS_LOW_SURROGATE(c))
          {
            
            
            PRUint32 ucs4 = SURROGATE_TO_UCS4(h, c);
            if (err)
              *err = PR_FALSE;
            *buffer = p;
            return ucs4;
          }
        else
          {
            
            
            
            
            
            
            
            
            
            NS_WARNING("got a High Surrogate but no low surrogate");

            if (err)
              *err = PR_TRUE;
            *buffer = p - 1;
            return 0xFFFD;
          }
      }
    else 
      {
        

        
        
        

        NS_WARNING("got a low Surrogate but no high surrogate");
        if (err)
          *err = PR_TRUE;
        *buffer = p;
        return 0xFFFD;
      }

    if (err)
      *err = PR_TRUE;
    return 0;
  }
};






class ConvertUTF8toUTF16
  {
    public:
      typedef char      value_type;
      typedef PRUnichar buffer_type;

    ConvertUTF8toUTF16( buffer_type* aBuffer )
        : mStart(aBuffer), mBuffer(aBuffer), mErrorEncountered(PR_FALSE) {}

    size_t Length() const { return mBuffer - mStart; }

    void NS_ALWAYS_INLINE write( const value_type* start, PRUint32 N )
      {
        if ( mErrorEncountered )
          return;

        
        
        const value_type* p = start;
        const value_type* end = start + N;
        buffer_type* out = mBuffer;
        for ( ; p != end ; )
          {
            PRBool overlong, err;
            PRUint32 ucs4 = UTF8CharEnumerator::NextChar(&p, end, &err,
                                                         &overlong);

            if ( err )
              {
                mErrorEncountered = PR_TRUE;
                mBuffer = out;
                return;
              }

            if ( overlong )
              {
                
                *out++ = UCS2_REPLACEMENT_CHAR;
              }
            else if ( ucs4 <= 0xD7FF )
              {
                *out++ = ucs4;
              }
            else if (  ucs4 <= 0xDFFF )
              {
                
                *out++ = UCS2_REPLACEMENT_CHAR;
              }
            else if ( ucs4 == 0xFFFE || ucs4 == 0xFFFF )
              {
                
                *out++ = UCS2_REPLACEMENT_CHAR;
              }
            else if ( ucs4 >= PLANE1_BASE )
              {
                if ( ucs4 >= UCS_END )
                  *out++ = UCS2_REPLACEMENT_CHAR;
                else {
                  *out++ = (buffer_type)H_SURROGATE(ucs4);
                  *out++ = (buffer_type)L_SURROGATE(ucs4);
                }
              }
            else
              {
                *out++ = ucs4;
              }
          }
        mBuffer = out;
      }

    void write_terminator()
      {
        *mBuffer = buffer_type(0);
      }

    private:
      buffer_type* const mStart;
      buffer_type* mBuffer;
      PRBool mErrorEncountered;
  };





class CalculateUTF8Length
  {
    public:
      typedef char value_type;

    CalculateUTF8Length() : mLength(0), mErrorEncountered(PR_FALSE) { }

    size_t Length() const { return mLength; }

    void NS_ALWAYS_INLINE write( const value_type* start, PRUint32 N )
      {
          
        if ( mErrorEncountered )
            return;

        
        
        const value_type* p = start;
        const value_type* end = start + N;
        for ( ; p < end ; ++mLength )
          {
            if ( UTF8traits::isASCII(*p) )
                p += 1;
            else if ( UTF8traits::is2byte(*p) )
                p += 2;
            else if ( UTF8traits::is3byte(*p) )
                p += 3;
            else if ( UTF8traits::is4byte(*p) ) {
                p += 4;
                
                
                
                
                
                
                
                
                
                
                ++mLength;
            }
            else if ( UTF8traits::is5byte(*p) )
                p += 5;
            else if ( UTF8traits::is6byte(*p) )
                p += 6;
            else
              {
                break;
              }
          }
        if ( p != end )
          {
            NS_ERROR("Not a UTF-8 string. This code should only be used for converting from known UTF-8 strings.");
            mErrorEncountered = PR_TRUE;
          }
      }

    private:
      size_t mLength;
      PRBool mErrorEncountered;
  };






class ConvertUTF16toUTF8
  {
    public:
      typedef PRUnichar value_type;
      typedef char      buffer_type;

    
    
    

    ConvertUTF16toUTF8( buffer_type* aBuffer )
        : mStart(aBuffer), mBuffer(aBuffer) {}

    size_t Size() const { return mBuffer - mStart; }

    void NS_ALWAYS_INLINE write( const value_type* start, PRUint32 N )
      {
        buffer_type *out = mBuffer; 

        for (const value_type *p = start, *end = start + N; p < end; ++p )
          {
            value_type c = *p;
            if (! (c & 0xFF80)) 
              {
                *out++ = (char)c;
              }
            else if (! (c & 0xF800)) 
              {
                *out++ = 0xC0 | (char)(c >> 6);
                *out++ = 0x80 | (char)(0x003F & c);
              }
            else if (!IS_SURROGATE(c)) 
              {
                *out++ = 0xE0 | (char)(c >> 12);
                *out++ = 0x80 | (char)(0x003F & (c >> 6));
                *out++ = 0x80 | (char)(0x003F & c );
              }
            else if (NS_IS_HIGH_SURROGATE(c)) 
              {
                
                value_type h = c;

                ++p;
                if (p == end)
                  {
                    
                    
                    
                    *out++ = '\xEF';
                    *out++ = '\xBF';
                    *out++ = '\xBD';

                    NS_WARNING("String ending in half a surrogate pair!");

                    break;
                  }
                c = *p;

                if (NS_IS_LOW_SURROGATE(c))
                  {
                    
                    
                    PRUint32 ucs4 = SURROGATE_TO_UCS4(h, c);

                    
                    *out++ = 0xF0 | (char)(ucs4 >> 18);
                    *out++ = 0x80 | (char)(0x003F & (ucs4 >> 12));
                    *out++ = 0x80 | (char)(0x003F & (ucs4 >> 6));
                    *out++ = 0x80 | (char)(0x003F & ucs4);
                  }
                else
                  {
                    
                    
                    
                    *out++ = '\xEF';
                    *out++ = '\xBF';
                    *out++ = '\xBD';

                    
                    
                    
                    
                    
                    
                    
                    p--;

                    NS_WARNING("got a High Surrogate but no low surrogate");
                  }
              }
            else 
              {
                
                
                *out++ = '\xEF';
                *out++ = '\xBF';
                *out++ = '\xBD';

                
                NS_WARNING("got a low Surrogate but no high surrogate");
              }
          }

        mBuffer = out;
      }

    void write_terminator()
      {
        *mBuffer = buffer_type(0);
      }

    private:
      buffer_type* const mStart;
      buffer_type* mBuffer;
  };






class CalculateUTF8Size
  {
    public:
      typedef PRUnichar value_type;

    CalculateUTF8Size()
      : mSize(0) { }

    size_t Size() const { return mSize; }

    void NS_ALWAYS_INLINE write( const value_type* start, PRUint32 N )
      {
        
        for (const value_type *p = start, *end = start + N; p < end; ++p )
          {
            value_type c = *p;
            if (! (c & 0xFF80)) 
              mSize += 1;
            else if (! (c & 0xF800)) 
              mSize += 2;
            else if (0xD800 != (0xF800 & c)) 
              mSize += 3;
            else if (0xD800 == (0xFC00 & c)) 
              {
                ++p;
                if (p == end)
                  {
                    
                    
                    
                    mSize += 3;

                    NS_WARNING("String ending in half a surrogate pair!");

                    break;
                  }
                c = *p;

                if (0xDC00 == (0xFC00 & c))
                  mSize += 4;
                else
                  {
                    
                    
                    
                    mSize += 3;

                    
                    
                    
                    
                    
                    
                    
                    p--;

                    NS_WARNING("got a high Surrogate but no low surrogate");
                  }
              }
            else 
              {
                
                
                mSize += 3;

                NS_WARNING("got a low Surrogate but no high surrogate");
              }
          }
      }

    private:
      size_t mSize;
  };

#ifdef MOZILLA_INTERNAL_API




template <class FromCharT, class ToCharT>
class LossyConvertEncoding
  {
    public:
      typedef FromCharT value_type;
 
      typedef FromCharT input_type;
      typedef ToCharT   output_type;

      typedef typename nsCharTraits<FromCharT>::unsigned_char_type unsigned_input_type;

    public:
      LossyConvertEncoding( output_type* aDestination ) : mDestination(aDestination) { }

      void
      write( const input_type* aSource, PRUint32 aSourceLength )
        {
          const input_type* done_writing = aSource + aSourceLength;
          while ( aSource < done_writing )
            *mDestination++ = (output_type)(unsigned_input_type)(*aSource++);  
        }

      void
      write_terminator()
        {
          *mDestination = output_type(0);
        }

    private:
      output_type* mDestination;
  };
#endif 

#endif 
