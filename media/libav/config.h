














#ifndef MOZ_LIBAV_CONFIG_H
#define MOZ_LIBAV_CONFIG_H
#if defined(XP_WIN)
#include "config_win.h"
#elif defined(XP_DARWIN)
#include "config_darwin.h"
#elif defined(XP_UNIX)
#include "config_unix.h"
#endif
#include "config_common.h"
#endif
