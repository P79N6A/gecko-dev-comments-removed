
















#ifndef MESSAGE_FACETS_H
#define MESSAGE_FACETS_H

#include <string>
#include <locale>
#include <hash_map>

#include "c_locale.h"

_STLP_BEGIN_NAMESPACE
_STLP_MOVE_TO_PRIV_NAMESPACE






struct _Catalog_locale_map {
  _Catalog_locale_map() : M(0) {}
  ~_Catalog_locale_map() { if (M) delete M; }

  void insert(nl_catd_type key, const locale& L);
  locale lookup(nl_catd_type key) const;
  void erase(nl_catd_type key);

  typedef hash_map<nl_catd_type, locale, hash<nl_catd_type>, equal_to<nl_catd_type>, 
                   allocator<pair<_STLP_CONST nl_catd_type, locale> > > map_type;
  map_type *M;

private:                        
  _Catalog_locale_map(const _Catalog_locale_map&);
  void operator=(const _Catalog_locale_map&);
};











#if defined (_STLP_USE_GLIBC2_LOCALIZATION)
#  define _STLP_USE_NL_CATD_MAPPING
#else



_STLP_STATIC_ASSERT(sizeof(nl_catd_type) <= sizeof(int))
#endif

class _STLP_CLASS_DECLSPEC _Catalog_nl_catd_map {
public:
  _Catalog_nl_catd_map()
  {}
  ~_Catalog_nl_catd_map()
  {}

  typedef hash_map<messages_base::catalog, nl_catd_type, hash<messages_base::catalog>, equal_to<messages_base::catalog>,
                   allocator<pair<_STLP_CONST messages_base::catalog, nl_catd_type> > > map_type;
  typedef hash_map<nl_catd_type, messages_base::catalog, hash<nl_catd_type>, equal_to<nl_catd_type>,
                   allocator<pair<_STLP_CONST nl_catd_type, messages_base::catalog> > > rmap_type;
  
  

  messages_base::catalog insert(nl_catd_type cat)
#if !defined (_STLP_USE_NL_CATD_MAPPING)
  { return (messages_base::catalog)cat; }
#else
  ;
#endif

  void erase(messages_base::catalog)
#if !defined (_STLP_USE_NL_CATD_MAPPING)
  {}
#else
  ;
#endif

  nl_catd_type operator [] ( messages_base::catalog cat )
#if !defined (_STLP_USE_NL_CATD_MAPPING)
  { return cat; }
#else
  { return cat < 0 ? 0 : M[cat]; }
#endif

private:
  _Catalog_nl_catd_map(const _Catalog_nl_catd_map&);
  _Catalog_nl_catd_map& operator =(const _Catalog_nl_catd_map&);

#if defined (_STLP_USE_NL_CATD_MAPPING)
  map_type M;
  rmap_type Mr;
  static _STLP_VOLATILE __stl_atomic_t _count;
#endif
};

class _Messages {
public:
  typedef messages_base::catalog catalog;

  _Messages(bool, const char *name);
  _Messages(bool, _Locale_messages*);

  catalog do_open(const string& __fn, const locale& __loc) const;
  string do_get(catalog __c, int __set, int __msgid,
                const string& __dfault) const;
#if !defined (_STLP_NO_WCHAR_T)
  wstring do_get(catalog __c, int __set, int __msgid,
                 const wstring& __dfault) const;
#endif
  void do_close(catalog __c) const; 
  ~_Messages(); 

private:
  _Locale_messages* _M_message_obj;
  _Catalog_locale_map* _M_map;
  mutable _Catalog_nl_catd_map _M_cat;

  
  _Messages(const _Messages&);
  _Messages& operator=(const _Messages&);
};

_STLP_MOVE_TO_STD_NAMESPACE

_STLP_END_NAMESPACE

#endif




