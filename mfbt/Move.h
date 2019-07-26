






#ifndef mozilla_Move_h_
#define mozilla_Move_h_

namespace mozilla {





































































































template<typename T>
class MoveRef
{
    T* pointer;

  public:
    explicit MoveRef(T& t) : pointer(&t) { }
    T& operator*() const { return *pointer; }
    T* operator->() const { return pointer; }
    operator T& () const { return *pointer; }
};

template<typename T>
inline MoveRef<T>
Move(T& t)
{
  return MoveRef<T>(t);
}

template<typename T>
inline MoveRef<T>
Move(const T& t)
{
  return MoveRef<T>(const_cast<T&>(t));
}

} 

#endif 
