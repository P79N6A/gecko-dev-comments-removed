





































#ifndef mozilla_places_Helpers_h_
#define mozilla_places_Helpers_h_





#include "mozIStorageStatementCallback.h"

namespace mozilla {
namespace places {




class AsyncStatementCallback : public mozIStorageStatementCallback
{
public:
  
  NS_IMETHOD HandleError(mozIStorageError *aError);
};





#define NS_DECL_ASYNCSTATEMENTCALLBACK \
  NS_IMETHOD HandleResult(mozIStorageResultSet *); \
  NS_IMETHOD HandleCompletion(PRUint16);

} 
} 

#endif 
