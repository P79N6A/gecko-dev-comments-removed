



































#ifndef nsChannelProperties_h__
#define nsChannelProperties_h__

#include "nsStringGlue.h"
#ifdef IMPL_NS_NET
#include "nsNetStrings.h"
#endif















#define NS_CHANNEL_PROP_CONTENT_LENGTH_STR "content-length"






#define NS_CHANNEL_PROP_CONTENT_DISPOSITION_STR "content-disposition"






#define NS_CHANNEL_PROP_CHANNEL_POLICY_STR "channel-policy"

#ifdef IMPL_NS_NET
#define NS_CHANNEL_PROP_CONTENT_LENGTH gNetStrings->kContentLength
#define NS_CHANNEL_PROP_CONTENT_DISPOSITION gNetStrings->kContentDisposition
#define NS_CHANNEL_PROP_CHANNEL_POLICY gNetStrings->kChannelPolicy
#else
#define NS_CHANNEL_PROP_CONTENT_LENGTH \
  NS_LITERAL_STRING(NS_CHANNEL_PROP_CONTENT_LENGTH_STR)
#define NS_CHANNEL_PROP_CONTENT_DISPOSITION \
  NS_LITERAL_STRING(NS_CHANNEL_PROP_CONTENT_DISPOSITION_STR)
#define NS_CHANNEL_PROP_CHANNEL_POLICY \
  NS_LITERAL_STRING(NS_CHANNEL_PROP_CHANNEL_POLICY_STR)
#endif

#endif
