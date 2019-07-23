







































#ifndef nsILE_h__
#define nsILE_h__

#include "nscore.h"
#include "nsISupports.h"
#include "nsCtlCIID.h"




class nsILE : public nsISupports {
public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ILE_IID)
  
  NS_IMETHOD NeedsCTLFix(const PRUnichar*, const PRInt32,
                         const PRInt32, PRBool *) = 0;

  NS_IMETHOD GetPresentationForm(const PRUnichar*, PRUint32,
                                 const char*, char*, PRSize*,
                                 PRBool = PR_FALSE) = 0;

  NS_IMETHOD PrevCluster(const PRUnichar*, PRUint32, 
                         const PRInt32, PRInt32*) = 0;

  NS_IMETHOD NextCluster(const PRUnichar*, PRUint32,
                         const PRInt32, PRInt32*) = 0;

  NS_IMETHOD GetRangeOfCluster(const PRUnichar*, PRUint32,
                               const PRInt32, PRInt32*, PRInt32*) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsILE, NS_ILE_IID)

#endif 

