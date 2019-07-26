




#ifndef _DirectShowUtils_h_
#define _DirectShowUtils_h_

#include <stdint.h>
#include "dshow.h"
#include "DShowTools.h"
#include "prlog.h"

namespace mozilla {



class Signal {
public:

  Signal(CriticalSection* aLock)
    : mLock(aLock)
  {
    CriticalSectionAutoEnter lock(*mLock);
    mEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
  }

  ~Signal() {
    CriticalSectionAutoEnter lock(*mLock);
    CloseHandle(mEvent);
  }

  
  void Notify() {
    SetEvent(mEvent);
  }

  
  HRESULT Wait() {
    mLock->Leave();
    DWORD result = WaitForSingleObject(mEvent, INFINITE);
    mLock->Enter();
    return result == WAIT_OBJECT_0 ? S_OK : E_FAIL;
  }

private:
  CriticalSection* mLock;
  HANDLE mEvent;
};

HRESULT
AddGraphToRunningObjectTable(IUnknown *aUnkGraph, DWORD *aOutRotRegister);

void
RemoveGraphFromRunningObjectTable(DWORD aRotRegister);

const char*
GetGraphNotifyString(long evCode);


HRESULT
CreateAndAddFilter(IGraphBuilder* aGraph,
                   REFGUID aFilterClsId,
                   LPCWSTR aFilterName,
                   IBaseFilter **aOutFilter);

HRESULT
AddMP3DMOWrapperFilter(IGraphBuilder* aGraph,
                       IBaseFilter **aOutFilter);



HRESULT
ConnectFilters(IGraphBuilder* aGraph,
               IBaseFilter* aOutputFilter,
               IBaseFilter* aInputFilter);

HRESULT
MatchUnconnectedPin(IPin* aPin,
                    PIN_DIRECTION aPinDir,
                    bool *aOutMatches);



inline int64_t
UsecsToRefTime(const int64_t aUsecs)
{
  return aUsecs * 10;
}



inline int64_t
RefTimeToUsecs(const int64_t hRefTime)
{
  return hRefTime / 10;
}



inline double
RefTimeToSeconds(const REFERENCE_TIME aRefTime)
{
  return double(aRefTime) / 10000000;
}


#if defined(PR_LOGGING)
const char*
GetDirectShowGuidName(const GUID& aGuid);
#endif

} 

#endif
