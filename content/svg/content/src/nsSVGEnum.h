





































#ifndef __NS_SVGENUM_H__
#define __NS_SVGENUM_H__

#include "nsISVGEnum.h"
#include "nsAString.h"
#include "nsIAtom.h"

struct nsSVGEnumMapping {
    nsIAtom **key;
    PRUint16 val;
};

nsresult
NS_NewSVGEnum(nsISVGEnum** result,
              PRUint16 value,
              struct nsSVGEnumMapping *mapping);

nsresult
NS_NewSVGEnum(nsISVGEnum** result,
              const nsAString &value,
              struct nsSVGEnumMapping *mapping);

#endif 


