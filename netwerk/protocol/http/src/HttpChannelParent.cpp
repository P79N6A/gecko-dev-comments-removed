







































#include "mozilla/net/HttpChannelParent.h"

namespace mozilla {
namespace net {


HttpChannelParent::HttpChannelParent()
{
}

HttpChannelParent::~HttpChannelParent()
{
}



nsresult HttpChannelParent::RecvasyncOpen(const nsCString& uri)
{
  puts("[HttpChannelParent] got asyncOpen msg");
}

}} 

