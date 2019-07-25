



#ifndef mozilla_dom_workers_imagedata_h__
#define mozilla_dom_workers_imagedata_h__

#include "Workers.h"

BEGIN_WORKERS_NAMESPACE

namespace imagedata {

bool
InitClass(JSContext* aCx, JSObject* aGlobal);

JSObject*
Create(JSContext* aCx, uint32_t aWidth, uint32_t aHeight, JSObject* aData);






bool
IsImageData(JSObject* aObj);

uint32_t
GetWidth(JSObject* aObj);

uint32_t
GetHeight(JSObject* aObj);

JSObject*
GetData(JSObject* aObj);

} 

END_WORKERS_NAMESPACE

#endif 
