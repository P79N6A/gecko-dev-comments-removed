






#ifndef InkCollector_h__
#define InkCollector_h__

#include <msinkaut.h>
#include "StaticPtr.h"

#define MOZ_WM_PEN_LEAVES_HOVER_OF_DIGITIZER  WM_USER + 0x83

class InkCollector : public _IInkCollectorEvents
{
public:
  InkCollector();

  void Shutdown();
  void SetTarget(HWND aTargetWindow);

  static StaticRefPtr<InkCollector> sInkCollector;
  friend StaticRefPtr<InkCollector>;

protected:
  
  virtual ULONG STDMETHODCALLTYPE AddRef() { return ++mRefCount; }
  virtual ULONG STDMETHODCALLTYPE Release()
  {
    MOZ_ASSERT(mRefCount);
    if (!--mRefCount) {
      delete this;
      return 0;
    }
    return mRefCount;
  }
  HRESULT __stdcall QueryInterface(REFIID aRiid, void **aObject);

  
  STDMETHOD(GetTypeInfoCount)(UINT* aInfo) { return E_NOTIMPL; }
  STDMETHOD(GetTypeInfo)(UINT aInfo, LCID aId, ITypeInfo** aTInfo) { return E_NOTIMPL; }
  STDMETHOD(GetIDsOfNames)(REFIID aRiid, LPOLESTR* aStrNames, UINT aNames,
                           LCID aId, DISPID* aDispId) { return E_NOTIMPL; }
  STDMETHOD(Invoke)(DISPID aDispIdMember, REFIID aRiid,
                    LCID aId, WORD wFlags,
                    DISPPARAMS* aDispParams, VARIANT* aVarResult,
                    EXCEPINFO* aExcepInfo, UINT* aArgErr);

  
  virtual ~InkCollector();
  void Initialize();
  void OnInitialize();
  void Enable(bool aNewState);
  void CursorOutOfRange(IInkCursor* aCursor) const;
  bool IsHardProximityTablet(IInkTablet* aTablet) const;

private:
  nsRefPtr<IUnknown>          mMarshaller;
  nsRefPtr<IInkCollector>     mInkCollector;
  nsRefPtr<IConnectionPoint>  mConnectionPoint;

  uint32_t            mRefCount         = 0;
  DWORD               mCookie           = 0;
  HWND                mTargetWindow     = 0;
  bool                mComInitialized   = false;
  bool                mEnabled          = false;
};

#endif 
