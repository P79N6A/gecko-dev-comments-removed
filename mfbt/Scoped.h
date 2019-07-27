







#ifndef mozilla_Scoped_h
#define mozilla_Scoped_h














































#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"
#include "mozilla/GuardObjects.h"
#include "mozilla/Move.h"
#include "mozilla/NullPtr.h"

namespace mozilla {
















template<typename Traits>
class Scoped
{
public:
  typedef typename Traits::type Resource;

  explicit Scoped(MOZ_GUARD_OBJECT_NOTIFIER_ONLY_PARAM)
    : mValue(Traits::empty())
  {
    MOZ_GUARD_OBJECT_NOTIFIER_INIT;
  }

  explicit Scoped(const Resource& aValue
                  MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
    : mValue(aValue)
  {
    MOZ_GUARD_OBJECT_NOTIFIER_INIT;
  }

  
  explicit Scoped(Scoped&& aOther
                  MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
    : mValue(Move(aOther.mValue))
  {
    MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    aOther.mValue = Traits::empty();
  }

  ~Scoped() { Traits::release(mValue); }

  
  operator const Resource&() const { return mValue; }
  const Resource& operator->() const { return mValue; }
  const Resource& get() const { return mValue; }
  
  Resource& rwget() { return mValue; }

  








  Resource forget()
  {
    Resource tmp = mValue;
    mValue = Traits::empty();
    return tmp;
  }

  




  void dispose()
  {
    Traits::release(mValue);
    mValue = Traits::empty();
  }

  bool operator==(const Resource& aOther) const { return mValue == aOther; }

  







  Scoped& operator=(const Resource& aOther) { return reset(aOther); }

  Scoped& reset(const Resource& aOther)
  {
    Traits::release(mValue);
    mValue = aOther;
    return *this;
  }

  
  Scoped& operator=(Scoped&& aRhs)
  {
    MOZ_ASSERT(&aRhs != this, "self-move-assignment not allowed");
    this->~Scoped();
    new(this) Scoped(Move(aRhs));
    return *this;
  }

private:
  explicit Scoped(const Scoped& aValue) MOZ_DELETE;
  Scoped& operator=(const Scoped& aValue) MOZ_DELETE;

private:
  Resource mValue;
  MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};









#define SCOPED_TEMPLATE(name, Traits)                                         \
template<typename Type>                                                       \
struct name : public mozilla::Scoped<Traits<Type> >                           \
{                                                                             \
  typedef mozilla::Scoped<Traits<Type> > Super;                               \
  typedef typename Super::Resource Resource;                                  \
  name& operator=(Resource aRhs)                                              \
  {                                                                           \
    Super::operator=(aRhs);                                                   \
    return *this;                                                             \
  }                                                                           \
  name& operator=(name&& aRhs)                                                \
  {                                                                           \
    Super::operator=(Move(aRhs));                                             \
    return *this;                                                             \
  }                                                                           \
  explicit name(MOZ_GUARD_OBJECT_NOTIFIER_ONLY_PARAM)                         \
    : Super(MOZ_GUARD_OBJECT_NOTIFIER_ONLY_PARAM_TO_PARENT)                   \
  {}                                                                          \
  explicit name(Resource aRhs                                                 \
                MOZ_GUARD_OBJECT_NOTIFIER_PARAM)                              \
    : Super(aRhs                                                              \
            MOZ_GUARD_OBJECT_NOTIFIER_PARAM_TO_PARENT)                        \
  {}                                                                          \
  explicit name(name&& aRhs                                                   \
                MOZ_GUARD_OBJECT_NOTIFIER_PARAM)                              \
    : Super(Move(aRhs)                                                        \
            MOZ_GUARD_OBJECT_NOTIFIER_PARAM_TO_PARENT)                        \
  {}                                                                          \
private:                                                                      \
  explicit name(name&) MOZ_DELETE;                                            \
  name& operator=(name&) MOZ_DELETE;                                          \
};








template<typename T>
struct ScopedFreePtrTraits
{
  typedef T* type;
  static T* empty() { return nullptr; }
  static void release(T* aPtr) { free(aPtr); }
};
SCOPED_TEMPLATE(ScopedFreePtr, ScopedFreePtrTraits)







template<typename T>
struct ScopedDeletePtrTraits : public ScopedFreePtrTraits<T>
{
  static void release(T* aPtr) { delete aPtr; }
};
SCOPED_TEMPLATE(ScopedDeletePtr, ScopedDeletePtrTraits)























#define MOZ_TYPE_SPECIFIC_SCOPED_POINTER_TEMPLATE(name, Type, Deleter) \
template <> inline void TypeSpecificDelete(Type* aValue) { Deleter(aValue); } \
typedef ::mozilla::TypeSpecificScopedPointer<Type> name;

template <typename T> void TypeSpecificDelete(T* aValue);

template <typename T>
struct TypeSpecificScopedPointerTraits
{
  typedef T* type;
  static type empty() { return nullptr; }
  static void release(type aValue)
  {
    if (aValue) {
      TypeSpecificDelete(aValue);
    }
  }
};

SCOPED_TEMPLATE(TypeSpecificScopedPointer, TypeSpecificScopedPointerTraits)

} 

#endif 
