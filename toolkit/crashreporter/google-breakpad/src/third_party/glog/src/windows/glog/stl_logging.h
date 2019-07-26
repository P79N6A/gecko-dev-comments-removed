

















































#ifndef UTIL_GTL_STL_LOGGING_INL_H_
#define UTIL_GTL_STL_LOGGING_INL_H_

#if !1
# error We do not support stl_logging for this compiler
#endif

#include <deque>
#include <list>
#include <map>
#include <ostream>
#include <set>
#include <utility>
#include <vector>

#ifdef __GNUC__
# include <ext/hash_set>
# include <ext/hash_map>
# include <ext/slist>
#endif

template<class First, class Second>
inline std::ostream& operator<<(std::ostream& out,
                                const std::pair<First, Second>& p) {
  out << '(' << p.first << ", " << p.second << ')';
  return out;
}

namespace google {

template<class Iter>
inline void PrintSequence(std::ostream& out, Iter begin, Iter end) {
  using ::operator<<;
  
  for (int i = 0; begin != end && i < 100; ++i, ++begin) {
    if (i > 0) out << ' ';
    out << *begin;
  }
  if (begin != end) {
    out << " ...";
  }
}

}

#define OUTPUT_TWO_ARG_CONTAINER(Sequence) \
template<class T1, class T2> \
inline std::ostream& operator<<(std::ostream& out, \
                                const Sequence<T1, T2>& seq) { \
  google::PrintSequence(out, seq.begin(), seq.end()); \
  return out; \
}

OUTPUT_TWO_ARG_CONTAINER(std::vector)
OUTPUT_TWO_ARG_CONTAINER(std::deque)
OUTPUT_TWO_ARG_CONTAINER(std::list)
#ifdef __GNUC__
OUTPUT_TWO_ARG_CONTAINER(__gnu_cxx::slist)
#endif

#undef OUTPUT_TWO_ARG_CONTAINER

#define OUTPUT_THREE_ARG_CONTAINER(Sequence) \
template<class T1, class T2, class T3> \
inline std::ostream& operator<<(std::ostream& out, \
                                const Sequence<T1, T2, T3>& seq) { \
  google::PrintSequence(out, seq.begin(), seq.end()); \
  return out; \
}

OUTPUT_THREE_ARG_CONTAINER(std::set)
OUTPUT_THREE_ARG_CONTAINER(std::multiset)

#undef OUTPUT_THREE_ARG_CONTAINER

#define OUTPUT_FOUR_ARG_CONTAINER(Sequence) \
template<class T1, class T2, class T3, class T4> \
inline std::ostream& operator<<(std::ostream& out, \
                                const Sequence<T1, T2, T3, T4>& seq) { \
  google::PrintSequence(out, seq.begin(), seq.end()); \
  return out; \
}

OUTPUT_FOUR_ARG_CONTAINER(std::map)
OUTPUT_FOUR_ARG_CONTAINER(std::multimap)
#ifdef __GNUC__
OUTPUT_FOUR_ARG_CONTAINER(__gnu_cxx::hash_set)
OUTPUT_FOUR_ARG_CONTAINER(__gnu_cxx::hash_multiset)
#endif

#undef OUTPUT_FOUR_ARG_CONTAINER

#define OUTPUT_FIVE_ARG_CONTAINER(Sequence) \
template<class T1, class T2, class T3, class T4, class T5> \
inline std::ostream& operator<<(std::ostream& out, \
                                const Sequence<T1, T2, T3, T4, T5>& seq) { \
  google::PrintSequence(out, seq.begin(), seq.end()); \
  return out; \
}

#ifdef __GNUC__
OUTPUT_FIVE_ARG_CONTAINER(__gnu_cxx::hash_map)
OUTPUT_FIVE_ARG_CONTAINER(__gnu_cxx::hash_multimap)
#endif

#undef OUTPUT_FIVE_ARG_CONTAINER

#endif  
