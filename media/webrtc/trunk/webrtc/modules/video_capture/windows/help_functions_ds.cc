









#include <initguid.h>  
                       
                       

#include "help_functions_ds.h"

#include <cguid.h>

namespace webrtc
{
namespace videocapturemodule
{

LONGLONG GetMaxOfFrameArray(LONGLONG *maxFps, long size)
{
    LONGLONG maxFPS = maxFps[0];
    for (int i = 0; i < size; i++)
    {
        if (maxFPS > maxFps[i])
            maxFPS = maxFps[i];
    }
    return maxFPS;
}

IPin* GetInputPin(IBaseFilter* filter)
{
    HRESULT hr;
    IPin* pin = NULL;
    IEnumPins* pPinEnum = NULL;
    filter->EnumPins(&pPinEnum);
    if (pPinEnum == NULL)
    {
        return NULL;
    }

    
    hr = pPinEnum->Reset(); 

    while (S_OK == pPinEnum->Next(1, &pin, NULL))
    {
        PIN_DIRECTION pPinDir;
        pin->QueryDirection(&pPinDir);
        if (PINDIR_INPUT == pPinDir) 
        {
            IPin* tempPin = NULL;
            if (S_OK != pin->ConnectedTo(&tempPin)) 
            {
                pPinEnum->Release();
                return pin;
            }
        }
        pin->Release();
    }
    pPinEnum->Release();
    return NULL;
}

IPin* GetOutputPin(IBaseFilter* filter, REFGUID Category)
{
    HRESULT hr;
    IPin* pin = NULL;
    IEnumPins* pPinEnum = NULL;
    filter->EnumPins(&pPinEnum);
    if (pPinEnum == NULL)
    {
        return NULL;
    }
    
    hr = pPinEnum->Reset(); 
    while (S_OK == pPinEnum->Next(1, &pin, NULL))
    {
        PIN_DIRECTION pPinDir;
        pin->QueryDirection(&pPinDir);
        if (PINDIR_OUTPUT == pPinDir) 
        {
            if (Category == GUID_NULL || PinMatchesCategory(pin, Category))
            {
                pPinEnum->Release();
                return pin;
            }
        }
        pin->Release();
        pin = NULL;
    }
    pPinEnum->Release();
    return NULL;
}

BOOL PinMatchesCategory(IPin *pPin, REFGUID Category)
{
    BOOL bFound = FALSE;
    IKsPropertySet *pKs = NULL;
    HRESULT hr = pPin->QueryInterface(IID_PPV_ARGS(&pKs));
    if (SUCCEEDED(hr))
    {
        GUID PinCategory;
        DWORD cbReturned;
        hr = pKs->Get(AMPROPSETID_Pin, AMPROPERTY_PIN_CATEGORY, NULL, 0, &PinCategory,
                      sizeof(GUID), &cbReturned);
        if (SUCCEEDED(hr) && (cbReturned == sizeof(GUID)))
        {
            bFound = (PinCategory == Category);
        }
        pKs->Release();
    }
    return bFound;
}
} 
} 

