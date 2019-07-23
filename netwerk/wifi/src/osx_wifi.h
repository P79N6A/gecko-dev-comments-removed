





























































#ifndef GEARS_GEOLOCATION_OSX_WIFI_H__
#define GEARS_GEOLOCATION_OSX_WIFI_H__

#include <CoreFoundation/CoreFoundation.h>

extern "C" {

typedef SInt32 WIErr;




typedef struct __WirelessContext *WirelessContextPtr;




typedef WIErr (*WirelessAttachFunction)(WirelessContextPtr *outContext,
                                        const UInt32);




typedef WIErr (*WirelessDetachFunction)(WirelessContextPtr inContext);

typedef UInt16 WINetworkInfoFlags;

struct WirelessNetworkInfo
{
  UInt16 channel;            
  SInt16 noise;              
  SInt16 signal;             
  UInt8 macAddress[6];       
  UInt16 beaconInterval;     
  WINetworkInfoFlags flags;  
  UInt16 nameLen;
  SInt8 name[32];
};












typedef WIErr (*WirelessScanSplitFunction)(WirelessContextPtr inContext,
                                           CFArrayRef *apList,
                                           CFArrayRef *adhocList,
                                           const UInt32 stripDups);

}  

#endif  
