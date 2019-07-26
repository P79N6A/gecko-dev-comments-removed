

















#include "stlport_prefix.h"

#include <locale>
#include <ostream>

_STLP_BEGIN_NAMESPACE




template <class Char>
static ptrdiff_t
__insert_grouping_aux(Char* first, Char* last, const string& grouping,
                      Char separator, Char Plus, Char Minus,
                      int basechars) {
  typedef string::size_type str_size;

  if (first == last)
    return 0;

  int sign = 0;

  if (*first == Plus || *first == Minus) {
    sign = 1;
    ++first;
  }

  first += basechars;
  Char* cur_group = last; 
                          
  int groupsize = 0; 
                     

  for ( str_size n = 0; ; ) { 
    if ( n < grouping.size() ) {
      groupsize = __STATIC_CAST(int, grouping[n++] );
    }

    if ((groupsize <= 0) || (groupsize >= cur_group - first) || (groupsize == CHAR_MAX)) {
      break;
    }

    
    cur_group -= groupsize;
    ++last;
    copy_backward(cur_group, last, last + 1);
    *cur_group = separator;
  }

  return (last - first) + sign + basechars;
}


template <class Char, class Str>
static void
__insert_grouping_aux(  Str& iostr, size_t __group_pos,
                      const string& grouping,
                      Char separator, Char Plus, Char Minus,
                      int basechars) {
  typedef string::size_type str_size;

  if (iostr.size() < __group_pos)
    return;

  int __first_pos = 0;
  Char __first = *iostr.begin();

  if (__first == Plus || __first == Minus) {
    ++__first_pos;
  }

  __first_pos += basechars;

  typename Str::iterator cur_group(iostr.begin() + __group_pos);    
                                                                    
  int groupsize = 0; 
                     

  for ( str_size n = 0; ; ) { 
    if ( n < grouping.size() ) {
      groupsize = __STATIC_CAST( int, grouping[n++] );
    }

    if ( (groupsize <= 0) || (groupsize >= ((cur_group - iostr.begin()) - __first_pos)) ||
         (groupsize == CHAR_MAX)) {
      break;
    }

    
    cur_group -= groupsize;
    cur_group = iostr.insert(cur_group, separator);
  }
}




_STLP_MOVE_TO_PRIV_NAMESPACE

_STLP_DECLSPEC const char* _STLP_CALL __hex_char_table_lo()
{ return "0123456789abcdefx"; }

_STLP_DECLSPEC const char* _STLP_CALL __hex_char_table_hi()
{ return "0123456789ABCDEFX"; }

char* _STLP_CALL
__write_integer(char* buf, ios_base::fmtflags flags, long x) {
  char tmp[64];
  char* bufend = tmp+64;
  char* beg = __write_integer_backward(bufend, flags, x);
  return copy(beg, bufend, buf);
}



ptrdiff_t _STLP_CALL
__insert_grouping(char * first, char * last, const string& grouping,
                  char separator, char Plus, char Minus, int basechars) {
  return __insert_grouping_aux(first, last, grouping,
                               separator, Plus, Minus, basechars);
}

void _STLP_CALL
__insert_grouping(__iostring &str, size_t group_pos, const string& grouping,
                  char separator, char Plus, char Minus, int basechars) {
  __insert_grouping_aux(str, group_pos, grouping, separator, Plus, Minus, basechars);
}

#if !defined (_STLP_NO_WCHAR_T)
ptrdiff_t _STLP_CALL
__insert_grouping(wchar_t* first, wchar_t* last, const string& grouping,
                  wchar_t separator, wchar_t Plus, wchar_t Minus,
                  int basechars) {
  return __insert_grouping_aux(first, last, grouping, separator,
                               Plus, Minus, basechars);
}

void _STLP_CALL
__insert_grouping(__iowstring &str, size_t group_pos, const string& grouping,
                  wchar_t separator, wchar_t Plus, wchar_t Minus,
                  int basechars) {
  __insert_grouping_aux(str, group_pos, grouping, separator, Plus, Minus, basechars);
}
#endif

_STLP_MOVE_TO_STD_NAMESPACE



#if !defined(_STLP_NO_FORCE_INSTANTIATE)
template class _STLP_CLASS_DECLSPEC ostreambuf_iterator<char, char_traits<char> >;

template class num_put<char, ostreambuf_iterator<char, char_traits<char> > >;
# ifndef _STLP_NO_WCHAR_T
template class ostreambuf_iterator<wchar_t, char_traits<wchar_t> >;
template class num_put<wchar_t, ostreambuf_iterator<wchar_t, char_traits<wchar_t> > >;

# endif 
#endif

_STLP_END_NAMESPACE




