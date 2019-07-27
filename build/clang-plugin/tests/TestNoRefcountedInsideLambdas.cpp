#define MOZ_STRONG_REF __attribute__((annotate("moz_strong_ref")))

struct RefCountedBase {
  void AddRef();
  void Release();
};

template <class T>
struct SmartPtr {
  T* MOZ_STRONG_REF t;
  T* operator->() const;
};

struct R : RefCountedBase {
  void method();
};

void take(...);
void foo() {
  R* ptr;
  SmartPtr<R> sp;
  take([&]() {
    ptr->method(); 
  });
  take([&]() {
    sp->method();
  });
  take([&]() {
    take(ptr); 
  });
  take([&]() {
    take(sp);
  });
  take([=]() {
    ptr->method(); 
  });
  take([=]() {
    sp->method();
  });
  take([=]() {
    take(ptr); 
  });
  take([=]() {
    take(sp);
  });
  take([ptr]() {
    ptr->method(); 
  });
  take([sp]() {
    sp->method();
  });
  take([ptr]() {
    take(ptr); 
  });
  take([sp]() {
    take(sp);
  });
}
