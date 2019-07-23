
























#ifndef _PIXMAN_SSE_H_
#define _PIXMAN_SSE_H_

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "pixman-private.h"

#ifdef USE_SSE2

#if !defined(__amd64__) && !defined(__x86_64__)
pixman_bool_t pixman_have_sse(void);
#else
#define pixman_have_sse() TRUE
#endif

#else
#define pixman_have_sse() FALSE
#endif

#ifdef USE_SSE2

void fbComposeSetupSSE(void);

#endif 

#endif 
