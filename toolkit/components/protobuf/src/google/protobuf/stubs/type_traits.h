
























































#ifndef GOOGLE_PROTOBUF_TYPE_TRAITS_H_
#define GOOGLE_PROTOBUF_TYPE_TRAITS_H_

#include <utility>                  

#include <google/protobuf/stubs/template_util.h>  

namespace google {
namespace protobuf {
namespace internal {

template <class T> struct is_integral;
template <class T> struct is_floating_point;
template <class T> struct is_pointer;

#if !defined(_MSC_VER) && !(defined(__GNUC__) && __GNUC__ <= 3)

template <class T> struct is_enum;
#endif
template <class T> struct is_reference;
template <class T> struct is_pod;
template <class T> struct has_trivial_constructor;
template <class T> struct has_trivial_copy;
template <class T> struct has_trivial_assign;
template <class T> struct has_trivial_destructor;
template <class T> struct remove_const;
template <class T> struct remove_volatile;
template <class T> struct remove_cv;
template <class T> struct remove_reference;
template <class T> struct add_reference;
template <class T> struct remove_pointer;
template <class T, class U> struct is_same;
#if !defined(_MSC_VER) && !(defined(__GNUC__) && __GNUC__ <= 3)
template <class From, class To> struct is_convertible;
#endif



template <class T> struct is_integral : false_type { };
template<> struct is_integral<bool> : true_type { };
template<> struct is_integral<char> : true_type { };
template<> struct is_integral<unsigned char> : true_type { };
template<> struct is_integral<signed char> : true_type { };
#if defined(_MSC_VER)



template<> struct is_integral<__wchar_t> : true_type { };
#else
template<> struct is_integral<wchar_t> : true_type { };
#endif
template<> struct is_integral<short> : true_type { };
template<> struct is_integral<unsigned short> : true_type { };
template<> struct is_integral<int> : true_type { };
template<> struct is_integral<unsigned int> : true_type { };
template<> struct is_integral<long> : true_type { };
template<> struct is_integral<unsigned long> : true_type { };
template<> struct is_integral<long long> : true_type { };
template<> struct is_integral<unsigned long long> : true_type { };
template <class T> struct is_integral<const T> : is_integral<T> { };
template <class T> struct is_integral<volatile T> : is_integral<T> { };
template <class T> struct is_integral<const volatile T> : is_integral<T> { };



template <class T> struct is_floating_point : false_type { };
template<> struct is_floating_point<float> : true_type { };
template<> struct is_floating_point<double> : true_type { };
template<> struct is_floating_point<long double> : true_type { };
template <class T> struct is_floating_point<const T>
    : is_floating_point<T> { };
template <class T> struct is_floating_point<volatile T>
    : is_floating_point<T> { };
template <class T> struct is_floating_point<const volatile T>
    : is_floating_point<T> { };




template <class T> struct is_pointer : false_type { };
template <class T> struct is_pointer<T*> : true_type { };
template <class T> struct is_pointer<const T> : is_pointer<T> { };
template <class T> struct is_pointer<volatile T> : is_pointer<T> { };
template <class T> struct is_pointer<const volatile T> : is_pointer<T> { };

#if !defined(_MSC_VER) && !(defined(__GNUC__) && __GNUC__ <= 3)

namespace internal {

template <class T> struct is_class_or_union {
  template <class U> static small_ tester(void (U::*)());
  template <class U> static big_ tester(...);
  static const bool value = sizeof(tester<T>(0)) == sizeof(small_);
};



template <bool NotUnum, class T> struct is_enum_impl
    : is_convertible<typename add_reference<T>::type, int> { };

template <class T> struct is_enum_impl<true, T> : false_type { };

}  
















template <class T> struct is_enum
    : internal::is_enum_impl<
          is_same<T, void>::value ||
              is_integral<T>::value ||
              is_floating_point<T>::value ||
              is_reference<T>::value ||
              internal::is_class_or_union<T>::value,
          T> { };

template <class T> struct is_enum<const T> : is_enum<T> { };
template <class T> struct is_enum<volatile T> : is_enum<T> { };
template <class T> struct is_enum<const volatile T> : is_enum<T> { };

#endif


template<typename T> struct is_reference : false_type {};
template<typename T> struct is_reference<T&> : true_type {};






template <class T> struct is_pod
 : integral_constant<bool, (is_integral<T>::value ||
                            is_floating_point<T>::value ||
#if !defined(_MSC_VER) && !(defined(__GNUC__) && __GNUC__ <= 3)
                            
                            is_enum<T>::value ||
#endif
                            is_pointer<T>::value)> { };
template <class T> struct is_pod<const T> : is_pod<T> { };
template <class T> struct is_pod<volatile T> : is_pod<T> { };
template <class T> struct is_pod<const volatile T> : is_pod<T> { };







template <class T> struct has_trivial_constructor : is_pod<T> { };
template <class T, class U> struct has_trivial_constructor<std::pair<T, U> >
  : integral_constant<bool,
                      (has_trivial_constructor<T>::value &&
                       has_trivial_constructor<U>::value)> { };
template <class A, int N> struct has_trivial_constructor<A[N]>
  : has_trivial_constructor<A> { };
template <class T> struct has_trivial_constructor<const T>
  : has_trivial_constructor<T> { };






template <class T> struct has_trivial_copy : is_pod<T> { };
template <class T, class U> struct has_trivial_copy<std::pair<T, U> >
  : integral_constant<bool,
                      (has_trivial_copy<T>::value &&
                       has_trivial_copy<U>::value)> { };
template <class A, int N> struct has_trivial_copy<A[N]>
  : has_trivial_copy<A> { };
template <class T> struct has_trivial_copy<const T> : has_trivial_copy<T> { };





template <class T> struct has_trivial_assign : is_pod<T> { };
template <class T, class U> struct has_trivial_assign<std::pair<T, U> >
  : integral_constant<bool,
                      (has_trivial_assign<T>::value &&
                       has_trivial_assign<U>::value)> { };
template <class A, int N> struct has_trivial_assign<A[N]>
  : has_trivial_assign<A> { };






template <class T> struct has_trivial_destructor : is_pod<T> { };
template <class T, class U> struct has_trivial_destructor<std::pair<T, U> >
  : integral_constant<bool,
                      (has_trivial_destructor<T>::value &&
                       has_trivial_destructor<U>::value)> { };
template <class A, int N> struct has_trivial_destructor<A[N]>
  : has_trivial_destructor<A> { };
template <class T> struct has_trivial_destructor<const T>
  : has_trivial_destructor<T> { };


template<typename T> struct remove_const { typedef T type; };
template<typename T> struct remove_const<T const> { typedef T type; };
template<typename T> struct remove_volatile { typedef T type; };
template<typename T> struct remove_volatile<T volatile> { typedef T type; };
template<typename T> struct remove_cv {
  typedef typename remove_const<typename remove_volatile<T>::type>::type type;
};



template<typename T> struct remove_reference { typedef T type; };
template<typename T> struct remove_reference<T&> { typedef T type; };

template <typename T> struct add_reference { typedef T& type; };
template <typename T> struct add_reference<T&> { typedef T& type; };


template<typename T> struct remove_pointer { typedef T type; };
template<typename T> struct remove_pointer<T*> { typedef T type; };
template<typename T> struct remove_pointer<T* const> { typedef T type; };
template<typename T> struct remove_pointer<T* volatile> { typedef T type; };
template<typename T> struct remove_pointer<T* const volatile> {
  typedef T type; };


template<typename T, typename U> struct is_same : public false_type { };
template<typename T> struct is_same<T, T> : public true_type { };


#if !defined(_MSC_VER) && !(defined(__GNUC__) && __GNUC__ <= 3)
namespace internal {










template <typename From, typename To>
struct ConvertHelper {
  static small_ Test(To);
  static big_ Test(...);
  static From Create();
};
}  


template <typename From, typename To>
struct is_convertible
    : integral_constant<bool,
                        sizeof(internal::ConvertHelper<From, To>::Test(
                                  internal::ConvertHelper<From, To>::Create()))
                        == sizeof(small_)> {
};
#endif

}  
}  
}  

#endif  
