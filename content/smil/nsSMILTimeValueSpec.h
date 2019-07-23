




































#ifndef NS_SMILTIMEVALUESPEC_H_
#define NS_SMILTIMEVALUESPEC_H_

#include "nsISupports.h"
#include "nsCOMPtr.h"
#include "nsSMILTypes.h"

class nsAString;
class nsSMILTimeValue;
class nsSMILTimedElement;













#define NS_SMILTIMEVALUESPEC_IID \
{ 0x39d2f376, 0x6bda, 0x42c0, { 0x85, 0x10, 0xa9, 0x3b, 0x24, 0x82, 0x8a, 0x80 } }

class nsSMILTimeValueSpec : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_SMILTIMEVALUESPEC_IID)
  NS_DECL_ISUPPORTS

protected:
  nsSMILTimeValueSpec(nsSMILTimedElement* aOwner, PRBool aIsBegin);

  friend already_AddRefed<nsSMILTimeValueSpec>
  NS_NewSMILTimeValueSpec(nsSMILTimedElement* aOwner,
                          PRBool aIsBegin,
                          const nsAString& aStringSpec);

  nsresult SetSpec(const nsAString& aStringSpec);

  nsSMILTimedElement* mOwner;
  PRPackedBool        mIsBegin;
  nsSMILTime          mOffset;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsSMILTimeValueSpec, NS_SMILTIMEVALUESPEC_IID)




already_AddRefed<nsSMILTimeValueSpec>
NS_NewSMILTimeValueSpec(nsSMILTimedElement* aOwner,
                        PRBool aIsBegin,
                        const nsAString& aStringSpec);

#endif 
