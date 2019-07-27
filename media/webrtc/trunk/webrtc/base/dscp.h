









#ifndef WEBRTC_BASE_DSCP_H_
#define WEBRTC_BASE_DSCP_H_

namespace rtc {


enum DiffServCodePoint {
  DSCP_NO_CHANGE = -1,
  DSCP_DEFAULT = 0,  
  DSCP_CS0  = 0,   
  DSCP_CS1  = 8,   
  DSCP_AF11 = 10,
  DSCP_AF12 = 12,
  DSCP_AF13 = 14,
  DSCP_CS2  = 16,
  DSCP_AF21 = 18,
  DSCP_AF22 = 20,
  DSCP_AF23 = 22,
  DSCP_CS3  = 24,
  DSCP_AF31 = 26,
  DSCP_AF32 = 28,
  DSCP_AF33 = 30,
  DSCP_CS4  = 32,
  DSCP_AF41 = 34,  
  DSCP_AF42 = 36,  
  DSCP_AF43 = 38,  
  DSCP_CS5  = 40,  
  DSCP_EF   = 46,  
  DSCP_CS6  = 48,  
  DSCP_CS7  = 56,  
};

}  

 #endif  
