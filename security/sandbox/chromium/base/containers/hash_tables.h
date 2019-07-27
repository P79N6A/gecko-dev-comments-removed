



















#ifndef BASE_CONTAINERS_HASH_TABLES_H_
#define BASE_CONTAINERS_HASH_TABLES_H_

#include <utility>

#include "base/basictypes.h"
#include "base/strings/string16.h"
#include "build/build_config.h"

#if defined(COMPILER_MSVC)
#include <unordered_map>
#include <unordered_set>

#define BASE_HASH_NAMESPACE std

#elif defined(COMPILER_GCC)

#define BASE_HASH_NAMESPACE base_hash




#ifdef __DEPRECATED
#define CHROME_OLD__DEPRECATED __DEPRECATED
#undef __DEPRECATED
#endif

#if defined(OS_ANDROID)
#include <hash_map>
#include <hash_set>
#define BASE_HASH_IMPL_NAMESPACE std
#else
#include <ext/hash_map>
#include <ext/hash_set>
#define BASE_HASH_IMPL_NAMESPACE __gnu_cxx
#endif

#include <string>

#ifdef CHROME_OLD__DEPRECATED
#define __DEPRECATED CHROME_OLD__DEPRECATED
#undef CHROME_OLD__DEPRECATED
#endif

namespace BASE_HASH_NAMESPACE {






template<typename T>
struct hash {
  std::size_t operator()(const T& value) const {
    return BASE_HASH_IMPL_NAMESPACE::hash<T>()(value);
  }
};

template<typename T>
struct hash<T*> {
  std::size_t operator()(T* value) const {
    return BASE_HASH_IMPL_NAMESPACE::hash<uintptr_t>()(
        reinterpret_cast<uintptr_t>(value));
  }
};

#if !defined(OS_ANDROID)





#define DEFINE_TRIVIAL_HASH(integral_type) \
    template<> \
    struct hash<integral_type> { \
      std::size_t operator()(integral_type value) const { \
        return static_cast<std::size_t>(value); \
      } \
    }

DEFINE_TRIVIAL_HASH(long long);
DEFINE_TRIVIAL_HASH(unsigned long long);

#undef DEFINE_TRIVIAL_HASH
#endif  







#define DEFINE_STRING_HASH(string_type) \
    template<> \
    struct hash<string_type> { \
      std::size_t operator()(const string_type& s) const { \
        std::size_t result = 0; \
        for (string_type::const_iterator i = s.begin(); i != s.end(); ++i) \
          result = (result * 131) + *i; \
        return result; \
      } \
    }

DEFINE_STRING_HASH(std::string);
DEFINE_STRING_HASH(base::string16);

#undef DEFINE_STRING_HASH

}  

#else  
#error define BASE_HASH_NAMESPACE for your compiler
#endif  

namespace base {


#if defined(COMPILER_MSVC)

template<class Key, class T,
         class Hash = std::hash<Key>,
         class Pred = std::equal_to<Key>,
         class Alloc = std::allocator<std::pair<const Key, T>>>
using hash_map = std::unordered_map<Key, T, Hash, Pred, Alloc>;

template<class Key, class T,
         class Hash = std::hash<Key>,
         class Pred = std::equal_to<Key>,
         class Alloc = std::allocator<std::pair<const Key, T>>>
using hash_multimap = std::unordered_multimap<Key, T, Hash, Pred, Alloc>;

template<class Key,
         class Hash = std::hash<Key>,
         class Pred = std::equal_to<Key>,
         class Alloc = std::allocator<Key>>
using hash_multiset = std::unordered_multiset<Key, Hash, Pred, Alloc>;

template<class Key,
         class Hash = std::hash<Key>,
         class Pred = std::equal_to<Key>,
         class Alloc = std::allocator<Key>>
using hash_set = std::unordered_set<Key, Hash, Pred, Alloc>;

#else  



template<class Key, class T,
         class Hash = BASE_HASH_NAMESPACE::hash<Key>,
         class Pred = std::equal_to<Key>,
         class Alloc = std::allocator<std::pair<const Key, T>>>
using hash_map = BASE_HASH_IMPL_NAMESPACE::hash_map<Key, T, Hash, Pred, Alloc>;

template<class Key, class T,
         class Hash = BASE_HASH_NAMESPACE::hash<Key>,
         class Pred = std::equal_to<Key>,
         class Alloc = std::allocator<std::pair<const Key, T>>>
using hash_multimap =
    BASE_HASH_IMPL_NAMESPACE::hash_multimap<Key, T, Hash, Pred, Alloc>;

template<class Key,
         class Hash = BASE_HASH_NAMESPACE::hash<Key>,
         class Pred = std::equal_to<Key>,
         class Alloc = std::allocator<Key>>
using hash_multiset =
    BASE_HASH_IMPL_NAMESPACE::hash_multiset<Key, Hash, Pred, Alloc>;

template<class Key,
         class Hash = BASE_HASH_NAMESPACE::hash<Key>,
         class Pred = std::equal_to<Key>,
         class Alloc = std::allocator<Key>>
using hash_set = BASE_HASH_IMPL_NAMESPACE::hash_set<Key, Hash, Pred, Alloc>;

#undef BASE_HASH_IMPL_NAMESPACE

#endif  










inline std::size_t HashInts32(uint32 value1, uint32 value2) {
  uint64 value1_64 = value1;
  uint64 hash64 = (value1_64 << 32) | value2;

  if (sizeof(std::size_t) >= sizeof(uint64))
    return static_cast<std::size_t>(hash64);

  uint64 odd_random = 481046412LL << 32 | 1025306955LL;
  uint32 shift_random = 10121U << 16;

  hash64 = hash64 * odd_random + shift_random;
  std::size_t high_bits = static_cast<std::size_t>(
      hash64 >> (8 * (sizeof(uint64) - sizeof(std::size_t))));
  return high_bits;
}






inline std::size_t HashInts64(uint64 value1, uint64 value2) {
  uint32 short_random1 = 842304669U;
  uint32 short_random2 = 619063811U;
  uint32 short_random3 = 937041849U;
  uint32 short_random4 = 3309708029U;

  uint32 value1a = static_cast<uint32>(value1 & 0xffffffff);
  uint32 value1b = static_cast<uint32>((value1 >> 32) & 0xffffffff);
  uint32 value2a = static_cast<uint32>(value2 & 0xffffffff);
  uint32 value2b = static_cast<uint32>((value2 >> 32) & 0xffffffff);

  uint64 product1 = static_cast<uint64>(value1a) * short_random1;
  uint64 product2 = static_cast<uint64>(value1b) * short_random2;
  uint64 product3 = static_cast<uint64>(value2a) * short_random3;
  uint64 product4 = static_cast<uint64>(value2b) * short_random4;

  uint64 hash64 = product1 + product2 + product3 + product4;

  if (sizeof(std::size_t) >= sizeof(uint64))
    return static_cast<std::size_t>(hash64);

  uint64 odd_random = 1578233944LL << 32 | 194370989LL;
  uint32 shift_random = 20591U << 16;

  hash64 = hash64 * odd_random + shift_random;
  std::size_t high_bits = static_cast<std::size_t>(
      hash64 >> (8 * (sizeof(uint64) - sizeof(std::size_t))));
  return high_bits;
}

#define DEFINE_32BIT_PAIR_HASH(Type1, Type2) \
inline std::size_t HashPair(Type1 value1, Type2 value2) { \
  return HashInts32(value1, value2); \
}

DEFINE_32BIT_PAIR_HASH(int16, int16);
DEFINE_32BIT_PAIR_HASH(int16, uint16);
DEFINE_32BIT_PAIR_HASH(int16, int32);
DEFINE_32BIT_PAIR_HASH(int16, uint32);
DEFINE_32BIT_PAIR_HASH(uint16, int16);
DEFINE_32BIT_PAIR_HASH(uint16, uint16);
DEFINE_32BIT_PAIR_HASH(uint16, int32);
DEFINE_32BIT_PAIR_HASH(uint16, uint32);
DEFINE_32BIT_PAIR_HASH(int32, int16);
DEFINE_32BIT_PAIR_HASH(int32, uint16);
DEFINE_32BIT_PAIR_HASH(int32, int32);
DEFINE_32BIT_PAIR_HASH(int32, uint32);
DEFINE_32BIT_PAIR_HASH(uint32, int16);
DEFINE_32BIT_PAIR_HASH(uint32, uint16);
DEFINE_32BIT_PAIR_HASH(uint32, int32);
DEFINE_32BIT_PAIR_HASH(uint32, uint32);

#undef DEFINE_32BIT_PAIR_HASH

#define DEFINE_64BIT_PAIR_HASH(Type1, Type2) \
inline std::size_t HashPair(Type1 value1, Type2 value2) { \
  return HashInts64(value1, value2); \
}

DEFINE_64BIT_PAIR_HASH(int16, int64);
DEFINE_64BIT_PAIR_HASH(int16, uint64);
DEFINE_64BIT_PAIR_HASH(uint16, int64);
DEFINE_64BIT_PAIR_HASH(uint16, uint64);
DEFINE_64BIT_PAIR_HASH(int32, int64);
DEFINE_64BIT_PAIR_HASH(int32, uint64);
DEFINE_64BIT_PAIR_HASH(uint32, int64);
DEFINE_64BIT_PAIR_HASH(uint32, uint64);
DEFINE_64BIT_PAIR_HASH(int64, int16);
DEFINE_64BIT_PAIR_HASH(int64, uint16);
DEFINE_64BIT_PAIR_HASH(int64, int32);
DEFINE_64BIT_PAIR_HASH(int64, uint32);
DEFINE_64BIT_PAIR_HASH(int64, int64);
DEFINE_64BIT_PAIR_HASH(int64, uint64);
DEFINE_64BIT_PAIR_HASH(uint64, int16);
DEFINE_64BIT_PAIR_HASH(uint64, uint16);
DEFINE_64BIT_PAIR_HASH(uint64, int32);
DEFINE_64BIT_PAIR_HASH(uint64, uint32);
DEFINE_64BIT_PAIR_HASH(uint64, int64);
DEFINE_64BIT_PAIR_HASH(uint64, uint64);

#undef DEFINE_64BIT_PAIR_HASH
}  

namespace BASE_HASH_NAMESPACE {




template<typename Type1, typename Type2>
struct hash<std::pair<Type1, Type2> > {
  std::size_t operator()(std::pair<Type1, Type2> value) const {
    return base::HashPair(value.first, value.second);
  }
};

}

#undef DEFINE_PAIR_HASH_FUNCTION_START
#undef DEFINE_PAIR_HASH_FUNCTION_END

#endif  
