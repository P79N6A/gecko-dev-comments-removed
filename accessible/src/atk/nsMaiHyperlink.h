







































#ifndef __MAI_HYPERLINK_H__
#define __MAI_HYPERLINK_H__

#include "nsMai.h"
#include "nsIAccessibleHyperLink.h"

struct _AtkHyperlink;
typedef struct _AtkHyperlink                      AtkHyperlink;





class MaiHyperlink
{
public:
    MaiHyperlink(nsIAccessibleHyperLink *aAcc);
    ~MaiHyperlink();

public:
    AtkHyperlink *GetAtkHyperlink(void);
    nsIAccessibleHyperLink *GetAccHyperlink(void) {
        return mHyperlink;
    }

protected:
    nsIAccessibleHyperLink *mHyperlink;
    AtkHyperlink *mMaiAtkHyperlink;
public:
    static nsresult Initialize(AtkHyperlink *aObj, MaiHyperlink *aClass);
};
#endif 
