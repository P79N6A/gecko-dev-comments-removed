

















#ifndef profiler_IntelPowerGadget_h
#define profiler_IntelPowerGadget_h

#ifdef _MSC_VER
typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
#else
#include <stdint.h>
#endif
#include "prlink.h"

typedef int (*IPGInitialize) ();
typedef int (*IPGGetNumNodes) (int *nNodes);
typedef int (*IPGGetNumMsrs) (int *nMsr);
typedef int (*IPGGetMsrName) (int iMsr, wchar_t *szName);
typedef int (*IPGGetMsrFunc) (int iMsr, int *pFuncID);
typedef int (*IPGReadMSR) (int iNode, unsigned int address, uint64_t *value);
typedef int (*IPGWriteMSR) (int iNode, unsigned int address, uint64_t value);
typedef int (*IPGGetIAFrequency) (int iNode, int *freqInMHz);
typedef int (*IPGGetTDP) (int iNode, double *TDP);
typedef int (*IPGGetMaxTemperature) (int iNode, int *degreeC);
typedef int (*IPGGetThresholds) (int iNode, int *degree1C, int *degree2C);
typedef int (*IPGGetTemperature) (int iNode, int *degreeC);
typedef int (*IPGReadSample) ();
typedef int (*IPGGetSysTime) (void *pSysTime);
typedef int (*IPGGetRDTSC) (uint64_t *pTSC);
typedef int (*IPGGetTimeInterval) (double *pOffset);
typedef int (*IPGGetBaseFrequency) (int iNode, double *pBaseFrequency);
typedef int (*IPGGetPowerData) (int iNode, int iMSR, double *pResult, int *nResult);
typedef int (*IPGStartLog) (wchar_t *szFileName);
typedef int (*IPGStopLog) ();

#if defined(__x86_64__) || defined(__x86_64) || defined(_M_AMD64)
#define PG_LIBRARY_NAME "EnergyLib64"
#else
#define PG_LIBRARY_NAME "EnergyLib32"
#endif


class IntelPowerGadget
{
public:

    IntelPowerGadget();
    ~IntelPowerGadget();

    
    bool Init();

    
    int GetNumberNodes();

    
    int GetNumberMsrs();

    
    int GetCPUFrequency(int);

    
    double GetTdp(int);

    
    int GetMaxTemp(int);

    
    
    int GetTemp(int);

    
    
    int TakeSample();

    
    uint64_t GetRdtsc();

    
    
    double GetInterval();

    
    double GetCPUBaseFrequency(int node);

    
    
    double GetTotalPackagePowerInWatts();
    double GetPackagePowerInWatts(int node);

    
    
    
    double GetTotalCPUPowerInWatts();
    double GetCPUPowerInWatts(int node);

    
    
    
    double GetTotalGPUPowerInWatts();
    double GetGPUPowerInWatts(int node);

private:

    PRLibrary *libpowergadget;
    IPGInitialize Initialize;
    IPGGetNumNodes GetNumNodes;
    IPGGetNumMsrs GetNumMsrs;
    IPGGetMsrName GetMsrName;
    IPGGetMsrFunc GetMsrFunc;
    IPGReadMSR ReadMSR;
    IPGWriteMSR WriteMSR;
    IPGGetIAFrequency GetIAFrequency;
    IPGGetTDP GetTDP;
    IPGGetMaxTemperature GetMaxTemperature;
    IPGGetThresholds GetThresholds;
    IPGGetTemperature GetTemperature;
    IPGReadSample ReadSample;
    IPGGetSysTime GetSysTime;
    IPGGetRDTSC GetRDTSC;
    IPGGetTimeInterval GetTimeInterval;
    IPGGetBaseFrequency GetBaseFrequency;
    IPGGetPowerData GetPowerData;
    IPGStartLog StartLog;
    IPGStopLog StopLog;

    int packageMSR;
    int cpuMSR;
    int freqMSR;
    int tempMSR;
};

#endif 
