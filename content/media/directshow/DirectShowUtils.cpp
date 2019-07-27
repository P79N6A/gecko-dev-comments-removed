




#include "dshow.h"
#include "dmodshow.h"
#include "wmcodecdsp.h"
#include "dmoreg.h"
#include "DirectShowUtils.h"
#include "nsAutoPtr.h"
#include "mozilla/ArrayUtils.h"
#include "mozilla/RefPtr.h"
#include "nsPrintfCString.h"

#define WARN(...) NS_WARNING(nsPrintfCString(__VA_ARGS__).get())

namespace mozilla {

#if defined(PR_LOGGING)




struct GuidToName {
  const char* name;
  const GUID guid;
};

#pragma push_macro("OUR_GUID_ENTRY")
#undef OUR_GUID_ENTRY
#define OUR_GUID_ENTRY(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
{ #name, {l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8} },

static const GuidToName GuidToNameTable[] = {
#include <uuids.h>
};

#pragma pop_macro("OUR_GUID_ENTRY")

const char*
GetDirectShowGuidName(const GUID& aGuid)
{
  const size_t len = ArrayLength(GuidToNameTable);
  for (unsigned i = 0; i < len; i++) {
    if (IsEqualGUID(aGuid, GuidToNameTable[i].guid)) {
      return GuidToNameTable[i].name;
    }
  }
  return "Unknown";
}
#endif 

void
RemoveGraphFromRunningObjectTable(DWORD aRotRegister)
{
  nsRefPtr<IRunningObjectTable> runningObjectTable;
  if (SUCCEEDED(GetRunningObjectTable(0, getter_AddRefs(runningObjectTable)))) {
    runningObjectTable->Revoke(aRotRegister);
  }
}

HRESULT
AddGraphToRunningObjectTable(IUnknown *aUnkGraph, DWORD *aOutRotRegister)
{
  HRESULT hr;

  nsRefPtr<IMoniker> moniker;
  nsRefPtr<IRunningObjectTable> runningObjectTable;

  hr = GetRunningObjectTable(0, getter_AddRefs(runningObjectTable));
  NS_ENSURE_TRUE(SUCCEEDED(hr), hr);

  const size_t STRING_LENGTH = 256;
  WCHAR wsz[STRING_LENGTH];

  StringCchPrintfW(wsz,
                   STRING_LENGTH,
                   L"FilterGraph %08x pid %08x",
                   (DWORD_PTR)aUnkGraph,
                   GetCurrentProcessId());

  hr = CreateItemMoniker(L"!", wsz, getter_AddRefs(moniker));
  NS_ENSURE_TRUE(SUCCEEDED(hr), hr);

  hr = runningObjectTable->Register(ROTFLAGS_REGISTRATIONKEEPSALIVE,
                                    aUnkGraph,
                                    moniker,
                                    aOutRotRegister);
  NS_ENSURE_TRUE(SUCCEEDED(hr), hr);

  return S_OK;
}

const char*
GetGraphNotifyString(long evCode)
{
#define CASE(x) case x: return #x
  switch(evCode) {
    CASE(EC_ACTIVATE); 
    CASE(EC_BANDWIDTHCHANGE); 
    CASE(EC_BUFFERING_DATA); 
    CASE(EC_BUILT); 
    CASE(EC_CLOCK_CHANGED); 
    CASE(EC_CLOCK_UNSET); 
    CASE(EC_CODECAPI_EVENT); 
    CASE(EC_COMPLETE); 
    CASE(EC_CONTENTPROPERTY_CHANGED); 
    CASE(EC_DEVICE_LOST); 
    CASE(EC_DISPLAY_CHANGED); 
    CASE(EC_END_OF_SEGMENT); 
    CASE(EC_EOS_SOON); 
    CASE(EC_ERROR_STILLPLAYING); 
    CASE(EC_ERRORABORT); 
    CASE(EC_ERRORABORTEX); 
    CASE(EC_EXTDEVICE_MODE_CHANGE); 
    CASE(EC_FILE_CLOSED); 
    CASE(EC_FULLSCREEN_LOST); 
    CASE(EC_GRAPH_CHANGED); 
    CASE(EC_LENGTH_CHANGED); 
    CASE(EC_LOADSTATUS); 
    CASE(EC_MARKER_HIT); 
    CASE(EC_NEED_RESTART); 
    CASE(EC_NEW_PIN); 
    CASE(EC_NOTIFY_WINDOW); 
    CASE(EC_OLE_EVENT); 
    CASE(EC_OPENING_FILE); 
    CASE(EC_PALETTE_CHANGED); 
    CASE(EC_PAUSED); 
    CASE(EC_PLEASE_REOPEN); 
    CASE(EC_PREPROCESS_COMPLETE); 
    CASE(EC_PROCESSING_LATENCY); 
    CASE(EC_QUALITY_CHANGE); 
    
    CASE(EC_REPAINT); 
    CASE(EC_SAMPLE_LATENCY); 
    
    CASE(EC_SCRUB_TIME); 
    CASE(EC_SEGMENT_STARTED); 
    CASE(EC_SHUTTING_DOWN); 
    CASE(EC_SNDDEV_IN_ERROR); 
    CASE(EC_SNDDEV_OUT_ERROR); 
    CASE(EC_STARVATION); 
    CASE(EC_STATE_CHANGE); 
    CASE(EC_STATUS); 
    CASE(EC_STEP_COMPLETE); 
    CASE(EC_STREAM_CONTROL_STARTED); 
    CASE(EC_STREAM_CONTROL_STOPPED); 
    CASE(EC_STREAM_ERROR_STILLPLAYING); 
    CASE(EC_STREAM_ERROR_STOPPED); 
    CASE(EC_TIMECODE_AVAILABLE); 
    CASE(EC_UNBUILT); 
    CASE(EC_USERABORT); 
    CASE(EC_VIDEO_SIZE_CHANGED); 
    CASE(EC_VIDEOFRAMEREADY); 
    CASE(EC_VMR_RECONNECTION_FAILED); 
    CASE(EC_VMR_RENDERDEVICE_SET); 
    CASE(EC_VMR_SURFACE_FLIPPED); 
    CASE(EC_WINDOW_DESTROYED); 
    CASE(EC_WMT_EVENT); 
    CASE(EC_WMT_INDEX_EVENT); 
    CASE(S_OK); 
    CASE(VFW_S_AUDIO_NOT_RENDERED); 
    CASE(VFW_S_DUPLICATE_NAME); 
    CASE(VFW_S_PARTIAL_RENDER); 
    CASE(VFW_S_VIDEO_NOT_RENDERED); 
    CASE(E_ABORT); 
    CASE(E_OUTOFMEMORY); 
    CASE(E_POINTER); 
    CASE(VFW_E_CANNOT_CONNECT); 
    CASE(VFW_E_CANNOT_RENDER); 
    CASE(VFW_E_NO_ACCEPTABLE_TYPES); 
    CASE(VFW_E_NOT_IN_GRAPH);

    default:
      return "Unknown Code";
  };
#undef CASE
}

HRESULT
CreateAndAddFilter(IGraphBuilder* aGraph,
                   REFGUID aFilterClsId,
                   LPCWSTR aFilterName,
                   IBaseFilter **aOutFilter)
{
  NS_ENSURE_TRUE(aGraph, E_POINTER);
  NS_ENSURE_TRUE(aOutFilter, E_POINTER);
  HRESULT hr;

  nsRefPtr<IBaseFilter> filter;
  hr = CoCreateInstance(aFilterClsId,
                        nullptr,
                        CLSCTX_INPROC_SERVER,
                        IID_IBaseFilter,
                        getter_AddRefs(filter));
  if (FAILED(hr)) {
    
    WARN("CoCreateInstance failed, hr=%x", hr);
    return hr;
  }

  hr = aGraph->AddFilter(filter, aFilterName);
  if (FAILED(hr)) {
    WARN("AddFilter failed, hr=%x", hr);
    return hr;
  }

  filter.forget(aOutFilter);

  return S_OK;
}

HRESULT
AddMP3DMOWrapperFilter(IGraphBuilder* aGraph,
                       IBaseFilter **aOutFilter)
{
  NS_ENSURE_TRUE(aGraph, E_POINTER);
  NS_ENSURE_TRUE(aOutFilter, E_POINTER);
  HRESULT hr;

  
  nsRefPtr<IBaseFilter> filter;
  hr = CoCreateInstance(CLSID_DMOWrapperFilter,
                        nullptr,
                        CLSCTX_INPROC_SERVER,
                        IID_IBaseFilter,
                        getter_AddRefs(filter));
  if (FAILED(hr)) {
    WARN("CoCreateInstance failed, hr=%x", hr);
    return hr;
  }

  
  nsRefPtr<IDMOWrapperFilter> dmoWrapper;
  hr = filter->QueryInterface(IID_IDMOWrapperFilter,
                              getter_AddRefs(dmoWrapper));
  if (FAILED(hr)) {
    WARN("QueryInterface failed, hr=%x", hr);
    return hr;
  }

  hr = dmoWrapper->Init(CLSID_CMP3DecMediaObject, DMOCATEGORY_AUDIO_DECODER);
  if (FAILED(hr)) {
    
    
    
    WARN("dmoWrapper Init failed, hr=%x", hr);
    return hr;
  }

  
  hr = aGraph->AddFilter(filter, L"MP3 Decoder DMO");
  if (FAILED(hr)) {
    WARN("AddFilter failed, hr=%x", hr);
    return hr;
  }

  filter.forget(aOutFilter);

  return S_OK;
}


HRESULT
MatchUnconnectedPin(IPin* aPin,
                    PIN_DIRECTION aPinDir,
                    bool *aOutMatches)
{
  NS_ENSURE_TRUE(aPin, E_POINTER);
  NS_ENSURE_TRUE(aOutMatches, E_POINTER);

  
  RefPtr<IPin> peer;
  HRESULT hr = aPin->ConnectedTo(byRef(peer));
  if (hr != VFW_E_NOT_CONNECTED) {
    *aOutMatches = false;
    return hr;
  }

  
  PIN_DIRECTION pinDir;
  hr = aPin->QueryDirection(&pinDir);
  NS_ENSURE_TRUE(SUCCEEDED(hr), hr);

  *aOutMatches = (pinDir == aPinDir);
  return S_OK;
}


TemporaryRef<IPin>
GetUnconnectedPin(IBaseFilter* aFilter, PIN_DIRECTION aPinDir)
{
  RefPtr<IEnumPins> enumPins;

  HRESULT hr = aFilter->EnumPins(byRef(enumPins));
  NS_ENSURE_TRUE(SUCCEEDED(hr), nullptr);

  
  RefPtr<IPin> pin;
  while (S_OK == enumPins->Next(1, byRef(pin), nullptr)) {
    bool matches = FALSE;
    if (SUCCEEDED(MatchUnconnectedPin(pin, aPinDir, &matches)) &&
        matches) {
      return pin;
    }
  }

  return nullptr;
}

HRESULT
ConnectFilters(IGraphBuilder* aGraph,
               IBaseFilter* aOutputFilter,
               IBaseFilter* aInputFilter)
{
  RefPtr<IPin> output = GetUnconnectedPin(aOutputFilter, PINDIR_OUTPUT);
  NS_ENSURE_TRUE(output, E_FAIL);

  RefPtr<IPin> input = GetUnconnectedPin(aInputFilter, PINDIR_INPUT);
  NS_ENSURE_TRUE(output, E_FAIL);

  return aGraph->Connect(output, input);
}

} 


#undef WARN
