





#ifndef mozilla_dom_file_filecommon_h__
#define mozilla_dom_file_filecommon_h__

#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsDOMEventTargetHelper.h"
#include "nsDebug.h"
#include "nsString.h"
#include "nsTArray.h"

#define BEGIN_FILE_NAMESPACE \
  namespace mozilla { namespace dom { namespace file {
#define END_FILE_NAMESPACE \
  } /* namespace file */ } /* namespace dom */ } /* namespace mozilla */
#define USING_FILE_NAMESPACE \
  using namespace mozilla::dom::file;

#endif 
