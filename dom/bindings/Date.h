







#ifndef mozilla_dom_Date_h
#define mozilla_dom_Date_h

class JSObject;
struct JSContext;

namespace JS {
class Value;
template<typename> class MutableHandle;
} 

namespace mozilla {
namespace dom {

class Date
{
public:
  
  Date();
  Date(double aMilliseconds)
    : mMsecSinceEpoch(aMilliseconds)
  {}

  bool IsUndefined() const;
  double TimeStamp() const
  {
    return mMsecSinceEpoch;
  }
  void SetTimeStamp(double aMilliseconds)
  {
    mMsecSinceEpoch = aMilliseconds;
  }
  
  
  bool SetTimeStamp(JSContext* aCx, JSObject* aObject);

  bool ToDateObject(JSContext* aCx, JS::MutableHandle<JS::Value> aRval) const;

private:
  double mMsecSinceEpoch;
};

} 
} 

#endif 
