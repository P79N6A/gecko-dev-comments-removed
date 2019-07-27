





#ifndef mozilla_dom_FileRequest_h
#define mozilla_dom_FileRequest_h

#include "nscore.h"

namespace mozilla {
namespace dom {

class FileHelper;




class FileRequestBase
{
public:
  NS_IMETHOD_(MozExternalRefCountType)
  AddRef() = 0;

  NS_IMETHOD_(MozExternalRefCountType)
  Release() = 0;

  virtual void
  OnProgress(uint64_t aProgress, uint64_t aProgressMax) = 0;

  virtual nsresult
  NotifyHelperCompleted(FileHelper* aFileHelper) = 0;

protected:
  FileRequestBase();

  virtual ~FileRequestBase();
};

} 
} 

#endif 
