



#include "nsNetStrings.h"
#include "nsChannelProperties.h"

nsNetStrings* gNetStrings;

nsNetStrings::nsNetStrings()
  : NS_LITERAL_STRING_INIT(kChannelPolicy, NS_CHANNEL_PROP_CHANNEL_POLICY_STR)
{}


