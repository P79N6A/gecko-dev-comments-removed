





































#ifndef nsIFileControlElement_h___
#define nsIFileControlElement_h___

#include "nsISupports.h"
#include "nsTArray.h"
#include "nsString.h"
#include "nsCOMArray.h"

class nsIFile;


#define NS_IFILECONTROLELEMENT_IID \
{ 0x1f6a32fd, 0x9cda, 0x43e9, \
  { 0x90, 0xef, 0x18, 0x0a, 0xd5, 0xe6, 0xcd, 0xa9 } }





class nsIFileControlElement : public nsISupports {
public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IFILECONTROLELEMENT_IID)

  





  virtual void GetDisplayFileName(nsAString& aFileName) = 0;

  


  virtual void SetFileNames(const nsTArray<nsString>& aFileNames) = 0;

  



  virtual void GetFileArray(nsCOMArray<nsIFile>& aFiles) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIFileControlElement,
                              NS_IFILECONTROLELEMENT_IID)

#endif 
