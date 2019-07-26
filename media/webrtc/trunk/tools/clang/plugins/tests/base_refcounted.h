



#ifndef BASE_REFCOUNTED_H_
#define BASE_REFCOUNTED_H_

namespace base {

template <typename T>
class RefCounted {
 public:
  RefCounted() {}
  ~RefCounted() {}
};

template <typename T>
class RefCountedThreadSafe {
 public:
  RefCountedThreadSafe() {}
  ~RefCountedThreadSafe() {}
};

}  




namespace WebKit {

template <typename T>
class RefCounted {
 public:
  RefCounted() {}
  ~RefCounted() {}
};

}  


class PublicRefCountedDtorInHeader
    : public base::RefCounted<PublicRefCountedDtorInHeader> {
 public:
  PublicRefCountedDtorInHeader() {}
  ~PublicRefCountedDtorInHeader() {}

 private:
  friend class base::RefCounted<PublicRefCountedDtorInHeader>;
};


class PublicRefCountedThreadSafeDtorInHeader
    : public base::RefCountedThreadSafe<
          PublicRefCountedThreadSafeDtorInHeader> {
 public:
  PublicRefCountedThreadSafeDtorInHeader() {}
  ~PublicRefCountedThreadSafeDtorInHeader() {}

 private:
  friend class base::RefCountedThreadSafe<
      PublicRefCountedThreadSafeDtorInHeader>;
};


class ProtectedRefCountedDtorInHeader
    : public base::RefCounted<ProtectedRefCountedDtorInHeader> {
 public:
  ProtectedRefCountedDtorInHeader() {}

 protected:
  ~ProtectedRefCountedDtorInHeader() {}

 private:
  friend class base::RefCounted<ProtectedRefCountedDtorInHeader>;
};


class PrivateRefCountedDtorInHeader
    : public base::RefCounted<PrivateRefCountedDtorInHeader> {
 public:
  PrivateRefCountedDtorInHeader() {}

 private:
  ~PrivateRefCountedDtorInHeader() {}
  friend class base::RefCounted<PrivateRefCountedDtorInHeader>;
};



class DerivedProtectedToPublicInHeader
    : public ProtectedRefCountedDtorInHeader {
 public:
  DerivedProtectedToPublicInHeader() {}
  ~DerivedProtectedToPublicInHeader() {}
};



class ImplicitDerivedProtectedToPublicInHeader
    : public ProtectedRefCountedDtorInHeader {
 public:
  ImplicitDerivedProtectedToPublicInHeader() {}
};


class WebKitPublicDtorInHeader
    : public WebKit::RefCounted<WebKitPublicDtorInHeader> {
 public:
  WebKitPublicDtorInHeader() {}
  ~WebKitPublicDtorInHeader() {}
};


class WebKitDerivedPublicDtorInHeader
    : public WebKitPublicDtorInHeader {
 public:
  WebKitDerivedPublicDtorInHeader() {}
  ~WebKitDerivedPublicDtorInHeader() {}
};

#endif  
