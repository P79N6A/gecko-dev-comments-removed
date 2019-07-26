
























































_STLP_BEGIN_NAMESPACE



#if defined (_STLP_DONT_RETURN_VOID) && defined (_STLP_NO_CLASS_PARTIAL_SPECIALIZATION) && defined (_STLP_MEMBER_TEMPLATE_CLASSES)

template<class _Result, class _Tp>
class _Mem_fun0_ptr : public unary_function<_Tp*, _Result> {
protected:
  typedef _Result (_Tp::*__fun_type) ();
  explicit _Mem_fun0_ptr(__fun_type __f) : _M_f(__f) {}

public:
  _Result operator ()(_Tp* __p) const { return (__p->*_M_f)(); }

private:
  __fun_type _M_f;
};

template<class _Result, class _Tp, class _Arg>
class _Mem_fun1_ptr : public binary_function<_Tp*,_Arg,_Result> {
protected:
  typedef _Result (_Tp::*__fun_type) (_Arg);
  explicit _Mem_fun1_ptr(__fun_type __f) : _M_f(__f) {}

public:
  _Result operator ()(_Tp* __p, _Arg __x) const { return (__p->*_M_f)(__x); }

private:
  __fun_type _M_f;
};

template<class _Result, class _Tp>
class _Const_mem_fun0_ptr : public unary_function<const _Tp*,_Result> {
protected:
  typedef _Result (_Tp::*__fun_type) () const;
  explicit _Const_mem_fun0_ptr(__fun_type __f) : _M_f(__f) {}

public:
  _Result operator ()(const _Tp* __p) const { return (__p->*_M_f)(); }

private:
  __fun_type _M_f;
};

template<class _Result, class _Tp, class _Arg>
class _Const_mem_fun1_ptr : public binary_function<const _Tp*,_Arg,_Result> {
protected:
  typedef _Result (_Tp::*__fun_type) (_Arg) const;
  explicit _Const_mem_fun1_ptr(__fun_type __f) : _M_f(__f) {}

public:
  _Result operator ()(const _Tp* __p, _Arg __x) const {
    return (__p->*_M_f)(__x); }

private:
  __fun_type _M_f;
};

template<class _Result, class _Tp>
class _Mem_fun0_ref : public unary_function<_Tp,_Result> {
protected:
  typedef _Result (_Tp::*__fun_type) ();
  explicit _Mem_fun0_ref(__fun_type __f) : _M_f(__f) {}

public:
  _Result operator ()(_Tp& __p) const { return (__p.*_M_f)(); }

private:
  __fun_type _M_f;
};

template<class _Result, class _Tp, class _Arg>
class _Mem_fun1_ref : public binary_function<_Tp,_Arg,_Result> {
protected:
  typedef _Result (_Tp::*__fun_type) (_Arg);
  explicit _Mem_fun1_ref(__fun_type __f) : _M_f(__f) {}

public:
  _Result operator ()(_Tp& __p, _Arg __x) const { return (__p.*_M_f)(__x); }

private:
  __fun_type _M_f;
};

template<class _Result, class _Tp>
class _Const_mem_fun0_ref : public unary_function<_Tp,_Result> {
protected:
  typedef _Result (_Tp::*__fun_type) () const;
  explicit _Const_mem_fun0_ref(__fun_type __f) : _M_f(__f) {}

public:
  _Result operator ()(const _Tp& __p) const { return (__p.*_M_f)(); }

private:
  __fun_type _M_f;
};

template<class _Result, class _Tp, class _Arg>
class _Const_mem_fun1_ref : public binary_function<_Tp,_Arg,_Result> {
protected:
  typedef _Result (_Tp::*__fun_type) (_Arg) const;
  explicit _Const_mem_fun1_ref(__fun_type __f) : _M_f(__f) {}

public:
  _Result operator ()(const _Tp& __p, _Arg __x) const { return (__p.*_M_f)(__x); }

private:
  __fun_type _M_f;
};

template<class _Result>
struct _Mem_fun_traits {
  template<class _Tp>
  struct _Args0 {
    typedef _Mem_fun0_ptr<_Result,_Tp>            _Ptr;
    typedef _Const_mem_fun0_ptr<_Result,_Tp>      _Ptr_const;
    typedef _Mem_fun0_ref<_Result,_Tp>            _Ref;
    typedef _Const_mem_fun0_ref<_Result,_Tp>      _Ref_const;
  };

  template<class _Tp, class _Arg>
  struct _Args1 {
    typedef _Mem_fun1_ptr<_Result,_Tp,_Arg>       _Ptr;
    typedef _Const_mem_fun1_ptr<_Result,_Tp,_Arg> _Ptr_const;
    typedef _Mem_fun1_ref<_Result,_Tp,_Arg>       _Ref;
    typedef _Const_mem_fun1_ref<_Result,_Tp,_Arg> _Ref_const;
  };
};

template<class _Arg, class _Result>
class _Ptr_fun1_base : public unary_function<_Arg, _Result> {
protected:
  typedef _Result (*__fun_type) (_Arg);
  explicit _Ptr_fun1_base(__fun_type __f) : _M_f(__f) {}

public:
  _Result operator()(_Arg __x) const { return _M_f(__x); }

private:
  __fun_type _M_f;
};

template <class _Arg1, class _Arg2, class _Result>
class _Ptr_fun2_base : public binary_function<_Arg1,_Arg2,_Result> {
protected:
  typedef _Result (*__fun_type) (_Arg1, _Arg2);
  explicit _Ptr_fun2_base(__fun_type __f) : _M_f(__f) {}

public:
  _Result operator()(_Arg1 __x, _Arg2 __y) const { return _M_f(__x, __y); }

private:
  __fun_type _M_f;
};

template<class _Result>
struct _Ptr_fun_traits {
  template<class _Arg> struct _Args1 {
    typedef _Ptr_fun1_base<_Arg,_Result> _Fun;
  };

  template<class _Arg1, class _Arg2> struct _Args2 {
    typedef _Ptr_fun2_base<_Arg1,_Arg2,_Result> _Fun;
  };
};


template<class _Tp>
class _Void_mem_fun0_ptr : public unary_function<_Tp*,void> {
protected:
  typedef void (_Tp::*__fun_type) ();
  explicit _Void_mem_fun0_ptr(__fun_type __f) : _M_f(__f) {}

public:
  void operator ()(_Tp* __p) const { (__p->*_M_f)(); }

private:
  __fun_type _M_f;
};

template<class _Tp, class _Arg>
class _Void_mem_fun1_ptr : public binary_function<_Tp*,_Arg,void> {
protected:
  typedef void (_Tp::*__fun_type) (_Arg);
  explicit _Void_mem_fun1_ptr(__fun_type __f) : _M_f(__f) {}

public:
  void operator ()(_Tp* __p, _Arg __x) const { (__p->*_M_f)(__x); }

private:
  __fun_type _M_f;
};

template<class _Tp>
class _Void_const_mem_fun0_ptr : public unary_function<const _Tp*,void> {
protected:
  typedef void (_Tp::*__fun_type) () const;
  explicit _Void_const_mem_fun0_ptr(__fun_type __f) : _M_f(__f) {}

public:
  void operator ()(const _Tp* __p) const { (__p->*_M_f)(); }

private:
  __fun_type _M_f;
};

template<class _Tp, class _Arg>
class _Void_const_mem_fun1_ptr : public binary_function<const _Tp*,_Arg,void> {
protected:
  typedef void (_Tp::*__fun_type) (_Arg) const;
  explicit _Void_const_mem_fun1_ptr(__fun_type __f) : _M_f(__f) {}

public:
  void operator ()(const _Tp* __p, _Arg __x) const { (__p->*_M_f)(__x); }

private:
  __fun_type _M_f;
};

template<class _Tp>
class _Void_mem_fun0_ref : public unary_function<_Tp,void> {
protected:
  typedef void (_Tp::*__fun_type) ();
  explicit _Void_mem_fun0_ref(__fun_type __f) : _M_f(__f) {}

public:
  void operator ()(_Tp& __p) const { (__p.*_M_f)(); }

private:
  __fun_type _M_f;
};

template<class _Tp, class _Arg>
class _Void_mem_fun1_ref : public binary_function<_Tp,_Arg,void> {
protected:
  typedef void (_Tp::*__fun_type) (_Arg);
  explicit _Void_mem_fun1_ref(__fun_type __f) : _M_f(__f) {}

public:
  void operator ()(_Tp& __p, _Arg __x) const { (__p.*_M_f)(__x); }

private:
  __fun_type _M_f;
};

template<class _Tp>
class _Void_const_mem_fun0_ref : public unary_function<_Tp,void> {
protected:
  typedef void (_Tp::*__fun_type) () const;
  explicit _Void_const_mem_fun0_ref(__fun_type __f) : _M_f(__f) {}

public:
  void operator ()(const _Tp& __p) const { (__p.*_M_f)(); }

private:
  __fun_type _M_f;
};

template<class _Tp, class _Arg>
class _Void_const_mem_fun1_ref : public binary_function<_Tp,_Arg,void> {
protected:
  typedef void (_Tp::*__fun_type) (_Arg) const;
  explicit _Void_const_mem_fun1_ref(__fun_type __f) : _M_f(__f) {}

public:
  void operator ()(const _Tp& __p, _Arg __x) const { (__p.*_M_f)(__x); }

private:
  __fun_type _M_f;
};

_STLP_TEMPLATE_NULL
struct _Mem_fun_traits<void> {
  template<class _Tp> struct _Args0 {
    typedef _Void_mem_fun0_ptr<_Tp>             _Ptr;
    typedef _Void_const_mem_fun0_ptr<_Tp>       _Ptr_const;
    typedef _Void_mem_fun0_ref<_Tp>             _Ref;
    typedef _Void_const_mem_fun0_ref<_Tp>       _Ref_const;
  };

  template<class _Tp, class _Arg> struct _Args1 {
    typedef _Void_mem_fun1_ptr<_Tp,_Arg>        _Ptr;
    typedef _Void_const_mem_fun1_ptr<_Tp,_Arg>  _Ptr_const;
    typedef _Void_mem_fun1_ref<_Tp,_Arg>        _Ref;
    typedef _Void_const_mem_fun1_ref<_Tp,_Arg>  _Ref_const;
  };
};

template<class _Arg>
class _Ptr_void_fun1_base : public unary_function<_Arg, void> {
protected:
  typedef void (*__fun_type) (_Arg);
  explicit _Ptr_void_fun1_base(__fun_type __f) : _M_f(__f) {}

public:
  void operator()(_Arg __x) const { _M_f(__x); }

private:
  __fun_type _M_f;
};

template <class _Arg1, class _Arg2>
class _Ptr_void_fun2_base : public binary_function<_Arg1,_Arg2,void> {
protected:
  typedef void (*__fun_type) (_Arg1, _Arg2);
  explicit _Ptr_void_fun2_base(__fun_type __f) : _M_f(__f) {}

public:
  void operator()(_Arg1 __x, _Arg2 __y) const { _M_f(__x, __y); }

private:
  __fun_type _M_f;
};

_STLP_TEMPLATE_NULL
struct _Ptr_fun_traits<void> {
  template<class _Arg> struct _Args1 {
    typedef _Ptr_void_fun1_base<_Arg> _Fun;
  };

  template<class _Arg1, class _Arg2> struct _Args2 {
    typedef _Ptr_void_fun2_base<_Arg1,_Arg2> _Fun;
  };
};





template<class _Result, class _Arg>
class _Ptr_fun1 :
  public _Ptr_fun_traits<_Result>::_STLP_TEMPLATE _Args1<_Arg>::_Fun {
protected:
  typedef typename _Ptr_fun_traits<_Result>::_STLP_TEMPLATE _Args1<_Arg>::_Fun _Base;
  explicit _Ptr_fun1(typename _Base::__fun_type __f) : _Base(__f) {}
};

template<class _Result, class _Arg1, class _Arg2>
class _Ptr_fun2 :
  public _Ptr_fun_traits<_Result>::_STLP_TEMPLATE _Args2<_Arg1,_Arg2>::_Fun {
protected:
  typedef typename _Ptr_fun_traits<_Result>::_STLP_TEMPLATE _Args2<_Arg1,_Arg2>::_Fun _Base;
  explicit _Ptr_fun2(typename _Base::__fun_type __f) : _Base(__f) {}
};

template <class _Result, class _Tp>
class mem_fun_t :
  public _Mem_fun_traits<_Result>::_STLP_TEMPLATE _Args0<_Tp>::_Ptr {
  typedef typename
    _Mem_fun_traits<_Result>::_STLP_TEMPLATE _Args0<_Tp>::_Ptr _Base;
public:
  explicit mem_fun_t(typename _Base::__fun_type __f) : _Base(__f) {}
};

template <class _Result, class _Tp>
class const_mem_fun_t :
  public _Mem_fun_traits<_Result>::_STLP_TEMPLATE _Args0<_Tp>::_Ptr_const {
  typedef typename
    _Mem_fun_traits<_Result>::_STLP_TEMPLATE _Args0<_Tp>::_Ptr_const _Base;
public:
  explicit const_mem_fun_t(typename _Base::__fun_type __f) : _Base(__f) {}
};

template <class _Result, class _Tp>
class mem_fun_ref_t :
  public _Mem_fun_traits<_Result>::_STLP_TEMPLATE _Args0<_Tp>::_Ref {
  typedef typename
    _Mem_fun_traits<_Result>::_STLP_TEMPLATE _Args0<_Tp>::_Ref _Base;
public:
  explicit mem_fun_ref_t(typename _Base::__fun_type __f) : _Base(__f) {}
};

template <class _Result, class _Tp>
class const_mem_fun_ref_t :
  public _Mem_fun_traits<_Result>::_STLP_TEMPLATE _Args0<_Tp>::_Ref_const {
  typedef typename
    _Mem_fun_traits<_Result>::_STLP_TEMPLATE _Args0<_Tp>::_Ref_const _Base;
public:
  explicit const_mem_fun_ref_t(typename _Base::__fun_type __f) : _Base(__f) {}
};

template <class _Result, class _Tp, class _Arg>
class mem_fun1_t :
  public _Mem_fun_traits<_Result>::_STLP_TEMPLATE _Args1<_Tp,_Arg>::_Ptr {
  typedef typename
    _Mem_fun_traits<_Result>::_STLP_TEMPLATE _Args1<_Tp,_Arg>::_Ptr _Base;
public:
  explicit mem_fun1_t(typename _Base::__fun_type __f) : _Base(__f) {}
};

template <class _Result, class _Tp, class _Arg>
class const_mem_fun1_t :
  public _Mem_fun_traits<_Result>::_STLP_TEMPLATE _Args1<_Tp,_Arg>::_Ptr_const {
  typedef typename
    _Mem_fun_traits<_Result>::_STLP_TEMPLATE _Args1<_Tp,_Arg>::_Ptr_const _Base;
public:
  explicit const_mem_fun1_t(typename _Base::__fun_type __f) : _Base(__f) {}
};

template <class _Result, class _Tp, class _Arg>
class mem_fun1_ref_t :
  public _Mem_fun_traits<_Result>::_STLP_TEMPLATE _Args1<_Tp,_Arg>::_Ref {
  typedef typename
    _Mem_fun_traits<_Result>::_STLP_TEMPLATE _Args1<_Tp,_Arg>::_Ref _Base;
public:
  explicit mem_fun1_ref_t(typename _Base::__fun_type __f) : _Base(__f) {}
};

template <class _Result, class _Tp, class _Arg>
class const_mem_fun1_ref_t :
  public _Mem_fun_traits<_Result>::_STLP_TEMPLATE _Args1<_Tp,_Arg>::_Ref_const {
  typedef typename
    _Mem_fun_traits<_Result>::_STLP_TEMPLATE _Args1<_Tp,_Arg>::_Ref_const _Base;
public:
  explicit const_mem_fun1_ref_t(typename _Base::__fun_type __f) : _Base(__f) {}
};

template <class _Arg, class _Result>
class pointer_to_unary_function :
  public _Ptr_fun1<_Result,_Arg> {
  typedef typename
    _Ptr_fun1<_Result,_Arg>::__fun_type __fun_type;
public:
  explicit pointer_to_unary_function(__fun_type __f)
    : _Ptr_fun1<_Result,_Arg>(__f) {}
};

template <class _Arg1, class _Arg2, class _Result>
class pointer_to_binary_function :
  public _Ptr_fun2<_Result,_Arg1,_Arg2> {
  typedef typename
    _Ptr_fun2<_Result,_Arg1,_Arg2>::__fun_type __fun_type;
public:
  explicit pointer_to_binary_function(__fun_type __f)
    : _Ptr_fun2<_Result,_Arg1,_Arg2>(__f) {}
};

#else

template <class _Ret, class _Tp>
class mem_fun_t : public unary_function<_Tp*,_Ret> {
  typedef _Ret (_Tp::*__fun_type)(void);
public:
  explicit mem_fun_t(__fun_type __pf) : _M_f(__pf) {}
  _Ret operator()(_Tp* __p) const { return (__p->*_M_f)(); }
private:
  __fun_type _M_f;
};

template <class _Ret, class _Tp>
class const_mem_fun_t : public unary_function<const _Tp*,_Ret> {
  typedef _Ret (_Tp::*__fun_type)(void) const;
public:
  explicit const_mem_fun_t(__fun_type __pf) : _M_f(__pf) {}
  _Ret operator()(const _Tp* __p) const { return (__p->*_M_f)(); }
private:
  __fun_type _M_f;
};

template <class _Ret, class _Tp>
class mem_fun_ref_t : public unary_function<_Tp,_Ret> {
  typedef _Ret (_Tp::*__fun_type)(void);
public:
  explicit mem_fun_ref_t(__fun_type __pf) : _M_f(__pf) {}
  _Ret operator()(_Tp& __r) const { return (__r.*_M_f)(); }
private:
  __fun_type _M_f;
};

template <class _Ret, class _Tp>
class const_mem_fun_ref_t : public unary_function<_Tp,_Ret> {
  typedef _Ret (_Tp::*__fun_type)(void) const;
public:
  explicit const_mem_fun_ref_t(__fun_type __pf) : _M_f(__pf) {}
  _Ret operator()(const _Tp& __r) const { return (__r.*_M_f)(); }
private:
  __fun_type _M_f;
};

template <class _Ret, class _Tp, class _Arg>
class mem_fun1_t : public binary_function<_Tp*,_Arg,_Ret> {
  typedef _Ret (_Tp::*__fun_type)(_Arg);
public:
  explicit mem_fun1_t(__fun_type __pf) : _M_f(__pf) {}
  _Ret operator()(_Tp* __p, _Arg __x) const { return (__p->*_M_f)(__x); }
private:
  __fun_type _M_f;
};

template <class _Ret, class _Tp, class _Arg>
class const_mem_fun1_t : public binary_function<const _Tp*,_Arg,_Ret> {
  typedef _Ret (_Tp::*__fun_type)(_Arg) const;
public:
  explicit const_mem_fun1_t(__fun_type __pf) : _M_f(__pf) {}
  _Ret operator()(const _Tp* __p, _Arg __x) const
    { return (__p->*_M_f)(__x); }
private:
  __fun_type _M_f;
};

template <class _Ret, class _Tp, class _Arg>
class mem_fun1_ref_t : public binary_function<_Tp,_Arg,_Ret> {
  typedef _Ret (_Tp::*__fun_type)(_Arg);
public:
  explicit mem_fun1_ref_t(__fun_type __pf) : _M_f(__pf) {}
  _Ret operator()(_Tp& __r, _Arg __x) const { return (__r.*_M_f)(__x); }
private:
  __fun_type _M_f;
};

template <class _Ret, class _Tp, class _Arg>
class const_mem_fun1_ref_t : public binary_function<_Tp,_Arg,_Ret> {
  typedef _Ret (_Tp::*__fun_type)(_Arg) const;
public:
  explicit const_mem_fun1_ref_t(__fun_type __pf) : _M_f(__pf) {}
  _Ret operator()(const _Tp& __r, _Arg __x) const { return (__r.*_M_f)(__x); }
private:
  __fun_type _M_f;
};

template <class _Arg, class _Result>
class pointer_to_unary_function : public unary_function<_Arg, _Result> {
protected:
  _Result (*_M_ptr)(_Arg);
public:
  pointer_to_unary_function() {}
  explicit pointer_to_unary_function(_Result (*__x)(_Arg)) : _M_ptr(__x) {}
  _Result operator()(_Arg __x) const { return _M_ptr(__x); }
};

template <class _Arg1, class _Arg2, class _Result>
class pointer_to_binary_function :
  public binary_function<_Arg1,_Arg2,_Result> {
protected:
    _Result (*_M_ptr)(_Arg1, _Arg2);
public:
    pointer_to_binary_function() {}
    explicit pointer_to_binary_function(_Result (*__x)(_Arg1, _Arg2))
      : _M_ptr(__x) {}
    _Result operator()(_Arg1 __x, _Arg2 __y) const {
      return _M_ptr(__x, __y);
    }
};

#  if defined (_STLP_DONT_RETURN_VOID) && !defined (_STLP_NO_CLASS_PARTIAL_SPECIALIZATION)

template <class _Tp>
class mem_fun_t<void, _Tp> : public unary_function<_Tp*,void> {
  typedef void (_Tp::*__fun_type)(void);
public:
  explicit mem_fun_t _STLP_PSPEC2(void,_Tp) (__fun_type __pf) : _M_f(__pf) {}
  void operator()(_Tp* __p) const { (__p->*_M_f)(); }
private:
  __fun_type _M_f;
};

template <class _Tp>
class const_mem_fun_t<void, _Tp> : public unary_function<const _Tp*,void> {
  typedef void (_Tp::*__fun_type)(void) const;
public:
  explicit const_mem_fun_t _STLP_PSPEC2(void,_Tp) (__fun_type __pf) : _M_f(__pf) {}
  void operator()(const _Tp* __p) const { (__p->*_M_f)(); }
private:
  __fun_type _M_f;
};

template <class _Tp>
class mem_fun_ref_t<void, _Tp> : public unary_function<_Tp,void> {
  typedef void (_Tp::*__fun_type)(void);
public:
  explicit mem_fun_ref_t _STLP_PSPEC2(void,_Tp) (__fun_type __pf) : _M_f(__pf) {}
  void operator()(_Tp& __r) const { (__r.*_M_f)(); }
private:
  __fun_type _M_f;
};

template <class _Tp>
class const_mem_fun_ref_t<void, _Tp> : public unary_function<_Tp,void> {
  typedef void (_Tp::*__fun_type)(void) const;
public:
  explicit const_mem_fun_ref_t _STLP_PSPEC2(void,_Tp) (__fun_type __pf) : _M_f(__pf) {}
  void operator()(const _Tp& __r) const { (__r.*_M_f)(); }
private:
  __fun_type _M_f;
};

template <class _Tp, class _Arg>
class mem_fun1_t<void, _Tp, _Arg> : public binary_function<_Tp*,_Arg,void> {
  typedef void (_Tp::*__fun_type)(_Arg);
public:
  explicit mem_fun1_t _STLP_PSPEC3(void,_Tp,_Arg) (__fun_type __pf) : _M_f(__pf) {}
  void operator()(_Tp* __p, _Arg __x) const { (__p->*_M_f)(__x); }
private:
  __fun_type _M_f;
};

template <class _Tp, class _Arg>
class const_mem_fun1_t<void, _Tp, _Arg>
  : public binary_function<const _Tp*,_Arg,void> {
  typedef void (_Tp::*__fun_type)(_Arg) const;
public:
  explicit const_mem_fun1_t _STLP_PSPEC3(void,_Tp,_Arg) (__fun_type __pf) : _M_f(__pf) {}
  void operator()(const _Tp* __p, _Arg __x) const { (__p->*_M_f)(__x); }
private:
  __fun_type _M_f;
};

template <class _Tp, class _Arg>
class mem_fun1_ref_t<void, _Tp, _Arg>
  : public binary_function<_Tp,_Arg,void> {
  typedef void (_Tp::*__fun_type)(_Arg);
public:
  explicit mem_fun1_ref_t _STLP_PSPEC3(void,_Tp,_Arg) (__fun_type __pf) : _M_f(__pf) {}
  void operator()(_Tp& __r, _Arg __x) const { (__r.*_M_f)(__x); }
private:
  __fun_type _M_f;
};

template <class _Tp, class _Arg>
class const_mem_fun1_ref_t<void, _Tp, _Arg>
  : public binary_function<_Tp,_Arg,void> {
  typedef void (_Tp::*__fun_type)(_Arg) const;
public:
  explicit const_mem_fun1_ref_t _STLP_PSPEC3(void,_Tp,_Arg) (__fun_type __pf) : _M_f(__pf) {}
  void operator()(const _Tp& __r, _Arg __x) const { (__r.*_M_f)(__x); }
private:
  __fun_type _M_f;
};

template <class _Arg>
class pointer_to_unary_function<_Arg, void> : public unary_function<_Arg, void> {
  typedef void (*__fun_type)(_Arg);
  __fun_type _M_ptr;
public:
  pointer_to_unary_function() {}
  explicit pointer_to_unary_function(__fun_type __x) : _M_ptr(__x) {}
  void operator()(_Arg __x) const { _M_ptr(__x); }
};

template <class _Arg1, class _Arg2>
class pointer_to_binary_function<_Arg1, _Arg2, void> : public binary_function<_Arg1,_Arg2,void> {
  typedef void (*__fun_type)(_Arg1, _Arg2);
  __fun_type _M_ptr;
public:
  pointer_to_binary_function() {}
  explicit pointer_to_binary_function(__fun_type __x) : _M_ptr(__x) {}
  void operator()(_Arg1 __x, _Arg2 __y) const { _M_ptr(__x, __y); }
};

#  endif

#endif

#if !defined (_STLP_MEMBER_POINTER_PARAM_BUG)





template <class _Result, class _Tp>
inline mem_fun_t<_Result,_Tp>
mem_fun(_Result (_Tp::*__f)()) { return mem_fun_t<_Result,_Tp>(__f); }

template <class _Result, class _Tp>
inline const_mem_fun_t<_Result,_Tp>
mem_fun(_Result (_Tp::*__f)() const)  { return const_mem_fun_t<_Result,_Tp>(__f); }

template <class _Result, class _Tp>
inline mem_fun_ref_t<_Result,_Tp>
mem_fun_ref(_Result (_Tp::*__f)())  { return mem_fun_ref_t<_Result,_Tp>(__f); }

template <class _Result, class _Tp>
inline const_mem_fun_ref_t<_Result,_Tp>
mem_fun_ref(_Result (_Tp::*__f)() const)  { return const_mem_fun_ref_t<_Result,_Tp>(__f); }

template <class _Result, class _Tp, class _Arg>
inline mem_fun1_t<_Result,_Tp,_Arg>
mem_fun(_Result (_Tp::*__f)(_Arg)) { return mem_fun1_t<_Result,_Tp,_Arg>(__f); }

template <class _Result, class _Tp, class _Arg>
inline const_mem_fun1_t<_Result,_Tp,_Arg>
mem_fun(_Result (_Tp::*__f)(_Arg) const) { return const_mem_fun1_t<_Result,_Tp,_Arg>(__f); }

template <class _Result, class _Tp, class _Arg>
inline mem_fun1_ref_t<_Result,_Tp,_Arg>
mem_fun_ref(_Result (_Tp::*__f)(_Arg)) { return mem_fun1_ref_t<_Result,_Tp,_Arg>(__f); }

template <class _Result, class _Tp, class _Arg>
inline const_mem_fun1_ref_t<_Result,_Tp,_Arg>
mem_fun_ref(_Result (_Tp::*__f)(_Arg) const) { return const_mem_fun1_ref_t<_Result,_Tp,_Arg>(__f); }

#  if !(defined (_STLP_NO_EXTENSIONS) || defined (_STLP_NO_ANACHRONISMS))


template <class _Result, class _Tp, class _Arg>
inline mem_fun1_t<_Result,_Tp,_Arg>
mem_fun1(_Result (_Tp::*__f)(_Arg)) { return mem_fun1_t<_Result,_Tp,_Arg>(__f); }

template <class _Result, class _Tp, class _Arg>
inline const_mem_fun1_t<_Result,_Tp,_Arg>
mem_fun1(_Result (_Tp::*__f)(_Arg) const) { return const_mem_fun1_t<_Result,_Tp,_Arg>(__f); }

template <class _Result, class _Tp, class _Arg>
inline mem_fun1_ref_t<_Result,_Tp,_Arg>
mem_fun1_ref(_Result (_Tp::*__f)(_Arg)) { return mem_fun1_ref_t<_Result,_Tp,_Arg>(__f); }

template <class _Result, class _Tp, class _Arg>
inline const_mem_fun1_ref_t<_Result,_Tp,_Arg>
mem_fun1_ref(_Result (_Tp::*__f)(_Arg) const) { return const_mem_fun1_ref_t<_Result,_Tp,_Arg>(__f); }

#  endif

#endif

template <class _Arg, class _Result>
inline pointer_to_unary_function<_Arg, _Result>
ptr_fun(_Result (*__f)(_Arg))
{ return pointer_to_unary_function<_Arg, _Result>(__f); }

template <class _Arg1, class _Arg2, class _Result>
inline pointer_to_binary_function<_Arg1,_Arg2,_Result>
ptr_fun(_Result (*__f)(_Arg1, _Arg2))
{ return pointer_to_binary_function<_Arg1,_Arg2,_Result>(__f); }

_STLP_END_NAMESPACE
