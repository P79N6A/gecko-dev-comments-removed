



#ifndef SANDBOX_LINUX_BPF_DSL_CONS_H_
#define SANDBOX_LINUX_BPF_DSL_CONS_H_

#include "base/memory/ref_counted.h"
#include "sandbox/sandbox_export.h"

namespace sandbox {
namespace cons {










































template <typename T>
class Cell;
template <typename T>
class ListIterator;


template <typename T>
using List = scoped_refptr<const Cell<T>>;


template <typename T>
List<T> Cons(const T& head, const List<T>& tail) {
  return List<T>(new const Cell<T>(head, tail));
}


template <typename T>
class Cell : public base::RefCounted<Cell<T>> {
 public:
  Cell(const T& head, const List<T>& tail) : head_(head), tail_(tail) {}

  
  const T& head() const { return head_; }

  
  const List<T>& tail() const { return tail_; }

 private:
  virtual ~Cell() {}

  T head_;
  List<T> tail_;

  friend class base::RefCounted<Cell<T>>;
  DISALLOW_COPY_AND_ASSIGN(Cell);
};



template <typename T>
ListIterator<T> begin(const List<T>& list) {
  return ListIterator<T>(list);
}




template <typename T>
ListIterator<T> end(const List<T>& list) {
  return ListIterator<T>();
}



template <typename T>
class ListIterator {
 public:
  ListIterator() : list_() {}
  explicit ListIterator(const List<T>& list) : list_(list) {}

  const T& operator*() const { return list_->head(); }

  ListIterator& operator++() {
    list_ = list_->tail();
    return *this;
  }

  friend bool operator==(const ListIterator& lhs, const ListIterator& rhs) {
    return lhs.list_ == rhs.list_;
  }

 private:
  List<T> list_;
};

template <typename T>
bool operator!=(const ListIterator<T>& lhs, const ListIterator<T>& rhs) {
  return !(lhs == rhs);
}

}  
}  

#endif  
