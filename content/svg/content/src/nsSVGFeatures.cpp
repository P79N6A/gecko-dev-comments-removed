






































#include "nsString.h"
#include "nsSVGUtils.h"
#include "nsGkAtoms.h"


PRBool
NS_SVG_TestFeature(const nsAString& fstr) {
  if (!NS_SVGEnabled()) {
    return PR_FALSE;
  }
  nsAutoString lstr(fstr);
  lstr.StripWhitespace();

#ifdef DEBUG_scooter
  NS_ConvertUTF16toUTF8 feature(lstr);
  printf("NS_SVG_TestFeature: testing for %s\n", feature.get());
#endif

#define SVG_SUPPORTED_FEATURE(str) if (lstr.Equals(NS_LITERAL_STRING(str).get())) return PR_TRUE;
#define SVG_UNSUPPORTED_FEATURE(str)
#include "nsSVGFeaturesList.h"
#undef SVG_SUPPORTED_FEATURE
#undef SVG_UNSUPPORTED_FEATURE
  return PR_FALSE;
}


PRBool
NS_SVG_TestFeatures(const nsAString& fstr) {
  nsAutoString lstr(fstr);
  
  PRInt32 vbegin = 0;
  PRInt32 vlen = lstr.Length();
  while (vbegin < vlen) {
    PRInt32 vend = lstr.FindChar(PRUnichar(' '), vbegin);
    if (vend == kNotFound) {
      vend = vlen;
    }
    if (NS_SVG_TestFeature(Substring(lstr, vbegin, vend-vbegin)) == PR_FALSE) {
      return PR_FALSE;
    }
    vbegin = vend+1;
  }
  return PR_TRUE;
}


static PRBool
NS_SVG_Conditional(const nsIAtom *atom, PRUint16 cond) {

#define SVG_ELEMENT(_atom, _supports) if (atom == nsGkAtoms::_atom) return (_supports & cond) != 0;
#include "nsSVGElementList.h"
#undef SVG_ELEMENT
  return PR_FALSE;
}

PRBool
NS_SVG_TestsSupported(const nsIAtom *atom) {
  return NS_SVG_Conditional(atom, SUPPORTS_TEST);
}

PRBool
NS_SVG_LangSupported(const nsIAtom *atom) {
  return NS_SVG_Conditional(atom, SUPPORTS_LANG);
}

#if 0

PRBool
NS_SVG_ExternalSupported(const nsIAtom *atom) {
  return NS_SVG_Conditional(atom, SUPPORTS_EXTERNAL);
}
#endif
