



#ifndef BASE_BASE_DROP_TARGET_H_
#define BASE_BASE_DROP_TARGET_H_

#include <objidl.h>

#include "base/ref_counted.h"

struct IDropTargetHelper;










class BaseDropTarget : public IDropTarget {
 public:
  
  explicit BaseDropTarget(HWND hwnd);
  virtual ~BaseDropTarget();

  
  
  
  
  
  void set_suspend(bool suspend) { suspend_ = suspend; }

  
  HRESULT __stdcall DragEnter(IDataObject* data_object,
                              DWORD key_state,
                              POINTL cursor_position,
                              DWORD* effect);
  HRESULT __stdcall DragOver(DWORD key_state,
                             POINTL cursor_position,
                             DWORD* effect);
  HRESULT __stdcall DragLeave();
  HRESULT __stdcall Drop(IDataObject* data_object,
                         DWORD key_state,
                         POINTL cursor_position,
                         DWORD* effect);

  
  HRESULT __stdcall QueryInterface(const IID& iid, void** object);
  ULONG __stdcall AddRef();
  ULONG __stdcall Release();

 protected:
  
  HWND GetHWND() { return hwnd_; }

  
  
  
  
  virtual DWORD OnDragEnter(IDataObject* data_object,
                            DWORD key_state,
                            POINT cursor_position,
                            DWORD effect);

  
  
  
  
  virtual DWORD OnDragOver(IDataObject* data_object,
                           DWORD key_state,
                           POINT cursor_position,
                           DWORD effect);

  
  
  virtual void OnDragLeave(IDataObject* data_object);

  
  
  virtual DWORD OnDrop(IDataObject* data_object,
                       DWORD key_state,
                       POINT cursor_position,
                       DWORD effect);

  
  static int32 GetDragIdentity() { return drag_identity_; }

 private:
  
  
  static IDropTargetHelper* DropHelper();

  
  scoped_refptr<IDataObject> current_data_object_;

  
  
  
  
  
  
  
  
  static IDropTargetHelper* cached_drop_target_helper_;

  
  
  
  
  
  
  static int32 drag_identity_;

  
  
  HWND hwnd_;

  
  
  bool suspend_;

  LONG ref_count_;

  DISALLOW_EVIL_CONSTRUCTORS(BaseDropTarget);
};

#endif  
