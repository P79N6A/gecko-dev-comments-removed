




































#ifndef npplat_h_
#define npplat_h_

#include "npapi.h"
#include "npfunctions.h"

#ifdef XP_WIN
#include "windows.h"
#endif

#ifdef XP_UNIX
#include <stdio.h>
#endif

#ifdef XP_MAC
#include <Carbon/Carbon.h>
#endif

#ifndef HIBYTE
#define HIBYTE(i) (i >> 8)
#endif

#ifndef LOBYTE
#define LOBYTE(i) (i & 0xff)
#endif

#endif 
