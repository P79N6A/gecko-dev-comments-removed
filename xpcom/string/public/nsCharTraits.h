




#ifndef nsCharTraits_h___
#define nsCharTraits_h___

#include <ctype.h>
  

#define FORCED_CPP_2BYTE_WCHAR_T


#if defined(HAVE_CPP_2BYTE_WCHAR_T) && !defined(FORCED_CPP_2BYTE_WCHAR_T)
#define USE_CPP_WCHAR_FUNCS
#endif

#ifdef USE_CPP_WCHAR_FUNCS
#include <wchar.h>
  
#endif

#include <string.h>
  

#ifndef nscore_h___
#include "nscore.h"
  
#endif





#ifdef NS_NO_XPCOM
#define NS_WARNING(msg)
#define NS_ASSERTION(cond, msg)
#define NS_ERROR(msg)
#else
#ifndef nsDebug_h__
#include "nsDebug.h"
  
#endif
#endif





















#define PLANE1_BASE          uint32_t(0x00010000)

#define NS_IS_HIGH_SURROGATE(u) ((uint32_t(u) & 0xFFFFFC00) == 0xD800)

#define NS_IS_LOW_SURROGATE(u)  ((uint32_t(u) & 0xFFFFFC00) == 0xDC00)

#define IS_SURROGATE(u)      ((uint32_t(u) & 0xFFFFF800) == 0xD800)






#define SURROGATE_TO_UCS4(h, l) (((uint32_t(h) & 0x03FF) << 10) + \
                                 (uint32_t(l) & 0x03FF) + PLANE1_BASE)






#define H_SURROGATE(c) PRUnichar(PRUnichar(uint32_t(c) >> 10) + \
                                 PRUnichar(0xD7C0)) 






#define L_SURROGATE(c) PRUnichar(PRUnichar(uint32_t(c) & uint32_t(0x03FF)) | \
                                 PRUnichar(0xDC00))

#define IS_IN_BMP(ucs) (uint32_t(ucs) < PLANE1_BASE)
#define UCS2_REPLACEMENT_CHAR PRUnichar(0xFFFD)

#define UCS_END uint32_t(0x00110000)
#define IS_VALID_CHAR(c) ((uint32_t(c) < UCS_END) && !IS_SURROGATE(c))
#define ENSURE_VALID_CHAR(c) (IS_VALID_CHAR(c) ? (c) : UCS2_REPLACEMENT_CHAR)

template <class CharT> struct nsCharTraits {};

template <>
struct nsCharTraits<PRUnichar>
  {
    typedef PRUnichar char_type;
    typedef uint16_t  unsigned_char_type;
    typedef char      incompatible_char_type;

    static char_type *sEmptyBuffer;

    static
    void
    assign( char_type& lhs, char_type rhs )
      {
        lhs = rhs;
      }


      

#ifdef USE_CPP_WCHAR_FUNCS
    typedef wint_t int_type;
#else
    typedef int int_type;
#endif

    static
    char_type
    to_char_type( int_type c )
      {
        return char_type(c);
      }

    static
    int_type
    to_int_type( char_type c )
      {
        return int_type( static_cast<unsigned_char_type>(c) );
      }

    static
    bool
    eq_int_type( int_type lhs, int_type rhs )
      {
        return lhs == rhs;
      }


      

    static
    bool
    eq( char_type lhs, char_type rhs )
      {
        return lhs == rhs;
      }

    static
    bool
    lt( char_type lhs, char_type rhs )
      {
        return lhs < rhs;
      }


      

    static
    char_type*
    move( char_type* s1, const char_type* s2, size_t n )
      {
        return static_cast<char_type*>(memmove(s1, s2, n * sizeof(char_type)));
      }

    static
    char_type*
    copy( char_type* s1, const char_type* s2, size_t n )
      {
        return static_cast<char_type*>(memcpy(s1, s2, n * sizeof(char_type)));
      }

    static
    char_type*
    copyASCII( char_type* s1, const char* s2, size_t n )
      {
        for (char_type* s = s1; n--; ++s, ++s2) {
          NS_ASSERTION(!(*s2 & ~0x7F), "Unexpected non-ASCII character");
          *s = *s2;
        }
        return s1;
      }

    static
    char_type*
    assign( char_type* s, size_t n, char_type c )
      {
#ifdef USE_CPP_WCHAR_FUNCS
        return static_cast<char_type*>(wmemset(s, to_int_type(c), n));
#else
        char_type* result = s;
        while ( n-- )
          assign(*s++, c);
        return result;
#endif
      }

    static
    int
    compare( const char_type* s1, const char_type* s2, size_t n )
      {
#ifdef USE_CPP_WCHAR_FUNCS
        return wmemcmp(s1, s2, n);
#else
        for ( ; n--; ++s1, ++s2 )
          {
            if ( !eq(*s1, *s2) )
              return to_int_type(*s1) - to_int_type(*s2);
          }

        return 0;
#endif
      }

    static
    int
    compareASCII( const char_type* s1, const char* s2, size_t n )
      {
        for ( ; n--; ++s1, ++s2 )
          {
            NS_ASSERTION(!(*s2 & ~0x7F), "Unexpected non-ASCII character");
            if ( !eq_int_type(to_int_type(*s1), to_int_type(*s2)) )
              return to_int_type(*s1) - to_int_type(*s2);
          }

        return 0;
      }

    
    
    
    static
    int
    compareASCIINullTerminated( const char_type* s1, size_t n, const char* s2 )
      {
        for ( ; n--; ++s1, ++s2 )
          {
            if ( !*s2 )
              return 1;
            NS_ASSERTION(!(*s2 & ~0x7F), "Unexpected non-ASCII character");
            if ( !eq_int_type(to_int_type(*s1), to_int_type(*s2)) )
              return to_int_type(*s1) - to_int_type(*s2);
          }

        if ( *s2 )
          return -1;

        return 0;
      }

    







    static
    char_type
    ASCIIToLower( char_type c )
      {
        if (c < 0x100)
          {
            if (c >= 'A' && c <= 'Z')
              return char_type(c + ('a' - 'A'));
          
            return c;
          }
        else
          {
            if (c == 0x212A) 
              return 'k';
            if (c == 0x0130) 
              return 'i';
            return c;
          }
      }

    static
    int
    compareLowerCaseToASCII( const char_type* s1, const char* s2, size_t n )
      {
        for ( ; n--; ++s1, ++s2 )
          {
            NS_ASSERTION(!(*s2 & ~0x7F), "Unexpected non-ASCII character");
            NS_ASSERTION(!(*s2 >= 'A' && *s2 <= 'Z'),
                         "Unexpected uppercase character");
            char_type lower_s1 = ASCIIToLower(*s1);
            if ( lower_s1 != to_char_type(*s2) )
              return to_int_type(lower_s1) - to_int_type(*s2);
          }

        return 0;
      }

    
    
    
    static
    int
    compareLowerCaseToASCIINullTerminated( const char_type* s1, size_t n, const char* s2 )
      {
        for ( ; n--; ++s1, ++s2 )
          {
            if ( !*s2 )
              return 1;
            NS_ASSERTION(!(*s2 & ~0x7F), "Unexpected non-ASCII character");
            NS_ASSERTION(!(*s2 >= 'A' && *s2 <= 'Z'),
                         "Unexpected uppercase character");
            char_type lower_s1 = ASCIIToLower(*s1);
            if ( lower_s1 != to_char_type(*s2) )
              return to_int_type(lower_s1) - to_int_type(*s2);
          }

        if ( *s2 )
          return -1;

        return 0;
      }

    static
    size_t
    length( const char_type* s )
      {
#ifdef USE_CPP_WCHAR_FUNCS
        return wcslen(s);
#else
        size_t result = 0;
        while ( !eq(*s++, char_type(0)) )
          ++result;
        return result;
#endif
      }

    static
    const char_type*
    find( const char_type* s, size_t n, char_type c )
      {
#ifdef USE_CPP_WCHAR_FUNCS
        return reinterpret_cast<const char_type*>(wmemchr(s, to_int_type(c), n));
#else
        while ( n-- )
          {
            if ( eq(*s, c) )
              return s;
            ++s;
          }

        return 0;
#endif
      }

#if 0
      

    typedef streamoff off_type;
    typedef streampos pos_type;
    typedef mbstate_t state_type;

    static
    int_type
    eof()
      {
#ifdef USE_CPP_WCHAR_FUNCS
        return WEOF;
#else
        return EOF;
#endif
      }

    static
    int_type
    not_eof( int_type c )
      {
        return eq_int_type(c, eof()) ? ~eof() : c;
      }

    
#endif
  };

template <>
struct nsCharTraits<char>
  {
    typedef char           char_type;
    typedef unsigned char  unsigned_char_type;
    typedef PRUnichar      incompatible_char_type;

    static char_type *sEmptyBuffer;

    static
    void
    assign( char_type& lhs, char_type rhs )
      {
        lhs = rhs;
      }


      

    typedef int int_type;

    static
    char_type
    to_char_type( int_type c )
      {
        return char_type(c);
      }

    static
    int_type
    to_int_type( char_type c )
      {
        return int_type( static_cast<unsigned_char_type>(c) );
      }

    static
    bool
    eq_int_type( int_type lhs, int_type rhs )
      {
        return lhs == rhs;
      }


      

    static
    bool
    eq( char_type lhs, char_type rhs )
      {
        return lhs == rhs;
      }

    static
    bool
    lt( char_type lhs, char_type rhs )
      {
        return lhs < rhs;
      }


      

    static
    char_type*
    move( char_type* s1, const char_type* s2, size_t n )
      {
        return static_cast<char_type*>(memmove(s1, s2, n * sizeof(char_type)));
      }

    static
    char_type*
    copy( char_type* s1, const char_type* s2, size_t n )
      {
        return static_cast<char_type*>(memcpy(s1, s2, n * sizeof(char_type)));
      }

    static
    char_type*
    copyASCII( char_type* s1, const char* s2, size_t n )
      {
        return copy(s1, s2, n);
      }

    static
    char_type*
    assign( char_type* s, size_t n, char_type c )
      {
        return static_cast<char_type*>(memset(s, to_int_type(c), n));
      }

    static
    int
    compare( const char_type* s1, const char_type* s2, size_t n )
      {
        return memcmp(s1, s2, n);
      }

    static
    int
    compareASCII( const char_type* s1, const char* s2, size_t n )
      {
#ifdef DEBUG
        for (size_t i = 0; i < n; ++i)
          {
            NS_ASSERTION(!(s2[i] & ~0x7F), "Unexpected non-ASCII character");
          }
#endif
        return compare(s1, s2, n);
      }

    
    
    
    static
    int
    compareASCIINullTerminated( const char_type* s1, size_t n, const char* s2 )
      {
        
        
        for ( ; n--; ++s1, ++s2 )
          {
            if ( !*s2 )
              return 1;
            NS_ASSERTION(!(*s2 & ~0x7F), "Unexpected non-ASCII character");
            if ( *s1 != *s2 )
              return to_int_type(*s1) - to_int_type(*s2);
          }

        if ( *s2 )
          return -1;

        return 0;
      }

    


    static
    char_type
    ASCIIToLower( char_type c )
      {
        if (c >= 'A' && c <= 'Z')
          return char_type(c + ('a' - 'A'));

        return c;
      }

    static
    int
    compareLowerCaseToASCII( const char_type* s1, const char* s2, size_t n )
      {
        for ( ; n--; ++s1, ++s2 )
          {
            NS_ASSERTION(!(*s2 & ~0x7F), "Unexpected non-ASCII character");
            NS_ASSERTION(!(*s2 >= 'A' && *s2 <= 'Z'),
                         "Unexpected uppercase character");
            char_type lower_s1 = ASCIIToLower(*s1);
            if ( lower_s1 != *s2 )
              return to_int_type(lower_s1) - to_int_type(*s2);
          }
        return 0;
      }

    
    
    
    static
    int
    compareLowerCaseToASCIINullTerminated( const char_type* s1, size_t n, const char* s2 )
      {
        for ( ; n--; ++s1, ++s2 )
          {
            if ( !*s2 )
              return 1;
            NS_ASSERTION(!(*s2 & ~0x7F), "Unexpected non-ASCII character");
            NS_ASSERTION(!(*s2 >= 'A' && *s2 <= 'Z'),
                         "Unexpected uppercase character");
            char_type lower_s1 = ASCIIToLower(*s1);
            if ( lower_s1 != *s2 )
              return to_int_type(lower_s1) - to_int_type(*s2);
          }

        if ( *s2 )
          return -1;

        return 0;
      }

    static
    size_t
    length( const char_type* s )
      {
        return strlen(s);
      }

    static
    const char_type*
    find( const char_type* s, size_t n, char_type c )
      {
        return reinterpret_cast<const char_type*>(memchr(s, to_int_type(c), n));
      }

#if 0
      

    typedef streamoff off_type;
    typedef streampos pos_type;
    typedef mbstate_t state_type;

    static
    int_type
    eof()
      {
        return EOF;
      }

    static
    int_type
    not_eof( int_type c )
      {
        return eq_int_type(c, eof()) ? ~eof() : c;
      }

    
#endif
  };

template <class InputIterator>
struct nsCharSourceTraits
  {
    typedef typename InputIterator::difference_type difference_type;

    static
    uint32_t
    readable_distance( const InputIterator& first, const InputIterator& last )
      {
        
        return uint32_t(last.get() - first.get());
      }

    static
    const typename InputIterator::value_type*
    read( const InputIterator& iter )
      {
        return iter.get();
      }

    static
    void
    advance( InputIterator& s, difference_type n )
      {
        s.advance(n);
      }
  };

#ifdef HAVE_CPP_PARTIAL_SPECIALIZATION

template <class CharT>
struct nsCharSourceTraits<CharT*>
  {
    typedef ptrdiff_t difference_type;

    static
    uint32_t
    readable_distance( CharT* s )
      {
        return uint32_t(nsCharTraits<CharT>::length(s));

      }

    static
    uint32_t
    readable_distance( CharT* first, CharT* last )
      {
        return uint32_t(last-first);
      }

    static
    const CharT*
    read( CharT* s )
      {
        return s;
      }

    static
    void
    advance( CharT*& s, difference_type n )
      {
        s += n;
      }
  };

#else

template <>
struct nsCharSourceTraits<const char*>
  {
    typedef ptrdiff_t difference_type;

    static
    uint32_t
    readable_distance( const char* s )
      {
        return uint32_t(nsCharTraits<char>::length(s));

      }

    static
    uint32_t
    readable_distance( const char* first, const char* last )
      {
        return uint32_t(last-first);
      }

    static
    const char*
    read( const char* s )
      {
        return s;
      }

    static
    void
    advance( const char*& s, difference_type n )
      {
        s += n;
      }
 };


template <>
struct nsCharSourceTraits<const PRUnichar*>
  {
    typedef ptrdiff_t difference_type;

    static
    uint32_t
    readable_distance( const PRUnichar* s )
      {
        return uint32_t(nsCharTraits<PRUnichar>::length(s));

      }

    static
    uint32_t
    readable_distance( const PRUnichar* first, const PRUnichar* last )
      {
        return uint32_t(last-first);
      }

    static
    const PRUnichar*
    read( const PRUnichar* s )
      {
        return s;
      }

    static
    void
    advance( const PRUnichar*& s, difference_type n )
      {
        s += n;
      }
 };

#endif


template <class OutputIterator>
struct nsCharSinkTraits
  {
    static
    void
    write( OutputIterator& iter, const typename OutputIterator::value_type* s, uint32_t n )
      {
        iter.write(s, n);
      }
  };

#ifdef HAVE_CPP_PARTIAL_SPECIALIZATION

template <class CharT>
struct nsCharSinkTraits<CharT*>
  {
    static
    void
    write( CharT*& iter, const CharT* s, uint32_t n )
      {
        nsCharTraits<CharT>::move(iter, s, n);
        iter += n;
      }
  };

#else

template <>
struct nsCharSinkTraits<char*>
  {
    static
    void
    write( char*& iter, const char* s, uint32_t n )
      {
        nsCharTraits<char>::move(iter, s, n);
        iter += n;
      }
  };

template <>
struct nsCharSinkTraits<PRUnichar*>
  {
    static
    void
    write( PRUnichar*& iter, const PRUnichar* s, uint32_t n )
      {
        nsCharTraits<PRUnichar>::move(iter, s, n);
        iter += n;
      }
  };

#endif

#endif 
