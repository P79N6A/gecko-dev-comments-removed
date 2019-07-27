









#ifndef WEBRTC_BASE_NATTYPE_H__
#define WEBRTC_BASE_NATTYPE_H__

namespace rtc {


enum NATType {
  NAT_OPEN_CONE,
  NAT_ADDR_RESTRICTED,
  NAT_PORT_RESTRICTED,
  NAT_SYMMETRIC
};


class NAT {
public:
  virtual ~NAT() { }

  
  
  virtual bool IsSymmetric() = 0;

  
  
  virtual bool FiltersIP() = 0;

  
  
  virtual bool FiltersPort() = 0;

  
  static NAT* Create(NATType type);
};

} 

#endif 
