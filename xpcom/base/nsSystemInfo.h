





#ifndef _NSSYSTEMINFO_H_
#define _NSSYSTEMINFO_H_

#include "nsHashPropertyBag.h"
#if defined(XP_WIN)
#include "nsIObserver.h"
#endif 

class nsSystemInfo final
  : public nsHashPropertyBag
#if defined(XP_WIN)
  , public nsIObserver
#endif 
{
public:
#if defined(XP_WIN)
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIOBSERVER
#endif 

  nsSystemInfo();

  nsresult Init();

  
  
  static uint32_t gUserUmask;

protected:
  void SetInt32Property(const nsAString& aPropertyName,
                        const int32_t aValue);
  void SetUint32Property(const nsAString& aPropertyName,
                         const uint32_t aValue);
  void SetUint64Property(const nsAString& aPropertyName,
                         const uint64_t aValue);

private:
  ~nsSystemInfo();

#if defined(XP_WIN)
  nsresult GetProfileHDDInfo();
#endif 
};

#define NS_SYSTEMINFO_CONTRACTID "@mozilla.org/system-info;1"
#define NS_SYSTEMINFO_CID \
{ 0xd962398a, 0x99e5, 0x49b2, \
{ 0x85, 0x7a, 0xc1, 0x59, 0x04, 0x9c, 0x7f, 0x6c } }

#endif 
