







































#include "mozilla/net/HttpChannelParent.h"

namespace mozilla {
namespace net {


HttpChannelParent::HttpChannelParent()
{
}

HttpChannelParent::~HttpChannelParent()
{
}



bool HttpChannelParent::RecvasyncOpen(const nsCString& uri)
{
  puts("[HttpChannelParent] got asyncOpen msg");
  return true;
}

}} 

