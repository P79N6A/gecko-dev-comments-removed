




























#ifndef COMMON_UNIQUE_STRING_H_
#define COMMON_UNIQUE_STRING_H_

#include <map>
#include <string>
#include "common/using_std_string.h"

namespace google_breakpad {


class UniqueString;


const UniqueString* ToUniqueString(string);


const UniqueString* ToUniqueString_n(const char* str, size_t n);


const char Index(const UniqueString*, int);


const char* const FromUniqueString(const UniqueString*);


int StrcmpUniqueString(const UniqueString*, const UniqueString*);


bool LessThan_UniqueString(const UniqueString*, const UniqueString*);



















inline static const UniqueString* ustr__empty() {
  static const UniqueString* us = NULL;
  if (!us) us = ToUniqueString("");
  return us;
}


inline static const UniqueString* ustr__ZSeip() {
  static const UniqueString* us = NULL;
  if (!us) us = ToUniqueString("$eip");
  return us;
}


inline static const UniqueString* ustr__ZSebp() {
  static const UniqueString* us = NULL;
  if (!us) us = ToUniqueString("$ebp");
  return us;
}


inline static const UniqueString* ustr__ZSesp() {
  static const UniqueString* us = NULL;
  if (!us) us = ToUniqueString("$esp");
  return us;
}


inline static const UniqueString* ustr__ZSebx() {
  static const UniqueString* us = NULL;
  if (!us) us = ToUniqueString("$ebx");
  return us;
}


inline static const UniqueString* ustr__ZSesi() {
  static const UniqueString* us = NULL;
  if (!us) us = ToUniqueString("$esi");
  return us;
}


inline static const UniqueString* ustr__ZSedi() {
  static const UniqueString* us = NULL;
  if (!us) us = ToUniqueString("$edi");
  return us;
}


inline static const UniqueString* ustr__ZDcbCalleeParams() {
  static const UniqueString* us = NULL;
  if (!us) us = ToUniqueString(".cbCalleeParams");
  return us;
}


inline static const UniqueString* ustr__ZDcbSavedRegs() {
  static const UniqueString* us = NULL;
  if (!us) us = ToUniqueString(".cbSavedRegs");
  return us;
}


inline static const UniqueString* ustr__ZDcbLocals() {
  static const UniqueString* us = NULL;
  if (!us) us = ToUniqueString(".cbLocals");
  return us;
}


inline static const UniqueString* ustr__ZDraSearchStart() {
  static const UniqueString* us = NULL;
  if (!us) us = ToUniqueString(".raSearchStart");
  return us;
}


inline static const UniqueString* ustr__ZDraSearch() {
  static const UniqueString* us = NULL;
  if (!us) us = ToUniqueString(".raSearch");
  return us;
}


inline static const UniqueString* ustr__ZDcbParams() {
  static const UniqueString* us = NULL;
  if (!us) us = ToUniqueString(".cbParams");
  return us;
}


inline static const UniqueString* ustr__Zplus() {
  static const UniqueString* us = NULL;
  if (!us) us = ToUniqueString("+");
  return us;
}


inline static const UniqueString* ustr__Zminus() {
  static const UniqueString* us = NULL;
  if (!us) us = ToUniqueString("-");
  return us;
}


inline static const UniqueString* ustr__Zstar() {
  static const UniqueString* us = NULL;
  if (!us) us = ToUniqueString("*");
  return us;
}


inline static const UniqueString* ustr__Zslash() {
  static const UniqueString* us = NULL;
  if (!us) us = ToUniqueString("/");
  return us;
}


inline static const UniqueString* ustr__Zpercent() {
  static const UniqueString* us = NULL;
  if (!us) us = ToUniqueString("%");
  return us;
}


inline static const UniqueString* ustr__Zat() {
  static const UniqueString* us = NULL;
  if (!us) us = ToUniqueString("@");
  return us;
}


inline static const UniqueString* ustr__Zcaret() {
  static const UniqueString* us = NULL;
  if (!us) us = ToUniqueString("^");
  return us;
}


inline static const UniqueString* ustr__Zeq() {
  static const UniqueString* us = NULL;
  if (!us) us = ToUniqueString("=");
  return us;
}


inline static const UniqueString* ustr__ZDcfa() {
  static const UniqueString* us = NULL;
  if (!us) us = ToUniqueString(".cfa");
  return us;
}


inline static const UniqueString* ustr__ZDra() {
  static const UniqueString* us = NULL;
  if (!us) us = ToUniqueString(".ra");
  return us;
}


inline static const UniqueString* ustr__pc() {
  static const UniqueString* us = NULL;
  if (!us) us = ToUniqueString("pc");
  return us;
}


inline static const UniqueString* ustr__lr() {
  static const UniqueString* us = NULL;
  if (!us) us = ToUniqueString("lr");
  return us;
}


inline static const UniqueString* ustr__sp() {
  static const UniqueString* us = NULL;
  if (!us) us = ToUniqueString("sp");
  return us;
}

template <typename ValueType>
class UniqueStringMap
{
 private:
  static const int N_FIXED = 10;

 public:
  UniqueStringMap() : n_fixed_(0), n_sets_(0), n_gets_(0), n_clears_(0) {};
  ~UniqueStringMap() {};

  
  void clear() {
    ++n_clears_;
    map_.clear();
    n_fixed_ = 0;
  }

  
  void set(const UniqueString* ix, ValueType v) {
    ++n_sets_;
    int i;
    for (i = 0; i < n_fixed_; ++i) {
      if (fixed_keys_[i] == ix) {
        fixed_vals_[i] = v;
        return;
      }
    }
    if (n_fixed_ < N_FIXED) {
      i = n_fixed_;
      fixed_keys_[i] = ix;
      fixed_vals_[i] = v;
      ++n_fixed_;
    } else {
      map_[ix] = v;
    }
  }

  
  ValueType get(bool* have, const UniqueString* ix) const {
    ++n_gets_;
    int i;
    for (i = 0; i < n_fixed_; ++i) {
      if (fixed_keys_[i] == ix) {
        *have = true;
        return fixed_vals_[i];
      }
    }
    typename std::map<const UniqueString*, ValueType>::const_iterator it
        = map_.find(ix);
    if (it == map_.end()) {
      *have = false;
      return ValueType();
    } else {
      *have = true;
      return it->second;
    }
  };

  
  ValueType get(const UniqueString* ix) const {
    ++n_gets_;
    bool found;
    ValueType v = get(&found, ix);
    return found ? v : ValueType();
  }

  
  bool have(const UniqueString* ix) const {
    ++n_gets_;
    bool found;
    (void)get(&found, ix);
    return found;
  }

  
  void copy_to_map(std::map<const UniqueString*, ValueType>* m) const {
    m->clear();
    int i;
    for (i = 0; i < n_fixed_; ++i) {
      (*m)[fixed_keys_[i]] = fixed_vals_[i];
    }
    m->insert(map_.begin(), map_.end());
  }

  
  
  
  

 private:
  
  const UniqueString* fixed_keys_[N_FIXED];
  ValueType           fixed_vals_[N_FIXED];
  int                 n_fixed_;  
  
  std::map<const UniqueString*, ValueType> map_;

  
  mutable int n_sets_, n_gets_, n_clears_;
};

}  

#endif  
