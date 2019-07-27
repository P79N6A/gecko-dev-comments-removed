





#ifndef __MAI_HYPERLINK_H__
#define __MAI_HYPERLINK_H__

#include "nsMai.h"
#include "Accessible.h"

struct _AtkHyperlink;
typedef struct _AtkHyperlink                      AtkHyperlink;

namespace mozilla {
namespace a11y {





class MaiHyperlink
{
public:
  explicit MaiHyperlink(uintptr_t aHyperLink);
  ~MaiHyperlink();

public:
  AtkHyperlink* GetAtkHyperlink() const { return mMaiAtkHyperlink; }
  Accessible* GetAccHyperlink()
    {
      if (!mHyperlink || mHyperlink & IS_PROXY)
        return nullptr;

      Accessible* link = reinterpret_cast<Accessible*>(mHyperlink);
      NS_ASSERTION(link->IsLink(), "Why isn't it a link!");
      return link;
    }

  ProxyAccessible* Proxy() const
  {
    if (!(mHyperlink & IS_PROXY))
      return nullptr;

    return reinterpret_cast<ProxyAccessible*>(mHyperlink & ~IS_PROXY);
  }

protected:
  uintptr_t mHyperlink;
  AtkHyperlink* mMaiAtkHyperlink;
};

} 
} 

#endif 
