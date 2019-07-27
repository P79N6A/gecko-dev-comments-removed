









#ifndef WEBRTC_BASE_LINUXFDWALK_H_
#define WEBRTC_BASE_LINUXFDWALK_H_

#ifdef __cplusplus
extern "C" {
#endif











int fdwalk(void (*func)(void *, int), void *opaque);

#ifdef __cplusplus
}  
#endif

#endif
