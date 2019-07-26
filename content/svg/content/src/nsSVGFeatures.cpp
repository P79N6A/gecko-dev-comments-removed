













#include "nsSVGFeatures.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "mozilla/Preferences.h"

using namespace mozilla;

 bool
nsSVGFeatures::HasFeature(nsISupports* aObject, const nsAString& aFeature)
{
  if (aFeature.EqualsLiteral("http://www.w3.org/TR/SVG11/feature#Script")) {
    nsCOMPtr<nsIContent> content(do_QueryInterface(aObject));
    if (content) {
      nsIDocument *doc = content->GetCurrentDoc();
      if (doc && doc->IsResourceDoc()) {
        
        return false;
      }
    }
    return Preferences::GetBool("javascript.enabled", false);
  }
#define SVG_SUPPORTED_FEATURE(str) if (aFeature.EqualsLiteral(str)) return true;
#define SVG_UNSUPPORTED_FEATURE(str)
#include "nsSVGFeaturesList.h"
#undef SVG_SUPPORTED_FEATURE
#undef SVG_UNSUPPORTED_FEATURE
  return false;
}

 bool
nsSVGFeatures::HasExtension(const nsAString& aExtension)
{
#define SVG_SUPPORTED_EXTENSION(str) if (aExtension.EqualsLiteral(str)) return true;
  SVG_SUPPORTED_EXTENSION("http://www.w3.org/1999/xhtml")
  SVG_SUPPORTED_EXTENSION("http://www.w3.org/1998/Math/MathML")
#undef SVG_SUPPORTED_EXTENSION

  return false;
}
