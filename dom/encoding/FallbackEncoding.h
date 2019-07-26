



#ifndef mozilla_dom_FallbackEncoding_h_
#define mozilla_dom_FallbackEncoding_h_

#include "nsString.h"

namespace mozilla {
namespace dom {

class FallbackEncoding
{
public:

  





  static void FromLocale(nsACString& aFallback);

  

  



  static void Initialize();

  



  static void Shutdown();

private:

  


  static FallbackEncoding* sInstance;

  FallbackEncoding();
  ~FallbackEncoding();

  


  void Invalidate()
  {
    mFallback.Truncate();
  }

  static int PrefChanged(const char*, void*);

  



  void Get(nsACString& aFallback);

  nsCString mFallback;
};

} 
} 

#endif 

