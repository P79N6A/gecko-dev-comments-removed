















#ifndef IFADDRS_ANDROID_EXT_H_included
#define IFADDRS_ANDROID_EXT_H_included

#include <arpa/inet.h>
#include <errno.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>






typedef struct ifaddrs {
    
    struct ifaddrs* ifa_next;

    
    char* ifa_name;

    
    unsigned int ifa_flags;

    
    struct sockaddr* ifa_addr;

    
    struct sockaddr* ifa_netmask;
} ifaddrs;

#ifdef __cplusplus
extern "C" {
#endif
    int getifaddrs(ifaddrs** result);
    void freeifaddrs(ifaddrs* addresses);
#ifdef __cplusplus
}
#endif

#endif
