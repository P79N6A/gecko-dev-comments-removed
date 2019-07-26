



#ifndef nsUTF8Utils_h_
#define nsUTF8Utils_h_





#include "nscore.h"
#include "mozilla/SSE.h"

#include "nsCharTraits.h"

class UTF8traits
  {
    public:
      static bool isASCII(char c) { return (c & 0x80) == 0x00; }
      static bool isInSeq(char c) { return (c & 0xC0) == 0x80; }
      static bool is2byte(char c) { return (c & 0xE0) == 0xC0; }
      static bool is3byte(char c) { return (c & 0xF0) == 0xE0; }
      static bool is4byte(char c) { return (c & 0xF8) == 0xF0; }
      static bool is5byte(char c) { return (c & 0xFC) == 0xF8; }
      static bool is6byte(char c) { return (c & 0xFE) == 0xFC; }
  };









class UTF8CharEnumerator
{
public:
  static uint32_t NextChar(const char **buffer, const char *end,
                           bool *err)
  {
    NS_ASSERTION(buffer && *buffer, "null buffer!");

    const char *p = *buffer;
    *err = false;

    if (p >= end)
      {
        *err = true;

        return 0;
      }

    char c = *p++;

    if ( UTF8traits::isASCII(c) )
      {
        *buffer = p;
        return c;
      }

    uint32_t ucs4;
    uint32_t minUcs4;
    int32_t state = 0;

    if (!CalcState(c, ucs4, minUcs4, state)) {
        NS_ERROR("Not a UTF-8 string. This code should only be used for converting from known UTF-8 strings.");
        *err = true;

        return 0;
    }

    while ( state-- )
      {
        if (p == end)
          {
            *err = true;

            return 0;
          }

        c = *p++;

        if (!AddByte(c, state, ucs4))
          {
            *err = true;

            return 0;
          }
      }

      if ( ucs4 < minUcs4 )
        {
          
          ucs4 = UCS2_REPLACEMENT_CHAR;
        }
      else if ( ucs4 >= 0xD800 &&
                (ucs4 <= 0xDFFF || ucs4 >= UCS_END))
        {
          
          ucs4 = UCS2_REPLACEMENT_CHAR;
        }

    *buffer = p;
    return ucs4;
  }

private:
  static bool CalcState(char c, uint32_t& ucs4, uint32_t& minUcs4,
                          int32_t& state)
  {
    if ( UTF8traits::is2byte(c) )
      {
        ucs4 = (uint32_t(c) << 6) & 0x000007C0L;
        state = 1;
        minUcs4 = 0x00000080;
      }
    else if ( UTF8traits::is3byte(c) )
      {
        ucs4 = (uint32_t(c) << 12) & 0x0000F000L;
        state = 2;
        minUcs4 = 0x00000800;
      }
    else if ( UTF8traits::is4byte(c) )
      {
        ucs4 = (uint32_t(c) << 18) & 0x001F0000L;
        state = 3;
        minUcs4 = 0x00010000;
      }
    else if ( UTF8traits::is5byte(c) )
      {
        ucs4 = (uint32_t(c) << 24) & 0x03000000L;
        state = 4;
        minUcs4 = 0x00200000;
      }
    else if ( UTF8traits::is6byte(c) )
      {
        ucs4 = (uint32_t(c) << 30) & 0x40000000L;
        state = 5;
        minUcs4 = 0x04000000;
      }
    else
      {
        return false;
      }

    return true;
  }

  static bool AddByte(char c, int32_t state, uint32_t& ucs4)
  {
    if ( UTF8traits::isInSeq(c) )
      {
        int32_t shift = state * 6;
        ucs4 |= (uint32_t(c) & 0x3F) << shift;
        return true;
      }

    return false;
  }
};









class UTF16CharEnumerator
{
public:
  static uint32_t NextChar(const char16_t **buffer, const char16_t *end,
                           bool *err = nullptr)
  {
    NS_ASSERTION(buffer && *buffer, "null buffer!");

    const char16_t *p = *buffer;

    if (p >= end)
      {
        NS_ERROR("No input to work with");
        if (err)
          *err = true;

        return 0;
      }

    char16_t c = *p++;

    if (!IS_SURROGATE(c)) 
      {
        if (err)
          *err = false;
        *buffer = p;
        return c;
      }
    else if (NS_IS_HIGH_SURROGATE(c)) 
      {
        if (p == end)
          {
            
            
            

            NS_WARNING("Unexpected end of buffer after high surrogate");

            if (err)
              *err = true;
            *buffer = p;
            return 0xFFFD;
          }

        
        char16_t h = c;

        c = *p++;

        if (NS_IS_LOW_SURROGATE(c))
          {
            
            
            uint32_t ucs4 = SURROGATE_TO_UCS4(h, c);
            if (err)
              *err = false;
            *buffer = p;
            return ucs4;
          }
        else
          {
            
            
            
            
            
            
            
            
            
            NS_WARNING("got a High Surrogate but no low surrogate");

            if (err)
              *err = true;
            *buffer = p - 1;
            return 0xFFFD;
          }
      }
    else 
      {
        

        
        
        

        NS_WARNING("got a low Surrogate but no high surrogate");
        if (err)
          *err = true;
        *buffer = p;
        return 0xFFFD;
      }

    if (err)
      *err = true;
    return 0;
  }
};






class ConvertUTF8toUTF16
  {
    public:
      typedef char      value_type;
      typedef char16_t buffer_type;

    ConvertUTF8toUTF16( buffer_type* aBuffer )
        : mStart(aBuffer), mBuffer(aBuffer), mErrorEncountered(false) {}

    size_t Length() const { return mBuffer - mStart; }

    bool ErrorEncountered() const { return mErrorEncountered; }

    void write( const value_type* start, uint32_t N )
      {
        if ( mErrorEncountered )
          return;

        
        
        const value_type* p = start;
        const value_type* end = start + N;
        buffer_type* out = mBuffer;
        for ( ; p != end ; )
          {
            bool err;
            uint32_t ucs4 = UTF8CharEnumerator::NextChar(&p, end, &err);

            if ( err )
              {
                mErrorEncountered = true;
                mBuffer = out;
                return;
              }

            if ( ucs4 >= PLANE1_BASE )
              {
                *out++ = (buffer_type)H_SURROGATE(ucs4);
                *out++ = (buffer_type)L_SURROGATE(ucs4);
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
      bool mErrorEncountered;
  };





class CalculateUTF8Length
  {
    public:
      typedef char value_type;

    CalculateUTF8Length() : mLength(0), mErrorEncountered(false) { }

    size_t Length() const { return mLength; }

    void write( const value_type* start, uint32_t N )
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
                
                
                
                
                
                

                
                
                
                
                
                

                
                
                

                
                
                
                
                
                
                
                

                
                
                
                
                

                if (p + 4 <= end) {
                  uint32_t c = ((uint32_t)(p[0] & 0x07)) << 6 |
                               ((uint32_t)(p[1] & 0x30));
                  if (c >= 0x010 && c < 0x110)
                    ++mLength;
                }

                p += 4;
            }
            else if ( UTF8traits::is5byte(*p) )
                p += 5;
            else if ( UTF8traits::is6byte(*p) )
                p += 6;
            else 
              {
                ++mLength; 
                break;
              }
          }
        if ( p != end )
          {
            NS_ERROR("Not a UTF-8 string. This code should only be used for converting from known UTF-8 strings.");
            --mLength; 
            mErrorEncountered = true;
          }
      }

    private:
      size_t mLength;
      bool mErrorEncountered;
  };






class ConvertUTF16toUTF8
  {
    public:
      typedef char16_t value_type;
      typedef char      buffer_type;

    
    
    

    ConvertUTF16toUTF8( buffer_type* aBuffer )
        : mStart(aBuffer), mBuffer(aBuffer) {}

    size_t Size() const { return mBuffer - mStart; }

    void write( const value_type* start, uint32_t N )
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
                    
                    
                    uint32_t ucs4 = SURROGATE_TO_UCS4(h, c);

                    
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
      typedef char16_t value_type;

    CalculateUTF8Size()
      : mSize(0) { }

    size_t Size() const { return mSize; }

    void write( const value_type* start, uint32_t N )
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




class LossyConvertEncoding8to16
  {
    public:
      typedef char      value_type;
      typedef char      input_type;
      typedef char16_t output_type;

    public:
      LossyConvertEncoding8to16( char16_t* aDestination ) :
        mDestination(aDestination) { }

      void
      write( const char* aSource, uint32_t aSourceLength )
        {
#ifdef MOZILLA_MAY_SUPPORT_SSE2
          if (mozilla::supports_sse2())
            {
              write_sse2(aSource, aSourceLength);
              return;
            }
#endif
          const char* done_writing = aSource + aSourceLength;
          while ( aSource < done_writing )
            *mDestination++ = (char16_t)(unsigned char)(*aSource++);
        }

      void
      write_sse2( const char* aSource, uint32_t aSourceLength );

      void
      write_terminator()
        {
          *mDestination = (char16_t)(0);
        }

    private:
      char16_t* mDestination;
  };





class LossyConvertEncoding16to8
  {
    public:
      typedef char16_t value_type;
      typedef char16_t input_type;
      typedef char      output_type;

      LossyConvertEncoding16to8( char* aDestination ) : mDestination(aDestination) { }

      void
      write( const char16_t* aSource, uint32_t aSourceLength)
        {
#ifdef MOZILLA_MAY_SUPPORT_SSE2
          if (mozilla::supports_sse2())
            {
              write_sse2(aSource, aSourceLength);
              return;
            }
#endif
            const char16_t* done_writing = aSource + aSourceLength;
            while ( aSource < done_writing )
              *mDestination++ = (char)(*aSource++);
        }

#ifdef MOZILLA_MAY_SUPPORT_SSE2
      void
      write_sse2( const char16_t* aSource, uint32_t aSourceLength );
#endif

      void
      write_terminator()
        {
          *mDestination = '\0';
        }

    private:
      char *mDestination;
  };
#endif 

#endif 
