






































#ifndef __MAI_HYPERLINK_H__
#define __MAI_HYPERLINK_H__

#include "nsMai.h"
#include "nsAccessible.h"

struct _AtkHyperlink;
typedef struct _AtkHyperlink                      AtkHyperlink;





class MaiHyperlink
{
public:
    MaiHyperlink(nsAccessible* aHyperLink);
    ~MaiHyperlink();

public:
    AtkHyperlink *GetAtkHyperlink(void);
  nsAccessible* GetAccHyperlink()
  { return mHyperlink && mHyperlink->IsLink() ? mHyperlink : nsnull; }

protected:
    nsAccessible* mHyperlink;
    AtkHyperlink *mMaiAtkHyperlink;
public:
    static nsresult Initialize(AtkHyperlink *aObj, MaiHyperlink *aClass);
};
#endif 
