




#ifndef __NS_SVGFEATURES_H__
#define __NS_SVGFEATURES_H__

#include "nsString.h"

class nsSVGFeatures
{
public:
  







  static bool
  HasFeature(nsISupports* aObject, const nsAString& aFeature);

  





  static bool
  HasExtension(const nsAString& aExtension);
};

#endif 
