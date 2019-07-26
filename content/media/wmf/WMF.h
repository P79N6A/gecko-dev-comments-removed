





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
#include <stdio.h>
#include <mferror.h>
#include <comdef.h>
#include <propvarutil.h>
#include <wmcodecdsp.h>

#pragma comment(lib,"uuid.lib")
#pragma comment(lib,"mfuuid.lib")

_COM_SMARTPTR_TYPEDEF(IMFSourceReader, IID_IMFSourceReader);
_COM_SMARTPTR_TYPEDEF(IMFMediaType, IID_IMFMediaType);
_COM_SMARTPTR_TYPEDEF(IMFSample, IID_IMFSample);
_COM_SMARTPTR_TYPEDEF(IMFMediaBuffer, IID_IMFMediaBuffer);
_COM_SMARTPTR_TYPEDEF(IMFAsyncResult, IID_IMFAsyncResult);
_COM_SMARTPTR_TYPEDEF(IMF2DBuffer, IID_IMF2DBuffer);

namespace mozilla {
namespace wmf {






HRESULT LoadDLLs();
HRESULT UnloadDLLs();




HRESULT MFStartup();

HRESULT MFShutdown();

HRESULT MFPutWorkItem(DWORD aWorkQueueId,
                      IMFAsyncCallback *aCallback,
                      IUnknown *aState);

HRESULT MFAllocateWorkQueue(DWORD *aOutWorkQueueId);

HRESULT MFUnlockWorkQueue(DWORD aWorkQueueId);

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

} 
} 



#pragma pop_macro("WINVER")

#endif
