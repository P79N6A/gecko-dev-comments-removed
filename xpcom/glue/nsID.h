





#ifndef nsID_h__
#define nsID_h__

#include <string.h>

#include "nscore.h"

#define NSID_LENGTH 39





struct nsID
{
  



  
  uint32_t m0;
  uint16_t m1;
  uint16_t m2;
  uint8_t m3[8];
  

  



  
  


  void Clear();

  




  inline bool Equals(const nsID& aOther) const
  {
    
    return
      (((uint32_t*)&m0)[0] == ((uint32_t*)&aOther.m0)[0]) &&
      (((uint32_t*)&m0)[1] == ((uint32_t*)&aOther.m0)[1]) &&
      (((uint32_t*)&m0)[2] == ((uint32_t*)&aOther.m0)[2]) &&
      (((uint32_t*)&m0)[3] == ((uint32_t*)&aOther.m0)[3]);
  }

  inline bool operator==(const nsID& aOther) const
  {
    return Equals(aOther);
  }

  



  bool Parse(const char* aIDStr);

#ifndef XPCOM_GLUE_AVOID_NSPR
  




  char* ToString() const;

  




  void ToProvidedString(char (&aDest)[NSID_LENGTH]) const;

#endif 

  
};





typedef nsID nsCID;


#define NS_DEFINE_CID(_name, _cidspec) \
  const nsCID _name = _cidspec

#define NS_DEFINE_NAMED_CID(_name) \
  static nsCID k##_name = _name

#define REFNSCID const nsCID&






typedef nsID nsIID;





#define REFNSIID const nsIID&






#define NS_DEFINE_IID(_name, _iidspec) \
  const nsIID _name = _iidspec








#define NS_DECLARE_STATIC_IID_ACCESSOR(the_iid)                         \
  template<typename T, typename U>                                      \
  struct COMTypeInfo;

#define NS_DEFINE_STATIC_IID_ACCESSOR(the_interface, the_iid)           \
  template<typename T>                                                  \
  struct the_interface::COMTypeInfo<the_interface, T> {                 \
    static const nsIID kIID NS_HIDDEN;                                  \
  };                                                                    \
  template<typename T>                                                  \
  const nsIID the_interface::COMTypeInfo<the_interface, T>::kIID NS_HIDDEN = the_iid;





#define NS_DEFINE_STATIC_CID_ACCESSOR(the_cid) \
  static const nsID& GetCID() {static const nsID cid = the_cid; return cid;}

#define NS_GET_IID(T) (T::COMTypeInfo<T, void>::kIID)
#define NS_GET_TEMPLATE_IID(T) (T::template COMTypeInfo<T, void>::kIID)

#endif
