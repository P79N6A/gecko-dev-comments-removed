





#ifndef mozilla_dom_file_DOMFileRequest_h
#define mozilla_dom_file_DOMFileRequest_h

#include "FileRequest.h"

class nsIDOMLockedFile;

BEGIN_FILE_NAMESPACE

class DOMFileRequest : public FileRequest
{
public:
  DOMFileRequest(nsPIDOMWindow* aWindow);

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  nsIDOMLockedFile* GetLockedFile() const;
  IMPL_EVENT_HANDLER(progress)
};

END_FILE_NAMESPACE

#endif
