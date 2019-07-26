







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
      : value(Traits::empty())
    {
      MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    explicit Scoped(const Resource& v
                    MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : value(v)
    {
      MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    
    explicit Scoped(Scoped&& v
                    MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : value(Move(v.value))
    {
      MOZ_GUARD_OBJECT_NOTIFIER_INIT;
      v.value = Traits::empty();
    }

    ~Scoped() {
      Traits::release(value);
    }

    
    operator const Resource&() const { return value; }
    const Resource& operator->() const { return value; }
    const Resource& get() const { return value; }
    
    Resource& rwget() { return value; }

    








    Resource forget() {
      Resource tmp = value;
      value = Traits::empty();
      return tmp;
    }

    




    void dispose() {
      Traits::release(value);
      value = Traits::empty();
    }

    bool operator==(const Resource& other) const {
      return value == other;
    }

    







    Scoped& operator=(const Resource& other) {
      return reset(other);
    }
    Scoped& reset(const Resource& other) {
      Traits::release(value);
      value = other;
      return *this;
    }

    
    Scoped& operator=(Scoped&& rhs) {
      MOZ_ASSERT(&rhs != this, "self-move-assignment not allowed");
      this->~Scoped();
      new(this) Scoped(Move(rhs));
      return *this;
    }

  private:
    explicit Scoped(const Scoped& value) MOZ_DELETE;
    Scoped& operator=(const Scoped& value) MOZ_DELETE;

  private:
    Resource value;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};









#define SCOPED_TEMPLATE(name, Traits)                          \
template<typename Type>                                        \
struct name : public mozilla::Scoped<Traits<Type> >            \
{                                                              \
    typedef mozilla::Scoped<Traits<Type> > Super;              \
    typedef typename Super::Resource Resource;                 \
    name& operator=(Resource rhs) {                            \
      Super::operator=(rhs);                                   \
      return *this;                                            \
    }                                                          \
    name& operator=(name&& rhs) {                              \
      Super::operator=(Move(rhs));                             \
      return *this;                                            \
    }                                                          \
    explicit name(MOZ_GUARD_OBJECT_NOTIFIER_ONLY_PARAM)        \
      : Super(MOZ_GUARD_OBJECT_NOTIFIER_ONLY_PARAM_TO_PARENT)  \
    {}                                                         \
    explicit name(Resource rhs                                 \
                  MOZ_GUARD_OBJECT_NOTIFIER_PARAM)             \
      : Super(rhs                                              \
              MOZ_GUARD_OBJECT_NOTIFIER_PARAM_TO_PARENT)       \
    {}                                                         \
    explicit name(name&& rhs                                   \
                  MOZ_GUARD_OBJECT_NOTIFIER_PARAM)             \
      : Super(Move(rhs)                                        \
              MOZ_GUARD_OBJECT_NOTIFIER_PARAM_TO_PARENT)       \
    {}                                                         \
  private:                                                     \
    explicit name(name&) MOZ_DELETE;                           \
    name& operator=(name&) MOZ_DELETE;                         \
};








template<typename T>
struct ScopedFreePtrTraits
{
    typedef T* type;
    static T* empty() { return nullptr; }
    static void release(T* ptr) { free(ptr); }
};
SCOPED_TEMPLATE(ScopedFreePtr, ScopedFreePtrTraits)







template<typename T>
struct ScopedDeletePtrTraits : public ScopedFreePtrTraits<T>
{
    static void release(T* ptr) { delete ptr; }
};
SCOPED_TEMPLATE(ScopedDeletePtr, ScopedDeletePtrTraits)







template<typename T>
struct ScopedDeleteArrayTraits : public ScopedFreePtrTraits<T>
{
    static void release(T* ptr) { delete [] ptr; }
};
SCOPED_TEMPLATE(ScopedDeleteArray, ScopedDeleteArrayTraits)























#define MOZ_TYPE_SPECIFIC_SCOPED_POINTER_TEMPLATE(name, Type, Deleter) \
template <> inline void TypeSpecificDelete(Type * value) { Deleter(value); } \
typedef ::mozilla::TypeSpecificScopedPointer<Type> name;

template <typename T> void TypeSpecificDelete(T * value);

template <typename T>
struct TypeSpecificScopedPointerTraits
{
    typedef T* type;
    static type empty() { return nullptr; }
    static void release(type value)
    {
      if (value)
        TypeSpecificDelete(value);
    }
};

SCOPED_TEMPLATE(TypeSpecificScopedPointer, TypeSpecificScopedPointerTraits)

} 

#endif 
