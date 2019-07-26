





#ifndef mozilla_dom_file_domfilehandle_h__
#define mozilla_dom_file_domfilehandle_h__

#include "FileCommon.h"

#include "FileHandle.h"

BEGIN_FILE_NAMESPACE

class DOMFileHandle : public FileHandle
{
public:
  static already_AddRefed<DOMFileHandle>
  Create(nsPIDOMWindow* aWindow,
         nsIFileStorage* aFileStorage,
         nsIFile* aFile);

  virtual already_AddRefed<nsISupports>
  CreateStream(nsIFile* aFile, bool aReadOnly);

  virtual already_AddRefed<nsIDOMFile>
  CreateFileObject(LockedFile* aLockedFile, uint32_t aFileSize);

protected:
  DOMFileHandle()
  { }

  ~DOMFileHandle()
  { }
};

END_FILE_NAMESPACE

#endif 
