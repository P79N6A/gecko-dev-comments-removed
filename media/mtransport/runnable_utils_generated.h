





template<typename C, typename M> class runnable_args0 : public runnable_args_base {
 public:
  runnable_args0(C o, M m) :
    o_(o), m_(m)  {}

  NS_IMETHOD Run() {
    ((*o_).*m_)();
    return NS_OK;
  }

 private:
  C o_;
  M m_;
};




template<typename C, typename M, typename R> class runnable_args0_ret : public runnable_args_base {
 public:
  runnable_args0_ret(C o, M m, R *r) :
    o_(o), m_(m), r_(r)  {}

  NS_IMETHOD Run() {
    *r_ = ((*o_).*m_)();
    return NS_OK;
  }

 private:
  C o_;
  M m_;
  R* r_;
};




template<typename C, typename M, typename A0> class runnable_args1 : public runnable_args_base {
 public:
  runnable_args1(C o, M m, A0 a0) :
    o_(o), m_(m), a0_(a0)  {}

  NS_IMETHOD Run() {
    ((*o_).*m_)(a0_);
    return NS_OK;
  }

 private:
  C o_;
  M m_;
  A0 a0_;
};




template<typename C, typename M, typename A0, typename R> class runnable_args1_ret : public runnable_args_base {
 public:
  runnable_args1_ret(C o, M m, A0 a0, R *r) :
    o_(o), m_(m), r_(r), a0_(a0)  {}

  NS_IMETHOD Run() {
    *r_ = ((*o_).*m_)(a0_);
    return NS_OK;
  }

 private:
  C o_;
  M m_;
  R* r_;
  A0 a0_;
};




template<typename C, typename M, typename A0, typename A1> class runnable_args2 : public runnable_args_base {
 public:
  runnable_args2(C o, M m, A0 a0, A1 a1) :
    o_(o), m_(m), a0_(a0), a1_(a1)  {}

  NS_IMETHOD Run() {
    ((*o_).*m_)(a0_, a1_);
    return NS_OK;
  }

 private:
  C o_;
  M m_;
  A0 a0_;
  A1 a1_;
};




template<typename C, typename M, typename A0, typename A1, typename R> class runnable_args2_ret : public runnable_args_base {
 public:
  runnable_args2_ret(C o, M m, A0 a0, A1 a1, R *r) :
    o_(o), m_(m), r_(r), a0_(a0), a1_(a1)  {}

  NS_IMETHOD Run() {
    *r_ = ((*o_).*m_)(a0_, a1_);
    return NS_OK;
  }

 private:
  C o_;
  M m_;
  R* r_;
  A0 a0_;
  A1 a1_;
};




template<typename C, typename M, typename A0, typename A1, typename A2> class runnable_args3 : public runnable_args_base {
 public:
  runnable_args3(C o, M m, A0 a0, A1 a1, A2 a2) :
    o_(o), m_(m), a0_(a0), a1_(a1), a2_(a2)  {}

  NS_IMETHOD Run() {
    ((*o_).*m_)(a0_, a1_, a2_);
    return NS_OK;
  }

 private:
  C o_;
  M m_;
  A0 a0_;
  A1 a1_;
  A2 a2_;
};




template<typename C, typename M, typename A0, typename A1, typename A2, typename R> class runnable_args3_ret : public runnable_args_base {
 public:
  runnable_args3_ret(C o, M m, A0 a0, A1 a1, A2 a2, R *r) :
    o_(o), m_(m), r_(r), a0_(a0), a1_(a1), a2_(a2)  {}

  NS_IMETHOD Run() {
    *r_ = ((*o_).*m_)(a0_, a1_, a2_);
    return NS_OK;
  }

 private:
  C o_;
  M m_;
  R* r_;
  A0 a0_;
  A1 a1_;
  A2 a2_;
};




template<typename C, typename M, typename A0, typename A1, typename A2, typename A3> class runnable_args4 : public runnable_args_base {
 public:
  runnable_args4(C o, M m, A0 a0, A1 a1, A2 a2, A3 a3) :
    o_(o), m_(m), a0_(a0), a1_(a1), a2_(a2), a3_(a3)  {}

  NS_IMETHOD Run() {
    ((*o_).*m_)(a0_, a1_, a2_, a3_);
    return NS_OK;
  }

 private:
  C o_;
  M m_;
  A0 a0_;
  A1 a1_;
  A2 a2_;
  A3 a3_;
};




template<typename C, typename M, typename A0, typename A1, typename A2, typename A3, typename R> class runnable_args4_ret : public runnable_args_base {
 public:
  runnable_args4_ret(C o, M m, A0 a0, A1 a1, A2 a2, A3 a3, R *r) :
    o_(o), m_(m), r_(r), a0_(a0), a1_(a1), a2_(a2), a3_(a3)  {}

  NS_IMETHOD Run() {
    *r_ = ((*o_).*m_)(a0_, a1_, a2_, a3_);
    return NS_OK;
  }

 private:
  C o_;
  M m_;
  R* r_;
  A0 a0_;
  A1 a1_;
  A2 a2_;
  A3 a3_;
};




template<typename C, typename M, typename A0, typename A1, typename A2, typename A3, typename A4> class runnable_args5 : public runnable_args_base {
 public:
  runnable_args5(C o, M m, A0 a0, A1 a1, A2 a2, A3 a3, A4 a4) :
    o_(o), m_(m), a0_(a0), a1_(a1), a2_(a2), a3_(a3), a4_(a4)  {}

  NS_IMETHOD Run() {
    ((*o_).*m_)(a0_, a1_, a2_, a3_, a4_);
    return NS_OK;
  }

 private:
  C o_;
  M m_;
  A0 a0_;
  A1 a1_;
  A2 a2_;
  A3 a3_;
  A4 a4_;
};




template<typename C, typename M, typename A0, typename A1, typename A2, typename A3, typename A4, typename R> class runnable_args5_ret : public runnable_args_base {
 public:
  runnable_args5_ret(C o, M m, A0 a0, A1 a1, A2 a2, A3 a3, A4 a4, R *r) :
    o_(o), m_(m), r_(r), a0_(a0), a1_(a1), a2_(a2), a3_(a3), a4_(a4)  {}

  NS_IMETHOD Run() {
    *r_ = ((*o_).*m_)(a0_, a1_, a2_, a3_, a4_);
    return NS_OK;
  }

 private:
  C o_;
  M m_;
  R* r_;
  A0 a0_;
  A1 a1_;
  A2 a2_;
  A3 a3_;
  A4 a4_;
};




template<typename C, typename M, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5> class runnable_args6 : public runnable_args_base {
 public:
  runnable_args6(C o, M m, A0 a0, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5) :
    o_(o), m_(m), a0_(a0), a1_(a1), a2_(a2), a3_(a3), a4_(a4), a5_(a5)  {}

  NS_IMETHOD Run() {
    ((*o_).*m_)(a0_, a1_, a2_, a3_, a4_, a5_);
    return NS_OK;
  }

 private:
  C o_;
  M m_;
  A0 a0_;
  A1 a1_;
  A2 a2_;
  A3 a3_;
  A4 a4_;
  A5 a5_;
};




template<typename C, typename M, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename R> class runnable_args6_ret : public runnable_args_base {
 public:
  runnable_args6_ret(C o, M m, A0 a0, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, R *r) :
    o_(o), m_(m), r_(r), a0_(a0), a1_(a1), a2_(a2), a3_(a3), a4_(a4), a5_(a5)  {}

  NS_IMETHOD Run() {
    *r_ = ((*o_).*m_)(a0_, a1_, a2_, a3_, a4_, a5_);
    return NS_OK;
  }

 private:
  C o_;
  M m_;
  R* r_;
  A0 a0_;
  A1 a1_;
  A2 a2_;
  A3 a3_;
  A4 a4_;
  A5 a5_;
};




template<typename C, typename M, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6> class runnable_args7 : public runnable_args_base {
 public:
  runnable_args7(C o, M m, A0 a0, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6) :
    o_(o), m_(m), a0_(a0), a1_(a1), a2_(a2), a3_(a3), a4_(a4), a5_(a5), a6_(a6)  {}

  NS_IMETHOD Run() {
    ((*o_).*m_)(a0_, a1_, a2_, a3_, a4_, a5_, a6_);
    return NS_OK;
  }

 private:
  C o_;
  M m_;
  A0 a0_;
  A1 a1_;
  A2 a2_;
  A3 a3_;
  A4 a4_;
  A5 a5_;
  A6 a6_;
};




template<typename C, typename M, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename R> class runnable_args7_ret : public runnable_args_base {
 public:
  runnable_args7_ret(C o, M m, A0 a0, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, R *r) :
    o_(o), m_(m), r_(r), a0_(a0), a1_(a1), a2_(a2), a3_(a3), a4_(a4), a5_(a5), a6_(a6)  {}

  NS_IMETHOD Run() {
    *r_ = ((*o_).*m_)(a0_, a1_, a2_, a3_, a4_, a5_, a6_);
    return NS_OK;
  }

 private:
  C o_;
  M m_;
  R* r_;
  A0 a0_;
  A1 a1_;
  A2 a2_;
  A3 a3_;
  A4 a4_;
  A5 a5_;
  A6 a6_;
};




template<typename C, typename M, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7> class runnable_args8 : public runnable_args_base {
 public:
  runnable_args8(C o, M m, A0 a0, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7) :
    o_(o), m_(m), a0_(a0), a1_(a1), a2_(a2), a3_(a3), a4_(a4), a5_(a5), a6_(a6), a7_(a7)  {}

  NS_IMETHOD Run() {
    ((*o_).*m_)(a0_, a1_, a2_, a3_, a4_, a5_, a6_, a7_);
    return NS_OK;
  }

 private:
  C o_;
  M m_;
  A0 a0_;
  A1 a1_;
  A2 a2_;
  A3 a3_;
  A4 a4_;
  A5 a5_;
  A6 a6_;
  A7 a7_;
};




template<typename C, typename M, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename R> class runnable_args8_ret : public runnable_args_base {
 public:
  runnable_args8_ret(C o, M m, A0 a0, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, R *r) :
    o_(o), m_(m), r_(r), a0_(a0), a1_(a1), a2_(a2), a3_(a3), a4_(a4), a5_(a5), a6_(a6), a7_(a7)  {}

  NS_IMETHOD Run() {
    *r_ = ((*o_).*m_)(a0_, a1_, a2_, a3_, a4_, a5_, a6_, a7_);
    return NS_OK;
  }

 private:
  C o_;
  M m_;
  R* r_;
  A0 a0_;
  A1 a1_;
  A2 a2_;
  A3 a3_;
  A4 a4_;
  A5 a5_;
  A6 a6_;
  A7 a7_;
};




template<typename C, typename M, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8> class runnable_args9 : public runnable_args_base {
 public:
  runnable_args9(C o, M m, A0 a0, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8) :
    o_(o), m_(m), a0_(a0), a1_(a1), a2_(a2), a3_(a3), a4_(a4), a5_(a5), a6_(a6), a7_(a7), a8_(a8)  {}

  NS_IMETHOD Run() {
    ((*o_).*m_)(a0_, a1_, a2_, a3_, a4_, a5_, a6_, a7_, a8_);
    return NS_OK;
  }

 private:
  C o_;
  M m_;
  A0 a0_;
  A1 a1_;
  A2 a2_;
  A3 a3_;
  A4 a4_;
  A5 a5_;
  A6 a6_;
  A7 a7_;
  A8 a8_;
};




template<typename C, typename M, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename R> class runnable_args9_ret : public runnable_args_base {
 public:
  runnable_args9_ret(C o, M m, A0 a0, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, R *r) :
    o_(o), m_(m), r_(r), a0_(a0), a1_(a1), a2_(a2), a3_(a3), a4_(a4), a5_(a5), a6_(a6), a7_(a7), a8_(a8)  {}

  NS_IMETHOD Run() {
    *r_ = ((*o_).*m_)(a0_, a1_, a2_, a3_, a4_, a5_, a6_, a7_, a8_);
    return NS_OK;
  }

 private:
  C o_;
  M m_;
  R* r_;
  A0 a0_;
  A1 a1_;
  A2 a2_;
  A3 a3_;
  A4 a4_;
  A5 a5_;
  A6 a6_;
  A7 a7_;
  A8 a8_;
};







template<typename C, typename M>
runnable_args0<C, M>* WrapRunnable(C o, M m) {
  return new runnable_args0<C, M>
    (o, m);
}


template<typename C, typename M, typename R>
runnable_args0_ret<C, M, R>* WrapRunnableRet(C o, M m, R* r) {
  return new runnable_args0_ret<C, M, R>
    (o, m, r);
}


template<typename C, typename M, typename A0>
runnable_args1<C, M, A0>* WrapRunnable(C o, M m, A0 a0) {
  return new runnable_args1<C, M, A0>
    (o, m, a0);
}


template<typename C, typename M, typename A0, typename R>
runnable_args1_ret<C, M, A0, R>* WrapRunnableRet(C o, M m, A0 a0, R* r) {
  return new runnable_args1_ret<C, M, A0, R>
    (o, m, a0, r);
}


template<typename C, typename M, typename A0, typename A1>
runnable_args2<C, M, A0, A1>* WrapRunnable(C o, M m, A0 a0, A1 a1) {
  return new runnable_args2<C, M, A0, A1>
    (o, m, a0, a1);
}


template<typename C, typename M, typename A0, typename A1, typename R>
runnable_args2_ret<C, M, A0, A1, R>* WrapRunnableRet(C o, M m, A0 a0, A1 a1, R* r) {
  return new runnable_args2_ret<C, M, A0, A1, R>
    (o, m, a0, a1, r);
}


template<typename C, typename M, typename A0, typename A1, typename A2>
runnable_args3<C, M, A0, A1, A2>* WrapRunnable(C o, M m, A0 a0, A1 a1, A2 a2) {
  return new runnable_args3<C, M, A0, A1, A2>
    (o, m, a0, a1, a2);
}


template<typename C, typename M, typename A0, typename A1, typename A2, typename R>
runnable_args3_ret<C, M, A0, A1, A2, R>* WrapRunnableRet(C o, M m, A0 a0, A1 a1, A2 a2, R* r) {
  return new runnable_args3_ret<C, M, A0, A1, A2, R>
    (o, m, a0, a1, a2, r);
}


template<typename C, typename M, typename A0, typename A1, typename A2, typename A3>
runnable_args4<C, M, A0, A1, A2, A3>* WrapRunnable(C o, M m, A0 a0, A1 a1, A2 a2, A3 a3) {
  return new runnable_args4<C, M, A0, A1, A2, A3>
    (o, m, a0, a1, a2, a3);
}


template<typename C, typename M, typename A0, typename A1, typename A2, typename A3, typename R>
runnable_args4_ret<C, M, A0, A1, A2, A3, R>* WrapRunnableRet(C o, M m, A0 a0, A1 a1, A2 a2, A3 a3, R* r) {
  return new runnable_args4_ret<C, M, A0, A1, A2, A3, R>
    (o, m, a0, a1, a2, a3, r);
}


template<typename C, typename M, typename A0, typename A1, typename A2, typename A3, typename A4>
runnable_args5<C, M, A0, A1, A2, A3, A4>* WrapRunnable(C o, M m, A0 a0, A1 a1, A2 a2, A3 a3, A4 a4) {
  return new runnable_args5<C, M, A0, A1, A2, A3, A4>
    (o, m, a0, a1, a2, a3, a4);
}


template<typename C, typename M, typename A0, typename A1, typename A2, typename A3, typename A4, typename R>
runnable_args5_ret<C, M, A0, A1, A2, A3, A4, R>* WrapRunnableRet(C o, M m, A0 a0, A1 a1, A2 a2, A3 a3, A4 a4, R* r) {
  return new runnable_args5_ret<C, M, A0, A1, A2, A3, A4, R>
    (o, m, a0, a1, a2, a3, a4, r);
}


template<typename C, typename M, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5>
runnable_args6<C, M, A0, A1, A2, A3, A4, A5>* WrapRunnable(C o, M m, A0 a0, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5) {
  return new runnable_args6<C, M, A0, A1, A2, A3, A4, A5>
    (o, m, a0, a1, a2, a3, a4, a5);
}


template<typename C, typename M, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename R>
runnable_args6_ret<C, M, A0, A1, A2, A3, A4, A5, R>* WrapRunnableRet(C o, M m, A0 a0, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, R* r) {
  return new runnable_args6_ret<C, M, A0, A1, A2, A3, A4, A5, R>
    (o, m, a0, a1, a2, a3, a4, a5, r);
}


template<typename C, typename M, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
runnable_args7<C, M, A0, A1, A2, A3, A4, A5, A6>* WrapRunnable(C o, M m, A0 a0, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6) {
  return new runnable_args7<C, M, A0, A1, A2, A3, A4, A5, A6>
    (o, m, a0, a1, a2, a3, a4, a5, a6);
}


template<typename C, typename M, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename R>
runnable_args7_ret<C, M, A0, A1, A2, A3, A4, A5, A6, R>* WrapRunnableRet(C o, M m, A0 a0, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, R* r) {
  return new runnable_args7_ret<C, M, A0, A1, A2, A3, A4, A5, A6, R>
    (o, m, a0, a1, a2, a3, a4, a5, a6, r);
}


template<typename C, typename M, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7>
runnable_args8<C, M, A0, A1, A2, A3, A4, A5, A6, A7>* WrapRunnable(C o, M m, A0 a0, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7) {
  return new runnable_args8<C, M, A0, A1, A2, A3, A4, A5, A6, A7>
    (o, m, a0, a1, a2, a3, a4, a5, a6, a7);
}


template<typename C, typename M, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename R>
runnable_args8_ret<C, M, A0, A1, A2, A3, A4, A5, A6, A7, R>* WrapRunnableRet(C o, M m, A0 a0, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, R* r) {
  return new runnable_args8_ret<C, M, A0, A1, A2, A3, A4, A5, A6, A7, R>
    (o, m, a0, a1, a2, a3, a4, a5, a6, a7, r);
}


template<typename C, typename M, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8>
runnable_args9<C, M, A0, A1, A2, A3, A4, A5, A6, A7, A8>* WrapRunnable(C o, M m, A0 a0, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8) {
  return new runnable_args9<C, M, A0, A1, A2, A3, A4, A5, A6, A7, A8>
    (o, m, a0, a1, a2, a3, a4, a5, a6, a7, a8);
}


template<typename C, typename M, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename R>
runnable_args9_ret<C, M, A0, A1, A2, A3, A4, A5, A6, A7, A8, R>* WrapRunnableRet(C o, M m, A0 a0, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, R* r) {
  return new runnable_args9_ret<C, M, A0, A1, A2, A3, A4, A5, A6, A7, A8, R>
    (o, m, a0, a1, a2, a3, a4, a5, a6, a7, a8, r);
}

