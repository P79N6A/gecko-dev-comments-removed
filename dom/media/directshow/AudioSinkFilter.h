





#if !defined(AudioSinkFilter_h_)
#define AudioSinkFilter_h_

#include "BaseFilter.h"
#include "DirectShowUtils.h"
#include "nsAutoPtr.h"
#include "mozilla/RefPtr.h"

namespace mozilla {

class AudioSinkInputPin;
class SampleSink;






class AudioSinkFilter: public mozilla::media::BaseFilter,
                       public IMediaSeeking
{

public:
  AudioSinkFilter(const wchar_t* aObjectName, HRESULT* aOutResult);
  virtual ~AudioSinkFilter();

  
  SampleSink* GetSampleSink();

  
  STDMETHODIMP QueryInterface(REFIID aIId, void **aInterface);
  STDMETHODIMP_(ULONG) AddRef();
  STDMETHODIMP_(ULONG) Release();

  
  
  int GetPinCount ();
  mozilla::media::BasePin* GetPin ( IN int Index);
  STDMETHODIMP Pause ();
  STDMETHODIMP Stop ();
  STDMETHODIMP GetClassID ( OUT CLSID * pCLSID);
  STDMETHODIMP Run(REFERENCE_TIME tStart);
  

  
  
  
  STDMETHODIMP GetCapabilities(DWORD* aCapabilities);
  STDMETHODIMP CheckCapabilities(DWORD* aCapabilities);
  STDMETHODIMP IsFormatSupported(const GUID* aFormat);
  STDMETHODIMP QueryPreferredFormat(GUID* aFormat);
  STDMETHODIMP GetTimeFormat(GUID* aFormat);
  STDMETHODIMP IsUsingTimeFormat(const GUID* aFormat);
  STDMETHODIMP SetTimeFormat(const GUID* aFormat);
  STDMETHODIMP GetDuration(LONGLONG* pDuration);
  STDMETHODIMP GetStopPosition(LONGLONG* pStop);
  STDMETHODIMP GetCurrentPosition(LONGLONG* aCurrent);
  STDMETHODIMP ConvertTimeFormat(LONGLONG* aTarget,
                                 const GUID* aTargetFormat,
                                 LONGLONG aSource,
                                 const GUID* aSourceFormat);
  STDMETHODIMP SetPositions(LONGLONG* aCurrent,
                            DWORD aCurrentFlags,
                            LONGLONG* aStop,
                            DWORD aStopFlags);
  STDMETHODIMP GetPositions(LONGLONG* aCurrent,
                            LONGLONG* aStop);
  STDMETHODIMP GetAvailable(LONGLONG* aEarliest,
                            LONGLONG* aLatest);
  STDMETHODIMP SetRate(double aRate);
  STDMETHODIMP GetRate(double* aRate);
  STDMETHODIMP GetPreroll(LONGLONG* aPreroll);

  
  
  static IUnknown * CreateInstance (IN LPUNKNOWN punk, OUT HRESULT * phr);

private:
  CriticalSection mFilterCritSec;

  
  
  
  nsAutoPtr<AudioSinkInputPin> mInputPin;
};

} 
#endif 
