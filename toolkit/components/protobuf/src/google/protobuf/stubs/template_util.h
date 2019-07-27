
















































#ifndef GOOGLE_PROTOBUF_TEMPLATE_UTIL_H_
#define GOOGLE_PROTOBUF_TEMPLATE_UTIL_H_

namespace google {
namespace protobuf {
namespace internal {



typedef char small_;

struct big_ {
  char dummy[2];
};


template <class T>
struct identity_ {
  typedef T type;
};






template<class T, T v>
struct integral_constant {
  static const T value = v;
  typedef T value_type;
  typedef integral_constant<T, v> type;
};

template <class T, T v> const T integral_constant<T, v>::value;





typedef integral_constant<bool, true>  true_type;
typedef integral_constant<bool, false> false_type;
typedef true_type  true_;
typedef false_type false_;




template<bool cond, typename A, typename B>
struct if_{
  typedef A type;
};

template<typename A, typename B>
struct if_<false, A, B> {
  typedef B type;
};







template<typename A, typename B>
struct type_equals_ : public false_ {
};

template<typename A>
struct type_equals_<A, A> : public true_ {
};



template<typename A, typename B>
struct and_ : public integral_constant<bool, (A::value && B::value)> {
};



template<typename A, typename B>
struct or_ : public integral_constant<bool, (A::value || B::value)> {
};


}  
}  
}  

#endif  
