



#include "base/base_drag_source.h"




BaseDragSource::BaseDragSource() : ref_count_(0) {
}




HRESULT BaseDragSource::QueryContinueDrag(BOOL escape_pressed,
                                          DWORD key_state) {
  if (escape_pressed) {
    OnDragSourceCancel();
    return DRAGDROP_S_CANCEL;
  }

  if (!(key_state & MK_LBUTTON)) {
    OnDragSourceDrop();
    return DRAGDROP_S_DROP;
  }

  OnDragSourceMove();
  return S_OK;
}

HRESULT BaseDragSource::GiveFeedback(DWORD effect) {
  return DRAGDROP_S_USEDEFAULTCURSORS;
}




HRESULT BaseDragSource::QueryInterface(const IID& iid, void** object) {
  *object = NULL;
  if (IsEqualIID(iid, IID_IUnknown) || IsEqualIID(iid, IID_IDropSource)) {
    *object = this;
  } else {
    return E_NOINTERFACE;
  }
  AddRef();
  return S_OK;
}

ULONG BaseDragSource::AddRef() {
  return ++ref_count_;
}

ULONG BaseDragSource::Release() {
  if (--ref_count_ == 0) {
    delete this;
    return 0U;
  }
  return ref_count_;
}
