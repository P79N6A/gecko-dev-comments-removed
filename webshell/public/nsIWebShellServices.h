



































#ifndef nsIWebShellServices_h___
#define nsIWebShellServices_h___

#include "nsISupports.h"
#include "nsIParser.h"




#define NS_IWEB_SHELL_SERVICES_IID \
{ 0x8b26a346, 0x031e, 0x11d3, {0xae, 0xea, 0x00, 0x10, 0x83, 0x00, 0xff, 0x91} }




class nsIWebShellServices : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IWEB_SHELL_SERVICES_IID)

  NS_IMETHOD ReloadDocument(const char* aCharset = nsnull , 
                            PRInt32 aSource = kCharsetUninitialized) = 0;
  NS_IMETHOD StopDocumentLoad(void) = 0;
  NS_IMETHOD SetRendering(PRBool aRender) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIWebShellServices, NS_IWEB_SHELL_SERVICES_IID)


#define NS_DECL_NSIWEBSHELLSERVICES \
  NS_IMETHOD ReloadDocument(const char *aCharset=nsnull, PRInt32 aSource=kCharsetUninitialized); \
  NS_IMETHOD StopDocumentLoad(void); \
  NS_IMETHOD SetRendering(PRBool aRender); 

#endif 
