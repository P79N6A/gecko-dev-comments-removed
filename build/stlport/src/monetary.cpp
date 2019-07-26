
















#include "stlport_prefix.h"

#include <locale>
#include <istream>

_STLP_BEGIN_NAMESPACE

static void _Init_monetary_formats(money_base::pattern& pos_format,
                                   money_base::pattern& neg_format) {
  pos_format.field[0] = (char) money_base::symbol;
  pos_format.field[1] = (char) money_base::sign;
  pos_format.field[2] = (char) money_base::none;
  pos_format.field[3] = (char) money_base::value;

  neg_format.field[0] = (char) money_base::symbol;
  neg_format.field[1] = (char) money_base::sign;
  neg_format.field[2] = (char) money_base::none;
  neg_format.field[3] = (char) money_base::value;
}


static const char _S_empty_string[] = "";
#ifndef _STLP_NO_WCHAR_T
static const wchar_t _S_empty_wstring[] = L"";
#endif





moneypunct<char, true>::moneypunct(size_t __refs) : locale::facet(__refs)
{ _Init_monetary_formats(_M_pos_format, _M_neg_format); }
moneypunct<char, true>::~moneypunct() {}

char moneypunct<char, true>::do_decimal_point() const {return ' ';}
char moneypunct<char, true>::do_thousands_sep() const {return ' ';}
string moneypunct<char, true>::do_grouping() const { return _S_empty_string; }
string moneypunct<char, true>::do_curr_symbol() const { return _S_empty_string; }
string moneypunct<char, true>::do_positive_sign() const { return _S_empty_string; }
string moneypunct<char, true>::do_negative_sign() const { return _S_empty_string; }
money_base::pattern moneypunct<char, true>::do_pos_format() const  {return _M_pos_format;}
money_base::pattern moneypunct<char, true>::do_neg_format() const {return _M_neg_format;}
int moneypunct<char, true>::do_frac_digits() const {return 0;}

moneypunct<char, false>::moneypunct(size_t __refs) : locale::facet(__refs)
{ _Init_monetary_formats(_M_pos_format, _M_neg_format); }
moneypunct<char, false>::~moneypunct() {}

char moneypunct<char, false>::do_decimal_point() const {return ' ';}
char moneypunct<char, false>::do_thousands_sep() const {return ' ';}

string moneypunct<char, false>::do_grouping() const { return _S_empty_string; }
string moneypunct<char, false>::do_curr_symbol() const { return _S_empty_string; }
string moneypunct<char, false>::do_positive_sign() const { return _S_empty_string; }
string moneypunct<char, false>::do_negative_sign() const { return _S_empty_string; }
money_base::pattern moneypunct<char, false>::do_pos_format() const {return _M_pos_format;}
money_base::pattern moneypunct<char, false>::do_neg_format() const {return _M_neg_format;}
int moneypunct<char, false>::do_frac_digits() const {return 0;}

#ifndef _STLP_NO_WCHAR_T
moneypunct<wchar_t, true>::moneypunct(size_t __refs) : locale::facet(__refs)
{ _Init_monetary_formats(_M_pos_format, _M_neg_format); }
moneypunct<wchar_t, true>::~moneypunct() {}

wchar_t moneypunct<wchar_t, true>::do_decimal_point() const {return L' ';}
wchar_t moneypunct<wchar_t, true>::do_thousands_sep() const {return L' ';}
string moneypunct<wchar_t, true>::do_grouping() const {return _S_empty_string;}

wstring moneypunct<wchar_t, true>::do_curr_symbol() const
{return _S_empty_wstring;}
wstring moneypunct<wchar_t, true>::do_positive_sign() const
{return _S_empty_wstring;}
wstring moneypunct<wchar_t, true>::do_negative_sign() const
{return _S_empty_wstring;}
int moneypunct<wchar_t, true>::do_frac_digits() const {return 0;}
money_base::pattern moneypunct<wchar_t, true>::do_pos_format() const
{return _M_pos_format;}
money_base::pattern moneypunct<wchar_t, true>::do_neg_format() const
{return _M_neg_format;}

moneypunct<wchar_t, false>::moneypunct(size_t __refs) : locale::facet(__refs)
{ _Init_monetary_formats(_M_pos_format, _M_neg_format); }
moneypunct<wchar_t, false>::~moneypunct() {}

wchar_t moneypunct<wchar_t, false>::do_decimal_point() const {return L' ';}
wchar_t moneypunct<wchar_t, false>::do_thousands_sep() const {return L' ';}
string moneypunct<wchar_t, false>::do_grouping() const { return _S_empty_string;}
wstring moneypunct<wchar_t, false>::do_curr_symbol() const
{return _S_empty_wstring;}
wstring moneypunct<wchar_t, false>::do_positive_sign() const
{return _S_empty_wstring;}
wstring moneypunct<wchar_t, false>::do_negative_sign() const
{return _S_empty_wstring;}
int moneypunct<wchar_t, false>::do_frac_digits() const {return 0;}

money_base::pattern moneypunct<wchar_t, false>::do_pos_format() const
{return _M_pos_format;}
money_base::pattern moneypunct<wchar_t, false>::do_neg_format() const
{return _M_neg_format;}

#endif 





#if !defined (_STLP_NO_FORCE_INSTANTIATE)

template class _STLP_CLASS_DECLSPEC money_get<char, istreambuf_iterator<char, char_traits<char> > >;
template class _STLP_CLASS_DECLSPEC money_put<char, ostreambuf_iterator<char, char_traits<char> > >;


#  ifndef _STLP_NO_WCHAR_T
template class _STLP_CLASS_DECLSPEC money_get<wchar_t, istreambuf_iterator<wchar_t, char_traits<wchar_t> > >;
template class _STLP_CLASS_DECLSPEC money_put<wchar_t, ostreambuf_iterator<wchar_t, char_traits<wchar_t> > >;


#  endif

#endif

#if !defined (_STLP_STATIC_CONST_INIT_BUG) && !defined (_STLP_NO_STATIC_CONST_DEFINITION)
const bool moneypunct<char, true>::intl;
const bool moneypunct<char, false>::intl;
#  ifndef _STLP_NO_WCHAR_T
const bool moneypunct<wchar_t, true>::intl;
const bool moneypunct<wchar_t, false>::intl;
#  endif
#endif

_STLP_END_NAMESPACE




