





































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

#ifndef nsDebug_h__
#include "nsDebug.h"
  
#endif

#ifdef HAVE_CPP_BOOL
  typedef bool nsCharTraits_bool;
#else
  typedef PRBool nsCharTraits_bool;
#endif





















#define PLANE1_BASE          PRUint32(0x00010000)

#define NS_IS_HIGH_SURROGATE(u) ((PRUint32(u) & 0xFFFFFC00) == 0xD800)

#define NS_IS_LOW_SURROGATE(u)  ((PRUint32(u) & 0xFFFFFC00) == 0xDC00)

#define IS_SURROGATE(u)      ((PRUint32(u) & 0xFFFFF800) == 0xD800)






#define SURROGATE_TO_UCS4(h, l) (((PRUint32(h) & 0x03FF) << 10) + \
                                 (PRUint32(l) & 0x03FF) + PLANE1_BASE)






#define H_SURROGATE(c) PRUnichar(PRUnichar(PRUint32(c) >> 10) + \
                                 PRUnichar(0xD7C0)) 






#define L_SURROGATE(c) PRUnichar(PRUnichar(PRUint32(c) & PRUint32(0x03FF)) | \
                                 PRUnichar(0xDC00))

#define IS_IN_BMP(ucs) (PRUint32(ucs) < PLANE1_BASE)
#define UCS2_REPLACEMENT_CHAR PRUnichar(0xFFFD)

#define UCS_END PRUint32(0x00110000)
#define IS_VALID_CHAR(c) ((PRUint32(c) < UCS_END) && !IS_SURROGATE(c))
#define ENSURE_VALID_CHAR(c) (IS_VALID_CHAR(c) ? (c) : UCS2_REPLACEMENT_CHAR)

template <class CharT> struct nsCharTraits {};

NS_SPECIALIZE_TEMPLATE
struct nsCharTraits<PRUnichar>
  {
    typedef PRUnichar char_type;
    typedef PRUint16  unsigned_char_type;
    typedef char      incompatible_char_type;

    NS_COM static const char_type *sEmptyBuffer;

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
    nsCharTraits_bool
    eq_int_type( int_type lhs, int_type rhs )
      {
        return lhs == rhs;
      }


      

    static
    nsCharTraits_bool
    eq( char_type lhs, char_type rhs )
      {
        return lhs == rhs;
      }

    static
    nsCharTraits_bool
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
          return (c >= 'A' && c <= 'Z') ? c + ('a' - 'A') : c;
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

NS_SPECIALIZE_TEMPLATE
struct nsCharTraits<char>
  {
    typedef char           char_type;
    typedef unsigned char  unsigned_char_type;
    typedef PRUnichar      incompatible_char_type;

    NS_COM static const char_type *sEmptyBuffer;

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
    nsCharTraits_bool
    eq_int_type( int_type lhs, int_type rhs )
      {
        return lhs == rhs;
      }


      

    static
    nsCharTraits_bool
    eq( char_type lhs, char_type rhs )
      {
        return lhs == rhs;
      }

    static
    nsCharTraits_bool
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
        return (c >= 'A' && c <= 'Z') ? (c + ('a' - 'A')) : c;
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
    PRUint32
    readable_distance( const InputIterator& first, const InputIterator& last )
      {
        
        return last.get() - first.get();
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
    PRUint32
    readable_distance( CharT* s )
      {
        return PRUint32(nsCharTraits<CharT>::length(s));

      }

    static
    PRUint32
    readable_distance( CharT* first, CharT* last )
      {
        return PRUint32(last-first);
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

NS_SPECIALIZE_TEMPLATE
struct nsCharSourceTraits<const char*>
  {
    typedef ptrdiff_t difference_type;

    static
    PRUint32
    readable_distance( const char* s )
      {
        return PRUint32(nsCharTraits<char>::length(s));

      }

    static
    PRUint32
    readable_distance( const char* first, const char* last )
      {
        return PRUint32(last-first);
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


NS_SPECIALIZE_TEMPLATE
struct nsCharSourceTraits<const PRUnichar*>
  {
    typedef ptrdiff_t difference_type;

    static
    PRUint32
    readable_distance( const PRUnichar* s )
      {
        return PRUint32(nsCharTraits<PRUnichar>::length(s));

      }

    static
    PRUint32
    readable_distance( const PRUnichar* first, const PRUnichar* last )
      {
        return PRUint32(last-first);
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
    PRUint32
    write( OutputIterator& iter, const typename OutputIterator::value_type* s, PRUint32 n )
      {
        return iter.write(s, n);
      }
  };

#ifdef HAVE_CPP_PARTIAL_SPECIALIZATION

template <class CharT>
struct nsCharSinkTraits<CharT*>
  {
    static
    PRUint32
    write( CharT*& iter, const CharT* s, PRUint32 n )
      {
        nsCharTraits<CharT>::move(iter, s, n);
        iter += n;
        return n;
      }
  };

#else

NS_SPECIALIZE_TEMPLATE
struct nsCharSinkTraits<char*>
  {
    static
    PRUint32
    write( char*& iter, const char* s, PRUint32 n )
      {
        nsCharTraits<char>::move(iter, s, n);
        iter += n;
        return n;
      }
  };

NS_SPECIALIZE_TEMPLATE
struct nsCharSinkTraits<PRUnichar*>
  {
    static
    PRUint32
    write( PRUnichar*& iter, const PRUnichar* s, PRUint32 n )
      {
        nsCharTraits<PRUnichar>::move(iter, s, n);
        iter += n;
        return n;
      }
  };

#endif

#endif 
