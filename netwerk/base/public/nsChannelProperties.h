



































#ifndef nsChannelProperties_h__
#define nsChannelProperties_h__

#include "nsStringGlue.h"
#ifdef IMPL_NS_NET
#include "nsNetStrings.h"
#endif















#define NS_CHANNEL_PROP_CONTENT_LENGTH_STR "content-length"

#ifdef IMPL_NS_NET
#define NS_CHANNEL_PROP_CONTENT_LENGTH gNetStrings->kContentLength
#else
#define NS_CHANNEL_PROP_CONTENT_LENGTH \
  NS_LITERAL_STRING(NS_CHANNEL_PROP_CONTENT_LENGTH_STR)
#endif

#endif
