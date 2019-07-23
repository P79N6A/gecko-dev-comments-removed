





































#ifndef __NS_SVGLENGTH_H__
#define __NS_SVGLENGTH_H__

#include "nsISVGLength.h"
#include "nsAString.h"

nsresult
NS_NewSVGLength(nsISVGLength** result,
                float value=0.0f,
                PRUint16 unit=nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER);

nsresult
NS_NewSVGLength(nsISVGLength** result,
                const nsAString &value);






#endif 
