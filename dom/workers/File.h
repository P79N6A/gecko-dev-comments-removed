






































#ifndef mozilla_dom_workers_file_h__
#define mozilla_dom_workers_file_h__

#include "Workers.h"

#include "jspubtd.h"

class nsIDOMFile;
class nsIDOMBlob;

BEGIN_WORKERS_NAMESPACE

namespace file {

bool
InitClasses(JSContext* aCx, JSObject* aGlobal);

JSObject*
CreateBlob(JSContext* aCx, nsIDOMBlob* aBlob);

nsIDOMBlob*
GetDOMBlobFromJSObject(JSObject* aObj);

JSObject*
CreateFile(JSContext* aCx, nsIDOMFile* aFile);

nsIDOMFile*
GetDOMFileFromJSObject(JSObject* aObj);

} 

END_WORKERS_NAMESPACE

#endif 
