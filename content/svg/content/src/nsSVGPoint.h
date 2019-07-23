






































#ifndef __NS_SVGPOINT_H__
#define __NS_SVGPOINT_H__

#include "nsIDOMSVGPoint.h"
#include "gfxPoint.h"

nsresult
NS_NewSVGPoint(nsIDOMSVGPoint** result, float x = 0.0f, float y = 0.0f);

nsresult
NS_NewSVGPoint(nsIDOMSVGPoint** result, const gfxPoint& point);

nsresult
NS_NewSVGReadonlyPoint(nsIDOMSVGPoint** result, float x = 0.0f, float y = 0.0f);

#endif 
