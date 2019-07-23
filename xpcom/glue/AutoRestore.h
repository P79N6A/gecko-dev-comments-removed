






































#ifndef mozilla_AutoRestore_h_
#define mozilla_AutoRestore_h_

namespace mozilla {

  











  template <class T>
  class AutoRestore
  {
  private:
    T& mLocation;
    T mValue;
  public:
    AutoRestore(T& aValue) : mLocation(aValue), mValue(aValue) {}
    ~AutoRestore() { mLocation = mValue; }
  };

}

#endif 
