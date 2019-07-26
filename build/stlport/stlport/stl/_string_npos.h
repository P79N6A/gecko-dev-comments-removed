


















#if defined (_STLP_STATIC_CONST_INIT_BUG)
  enum { npos = -1 };
#elif defined (__GNUC__) && (__GNUC__ == 2) && (__GNUC_MINOR__ == 96)
  
  static const size_t npos;
#else
  static const size_t npos = ~(size_t)0;
#endif
