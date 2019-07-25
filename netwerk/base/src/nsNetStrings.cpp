



#include "nsNetStrings.h"
#include "nsChannelProperties.h"

NS_HIDDEN_(nsNetStrings*) gNetStrings;

nsNetStrings::nsNetStrings()
  : NS_LITERAL_STRING_INIT(kContentLength, NS_CHANNEL_PROP_CONTENT_LENGTH_STR),
    NS_LITERAL_STRING_INIT(kChannelPolicy, NS_CHANNEL_PROP_CHANNEL_POLICY_STR)
{}


