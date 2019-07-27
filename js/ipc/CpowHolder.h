






#ifndef mozilla_jsipc_CpowHolder_h__
#define mozilla_jsipc_CpowHolder_h__

#include "js/TypeDecls.h"

namespace mozilla {
namespace jsipc {

class CpowHolder
{
  public:
    virtual bool ToObject(JSContext* cx, JS::MutableHandle<JSObject*> objp) = 0;
};

} 
} 

#endif 
