





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
  explicit MaiHyperlink(Accessible* aHyperLink);
  ~MaiHyperlink();

public:
  AtkHyperlink* GetAtkHyperlink() const { return mMaiAtkHyperlink; }
  Accessible* GetAccHyperlink()
    { return mHyperlink && mHyperlink->IsLink() ? mHyperlink : nullptr; }

protected:
  Accessible* mHyperlink;
  AtkHyperlink* mMaiAtkHyperlink;
};

} 
} 

#endif 
