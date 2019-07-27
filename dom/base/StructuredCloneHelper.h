




#ifndef mozilla_dom_StructuredCloneHelper_h
#define mozilla_dom_StructuredCloneHelper_h

#include "js/StructuredClone.h"
#include "nsAutoPtr.h"

namespace mozilla {
namespace dom {

class StructuredCloneHelperInternal
{
public:
  
  

  virtual JSObject* ReadCallback(JSContext* aCx,
                                 JSStructuredCloneReader* aReader,
                                 uint32_t aTag,
                                 uint32_t aIndex) = 0;

  virtual bool WriteCallback(JSContext* aCx,
                             JSStructuredCloneWriter* aWriter,
                             JS::Handle<JSObject*> aObj) = 0;

  
  

  virtual bool
  ReadTransferCallback(JSContext* aCx,
                       JSStructuredCloneReader* aReader,
                       uint32_t aTag,
                       void* aContent,
                       uint64_t aExtraData,
                       JS::MutableHandleObject aReturnObject);

  virtual bool
  WriteTransferCallback(JSContext* aCx,
                        JS::Handle<JSObject*> aObj,
                        
                        uint32_t* aTag,
                        JS::TransferableOwnership* aOwnership,
                        void** aContent,
                        uint64_t* aExtraData);

  virtual void
  FreeTransferCallback(uint32_t aTag,
                       JS::TransferableOwnership aOwnership,
                       void* aContent,
                       uint64_t aExtraData);

  

  bool Write(JSContext* aCx,
             JS::Handle<JS::Value> aValue);

  bool Write(JSContext* aCx,
             JS::Handle<JS::Value> aValue,
             JS::Handle<JS::Value> aTransfer);

  bool Read(JSContext* aCx,
            JS::MutableHandle<JS::Value> aValue);

protected:
  nsAutoPtr<JSAutoStructuredCloneBuffer> mBuffer;
};

} 
} 

#endif 
