



#ifndef SANDBOX_LINUX_SECCOMP_BPF_SYSCALL_ITERATOR_H__
#define SANDBOX_LINUX_SECCOMP_BPF_SYSCALL_ITERATOR_H__

#include <stdint.h>

#include <iterator>

#include "base/macros.h"
#include "sandbox/sandbox_export.h"

namespace sandbox {
















class SANDBOX_EXPORT SyscallSet {
 public:
  class Iterator;

  SyscallSet(const SyscallSet& ss) : set_(ss.set_) {}
  ~SyscallSet() {}

  Iterator begin() const;
  Iterator end() const;

  
  
  static SyscallSet All() { return SyscallSet(Set::ALL); }

  
  
  static SyscallSet ValidOnly() { return SyscallSet(Set::VALID_ONLY); }

  
  
  
  static SyscallSet InvalidOnly() { return SyscallSet(Set::INVALID_ONLY); }

  
  
  static bool IsValid(uint32_t num);

 private:
  enum class Set { ALL, VALID_ONLY, INVALID_ONLY };

  explicit SyscallSet(Set set) : set_(set) {}

  Set set_;

  friend bool operator==(const SyscallSet&, const SyscallSet&);
  DISALLOW_ASSIGN(SyscallSet);
};

SANDBOX_EXPORT bool operator==(const SyscallSet& lhs, const SyscallSet& rhs);



class SyscallSet::Iterator
    : public std::iterator<std::input_iterator_tag, uint32_t> {
 public:
  Iterator(const Iterator& it)
      : set_(it.set_), done_(it.done_), num_(it.num_) {}
  ~Iterator() {}

  uint32_t operator*() const;
  Iterator& operator++();

 private:
  Iterator(Set set, bool done);

  uint32_t NextSyscall() const;

  Set set_;
  bool done_;
  uint32_t num_;

  friend SyscallSet;
  friend bool operator==(const Iterator&, const Iterator&);
  DISALLOW_ASSIGN(Iterator);
};

SANDBOX_EXPORT bool operator==(const SyscallSet::Iterator& lhs,
                               const SyscallSet::Iterator& rhs);
SANDBOX_EXPORT bool operator!=(const SyscallSet::Iterator& lhs,
                               const SyscallSet::Iterator& rhs);

}  

#endif
