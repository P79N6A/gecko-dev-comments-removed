




#ifndef MOZILLA_SVGFRAGMENTIDENTIFIER_H__
#define MOZILLA_SVGFRAGMENTIDENTIFIER_H__

#include "nsString.h"

class nsIDocument;
class nsSVGViewElement;

namespace mozilla {

namespace dom {
class SVGSVGElement;
}





class SVGFragmentIdentifier
{
  
  SVGFragmentIdentifier() MOZ_DELETE;

public:
  



  static bool ProcessFragmentIdentifier(nsIDocument *aDocument,
                                        const nsAString &aAnchorName);

private:
 



  static bool ProcessSVGViewSpec(const nsAString &aViewSpec, dom::SVGSVGElement *root);

  
  
  static void SaveOldPreserveAspectRatio(dom::SVGSVGElement *root);
  static void RestoreOldPreserveAspectRatio(dom::SVGSVGElement *root);
  static void SaveOldViewBox(dom::SVGSVGElement *root);
  static void RestoreOldViewBox(dom::SVGSVGElement *root);
  static void SaveOldZoomAndPan(dom::SVGSVGElement *root);
  static void RestoreOldZoomAndPan(dom::SVGSVGElement *root);
  static void ClearTransform(dom::SVGSVGElement *root);
};

} 

#endif 
