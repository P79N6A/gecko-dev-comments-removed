



#ifndef mozilla_dom_FallbackEncoding_h_
#define mozilla_dom_FallbackEncoding_h_

#include "nsString.h"

namespace mozilla {
namespace dom {

class FallbackEncoding
{
public:

  


  static bool sGuessFallbackFromTopLevelDomain;

  





  static void FromLocale(nsACString& aFallback);

  





  static bool IsParticipatingTopLevelDomain(const nsACString& aTLD);

  






  static void FromTopLevelDomain(const nsACString& aTLD, nsACString& aFallback);

  

  



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

  static void PrefChanged(const char*, void*);

  



  void Get(nsACString& aFallback);

  nsCString mFallback;
};

} 
} 

#endif 

