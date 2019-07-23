




































#include "nsSMILTimeValueSpec.h"
#include "nsSMILTimeValue.h"
#include "nsSMILTimedElement.h"
#include "nsSMILInstanceTime.h"
#include "nsSMILParserUtils.h"
#include "nsString.h"
#include "nsAutoPtr.h"




already_AddRefed<nsSMILTimeValueSpec>
NS_NewSMILTimeValueSpec(nsSMILTimedElement* aOwner,
                        PRBool aIsBegin,
                        const nsAString& aStringSpec)
{
  nsSMILTimeValueSpec* result = new nsSMILTimeValueSpec(aOwner, aIsBegin);
  NS_ENSURE_TRUE(result, nsnull);

  NS_ADDREF(result); 
  nsresult rv = result->SetSpec(aStringSpec);
  if (NS_FAILED(rv)) {
    NS_RELEASE(result);
    return nsnull;
  }
  return result;
}

nsSMILTimeValueSpec::nsSMILTimeValueSpec(nsSMILTimedElement* aOwner,
                                         PRBool aIsBegin)
  : mOwner(aOwner),
    mIsBegin(aIsBegin),
    mOffset() 
{
}




NS_IMPL_ISUPPORTS1(nsSMILTimeValueSpec,
                   nsISupports)




nsresult
nsSMILTimeValueSpec::SetSpec(const nsAString& aStringSpec)
{
  
  nsSMILTimeValue clockTime;
  nsresult rv = nsSMILParserUtils::ParseClockValue(aStringSpec, &clockTime,
                              nsSMILParserUtils::kClockValueAllowSign
                              | nsSMILParserUtils::kClockValueAllowIndefinite);

  if (NS_FAILED(rv) || (!clockTime.IsResolved() && !clockTime.IsIndefinite()))
    return NS_ERROR_FAILURE;

  if (clockTime.IsResolved())
    mOffset = clockTime.GetMillis();

  if (mOwner) {
    nsSMILInstanceTime instance(clockTime, this);
    mOwner->AddInstanceTime(instance, mIsBegin);
  }

  return rv;
}
