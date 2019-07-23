


















































#ifndef MOZILLA_INTERNAL_API

#include "nsEmbedString.h"

typedef nsCString nsPromiseFlatCString;
typedef nsCString nsCAutoString;

#define PromiseFlatCString nsCString

#else
#include "nsString.h"
#include "nsPromiseFlatString.h"
#endif
