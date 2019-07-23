



#ifndef BASE_BASE_DRAG_SOURCE_H_
#define BASE_BASE_DRAG_SOURCE_H_

#include <objidl.h>

#include "base/basictypes.h"










class BaseDragSource : public IDropSource {
 public:
  BaseDragSource();
  virtual ~BaseDragSource() { }

  
  HRESULT __stdcall QueryContinueDrag(BOOL escape_pressed, DWORD key_state);
  HRESULT __stdcall GiveFeedback(DWORD effect);

  
  HRESULT __stdcall QueryInterface(const IID& iid, void** object);
  ULONG __stdcall AddRef();
  ULONG __stdcall Release();

 protected:
  virtual void OnDragSourceCancel() { }
  virtual void OnDragSourceDrop() { }
  virtual void OnDragSourceMove() { }

 private:
  LONG ref_count_;

  DISALLOW_EVIL_CONSTRUCTORS(BaseDragSource);
};

#endif  
