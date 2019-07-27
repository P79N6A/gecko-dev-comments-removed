







#ifndef mozilla_dom_Date_h
#define mozilla_dom_Date_h

#include "js/TypeDecls.h"

namespace mozilla {
namespace dom {

class Date
{
public:
  
  Date();
  explicit Date(double aMilliseconds)
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
