




































#ifndef nsID_h__
#define nsID_h__

#include <string.h>

#ifndef nscore_h___
#include "nscore.h"
#endif






struct nsID {
  



  
  PRUint32 m0;
  PRUint16 m1;
  PRUint16 m2;
  PRUint8 m3[8];
  

  



  
  




  inline PRBool Equals(const nsID& other) const {
    
    
    
    
    
    

    return (PRBool)
      ((((PRUint32*) &m0)[0] == ((PRUint32*) &other.m0)[0]) &&
       (((PRUint32*) &m0)[1] == ((PRUint32*) &other.m0)[1]) &&
       (((PRUint32*) &m0)[2] == ((PRUint32*) &other.m0)[2]) &&
       (((PRUint32*) &m0)[3] == ((PRUint32*) &other.m0)[3]));
  }

  



  NS_COM_GLUE PRBool Parse(const char *aIDStr);

  



  NS_COM_GLUE char* ToString() const;
  
};





typedef nsID nsCID;


#define NS_DEFINE_CID(_name, _cidspec) \
  const nsCID _name = _cidspec

#define REFNSCID const nsCID&






typedef nsID nsIID;





#define REFNSIID const nsIID&





 
#define NS_DEFINE_IID(_name, _iidspec) \
  const nsIID _name = _iidspec








#define NS_DECLARE_STATIC_IID_ACCESSOR(the_iid)                         \
  template <class Dummy>                                                \
  struct COMTypeInfo                                                    \
  {                                                                     \
    static const nsIID kIID NS_HIDDEN;                                  \
  };                                                                    \
  static const nsIID& GetIID() {return COMTypeInfo<int>::kIID;}

#define NS_DEFINE_STATIC_IID_ACCESSOR(the_interface, the_iid)           \
  template <class Dummy>                                                \
  const nsIID the_interface::COMTypeInfo<Dummy>::kIID NS_HIDDEN = the_iid;





#define NS_DEFINE_STATIC_CID_ACCESSOR(the_cid) \
  static const nsID& GetCID() {static const nsID cid = the_cid; return cid;}

#define NS_GET_IID(T) (::T::COMTypeInfo<int>::kIID)
#define NS_GET_TEMPLATE_IID(T) (T::template COMTypeInfo<int>::kIID)

#endif
