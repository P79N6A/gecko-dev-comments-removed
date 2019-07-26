





















#ifndef _STLP_INTERNAL_TIME_FACETS_H
#define _STLP_INTERNAL_TIME_FACETS_H

#ifndef _STLP_INTERNAL_CTIME
#  include <stl/_ctime.h>                
#endif

#ifndef _STLP_C_LOCALE_H
#  include <stl/c_locale.h>
#endif

#ifndef _STLP_IOS_BASE_H
#  include <stl/_ios_base.h>
#endif

#ifndef _STLP_INTERNAL_IOSTREAM_STRING_H
#  include <stl/_iostream_string.h>
#endif

#ifndef _STLP_FACETS_FWD_H
#  include <stl/_facets_fwd.h>
#endif

_STLP_BEGIN_NAMESPACE

_STLP_MOVE_TO_PRIV_NAMESPACE














class _STLP_CLASS_DECLSPEC _Time_Info_Base {
public:
  string _M_time_format;
  string _M_date_format;
  string _M_date_time_format;
  string _M_long_date_format;
  string _M_long_date_time_format;
};

class _STLP_CLASS_DECLSPEC _Time_Info : public _Time_Info_Base {
public:
  string _M_dayname[14];
  string _M_monthname[24];
  string _M_am_pm[2];
};

#ifndef _STLP_NO_WCHAR_T
class _STLP_CLASS_DECLSPEC _WTime_Info : public _Time_Info_Base {
public:
  wstring _M_dayname[14];
  wstring _M_monthname[24];
  wstring _M_am_pm[2];
};
#endif

_STLP_MOVE_TO_STD_NAMESPACE

class _STLP_CLASS_DECLSPEC time_base {
public:
  enum dateorder {no_order, dmy, mdy, ymd, ydm};
};

_STLP_MOVE_TO_PRIV_NAMESPACE

template <class _Ch>
class time_init;

_STLP_TEMPLATE_NULL
class _STLP_CLASS_DECLSPEC time_init<char> {
protected:
  time_init();
  time_init(const char *name);
  time_init(_Locale_time*);
#if defined (__BORLANDC__)
  static
#endif
  _Time_Info _M_timeinfo;
  time_base::dateorder _M_dateorder;
};

#ifndef _STLP_NO_WCHAR_T
_STLP_TEMPLATE_NULL
class _STLP_CLASS_DECLSPEC time_init<wchar_t> {
protected:
  time_init();
  time_init(const char *name);
  time_init(_Locale_time*);
#if defined (__BORLANDC__)
  static
#endif
  _WTime_Info _M_timeinfo;
  time_base::dateorder _M_dateorder;
};
#endif

_STLP_MOVE_TO_STD_NAMESPACE

template <class _Ch, class _InIt>
class time_get : public locale::facet, public time_base, public _STLP_PRIV time_init<_Ch> {
public:
  typedef _Ch   char_type;
  typedef _InIt iter_type;

  explicit time_get(size_t __refs = 0) : locale::facet(__refs)
  {}

  dateorder date_order() const { return do_date_order(); }
  iter_type get_time(iter_type __s, iter_type  __end, ios_base&  __str,
                     ios_base::iostate&  __err, tm* __t) const
  { return do_get_time(__s,  __end,  __str,  __err, __t); }
  iter_type get_date(iter_type __s, iter_type  __end, ios_base&  __str,
                     ios_base::iostate&  __err, tm* __t) const
  { return do_get_date(__s,  __end,  __str,  __err, __t); }
  iter_type get_weekday(iter_type __s, iter_type  __end, ios_base&  __str,
                        ios_base::iostate&  __err, tm* __t) const
  { return do_get_weekday(__s,  __end,  __str,  __err, __t); }
  iter_type get_monthname(iter_type __s, iter_type  __end, ios_base&  __str,
                          ios_base::iostate&  __err, tm* __t) const
  { return do_get_monthname(__s,  __end,  __str,  __err, __t); }
  iter_type get_year(iter_type __s, iter_type  __end, ios_base&  __str,
                     ios_base::iostate&  __err, tm* __t) const
  { return do_get_year(__s,  __end,  __str,  __err, __t); }

  static locale::id id;

protected:
  time_get(const char* __name, size_t __refs)
    : locale::facet(__refs), _STLP_PRIV time_init<_Ch>(__name)
  {}
  time_get(_Locale_time *__time)
    : _STLP_PRIV time_init<_Ch>(__time)
  {}

  ~time_get() {}

  virtual dateorder do_date_order() const { return this->_M_dateorder; }

  virtual iter_type do_get_time(iter_type __s, iter_type  __end,
                                ios_base&, ios_base::iostate&  __err,
                                tm* __t) const;

  virtual iter_type do_get_date(iter_type __s, iter_type  __end,
                                ios_base&, ios_base::iostate& __err,
                                tm* __t) const;

  virtual iter_type do_get_weekday(iter_type __s, iter_type  __end,
                                   ios_base&,
                                   ios_base::iostate& __err,
                                   tm* __t) const;
  virtual iter_type do_get_monthname(iter_type __s, iter_type  __end,
                                     ios_base&,
                                     ios_base::iostate& __err,
                                     tm* __t) const;

  virtual iter_type do_get_year(iter_type __s, iter_type  __end,
                                ios_base&, ios_base::iostate& __err,
                                tm* __t) const;
};

#if defined (_STLP_LIMITED_DEFAULT_TEMPLATES)
template <class _Ch, class _InIt>
#else
template <class _Ch, class _InIt = istreambuf_iterator<_Ch, char_traits<_Ch> > >
#endif
class time_get_byname : public time_get<_Ch, _InIt> {
  friend class _Locale_impl;
public:
  typedef  time_base::dateorder dateorder;
  typedef _InIt                 iter_type;

  explicit time_get_byname(const char* __name, size_t __refs = 0)
    : time_get<_Ch, _InIt>(__name, __refs) {}

protected:
  ~time_get_byname() {}
  dateorder do_date_order() const { return this->_M_dateorder; }

private:
  time_get_byname(_Locale_time *__time)
    : time_get<_Ch, _InIt>(__time)
  {}

  typedef time_get_byname<_Ch, _InIt> _Self;
  
  time_get_byname(_Self const&);
  _Self& operator = (_Self const&);
};











_STLP_MOVE_TO_PRIV_NAMESPACE

_STLP_DECLSPEC void _STLP_CALL
__write_formatted_time(__iostring&, const ctype<char>& __ct,
                       char __format, char __modifier,
                       const _Time_Info& __table, const tm* __t);

#ifndef _STLP_NO_WCHAR_T
_STLP_DECLSPEC void _STLP_CALL
__write_formatted_time(__iowstring&, const ctype<wchar_t>& __ct,
                       char __format, char __modifier,
                       const _WTime_Info& __table, const tm* __t);
#endif

_STLP_MOVE_TO_STD_NAMESPACE

template <class _Ch, class _OutIt>
class time_put : public locale::facet, public time_base, public _STLP_PRIV time_init<_Ch> {
public:
  typedef _Ch      char_type;
  typedef _OutIt iter_type;

  explicit time_put(size_t __refs = 0) : locale::facet(__refs)
  {}

  _OutIt put(iter_type __s, ios_base& __f, _Ch __fill,
                  const tm* __tmb,
                  const _Ch* __pat, const _Ch* __pat_end) const;

  _OutIt put(iter_type __s, ios_base& __f, _Ch  __fill,
                  const tm* __tmb, char __format, char __modifier = 0) const
  { return do_put(__s, __f,  __fill, __tmb, __format, __modifier); }

  static locale::id id;

protected:
  time_put(const char* __name, size_t __refs)
    : locale::facet(__refs), _STLP_PRIV time_init<_Ch>(__name)
  {}
  time_put(_Locale_time *__time)
    : _STLP_PRIV time_init<_Ch>(__time)
  {}
  ~time_put() {}
  virtual iter_type do_put(iter_type __s, ios_base& __f,
                           char_type  , const tm* __tmb,
                           char __format, char ) const;
};

#if defined (_STLP_LIMITED_DEFAULT_TEMPLATES)
template <class _Ch, class _OutIt>
#else
template <class _Ch, class _OutIt = ostreambuf_iterator<_Ch, char_traits<_Ch> > >
#endif
class time_put_byname : public time_put<_Ch, _OutIt> {
  friend class _Locale_impl;
public:
  typedef time_base::dateorder dateorder;
  typedef _OutIt iter_type;
  typedef _Ch   char_type;

  explicit time_put_byname(const char * __name, size_t __refs = 0)
    : time_put<_Ch, _OutIt>(__name, __refs)
  {}

protected:
  ~time_put_byname() {}

private:
  time_put_byname(_Locale_time *__time)
    : time_put<_Ch, _OutIt>(__time)
  {}

  typedef time_put_byname<_Ch, _OutIt> _Self;
  
  time_put_byname(_Self const&);
  _Self& operator = (_Self const&);
};

#if defined (_STLP_USE_TEMPLATE_EXPORT)
_STLP_EXPORT_TEMPLATE_CLASS time_get<char, istreambuf_iterator<char, char_traits<char> > >;
_STLP_EXPORT_TEMPLATE_CLASS time_put<char, ostreambuf_iterator<char, char_traits<char> > >;
#  if !defined (_STLP_NO_WCHAR_T)
_STLP_EXPORT_TEMPLATE_CLASS time_get<wchar_t, istreambuf_iterator<wchar_t, char_traits<wchar_t> > >;
_STLP_EXPORT_TEMPLATE_CLASS time_put<wchar_t, ostreambuf_iterator<wchar_t, char_traits<wchar_t> > >;
#  endif

#endif

_STLP_END_NAMESPACE

#if defined (_STLP_EXPOSE_STREAM_IMPLEMENTATION) && !defined (_STLP_LINK_TIME_INSTANTIATION)
#  include <stl/_time_facets.c>
#endif

#endif 




