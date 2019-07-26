





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
  MaiHyperlink(Accessible* aHyperLink);
  ~MaiHyperlink();

public:
  AtkHyperlink *GetAtkHyperlink(void);
  Accessible* GetAccHyperlink()
    { return mHyperlink && mHyperlink->IsLink() ? mHyperlink : nullptr; }

protected:
  Accessible* mHyperlink;
  AtkHyperlink* mMaiAtkHyperlink;
public:
  static nsresult Initialize(AtkHyperlink *aObj, MaiHyperlink *aClass);
};

} 
} 

#endif 
