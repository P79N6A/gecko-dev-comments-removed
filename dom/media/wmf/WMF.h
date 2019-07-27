





#ifndef WMF_H_
#define WMF_H_

#if WINVER < _WIN32_WINNT_WIN7
#error \
You must include WMF.h before including mozilla headers, \
otherwise mozconfig.h will be included \
and that sets WINVER to WinXP, \
which makes Windows Media Foundation unavailable.
#endif

#pragma push_macro("WINVER")
#undef WINVER
#define WINVER _WIN32_WINNT_WIN7

#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mfobjects.h>
#include <ks.h>
#include <stdio.h>
#include <mferror.h>
#include <propvarutil.h>
#include <wmcodecdsp.h>
#include <d3d9.h>
#include <dxva2api.h>
#include <wmcodecdsp.h>
#include <codecapi.h>




#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif


#ifndef CLSID_CMSAACDecMFT
extern "C" const CLSID CLSID_CMSAACDecMFT;
#define WMF_MUST_DEFINE_AAC_MFT_CLSID
#endif

namespace mozilla {
namespace wmf {






HRESULT LoadDLLs();
HRESULT UnloadDLLs();




HRESULT MFStartup();

HRESULT MFShutdown();

HRESULT MFCreateAsyncResult(IUnknown *aUunkObject,
                            IMFAsyncCallback *aCallback,
                            IUnknown *aUnkState,
                            IMFAsyncResult **aOutAsyncResult);

HRESULT MFInvokeCallback(IMFAsyncResult *aAsyncResult);

HRESULT MFCreateMediaType(IMFMediaType **aOutMFType);

HRESULT MFCreateSourceReaderFromByteStream(IMFByteStream *aByteStream,
                                           IMFAttributes *aAttributes,
                                           IMFSourceReader **aOutSourceReader);

HRESULT PropVariantToUInt32(REFPROPVARIANT aPropvar, ULONG *aOutUL);

HRESULT PropVariantToInt64(REFPROPVARIANT aPropVar, LONGLONG *aOutLL);

HRESULT MFTGetInfo(CLSID aClsidMFT,
                   LPWSTR *aOutName,
                   MFT_REGISTER_TYPE_INFO **aOutInputTypes,
                   UINT32 *aOutNumInputTypes,
                   MFT_REGISTER_TYPE_INFO **aOutOutputTypes,
                   UINT32 *aOutNumOutputTypes,
                   IMFAttributes **aOutAttributes);

HRESULT MFGetStrideForBitmapInfoHeader(DWORD aFormat,
                                       DWORD aWidth,
                                       LONG *aOutStride);




HRESULT MFCreateSourceReaderFromURL(LPCWSTR aURL,
                                    IMFAttributes *aAttributes,
                                    IMFSourceReader **aSourceReader);

HRESULT MFCreateAttributes(IMFAttributes **ppMFAttributes, UINT32 cInitialSize);

HRESULT MFGetPluginControl(IMFPluginControl **aOutPluginControl);

HRESULT MFTEnumEx(GUID guidCategory,
                  UINT32 Flags,
                  const MFT_REGISTER_TYPE_INFO *pInputType,
                  const MFT_REGISTER_TYPE_INFO *pOutputType,
                  IMFActivate ***pppMFTActivate,
                  UINT32 *pcMFTActivate);

HRESULT MFGetService(IUnknown *punkObject,
                     REFGUID guidService,
                     REFIID riid,
                     LPVOID *ppvObject);

HRESULT DXVA2CreateDirect3DDeviceManager9(UINT *pResetToken,
                                          IDirect3DDeviceManager9 **ppDXVAManager);


HRESULT MFCreateDXGIDeviceManager(UINT *pResetToken, IMFDXGIDeviceManager **ppDXVAManager);

HRESULT MFCreateSample(IMFSample **ppIMFSample);

HRESULT MFCreateAlignedMemoryBuffer(DWORD cbMaxLength,
                                    DWORD fAlignmentFlags,
                                    IMFMediaBuffer **ppBuffer);

HRESULT MFCreateDXGISurfaceBuffer(REFIID riid,
                                  IUnknown *punkSurface,
                                  UINT uSubresourceIndex,
                                  BOOL fButtomUpWhenLinear,
                                  IMFMediaBuffer **ppBuffer);

} 
} 



#pragma pop_macro("WINVER")

#endif
