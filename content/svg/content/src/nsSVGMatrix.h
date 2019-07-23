





































#ifndef __NS_SVGMATRIX_H__
#define __NS_SVGMATRIX_H__

#include "nsIDOMSVGMatrix.h"
#include "cairo.h"

nsresult
NS_NewSVGMatrix(nsIDOMSVGMatrix** result,
                float a = 1.0f, float b = 0.0f,
                float c = 0.0f, float d = 1.0f,
                float e = 0.0f, float f = 0.0f);

#endif 
