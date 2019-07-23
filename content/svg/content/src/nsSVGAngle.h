



































#ifndef __NS_SVGANGLE_H__
#define __NS_SVGANGLE_H__

#include "nsIDOMSVGAngle.h"
#include "nsAString.h"

nsresult
NS_NewSVGAngle(nsIDOMSVGAngle** result,
               float value=0.0f,
               PRUint16 unit=nsIDOMSVGAngle::SVG_ANGLETYPE_UNSPECIFIED);

nsresult
NS_NewSVGAngle(nsIDOMSVGAngle** result,
               const nsAString &value);

#endif 
