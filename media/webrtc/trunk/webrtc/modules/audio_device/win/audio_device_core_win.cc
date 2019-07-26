









#pragma warning(disable: 4995)  //  name was marked as #pragma deprecated

#if (_MSC_VER >= 1310) && (_MSC_VER < 1400)




#pragma message(">> INFO: Windows Core Audio is not supported in VS 2003")
#endif

#include "audio_device_config.h"

#if defined(WEBRTC_WINDOWS_CORE_AUDIO_BUILD)
#pragma message(">> INFO: WEBRTC_WINDOWS_CORE_AUDIO_BUILD is defined")
#else
#pragma message(">> INFO: WEBRTC_WINDOWS_CORE_AUDIO_BUILD is *not* defined")
#endif

#ifdef WEBRTC_WINDOWS_CORE_AUDIO_BUILD

#include "audio_device_core_win.h"

#include <assert.h>
#include <string.h>

#include <windows.h>
#include <comdef.h>
#include <dmo.h>
#include "Functiondiscoverykeys_devpkey.h"
#include <mmsystem.h>
#include <strsafe.h>
#include <uuids.h>

#include "audio_device_utility.h"
#include "system_wrappers/interface/sleep.h"
#include "trace.h"


#define EXIT_ON_ERROR(hres)    do { if (FAILED(hres)) goto Exit; } while(0)


#define SAFE_RELEASE(p)     do { if ((p)) { (p)->Release(); (p) = NULL; } } while(0)

#define ROUND(x) ((x) >=0 ? (int)((x) + 0.5) : (int)((x) - 0.5))


#define REFTIMES_PER_MILLISEC  10000

typedef struct tagTHREADNAME_INFO
{
   DWORD dwType;        
   LPCSTR szName;       
   DWORD dwThreadID;    
   DWORD dwFlags;       
} THREADNAME_INFO;

namespace webrtc {
namespace {

enum { COM_THREADING_MODEL = COINIT_MULTITHREADED };

enum
{
    kAecCaptureStreamIndex = 0,
    kAecRenderStreamIndex = 1
};







class MediaBufferImpl : public IMediaBuffer
{
public:
    explicit MediaBufferImpl(DWORD maxLength)
        : _data(new BYTE[maxLength]),
          _length(0),
          _maxLength(maxLength),
          _refCount(0)
    {}

    
    STDMETHOD(GetBufferAndLength(BYTE** ppBuffer, DWORD* pcbLength))
    {
        if (!ppBuffer || !pcbLength)
        {
            return E_POINTER;
        }

        *ppBuffer = _data;
        *pcbLength = _length;

        return S_OK;
    }

    STDMETHOD(GetMaxLength(DWORD* pcbMaxLength))
    {
        if (!pcbMaxLength)
        {
            return E_POINTER;
        }

        *pcbMaxLength = _maxLength;
        return S_OK;
    }

    STDMETHOD(SetLength(DWORD cbLength))
    {
        if (cbLength > _maxLength)
        {
            return E_INVALIDARG;
        }

        _length = cbLength;
        return S_OK;
    }

    
    STDMETHOD_(ULONG, AddRef())
    {
        return InterlockedIncrement(&_refCount);
    }

    STDMETHOD(QueryInterface(REFIID riid, void** ppv))
    {
        if (!ppv)
        {
            return E_POINTER;
        }
        else if (riid != IID_IMediaBuffer && riid != IID_IUnknown)
        {
            return E_NOINTERFACE;
        }

        *ppv = static_cast<IMediaBuffer*>(this);
        AddRef();
        return S_OK;
    }

    STDMETHOD_(ULONG, Release())
    {
        LONG refCount = InterlockedDecrement(&_refCount);
        if (refCount == 0)
        {
            delete this;
        }

        return refCount;
    }

private:
    ~MediaBufferImpl()
    {
        delete [] _data;
    }

    BYTE* _data;
    DWORD _length;
    const DWORD _maxLength;
    LONG _refCount;
};
}  









bool AudioDeviceWindowsCore::CoreAudioIsSupported()
{
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, -1, "%s", __FUNCTION__);

    bool MMDeviceIsAvailable(false);
    bool coreAudioIsSupported(false);

    HRESULT hr(S_OK);
    TCHAR buf[MAXERRORLENGTH];
    TCHAR errorText[MAXERRORLENGTH];

    
    
    
    
    OSVERSIONINFOEX osvi;
    DWORDLONG dwlConditionMask = 0;
    int op = VER_LESS_EQUAL;

    
    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    osvi.dwMajorVersion = 6;
    osvi.dwMinorVersion = 0;
    osvi.wServicePackMajor = 0;
    osvi.wServicePackMinor = 0;
    osvi.wProductType = VER_NT_WORKSTATION;

    
    VER_SET_CONDITION(dwlConditionMask, VER_MAJORVERSION, op);
    VER_SET_CONDITION(dwlConditionMask, VER_MINORVERSION, op);
    VER_SET_CONDITION(dwlConditionMask, VER_SERVICEPACKMAJOR, op);
    VER_SET_CONDITION(dwlConditionMask, VER_SERVICEPACKMINOR, op);
    VER_SET_CONDITION(dwlConditionMask, VER_PRODUCT_TYPE, VER_EQUAL);

    DWORD dwTypeMask = VER_MAJORVERSION | VER_MINORVERSION |
                       VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR |
                       VER_PRODUCT_TYPE;

    
    BOOL isVistaRTMorXP = VerifyVersionInfo(&osvi, dwTypeMask,
                                            dwlConditionMask);
    if (isVistaRTMorXP != 0)
    {
        WEBRTC_TRACE(kTraceStateInfo, kTraceAudioDevice, -1,
            "*** Windows Core Audio is only supported on Vista SP1 or later "
            "=> will revert to the Wave API ***");
        return false;
    }

    

    
    
    
    
    
    ScopedCOMInitializer comInit(ScopedCOMInitializer::kMTA);
    if (!comInit.succeeded()) {
      
      
      return false;
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    IMMDeviceEnumerator* pIMMD(NULL);
    const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
    const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);

    hr = CoCreateInstance(
            CLSID_MMDeviceEnumerator,   
            NULL,
            CLSCTX_ALL,
            IID_IMMDeviceEnumerator,    
            (void**)&pIMMD );

    if (FAILED(hr))
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, -1,
            "AudioDeviceWindowsCore::CoreAudioIsSupported() Failed to create the required COM object", hr);
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, -1,
            "AudioDeviceWindowsCore::CoreAudioIsSupported() CoCreateInstance(MMDeviceEnumerator) failed (hr=0x%x)", hr);

        const DWORD dwFlags = FORMAT_MESSAGE_FROM_SYSTEM |
                              FORMAT_MESSAGE_IGNORE_INSERTS;
        const DWORD dwLangID = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);

        
        
        DWORD messageLength = ::FormatMessageW(dwFlags,
                                               0,
                                               hr,
                                               dwLangID,
                                               errorText,
                                               MAXERRORLENGTH,
                                               NULL);

        assert(messageLength <= MAXERRORLENGTH);

        
        for (; messageLength && ::isspace(errorText[messageLength - 1]);
             --messageLength)
        {
            errorText[messageLength - 1] = '\0';
        }

        StringCchPrintf(buf, MAXERRORLENGTH, TEXT("Error details: "));
        StringCchCat(buf, MAXERRORLENGTH, errorText);
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, -1, "%S", buf);
    }
    else
    {
        MMDeviceIsAvailable = true;
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, -1,
            "AudioDeviceWindowsCore::CoreAudioIsSupported() CoCreateInstance(MMDeviceEnumerator) succeeded", hr);
        SAFE_RELEASE(pIMMD);
    }

    
    
    
    
    if (MMDeviceIsAvailable)
    {
        coreAudioIsSupported = false;

        AudioDeviceWindowsCore* p = new AudioDeviceWindowsCore(-1);
        if (p == NULL)
        {
            return false;
        }

        int ok(0);
        int temp_ok(0);
        bool available(false);

        ok |= p->Init();

        WebRtc_Word16 numDevsRec = p->RecordingDevices();
        for (WebRtc_UWord16 i = 0; i < numDevsRec; i++)
        {
            ok |= p->SetRecordingDevice(i);
            temp_ok = p->RecordingIsAvailable(available);
            ok |= temp_ok;
            ok |= (available == false);
            if (available)
            {
                ok |= p->InitMicrophone();
            }
            if (ok)
            {
                WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, -1,
                    "AudioDeviceWindowsCore::CoreAudioIsSupported() Failed to use Core Audio Recording for device id=%i", i);
            }
        }

        WebRtc_Word16 numDevsPlay = p->PlayoutDevices();
        for (WebRtc_UWord16 i = 0; i < numDevsPlay; i++)
        {
            ok |= p->SetPlayoutDevice(i);
            temp_ok = p->PlayoutIsAvailable(available);
            ok |= temp_ok;
            ok |= (available == false);
            if (available)
            {
                ok |= p->InitSpeaker();
            }
            if (ok)
            {
                WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, -1 ,
                    "AudioDeviceWindowsCore::CoreAudioIsSupported() Failed to use Core Audio Playout for device id=%i", i);
            }
        }

        ok |= p->Terminate();

        if (ok == 0)
        {
            coreAudioIsSupported = true;
        }

        delete p;
    }

    if (coreAudioIsSupported)
    {
        WEBRTC_TRACE(kTraceStateInfo, kTraceAudioDevice, -1, "*** Windows Core Audio is supported ***");
    }
    else
    {
        WEBRTC_TRACE(kTraceStateInfo, kTraceAudioDevice, -1, "*** Windows Core Audio is NOT supported => will revert to the Wave API ***");
    }

    return (coreAudioIsSupported);
}









AudioDeviceWindowsCore::AudioDeviceWindowsCore(const WebRtc_Word32 id) :
    _comInit(ScopedCOMInitializer::kMTA),
    _critSect(*CriticalSectionWrapper::CreateCriticalSection()),
    _volumeMutex(*CriticalSectionWrapper::CreateCriticalSection()),
    _id(id),
    _ptrAudioBuffer(NULL),
    _ptrEnumerator(NULL),
    _ptrRenderCollection(NULL),
    _ptrCaptureCollection(NULL),
    _ptrDeviceOut(NULL),
    _ptrDeviceIn(NULL),
    _ptrClientOut(NULL),
    _ptrClientIn(NULL),
    _ptrRenderClient(NULL),
    _ptrCaptureClient(NULL),
    _ptrCaptureVolume(NULL),
    _ptrRenderSimpleVolume(NULL),
    _dmo(NULL),
    _mediaBuffer(NULL),
    _builtInAecEnabled(false),
    _playAudioFrameSize(0),
    _playSampleRate(0),
    _playBlockSize(0),
    _playChannels(2),
    _sndCardPlayDelay(0),
    _sndCardRecDelay(0),
    _sampleDriftAt48kHz(0),
    _driftAccumulator(0),
    _writtenSamples(0),
    _readSamples(0),
    _playAcc(0),
    _recAudioFrameSize(0),
    _recSampleRate(0),
    _recBlockSize(0),
    _recChannels(2),
    _avrtLibrary(NULL),
    _winSupportAvrt(false),
    _hRenderSamplesReadyEvent(NULL),
    _hPlayThread(NULL),
    _hCaptureSamplesReadyEvent(NULL),
    _hRecThread(NULL),
    _hShutdownRenderEvent(NULL),
    _hShutdownCaptureEvent(NULL),
    _hRenderStartedEvent(NULL),
    _hCaptureStartedEvent(NULL),
    _hGetCaptureVolumeThread(NULL),
    _hSetCaptureVolumeThread(NULL),
    _hSetCaptureVolumeEvent(NULL),
    _hMmTask(NULL),
    _initialized(false),
    _recording(false),
    _playing(false),
    _recIsInitialized(false),
    _playIsInitialized(false),
    _speakerIsInitialized(false),
    _microphoneIsInitialized(false),
    _AGC(false),
    _playWarning(0),
    _playError(0),
    _recWarning(0),
    _recError(0),
    _playBufType(AudioDeviceModule::kAdaptiveBufferSize),
    _playBufDelay(80),
    _playBufDelayFixed(80),
    _usingInputDeviceIndex(false),
    _usingOutputDeviceIndex(false),
    _inputDevice(AudioDeviceModule::kDefaultCommunicationDevice),
    _outputDevice(AudioDeviceModule::kDefaultCommunicationDevice),
    _inputDeviceIndex(0),
    _outputDeviceIndex(0),
    _newMicLevel(0)
{
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, id, "%s created", __FUNCTION__);
    assert(_comInit.succeeded());

    
    if (!_avrtLibrary)
    {
        
        _avrtLibrary = LoadLibrary(TEXT("Avrt.dll"));
        if (_avrtLibrary)
        {
            
            
            WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "AudioDeviceWindowsCore::AudioDeviceWindowsCore() The Avrt DLL module is now loaded");

            _PAvRevertMmThreadCharacteristics = (PAvRevertMmThreadCharacteristics)GetProcAddress(_avrtLibrary, "AvRevertMmThreadCharacteristics");
            _PAvSetMmThreadCharacteristicsA = (PAvSetMmThreadCharacteristicsA)GetProcAddress(_avrtLibrary, "AvSetMmThreadCharacteristicsA");
            _PAvSetMmThreadPriority = (PAvSetMmThreadPriority)GetProcAddress(_avrtLibrary, "AvSetMmThreadPriority");

            if ( _PAvRevertMmThreadCharacteristics &&
                 _PAvSetMmThreadCharacteristicsA &&
                 _PAvSetMmThreadPriority)
            {
                WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "AudioDeviceWindowsCore::AudioDeviceWindowsCore() AvRevertMmThreadCharacteristics() is OK");
                WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "AudioDeviceWindowsCore::AudioDeviceWindowsCore() AvSetMmThreadCharacteristicsA() is OK");
                WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "AudioDeviceWindowsCore::AudioDeviceWindowsCore() AvSetMmThreadPriority() is OK");
                _winSupportAvrt = true;
            }
        }
    }

    
    
    
    
    
    _hRenderSamplesReadyEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    _hCaptureSamplesReadyEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    _hShutdownRenderEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    _hShutdownCaptureEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    _hRenderStartedEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    _hCaptureStartedEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    _hSetCaptureVolumeEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    _perfCounterFreq.QuadPart = 1;
    _perfCounterFactor = 0.0;
    _avgCPULoad = 0.0;

    
    _recChannelsPrioList[0] = 2;    
    _recChannelsPrioList[1] = 1;    

    
    _playChannelsPrioList[0] = 2;    
    _playChannelsPrioList[1] = 1;    

    HRESULT hr;

    
    

    
    
    
    CoCreateInstance(
      __uuidof(MMDeviceEnumerator),
      NULL,
      CLSCTX_ALL,
      __uuidof(IMMDeviceEnumerator),
      reinterpret_cast<void**>(&_ptrEnumerator));
    assert(NULL != _ptrEnumerator);

    
    {
        IMediaObject* ptrDMO = NULL;
        hr = CoCreateInstance(CLSID_CWMAudioAEC,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IMediaObject,
                              reinterpret_cast<void**>(&ptrDMO));
        if (FAILED(hr) || ptrDMO == NULL)
        {
            
            
            _builtInAecEnabled = false;
            _TraceCOMError(hr);
        }
        _dmo = ptrDMO;
        SAFE_RELEASE(ptrDMO);
    }
}





AudioDeviceWindowsCore::~AudioDeviceWindowsCore()
{
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, _id, "%s destroyed", __FUNCTION__);

    Terminate();

    
    
    SAFE_RELEASE(_ptrEnumerator);

    _ptrAudioBuffer = NULL;

    if (NULL != _hRenderSamplesReadyEvent)
    {
        CloseHandle(_hRenderSamplesReadyEvent);
        _hRenderSamplesReadyEvent = NULL;
    }

    if (NULL != _hCaptureSamplesReadyEvent)
    {
        CloseHandle(_hCaptureSamplesReadyEvent);
        _hCaptureSamplesReadyEvent = NULL;
    }

    if (NULL != _hRenderStartedEvent)
    {
        CloseHandle(_hRenderStartedEvent);
        _hRenderStartedEvent = NULL;
    }

    if (NULL != _hCaptureStartedEvent)
    {
        CloseHandle(_hCaptureStartedEvent);
        _hCaptureStartedEvent = NULL;
    }

    if (NULL != _hShutdownRenderEvent)
    {
        CloseHandle(_hShutdownRenderEvent);
        _hShutdownRenderEvent = NULL;
    }

    if (NULL != _hShutdownCaptureEvent)
    {
        CloseHandle(_hShutdownCaptureEvent);
        _hShutdownCaptureEvent = NULL;
    }

    if (NULL != _hSetCaptureVolumeEvent)
    {
        CloseHandle(_hSetCaptureVolumeEvent);
        _hSetCaptureVolumeEvent = NULL;
    }

    if (_avrtLibrary)
    {
        BOOL freeOK = FreeLibrary(_avrtLibrary);
        if (!freeOK)
        {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                "AudioDeviceWindowsCore::~AudioDeviceWindowsCore() failed to free the loaded Avrt DLL module correctly");
        }
        else
        {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                "AudioDeviceWindowsCore::~AudioDeviceWindowsCore() the Avrt DLL module is now unloaded");
        }
    }

    delete &_critSect;
    delete &_volumeMutex;
}









void AudioDeviceWindowsCore::AttachAudioBuffer(AudioDeviceBuffer* audioBuffer)
{

    _ptrAudioBuffer = audioBuffer;

    
    
    
    _ptrAudioBuffer->SetRecordingSampleRate(0);
    _ptrAudioBuffer->SetPlayoutSampleRate(0);
    _ptrAudioBuffer->SetRecordingChannels(0);
    _ptrAudioBuffer->SetPlayoutChannels(0);
}





WebRtc_Word32 AudioDeviceWindowsCore::ActiveAudioLayer(AudioDeviceModule::AudioLayer& audioLayer) const
{
    audioLayer = AudioDeviceModule::kWindowsCoreAudio;
    return 0;
}





WebRtc_Word32 AudioDeviceWindowsCore::Init()
{

    CriticalSectionScoped lock(&_critSect);

    if (_initialized)
    {
        return 0;
    }

    _playWarning = 0;
    _playError = 0;
    _recWarning = 0;
    _recError = 0;

    
    
    
    
    _EnumerateEndpointDevicesAll(eRender);
    _EnumerateEndpointDevicesAll(eCapture);

    _initialized = true;

    return 0;
}





WebRtc_Word32 AudioDeviceWindowsCore::Terminate()
{

    CriticalSectionScoped lock(&_critSect);

    if (!_initialized) {
        return 0;
    }

    _initialized = false;
    _speakerIsInitialized = false;
    _microphoneIsInitialized = false;
    _playing = false;
    _recording = false;

    SAFE_RELEASE(_ptrRenderCollection);
    SAFE_RELEASE(_ptrCaptureCollection);
    SAFE_RELEASE(_ptrDeviceOut);
    SAFE_RELEASE(_ptrDeviceIn);
    SAFE_RELEASE(_ptrClientOut);
    SAFE_RELEASE(_ptrClientIn);
    SAFE_RELEASE(_ptrRenderClient);
    SAFE_RELEASE(_ptrCaptureClient);
    SAFE_RELEASE(_ptrCaptureVolume);
    SAFE_RELEASE(_ptrRenderSimpleVolume);

    return 0;
}





bool AudioDeviceWindowsCore::Initialized() const
{
    return (_initialized);
}





WebRtc_Word32 AudioDeviceWindowsCore::SpeakerIsAvailable(bool& available)
{

    CriticalSectionScoped lock(&_critSect);

    if (_ptrDeviceOut == NULL)
    {
        return -1;
    }

    available = true;

    return 0;
}





WebRtc_Word32 AudioDeviceWindowsCore::InitSpeaker()
{

    CriticalSectionScoped lock(&_critSect);

    if (_playing)
    {
        return -1;
    }

    if (_ptrDeviceOut == NULL)
    {
        return -1;
    }

    if (_usingOutputDeviceIndex)
    {
        WebRtc_Word16 nDevices = PlayoutDevices();
        if (_outputDeviceIndex > (nDevices - 1))
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id, "current device selection is invalid => unable to initialize");
            return -1;
        }
    }

    WebRtc_Word32 ret(0);

    SAFE_RELEASE(_ptrDeviceOut);
    if (_usingOutputDeviceIndex)
    {
        
        ret = _GetListDevice(eRender, _outputDeviceIndex, &_ptrDeviceOut);
    }
    else
    {
        ERole role;
        (_outputDevice == AudioDeviceModule::kDefaultDevice) ? role = eConsole : role = eCommunications;
        
        ret = _GetDefaultDevice(eRender, role, &_ptrDeviceOut);
    }

    if (ret != 0 || (_ptrDeviceOut == NULL))
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id, "failed to initialize the rendering enpoint device");
        SAFE_RELEASE(_ptrDeviceOut);
        return -1;
    }

    IAudioSessionManager* pManager = NULL;
    ret = _ptrDeviceOut->Activate(__uuidof(IAudioSessionManager),
                                  CLSCTX_ALL,
                                  NULL,
                                  (void**)&pManager);
    if (ret != 0 || pManager == NULL)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                    "  failed to initialize the render manager");
        SAFE_RELEASE(pManager);
        return -1;
    }

    SAFE_RELEASE(_ptrRenderSimpleVolume);
    ret = pManager->GetSimpleAudioVolume(NULL, FALSE, &_ptrRenderSimpleVolume);
    if (ret != 0 || _ptrRenderSimpleVolume == NULL)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                    "  failed to initialize the render simple volume");
        SAFE_RELEASE(pManager);
        SAFE_RELEASE(_ptrRenderSimpleVolume);
        return -1;
    }
    SAFE_RELEASE(pManager);

    _speakerIsInitialized = true;

    return 0;
}





WebRtc_Word32 AudioDeviceWindowsCore::MicrophoneIsAvailable(bool& available)
{

    CriticalSectionScoped lock(&_critSect);

    if (_ptrDeviceIn == NULL)
    {
        return -1;
    }

    available = true;

    return 0;
}





WebRtc_Word32 AudioDeviceWindowsCore::InitMicrophone()
{

    CriticalSectionScoped lock(&_critSect);

    if (_recording)
    {
        return -1;
    }

    if (_ptrDeviceIn == NULL)
    {
        return -1;
    }

    if (_usingInputDeviceIndex)
    {
        WebRtc_Word16 nDevices = RecordingDevices();
        if (_inputDeviceIndex > (nDevices - 1))
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id, "current device selection is invalid => unable to initialize");
            return -1;
        }
    }

    WebRtc_Word32 ret(0);

    SAFE_RELEASE(_ptrDeviceIn);
    if (_usingInputDeviceIndex)
    {
        
        ret = _GetListDevice(eCapture, _inputDeviceIndex, &_ptrDeviceIn);
    }
    else
    {
        ERole role;
        (_inputDevice == AudioDeviceModule::kDefaultDevice) ? role = eConsole : role = eCommunications;
        
        ret = _GetDefaultDevice(eCapture, role, &_ptrDeviceIn);
    }

    if (ret != 0 || (_ptrDeviceIn == NULL))
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id, "failed to initialize the capturing enpoint device");
        SAFE_RELEASE(_ptrDeviceIn);
        return -1;
    }

    ret = _ptrDeviceIn->Activate(__uuidof(IAudioEndpointVolume),
                                 CLSCTX_ALL,
                                 NULL,
                                 reinterpret_cast<void **>(&_ptrCaptureVolume));
    if (ret != 0 || _ptrCaptureVolume == NULL)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                    "  failed to initialize the capture volume");
        SAFE_RELEASE(_ptrCaptureVolume);
        return -1;
    }

    _microphoneIsInitialized = true;

    return 0;
}





bool AudioDeviceWindowsCore::SpeakerIsInitialized() const
{

    return (_speakerIsInitialized);
}





bool AudioDeviceWindowsCore::MicrophoneIsInitialized() const
{

    return (_microphoneIsInitialized);
}





WebRtc_Word32 AudioDeviceWindowsCore::SpeakerVolumeIsAvailable(bool& available)
{

    CriticalSectionScoped lock(&_critSect);

    if (_ptrDeviceOut == NULL)
    {
        return -1;
    }

    HRESULT hr = S_OK;
    IAudioSessionManager* pManager = NULL;
    ISimpleAudioVolume* pVolume = NULL;

    hr = _ptrDeviceOut->Activate(__uuidof(IAudioSessionManager), CLSCTX_ALL, NULL, (void**)&pManager);
    EXIT_ON_ERROR(hr);

    hr = pManager->GetSimpleAudioVolume(NULL, FALSE, &pVolume);
    EXIT_ON_ERROR(hr);

    float volume(0.0f);
    hr = pVolume->GetMasterVolume(&volume);
    if (FAILED(hr))
    {
        available = false;
    }
    available = true;

    SAFE_RELEASE(pManager);
    SAFE_RELEASE(pVolume);

    return 0;

Exit:
    _TraceCOMError(hr);
    SAFE_RELEASE(pManager);
    SAFE_RELEASE(pVolume);
    return -1;
}





WebRtc_Word32 AudioDeviceWindowsCore::SetSpeakerVolume(WebRtc_UWord32 volume)
{

    {
        CriticalSectionScoped lock(&_critSect);

        if (!_speakerIsInitialized)
        {
        return -1;
        }

        if (_ptrDeviceOut == NULL)
        {
            return -1;
        }
    }

    if (volume < (WebRtc_UWord32)MIN_CORE_SPEAKER_VOLUME ||
        volume > (WebRtc_UWord32)MAX_CORE_SPEAKER_VOLUME)
    {
        return -1;
    }

    HRESULT hr = S_OK;

    
    const float fLevel = (float)volume/MAX_CORE_SPEAKER_VOLUME;
    _volumeMutex.Enter();
    hr = _ptrRenderSimpleVolume->SetMasterVolume(fLevel,NULL);
    _volumeMutex.Leave();
    EXIT_ON_ERROR(hr);

    return 0;

Exit:
    _TraceCOMError(hr);
    return -1;
}





WebRtc_Word32 AudioDeviceWindowsCore::SpeakerVolume(WebRtc_UWord32& volume) const
{

    {
        CriticalSectionScoped lock(&_critSect);

        if (!_speakerIsInitialized)
        {
            return -1;
        }

        if (_ptrDeviceOut == NULL)
        {
            return -1;
        }
    }

    HRESULT hr = S_OK;
    float fLevel(0.0f);

    _volumeMutex.Enter();
    hr = _ptrRenderSimpleVolume->GetMasterVolume(&fLevel);
    _volumeMutex.Leave();
    EXIT_ON_ERROR(hr);

    
    volume = static_cast<WebRtc_UWord32> (fLevel*MAX_CORE_SPEAKER_VOLUME);

    return 0;

Exit:
    _TraceCOMError(hr);
    return -1;
}





WebRtc_Word32 AudioDeviceWindowsCore::SetWaveOutVolume(WebRtc_UWord16 volumeLeft, WebRtc_UWord16 volumeRight)
{
    return -1;
}





WebRtc_Word32 AudioDeviceWindowsCore::WaveOutVolume(WebRtc_UWord16& volumeLeft, WebRtc_UWord16& volumeRight) const
{
    return -1;
}










WebRtc_Word32 AudioDeviceWindowsCore::MaxSpeakerVolume(WebRtc_UWord32& maxVolume) const
{

    if (!_speakerIsInitialized)
    {
        return -1;
    }

    maxVolume = static_cast<WebRtc_UWord32> (MAX_CORE_SPEAKER_VOLUME);

    return 0;
}





WebRtc_Word32 AudioDeviceWindowsCore::MinSpeakerVolume(WebRtc_UWord32& minVolume) const
{

    if (!_speakerIsInitialized)
    {
        return -1;
    }

    minVolume = static_cast<WebRtc_UWord32> (MIN_CORE_SPEAKER_VOLUME);

    return 0;
}





WebRtc_Word32 AudioDeviceWindowsCore::SpeakerVolumeStepSize(WebRtc_UWord16& stepSize) const
{

    if (!_speakerIsInitialized)
    {
        return -1;
    }

    stepSize = CORE_SPEAKER_VOLUME_STEP_SIZE;

    return 0;
}





WebRtc_Word32 AudioDeviceWindowsCore::SpeakerMuteIsAvailable(bool& available)
{

    CriticalSectionScoped lock(&_critSect);

    if (_ptrDeviceOut == NULL)
    {
        return -1;
    }

    HRESULT hr = S_OK;
    IAudioEndpointVolume* pVolume = NULL;

    
    hr = _ptrDeviceOut->Activate(__uuidof(IAudioEndpointVolume),
        CLSCTX_ALL, NULL,  reinterpret_cast<void**>(&pVolume));
    EXIT_ON_ERROR(hr);

    BOOL mute;
    hr = pVolume->GetMute(&mute);
    if (FAILED(hr))
        available = false;
    else
        available = true;

    SAFE_RELEASE(pVolume);

    return 0;

Exit:
    _TraceCOMError(hr);
    SAFE_RELEASE(pVolume);
    return -1;
}





WebRtc_Word32 AudioDeviceWindowsCore::SetSpeakerMute(bool enable)
{

    CriticalSectionScoped lock(&_critSect);

    if (!_speakerIsInitialized)
    {
        return -1;
    }

    if (_ptrDeviceOut == NULL)
    {
        return -1;
    }

    HRESULT hr = S_OK;
    IAudioEndpointVolume* pVolume = NULL;

    
    hr = _ptrDeviceOut->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL,  reinterpret_cast<void**>(&pVolume));
    EXIT_ON_ERROR(hr);

    const BOOL mute(enable);
    hr = pVolume->SetMute(mute, NULL);
    EXIT_ON_ERROR(hr);

    SAFE_RELEASE(pVolume);

    return 0;

Exit:
    _TraceCOMError(hr);
    SAFE_RELEASE(pVolume);
    return -1;
}





WebRtc_Word32 AudioDeviceWindowsCore::SpeakerMute(bool& enabled) const
{

    if (!_speakerIsInitialized)
    {
        return -1;
    }

    if (_ptrDeviceOut == NULL)
    {
        return -1;
    }

    HRESULT hr = S_OK;
    IAudioEndpointVolume* pVolume = NULL;

    
    hr = _ptrDeviceOut->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL,  reinterpret_cast<void**>(&pVolume));
    EXIT_ON_ERROR(hr);

    BOOL mute;
    hr = pVolume->GetMute(&mute);
    EXIT_ON_ERROR(hr);

    enabled = (mute == TRUE) ? true : false;

    SAFE_RELEASE(pVolume);

    return 0;

Exit:
    _TraceCOMError(hr);
    SAFE_RELEASE(pVolume);
    return -1;
}





WebRtc_Word32 AudioDeviceWindowsCore::MicrophoneMuteIsAvailable(bool& available)
{

    CriticalSectionScoped lock(&_critSect);

    if (_ptrDeviceIn == NULL)
    {
        return -1;
    }

    HRESULT hr = S_OK;
    IAudioEndpointVolume* pVolume = NULL;

    
    hr = _ptrDeviceIn->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL,  reinterpret_cast<void**>(&pVolume));
    EXIT_ON_ERROR(hr);

    BOOL mute;
    hr = pVolume->GetMute(&mute);
    if (FAILED(hr))
        available = false;
    else
        available = true;

    SAFE_RELEASE(pVolume);
    return 0;

Exit:
    _TraceCOMError(hr);
    SAFE_RELEASE(pVolume);
    return -1;
}





WebRtc_Word32 AudioDeviceWindowsCore::SetMicrophoneMute(bool enable)
{

    if (!_microphoneIsInitialized)
    {
        return -1;
    }

    if (_ptrDeviceIn == NULL)
    {
        return -1;
    }

    HRESULT hr = S_OK;
    IAudioEndpointVolume* pVolume = NULL;

    
    hr = _ptrDeviceIn->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL,  reinterpret_cast<void**>(&pVolume));
    EXIT_ON_ERROR(hr);

    const BOOL mute(enable);
    hr = pVolume->SetMute(mute, NULL);
    EXIT_ON_ERROR(hr);

    SAFE_RELEASE(pVolume);
    return 0;

Exit:
    _TraceCOMError(hr);
    SAFE_RELEASE(pVolume);
    return -1;
}





WebRtc_Word32 AudioDeviceWindowsCore::MicrophoneMute(bool& enabled) const
{

    if (!_microphoneIsInitialized)
    {
        return -1;
    }

    HRESULT hr = S_OK;
    IAudioEndpointVolume* pVolume = NULL;

    
    hr = _ptrDeviceIn->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL,  reinterpret_cast<void**>(&pVolume));
    EXIT_ON_ERROR(hr);

    BOOL mute;
    hr = pVolume->GetMute(&mute);
    EXIT_ON_ERROR(hr);

    enabled = (mute == TRUE) ? true : false;

    SAFE_RELEASE(pVolume);
    return 0;

Exit:
    _TraceCOMError(hr);
    SAFE_RELEASE(pVolume);
    return -1;
}





WebRtc_Word32 AudioDeviceWindowsCore::MicrophoneBoostIsAvailable(bool& available)
{

    available = false;
    return 0;
}





WebRtc_Word32 AudioDeviceWindowsCore::SetMicrophoneBoost(bool enable)
{

    if (!_microphoneIsInitialized)
    {
        return -1;
    }

    return -1;
}





WebRtc_Word32 AudioDeviceWindowsCore::MicrophoneBoost(bool& enabled) const
{

    if (!_microphoneIsInitialized)
    {
        return -1;
    }

    return -1;
}





WebRtc_Word32 AudioDeviceWindowsCore::StereoRecordingIsAvailable(bool& available)
{

    available = true;
    return 0;
}





WebRtc_Word32 AudioDeviceWindowsCore::SetStereoRecording(bool enable)
{

    CriticalSectionScoped lock(&_critSect);

    if (enable)
    {
        _recChannelsPrioList[0] = 2;    
        _recChannelsPrioList[1] = 1;
        _recChannels = 2;
    }
    else
    {
        _recChannelsPrioList[0] = 1;    
        _recChannelsPrioList[1] = 2;
        _recChannels = 1;
    }

    return 0;
}





WebRtc_Word32 AudioDeviceWindowsCore::StereoRecording(bool& enabled) const
{

    if (_recChannels == 2)
        enabled = true;
    else
        enabled = false;

    return 0;
}





WebRtc_Word32 AudioDeviceWindowsCore::StereoPlayoutIsAvailable(bool& available)
{

    available = true;
    return 0;
}





WebRtc_Word32 AudioDeviceWindowsCore::SetStereoPlayout(bool enable)
{

    CriticalSectionScoped lock(&_critSect);

    if (enable)
    {
        _playChannelsPrioList[0] = 2;    
        _playChannelsPrioList[1] = 1;
        _playChannels = 2;
    }
    else
    {
        _playChannelsPrioList[0] = 1;    
        _playChannelsPrioList[1] = 2;
        _playChannels = 1;
    }

    return 0;
}





WebRtc_Word32 AudioDeviceWindowsCore::StereoPlayout(bool& enabled) const
{

    if (_playChannels == 2)
        enabled = true;
    else
        enabled = false;

    return 0;
}





WebRtc_Word32 AudioDeviceWindowsCore::SetAGC(bool enable)
{
    CriticalSectionScoped lock(&_critSect);
    _AGC = enable;
    return 0;
}





bool AudioDeviceWindowsCore::AGC() const
{
    CriticalSectionScoped lock(&_critSect);
    return _AGC;
}





WebRtc_Word32 AudioDeviceWindowsCore::MicrophoneVolumeIsAvailable(bool& available)
{

    CriticalSectionScoped lock(&_critSect);

    if (_ptrDeviceIn == NULL)
    {
        return -1;
    }

    HRESULT hr = S_OK;
    IAudioEndpointVolume* pVolume = NULL;

    hr = _ptrDeviceIn->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, reinterpret_cast<void**>(&pVolume));
    EXIT_ON_ERROR(hr);

    float volume(0.0f);
    hr = pVolume->GetMasterVolumeLevelScalar(&volume);
    if (FAILED(hr))
    {
        available = false;
    }
    available = true;

    SAFE_RELEASE(pVolume);
    return 0;

Exit:
    _TraceCOMError(hr);
    SAFE_RELEASE(pVolume);
    return -1;
}





WebRtc_Word32 AudioDeviceWindowsCore::SetMicrophoneVolume(WebRtc_UWord32 volume)
{
    WEBRTC_TRACE(kTraceStream, kTraceAudioDevice, _id, "AudioDeviceWindowsCore::SetMicrophoneVolume(volume=%u)", volume);

    {
        CriticalSectionScoped lock(&_critSect);

        if (!_microphoneIsInitialized)
        {
            return -1;
        }

        if (_ptrDeviceIn == NULL)
        {
            return -1;
        }
    }

    if (volume < static_cast<WebRtc_UWord32>(MIN_CORE_MICROPHONE_VOLUME) ||
        volume > static_cast<WebRtc_UWord32>(MAX_CORE_MICROPHONE_VOLUME))
    {
        return -1;
    }

    HRESULT hr = S_OK;
    
    const float fLevel = static_cast<float>(volume)/MAX_CORE_MICROPHONE_VOLUME;
    _volumeMutex.Enter();
    _ptrCaptureVolume->SetMasterVolumeLevelScalar(fLevel, NULL);
    _volumeMutex.Leave();
    EXIT_ON_ERROR(hr);

    return 0;

Exit:
    _TraceCOMError(hr);
    return -1;
}





WebRtc_Word32 AudioDeviceWindowsCore::MicrophoneVolume(WebRtc_UWord32& volume) const
{
    {
        CriticalSectionScoped lock(&_critSect);

        if (!_microphoneIsInitialized)
        {
            return -1;
        }

        if (_ptrDeviceIn == NULL)
        {
            return -1;
        }
    }

    HRESULT hr = S_OK;
    float fLevel(0.0f);
    volume = 0;
    _volumeMutex.Enter();
    hr = _ptrCaptureVolume->GetMasterVolumeLevelScalar(&fLevel);
    _volumeMutex.Leave();
    EXIT_ON_ERROR(hr);

    
    volume = static_cast<WebRtc_UWord32> (fLevel*MAX_CORE_MICROPHONE_VOLUME);

    return 0;

Exit:
    _TraceCOMError(hr);
    return -1;
}










WebRtc_Word32 AudioDeviceWindowsCore::MaxMicrophoneVolume(WebRtc_UWord32& maxVolume) const
{
    WEBRTC_TRACE(kTraceStream, kTraceAudioDevice, _id, "%s", __FUNCTION__);

    if (!_microphoneIsInitialized)
    {
        return -1;
    }

    maxVolume = static_cast<WebRtc_UWord32> (MAX_CORE_MICROPHONE_VOLUME);

    return 0;
}





WebRtc_Word32 AudioDeviceWindowsCore::MinMicrophoneVolume(WebRtc_UWord32& minVolume) const
{

    if (!_microphoneIsInitialized)
    {
        return -1;
    }

    minVolume = static_cast<WebRtc_UWord32> (MIN_CORE_MICROPHONE_VOLUME);

    return 0;
}





WebRtc_Word32 AudioDeviceWindowsCore::MicrophoneVolumeStepSize(WebRtc_UWord16& stepSize) const
{

    if (!_microphoneIsInitialized)
    {
        return -1;
    }

    stepSize = CORE_MICROPHONE_VOLUME_STEP_SIZE;

    return 0;
}





WebRtc_Word16 AudioDeviceWindowsCore::PlayoutDevices()
{

    CriticalSectionScoped lock(&_critSect);

    if (_RefreshDeviceList(eRender) != -1)
    {
        return (_DeviceListCount(eRender));
    }

    return -1;
}





WebRtc_Word32 AudioDeviceWindowsCore::SetPlayoutDevice(WebRtc_UWord16 index)
{

    if (_playIsInitialized)
    {
        return -1;
    }

    
    UINT nDevices = PlayoutDevices();

    if (index < 0 || index > (nDevices-1))
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id, "device index is out of range [0,%u]", (nDevices-1));
        return -1;
    }

    CriticalSectionScoped lock(&_critSect);

    HRESULT hr(S_OK);

    assert(_ptrRenderCollection != NULL);

    
    SAFE_RELEASE(_ptrDeviceOut);
    hr = _ptrRenderCollection->Item(
                                 index,
                                 &_ptrDeviceOut);
    if (FAILED(hr))
    {
        _TraceCOMError(hr);
        SAFE_RELEASE(_ptrDeviceOut);
        return -1;
    }

    WCHAR szDeviceName[MAX_PATH];
    const int bufferLen = sizeof(szDeviceName)/sizeof(szDeviceName)[0];

    
    if (_GetDeviceName(_ptrDeviceOut, szDeviceName, bufferLen) == 0)
    {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "friendly name: \"%S\"", szDeviceName);
    }

    _usingOutputDeviceIndex = true;
    _outputDeviceIndex = index;

    return 0;
}





WebRtc_Word32 AudioDeviceWindowsCore::SetPlayoutDevice(AudioDeviceModule::WindowsDeviceType device)
{
    if (_playIsInitialized)
    {
        return -1;
    }

    ERole role(eCommunications);

    if (device == AudioDeviceModule::kDefaultDevice)
    {
        role = eConsole;
    }
    else if (device == AudioDeviceModule::kDefaultCommunicationDevice)
    {
        role = eCommunications;
    }

    CriticalSectionScoped lock(&_critSect);

    
    _RefreshDeviceList(eRender);

    HRESULT hr(S_OK);

    assert(_ptrEnumerator != NULL);

    
    SAFE_RELEASE(_ptrDeviceOut);
    hr = _ptrEnumerator->GetDefaultAudioEndpoint(
                           eRender,
                           role,
                           &_ptrDeviceOut);
    if (FAILED(hr))
    {
        _TraceCOMError(hr);
        SAFE_RELEASE(_ptrDeviceOut);
        return -1;
    }

    WCHAR szDeviceName[MAX_PATH];
    const int bufferLen = sizeof(szDeviceName)/sizeof(szDeviceName)[0];

    
    if (_GetDeviceName(_ptrDeviceOut, szDeviceName, bufferLen) == 0)
    {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "friendly name: \"%S\"", szDeviceName);
    }

    _usingOutputDeviceIndex = false;
    _outputDevice = device;

    return 0;
}





WebRtc_Word32 AudioDeviceWindowsCore::PlayoutDeviceName(
    WebRtc_UWord16 index,
    char name[kAdmMaxDeviceNameSize],
    char guid[kAdmMaxGuidSize])
{

    bool defaultCommunicationDevice(false);
    const WebRtc_Word16 nDevices(PlayoutDevices());  

    
    if (index == (WebRtc_UWord16)(-1))
    {
        defaultCommunicationDevice = true;
        index = 0;
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "Default Communication endpoint device will be used");
    }

    if ((index > (nDevices-1)) || (name == NULL))
    {
        return -1;
    }

    memset(name, 0, kAdmMaxDeviceNameSize);

    if (guid != NULL)
    {
        memset(guid, 0, kAdmMaxGuidSize);
    }

    CriticalSectionScoped lock(&_critSect);

    WebRtc_Word32 ret(-1);
    WCHAR szDeviceName[MAX_PATH];
    const int bufferLen = sizeof(szDeviceName)/sizeof(szDeviceName)[0];

    
    if (defaultCommunicationDevice)
    {
        ret = _GetDefaultDeviceName(eRender, eCommunications, szDeviceName, bufferLen);
    }
    else
    {
        ret = _GetListDeviceName(eRender, index, szDeviceName, bufferLen);
    }

    if (ret == 0)
    {
        
        if (WideCharToMultiByte(CP_UTF8, 0, szDeviceName, -1, name, kAdmMaxDeviceNameSize, NULL, NULL) == 0)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id, "WideCharToMultiByte(CP_UTF8) failed with error code %d", GetLastError());
        }
    }

    
    if (defaultCommunicationDevice)
    {
        ret = _GetDefaultDeviceID(eRender, eCommunications, szDeviceName, bufferLen);
    }
    else
    {
        ret = _GetListDeviceID(eRender, index, szDeviceName, bufferLen);
    }

    if (guid != NULL && ret == 0)
    {
        
        if (WideCharToMultiByte(CP_UTF8, 0, szDeviceName, -1, guid, kAdmMaxGuidSize, NULL, NULL) == 0)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id, "WideCharToMultiByte(CP_UTF8) failed with error code %d", GetLastError());
        }
    }

    return ret;
}





WebRtc_Word32 AudioDeviceWindowsCore::RecordingDeviceName(
    WebRtc_UWord16 index,
    char name[kAdmMaxDeviceNameSize],
    char guid[kAdmMaxGuidSize])
{

    bool defaultCommunicationDevice(false);
    const WebRtc_Word16 nDevices(RecordingDevices());  

    
    if (index == (WebRtc_UWord16)(-1))
    {
        defaultCommunicationDevice = true;
        index = 0;
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "Default Communication endpoint device will be used");
    }

    if ((index > (nDevices-1)) || (name == NULL))
    {
        return -1;
    }

    memset(name, 0, kAdmMaxDeviceNameSize);

    if (guid != NULL)
    {
        memset(guid, 0, kAdmMaxGuidSize);
    }

    CriticalSectionScoped lock(&_critSect);

    WebRtc_Word32 ret(-1);
    WCHAR szDeviceName[MAX_PATH];
    const int bufferLen = sizeof(szDeviceName)/sizeof(szDeviceName)[0];

    
    if (defaultCommunicationDevice)
    {
        ret = _GetDefaultDeviceName(eCapture, eCommunications, szDeviceName, bufferLen);
    }
    else
    {
        ret = _GetListDeviceName(eCapture, index, szDeviceName, bufferLen);
    }

    if (ret == 0)
    {
        
        if (WideCharToMultiByte(CP_UTF8, 0, szDeviceName, -1, name, kAdmMaxDeviceNameSize, NULL, NULL) == 0)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id, "WideCharToMultiByte(CP_UTF8) failed with error code %d", GetLastError());
        }
    }

    
    if (defaultCommunicationDevice)
    {
        ret = _GetDefaultDeviceID(eCapture, eCommunications, szDeviceName, bufferLen);
    }
    else
    {
        ret = _GetListDeviceID(eCapture, index, szDeviceName, bufferLen);
    }

    if (guid != NULL && ret == 0)
    {
        
        if (WideCharToMultiByte(CP_UTF8, 0, szDeviceName, -1, guid, kAdmMaxGuidSize, NULL, NULL) == 0)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id, "WideCharToMultiByte(CP_UTF8) failed with error code %d", GetLastError());
        }
    }

    return ret;
}





WebRtc_Word16 AudioDeviceWindowsCore::RecordingDevices()
{

    CriticalSectionScoped lock(&_critSect);

    if (_RefreshDeviceList(eCapture) != -1)
    {
        return (_DeviceListCount(eCapture));
    }

    return -1;
}





WebRtc_Word32 AudioDeviceWindowsCore::SetRecordingDevice(WebRtc_UWord16 index)
{

    if (_recIsInitialized)
    {
        return -1;
    }

    
    UINT nDevices = RecordingDevices();

    if (index < 0 || index > (nDevices-1))
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id, "device index is out of range [0,%u]", (nDevices-1));
        return -1;
    }

    CriticalSectionScoped lock(&_critSect);

    HRESULT hr(S_OK);

    assert(_ptrCaptureCollection != NULL);

    
    SAFE_RELEASE(_ptrDeviceIn);
    hr = _ptrCaptureCollection->Item(
                                 index,
                                 &_ptrDeviceIn);
    if (FAILED(hr))
    {
        _TraceCOMError(hr);
        SAFE_RELEASE(_ptrDeviceIn);
        return -1;
    }

    WCHAR szDeviceName[MAX_PATH];
    const int bufferLen = sizeof(szDeviceName)/sizeof(szDeviceName)[0];

    
    if (_GetDeviceName(_ptrDeviceIn, szDeviceName, bufferLen) == 0)
    {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "friendly name: \"%S\"", szDeviceName);
    }

    _usingInputDeviceIndex = true;
    _inputDeviceIndex = index;

    return 0;
}





WebRtc_Word32 AudioDeviceWindowsCore::SetRecordingDevice(AudioDeviceModule::WindowsDeviceType device)
{
    if (_recIsInitialized)
    {
        return -1;
    }

    ERole role(eCommunications);

    if (device == AudioDeviceModule::kDefaultDevice)
    {
        role = eConsole;
    }
    else if (device == AudioDeviceModule::kDefaultCommunicationDevice)
    {
        role = eCommunications;
    }

    CriticalSectionScoped lock(&_critSect);

    
    _RefreshDeviceList(eCapture);

    HRESULT hr(S_OK);

    assert(_ptrEnumerator != NULL);

    
    SAFE_RELEASE(_ptrDeviceIn);
    hr = _ptrEnumerator->GetDefaultAudioEndpoint(
                           eCapture,
                           role,
                           &_ptrDeviceIn);
    if (FAILED(hr))
    {
        _TraceCOMError(hr);
        SAFE_RELEASE(_ptrDeviceIn);
        return -1;
    }

    WCHAR szDeviceName[MAX_PATH];
    const int bufferLen = sizeof(szDeviceName)/sizeof(szDeviceName)[0];

    
    if (_GetDeviceName(_ptrDeviceIn, szDeviceName, bufferLen) == 0)
    {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "friendly name: \"%S\"", szDeviceName);
    }

    _usingInputDeviceIndex = false;
    _inputDevice = device;

    return 0;
}





WebRtc_Word32 AudioDeviceWindowsCore::PlayoutIsAvailable(bool& available)
{

    available = false;

    
    WebRtc_Word32 res = InitPlayout();

    
    StopPlayout();

    if (res != -1)
    {
        available = true;
    }

    return 0;
}





WebRtc_Word32 AudioDeviceWindowsCore::RecordingIsAvailable(bool& available)
{

    available = false;

    
    WebRtc_Word32 res = InitRecording();

    
    StopRecording();

    if (res != -1)
    {
        available = true;
    }

    return 0;
}





WebRtc_Word32 AudioDeviceWindowsCore::InitPlayout()
{

    CriticalSectionScoped lock(&_critSect);

    if (_playing)
    {
        return -1;
    }

    if (_playIsInitialized)
    {
        return 0;
    }

    if (_ptrDeviceOut == NULL)
    {
        return -1;
    }

    
    if (InitSpeaker() == -1)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "InitSpeaker() failed");
    }

    
    if (_ptrDeviceOut == NULL)
    {
        return -1;
    }

    if (_builtInAecEnabled && _recIsInitialized)
    {
        
        
        if (SetDMOProperties() == -1)
        {
            return -1;
        }
    }

    HRESULT hr = S_OK;
    WAVEFORMATEX* pWfxOut = NULL;
    WAVEFORMATEX Wfx;
    WAVEFORMATEX* pWfxClosestMatch = NULL;

    
    SAFE_RELEASE(_ptrClientOut);
    hr = _ptrDeviceOut->Activate(
                          __uuidof(IAudioClient),
                          CLSCTX_ALL,
                          NULL,
                          (void**)&_ptrClientOut);
    EXIT_ON_ERROR(hr);

    
    
    hr = _ptrClientOut->GetMixFormat(&pWfxOut);
    if (SUCCEEDED(hr))
    {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "Audio Engine's current rendering mix format:");
        
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "wFormatTag     : 0x%X (%u)", pWfxOut->wFormatTag, pWfxOut->wFormatTag);
        
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "nChannels      : %d", pWfxOut->nChannels);
        
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "nSamplesPerSec : %d", pWfxOut->nSamplesPerSec);
        
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "nAvgBytesPerSec: %d", pWfxOut->nAvgBytesPerSec);
        
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "nBlockAlign    : %d", pWfxOut->nBlockAlign);
        
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "wBitsPerSample : %d", pWfxOut->wBitsPerSample);
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "cbSize         : %d", pWfxOut->cbSize);
    }

    
    Wfx.wFormatTag = WAVE_FORMAT_PCM;
    Wfx.wBitsPerSample = 16;
    Wfx.cbSize = 0;

    const int freqs[] = {48000, 44100, 16000, 96000, 32000, 8000};
    hr = S_FALSE;

    
    for (int freq = 0; freq < sizeof(freqs)/sizeof(freqs[0]); freq++)
    {
        for (int chan = 0; chan < sizeof(_playChannelsPrioList)/sizeof(_playChannelsPrioList[0]); chan++)
        {
            Wfx.nChannels = _playChannelsPrioList[chan];
            Wfx.nSamplesPerSec = freqs[freq];
            Wfx.nBlockAlign = Wfx.nChannels * Wfx.wBitsPerSample / 8;
            Wfx.nAvgBytesPerSec = Wfx.nSamplesPerSec * Wfx.nBlockAlign;
            
            
            
            hr = _ptrClientOut->IsFormatSupported(
                                  AUDCLNT_SHAREMODE_SHARED,
                                  &Wfx,
                                  &pWfxClosestMatch);
            if (hr == S_OK)
            {
                break;
            }
            else
            {
                WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "nChannels=%d, nSamplesPerSec=%d is not supported",
                    Wfx.nChannels, Wfx.nSamplesPerSec);
            }
        }
        if (hr == S_OK)
            break;
    }

    
    
    
    if (hr == S_OK)
    {
        _playAudioFrameSize = Wfx.nBlockAlign;
        _playBlockSize = Wfx.nSamplesPerSec/100;
        _playSampleRate = Wfx.nSamplesPerSec;
        _devicePlaySampleRate = Wfx.nSamplesPerSec; 
        _devicePlayBlockSize = Wfx.nSamplesPerSec/100;
        if (_playBlockSize == 441)
        {
            _playSampleRate = 44000;    
            _playBlockSize = 440;       
        }
        _playChannels = Wfx.nChannels;

        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "VoE selected this rendering format:");
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "wFormatTag         : 0x%X (%u)", Wfx.wFormatTag, Wfx.wFormatTag);
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "nChannels          : %d", Wfx.nChannels);
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "nSamplesPerSec     : %d", Wfx.nSamplesPerSec);
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "nAvgBytesPerSec    : %d", Wfx.nAvgBytesPerSec);
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "nBlockAlign        : %d", Wfx.nBlockAlign);
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "wBitsPerSample     : %d", Wfx.wBitsPerSample);
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "cbSize             : %d", Wfx.cbSize);
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "Additional settings:");
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "_playAudioFrameSize: %d", _playAudioFrameSize);
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "_playBlockSize     : %d", _playBlockSize);
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "_playChannels      : %d", _playChannels);
    }

    _Get44kHzDrift();

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    REFERENCE_TIME hnsBufferDuration = 0;  
    if (_devicePlaySampleRate == 44100)
    {
        
        
        
        
        
        
        hnsBufferDuration = 30*10000;
    }
    hr = _ptrClientOut->Initialize(
                          AUDCLNT_SHAREMODE_SHARED,             
                          AUDCLNT_STREAMFLAGS_EVENTCALLBACK,    
                          hnsBufferDuration,                    
                          0,                                    
                          &Wfx,                                 
                          NULL);                                

    if (FAILED(hr))
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id, "IAudioClient::Initialize() failed:");
        if (pWfxClosestMatch != NULL)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id, "closest mix format: #channels=%d, samples/sec=%d, bits/sample=%d",
                pWfxClosestMatch->nChannels, pWfxClosestMatch->nSamplesPerSec, pWfxClosestMatch->wBitsPerSample);
        }
        else
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id, "no format suggested");
        }
    }
    EXIT_ON_ERROR(hr);

    if (_ptrAudioBuffer)
    {
        
        _ptrAudioBuffer->SetPlayoutSampleRate(_playSampleRate);
        _ptrAudioBuffer->SetPlayoutChannels((WebRtc_UWord8)_playChannels);
    }
    else
    {
        
        
        
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "AudioDeviceBuffer must be attached before streaming can start");
    }

    
    
    UINT bufferFrameCount(0);
    hr = _ptrClientOut->GetBufferSize(
                          &bufferFrameCount);
    if (SUCCEEDED(hr))
    {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "IAudioClient::GetBufferSize() => %u (<=> %u bytes)",
            bufferFrameCount, bufferFrameCount*_playAudioFrameSize);
    }

    
    
    hr = _ptrClientOut->SetEventHandle(
                          _hRenderSamplesReadyEvent);
    EXIT_ON_ERROR(hr);

    
    SAFE_RELEASE(_ptrRenderClient);
    hr = _ptrClientOut->GetService(
                          __uuidof(IAudioRenderClient),
                          (void**)&_ptrRenderClient);
    EXIT_ON_ERROR(hr);

    
    _playIsInitialized = true;

    CoTaskMemFree(pWfxOut);
    CoTaskMemFree(pWfxClosestMatch);

    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "render side is now initialized");
    return 0;

Exit:
    _TraceCOMError(hr);
    CoTaskMemFree(pWfxOut);
    CoTaskMemFree(pWfxClosestMatch);
    SAFE_RELEASE(_ptrClientOut);
    SAFE_RELEASE(_ptrRenderClient);
    return -1;
}





WebRtc_Word32 AudioDeviceWindowsCore::InitRecordingDMO()
{
    assert(_builtInAecEnabled);
    assert(_dmo != NULL);

    if (SetDMOProperties() == -1)
    {
        return -1;
    }

    DMO_MEDIA_TYPE mt = {0};
    HRESULT hr = MoInitMediaType(&mt, sizeof(WAVEFORMATEX));
    if (FAILED(hr))
    {
        MoFreeMediaType(&mt);
        _TraceCOMError(hr);
        return -1;
    }
    mt.majortype = MEDIATYPE_Audio;
    mt.subtype = MEDIASUBTYPE_PCM;
    mt.formattype = FORMAT_WaveFormatEx;

    
    
    
    
    WAVEFORMATEX* ptrWav = reinterpret_cast<WAVEFORMATEX*>(mt.pbFormat);
    ptrWav->wFormatTag = WAVE_FORMAT_PCM;
    ptrWav->nChannels = 1;
    
    ptrWav->nSamplesPerSec = 16000;
    ptrWav->nAvgBytesPerSec = 32000;
    ptrWav->nBlockAlign = 2;
    ptrWav->wBitsPerSample = 16;
    ptrWav->cbSize = 0;

    
    _recAudioFrameSize = ptrWav->nBlockAlign;
    _recSampleRate = ptrWav->nSamplesPerSec;
    _recBlockSize = ptrWav->nSamplesPerSec / 100;
    _recChannels = ptrWav->nChannels;

    
    hr = _dmo->SetOutputType(kAecCaptureStreamIndex, &mt, 0);
    MoFreeMediaType(&mt);
    if (FAILED(hr))
    {
        _TraceCOMError(hr);
        return -1;
    }

    if (_ptrAudioBuffer)
    {
        _ptrAudioBuffer->SetRecordingSampleRate(_recSampleRate);
        _ptrAudioBuffer->SetRecordingChannels(_recChannels);
    }
    else
    {
        
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
            "AudioDeviceBuffer must be attached before streaming can start");
    }

    _mediaBuffer = new MediaBufferImpl(_recBlockSize * _recAudioFrameSize);

    
    hr = _dmo->AllocateStreamingResources();
    if (FAILED(hr))
    {
         _TraceCOMError(hr);
        return -1;
    }

    _recIsInitialized = true;
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
        "Capture side is now initialized");

    return 0;
}





WebRtc_Word32 AudioDeviceWindowsCore::InitRecording()
{

    CriticalSectionScoped lock(&_critSect);

    if (_recording)
    {
        return -1;
    }

    if (_recIsInitialized)
    {
        return 0;
    }

    if (QueryPerformanceFrequency(&_perfCounterFreq) == 0)
    {
        return -1;
    }
    _perfCounterFactor = 10000000.0 / (double)_perfCounterFreq.QuadPart;

    if (_ptrDeviceIn == NULL)
    {
        return -1;
    }

    
    if (InitMicrophone() == -1)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "InitMicrophone() failed");
    }

    
    if (_ptrDeviceIn == NULL)
    {
        return -1;
    }

    if (_builtInAecEnabled)
    {
        
        return InitRecordingDMO();
    }

    HRESULT hr = S_OK;
    WAVEFORMATEX* pWfxIn = NULL;
    WAVEFORMATEX Wfx;
    WAVEFORMATEX* pWfxClosestMatch = NULL;

    
    SAFE_RELEASE(_ptrClientIn);
    hr = _ptrDeviceIn->Activate(
                          __uuidof(IAudioClient),
                          CLSCTX_ALL,
                          NULL,
                          (void**)&_ptrClientIn);
    EXIT_ON_ERROR(hr);

    
    
    hr = _ptrClientIn->GetMixFormat(&pWfxIn);
    if (SUCCEEDED(hr))
    {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "Audio Engine's current capturing mix format:");
        
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "wFormatTag     : 0x%X (%u)", pWfxIn->wFormatTag, pWfxIn->wFormatTag);
        
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "nChannels      : %d", pWfxIn->nChannels);
        
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "nSamplesPerSec : %d", pWfxIn->nSamplesPerSec);
        
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "nAvgBytesPerSec: %d", pWfxIn->nAvgBytesPerSec);
        
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "nBlockAlign    : %d", pWfxIn->nBlockAlign);
        
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "wBitsPerSample : %d", pWfxIn->wBitsPerSample);
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "cbSize         : %d", pWfxIn->cbSize);
    }

    
    Wfx.wFormatTag = WAVE_FORMAT_PCM;
    Wfx.wBitsPerSample = 16;
    Wfx.cbSize = 0;

    const int freqs[6] = {48000, 44100, 16000, 96000, 32000, 8000};
    hr = S_FALSE;

    
    for (int freq = 0; freq < sizeof(freqs)/sizeof(freqs[0]); freq++)
    {
        for (int chan = 0; chan < sizeof(_recChannelsPrioList)/sizeof(_recChannelsPrioList[0]); chan++)
        {
            Wfx.nChannels = _recChannelsPrioList[chan];
            Wfx.nSamplesPerSec = freqs[freq];
            Wfx.nBlockAlign = Wfx.nChannels * Wfx.wBitsPerSample / 8;
            Wfx.nAvgBytesPerSec = Wfx.nSamplesPerSec * Wfx.nBlockAlign;
            
            
            
            hr = _ptrClientIn->IsFormatSupported(
                                  AUDCLNT_SHAREMODE_SHARED,
                                  &Wfx,
                                  &pWfxClosestMatch);
            if (hr == S_OK)
            {
                break;
            }
            else
            {
                WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "nChannels=%d, nSamplesPerSec=%d is not supported",
                    Wfx.nChannels, Wfx.nSamplesPerSec);
            }
        }
        if (hr == S_OK)
            break;
    }

    if (hr == S_OK)
    {
        _recAudioFrameSize = Wfx.nBlockAlign;
        _recSampleRate = Wfx.nSamplesPerSec;
        _recBlockSize = Wfx.nSamplesPerSec/100;
        _recChannels = Wfx.nChannels;
        if (_recBlockSize == 441)
        {
            _recSampleRate = 44000; 
            _recBlockSize = 440;    
        }

        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "VoE selected this capturing format:");
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "wFormatTag        : 0x%X (%u)", Wfx.wFormatTag, Wfx.wFormatTag);
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "nChannels         : %d", Wfx.nChannels);
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "nSamplesPerSec    : %d", Wfx.nSamplesPerSec);
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "nAvgBytesPerSec   : %d", Wfx.nAvgBytesPerSec);
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "nBlockAlign       : %d", Wfx.nBlockAlign);
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "wBitsPerSample    : %d", Wfx.wBitsPerSample);
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "cbSize            : %d", Wfx.cbSize);
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "Additional settings:");
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "_recAudioFrameSize: %d", _recAudioFrameSize);
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "_recBlockSize     : %d", _recBlockSize);
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "_recChannels      : %d", _recChannels);
    }

    _Get44kHzDrift();

    
    hr = _ptrClientIn->Initialize(
                          AUDCLNT_SHAREMODE_SHARED,             
                          AUDCLNT_STREAMFLAGS_EVENTCALLBACK |   
                          AUDCLNT_STREAMFLAGS_NOPERSIST,        
                          0,                                    
                          0,                                    
                          &Wfx,                                 
                          NULL);                                


    if (hr != S_OK)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id, "IAudioClient::Initialize() failed:");
        if (pWfxClosestMatch != NULL)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id, "closest mix format: #channels=%d, samples/sec=%d, bits/sample=%d",
                pWfxClosestMatch->nChannels, pWfxClosestMatch->nSamplesPerSec, pWfxClosestMatch->wBitsPerSample);
        }
        else
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id, "no format suggested");
        }
    }
    EXIT_ON_ERROR(hr);

    if (_ptrAudioBuffer)
    {
        
        _ptrAudioBuffer->SetRecordingSampleRate(_recSampleRate);
        _ptrAudioBuffer->SetRecordingChannels((WebRtc_UWord8)_recChannels);
    }
    else
    {
        
        
        
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "AudioDeviceBuffer must be attached before streaming can start");
    }

    
    
    UINT bufferFrameCount(0);
    hr = _ptrClientIn->GetBufferSize(
                          &bufferFrameCount);
    if (SUCCEEDED(hr))
    {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "IAudioClient::GetBufferSize() => %u (<=> %u bytes)",
            bufferFrameCount, bufferFrameCount*_recAudioFrameSize);
    }

    
    
    hr = _ptrClientIn->SetEventHandle(
                          _hCaptureSamplesReadyEvent);
    EXIT_ON_ERROR(hr);

    
    SAFE_RELEASE(_ptrCaptureClient);
    hr = _ptrClientIn->GetService(
                          __uuidof(IAudioCaptureClient),
                          (void**)&_ptrCaptureClient);
    EXIT_ON_ERROR(hr);

    
    _recIsInitialized = true;

    CoTaskMemFree(pWfxIn);
    CoTaskMemFree(pWfxClosestMatch);

    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "capture side is now initialized");
    return 0;

Exit:
    _TraceCOMError(hr);
    CoTaskMemFree(pWfxIn);
    CoTaskMemFree(pWfxClosestMatch);
    SAFE_RELEASE(_ptrClientIn);
    SAFE_RELEASE(_ptrCaptureClient);
    return -1;
}





WebRtc_Word32 AudioDeviceWindowsCore::StartRecording()
{

    if (!_recIsInitialized)
    {
        return -1;
    }

    if (_hRecThread != NULL)
    {
        return 0;
    }

    if (_recording)
    {
        return 0;
    }

    {
        CriticalSectionScoped critScoped(&_critSect);

        
        LPTHREAD_START_ROUTINE lpStartAddress = WSAPICaptureThread;
        if (_builtInAecEnabled)
        {
            
            lpStartAddress = WSAPICaptureThreadPollDMO;

            if (!_playing)
            {
                
                
                WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                    "Playout must be started before recording when using the "
                    "built-in AEC");
                return -1;
            }
        }

        assert(_hRecThread == NULL);
        _hRecThread = CreateThread(NULL,
                                   0,
                                   lpStartAddress,
                                   this,
                                   0,
                                   NULL);
        if (_hRecThread == NULL)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "failed to create the recording thread");
            return -1;
        }

        
        SetThreadPriority(_hRecThread, THREAD_PRIORITY_TIME_CRITICAL);

        assert(_hGetCaptureVolumeThread == NULL);
        _hGetCaptureVolumeThread = CreateThread(NULL,
                                                0,
                                                GetCaptureVolumeThread,
                                                this,
                                                0,
                                                NULL);
        if (_hGetCaptureVolumeThread == NULL)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "  failed to create the volume getter thread");
            return -1;
        }

        assert(_hSetCaptureVolumeThread == NULL);
        _hSetCaptureVolumeThread = CreateThread(NULL,
                                                0,
                                                SetCaptureVolumeThread,
                                                this,
                                                0,
                                                NULL);
        if (_hSetCaptureVolumeThread == NULL)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "  failed to create the volume setter thread");
            return -1;
        }
    }  

    DWORD ret = WaitForSingleObject(_hCaptureStartedEvent, 1000);
    if (ret != WAIT_OBJECT_0)
    {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
            "capturing did not start up properly");
        return -1;
    }
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
        "capture audio stream has now started...");

    _avgCPULoad = 0.0f;
    _playAcc = 0;
    _recording = true;

    return 0;
}





WebRtc_Word32 AudioDeviceWindowsCore::StopRecording()
{
    WebRtc_Word32 err = 0;

    if (!_recIsInitialized)
    {
        return 0;
    }

    _Lock();

    if (_hRecThread == NULL)
    {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
            "no capturing stream is active => close down WASAPI only");
        SAFE_RELEASE(_ptrClientIn);
        SAFE_RELEASE(_ptrCaptureClient);
        _recIsInitialized = false;
        _recording = false;
        _UnLock();
        return 0;
    }

    
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
        "closing down the webrtc_core_audio_capture_thread...");
    
    SetEvent(_hShutdownCaptureEvent);

    _UnLock();
    DWORD ret = WaitForSingleObject(_hRecThread, 2000);
    if (ret != WAIT_OBJECT_0)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
            "failed to close down webrtc_core_audio_capture_thread");
        err = -1;
    }
    else
    {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
            "webrtc_core_audio_capture_thread is now closed");
    }

    ret = WaitForSingleObject(_hGetCaptureVolumeThread, 2000);
    if (ret != WAIT_OBJECT_0)
    {
        
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  failed to close down volume getter thread");
        err = -1;
    }
    else
    {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
            "  volume getter thread is now closed");
    }

    ret = WaitForSingleObject(_hSetCaptureVolumeThread, 2000);
    if (ret != WAIT_OBJECT_0)
    {
        
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  failed to close down volume setter thread");
        err = -1;
    }
    else
    {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
            "  volume setter thread is now closed");
    }
    _Lock();

    ResetEvent(_hShutdownCaptureEvent); 
    
    assert(err == -1 || _ptrClientIn == NULL);
    assert(err == -1 || _ptrCaptureClient == NULL);

    _recIsInitialized = false;
    _recording = false;

    
    
    CloseHandle(_hRecThread);
    _hRecThread = NULL;

    CloseHandle(_hGetCaptureVolumeThread);
    _hGetCaptureVolumeThread = NULL;

    CloseHandle(_hSetCaptureVolumeThread);
    _hSetCaptureVolumeThread = NULL;

    if (_builtInAecEnabled)
    {
        assert(_dmo != NULL);
        
        
        HRESULT hr = _dmo->FreeStreamingResources();
        if (FAILED(hr))
        {
            _TraceCOMError(hr);
            err = -1;
        }
    }

    
    _sndCardRecDelay = 0;

    _UnLock();

    return err;
}





bool AudioDeviceWindowsCore::RecordingIsInitialized() const
{
    return (_recIsInitialized);
}





bool AudioDeviceWindowsCore::Recording() const
{
    return (_recording);
}





bool AudioDeviceWindowsCore::PlayoutIsInitialized() const
{

    return (_playIsInitialized);
}





WebRtc_Word32 AudioDeviceWindowsCore::StartPlayout()
{

    if (!_playIsInitialized)
    {
        return -1;
    }

    if (_hPlayThread != NULL)
    {
        return 0;
    }

    if (_playing)
    {
        return 0;
    }

    {
        CriticalSectionScoped critScoped(&_critSect);

        
        assert(_hPlayThread == NULL);
        _hPlayThread = CreateThread(
                         NULL,
                         0,
                         WSAPIRenderThread,
                         this,
                         0,
                         NULL);
        if (_hPlayThread == NULL)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                "failed to create the playout thread");
            return -1;
        }

        
        SetThreadPriority(_hPlayThread, THREAD_PRIORITY_TIME_CRITICAL);
    }  

    DWORD ret = WaitForSingleObject(_hRenderStartedEvent, 1000);
    if (ret != WAIT_OBJECT_0)
    {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
            "rendering did not start up properly");
        return -1;
    }

    _playing = true;
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
        "rendering audio stream has now started...");

    return 0;
}





WebRtc_Word32 AudioDeviceWindowsCore::StopPlayout()
{

    if (!_playIsInitialized)
    {
        return 0;
    }

    {
        CriticalSectionScoped critScoped(&_critSect) ;

        if (_hPlayThread == NULL)
        {
            WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                "no rendering stream is active => close down WASAPI only");
            SAFE_RELEASE(_ptrClientOut);
            SAFE_RELEASE(_ptrRenderClient);
            _playIsInitialized = false;
            _playing = false;
            return 0;
        }

        
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
            "closing down the webrtc_core_audio_render_thread...");
        SetEvent(_hShutdownRenderEvent);
    }  

    DWORD ret = WaitForSingleObject(_hPlayThread, 2000);
    if (ret != WAIT_OBJECT_0)
    {
        
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
            "failed to close down webrtc_core_audio_render_thread");
        CloseHandle(_hPlayThread);
        _hPlayThread = NULL;
        _playIsInitialized = false;
        _playing = false;
        return -1;
    }

    {
        CriticalSectionScoped critScoped(&_critSect);
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
            "webrtc_core_audio_render_thread is now closed");

        
        
        
        ResetEvent(_hShutdownRenderEvent);

        SAFE_RELEASE(_ptrClientOut);
        SAFE_RELEASE(_ptrRenderClient);

        _playIsInitialized = false;
        _playing = false;

        CloseHandle(_hPlayThread);
        _hPlayThread = NULL;

        if (_builtInAecEnabled && _recording)
        {
            
            
            
            
            
            
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                "Recording should be stopped before playout when using the "
                "built-in AEC");
        }

        
        _sndCardPlayDelay = 0;
    }  

    return 0;
}





WebRtc_Word32 AudioDeviceWindowsCore::PlayoutDelay(WebRtc_UWord16& delayMS) const
{
    CriticalSectionScoped critScoped(&_critSect);
    delayMS = static_cast<WebRtc_UWord16>(_sndCardPlayDelay);
    return 0;
}





WebRtc_Word32 AudioDeviceWindowsCore::RecordingDelay(WebRtc_UWord16& delayMS) const
{
    CriticalSectionScoped critScoped(&_critSect);
    delayMS = static_cast<WebRtc_UWord16>(_sndCardRecDelay);
    return 0;
}





bool AudioDeviceWindowsCore::Playing() const
{
    return (_playing);
}




WebRtc_Word32 AudioDeviceWindowsCore::SetPlayoutBuffer(const AudioDeviceModule::BufferType type, WebRtc_UWord16 sizeMS)
{

    CriticalSectionScoped lock(&_critSect);

    _playBufType = type;

    if (type == AudioDeviceModule::kFixedBufferSize)
    {
        _playBufDelayFixed = sizeMS;
    }

    return 0;
}





WebRtc_Word32 AudioDeviceWindowsCore::PlayoutBuffer(AudioDeviceModule::BufferType& type, WebRtc_UWord16& sizeMS) const
{
    CriticalSectionScoped lock(&_critSect);
    type = _playBufType;

    if (type == AudioDeviceModule::kFixedBufferSize)
    {
        sizeMS = _playBufDelayFixed;
    }
    else
    {
        
        sizeMS = static_cast<WebRtc_UWord16>(_sndCardPlayDelay);
    }

    return 0;
}





WebRtc_Word32 AudioDeviceWindowsCore::CPULoad(WebRtc_UWord16& load) const
{

    load = static_cast<WebRtc_UWord16> (100*_avgCPULoad);

    return 0;
}





bool AudioDeviceWindowsCore::PlayoutWarning() const
{
    return ( _playWarning > 0);
}





bool AudioDeviceWindowsCore::PlayoutError() const
{
    return ( _playError > 0);
}





bool AudioDeviceWindowsCore::RecordingWarning() const
{
    return ( _recWarning > 0);
}





bool AudioDeviceWindowsCore::RecordingError() const
{
    return ( _recError > 0);
}





void AudioDeviceWindowsCore::ClearPlayoutWarning()
{
    _playWarning = 0;
}





void AudioDeviceWindowsCore::ClearPlayoutError()
{
    _playError = 0;
}





void AudioDeviceWindowsCore::ClearRecordingWarning()
{
    _recWarning = 0;
}





void AudioDeviceWindowsCore::ClearRecordingError()
{
    _recError = 0;
}









DWORD WINAPI AudioDeviceWindowsCore::WSAPIRenderThread(LPVOID context)
{
    return reinterpret_cast<AudioDeviceWindowsCore*>(context)->
        DoRenderThread();
}





DWORD WINAPI AudioDeviceWindowsCore::WSAPICaptureThread(LPVOID context)
{
    return reinterpret_cast<AudioDeviceWindowsCore*>(context)->
        DoCaptureThread();
}

DWORD WINAPI AudioDeviceWindowsCore::WSAPICaptureThreadPollDMO(LPVOID context)
{
    return reinterpret_cast<AudioDeviceWindowsCore*>(context)->
        DoCaptureThreadPollDMO();
}

DWORD WINAPI AudioDeviceWindowsCore::GetCaptureVolumeThread(LPVOID context)
{
    return reinterpret_cast<AudioDeviceWindowsCore*>(context)->
        DoGetCaptureVolumeThread();
}

DWORD WINAPI AudioDeviceWindowsCore::SetCaptureVolumeThread(LPVOID context)
{
    return reinterpret_cast<AudioDeviceWindowsCore*>(context)->
        DoSetCaptureVolumeThread();
}

DWORD AudioDeviceWindowsCore::DoGetCaptureVolumeThread()
{
    HANDLE waitObject = _hShutdownCaptureEvent;

    while (1)
    {
        if (AGC())
        {
            WebRtc_UWord32 currentMicLevel = 0;
            if (MicrophoneVolume(currentMicLevel) == 0)
            {
                
                _Lock();
                if (_ptrAudioBuffer)
                {
                    _ptrAudioBuffer->SetCurrentMicLevel(currentMicLevel);
                }
                _UnLock();
            }
        }

        DWORD waitResult = WaitForSingleObject(waitObject,
                                               GET_MIC_VOLUME_INTERVAL_MS);
        switch (waitResult)
        {
            case WAIT_OBJECT_0: 
                return 0;
            case WAIT_TIMEOUT:  
                break;
            default:            
                WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                    "  unknown wait termination on get volume thread");
                return -1;
        }
    }
}

DWORD AudioDeviceWindowsCore::DoSetCaptureVolumeThread()
{
    HANDLE waitArray[2] = {_hShutdownCaptureEvent, _hSetCaptureVolumeEvent};

    while (1)
    {
        DWORD waitResult = WaitForMultipleObjects(2, waitArray, FALSE, INFINITE);
        switch (waitResult)
        {
            case WAIT_OBJECT_0:      
                return 0;
            case WAIT_OBJECT_0 + 1:  
                break;
            default:                 
                WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                    "  unknown wait termination on set volume thread");
                    return -1;
        }

        _Lock();
        WebRtc_UWord32 newMicLevel = _newMicLevel;
        _UnLock();

        if (SetMicrophoneVolume(newMicLevel) == -1)
        {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                "  the required modification of the microphone volume failed");
        }
    }
}





DWORD AudioDeviceWindowsCore::DoRenderThread()
{

    bool keepPlaying = true;
    HANDLE waitArray[2] = {_hShutdownRenderEvent, _hRenderSamplesReadyEvent};
    HRESULT hr = S_OK;
    HANDLE hMmTask = NULL;

    LARGE_INTEGER t1;
    LARGE_INTEGER t2;
    WebRtc_Word32 time(0);

    
    ScopedCOMInitializer comInit(ScopedCOMInitializer::kMTA);
    if (!comInit.succeeded()) {
      WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
          "failed to initialize COM in render thread");
      return -1;
    }

    _SetThreadName(-1, "webrtc_core_audio_render_thread");

    
    
    if (_winSupportAvrt)
    {
        DWORD taskIndex(0);
        hMmTask = _PAvSetMmThreadCharacteristicsA("Pro Audio", &taskIndex);
        if (hMmTask)
        {
            if (FALSE == _PAvSetMmThreadPriority(hMmTask, AVRT_PRIORITY_CRITICAL))
            {
                WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "failed to boost play-thread using MMCSS");
            }
            WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "render thread is now registered with MMCSS (taskIndex=%d)", taskIndex);
        }
        else
        {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "failed to enable MMCSS on render thread (err=%d)", GetLastError());
            _TraceCOMError(GetLastError());
        }
    }

    _Lock();

    IAudioClock* clock = NULL;

    
    
    
    UINT32 bufferLength = 0;
    hr = _ptrClientOut->GetBufferSize(&bufferLength);
    EXIT_ON_ERROR(hr);
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "[REND] size of buffer       : %u", bufferLength);

    
    
    REFERENCE_TIME latency;
    _ptrClientOut->GetStreamLatency(&latency);
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "[REND] max stream latency   : %u (%3.2f ms)",
        (DWORD)latency, (double)(latency/10000.0));

    
    
    
    
    
    
    
    
    
    REFERENCE_TIME devPeriod = 0;
    REFERENCE_TIME devPeriodMin = 0;
    _ptrClientOut->GetDevicePeriod(&devPeriod, &devPeriodMin);
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "[REND] device period        : %u (%3.2f ms)",
        (DWORD)devPeriod, (double)(devPeriod/10000.0));

    
    
    
    int playout_delay = 10 * (bufferLength / _playBlockSize) +
        (int)((latency + devPeriod) / 10000);
    _sndCardPlayDelay = playout_delay;
    _writtenSamples = 0;
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "[REND] initial delay        : %u", playout_delay);

    double endpointBufferSizeMS = 10.0 * ((double)bufferLength / (double)_devicePlayBlockSize);
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "[REND] endpointBufferSizeMS : %3.2f", endpointBufferSizeMS);

    
    
    BYTE *pData = NULL;
    hr = _ptrRenderClient->GetBuffer(bufferLength, &pData);
    EXIT_ON_ERROR(hr);

    hr = _ptrRenderClient->ReleaseBuffer(bufferLength, AUDCLNT_BUFFERFLAGS_SILENT);
    EXIT_ON_ERROR(hr);

    _writtenSamples += bufferLength;

    hr = _ptrClientOut->GetService(__uuidof(IAudioClock), (void**)&clock);
    if (FAILED(hr)) {
      WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                   "failed to get IAudioClock interface from the IAudioClient");
    }

    
    hr = _ptrClientOut->Start();
    EXIT_ON_ERROR(hr);

    _UnLock();

    
    
    SetEvent(_hRenderStartedEvent);

    

    while (keepPlaying)
    {
        
        DWORD waitResult = WaitForMultipleObjects(2, waitArray, FALSE, 500);
        switch (waitResult)
        {
        case WAIT_OBJECT_0 + 0:     
            keepPlaying = false;
            break;
        case WAIT_OBJECT_0 + 1:     
            break;
        case WAIT_TIMEOUT:          
            _ptrClientOut->Stop();
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "render event timed out after 0.5 seconds");
            goto Exit;
        default:                    
            _ptrClientOut->Stop();
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "unknown wait termination on render side");
            goto Exit;
        }

        while (keepPlaying)
        {
            _Lock();

            
            
            if (_ptrRenderClient == NULL || _ptrClientOut == NULL)
            {
                _UnLock();
                WEBRTC_TRACE(kTraceCritical, kTraceAudioDevice, _id,
                    "output state has been modified during unlocked period");
                goto Exit;
            }

            
            UINT32 padding = 0;
            hr = _ptrClientOut->GetCurrentPadding(&padding);
            EXIT_ON_ERROR(hr);

            
            WebRtc_UWord32 framesAvailable = bufferLength - padding;
            

            
            if (framesAvailable < _playBlockSize)
            {
                
                _UnLock();
                break;
            }

            
            const WebRtc_UWord32 n10msBuffers = (framesAvailable / _playBlockSize);
            for (WebRtc_UWord32 n = 0; n < n10msBuffers; n++)
            {
                
                hr = _ptrRenderClient->GetBuffer(_playBlockSize, &pData);
                EXIT_ON_ERROR(hr);

                QueryPerformanceCounter(&t1);    

                if (_ptrAudioBuffer)
                {
                    
                    _UnLock();
                    WebRtc_Word32 nSamples =
                    _ptrAudioBuffer->RequestPlayoutData(_playBlockSize);
                    _Lock();

                    if (nSamples == -1)
                    {
                        _UnLock();
                        WEBRTC_TRACE(kTraceCritical, kTraceAudioDevice, _id,
                                     "failed to read data from render client");
                        goto Exit;
                    }

                    
                    if (_ptrRenderClient == NULL || _ptrClientOut == NULL)
                    {
                        _UnLock();
                        WEBRTC_TRACE(kTraceCritical, kTraceAudioDevice, _id, "output state has been modified during unlocked period");
                        goto Exit;
                    }
                    if (nSamples != static_cast<WebRtc_Word32>(_playBlockSize))
                    {
                        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "nSamples(%d) != _playBlockSize(%d)", nSamples, _playBlockSize);
                    }

                    
                    nSamples = _ptrAudioBuffer->GetPlayoutData((WebRtc_Word8*)pData);
                }

                QueryPerformanceCounter(&t2);    
                time = (int)(t2.QuadPart-t1.QuadPart);
                _playAcc += time;

                DWORD dwFlags(0);
                hr = _ptrRenderClient->ReleaseBuffer(_playBlockSize, dwFlags);
                
                
                EXIT_ON_ERROR(hr);

                _writtenSamples += _playBlockSize;
            }

            
            if (clock) {
              UINT64 pos = 0;
              UINT64 freq = 1;
              clock->GetPosition(&pos, NULL);
              clock->GetFrequency(&freq);
              playout_delay = ROUND((double(_writtenSamples) /
                  _devicePlaySampleRate - double(pos) / freq) * 1000.0);
              _sndCardPlayDelay = playout_delay;
            }

            _UnLock();
        }
    }

    

    SleepMs(static_cast<DWORD>(endpointBufferSizeMS+0.5));
    hr = _ptrClientOut->Stop();

Exit:
    SAFE_RELEASE(clock);

    if (FAILED(hr))
    {
        _UnLock();
        _ptrClientOut->Stop();
        _TraceCOMError(hr);
    }

    if (_winSupportAvrt)
    {
        if (NULL != hMmTask)
        {
            _PAvRevertMmThreadCharacteristics(hMmTask);
        }
    }

    if (keepPlaying)
    {
        hr = _ptrClientOut->Stop();
        if (FAILED(hr))
        {
            _TraceCOMError(hr);
        }
        hr = _ptrClientOut->Reset();
        if (FAILED(hr))
        {
            _TraceCOMError(hr);
        }

        
        _playError = 1;
        WEBRTC_TRACE(kTraceError, kTraceUtility, _id, "kPlayoutError message posted: rendering thread has ended pre-maturely");
    }
    else
    {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "_Rendering thread is now terminated properly");
    }

    return (DWORD)hr;
}

DWORD AudioDeviceWindowsCore::InitCaptureThreadPriority()
{
    _hMmTask = NULL;

    _SetThreadName(-1, "webrtc_core_audio_capture_thread");

    
    
    if (_winSupportAvrt)
    {
        DWORD taskIndex(0);
        _hMmTask = _PAvSetMmThreadCharacteristicsA("Pro Audio", &taskIndex);
        if (_hMmTask)
        {
            if (!_PAvSetMmThreadPriority(_hMmTask, AVRT_PRIORITY_CRITICAL))
            {
                WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                    "failed to boost rec-thread using MMCSS");
            }
            WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                "capture thread is now registered with MMCSS (taskIndex=%d)",
                taskIndex);
        }
        else
        {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                "failed to enable MMCSS on capture thread (err=%d)",
                GetLastError());
            _TraceCOMError(GetLastError());
        }
    }

    return S_OK;
}

void AudioDeviceWindowsCore::RevertCaptureThreadPriority()
{
    if (_winSupportAvrt)
    {
        if (NULL != _hMmTask)
        {
            _PAvRevertMmThreadCharacteristics(_hMmTask);
        }
    }

    _hMmTask = NULL;
}

DWORD AudioDeviceWindowsCore::DoCaptureThreadPollDMO()
{
    assert(_mediaBuffer != NULL);
    bool keepRecording = true;

    
    ScopedCOMInitializer comInit(ScopedCOMInitializer::kMTA);
    if (!comInit.succeeded()) {
      WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
        "failed to initialize COM in polling DMO thread");
      return -1;
    }

    HRESULT hr = InitCaptureThreadPriority();
    if (FAILED(hr))
    {
        return hr;
    }

    
    
    SetEvent(_hCaptureStartedEvent);

    
    while (keepRecording)
    {
        
        
        DWORD waitResult = WaitForSingleObject(_hShutdownCaptureEvent, 5);
        switch (waitResult)
        {
        case WAIT_OBJECT_0:         
            keepRecording = false;
            break;
        case WAIT_TIMEOUT:          
            break;
        default:                    
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                "Unknown wait termination on capture side");
            hr = -1; 
            keepRecording = false;
            break;
        }

        while (keepRecording)
        {
            CriticalSectionScoped critScoped(&_critSect);

            DWORD dwStatus = 0;
            {
                DMO_OUTPUT_DATA_BUFFER dmoBuffer = {0};
                dmoBuffer.pBuffer = _mediaBuffer;
                dmoBuffer.pBuffer->AddRef();

                
                
                
                hr = _dmo->ProcessOutput(0, 1, &dmoBuffer, &dwStatus);
                SAFE_RELEASE(dmoBuffer.pBuffer);
                dwStatus = dmoBuffer.dwStatus;
            }
            if (FAILED(hr))
            {
                _TraceCOMError(hr);
                keepRecording = false;
                assert(false);
                break;
            }

            ULONG bytesProduced = 0;
            BYTE* data;
            
            
            hr = _mediaBuffer->GetBufferAndLength(&data, &bytesProduced);
            if (FAILED(hr))
            {
                _TraceCOMError(hr);
                keepRecording = false;
                assert(false);
                break;
            }

            

            if (bytesProduced > 0)
            {
                const int kSamplesProduced = bytesProduced / _recAudioFrameSize;
                
                
                
                assert(kSamplesProduced == static_cast<int>(_recBlockSize));
                assert(sizeof(BYTE) == sizeof(WebRtc_Word8));
                _ptrAudioBuffer->SetRecordedBuffer(
                    reinterpret_cast<WebRtc_Word8*>(data),
                    kSamplesProduced);
                _ptrAudioBuffer->SetVQEData(0, 0, 0);

                _UnLock();  
                _ptrAudioBuffer->DeliverRecordedData();
                _Lock();
            }

            
            hr = _mediaBuffer->SetLength(0);
            if (FAILED(hr))
            {
                _TraceCOMError(hr);
                keepRecording = false;
                assert(false);
                break;
            }

            if (!(dwStatus & DMO_OUTPUT_DATA_BUFFERF_INCOMPLETE))
            {
                
                
                
                break;
            }
        }
    }
    

    RevertCaptureThreadPriority();

    if (FAILED(hr))
    {
        
        _recError = 1;
        WEBRTC_TRACE(kTraceError, kTraceUtility, _id,
            "kRecordingError message posted: capturing thread has ended "
            "prematurely");
    }
    else
    {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
            "Capturing thread is now terminated properly");
    }

    return hr;
}






DWORD AudioDeviceWindowsCore::DoCaptureThread()
{

    bool keepRecording = true;
    HANDLE waitArray[2] = {_hShutdownCaptureEvent, _hCaptureSamplesReadyEvent};
    HRESULT hr = S_OK;

    LARGE_INTEGER t1;
    LARGE_INTEGER t2;
    WebRtc_Word32 time(0);

    BYTE* syncBuffer = NULL;
    UINT32 syncBufIndex = 0;

    _readSamples = 0;

    
    ScopedCOMInitializer comInit(ScopedCOMInitializer::kMTA);
    if (!comInit.succeeded()) {
      WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
        "failed to initialize COM in capture thread");
      return -1;
    }

    hr = InitCaptureThreadPriority();
    if (FAILED(hr))
    {
        return hr;
    }

    _Lock();

    
    
    
    UINT32 bufferLength = 0;
    hr = _ptrClientIn->GetBufferSize(&bufferLength);
    EXIT_ON_ERROR(hr);
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "[CAPT] size of buffer       : %u", bufferLength);

    
    
    
    
    const UINT32 syncBufferSize = 2*(bufferLength * _recAudioFrameSize);
    syncBuffer = new BYTE[syncBufferSize];
    if (syncBuffer == NULL)
    {
        return E_POINTER;
    }
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "[CAPT] size of sync buffer  : %u [bytes]", syncBufferSize);

    
    
    REFERENCE_TIME latency;
    _ptrClientIn->GetStreamLatency(&latency);
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "[CAPT] max stream latency   : %u (%3.2f ms)",
        (DWORD)latency, (double)(latency / 10000.0));

    
    
    
    REFERENCE_TIME devPeriod = 0;
    REFERENCE_TIME devPeriodMin = 0;
    _ptrClientIn->GetDevicePeriod(&devPeriod, &devPeriodMin);
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "[CAPT] device period        : %u (%3.2f ms)",
        (DWORD)devPeriod, (double)(devPeriod / 10000.0));

    double extraDelayMS = (double)((latency + devPeriod) / 10000.0);
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "[CAPT] extraDelayMS         : %3.2f", extraDelayMS);

    double endpointBufferSizeMS = 10.0 * ((double)bufferLength / (double)_recBlockSize);
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "[CAPT] endpointBufferSizeMS : %3.2f", endpointBufferSizeMS);

    
    
    hr = _ptrClientIn->Start();
    EXIT_ON_ERROR(hr);

    _UnLock();

    
    
    SetEvent(_hCaptureStartedEvent);

    

    while (keepRecording)
    {
        
        DWORD waitResult = WaitForMultipleObjects(2, waitArray, FALSE, 500);
        switch (waitResult)
        {
        case WAIT_OBJECT_0 + 0:        
            keepRecording = false;
            break;
        case WAIT_OBJECT_0 + 1:        
            break;
        case WAIT_TIMEOUT:            
            _ptrClientIn->Stop();
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "capture event timed out after 0.5 seconds");
            goto Exit;
        default:                    
            _ptrClientIn->Stop();
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "unknown wait termination on capture side");
            goto Exit;
        }

        while (keepRecording)
        {
            BYTE *pData = 0;
            UINT32 framesAvailable = 0;
            DWORD flags = 0;
            UINT64 recTime = 0;
            UINT64 recPos = 0;

            _Lock();

            
            
            if (_ptrCaptureClient == NULL || _ptrClientIn == NULL)
            {
                _UnLock();
                WEBRTC_TRACE(kTraceCritical, kTraceAudioDevice, _id,
                    "input state has been modified during unlocked period");
                goto Exit;
            }

            
            
            hr = _ptrCaptureClient->GetBuffer(&pData,           
                                              &framesAvailable, 
                                              &flags,           
                                              &recPos,          
                                              &recTime);        

            if (SUCCEEDED(hr))
            {
                if (AUDCLNT_S_BUFFER_EMPTY == hr)
                {
                    
                    _UnLock();
                    break;
                }

                if (flags & AUDCLNT_BUFFERFLAGS_SILENT)
                {
                    
                    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "AUDCLNT_BUFFERFLAGS_SILENT");
                    pData = NULL;
                }

                assert(framesAvailable != 0);

                if (pData)
                {
                    CopyMemory(&syncBuffer[syncBufIndex*_recAudioFrameSize], pData, framesAvailable*_recAudioFrameSize);
                }
                else
                {
                    ZeroMemory(&syncBuffer[syncBufIndex*_recAudioFrameSize], framesAvailable*_recAudioFrameSize);
                }
                assert(syncBufferSize >= (syncBufIndex*_recAudioFrameSize)+framesAvailable*_recAudioFrameSize);

                
                
                hr = _ptrCaptureClient->ReleaseBuffer(framesAvailable);
                EXIT_ON_ERROR(hr);

                _readSamples += framesAvailable;
                syncBufIndex += framesAvailable;

                QueryPerformanceCounter(&t1);

                
                WebRtc_UWord32 sndCardRecDelay = (WebRtc_UWord32)
                    (((((UINT64)t1.QuadPart * _perfCounterFactor) - recTime)
                        / 10000) + (10*syncBufIndex) / _recBlockSize - 10);
                WebRtc_UWord32 sndCardPlayDelay =
                    static_cast<WebRtc_UWord32>(_sndCardPlayDelay);

                _sndCardRecDelay = sndCardRecDelay;

                while (syncBufIndex >= _recBlockSize)
                {
                    if (_ptrAudioBuffer)
                    {
                        _ptrAudioBuffer->SetRecordedBuffer((const WebRtc_Word8*)syncBuffer, _recBlockSize);

                        _driftAccumulator += _sampleDriftAt48kHz;
                        const WebRtc_Word32 clockDrift =
                            static_cast<WebRtc_Word32>(_driftAccumulator);
                        _driftAccumulator -= clockDrift;

                        _ptrAudioBuffer->SetVQEData(sndCardPlayDelay,
                                                    sndCardRecDelay,
                                                    clockDrift);

                        QueryPerformanceCounter(&t1);    

                        _UnLock();  
                        _ptrAudioBuffer->DeliverRecordedData();
                        _Lock();    

                        QueryPerformanceCounter(&t2);    

                        
                        
                        
                        
                        time = (int)(t2.QuadPart - t1.QuadPart);
                        _avgCPULoad = (float)(_avgCPULoad*.99 + (time + _playAcc) / (double)(_perfCounterFreq.QuadPart));
                        _playAcc = 0;

                        
                        if (_ptrCaptureClient == NULL || _ptrClientIn == NULL)
                        {
                            _UnLock();
                            WEBRTC_TRACE(kTraceCritical, kTraceAudioDevice, _id, "input state has been modified during unlocked period");
                            goto Exit;
                        }
                    }

                    
                    MoveMemory(&syncBuffer[0], &syncBuffer[_recBlockSize*_recAudioFrameSize], (syncBufIndex-_recBlockSize)*_recAudioFrameSize);
                    syncBufIndex -= _recBlockSize;
                    sndCardRecDelay -= 10;
                }

                if (_AGC)
                {
                    WebRtc_UWord32 newMicLevel = _ptrAudioBuffer->NewMicLevel();
                    if (newMicLevel != 0)
                    {
                        
                        
                        WEBRTC_TRACE(kTraceStream, kTraceAudioDevice, _id, "AGC change of volume: new=%u",  newMicLevel);
                        
                        
                        _newMicLevel = newMicLevel;
                        SetEvent(_hSetCaptureVolumeEvent);
                    }
                }
            }
            else
            {
                
                
                
                
                
                WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                    "IAudioCaptureClient::GetBuffer returned AUDCLNT_E_BUFFER_ERROR, hr = 0x%08X",  hr);
                goto Exit;
            }

            _UnLock();
        }
    }

    

    hr = _ptrClientIn->Stop();

Exit:
    if (FAILED(hr))
    {
        _UnLock();
        _ptrClientIn->Stop();
        _TraceCOMError(hr);
    }

    RevertCaptureThreadPriority();

    if (keepRecording)
    {
        if (_ptrClientIn != NULL)
        {
            hr = _ptrClientIn->Stop();
            if (FAILED(hr))
            {
                _TraceCOMError(hr);
            }
            hr = _ptrClientIn->Reset();
            if (FAILED(hr))
            {
                _TraceCOMError(hr);
            }
        }

        
        _recError = 1;
        WEBRTC_TRACE(kTraceError, kTraceUtility, _id, "kRecordingError message posted: capturing thread has ended pre-maturely");
    }
    else
    {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "_Capturing thread is now terminated properly");
    }

    SAFE_RELEASE(_ptrClientIn);
    SAFE_RELEASE(_ptrCaptureClient);

    if (syncBuffer)
    {
        delete [] syncBuffer;
    }

    return (DWORD)hr;
}

int32_t AudioDeviceWindowsCore::EnableBuiltInAEC(bool enable)
{

    if (_recIsInitialized)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
            "Attempt to set Windows AEC with recording already initialized");
        return -1;
    }

    if (_dmo == NULL)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
            "Built-in AEC DMO was not initialized properly at create time");
        return -1;
    }

    _builtInAecEnabled = enable;
    return 0;
}

bool AudioDeviceWindowsCore::BuiltInAECIsEnabled() const
{
    return _builtInAecEnabled;
}

int AudioDeviceWindowsCore::SetDMOProperties()
{
    HRESULT hr = S_OK;
    assert(_dmo != NULL);

    scoped_refptr<IPropertyStore> ps;
    {
        IPropertyStore* ptrPS = NULL;
        hr = _dmo->QueryInterface(IID_IPropertyStore,
                                  reinterpret_cast<void**>(&ptrPS));
        if (FAILED(hr) || ptrPS == NULL)
        {
            _TraceCOMError(hr);
            return -1;
        }
        ps = ptrPS;
        SAFE_RELEASE(ptrPS);
    }

    
    
    if (SetVtI4Property(ps,
                        MFPKEY_WMAAECMA_SYSTEM_MODE,
                        SINGLE_CHANNEL_AEC))
    {
        return -1;
    }

    
    
    if (SetBoolProperty(ps,
                        MFPKEY_WMAAECMA_DMO_SOURCE_MODE,
                        VARIANT_TRUE) == -1)
    {
        return -1;
    }

    
    
    if (SetBoolProperty(ps,
                        MFPKEY_WMAAECMA_FEATURE_MODE,
                        VARIANT_TRUE) == -1)
    {
        return -1;
    }

    
    if (SetBoolProperty(ps,
                        MFPKEY_WMAAECMA_MIC_GAIN_BOUNDER,
                        VARIANT_FALSE) == -1)
    {
        return -1;
    }

    
    
    if (SetVtI4Property(ps,
                        MFPKEY_WMAAECMA_FEATR_NS,
                        0) == -1)
    {
        return -1;
    }

    
    
    
    
    
    
    
    
    
    

    
    
    int inDevIndex = _inputDeviceIndex;
    int outDevIndex = _outputDeviceIndex;
    if (!_usingInputDeviceIndex)
    {
        ERole role = eCommunications;
        if (_inputDevice == AudioDeviceModule::kDefaultDevice)
        {
            role = eConsole;
        }

        if (_GetDefaultDeviceIndex(eCapture, role, &inDevIndex) == -1)
        {
            return -1;
        }
    }

    if (!_usingOutputDeviceIndex)
    {
        ERole role = eCommunications;
        if (_outputDevice == AudioDeviceModule::kDefaultDevice)
        {
            role = eConsole;
        }

        if (_GetDefaultDeviceIndex(eRender, role, &outDevIndex) == -1)
        {
            return -1;
        }
    }

    DWORD devIndex = static_cast<uint32_t>(outDevIndex << 16) +
                     static_cast<uint32_t>(0x0000ffff & inDevIndex);
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
        "Capture device index: %d, render device index: %d",
        inDevIndex, outDevIndex);
    if (SetVtI4Property(ps,
                        MFPKEY_WMAAECMA_DEVICE_INDEXES,
                        devIndex) == -1)
    {
        return -1;
    }

    return 0;
}

int AudioDeviceWindowsCore::SetBoolProperty(IPropertyStore* ptrPS,
                                            REFPROPERTYKEY key,
                                            VARIANT_BOOL value)
{
    PROPVARIANT pv;
    PropVariantInit(&pv);
    pv.vt = VT_BOOL;
    pv.boolVal = value;
    HRESULT hr = ptrPS->SetValue(key, pv);
    PropVariantClear(&pv);
    if (FAILED(hr))
    {
        _TraceCOMError(hr);
        return -1;
    }
    return 0;
}

int AudioDeviceWindowsCore::SetVtI4Property(IPropertyStore* ptrPS,
                                            REFPROPERTYKEY key,
                                            LONG value)
{
    PROPVARIANT pv;
    PropVariantInit(&pv);
    pv.vt = VT_I4;
    pv.lVal = value;
    HRESULT hr = ptrPS->SetValue(key, pv);
    PropVariantClear(&pv);
    if (FAILED(hr))
    {
        _TraceCOMError(hr);
        return -1;
    }
    return 0;
}









WebRtc_Word32 AudioDeviceWindowsCore::_RefreshDeviceList(EDataFlow dir)
{
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "%s", __FUNCTION__);

    HRESULT hr = S_OK;
    IMMDeviceCollection *pCollection = NULL;

    assert(dir == eRender || dir == eCapture);
    assert(_ptrEnumerator != NULL);

    
    hr = _ptrEnumerator->EnumAudioEndpoints(
                           dir,
                           DEVICE_STATE_ACTIVE,
                           &pCollection);
    if (FAILED(hr))
    {
        _TraceCOMError(hr);
        SAFE_RELEASE(pCollection);
        return -1;
    }

    if (dir == eRender)
    {
        SAFE_RELEASE(_ptrRenderCollection);
        _ptrRenderCollection = pCollection;
    }
    else
    {
        SAFE_RELEASE(_ptrCaptureCollection);
        _ptrCaptureCollection = pCollection;
    }

    return 0;
}








WebRtc_Word16 AudioDeviceWindowsCore::_DeviceListCount(EDataFlow dir)
{
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "%s", __FUNCTION__);

    HRESULT hr = S_OK;
    UINT count = 0;

    assert(eRender == dir || eCapture == dir);

    if (eRender == dir && NULL != _ptrRenderCollection)
    {
        hr = _ptrRenderCollection->GetCount(&count);
    }
    else if (NULL != _ptrCaptureCollection)
    {
        hr = _ptrCaptureCollection->GetCount(&count);
    }

    if (FAILED(hr))
    {
        _TraceCOMError(hr);
        return -1;
    }

    return static_cast<WebRtc_Word16> (count);
}












WebRtc_Word32 AudioDeviceWindowsCore::_GetListDeviceName(EDataFlow dir, int index, LPWSTR szBuffer, int bufferLen)
{
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "%s", __FUNCTION__);

    HRESULT hr = S_OK;
    IMMDevice *pDevice = NULL;

    assert(dir == eRender || dir == eCapture);

    if (eRender == dir && NULL != _ptrRenderCollection)
    {
        hr = _ptrRenderCollection->Item(index, &pDevice);
    }
    else if (NULL != _ptrCaptureCollection)
    {
        hr = _ptrCaptureCollection->Item(index, &pDevice);
    }

    if (FAILED(hr))
    {
        _TraceCOMError(hr);
        SAFE_RELEASE(pDevice);
        return -1;
    }

    WebRtc_Word32 res = _GetDeviceName(pDevice, szBuffer, bufferLen);
    SAFE_RELEASE(pDevice);
    return res;
}










WebRtc_Word32 AudioDeviceWindowsCore::_GetDefaultDeviceName(EDataFlow dir, ERole role, LPWSTR szBuffer, int bufferLen)
{
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "%s", __FUNCTION__);

    HRESULT hr = S_OK;
    IMMDevice *pDevice = NULL;

    assert(dir == eRender || dir == eCapture);
    assert(role == eConsole || role == eCommunications);
    assert(_ptrEnumerator != NULL);

    hr = _ptrEnumerator->GetDefaultAudioEndpoint(
                           dir,
                           role,
                           &pDevice);

    if (FAILED(hr))
    {
        _TraceCOMError(hr);
        SAFE_RELEASE(pDevice);
        return -1;
    }

    WebRtc_Word32 res = _GetDeviceName(pDevice, szBuffer, bufferLen);
    SAFE_RELEASE(pDevice);
    return res;
}












WebRtc_Word32 AudioDeviceWindowsCore::_GetListDeviceID(EDataFlow dir, int index, LPWSTR szBuffer, int bufferLen)
{
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "%s", __FUNCTION__);

    HRESULT hr = S_OK;
    IMMDevice *pDevice = NULL;

    assert(dir == eRender || dir == eCapture);

    if (eRender == dir && NULL != _ptrRenderCollection)
    {
        hr = _ptrRenderCollection->Item(index, &pDevice);
    }
    else if (NULL != _ptrCaptureCollection)
    {
        hr = _ptrCaptureCollection->Item(index, &pDevice);
    }

    if (FAILED(hr))
    {
        _TraceCOMError(hr);
        SAFE_RELEASE(pDevice);
        return -1;
    }

    WebRtc_Word32 res = _GetDeviceID(pDevice, szBuffer, bufferLen);
    SAFE_RELEASE(pDevice);
    return res;
}










WebRtc_Word32 AudioDeviceWindowsCore::_GetDefaultDeviceID(EDataFlow dir, ERole role, LPWSTR szBuffer, int bufferLen)
{
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "%s", __FUNCTION__);

    HRESULT hr = S_OK;
    IMMDevice *pDevice = NULL;

    assert(dir == eRender || dir == eCapture);
    assert(role == eConsole || role == eCommunications);
    assert(_ptrEnumerator != NULL);

    hr = _ptrEnumerator->GetDefaultAudioEndpoint(
                           dir,
                           role,
                           &pDevice);

    if (FAILED(hr))
    {
        _TraceCOMError(hr);
        SAFE_RELEASE(pDevice);
        return -1;
    }

    WebRtc_Word32 res = _GetDeviceID(pDevice, szBuffer, bufferLen);
    SAFE_RELEASE(pDevice);
    return res;
}

WebRtc_Word32 AudioDeviceWindowsCore::_GetDefaultDeviceIndex(EDataFlow dir,
                                                             ERole role,
                                                             int* index)
{
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "%s", __FUNCTION__);

    HRESULT hr = S_OK;
    WCHAR szDefaultDeviceID[MAX_PATH] = {0};
    WCHAR szDeviceID[MAX_PATH] = {0};

    const size_t kDeviceIDLength = sizeof(szDeviceID)/sizeof(szDeviceID[0]);
    assert(kDeviceIDLength ==
        sizeof(szDefaultDeviceID) / sizeof(szDefaultDeviceID[0]));

    if (_GetDefaultDeviceID(dir,
                            role,
                            szDefaultDeviceID,
                            kDeviceIDLength) == -1)
    {
        return -1;
    }

    IMMDeviceCollection* collection = _ptrCaptureCollection;
    if (dir == eRender)
    {
        collection = _ptrRenderCollection;
    }

    if (!collection)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
            "Device collection not valid");
        return -1;
    }

    UINT count = 0;
    hr = collection->GetCount(&count);
    if (FAILED(hr))
    {
        _TraceCOMError(hr);
        return -1;
    }

    *index = -1;
    for (UINT i = 0; i < count; i++)
    {
        memset(szDeviceID, 0, sizeof(szDeviceID));
        scoped_refptr<IMMDevice> device;
        {
            IMMDevice* ptrDevice = NULL;
            hr = collection->Item(i, &ptrDevice);
            if (FAILED(hr) || ptrDevice == NULL)
            {
                _TraceCOMError(hr);
                return -1;
            }
            device = ptrDevice;
            SAFE_RELEASE(ptrDevice);
        }

        if (_GetDeviceID(device, szDeviceID, kDeviceIDLength) == -1)
        {
           return -1;
        }

        if (wcsncmp(szDefaultDeviceID, szDeviceID, kDeviceIDLength) == 0)
        {
            
            *index = i;
            break;
        }

    }

    if (*index == -1)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
            "Unable to find collection index for default device");
        return -1;
    }

    return 0;
}





WebRtc_Word32 AudioDeviceWindowsCore::_GetDeviceName(IMMDevice* pDevice,
                                                     LPWSTR pszBuffer,
                                                     int bufferLen)
{
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "%s", __FUNCTION__);

    static const WCHAR szDefault[] = L"<Device not available>";

    HRESULT hr = E_FAIL;
    IPropertyStore *pProps = NULL;
    PROPVARIANT varName;

    assert(pszBuffer != NULL);
    assert(bufferLen > 0);

    if (pDevice != NULL)
    {
        hr = pDevice->OpenPropertyStore(STGM_READ, &pProps);
        if (FAILED(hr))
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                "IMMDevice::OpenPropertyStore failed, hr = 0x%08X", hr);
        }
    }

    
    PropVariantInit(&varName);

    if (SUCCEEDED(hr))
    {
        
        hr = pProps->GetValue(PKEY_Device_FriendlyName, &varName);
        if (FAILED(hr))
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                "IPropertyStore::GetValue failed, hr = 0x%08X", hr);
        }
    }

    if ((SUCCEEDED(hr)) && (VT_EMPTY == varName.vt))
    {
        hr = E_FAIL;
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
            "IPropertyStore::GetValue returned no value, hr = 0x%08X", hr);
    }

    if ((SUCCEEDED(hr)) && (VT_LPWSTR != varName.vt))
    {
        
        hr = E_UNEXPECTED;
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
            "IPropertyStore::GetValue returned unexpected type, hr = 0x%08X", hr);
    }

    if (SUCCEEDED(hr) && (varName.pwszVal != NULL))
    {
        
        wcsncpy_s(pszBuffer, bufferLen, varName.pwszVal, _TRUNCATE);
    }
    else
    {
        
        wcsncpy_s(pszBuffer, bufferLen, szDefault, _TRUNCATE);
    }

    PropVariantClear(&varName);
    SAFE_RELEASE(pProps);

    return 0;
}





WebRtc_Word32 AudioDeviceWindowsCore::_GetDeviceID(IMMDevice* pDevice, LPWSTR pszBuffer, int bufferLen)
{
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "%s", __FUNCTION__);

    static const WCHAR szDefault[] = L"<Device not available>";

    HRESULT hr = E_FAIL;
    LPWSTR pwszID = NULL;

    assert(pszBuffer != NULL);
    assert(bufferLen > 0);

    if (pDevice != NULL)
    {
        hr = pDevice->GetId(&pwszID);
    }

    if (hr == S_OK)
    {
        
        wcsncpy_s(pszBuffer, bufferLen, pwszID, _TRUNCATE);
    }
    else
    {
        
        wcsncpy_s(pszBuffer, bufferLen, szDefault, _TRUNCATE);
    }

    CoTaskMemFree(pwszID);
    return 0;
}





WebRtc_Word32 AudioDeviceWindowsCore::_GetDefaultDevice(EDataFlow dir, ERole role, IMMDevice** ppDevice)
{
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "%s", __FUNCTION__);

    HRESULT hr(S_OK);

    assert(_ptrEnumerator != NULL);

    hr = _ptrEnumerator->GetDefaultAudioEndpoint(
                                   dir,
                                   role,
                                   ppDevice);
    if (FAILED(hr))
    {
        _TraceCOMError(hr);
        return -1;
    }

    return 0;
}





WebRtc_Word32 AudioDeviceWindowsCore::_GetListDevice(EDataFlow dir, int index, IMMDevice** ppDevice)
{
    HRESULT hr(S_OK);

    assert(_ptrEnumerator != NULL);

    IMMDeviceCollection *pCollection = NULL;

    hr = _ptrEnumerator->EnumAudioEndpoints(
                               dir,
                               DEVICE_STATE_ACTIVE,        
                               &pCollection);
    if (FAILED(hr))
    {
        _TraceCOMError(hr);
        SAFE_RELEASE(pCollection);
        return -1;
    }

    hr = pCollection->Item(
                        index,
                        ppDevice);
    if (FAILED(hr))
    {
        _TraceCOMError(hr);
        SAFE_RELEASE(pCollection);
        return -1;
    }

    return 0;
}





WebRtc_Word32 AudioDeviceWindowsCore::_EnumerateEndpointDevicesAll(EDataFlow dataFlow) const
{
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "%s", __FUNCTION__);

    assert(_ptrEnumerator != NULL);

    HRESULT hr = S_OK;
    IMMDeviceCollection *pCollection = NULL;
    IMMDevice *pEndpoint = NULL;
    IPropertyStore *pProps = NULL;
    IAudioEndpointVolume* pEndpointVolume = NULL;
    LPWSTR pwszID = NULL;

    
    
    
    hr = _ptrEnumerator->EnumAudioEndpoints(
                                 dataFlow,            
                                 DEVICE_STATE_ACTIVE | DEVICE_STATE_DISABLED | DEVICE_STATE_UNPLUGGED,
                                 &pCollection);        

    EXIT_ON_ERROR(hr);

    

    UINT count = 0;

    
    hr = pCollection->GetCount(&count);
    EXIT_ON_ERROR(hr);
    if (dataFlow == eRender)
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "#rendering endpoint devices (counting all): %u", count);
    else if (dataFlow == eCapture)
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "#capturing endpoint devices (counting all): %u", count);

    if (count == 0)
    {
        return 0;
    }

    
    for (ULONG i = 0; i < count; i++)
    {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "Endpoint %d:", i);

        
        
        hr = pCollection->Item(
                            i,
                            &pEndpoint);
        EXIT_ON_ERROR(hr);

        

        
        hr = pEndpoint->GetId(&pwszID);
        EXIT_ON_ERROR(hr);
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "ID string    : %S", pwszID);

        
        
        hr = pEndpoint->OpenPropertyStore(
                          STGM_READ,
                          &pProps);
        EXIT_ON_ERROR(hr);

        

        PROPVARIANT varName;
        
        PropVariantInit(&varName);

        
        
        hr = pProps->GetValue(
                       PKEY_Device_FriendlyName,
                       &varName);
        EXIT_ON_ERROR(hr);
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "friendly name: \"%S\"", varName.pwszVal);

        
        DWORD dwState;
        hr = pEndpoint->GetState(&dwState);
        EXIT_ON_ERROR(hr);
        if (dwState & DEVICE_STATE_ACTIVE)
            WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "state (0x%x)  : *ACTIVE*", dwState);
        if (dwState & DEVICE_STATE_DISABLED)
            WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "state (0x%x)  : DISABLED", dwState);
        if (dwState & DEVICE_STATE_NOTPRESENT)
            WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "state (0x%x)  : NOTPRESENT", dwState);
        if (dwState & DEVICE_STATE_UNPLUGGED)
            WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "state (0x%x)  : UNPLUGGED", dwState);

        
        DWORD dwHwSupportMask = 0;
        hr = pEndpoint->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL,
                               NULL, (void**)&pEndpointVolume);
        EXIT_ON_ERROR(hr);
        hr = pEndpointVolume->QueryHardwareSupport(&dwHwSupportMask);
        EXIT_ON_ERROR(hr);
        if (dwHwSupportMask & ENDPOINT_HARDWARE_SUPPORT_VOLUME)
            
            WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "hwmask (0x%x) : HARDWARE_SUPPORT_VOLUME", dwHwSupportMask);
        if (dwHwSupportMask & ENDPOINT_HARDWARE_SUPPORT_MUTE)
            
            WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "hwmask (0x%x) : HARDWARE_SUPPORT_MUTE", dwHwSupportMask);
        if (dwHwSupportMask & ENDPOINT_HARDWARE_SUPPORT_METER)
            
            WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "hwmask (0x%x) : HARDWARE_SUPPORT_METER", dwHwSupportMask);

        
        UINT nChannelCount(0);
        hr = pEndpointVolume->GetChannelCount(
                                &nChannelCount);
        EXIT_ON_ERROR(hr);
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "#channels    : %u", nChannelCount);

        if (dwHwSupportMask & ENDPOINT_HARDWARE_SUPPORT_VOLUME)
        {
            
            float fLevelMinDB(0.0);
            float fLevelMaxDB(0.0);
            float fVolumeIncrementDB(0.0);
            hr = pEndpointVolume->GetVolumeRange(
                                    &fLevelMinDB,
                                    &fLevelMaxDB,
                                    &fVolumeIncrementDB);
            EXIT_ON_ERROR(hr);
            WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "volume range : %4.2f (min), %4.2f (max), %4.2f (inc) [dB]",
                fLevelMinDB, fLevelMaxDB, fVolumeIncrementDB);

            
            
            
            
            
            int n = (int)((fLevelMaxDB-fLevelMinDB)/fVolumeIncrementDB);
            WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "#intervals   : %d", n);

            
            
            
            
            
            
            UINT nStep(0);
            UINT nStepCount(0);
            hr = pEndpointVolume->GetVolumeStepInfo(
                                    &nStep,
                                    &nStepCount);
            EXIT_ON_ERROR(hr);
            WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "volume steps : %d (nStep), %d (nStepCount)", nStep, nStepCount);
        }

        CoTaskMemFree(pwszID);
        pwszID = NULL;
        PropVariantClear(&varName);
        SAFE_RELEASE(pProps);
        SAFE_RELEASE(pEndpoint);
        SAFE_RELEASE(pEndpointVolume);
    }
    SAFE_RELEASE(pCollection);
    return 0;

Exit:
    _TraceCOMError(hr);
    CoTaskMemFree(pwszID);
    pwszID = NULL;
    SAFE_RELEASE(pCollection);
    SAFE_RELEASE(pEndpoint);
    SAFE_RELEASE(pEndpointVolume);
    SAFE_RELEASE(pProps);
    return -1;
}





void AudioDeviceWindowsCore::_TraceCOMError(HRESULT hr) const
{
    TCHAR buf[MAXERRORLENGTH];
    TCHAR errorText[MAXERRORLENGTH];

    const DWORD dwFlags = FORMAT_MESSAGE_FROM_SYSTEM |
                          FORMAT_MESSAGE_IGNORE_INSERTS;
    const DWORD dwLangID = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);

    
    
    DWORD messageLength = ::FormatMessageW(dwFlags,
                                           0,
                                           hr,
                                           dwLangID,
                                           errorText,
                                           MAXERRORLENGTH,
                                           NULL);

    assert(messageLength <= MAXERRORLENGTH);

    
    for (; messageLength && ::isspace(errorText[messageLength - 1]);
         --messageLength)
    {
        errorText[messageLength - 1] = '\0';
    }

    WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
        "Core Audio method failed (hr=0x%x)", hr);
    StringCchPrintf(buf, MAXERRORLENGTH, TEXT("Error details: "));
    StringCchCat(buf, MAXERRORLENGTH, errorText);
    WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id, "%s", WideToUTF8(buf));
}





void AudioDeviceWindowsCore::_SetThreadName(DWORD dwThreadID, LPCSTR szThreadName)
{
    
    

    THREADNAME_INFO info;
    info.dwType = 0x1000;
    info.szName = szThreadName;
    info.dwThreadID = dwThreadID;
    info.dwFlags = 0;

    __try
    {
        RaiseException( 0x406D1388, 0, sizeof(info)/sizeof(DWORD), (ULONG_PTR *)&info );
    }
    __except (EXCEPTION_CONTINUE_EXECUTION)
    {
    }
}





void AudioDeviceWindowsCore::_Get44kHzDrift()
{
    
    
    
    
    _sampleDriftAt48kHz = 0;
    _driftAccumulator = 0;

    if (_playSampleRate == 44000 && _recSampleRate != 44000)
    {
        _sampleDriftAt48kHz = 480.0f/440;
    }
    else if(_playSampleRate != 44000 && _recSampleRate == 44000)
    {
        _sampleDriftAt48kHz = -480.0f/441;
    }
}





char* AudioDeviceWindowsCore::WideToUTF8(const TCHAR* src) const {
#ifdef UNICODE
    const size_t kStrLen = sizeof(_str);
    memset(_str, 0, kStrLen);
    
    int required_size = WideCharToMultiByte(CP_UTF8, 0, src, -1, _str, 0, 0, 0);
    if (required_size <= kStrLen)
    {
        
        if (WideCharToMultiByte(CP_UTF8, 0, src, -1, _str, kStrLen, 0, 0) == 0)
            memset(_str, 0, kStrLen);
    }
    return _str;
#else
    return const_cast<char*>(src);
#endif
}

}  

#endif  
