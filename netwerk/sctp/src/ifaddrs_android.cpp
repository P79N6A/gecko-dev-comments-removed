

















#include "ifaddrs-android-ext.h"

#include <stdlib.h>
#include <string.h>
#include "ScopedFd.h"
#include "LocalArray.h"



uint8_t* sockaddrBytes(int family, sockaddr_storage* ss) {
    if (family == AF_INET) {
        sockaddr_in* ss4 = reinterpret_cast<sockaddr_in*>(ss);
        return reinterpret_cast<uint8_t*>(&ss4->sin_addr);
    } else if (family == AF_INET6) {
        sockaddr_in6* ss6 = reinterpret_cast<sockaddr_in6*>(ss);
        return reinterpret_cast<uint8_t*>(&ss6->sin6_addr);
    }
    return NULL;
}




bool ifa_setNameAndFlagsByIndex(ifaddrs *self, int interfaceIndex) {
    
    char buf[IFNAMSIZ];
    char* name = if_indextoname(interfaceIndex, buf);
    if (name == NULL) {
        return false;
    }
    self->ifa_name = new char[strlen(name) + 1];
    strcpy(self->ifa_name, name);

    
    ScopedFd fd(socket(AF_INET, SOCK_DGRAM, 0));
    if (fd.get() == -1) {
        return false;
    }
    ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, name);
    int rc = ioctl(fd.get(), SIOCGIFFLAGS, &ifr);
    if (rc == -1) {
        return false;
    }
    self->ifa_flags = ifr.ifr_flags;
    return true;
}





void ifa_setAddress(ifaddrs *self, int family, void* data, size_t byteCount) {
    
    sockaddr_storage* ss = new sockaddr_storage;
    memset(ss, 0, sizeof(*ss));
    self->ifa_addr = reinterpret_cast<sockaddr*>(ss);
    ss->ss_family = family;
    uint8_t* dst = sockaddrBytes(family, ss);
    memcpy(dst, data, byteCount);
}



void ifa_setNetmask(ifaddrs *self, int family, size_t prefixLength) {
    
    sockaddr_storage* ss = new sockaddr_storage;
    memset(ss, 0, sizeof(*ss));
    self->ifa_netmask = reinterpret_cast<sockaddr*>(ss);
    ss->ss_family = family;
    uint8_t* dst = sockaddrBytes(family, ss);
    memset(dst, 0xff, prefixLength / 8);
    if ((prefixLength % 8) != 0) {
        dst[prefixLength/8] = (0xff << (8 - (prefixLength % 8)));
    }
}


struct addrReq_struct {
    nlmsghdr netlinkHeader;
    ifaddrmsg msg;
};

inline bool sendNetlinkMessage(int fd, const void* data, size_t byteCount) {
    ssize_t sentByteCount = TEMP_FAILURE_RETRY(send(fd, data, byteCount, 0));
    return (sentByteCount == static_cast<ssize_t>(byteCount));
}

inline ssize_t recvNetlinkMessage(int fd, char* buf, size_t byteCount) {
    return TEMP_FAILURE_RETRY(recv(fd, buf, byteCount, 0));
}


int getifaddrs(ifaddrs** result)
{
    
    *result = NULL;

    
    ScopedFd fd(socket(AF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE));
    if (fd.get() < 0) {
        return -1;
    }

    
    addrReq_struct addrRequest;
    memset(&addrRequest, 0, sizeof(addrRequest));
    addrRequest.netlinkHeader.nlmsg_flags = NLM_F_REQUEST | NLM_F_MATCH;
    addrRequest.netlinkHeader.nlmsg_type = RTM_GETADDR;
    addrRequest.netlinkHeader.nlmsg_len = NLMSG_ALIGN(NLMSG_LENGTH(sizeof(addrRequest)));
    addrRequest.msg.ifa_family = AF_UNSPEC; 
    addrRequest.msg.ifa_index = 0; 
    if (!sendNetlinkMessage(fd.get(), &addrRequest, addrRequest.netlinkHeader.nlmsg_len)) {
        return -1;
    }

    
    LocalArray<0> buf(65536); 
    ssize_t bytesRead;
    while ((bytesRead  = recvNetlinkMessage(fd.get(), &buf[0], buf.size())) > 0) {
        nlmsghdr* hdr = reinterpret_cast<nlmsghdr*>(&buf[0]);
        for (; NLMSG_OK(hdr, (size_t)bytesRead); hdr = NLMSG_NEXT(hdr, bytesRead)) {
            switch (hdr->nlmsg_type) {
            case NLMSG_DONE:
                return 0;
            case NLMSG_ERROR:
                return -1;
            case RTM_NEWADDR:
                {
                    ifaddrmsg* address = reinterpret_cast<ifaddrmsg*>(NLMSG_DATA(hdr));
                    rtattr* rta = IFA_RTA(address);
                    size_t ifaPayloadLength = IFA_PAYLOAD(hdr);
                    while (RTA_OK(rta, ifaPayloadLength)) {
                        if (rta->rta_type == IFA_LOCAL) {
                            int family = address->ifa_family;
                            if (family == AF_INET || family == AF_INET6) {
                                ifaddrs *next = *result;
                                *result = new ifaddrs;
                                memset(*result, 0, sizeof(ifaddrs));
                                (*result)->ifa_next = next;
                                if (!ifa_setNameAndFlagsByIndex(*result, address->ifa_index)) {
                                    return -1;
                                }
                                ifa_setAddress(*result, family, RTA_DATA(rta), RTA_PAYLOAD(rta));
                                ifa_setNetmask(*result, family, address->ifa_prefixlen);
                            }
                        }
                        rta = RTA_NEXT(rta, ifaPayloadLength);
                    }
                }
                break;
            }
        }
    }
    
    return -1;
}


void freeifaddrs(ifaddrs* addresses) {
    ifaddrs* self = addresses;
    while (self != NULL) {
        delete[] self->ifa_name;
        delete self->ifa_addr;
        delete self->ifa_netmask;
        ifaddrs* next = self->ifa_next;
        delete self;
        self = next;
    }
}
