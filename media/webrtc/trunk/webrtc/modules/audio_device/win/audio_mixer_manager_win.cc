









#include "audio_mixer_manager_win.h"
#include "trace.h"

#include <strsafe.h>    
#include <cassert>      

#ifdef _WIN32


#pragma warning(disable:4312)
#endif


#ifndef WAVE_MAPPED_kDefaultCommunicationDevice
#define  WAVE_MAPPED_kDefaultCommunicationDevice   0x0010
#endif

namespace webrtc {





AudioMixerManager::AudioMixerManager(const WebRtc_Word32 id) :
    _critSect(*CriticalSectionWrapper::CreateCriticalSection()),
    _id(id),
    _inputMixerHandle(NULL),
    _outputMixerHandle(NULL)
{
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, _id, "%s constructed", __FUNCTION__);
    ClearSpeakerState();
    ClearMicrophoneState();
}

AudioMixerManager::~AudioMixerManager()
{
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, _id, "%s destructed", __FUNCTION__);

    Close();

    delete &_critSect;
}









WebRtc_Word32 AudioMixerManager::Close()
{
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "%s", __FUNCTION__);

    CriticalSectionScoped lock(&_critSect);

    if (_outputMixerHandle != NULL)
    {
        mixerClose(_outputMixerHandle);
        _outputMixerHandle = NULL;
    }
    if (_inputMixerHandle != NULL)
    {
        mixerClose(_inputMixerHandle);
        _inputMixerHandle = NULL;
    }
    return 0;

}





WebRtc_Word32 AudioMixerManager::CloseSpeaker()
{
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "%s", __FUNCTION__);

    CriticalSectionScoped lock(&_critSect);

    if (_outputMixerHandle == NULL)
    {
        return -1;
    }

    ClearSpeakerState(_outputMixerID);

    mixerClose(_outputMixerHandle);
    _outputMixerHandle = NULL;

    return 0;
}





WebRtc_Word32 AudioMixerManager::CloseMicrophone()
{
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "%s", __FUNCTION__);

    CriticalSectionScoped lock(&_critSect);

    if (_inputMixerHandle == NULL)
    {
        return -1;
    }

    ClearMicrophoneState(_inputMixerID);

    mixerClose(_inputMixerHandle);
    _inputMixerHandle = NULL;

    return 0;
}





WebRtc_Word32 AudioMixerManager::EnumerateAll()
{
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "%s", __FUNCTION__);

    UINT nDevices = mixerGetNumDevs();
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "#mixer devices: %u", nDevices);

    MIXERCAPS    caps;
    MIXERLINE    destLine;
    MIXERLINE    sourceLine;
    MIXERCONTROL controlArray[MAX_NUMBER_OF_LINE_CONTROLS];

    UINT mixId(0);
    UINT destId(0);
    UINT sourceId(0);

    for (mixId = 0; mixId < nDevices; mixId++)
    {
        if (!GetCapabilities(mixId, caps, true))
            continue;

        for (destId = 0; destId < caps.cDestinations; destId++)
        {
            GetDestinationLineInfo(mixId, destId, destLine, true);
            GetAllLineControls(mixId, destLine, controlArray, true);

            for (sourceId = 0; sourceId < destLine.cConnections; sourceId++)
            {
                GetSourceLineInfo(mixId, destId, sourceId, sourceLine, true);
                GetAllLineControls(mixId, sourceLine, controlArray, true);
            }
        }
    }

    return 0;
}





WebRtc_Word32 AudioMixerManager::EnumerateSpeakers()
{
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "%s", __FUNCTION__);

    UINT nDevices = mixerGetNumDevs();
    if (nDevices > MAX_NUMBER_MIXER_DEVICES)
    {
        assert(false);
        return -1;
    }
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "#mixer devices: %u", nDevices);

    MIXERCAPS    caps;
    MIXERLINE    destLine;
    MIXERCONTROL controlArray[MAX_NUMBER_OF_LINE_CONTROLS];

    UINT mixId(0);
    UINT destId(0);

    ClearSpeakerState();

    
    for (mixId = 0; mixId < nDevices; mixId++)
    {
        
        GetCapabilities(mixId, caps);
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "[mixerID=%d] %s: ", mixId, WideToUTF8(caps.szPname));
        
        for (destId = 0; destId < caps.cDestinations; destId++)
        {
            GetDestinationLineInfo(mixId, destId, destLine);
            if ((destLine.cControls == 0)                         ||    
                (destLine.cConnections == 0)                      ||    
                (destLine.fdwLine & MIXERLINE_LINEF_DISCONNECTED) ||    
                !(destLine.fdwLine & MIXERLINE_LINEF_ACTIVE))           
            {
                
                continue;
            }
            if ((destLine.dwComponentType == MIXERLINE_COMPONENTTYPE_DST_SPEAKERS) ||
                (destLine.dwComponentType == MIXERLINE_COMPONENTTYPE_DST_HEADPHONES))
            {
                WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "found valid speaker/headphone (name: %s, ID: %u)", WideToUTF8(destLine.szName), destLine.dwLineID);
                _speakerState[mixId].dwLineID = destLine.dwLineID;
                _speakerState[mixId].speakerIsValid = true;
                
                GetAllLineControls(mixId, destLine, controlArray);
                for (UINT c = 0; c < destLine.cControls; c++)
                {
                    if (controlArray[c].dwControlType == MIXERCONTROL_CONTROLTYPE_VOLUME)
                    {
                        _speakerState[mixId].dwVolumeControlID = controlArray[c].dwControlID;
                        _speakerState[mixId].volumeControlIsValid = true;
                        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "found volume control (name: %s, ID: %u)", WideToUTF8(controlArray[c].szName), controlArray[c].dwControlID);
                    }
                    else if (controlArray[c].dwControlType == MIXERCONTROL_CONTROLTYPE_MUTE)
                    {
                        _speakerState[mixId].dwMuteControlID = controlArray[c].dwControlID;
                        _speakerState[mixId].muteControlIsValid = true;
                        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "found mute control (name: %s, ID: %u)", WideToUTF8(controlArray[c].szName), controlArray[c].dwControlID);
                    }
                }
                break;
            }
        }
        if (!SpeakerIsValid(mixId))
        {
            WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "unable to find a valid speaker destination line", mixId);
        }
    }

    if (ValidSpeakers() == 0)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "failed to locate any valid speaker line");
        return -1;
    }

    return 0;
}





WebRtc_Word32 AudioMixerManager::EnumerateMicrophones()
{
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "%s", __FUNCTION__);

    UINT nDevices = mixerGetNumDevs();
    if (nDevices > MAX_NUMBER_MIXER_DEVICES)
    {
        assert(false);
        return -1;
    }
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "#mixer devices: %u", nDevices);

    MIXERCAPS    caps;
    MIXERLINE    destLine;
    MIXERLINE    sourceLine;
    MIXERCONTROL controlArray[MAX_NUMBER_OF_LINE_CONTROLS];

    UINT mixId(0);
    UINT destId(0);

    ClearMicrophoneState();

    
    for (mixId = 0; mixId < nDevices; mixId++)
    {
        
        GetCapabilities(mixId, caps);
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "[mixerID=%d] %s: ", mixId, WideToUTF8(caps.szPname));
        
        for (destId = 0; destId < caps.cDestinations; destId++)
        {
            GetDestinationLineInfo(mixId, destId, destLine);

            if ((destLine.cConnections == 0)                      ||    
                (destLine.fdwLine & MIXERLINE_LINEF_DISCONNECTED) ||    
               !(destLine.fdwLine & MIXERLINE_LINEF_ACTIVE))            
            {
                
                
                continue;
            }

            if (destLine.dwComponentType == MIXERLINE_COMPONENTTYPE_DST_WAVEIN)
            {
                WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "found valid Wave In destination (name: %s, ID: %u)", WideToUTF8(destLine.szName), destLine.dwLineID);
                _microphoneState[mixId].dwLineID = destLine.dwLineID;
                _microphoneState[mixId].microphoneIsValid = true;

                
                if (!GetAllLineControls(mixId, destLine, controlArray))
                {
                    
                    
                    
                    

                    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, 
                    "this destination has no controls => must control source");
                    for (DWORD sourceId = 0; sourceId < destLine.cConnections; sourceId++)
                    {
                        GetSourceLineInfo(mixId, destId, sourceId, sourceLine, false); 
                        if (sourceLine.dwComponentType == 
                            MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE)
                        {
                            WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, 
                            "found microphone source ( name: %s, ID: %u)", 
                            WideToUTF8(sourceLine.szName), sourceId);
                            GetAllLineControls(mixId, sourceLine, controlArray, false);
                            
                            
                            for (UINT sc = 0; sc < sourceLine.cControls; sc++)
                            {
                                if (controlArray[sc].dwControlType == 
                                    MIXERCONTROL_CONTROLTYPE_VOLUME)
                                {
                                    
                                    _microphoneState[mixId].dwVolumeControlID = 
                                    controlArray[sc].dwControlID;
                                    _microphoneState[mixId].volumeControlIsValid = true;
                                    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, 
                                    "found volume control (name: %s, ID: %u)", 
                                    WideToUTF8(controlArray[sc].szName), 
                                    controlArray[sc].dwControlID);
                                }
                                else if (controlArray[sc].dwControlType == 
                                         MIXERCONTROL_CONTROLTYPE_MUTE)
                                {
                                    
                                    _microphoneState[mixId].dwMuteControlID =
                                    controlArray[sc].dwControlID;
                                    _microphoneState[mixId].muteControlIsValid = true;
                                    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, 
                                    "found mute control (name: %s, ID: %u)", 
                                    WideToUTF8(controlArray[sc].szName), 
                                    controlArray[sc].dwControlID);
                                }
                                else if (controlArray[sc].dwControlType == 
                                         MIXERCONTROL_CONTROLTYPE_ONOFF ||
                                         controlArray[sc].dwControlType == 
                                         MIXERCONTROL_CONTROLTYPE_LOUDNESS)
                                {
                                    
                                    _microphoneState[mixId].dwOnOffControlID = 
                                    controlArray[sc].dwControlID;
                                    _microphoneState[mixId].onOffControlIsValid = true;
                                    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, 
                                    "found on/off control (name: %s, ID: %u)", 
                                    WideToUTF8(controlArray[sc].szName), 
                                    controlArray[sc].dwControlID);
                                 }
                             }
                         }
                    }

                    break;
                }

                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                

                if ((destLine.cControls == 1) &&
                    (controlArray[0].dwControlType == MIXERCONTROL_CONTROLTYPE_MUX))
                {
                    
                    
                    
                    

                    UINT selection(0);
                    const DWORD nItemsInMux(controlArray[0].cMultipleItems);

                    
                    if (GetSelectedMuxSource(mixId, controlArray[0].dwControlID, nItemsInMux, selection))
                    {
                        
                        
                        
                        
                        
                        
                        
                        if (!GetSourceLineInfo(mixId, destId, selection, sourceLine)  ||
                           (sourceLine.cControls == 0)                                ||
                           (sourceLine.fdwLine & MIXERLINE_LINEF_DISCONNECTED)        ||
                          !(sourceLine.fdwLine & MIXERLINE_LINEF_ACTIVE))               
                        {
                            continue;
                        }

                        if (sourceLine.dwComponentType != MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE)
                        {
                            
                            TraceComponentType(sourceLine.dwComponentType);
                            
                            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "the selected (to be controlled) source is not a microphone type");
                        }

                        
                        GetAllLineControls(mixId, sourceLine, controlArray);
                        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "MUX selection is %u [0,%u]", selection, nItemsInMux-1);

                        
                        for (UINT sc = 0; sc < sourceLine.cControls; sc++)
                        {
                            if (controlArray[sc].dwControlType == MIXERCONTROL_CONTROLTYPE_VOLUME)
                            {
                                
                                _microphoneState[mixId].dwVolumeControlID = controlArray[sc].dwControlID;
                                _microphoneState[mixId].volumeControlIsValid = true;
                                WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "found volume control (name: %s, ID: %u)", WideToUTF8(controlArray[sc].szName), controlArray[sc].dwControlID);
                            }
                            else if (controlArray[sc].dwControlType == MIXERCONTROL_CONTROLTYPE_MUTE)
                            {
                                
                                _microphoneState[mixId].dwMuteControlID = controlArray[sc].dwControlID;
                                _microphoneState[mixId].muteControlIsValid = true;
                                WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "found mute control (name: %s, ID: %u)", WideToUTF8(controlArray[sc].szName), controlArray[sc].dwControlID);
                            }
                            else if (controlArray[sc].dwControlType == MIXERCONTROL_CONTROLTYPE_ONOFF ||
                                     controlArray[sc].dwControlType == MIXERCONTROL_CONTROLTYPE_LOUDNESS)
                            {
                                
                                _microphoneState[mixId].dwOnOffControlID = controlArray[sc].dwControlID;
                                _microphoneState[mixId].onOffControlIsValid = true;
                                WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "found on/off control (name: %s, ID: %u)", WideToUTF8(controlArray[sc].szName), controlArray[sc].dwControlID);
                            }
                        }
                    }
                    else
                    {
                        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id, "failed to detect which source to control");
                    }

                }
                else if (destLine.cConnections == 1)
                {
                    

                    GetSourceLineInfo(mixId, destId, 0, sourceLine);
                    if ((sourceLine.dwComponentType == MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE) &&
                        (sourceLine.cControls > 0))
                    {
                        
                        
                        
                        

                        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "microphone source controls will not be controlled");
                    }
                    else if ((sourceLine.dwComponentType == MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE) &&
                             (sourceLine.cControls == 0))
                    {
                        
                        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "microphone source has no controls => use master controls instead");
                    }
                    else
                    {
                        
                        TraceComponentType(sourceLine.dwComponentType);
                        
                        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "the connected (to be controlled) source is not a microphone type");
                    }

                    
                    

                    
                    for (UINT dc = 0; dc < destLine.cControls; dc++)
                    {
                        if (controlArray[dc].dwControlType == MIXERCONTROL_CONTROLTYPE_VOLUME)
                        {
                            
                            _microphoneState[mixId].dwVolumeControlID = controlArray[dc].dwControlID;
                            _microphoneState[mixId].volumeControlIsValid = true;
                            WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "found volume control (name: %s, ID: %u)", WideToUTF8(controlArray[dc].szName), controlArray[dc].dwControlID);
                        }
                        else if (controlArray[dc].dwControlType == MIXERCONTROL_CONTROLTYPE_MUTE)
                        {
                            
                            _microphoneState[mixId].dwMuteControlID = controlArray[dc].dwControlID;
                            _microphoneState[mixId].muteControlIsValid = true;
                            WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "found mute control (name: %s, ID: %u)", WideToUTF8(controlArray[dc].szName), controlArray[dc].dwControlID);
                        }
                        else if (controlArray[dc].dwControlType == MIXERCONTROL_CONTROLTYPE_ONOFF ||
                                 controlArray[dc].dwControlType == MIXERCONTROL_CONTROLTYPE_LOUDNESS ||
                                 controlArray[dc].dwControlType == MIXERCONTROL_CONTROLTYPE_BOOLEAN)
                        {
                            
                            _microphoneState[mixId].dwOnOffControlID = controlArray[dc].dwControlID;
                            _microphoneState[mixId].onOffControlIsValid = true;
                            WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "found on/off control (name: %s, ID: %u)", WideToUTF8(controlArray[dc].szName), controlArray[dc].dwControlID);
                        }
                    }
                }
                else
                {
                    
                    
                    WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id, "failed to locate valid microphone controls for this mixer");
                }
                break;
            }
        }  

        if (!MicrophoneIsValid(mixId))
        {
            WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "unable to find a valid microphone destination line", mixId);
        }
    }  

    if (ValidMicrophones() == 0)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "failed to locate any valid microphone line");
        return -1;
    }

    return 0;
}








WebRtc_Word32 AudioMixerManager::OpenSpeaker(AudioDeviceModule::WindowsDeviceType device)
{
    if (device == AudioDeviceModule::kDefaultDevice)
    {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "AudioMixerManager::OpenSpeaker(kDefaultDevice)");
    }
    else if (device == AudioDeviceModule::kDefaultCommunicationDevice)
    {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "AudioMixerManager::OpenSpeaker(kDefaultCommunicationDevice)");
    }

    CriticalSectionScoped lock(&_critSect);

    
    
    if (_outputMixerHandle != NULL)
    {
        mixerClose(_outputMixerHandle);
        _outputMixerHandle = NULL;
    }

    MMRESULT     res;
    WAVEFORMATEX waveFormat;
    HWAVEOUT     hWaveOut(NULL);

    waveFormat.wFormatTag      = WAVE_FORMAT_PCM ;
    waveFormat.nChannels       = 2;
    waveFormat.nSamplesPerSec  = 48000;
    waveFormat.wBitsPerSample  = 16;
    waveFormat.nBlockAlign     = waveFormat.nChannels * waveFormat.wBitsPerSample / 8;
    waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
    waveFormat.cbSize          = 0;

    
    
    
    
    if (device == AudioDeviceModule::kDefaultCommunicationDevice)
    {
        
        res = waveOutOpen(NULL, WAVE_MAPPER, &waveFormat, 0, 0, CALLBACK_NULL |
            WAVE_MAPPED_kDefaultCommunicationDevice | WAVE_FORMAT_QUERY);
        if (MMSYSERR_NOERROR == res)
        {
            
            res = waveOutOpen(&hWaveOut, WAVE_MAPPER, &waveFormat, 0, 0, CALLBACK_NULL | WAVE_MAPPED_kDefaultCommunicationDevice);
            WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "opening default communication device");
        }
        else
        {
            
            res = waveOutOpen(&hWaveOut, WAVE_MAPPER, &waveFormat, 0, 0, CALLBACK_NULL);
            WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                "unable to open default communication device => using default instead");
        }
    }
    else if (device == AudioDeviceModule::kDefaultDevice)
    {
        
        res = waveOutOpen(&hWaveOut, WAVE_MAPPER, &waveFormat, 0, 0, CALLBACK_NULL);
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "opening default output device");
    }

    if (MMSYSERR_NOERROR != res)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "waveOutOpen() failed (err=%d)", res);
        TraceWaveOutError(res);
    }

    UINT   mixerId(0);
    HMIXER hMixer(NULL);

    
    
    
    res = mixerGetID((HMIXEROBJ)hWaveOut, &mixerId, MIXER_OBJECTF_HWAVEOUT);
    if (MMSYSERR_NOERROR != res)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "mixerGetID(MIXER_OBJECTF_HWAVEOUT) failed (err=%d)", res);
        
        mixerId = 0;
    }
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "specified output device <=> mixer ID %u", mixerId);

    
    
    waveOutClose(hWaveOut);

    
    
    
    if (!SpeakerIsValid(mixerId))
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "it is not possible to control the speaker volume for this mixer device");
        return -1;
    }

    
    
    
    res = mixerOpen(&hMixer, mixerId, 0, 0, MIXER_OBJECTF_MIXER);
    if (MMSYSERR_NOERROR != res)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "mixerOpen() failed (err=%d)", res);
    }

    
    
    _outputMixerHandle = hMixer;
    _outputMixerID = mixerId;

    if (_outputMixerHandle != NULL)
    {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "the output mixer device is now open (0x%x)", _outputMixerHandle);
    }

    return 0;
}








WebRtc_Word32 AudioMixerManager::OpenSpeaker(WebRtc_UWord16 index)
{
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "AudioMixerManager::OpenSpeaker(index=%d)", index);

    CriticalSectionScoped lock(&_critSect);

    
    
    if (_outputMixerHandle != NULL)
    {
        mixerClose(_outputMixerHandle);
        _outputMixerHandle = NULL;
    }

    MMRESULT     res;
    WAVEFORMATEX waveFormat;
    HWAVEOUT     hWaveOut(NULL);

    const UINT   deviceID(index);  

    waveFormat.wFormatTag      = WAVE_FORMAT_PCM ;
    waveFormat.nChannels       = 2;
    waveFormat.nSamplesPerSec  = 48000;
    waveFormat.wBitsPerSample  = 16;
    waveFormat.nBlockAlign     = waveFormat.nChannels * waveFormat.wBitsPerSample / 8;
    waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
    waveFormat.cbSize          = 0;

    
    
    
    
    res = waveOutOpen(&hWaveOut, deviceID, &waveFormat, 0, 0, CALLBACK_NULL);
    if (MMSYSERR_NOERROR != res)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "waveOutOpen(deviceID=%u) failed (err=%d)", index, res);
        TraceWaveOutError(res);
    }

    UINT   mixerId(0);
    HMIXER hMixer(NULL);

    
    
    
    res = mixerGetID((HMIXEROBJ)hWaveOut, &mixerId, MIXER_OBJECTF_HWAVEOUT);
    if (MMSYSERR_NOERROR != res)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "mixerGetID(MIXER_OBJECTF_HWAVEOUT) failed (err=%d)", res);
        
        mixerId = 0;
    }
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "specified output device <=> mixer ID %u", mixerId);

    
    
    waveOutClose(hWaveOut);

    
    
    
    if (!SpeakerIsValid(mixerId))
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "it is not possible to control the speaker volume for this mixer device");
        return -1;
    }

    
    
    
    res = mixerOpen(&hMixer, mixerId, 0, 0, MIXER_OBJECTF_MIXER);
    if (MMSYSERR_NOERROR != res)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "mixerOpen() failed (err=%d)", res);
    }

    
    
    _outputMixerHandle = hMixer;
    _outputMixerID = mixerId;

    if (_outputMixerHandle != NULL)
    {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "the output mixer device is now open (0x%x)", _outputMixerHandle);
    }

    return 0;
}








WebRtc_Word32 AudioMixerManager::OpenMicrophone(AudioDeviceModule::WindowsDeviceType device)
{
    if (device == AudioDeviceModule::kDefaultDevice)
    {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "AudioMixerManager::OpenMicrophone(kDefaultDevice)");
    }
    else if (device == AudioDeviceModule::kDefaultCommunicationDevice)
    {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "AudioMixerManager::OpenMicrophone(kDefaultCommunicationDevice)");
    }

    CriticalSectionScoped lock(&_critSect);

    
    
    if (_inputMixerHandle != NULL)
    {
        mixerClose(_inputMixerHandle);
        _inputMixerHandle = NULL;
    }

    MMRESULT     res;
    WAVEFORMATEX waveFormat;
    HWAVEIN         hWaveIn(NULL);

    waveFormat.wFormatTag      = WAVE_FORMAT_PCM ;
    waveFormat.nChannels       = 1;
    waveFormat.nSamplesPerSec  = 48000;
    waveFormat.wBitsPerSample  = 16;
    waveFormat.nBlockAlign     = waveFormat.nChannels * waveFormat.wBitsPerSample / 8;
    waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
    waveFormat.cbSize          = 0 ;

    
    
    
    
    if (device == AudioDeviceModule::kDefaultCommunicationDevice)
    {
        
        res = waveInOpen(NULL, WAVE_MAPPER, &waveFormat, 0, 0, CALLBACK_NULL |
            WAVE_MAPPED_kDefaultCommunicationDevice | WAVE_FORMAT_QUERY);
        if (MMSYSERR_NOERROR == res)
        {
            
            res = waveInOpen(&hWaveIn, WAVE_MAPPER, &waveFormat, 0, 0, CALLBACK_NULL | WAVE_MAPPED_kDefaultCommunicationDevice);
            WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "opening default communication device");
        }
        else
        {
            
            res = waveInOpen(&hWaveIn, WAVE_MAPPER, &waveFormat, 0, 0, CALLBACK_NULL);
            WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                "unable to open default communication device => using default instead");
        }
    }
    else if (device == AudioDeviceModule::kDefaultDevice)
    {
        
        res = waveInOpen(&hWaveIn, WAVE_MAPPER, &waveFormat, 0, 0, CALLBACK_NULL);
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "opening default input device");
    }

    if (MMSYSERR_NOERROR != res)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "waveInOpen() failed (err=%d)", res);
        TraceWaveInError(res);
    }

    UINT   mixerId(0);
    HMIXER hMixer(NULL);

    
    
    
    res = mixerGetID((HMIXEROBJ)hWaveIn, &mixerId, MIXER_OBJECTF_HWAVEIN);
    if (MMSYSERR_NOERROR != res)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "mixerGetID(MIXER_OBJECTF_HWAVEIN) failed (err=%d)", res);
        
        mixerId = 0;
    }
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "specified input device <=> mixer ID %u", mixerId);

    
    
    waveInClose(hWaveIn);

    
    
    
    if (!MicrophoneIsValid(mixerId))
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "it is not possible to control the microphone volume for this mixer device");
        return -1;
    }

    
    
    
    res = mixerOpen(&hMixer, mixerId, 0, 0, MIXER_OBJECTF_MIXER);
    if (MMSYSERR_NOERROR != res)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "mixerOpen() failed (err=%d)", res);
    }

    
    
    _inputMixerHandle = hMixer;
    _inputMixerID = mixerId;

    if (_inputMixerHandle != NULL)
    {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "the input mixer device is now open (0x%x)", _inputMixerHandle);
    }

    return 0;
}








WebRtc_Word32 AudioMixerManager::OpenMicrophone(WebRtc_UWord16 index)
{
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "AudioMixerManager::OpenMicrophone(index=%d)", index);

    CriticalSectionScoped lock(&_critSect);

    
    
    if (_inputMixerHandle != NULL)
    {
        mixerClose(_inputMixerHandle);
        _inputMixerHandle = NULL;
    }

    MMRESULT     res;
    WAVEFORMATEX waveFormat;
    HWAVEIN         hWaveIn(NULL);

    const UINT   deviceID(index);  

    waveFormat.wFormatTag      = WAVE_FORMAT_PCM ;
    waveFormat.nChannels       = 1;
    waveFormat.nSamplesPerSec  = 48000;
    waveFormat.wBitsPerSample  = 16;
    waveFormat.nBlockAlign     = waveFormat.nChannels * waveFormat.wBitsPerSample / 8;
    waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
    waveFormat.cbSize          = 0;

    
    
    
    
    res = waveInOpen(&hWaveIn, deviceID, &waveFormat, 0, 0, CALLBACK_NULL);
    if (MMSYSERR_NOERROR != res)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "waveInOpen(deviceID=%u) failed (err=%d)", index, res);
        TraceWaveInError(res);
    }

    UINT   mixerId(0);
    HMIXER hMixer(NULL);

    
    
    
    res = mixerGetID((HMIXEROBJ)hWaveIn, &mixerId, MIXER_OBJECTF_HWAVEIN);
    if (MMSYSERR_NOERROR != res)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "mixerGetID(MIXER_OBJECTF_HWAVEIN) failed (err=%d)", res);
        
        mixerId = 0;
    }
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "specified input device <=> mixer ID %u", mixerId);

    
    
    waveInClose(hWaveIn);

    
    
    
    if (!MicrophoneIsValid(mixerId))
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "it is not possible to control the microphone volume for this mixer device");
        return -1;
    }

    
    
    
    res = mixerOpen(&hMixer, mixerId, 0, 0, MIXER_OBJECTF_MIXER);
    if (MMSYSERR_NOERROR != res)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "mixerOpen() failed (err=%d)", res);
    }

    
    
    _inputMixerHandle = hMixer;
    _inputMixerID = mixerId;

    if (_inputMixerHandle != NULL)
    {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "the input mixer device is now open (0x%x)", _inputMixerHandle);
    }

    return 0;
}





bool AudioMixerManager::SpeakerIsInitialized() const
{
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, _id, "%s", __FUNCTION__);

    return (_outputMixerHandle != NULL);
}





bool AudioMixerManager::MicrophoneIsInitialized() const
{
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, _id, "%s", __FUNCTION__);

    return (_inputMixerHandle != NULL);
}





WebRtc_Word32 AudioMixerManager::SetSpeakerVolume(WebRtc_UWord32 volume)
{
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "AudioMixerManager::SetSpeakerVolume(volume=%u)", volume);

    CriticalSectionScoped lock(&_critSect);

    if (_outputMixerHandle == NULL)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "no avaliable output mixer exists");
        return -1;
    }

    const UINT mixerID(_outputMixerID);
    const DWORD dwControlID(_speakerState[_outputMixerID].dwVolumeControlID);
    DWORD dwValue(volume);

    
    
    if (!SetUnsignedControlValue(mixerID, dwControlID, dwValue))
    {
        return -1;
    }

    return (0);
}








WebRtc_Word32 AudioMixerManager::SpeakerVolume(WebRtc_UWord32& volume) const
{

    if (_outputMixerHandle == NULL)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "no avaliable output mixer exists");
        return -1;
    }

    const UINT mixerID(_outputMixerID);
    const DWORD dwControlID(_speakerState[_outputMixerID].dwVolumeControlID);
    DWORD dwValue(0);

    
    
    if (!GetUnsignedControlValue(mixerID, dwControlID, dwValue))
    {
        return -1;
    }

    volume = dwValue;

    return 0;
}








WebRtc_Word32 AudioMixerManager::MaxSpeakerVolume(WebRtc_UWord32& maxVolume) const
{

    if (_outputMixerHandle == NULL)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "no avaliable output mixer exists");
        return -1;
    }

    const UINT mixerID(_outputMixerID);
    const DWORD dwControlID(_speakerState[_outputMixerID].dwVolumeControlID);
    MIXERCONTROL mixerControl;

    
    
    if (!GetLineControl(mixerID, dwControlID, mixerControl))
    {
        return -1;
    }

    maxVolume = mixerControl.Bounds.dwMaximum;

    return 0;
}





WebRtc_Word32 AudioMixerManager::MinSpeakerVolume(WebRtc_UWord32& minVolume) const
{

    if (_outputMixerHandle == NULL)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "no avaliable output mixer exists");
        return -1;
    }

    const UINT mixerID(_outputMixerID);
    const DWORD dwControlID(_speakerState[_outputMixerID].dwVolumeControlID);
    MIXERCONTROL mixerControl;

    
    
    if (!GetLineControl(mixerID, dwControlID, mixerControl))
    {
        return -1;
    }

    minVolume = mixerControl.Bounds.dwMinimum;

    return 0;
}





WebRtc_Word32 AudioMixerManager::SpeakerVolumeStepSize(WebRtc_UWord16& stepSize) const
{

    if (_outputMixerHandle == NULL)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "no avaliable output mixer exists");
        return -1;
    }

    const UINT mixerID(_outputMixerID);
    MIXERCONTROL mixerControl;

    
    
    if (!GetLineControl(mixerID, _speakerState[mixerID].dwVolumeControlID, mixerControl))
    {
        return -1;
    }

    stepSize = static_cast<WebRtc_UWord16> (mixerControl.Metrics.cSteps);

    return 0;
}





WebRtc_Word32 AudioMixerManager::SpeakerVolumeIsAvailable(bool& available)
{
    if (_outputMixerHandle == NULL)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "no avaliable output mixer exists");
        return -1;
    }

    available = _speakerState[_outputMixerID].volumeControlIsValid;

    return 0;
}





WebRtc_Word32 AudioMixerManager::SpeakerMuteIsAvailable(bool& available)
{
    if (_outputMixerHandle == NULL)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "no avaliable output mixer exists");
        return -1;
    }

    available = _speakerState[_outputMixerID].muteControlIsValid;

    return 0;
}







WebRtc_Word32 AudioMixerManager::SetSpeakerMute(bool enable)
{
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "AudioMixerManager::SetSpeakerMute(enable=%u)", enable);

    CriticalSectionScoped lock(&_critSect);

    if (_outputMixerHandle == NULL)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "no avaliable output mixer exists");
        return -1;
    }

    
    
    
    
    if (!_speakerState[_outputMixerID].muteControlIsValid)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "it is not possible to mute this speaker line");
        return -1;
    }

    const DWORD dwControlID(_speakerState[_outputMixerID].dwMuteControlID);

    
    
    if (!SetBooleanControlValue(_outputMixerID, dwControlID, enable))
    {
        return -1;
    }

    return (0);
}





WebRtc_Word32 AudioMixerManager::SpeakerMute(bool& enabled) const
{

    if (_outputMixerHandle == NULL)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "no avaliable output mixer exists");
        return -1;
    }

    
    
    
    
    if (!_speakerState[_outputMixerID].muteControlIsValid)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "it is not possible to mute this speaker line");
        return -1;
    }

    const DWORD dwControlID(_speakerState[_outputMixerID].dwMuteControlID);
    bool value(false);

    
    
    if (!GetBooleanControlValue(_outputMixerID, dwControlID, value))
    {
        return -1;
    }

    enabled = value;

    return 0;
}





WebRtc_Word32 AudioMixerManager::MicrophoneMuteIsAvailable(bool& available)
{
    if (_inputMixerHandle == NULL)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "no avaliable input mixer exists");
        return -1;
    }

    available = _microphoneState[_inputMixerID].muteControlIsValid;

    return 0;
}







WebRtc_Word32 AudioMixerManager::SetMicrophoneMute(bool enable)
{
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "AudioMixerManager::SetMicrophoneMute(enable=%u)", enable);

    CriticalSectionScoped lock(&_critSect);

    if (_inputMixerHandle == NULL)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "no avaliable input mixer exists");
        return -1;
    }

    
    
    
    
    if (!_microphoneState[_inputMixerID].muteControlIsValid)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "it is not possible to mute this microphone line");
        return -1;
    }

    const DWORD dwControlID(_microphoneState[_inputMixerID].dwMuteControlID);

    
    
    if (!SetBooleanControlValue(_inputMixerID, dwControlID, enable))
    {
        return -1;
    }

    return (0);
}





WebRtc_Word32 AudioMixerManager::MicrophoneMute(bool& enabled) const
{

    if (_inputMixerHandle == NULL)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "no avaliable input mixer exists");
        return -1;
    }

    
    
    
    
    if (!_microphoneState[_inputMixerID].muteControlIsValid)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "it is not possible to mute this microphone line");
        return -1;
    }

    const DWORD dwControlID(_microphoneState[_inputMixerID].dwMuteControlID);
    bool value(false);

    
    
    if (!GetBooleanControlValue(_inputMixerID, dwControlID, value))
    {
        return -1;
    }

    enabled = value;

    return 0;
}





WebRtc_Word32 AudioMixerManager::MicrophoneBoostIsAvailable(bool& available)
{
    if (_inputMixerHandle == NULL)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "no avaliable input mixer exists");
        return -1;
    }

    available = _microphoneState[_inputMixerID].onOffControlIsValid;

    return 0;
}





WebRtc_Word32 AudioMixerManager::SetMicrophoneBoost(bool enable)
{
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "AudioMixerManager::SetMicrophoneBoost(enable=%u)", enable);

    CriticalSectionScoped lock(&_critSect);

    if (_inputMixerHandle == NULL)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "no avaliable input mixer exists");
        return -1;
    }

    
    
    
    
    if (!_microphoneState[_inputMixerID].onOffControlIsValid)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "no boost control exists for this wave-in line");
        return -1;
    }

    const DWORD dwControlID(_microphoneState[_inputMixerID].dwOnOffControlID);

    
    
    if (!SetBooleanControlValue(_inputMixerID, dwControlID, enable))
    {
        return -1;
    }

    return (0);
}





WebRtc_Word32 AudioMixerManager::MicrophoneBoost(bool& enabled) const
{

    if (_inputMixerHandle == NULL)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "no avaliable input mixer exists");
        return -1;
    }

    
    
    
    
    if (!_microphoneState[_inputMixerID].onOffControlIsValid)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "no boost control exists for this wave-in line");
        return -1;
    }

    const DWORD dwControlID(_microphoneState[_inputMixerID].dwOnOffControlID);
    bool value(false);

    
    
    if (!GetBooleanControlValue(_inputMixerID, dwControlID, value))
    {
        return -1;
    }

    enabled = value;

    return 0;
}





WebRtc_Word32 AudioMixerManager::MicrophoneVolumeIsAvailable(bool& available)
{
    if (_inputMixerHandle == NULL)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "no avaliable input mixer exists");
        return -1;
    }

    available = _microphoneState[_inputMixerID].volumeControlIsValid;

    return 0;
}





WebRtc_Word32 AudioMixerManager::SetMicrophoneVolume(WebRtc_UWord32 volume)
{
    CriticalSectionScoped lock(&_critSect);

    if (_inputMixerHandle == NULL)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "no avaliable input mixer exists");
        return -1;
    }

    const UINT mixerID(_inputMixerID);
    const DWORD dwControlID(_microphoneState[_inputMixerID].dwVolumeControlID);
    DWORD dwValue(volume);

    
    
    if (!SetUnsignedControlValue(mixerID, dwControlID, dwValue))
    {
        return -1;
    }

    return (0);
}





WebRtc_Word32 AudioMixerManager::MicrophoneVolume(WebRtc_UWord32& volume) const
{
    CriticalSectionScoped lock(&_critSect);

    if (_inputMixerHandle == NULL)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "no avaliable input mixer exists");
        return -1;
    }

    const UINT mixerID(_inputMixerID);
    const DWORD dwControlID(_microphoneState[_inputMixerID].dwVolumeControlID);
    DWORD dwValue(0);

    
    
    if (!GetUnsignedControlValue(mixerID, dwControlID, dwValue))
    {
        return -1;
    }

    volume = dwValue;

    return 0;
}





WebRtc_Word32 AudioMixerManager::MaxMicrophoneVolume(WebRtc_UWord32& maxVolume) const
{
    WEBRTC_TRACE(kTraceStream, kTraceAudioDevice, _id, "%s", __FUNCTION__);

    if (_inputMixerHandle == NULL)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "no avaliable input mixer exists");
        return -1;
    }

    const UINT mixerID(_inputMixerID);
    const DWORD dwControlID(_microphoneState[_inputMixerID].dwVolumeControlID);
    MIXERCONTROL mixerControl;

    
    
    if (!GetLineControl(mixerID, dwControlID, mixerControl))
    {
        return -1;
    }

    maxVolume = mixerControl.Bounds.dwMaximum;

    return 0;
}





WebRtc_Word32 AudioMixerManager::MinMicrophoneVolume(WebRtc_UWord32& minVolume) const
{

    if (_inputMixerHandle == NULL)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "no avaliable input mixer exists");
        return -1;
    }

    const UINT mixerID(_inputMixerID);
    const DWORD dwControlID(_microphoneState[_inputMixerID].dwVolumeControlID);
    MIXERCONTROL mixerControl;

    
    
    if (!GetLineControl(mixerID, dwControlID, mixerControl))
    {
        return -1;
    }

    minVolume = mixerControl.Bounds.dwMinimum;

    return 0;
}





WebRtc_Word32 AudioMixerManager::MicrophoneVolumeStepSize(WebRtc_UWord16& stepSize) const
{

    if (_inputMixerHandle == NULL)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "no avaliable input mixer exists");
        return -1;
    }

    const UINT mixerID(_inputMixerID);
    const DWORD dwControlID(_microphoneState[_inputMixerID].dwVolumeControlID);
    MIXERCONTROL mixerControl;

    
    
    if (!GetLineControl(mixerID, dwControlID, mixerControl))
    {
        return -1;
    }

    stepSize = static_cast<WebRtc_UWord16> (mixerControl.Metrics.cSteps);

    return 0;
}













UINT AudioMixerManager::Devices() const
{
    UINT nDevs = mixerGetNumDevs();
    return nDevs;
}







UINT AudioMixerManager::DestinationLines(UINT mixId) const
{
    MIXERCAPS caps;
    if (!GetCapabilities(mixId, caps))
    {
        return 0;
    }
    return (caps.cDestinations);
}






UINT AudioMixerManager::SourceLines(UINT mixId, DWORD destId) const
{
    MIXERLINE dline;
    if (!GetDestinationLineInfo(mixId, destId, dline))
    {
        return 0;
    }
    return (dline.cConnections);
}







bool AudioMixerManager::GetCapabilities(UINT mixId, MIXERCAPS& caps, bool trace) const
{
    MMRESULT res;
    MIXERCAPS mcaps;

    res = mixerGetDevCaps(mixId, &mcaps, sizeof(MIXERCAPS));
    if (res != MMSYSERR_NOERROR)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "mixerGetDevCaps() failed (err=%d)", res);
        return false;
    }

    memcpy(&caps, &mcaps, sizeof(MIXERCAPS));

    if (trace)
    {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "===============================================================");
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "Mixer ID %u:", mixId);
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "manufacturer ID      : %u", caps.wMid);
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "product ID           : %u", caps.wPid);
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "version of driver    : %u", caps.vDriverVersion);
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "product name         : %s", WideToUTF8(caps.szPname));
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "misc. support bits   : %u", caps.fdwSupport);
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "count of destinations: %u (+)", caps.cDestinations);
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "===============================================================");
    }

    if (caps.cDestinations == 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id, "invalid number of mixer destinations");
        return false;
    }

    return true;
}





bool AudioMixerManager::GetDestinationLineInfo(UINT mixId, DWORD destId, MIXERLINE& line, bool trace) const
{
    MMRESULT  res;
    MIXERLINE mline;

    mline.cbStruct = sizeof(MIXERLINE);
    mline.dwDestination = destId;   
    mline.dwSource = 0;             

    
    
    
    
    res = mixerGetLineInfo(reinterpret_cast<HMIXEROBJ>(mixId), &mline, MIXER_OBJECTF_MIXER | MIXER_GETLINEINFOF_DESTINATION);
    if (res != MMSYSERR_NOERROR)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "mixerGetLineInfo(MIXER_GETLINEINFOF_DESTINATION) failed (err=%d)", res);
        return false;
    }

    memcpy(&line, &mline, sizeof(MIXERLINE));

    if (trace)
    {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "> Destination Line ID %u:", destId);
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "destination line index : %u", mline.dwDestination);
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "dwLineID               : %lu (unique)", mline.dwLineID);
        TraceStatusAndSupportFlags(mline.fdwLine);
        TraceComponentType(mline.dwComponentType);
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "count of channels      : %u", mline.cChannels);
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "# audio source lines   : %u (+)", mline.cConnections);    
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "# controls             : %u (*)", mline.cControls);       
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "short name             : %s", WideToUTF8(mline.szShortName));
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "full name              : %s", WideToUTF8(mline.szName));
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
        TraceTargetType(mline.Target.dwType);
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "target device ID       : %lu", mline.Target.dwDeviceID);
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "manufacturer ID        : %u", mline.Target.wMid);
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "product ID             : %u", mline.Target.wPid);
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "driver version         : %u", mline.Target.vDriverVersion);
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "product name           : %s", WideToUTF8(mline.Target.szPname));
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "---------------------------------------------------------------");
    }

    return true;
}





bool AudioMixerManager::GetSourceLineInfo(UINT mixId, DWORD destId, DWORD srcId, MIXERLINE& line, bool trace) const
{
    MMRESULT  res;
    MIXERLINE mline;

    mline.cbStruct = sizeof(MIXERLINE);
    mline.dwDestination = destId;   
    mline.dwSource = srcId;         

    
    
    
    
    res = mixerGetLineInfo(reinterpret_cast<HMIXEROBJ>(mixId), &mline, MIXER_OBJECTF_MIXER | MIXER_GETLINEINFOF_SOURCE);
    if (res != MMSYSERR_NOERROR)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "mixerGetLineInfo(MIXER_GETLINEINFOF_SOURCE) failed (err=%d)", res);
        return false;
    }

    memcpy(&line, &mline, sizeof(MIXERLINE));

    if (trace)
    {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, " >> Source Line ID %u:", srcId);
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "destination line index : %u", mline.dwDestination);
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "dwSource               : %u", mline.dwSource);
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "dwLineID               : %lu (unique)", mline.dwLineID);
        TraceStatusAndSupportFlags(mline.fdwLine);
        TraceComponentType(mline.dwComponentType);
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "# controls             : %u (*)", mline.cControls);
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "full name              : %s", WideToUTF8(mline.szName));
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
        TraceTargetType(mline.Target.dwType);
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "---------------------------------------------------------------");
    }

    return true;
}





bool AudioMixerManager::GetAllLineControls(UINT mixId, const MIXERLINE& line, MIXERCONTROL* controlArray, bool trace) const
{
    
    
    if (line.cControls == 0)
        return false;

    MMRESULT          res;
    MIXERLINECONTROLS mlineControls;            

    mlineControls.dwLineID  = line.dwLineID;    
    mlineControls.cControls = line.cControls;   
    mlineControls.pamxctrl  = controlArray;     
    mlineControls.cbStruct  = sizeof(MIXERLINECONTROLS);
    mlineControls.cbmxctrl  = sizeof(MIXERCONTROL);

    
    
    res = mixerGetLineControls(reinterpret_cast<HMIXEROBJ>(mixId), &mlineControls, MIXER_OBJECTF_MIXER | MIXER_GETLINECONTROLSF_ALL);
    if (res != MMSYSERR_NOERROR)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "mixerGetLineControls(MIXER_GETLINECONTROLSF_ALL) failed  (err=%d)", res);
        return false;
    }

    if (trace)
    {
        for (UINT c = 0; c < line.cControls; c++)
        {
            WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, " >> Control ID %u:", c);
            WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "dwControlID            : %u (unique)", controlArray[c].dwControlID);
            TraceControlType(controlArray[c].dwControlType);
            TraceControlStatusAndSupportFlags(controlArray[c].fdwControl);
            WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "cMultipleItems         : %u", controlArray[c].cMultipleItems);
            WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "short name             : %s", WideToUTF8(controlArray[c].szShortName));
            WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "full name              : %s", WideToUTF8(controlArray[c].szName));
            if ((controlArray[c].dwControlType & MIXERCONTROL_CT_UNITS_MASK) == MIXERCONTROL_CT_UNITS_SIGNED)
            {
                WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "min signed value       : %d", controlArray[c].Bounds.lMinimum);
                WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "max signed value       : %d", controlArray[c].Bounds.lMaximum);
            }
            else if ((controlArray[c].dwControlType & MIXERCONTROL_CT_UNITS_MASK) == MIXERCONTROL_CT_UNITS_UNSIGNED ||
                     (controlArray[c].dwControlType & MIXERCONTROL_CT_UNITS_MASK) == MIXERCONTROL_CT_UNITS_BOOLEAN)
            {
                WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "min unsigned value     : %u",  controlArray[c].Bounds.dwMinimum);
                WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "max unsigned value     : %u", controlArray[c].Bounds.dwMaximum);
            }
            if (controlArray[c].dwControlType  != MIXERCONTROL_CONTROLTYPE_CUSTOM)
            {
                WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "cSteps                 : %u",  controlArray[c].Metrics.cSteps);
            }
            WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "...............................................................");
            GetControlDetails(mixId, controlArray[c], true);
            WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "...............................................................");

        }
    }

    return true;
}





bool AudioMixerManager::GetLineControl(UINT mixId, DWORD dwControlID, MIXERCONTROL& control) const
{
    MMRESULT          res;
    MIXERLINECONTROLS mlineControl;

    mlineControl.dwControlID = dwControlID;
    mlineControl.cControls   = 1;
    mlineControl.pamxctrl    = &control;
    mlineControl.cbStruct    = sizeof(MIXERLINECONTROLS);
    mlineControl.cbmxctrl    = sizeof(MIXERCONTROL);

    
    
    res = mixerGetLineControls(reinterpret_cast<HMIXEROBJ>(mixId), &mlineControl, MIXER_OBJECTF_MIXER | MIXER_GETLINECONTROLSF_ONEBYID);
    if (res != MMSYSERR_NOERROR)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "mixerGetLineControls(MIXER_GETLINECONTROLSF_ONEBYID) failed (err=%d)", res);
        return false;
    }

    return true;
}





bool AudioMixerManager::GetControlDetails(UINT mixId, MIXERCONTROL& controlArray, bool trace) const
{
    assert(controlArray.cMultipleItems <= MAX_NUMBER_OF_MULTIPLE_ITEMS);

    MMRESULT                     res;
    MIXERCONTROLDETAILS          controlDetails;

    MIXERCONTROLDETAILS_UNSIGNED valueUnsigned[MAX_NUMBER_OF_MULTIPLE_ITEMS];
    MIXERCONTROLDETAILS_SIGNED   valueSigned[MAX_NUMBER_OF_MULTIPLE_ITEMS];
    MIXERCONTROLDETAILS_BOOLEAN  valueBoolean[MAX_NUMBER_OF_MULTIPLE_ITEMS];

    enum ControlType
    {
        CT_UNITS_UNSIGNED,
        CT_UNITS_SIGNED,
        CT_UNITS_BOOLEAN
    };

    ControlType ctype(CT_UNITS_UNSIGNED);

    controlDetails.cbStruct       = sizeof(MIXERCONTROLDETAILS);
    controlDetails.dwControlID    = controlArray.dwControlID;       
    controlDetails.cChannels      = 1;                              
    controlDetails.cMultipleItems = controlArray.cMultipleItems;    
                                                                    
    if (controlDetails.cMultipleItems > MAX_NUMBER_OF_MULTIPLE_ITEMS)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "cMultipleItems > %d", MAX_NUMBER_OF_MULTIPLE_ITEMS);
        controlDetails.cMultipleItems = MAX_NUMBER_OF_MULTIPLE_ITEMS;
    }

    if ((controlArray.dwControlType & MIXERCONTROL_CT_UNITS_MASK) == MIXERCONTROL_CT_UNITS_SIGNED)
    {
        ctype = CT_UNITS_SIGNED;
        controlDetails.cbDetails = sizeof(MIXERCONTROLDETAILS_SIGNED);
        controlDetails.paDetails = &valueSigned[0];
    }
    else if ((controlArray.dwControlType & MIXERCONTROL_CT_UNITS_MASK) == MIXERCONTROL_CT_UNITS_UNSIGNED)
    {
        ctype = CT_UNITS_UNSIGNED;
        controlDetails.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
        controlDetails.paDetails = &valueUnsigned[0];
    }
    else if ((controlArray.dwControlType & MIXERCONTROL_CT_UNITS_MASK) == MIXERCONTROL_CT_UNITS_BOOLEAN)
    {
        ctype = CT_UNITS_BOOLEAN;
        controlDetails.cbDetails = sizeof(MIXERCONTROLDETAILS_BOOLEAN);
        controlDetails.paDetails = &valueBoolean[0];
    }

    
    
    res = mixerGetControlDetails(reinterpret_cast<HMIXEROBJ>(mixId), &controlDetails, MIXER_OBJECTF_MIXER | MIXER_GETCONTROLDETAILSF_VALUE);
    if (res != MMSYSERR_NOERROR)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "mixerGetControlDetails(MIXER_GETCONTROLDETAILSF_VALUE) failed (err=%d)", res);
        return false;
    }

    if (trace)
    {
        UINT nItems(1);
        nItems = (controlDetails.cMultipleItems > 0 ? controlDetails.cMultipleItems : 1);
        for (UINT i = 0; i < nItems; i++)
        {
            if (ctype == CT_UNITS_SIGNED)
            {
                WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "signed value           : %d", valueSigned[i].lValue);
            }
            else if (ctype == CT_UNITS_UNSIGNED)
            {
                WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "unsigned value         : %u", valueUnsigned[i].dwValue);
            }
            else if (ctype == CT_UNITS_BOOLEAN)
            {
                WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "boolean value          : %u", valueBoolean[i].fValue);
            }
        }
    }

    return true;
}





bool AudioMixerManager::GetUnsignedControlValue(UINT mixId, DWORD dwControlID, DWORD& dwValue) const
{
    MMRESULT                     res;
    MIXERCONTROLDETAILS          controlDetails;
    MIXERCONTROLDETAILS_UNSIGNED valueUnsigned;

    controlDetails.dwControlID    = dwControlID;
    controlDetails.cbStruct       = sizeof(MIXERCONTROLDETAILS);
    controlDetails.cChannels      = 1;
    controlDetails.cMultipleItems = 0;
    controlDetails.cbDetails      = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
    controlDetails.paDetails      = &valueUnsigned;

    
    
    res = mixerGetControlDetails(reinterpret_cast<HMIXEROBJ>(mixId), &controlDetails, MIXER_OBJECTF_MIXER | MIXER_GETCONTROLDETAILSF_VALUE);
    if (res != MMSYSERR_NOERROR)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "mixerGetControlDetails(MIXER_GETCONTROLDETAILSF_VALUE) failed (err=%d)", res);
        return false;
    }

    
    
    dwValue = valueUnsigned.dwValue;

    return true;
}





bool AudioMixerManager::SetUnsignedControlValue(UINT mixId, DWORD dwControlID, DWORD dwValue) const
{
    WEBRTC_TRACE(kTraceStream, kTraceAudioDevice, _id, "AudioMixerManager::SetUnsignedControlValue(mixId=%u, dwControlID=%d, dwValue=%d)", mixId, dwControlID, dwValue);

    MMRESULT                     res;
    MIXERCONTROLDETAILS          controlDetails;
    MIXERCONTROLDETAILS_UNSIGNED valueUnsigned;

    controlDetails.dwControlID    = dwControlID;
    controlDetails.cbStruct       = sizeof(MIXERCONTROLDETAILS);
    controlDetails.cChannels      = 1;
    controlDetails.cMultipleItems = 0;
    controlDetails.cbDetails      = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
    controlDetails.paDetails      = &valueUnsigned;

    valueUnsigned.dwValue         = dwValue;

    
    
    res = mixerSetControlDetails(reinterpret_cast<HMIXEROBJ>(mixId), &controlDetails, MIXER_OBJECTF_MIXER | MIXER_GETCONTROLDETAILSF_VALUE);
    if (res != MMSYSERR_NOERROR)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "mixerSetControlDetails(MIXER_GETCONTROLDETAILSF_VALUE) failed (err=%d)", res);
        return false;
    }

    return true;
}





bool AudioMixerManager::SetBooleanControlValue(UINT mixId, DWORD dwControlID, bool value) const
{
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "AudioMixerManager::SetBooleanControlValue(mixId=%u, dwControlID=%d, value=%d)", mixId, dwControlID, value);

    MMRESULT                    res;
    MIXERCONTROLDETAILS         controlDetails;
    MIXERCONTROLDETAILS_BOOLEAN valueBoolean;

    controlDetails.dwControlID    = dwControlID;
    controlDetails.cbStruct       = sizeof(MIXERCONTROLDETAILS);
    controlDetails.cChannels      = 1;
    controlDetails.cMultipleItems = 0;
    controlDetails.cbDetails      = sizeof(MIXERCONTROLDETAILS_BOOLEAN);
    controlDetails.paDetails      = &valueBoolean;

    if (value == true)
        valueBoolean.fValue = TRUE;
    else
        valueBoolean.fValue = FALSE;

    
    
    res = mixerSetControlDetails(reinterpret_cast<HMIXEROBJ>(mixId), &controlDetails, MIXER_OBJECTF_MIXER | MIXER_GETCONTROLDETAILSF_VALUE);
    if (res != MMSYSERR_NOERROR)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "mixerSetControlDetails(MIXER_GETCONTROLDETAILSF_VALUE) failed (err=%d)", res);
        return false;
    }

    return true;
}





bool AudioMixerManager::GetBooleanControlValue(UINT mixId, DWORD dwControlID, bool& value) const
{
    MMRESULT                    res;
    MIXERCONTROLDETAILS         controlDetails;
    MIXERCONTROLDETAILS_BOOLEAN valueBoolean;

    controlDetails.dwControlID    = dwControlID;
    controlDetails.cbStruct       = sizeof(MIXERCONTROLDETAILS);
    controlDetails.cChannels      = 1;
    controlDetails.cMultipleItems = 0;
    controlDetails.cbDetails      = sizeof(MIXERCONTROLDETAILS_BOOLEAN);
    controlDetails.paDetails      = &valueBoolean;

    
    
    res = mixerGetControlDetails(reinterpret_cast<HMIXEROBJ>(mixId), &controlDetails, MIXER_OBJECTF_MIXER | MIXER_GETCONTROLDETAILSF_VALUE);
    if (res != MMSYSERR_NOERROR)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "mixerGetControlDetails(MIXER_GETCONTROLDETAILSF_VALUE) failed (err=%d)", res);
        return false;
    }

    
    
    if (valueBoolean.fValue == 0)
        value = false;
    else
        value = true;

    return true;
}





bool AudioMixerManager::GetSelectedMuxSource(UINT mixId, DWORD dwControlID, DWORD cMultipleItems, UINT& index) const
{
    assert(cMultipleItems <= MAX_NUMBER_OF_MULTIPLE_ITEMS);

    MMRESULT                    res;
    MIXERCONTROLDETAILS         controlDetails;
    MIXERCONTROLDETAILS_BOOLEAN valueBoolean[MAX_NUMBER_OF_MULTIPLE_ITEMS];
    memset(&valueBoolean, 0, sizeof(valueBoolean));

    controlDetails.dwControlID    = dwControlID;
    controlDetails.cbStruct       = sizeof(MIXERCONTROLDETAILS);
    controlDetails.cChannels      = 1;
    controlDetails.cMultipleItems = cMultipleItems;
    controlDetails.cbDetails      = sizeof(MIXERCONTROLDETAILS_BOOLEAN);
    controlDetails.paDetails      = &valueBoolean;

    
    
    res = mixerGetControlDetails(reinterpret_cast<HMIXEROBJ>(mixId), &controlDetails, MIXER_OBJECTF_MIXER | MIXER_GETCONTROLDETAILSF_VALUE);
    if (res != MMSYSERR_NOERROR)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "mixerGetControlDetails(MIXER_GETCONTROLDETAILSF_VALUE) failed (err=%d)", res);
        return false;
    }

    
    
    
    
    
    
    
    index = 0;
    for (DWORD i = 0; i < cMultipleItems; i++)
    {
        if (valueBoolean[i].fValue > 0)
        {
            index = (cMultipleItems - 1) - i;
            break;
        }
    }

    return true;
}





void AudioMixerManager::TraceStatusAndSupportFlags(DWORD fdwLine) const
{
    TCHAR buf[128];

    StringCchPrintf(buf, 128, TEXT("status & support flags : 0x%x "), fdwLine);

    switch (fdwLine)
    {
    case MIXERLINE_LINEF_ACTIVE:
        StringCchCat(buf, 128, TEXT("(ACTIVE DESTINATION)"));
        break;
    case MIXERLINE_LINEF_DISCONNECTED:
        StringCchCat(buf, 128, TEXT("(DISCONNECTED)"));
        break;
    case MIXERLINE_LINEF_SOURCE:
        StringCchCat(buf, 128, TEXT("(INACTIVE SOURCE)"));
        break;
    case MIXERLINE_LINEF_SOURCE | MIXERLINE_LINEF_ACTIVE:
        StringCchCat(buf, 128, TEXT("(ACTIVE SOURCE)"));
        break;
    default:
        StringCchCat(buf, 128, TEXT("(INVALID)"));
        break;
    }

    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "%s", WideToUTF8(buf));
}





void AudioMixerManager::TraceComponentType(DWORD dwComponentType) const
{
    TCHAR buf[128];

    StringCchPrintf(buf, 128, TEXT("component type         : 0x%x "), dwComponentType);

    switch (dwComponentType)
    {
    
    case MIXERLINE_COMPONENTTYPE_DST_UNDEFINED:
        StringCchCat(buf, 128, TEXT("(DST_UNDEFINED)"));
        break;
    case MIXERLINE_COMPONENTTYPE_DST_DIGITAL:
        StringCchCat(buf, 128, TEXT("(DST_DIGITAL)"));
        break;
    case MIXERLINE_COMPONENTTYPE_DST_LINE:
        StringCchCat(buf, 128, TEXT("(DST_LINE)"));
        break;
    case MIXERLINE_COMPONENTTYPE_DST_MONITOR:
        StringCchCat(buf, 128, TEXT("(DST_MONITOR)"));
        break;
    case MIXERLINE_COMPONENTTYPE_DST_SPEAKERS:
        StringCchCat(buf, 128, TEXT("(DST_SPEAKERS)"));
        break;
    case MIXERLINE_COMPONENTTYPE_DST_HEADPHONES:
        StringCchCat(buf, 128, TEXT("(DST_HEADPHONES)"));
        break;
    case MIXERLINE_COMPONENTTYPE_DST_TELEPHONE:
        StringCchCat(buf, 128, TEXT("(DST_TELEPHONE)"));
        break;
    case MIXERLINE_COMPONENTTYPE_DST_WAVEIN:
        StringCchCat(buf, 128, TEXT("(DST_WAVEIN)"));
        break;
    case MIXERLINE_COMPONENTTYPE_DST_VOICEIN:
        StringCchCat(buf, 128, TEXT("(DST_VOICEIN)"));
        break;
    
    case MIXERLINE_COMPONENTTYPE_SRC_UNDEFINED:
        StringCchCat(buf, 128, TEXT("(SRC_UNDEFINED)"));
        break;
    case MIXERLINE_COMPONENTTYPE_SRC_DIGITAL:
        StringCchCat(buf, 128, TEXT("(SRC_DIGITAL)"));
        break;
    case MIXERLINE_COMPONENTTYPE_SRC_LINE:
        StringCchCat(buf, 128, TEXT("(SRC_LINE)"));
        break;
    case MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE:
        StringCchCat(buf, 128, TEXT("(SRC_MICROPHONE)"));
        break;
    case MIXERLINE_COMPONENTTYPE_SRC_SYNTHESIZER:
        StringCchCat(buf, 128, TEXT("(SRC_SYNTHESIZER)"));
        break;
    case MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC:
        StringCchCat(buf, 128, TEXT("(SRC_COMPACTDISC)"));
        break;
    case MIXERLINE_COMPONENTTYPE_SRC_TELEPHONE:
        StringCchCat(buf, 128, TEXT("(SRC_TELEPHONE)"));
        break;
    case MIXERLINE_COMPONENTTYPE_SRC_PCSPEAKER:
        StringCchCat(buf, 128, TEXT("(SRC_PCSPEAKER)"));
        break;
    case MIXERLINE_COMPONENTTYPE_SRC_WAVEOUT:
        StringCchCat(buf, 128, TEXT("(SRC_WAVEOUT)"));
        break;
    case MIXERLINE_COMPONENTTYPE_SRC_AUXILIARY:
        StringCchCat(buf, 128, TEXT("(SRC_AUXILIARY)"));
        break;
    case MIXERLINE_COMPONENTTYPE_SRC_ANALOG:
        StringCchCat(buf, 128, TEXT("(SRC_ANALOG)"));
        break;
    default:
        StringCchCat(buf, 128, TEXT("(INVALID)"));
        break;
    }

    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "%s", WideToUTF8(buf));
}





void AudioMixerManager::TraceTargetType(DWORD dwType) const
{
    TCHAR buf[128];

    StringCchPrintf(buf, 128, TEXT("media device type      : 0x%x "), dwType);

    switch (dwType)
    {
    case MIXERLINE_TARGETTYPE_UNDEFINED:
        StringCchCat(buf, 128, TEXT("(UNDEFINED)"));
        break;
    case MIXERLINE_TARGETTYPE_WAVEOUT:
        StringCchCat(buf, 128, TEXT("(WAVEOUT)"));
        break;
    case MIXERLINE_TARGETTYPE_WAVEIN:
        StringCchCat(buf, 128, TEXT("(WAVEIN)"));
        break;
    case MIXERLINE_TARGETTYPE_MIDIOUT:
        StringCchCat(buf, 128, TEXT("(MIDIOUT)"));
        break;
    case MIXERLINE_TARGETTYPE_MIDIIN:
        StringCchCat(buf, 128, TEXT("(MIDIIN)"));
        break;
    default:
        StringCchCat(buf, 128, TEXT("(INVALID)"));
        break;
    }

    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "%s", WideToUTF8(buf));
}





void AudioMixerManager::TraceControlType(DWORD dwControlType) const
{
    TCHAR buf[128];

    
    
    StringCchPrintf(buf, 128, TEXT("class type             : 0x%x "), dwControlType);

    switch (dwControlType & MIXERCONTROL_CT_CLASS_MASK)
    {
    case MIXERCONTROL_CT_CLASS_CUSTOM:
        StringCchCat(buf, 128, TEXT("(CT_CLASS_CUSTOM)"));
        break;
    case MIXERCONTROL_CT_CLASS_METER:
        StringCchCat(buf, 128, TEXT("(CT_CLASS_METER)"));
        break;
    case MIXERCONTROL_CT_CLASS_SWITCH:
        StringCchCat(buf, 128, TEXT("(CT_CLASS_SWITCH)"));
        break;
    case MIXERCONTROL_CT_CLASS_NUMBER:
        StringCchCat(buf, 128, TEXT("(CT_CLASS_NUMBER)"));
        break;
    case MIXERCONTROL_CT_CLASS_SLIDER:
        StringCchCat(buf, 128, TEXT("(CT_CLASS_SLIDER)"));
        break;
    case MIXERCONTROL_CT_CLASS_FADER:
        StringCchCat(buf, 128, TEXT("(CT_CLASS_FADER)"));
        break;
    case MIXERCONTROL_CT_CLASS_TIME:
        StringCchCat(buf, 128, TEXT("(CT_CLASS_TIME)"));
        break;
    case MIXERCONTROL_CT_CLASS_LIST:
        StringCchCat(buf, 128, TEXT("(CT_CLASS_LIST)"));
        break;
    default:
        StringCchCat(buf, 128, TEXT("(INVALID)"));
        break;
    }

    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "%s", WideToUTF8(buf));

    
    
    StringCchPrintf(buf, 128, TEXT("control type           : 0x%x "), dwControlType);

    switch (dwControlType)
    {
    case MIXERCONTROL_CONTROLTYPE_CUSTOM:
        StringCchCat(buf, 128, TEXT("(CONTROLTYPE_CUSTOM)"));
        break;
    case MIXERCONTROL_CONTROLTYPE_BOOLEANMETER:
        StringCchCat(buf, 128, TEXT("(CONTROLTYPE_BOOLEANMETER)"));
        break;
    case MIXERCONTROL_CONTROLTYPE_SIGNEDMETER:
        StringCchCat(buf, 128, TEXT("(CONTROLTYPE_SIGNEDMETER)"));
        break;
    case MIXERCONTROL_CONTROLTYPE_PEAKMETER:
        StringCchCat(buf, 128, TEXT("(CONTROLTYPE_PEAKMETER)"));
        break;
    case MIXERCONTROL_CONTROLTYPE_UNSIGNEDMETER:
        StringCchCat(buf, 128, TEXT("(CONTROLTYPE_UNSIGNEDMETER)"));
        break;
    case MIXERCONTROL_CONTROLTYPE_BOOLEAN:
        StringCchCat(buf, 128, TEXT("(CONTROLTYPE_BOOLEAN)"));
        break;
    case MIXERCONTROL_CONTROLTYPE_ONOFF:
        StringCchCat(buf, 128, TEXT("(CONTROLTYPE_ONOFF)"));
        break;
    case MIXERCONTROL_CONTROLTYPE_MUTE:
        StringCchCat(buf, 128, TEXT("(CONTROLTYPE_MUTE)"));
        break;
    case MIXERCONTROL_CONTROLTYPE_MONO:
        StringCchCat(buf, 128, TEXT("(CONTROLTYPE_MONO)"));
        break;
    case MIXERCONTROL_CONTROLTYPE_LOUDNESS:
        StringCchCat(buf, 128, TEXT("(CONTROLTYPE_LOUDNESS)"));
        break;
    case MIXERCONTROL_CONTROLTYPE_STEREOENH:
        StringCchCat(buf, 128, TEXT("(CONTROLTYPE_STEREOENH)"));
        break;
    case MIXERCONTROL_CONTROLTYPE_BASS_BOOST:
        StringCchCat(buf, 128, TEXT("(CONTROLTYPE_BASS_BOOST)"));
        break;
    case MIXERCONTROL_CONTROLTYPE_BUTTON:
        StringCchCat(buf, 128, TEXT("(CONTROLTYPE_BUTTON)"));
        break;
    case MIXERCONTROL_CONTROLTYPE_DECIBELS:
        StringCchCat(buf, 128, TEXT("(CONTROLTYPE_DECIBELS)"));
        break;
    case MIXERCONTROL_CONTROLTYPE_SIGNED:
        StringCchCat(buf, 128, TEXT("(CONTROLTYPE_SIGNED)"));
        break;
    case MIXERCONTROL_CONTROLTYPE_UNSIGNED:
        StringCchCat(buf, 128, TEXT("(CONTROLTYPE_UNSIGNED)"));
        break;
    case MIXERCONTROL_CONTROLTYPE_PERCENT:
        StringCchCat(buf, 128, TEXT("(CONTROLTYPE_PERCENT)"));
        break;
    case MIXERCONTROL_CONTROLTYPE_SLIDER:
        StringCchCat(buf, 128, TEXT("(CONTROLTYPE_SLIDER)"));
        break;
    case MIXERCONTROL_CONTROLTYPE_PAN:
        StringCchCat(buf, 128, TEXT("(CONTROLTYPE_PAN)"));
        break;
    case MIXERCONTROL_CONTROLTYPE_QSOUNDPAN:
        StringCchCat(buf, 128, TEXT("(CONTROLTYPE_QSOUNDPAN)"));
        break;
    case MIXERCONTROL_CONTROLTYPE_FADER:
        StringCchCat(buf, 128, TEXT("(CONTROLTYPE_FADER)"));
        break;
    case MIXERCONTROL_CONTROLTYPE_VOLUME:
        StringCchCat(buf, 128, TEXT("(CONTROLTYPE_VOLUME)"));
        break;
    case MIXERCONTROL_CONTROLTYPE_BASS:
        StringCchCat(buf, 128, TEXT("(CONTROLTYPE_BASS)"));
        break;
    case MIXERCONTROL_CONTROLTYPE_TREBLE:
        StringCchCat(buf, 128, TEXT("(CONTROLTYPE_TREBLE)"));
        break;
    case MIXERCONTROL_CONTROLTYPE_EQUALIZER:
        StringCchCat(buf, 128, TEXT("(CONTROLTYPE_EQUALIZER)"));
        break;
    case MIXERCONTROL_CONTROLTYPE_SINGLESELECT:
        StringCchCat(buf, 128, TEXT("(CONTROLTYPE_SINGLESELECT)"));
        break;
    case MIXERCONTROL_CONTROLTYPE_MUX:
        StringCchCat(buf, 128, TEXT("(CONTROLTYPE_MUX)"));
        break;
    case MIXERCONTROL_CONTROLTYPE_MULTIPLESELECT:
        StringCchCat(buf, 128, TEXT("(CONTROLTYPE_MULTIPLESELECT)"));
        break;
    case MIXERCONTROL_CONTROLTYPE_MIXER:
        StringCchCat(buf, 128, TEXT("(CONTROLTYPE_MIXER)"));
        break;
    case MIXERCONTROL_CONTROLTYPE_MICROTIME:
        StringCchCat(buf, 128, TEXT("(CONTROLTYPE_MICROTIME)"));
        break;
    case MIXERCONTROL_CONTROLTYPE_MILLITIME:
        StringCchCat(buf, 128, TEXT("(CONTROLTYPE_MILLITIME)"));
        break;
    default:
        StringCchCat(buf, 128, TEXT("(INVALID)"));
        break;
    }

    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "%s", WideToUTF8(buf));
}































void AudioMixerManager::TraceControlStatusAndSupportFlags(DWORD fdwControl) const
{
    TCHAR buf[128];

    StringCchPrintf(buf, 128, TEXT("control support flags  : 0x%x "), fdwControl);

    if (fdwControl & MIXERCONTROL_CONTROLF_DISABLED)
    {
        
        
        
        StringCchCat(buf, 128, TEXT("(CONTROLF_DISABLED)"));
    }

    if (fdwControl & MIXERCONTROL_CONTROLF_MULTIPLE)
    {
        
        
        
        
        StringCchCat(buf, 128, TEXT("(CONTROLF_MULTIPLE)"));
    }

    if (fdwControl & MIXERCONTROL_CONTROLF_UNIFORM)
    {
        
        
        
        
        
        StringCchCat(buf, 128, TEXT("(CONTROLF_UNIFORM)"));
    }

    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "%s", WideToUTF8(buf));
}





void AudioMixerManager::ClearSpeakerState(UINT idx)
{
    _speakerState[idx].dwLineID = 0L;
    _speakerState[idx].dwVolumeControlID = 0L;
    _speakerState[idx].dwMuteControlID = 0L;
    _speakerState[idx].speakerIsValid = false;
    _speakerState[idx].muteControlIsValid = false;
    _speakerState[idx].volumeControlIsValid = false;
}





void AudioMixerManager::ClearSpeakerState()
{
    for (int i = 0; i < MAX_NUMBER_MIXER_DEVICES; i++)
    {
        ClearSpeakerState(i);
    }
}





bool AudioMixerManager::SpeakerIsValid(UINT idx) const
{
    return (_speakerState[idx].speakerIsValid);
}







UINT AudioMixerManager::ValidSpeakers() const
{
    UINT nSpeakers(0);
    for (int i = 0; i < MAX_NUMBER_MIXER_DEVICES; i++)
    {
        if (SpeakerIsValid(i))
            nSpeakers++;
    }
    return nSpeakers;
}





void AudioMixerManager::ClearMicrophoneState(UINT idx)
{
    _microphoneState[idx].dwLineID = 0L;
    _microphoneState[idx].dwVolumeControlID = 0L;
    _microphoneState[idx].dwMuteControlID = 0L;
    _microphoneState[idx].dwOnOffControlID = 0L;
    _microphoneState[idx].microphoneIsValid = false;
    _microphoneState[idx].muteControlIsValid = false;
    _microphoneState[idx].volumeControlIsValid = false;
    _microphoneState[idx].onOffControlIsValid = false;
}





void AudioMixerManager::ClearMicrophoneState()
{
    for (int i = 0; i < MAX_NUMBER_MIXER_DEVICES; i++)
    {
        ClearMicrophoneState(i);
    }
}





bool AudioMixerManager::MicrophoneIsValid(UINT idx) const
{
    return (_microphoneState[idx].microphoneIsValid);

}








UINT AudioMixerManager::ValidMicrophones() const
{
    UINT nMicrophones(0);
    for (int i = 0; i < MAX_NUMBER_MIXER_DEVICES; i++)
    {
        if (MicrophoneIsValid(i))
            nMicrophones++;
    }
    return nMicrophones;
}





void AudioMixerManager::TraceWaveInError(MMRESULT error) const
{
    TCHAR buf[MAXERRORLENGTH];
    TCHAR msg[MAXERRORLENGTH];

    StringCchPrintf(buf, MAXERRORLENGTH, TEXT("Error details: "));
    waveInGetErrorText(error, msg, MAXERRORLENGTH);
    StringCchCat(buf, MAXERRORLENGTH, msg);
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "%s", WideToUTF8(buf));
}





void AudioMixerManager::TraceWaveOutError(MMRESULT error) const
{
    TCHAR buf[MAXERRORLENGTH];
    TCHAR msg[MAXERRORLENGTH];

    StringCchPrintf(buf, MAXERRORLENGTH, TEXT("Error details: "));
    waveOutGetErrorText(error, msg, MAXERRORLENGTH);
    StringCchCat(buf, MAXERRORLENGTH, msg);
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "%s", WideToUTF8(buf));
}





char* AudioMixerManager::WideToUTF8(const TCHAR* src) const {
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
