



#ifndef OVERRIDDEN_METHODS_H_
#define OVERRIDDEN_METHODS_H_


class BaseClass {
 public:
  virtual ~BaseClass() {}
  virtual void SomeMethod() = 0;
  virtual void SomeOtherMethod() = 0;
  virtual void SomeInlineMethod() = 0;
  virtual void SomeNonPureBaseMethod() {}
};

class InterimClass : public BaseClass {
  
  virtual void SomeMethod() = 0;
};

namespace WebKit {
class WebKitObserver {
 public:
  virtual void WebKitModifiedSomething() {};
};
}  

namespace webkit_glue {
class WebKitObserverImpl : WebKit::WebKitObserver {
 public:
  virtual void WebKitModifiedSomething() {};
};
}  

class DerivedClass : public InterimClass,
                     public webkit_glue::WebKitObserverImpl {
 public:
  
  virtual ~DerivedClass() {}
  
  virtual void SomeMethod();
  
  virtual void SomeOtherMethod() override;
  
  virtual void SomeInlineMethod() {}
  
  virtual void WebKitModifiedSomething();
  
  virtual void SomeNonPureBaseMethod() {}
};

#endif  
