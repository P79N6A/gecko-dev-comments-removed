




#ifndef MOZILLA_SVGFRAGMENTIDENTIFIER_H__
#define MOZILLA_SVGFRAGMENTIDENTIFIER_H__

#include "nsString.h"

class nsIDocument;
class nsSVGSVGElement;
class nsSVGViewElement;

namespace mozilla {





class SVGFragmentIdentifier
{
  
  SVGFragmentIdentifier() MOZ_DELETE;

public:
  



  static bool ProcessFragmentIdentifier(nsIDocument *aDocument,
                                        const nsAString &aAnchorName);

private:
 



  static bool ProcessSVGViewSpec(const nsAString &aViewSpec, nsSVGSVGElement *root);

  
  
  static void SaveOldPreserveAspectRatio(nsSVGSVGElement *root);
  static void RestoreOldPreserveAspectRatio(nsSVGSVGElement *root);
  static void SaveOldViewBox(nsSVGSVGElement *root);
  static void RestoreOldViewBox(nsSVGSVGElement *root);
  static void SaveOldZoomAndPan(nsSVGSVGElement *root);
  static void RestoreOldZoomAndPan(nsSVGSVGElement *root);
  static void ClearTransform(nsSVGSVGElement *root);
};

} 

#endif 
