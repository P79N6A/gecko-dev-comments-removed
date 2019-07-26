








#include <stdint.h>
const PRTime gPreloadListExpirationTime = INT64_C(1387016108776000);

class nsSTSPreload
{
  public:
    const char *mHost;
    const bool mIncludeSubdomains;
};

static const nsSTSPreload kSTSPreloadList[] = {
  { "alpha.irccloud.com", false },
  { "haste.ch", true },
  { "passwd.io", true },
  { "piratenlogin.de", true },
};
