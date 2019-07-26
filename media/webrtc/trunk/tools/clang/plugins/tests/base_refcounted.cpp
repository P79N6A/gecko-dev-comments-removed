



#include "base_refcounted.h"

#include <cstddef>

namespace {


class AnonymousDerivedProtectedToPublicInImpl
    : public ProtectedRefCountedDtorInHeader {
 public:
  AnonymousDerivedProtectedToPublicInImpl() {}
  ~AnonymousDerivedProtectedToPublicInImpl() {}
};

}  


class PublicRefCountedDtorInImpl
    : public base::RefCounted<PublicRefCountedDtorInImpl> {
 public:
  PublicRefCountedDtorInImpl() {}
  ~PublicRefCountedDtorInImpl() {}

 private:
  friend class base::RefCounted<PublicRefCountedDtorInImpl>;
};

class Foo {
 public:
  class BarInterface {
   protected:
    virtual ~BarInterface() {}
  };

  typedef base::RefCounted<BarInterface> RefCountedBar;
  typedef RefCountedBar AnotherTypedef;
};

class Baz {
 public:
  typedef typename Foo::AnotherTypedef MyLocalTypedef;
};


class UnsafeTypedefChainInImpl : public Baz::MyLocalTypedef {
 public:
  UnsafeTypedefChainInImpl() {}
  ~UnsafeTypedefChainInImpl() {}
};

int main() {
  PublicRefCountedDtorInHeader bad;
  PublicRefCountedDtorInImpl also_bad;

  ProtectedRefCountedDtorInHeader* protected_ok = NULL;
  PrivateRefCountedDtorInHeader* private_ok = NULL;

  DerivedProtectedToPublicInHeader still_bad;
  PublicRefCountedThreadSafeDtorInHeader another_bad_variation;
  AnonymousDerivedProtectedToPublicInImpl and_this_is_bad_too;
  ImplicitDerivedProtectedToPublicInHeader bad_yet_again;
  UnsafeTypedefChainInImpl and_again_this_is_bad;

  WebKitPublicDtorInHeader ignored;
  WebKitDerivedPublicDtorInHeader still_ignored;

  return 0;
}
