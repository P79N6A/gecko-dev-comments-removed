















#include "WMFUtils.h"

#include <stdio.h>

#define INITGUID
#include <guiddef.h>

#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "wmcodecdspuuid")
#pragma comment(lib, "mfplat.lib")

void LOG(const char* format, ...)
{
#ifdef WMF_DECODER_LOG
  va_list args;
  va_start(args, format);
  vprintf(format, args);
#endif
}

#ifdef WMF_MUST_DEFINE_AAC_MFT_CLSID


DEFINE_GUID(CLSID_CMSAACDecMFT, 0x32D186A7, 0x218F, 0x4C75, 0x88, 0x76, 0xDD, 0x77, 0x27, 0x3A, 0x89, 0x99);
#endif

namespace wmf {


#define MFPLAT_FUNC(_func) \
  decltype(::_func)* _func;
#include "WMFSymbols.h"
#undef MFPLAT_FUNC

static bool
LinkMfplat()
{
  static bool sInitDone = false;
  static bool sInitOk = false;
  if (!sInitDone) {
    sInitDone = true;
    auto handle = GetModuleHandleA("mfplat.dll");
#define MFPLAT_FUNC(_func) \
    if (!(_func = (decltype(_func))(GetProcAddress(handle, #_func)))) { \
      return false; \
    }
#include "WMFSymbols.h"
#undef MFPLAT_FUNC
    sInitOk = true;
  }
  return sInitOk;
}

bool
EnsureLibs()
{
  static bool sInitDone = false;
  static bool sInitOk = false;
  if (!sInitDone) {
    
    
    
    sInitOk = LinkMfplat() &&
      (!!GetModuleHandleA("msauddecmft.dll") || !!GetModuleHandleA("msmpeg2adec.dll")) &&
      !!GetModuleHandleA("msmpeg2vdec.dll");
    sInitDone = true;
  }
  return sInitOk;
}

int32_t
MFOffsetToInt32(const MFOffset& aOffset)
{
  return int32_t(aOffset.value + (aOffset.fract / 65536.0f));
}



HRESULT
GetPictureRegion(IMFMediaType* aMediaType, IntRect& aOutPictureRegion)
{
  
  
  BOOL panScan = MFGetAttributeUINT32(aMediaType, MF_MT_PAN_SCAN_ENABLED, FALSE);

  
  HRESULT hr = E_FAIL;
  MFVideoArea videoArea;
  memset(&videoArea, 0, sizeof(MFVideoArea));
  if (panScan) {
    hr = aMediaType->GetBlob(MF_MT_PAN_SCAN_APERTURE,
                             (UINT8*)&videoArea,
                             sizeof(MFVideoArea),
                             NULL);
  }

  
  
  if (!panScan || hr == MF_E_ATTRIBUTENOTFOUND) {
    hr = aMediaType->GetBlob(MF_MT_MINIMUM_DISPLAY_APERTURE,
                             (UINT8*)&videoArea,
                             sizeof(MFVideoArea),
                             NULL);
  }

  if (hr == MF_E_ATTRIBUTENOTFOUND) {
    
    
    hr = aMediaType->GetBlob(MF_MT_GEOMETRIC_APERTURE,
                             (UINT8*)&videoArea,
                             sizeof(MFVideoArea),
                             NULL);
  }

  if (SUCCEEDED(hr)) {
    
    aOutPictureRegion = IntRect(MFOffsetToInt32(videoArea.OffsetX),
                                  MFOffsetToInt32(videoArea.OffsetY),
                                  videoArea.Area.cx,
                                  videoArea.Area.cy);
    return S_OK;
  }

  
  UINT32 width = 0, height = 0;
  hr = MFGetAttributeSize(aMediaType, MF_MT_FRAME_SIZE, &width, &height);
  ENSURE(SUCCEEDED(hr), hr);
  aOutPictureRegion = IntRect(0, 0, width, height);
  return S_OK;
}


HRESULT
GetDefaultStride(IMFMediaType *aType, uint32_t* aOutStride)
{
  
  HRESULT hr = aType->GetUINT32(MF_MT_DEFAULT_STRIDE, aOutStride);
  if (SUCCEEDED(hr)) {
    return S_OK;
  }

  
  GUID subtype = GUID_NULL;
  uint32_t width = 0;
  uint32_t height = 0;

  hr = aType->GetGUID(MF_MT_SUBTYPE, &subtype);
  ENSURE(SUCCEEDED(hr), hr);

  hr = MFGetAttributeSize(aType, MF_MT_FRAME_SIZE, &width, &height);
  ENSURE(SUCCEEDED(hr), hr);

  hr = MFGetStrideForBitmapInfoHeader(subtype.Data1, width, (LONG*)(aOutStride));
  ENSURE(SUCCEEDED(hr), hr);

  return hr;
}

void dump(const uint8_t* data, uint32_t len, const char* filename)
{
  FILE* f = 0;
  fopen_s(&f, filename, "wb");
  fwrite(data, len, 1, f);
  fclose(f);
}

HRESULT
CreateMFT(const CLSID& clsid,
          const char* aDllName,
          CComPtr<IMFTransform>& aOutMFT)
{
  HMODULE module = ::GetModuleHandleA(aDllName);
  if (!module) {
    LOG("Failed to get %S\n", aDllName);
    return E_FAIL;
  }

  typedef HRESULT (WINAPI* DllGetClassObjectFnPtr)(const CLSID& clsid,
                                                   const IID& iid,
                                                   void** object);

  DllGetClassObjectFnPtr GetClassObjPtr =
    reinterpret_cast<DllGetClassObjectFnPtr>(GetProcAddress(module, "DllGetClassObject"));
  if (!GetClassObjPtr) {
    LOG("Failed to get DllGetClassObject\n");
    return E_FAIL;
  }

  CComPtr<IClassFactory> classFactory;
  HRESULT hr = GetClassObjPtr(clsid,
                              __uuidof(IClassFactory),
                              reinterpret_cast<void**>(static_cast<IClassFactory**>(&classFactory)));
  if (FAILED(hr)) {
    LOG("Failed to get H264 IClassFactory\n");
    return E_FAIL;
  }

  hr = classFactory->CreateInstance(NULL,
                                    __uuidof(IMFTransform),
                                    reinterpret_cast<void**>(static_cast<IMFTransform**>(&aOutMFT)));
  if (FAILED(hr)) {
    LOG("Failed to get create MFT\n");
    return E_FAIL;
  }

  return S_OK;
}

} 
