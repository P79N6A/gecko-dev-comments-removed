










































































#ifndef DASHWEBMODPARSER_H_
#define DASHWEBMODPARSER_H_

#include "nsCOMPtr.h"
#include "IMPDParser.h"
#include "MPD.h"
#include "Period.h"
#include "AdaptationSet.h"
#include "Representation.h"
#include "SegmentBase.h"

class nsIDOMElement;

namespace mozilla {
namespace net {

class nsDASHWebMODParser : public IMPDParser
{
public:
  nsDASHWebMODParser(nsIDOMElement* aRoot);
  virtual ~nsDASHWebMODParser();

  
  MPD* Parse();

private:
  
  nsresult    VerifyMPDAttributes();

  
  nsresult    SetMPDBaseUrls(MPD* aMpd);

  
  nsresult    SetPeriods(MPD* aMpd);

  
  nsresult    SetAdaptationSets(nsIDOMElement* aPeriodNode,
                                Period* aPeriod,
                                bool &bIgnoreThisRep);

  
  nsresult    ValidateAdaptationSetAttributes(nsIDOMElement* aChild,
                                              bool &bAttributesValid);

  
  nsresult    SetRepresentations(nsIDOMElement* aAdaptationSetNode,
                                 AdaptationSet* aAdaptationSet,
                                 bool &bIgnoreThisRep);

  
  
  nsresult    SetRepresentationBaseUrls(nsIDOMElement* aRepNode,
                                        Representation* aRep,
                                        bool &bIgnoreThisRep);

  
  
  
  nsresult    SetRepSegmentBase(nsIDOMElement* aRepElem,
                                Representation* aRep,
                                bool &bIgnoreThisRep);
  
  
  nsresult    SetSegmentBaseInit(nsIDOMElement* aSegBaseElem,
                                 SegmentBase* aSegBase,
                                 bool &bIgnoreThisSegBase);

  
  nsresult    GetAttribute(nsIDOMElement* aElem,
                           const nsAString& aAttribute,
                           nsAString& aValue);

  
  nsresult    GetTime(nsAString& aTimeStr, double& aTime);

  
  
  nsCOMPtr<nsIDOMElement>  mRoot;
};

}
}

#endif 
