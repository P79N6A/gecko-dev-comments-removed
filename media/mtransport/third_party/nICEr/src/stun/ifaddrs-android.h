































#ifndef WEBRTC_BASE_IFADDRS_ANDROID_H_
#define WEBRTC_BASE_IFADDRS_ANDROID_H_

#include <stdio.h>
#include <sys/socket.h>





struct ifaddrs {
  struct ifaddrs* ifa_next;
  char* ifa_name;
  unsigned int ifa_flags;
  struct sockaddr* ifa_addr;
  struct sockaddr* ifa_netmask;
  

};

int android_getifaddrs(struct ifaddrs** result);
void android_freeifaddrs(struct ifaddrs* addrs);

#endif  

