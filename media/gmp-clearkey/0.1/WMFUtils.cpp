















#include "stdafx.h"

void LOG(wchar_t* format, ...)
{
  va_list args;
  va_start(args, format);

  WCHAR msg[MAX_PATH];
  if (FAILED(StringCbVPrintf(msg, sizeof(msg), format, args))) {
    return;
  }

  OutputDebugString(msg);
}


#ifdef TEST_DECODING

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

#endif

void dump(const uint8_t* data, uint32_t len, const char* filename)
{
  FILE* f = 0;
  fopen_s(&f, filename, "wb");
  fwrite(data, len, 1, f);
  fclose(f);
}

HRESULT
CreateMFT(const CLSID& clsid,
          const wchar_t* aDllName,
          CComPtr<IMFTransform>& aOutMFT)
{
  HMODULE module = ::GetModuleHandle(aDllName);
  if (!module) {
    LOG(L"Failed to get %S\n", aDllName);
    return E_FAIL;
  }

  typedef HRESULT (WINAPI* DllGetClassObjectFnPtr)(const CLSID& clsid,
                                                   const IID& iid,
                                                   void** object);

  DllGetClassObjectFnPtr GetClassObjPtr =
    reinterpret_cast<DllGetClassObjectFnPtr>(GetProcAddress(module, "DllGetClassObject"));
  if (!GetClassObjPtr) {
    LOG(L"Failed to get DllGetClassObject\n");
    return E_FAIL;
  }

  CComPtr<IClassFactory> classFactory;
  HRESULT hr = GetClassObjPtr(clsid,
                              __uuidof(IClassFactory),
                              reinterpret_cast<void**>(static_cast<IClassFactory**>(&classFactory)));
  if (FAILED(hr)) {
    LOG(L"Failed to get H264 IClassFactory\n");
    return E_FAIL;
  }

  hr = classFactory->CreateInstance(NULL,
                                    __uuidof(IMFTransform),
                                    reinterpret_cast<void**>(static_cast<IMFTransform**>(&aOutMFT)));
  if (FAILED(hr)) {
    LOG(L"Failed to get create MFT\n");
    return E_FAIL;
  }

  return S_OK;
}