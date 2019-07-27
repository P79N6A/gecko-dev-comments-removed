



#ifndef nsIWebShellServices_h___
#define nsIWebShellServices_h___

#include "nsISupports.h"
#include "nsCharsetSource.h"




#define NS_IWEB_SHELL_SERVICES_IID \
{ 0x0c628af0, 0x5638, 0x4703, {0x8f, 0x99, 0xed, 0x61, 0x34, 0xc9, 0xde, 0x18} }



class nsIWebShellServices : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IWEB_SHELL_SERVICES_IID)

  NS_IMETHOD ReloadDocument(const char* aCharset = nullptr ,
                            int32_t aSource = kCharsetUninitialized) = 0;
  NS_IMETHOD StopDocumentLoad(void) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIWebShellServices, NS_IWEB_SHELL_SERVICES_IID)


#define NS_DECL_NSIWEBSHELLSERVICES \
  NS_IMETHOD ReloadDocument(const char* aCharset = nullptr, \
                            int32_t aSource = kCharsetUninitialized) MOZ_OVERRIDE; \
  NS_IMETHOD StopDocumentLoad(void) MOZ_OVERRIDE;

#endif 
