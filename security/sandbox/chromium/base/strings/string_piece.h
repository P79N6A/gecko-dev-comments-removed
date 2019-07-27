


























#ifndef BASE_STRINGS_STRING_PIECE_H_
#define BASE_STRINGS_STRING_PIECE_H_

#include <stddef.h>

#include <iosfwd>
#include <string>

#include "base/base_export.h"
#include "base/basictypes.h"
#include "base/containers/hash_tables.h"
#include "base/strings/string16.h"
#include "mozilla/Attributes.h"

namespace base {

template <typename STRING_TYPE> class BasicStringPiece;
typedef BasicStringPiece<std::string> StringPiece;
typedef BasicStringPiece<string16> StringPiece16;










namespace internal {

BASE_EXPORT void CopyToString(const StringPiece& self, std::string* target);
BASE_EXPORT void CopyToString(const StringPiece16& self, string16* target);

BASE_EXPORT void AppendToString(const StringPiece& self, std::string* target);
BASE_EXPORT void AppendToString(const StringPiece16& self, string16* target);

BASE_EXPORT size_t copy(const StringPiece& self,
                        char* buf,
                        size_t n,
                        size_t pos);
BASE_EXPORT size_t copy(const StringPiece16& self,
                        char16* buf,
                        size_t n,
                        size_t pos);

BASE_EXPORT size_t find(const StringPiece& self,
                        const StringPiece& s,
                        size_t pos);
BASE_EXPORT size_t find(const StringPiece16& self,
                        const StringPiece16& s,
                        size_t pos);
BASE_EXPORT size_t find(const StringPiece& self,
                        char c,
                        size_t pos);
BASE_EXPORT size_t find(const StringPiece16& self,
                        char16 c,
                        size_t pos);

BASE_EXPORT size_t rfind(const StringPiece& self,
                         const StringPiece& s,
                         size_t pos);
BASE_EXPORT size_t rfind(const StringPiece16& self,
                         const StringPiece16& s,
                         size_t pos);
BASE_EXPORT size_t rfind(const StringPiece& self,
                         char c,
                         size_t pos);
BASE_EXPORT size_t rfind(const StringPiece16& self,
                         char16 c,
                         size_t pos);

BASE_EXPORT size_t find_first_of(const StringPiece& self,
                                 const StringPiece& s,
                                 size_t pos);
BASE_EXPORT size_t find_first_of(const StringPiece16& self,
                                 const StringPiece16& s,
                                 size_t pos);

BASE_EXPORT size_t find_first_not_of(const StringPiece& self,
                                     const StringPiece& s,
                                     size_t pos);
BASE_EXPORT size_t find_first_not_of(const StringPiece16& self,
                                     const StringPiece16& s,
                                     size_t pos);
BASE_EXPORT size_t find_first_not_of(const StringPiece& self,
                                     char c,
                                     size_t pos);
BASE_EXPORT size_t find_first_not_of(const StringPiece16& self,
                                     char16 c,
                                     size_t pos);

BASE_EXPORT size_t find_last_of(const StringPiece& self,
                                const StringPiece& s,
                                size_t pos);
BASE_EXPORT size_t find_last_of(const StringPiece16& self,
                                const StringPiece16& s,
                                size_t pos);
BASE_EXPORT size_t find_last_of(const StringPiece& self,
                                char c,
                                size_t pos);
BASE_EXPORT size_t find_last_of(const StringPiece16& self,
                                char16 c,
                                size_t pos);

BASE_EXPORT size_t find_last_not_of(const StringPiece& self,
                                    const StringPiece& s,
                                    size_t pos);
BASE_EXPORT size_t find_last_not_of(const StringPiece16& self,
                                    const StringPiece16& s,
                                    size_t pos);
BASE_EXPORT size_t find_last_not_of(const StringPiece16& self,
                                    char16 c,
                                    size_t pos);
BASE_EXPORT size_t find_last_not_of(const StringPiece& self,
                                    char c,
                                    size_t pos);

BASE_EXPORT StringPiece substr(const StringPiece& self,
                               size_t pos,
                               size_t n);
BASE_EXPORT StringPiece16 substr(const StringPiece16& self,
                                 size_t pos,
                                 size_t n);

}  









template <typename STRING_TYPE> class BasicStringPiece {
 public:
  
  typedef size_t size_type;
  typedef typename STRING_TYPE::value_type value_type;
  typedef const value_type* pointer;
  typedef const value_type& reference;
  typedef const value_type& const_reference;
  typedef ptrdiff_t difference_type;
  typedef const value_type* const_iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

  static const size_type npos;

 public:
  
  
  
  BasicStringPiece() : ptr_(NULL), length_(0) {}
  MOZ_IMPLICIT BasicStringPiece(const value_type* str)
      : ptr_(str),
        length_((str == NULL) ? 0 : STRING_TYPE::traits_type::length(str)) {}
  MOZ_IMPLICIT BasicStringPiece(const STRING_TYPE& str)
      : ptr_(str.data()), length_(str.size()) {}
  BasicStringPiece(const value_type* offset, size_type len)
      : ptr_(offset), length_(len) {}
  BasicStringPiece(const typename STRING_TYPE::const_iterator& begin,
                    const typename STRING_TYPE::const_iterator& end)
      : ptr_((end > begin) ? &(*begin) : NULL),
        length_((end > begin) ? (size_type)(end - begin) : 0) {}

  
  
  
  
  const value_type* data() const { return ptr_; }
  size_type size() const { return length_; }
  size_type length() const { return length_; }
  bool empty() const { return length_ == 0; }

  void clear() {
    ptr_ = NULL;
    length_ = 0;
  }
  void set(const value_type* data, size_type len) {
    ptr_ = data;
    length_ = len;
  }
  void set(const value_type* str) {
    ptr_ = str;
    length_ = str ? STRING_TYPE::traits_type::length(str) : 0;
  }

  value_type operator[](size_type i) const { return ptr_[i]; }

  void remove_prefix(size_type n) {
    ptr_ += n;
    length_ -= n;
  }

  void remove_suffix(size_type n) {
    length_ -= n;
  }

  int compare(const BasicStringPiece<STRING_TYPE>& x) const {
    int r = wordmemcmp(
        ptr_, x.ptr_, (length_ < x.length_ ? length_ : x.length_));
    if (r == 0) {
      if (length_ < x.length_) r = -1;
      else if (length_ > x.length_) r = +1;
    }
    return r;
  }

  STRING_TYPE as_string() const {
    
    return empty() ? STRING_TYPE() : STRING_TYPE(data(), size());
  }

  const_iterator begin() const { return ptr_; }
  const_iterator end() const { return ptr_ + length_; }
  const_reverse_iterator rbegin() const {
    return const_reverse_iterator(ptr_ + length_);
  }
  const_reverse_iterator rend() const {
    return const_reverse_iterator(ptr_);
  }

  size_type max_size() const { return length_; }
  size_type capacity() const { return length_; }

  static int wordmemcmp(const value_type* p,
                        const value_type* p2,
                        size_type N) {
    return STRING_TYPE::traits_type::compare(p, p2, N);
  }

  
  
  void CopyToString(STRING_TYPE* target) const {
    internal::CopyToString(*this, target);
  }

  void AppendToString(STRING_TYPE* target) const {
    internal::AppendToString(*this, target);
  }

  size_type copy(value_type* buf, size_type n, size_type pos = 0) const {
    return internal::copy(*this, buf, n, pos);
  }

  
  bool starts_with(const BasicStringPiece& x) const {
    return ((this->length_ >= x.length_) &&
            (wordmemcmp(this->ptr_, x.ptr_, x.length_) == 0));
  }

  
  bool ends_with(const BasicStringPiece& x) const {
    return ((this->length_ >= x.length_) &&
            (wordmemcmp(this->ptr_ + (this->length_-x.length_),
                        x.ptr_, x.length_) == 0));
  }

  
  size_type find(const BasicStringPiece<STRING_TYPE>& s,
                 size_type pos = 0) const {
    return internal::find(*this, s, pos);
  }
  size_type find(value_type c, size_type pos = 0) const {
    return internal::find(*this, c, pos);
  }

  
  size_type rfind(const BasicStringPiece& s,
                  size_type pos = BasicStringPiece::npos) const {
    return internal::rfind(*this, s, pos);
  }
  size_type rfind(value_type c, size_type pos = BasicStringPiece::npos) const {
    return internal::rfind(*this, c, pos);
  }

  
  size_type find_first_of(const BasicStringPiece& s,
                          size_type pos = 0) const {
    return internal::find_first_of(*this, s, pos);
  }
  size_type find_first_of(value_type c, size_type pos = 0) const {
    return find(c, pos);
  }

  
  size_type find_first_not_of(const BasicStringPiece& s,
                              size_type pos = 0) const {
    return internal::find_first_not_of(*this, s, pos);
  }
  size_type find_first_not_of(value_type c, size_type pos = 0) const {
    return internal::find_first_not_of(*this, c, pos);
  }

  
  size_type find_last_of(const BasicStringPiece& s,
                         size_type pos = BasicStringPiece::npos) const {
    return internal::find_last_of(*this, s, pos);
  }
  size_type find_last_of(value_type c,
                         size_type pos = BasicStringPiece::npos) const {
    return rfind(c, pos);
  }

  
  size_type find_last_not_of(const BasicStringPiece& s,
                             size_type pos = BasicStringPiece::npos) const {
    return internal::find_last_not_of(*this, s, pos);
  }
  size_type find_last_not_of(value_type c,
                             size_type pos = BasicStringPiece::npos) const {
    return internal::find_last_not_of(*this, c, pos);
  }

  
  BasicStringPiece substr(size_type pos,
                          size_type n = BasicStringPiece::npos) const {
    return internal::substr(*this, pos, n);
  }

 protected:
  const value_type* ptr_;
  size_type     length_;
};

template <typename STRING_TYPE>
const typename BasicStringPiece<STRING_TYPE>::size_type
BasicStringPiece<STRING_TYPE>::npos =
    typename BasicStringPiece<STRING_TYPE>::size_type(-1);


#if !defined(COMPILER_MSVC)
extern template class BASE_EXPORT BasicStringPiece<std::string>;
extern template class BASE_EXPORT BasicStringPiece<string16>;
#endif



BASE_EXPORT bool operator==(const StringPiece& x, const StringPiece& y);

inline bool operator!=(const StringPiece& x, const StringPiece& y) {
  return !(x == y);
}

inline bool operator<(const StringPiece& x, const StringPiece& y) {
  const int r = StringPiece::wordmemcmp(
      x.data(), y.data(), (x.size() < y.size() ? x.size() : y.size()));
  return ((r < 0) || ((r == 0) && (x.size() < y.size())));
}

inline bool operator>(const StringPiece& x, const StringPiece& y) {
  return y < x;
}

inline bool operator<=(const StringPiece& x, const StringPiece& y) {
  return !(x > y);
}

inline bool operator>=(const StringPiece& x, const StringPiece& y) {
  return !(x < y);
}



inline bool operator==(const StringPiece16& x, const StringPiece16& y) {
  if (x.size() != y.size())
    return false;

  return StringPiece16::wordmemcmp(x.data(), y.data(), x.size()) == 0;
}

inline bool operator!=(const StringPiece16& x, const StringPiece16& y) {
  return !(x == y);
}

inline bool operator<(const StringPiece16& x, const StringPiece16& y) {
  const int r = StringPiece16::wordmemcmp(
      x.data(), y.data(), (x.size() < y.size() ? x.size() : y.size()));
  return ((r < 0) || ((r == 0) && (x.size() < y.size())));
}

inline bool operator>(const StringPiece16& x, const StringPiece16& y) {
  return y < x;
}

inline bool operator<=(const StringPiece16& x, const StringPiece16& y) {
  return !(x > y);
}

inline bool operator>=(const StringPiece16& x, const StringPiece16& y) {
  return !(x < y);
}

BASE_EXPORT std::ostream& operator<<(std::ostream& o,
                                     const StringPiece& piece);

}  









#define HASH_STRING_PIECE(StringPieceType, string_piece)                \
  std::size_t result = 0;                                               \
  for (StringPieceType::const_iterator i = string_piece.begin();        \
       i != string_piece.end(); ++i)                                    \
    result = (result * 131) + *i;                                       \
  return result;                                                        \

namespace BASE_HASH_NAMESPACE {

template<>
struct hash<base::StringPiece> {
  std::size_t operator()(const base::StringPiece& sp) const {
    HASH_STRING_PIECE(base::StringPiece, sp);
  }
};
template<>
struct hash<base::StringPiece16> {
  std::size_t operator()(const base::StringPiece16& sp16) const {
    HASH_STRING_PIECE(base::StringPiece16, sp16);
  }
};

}  

#endif  
