












#if defined( _STLP_USE_MSIPL ) && !defined( _STLP_MSL_STRING_H_ )
#define _STLP_MSL_STRING_H_


# define basic_string __msl_basic_string
# define b_str_ref __msl_b_str_ref
# define basic_istream __msl_basic_istream
# define basic_ostream __msl_basic_ostream
# define string __msl_string
# define wstring __msl_wstring
# define iterator_traits __msl_iterator_traits

namespace std
{
  template<class charT, class traits> class basic_istream;
  template<class charT, class traits> class basic_ostream;
}

#if defined (_STLP_HAS_INCLUDE_NEXT)
#  include_next <string>
#else
#  include _STLP_NATIVE_HEADER(string)
#endif


# undef basic_string
# undef b_str_ref
# undef basic_istream
# undef basic_ostream
# undef string
# undef wstring
# undef iterator_traits

#endif
