














#include "../../video_capture_config.h"
#include "video_capture_quick_time_info.h"

#include "critical_section_wrapper.h"
#include "event_wrapper.h"
#include "thread_wrapper.h"
#include "trace.h"
#include "video_capture.h"

namespace webrtc
{

VideoCaptureMacQuickTimeInfo::VideoCaptureMacQuickTimeInfo(
    const int32_t iID) :
    DeviceInfoImpl(iID), _id(iID),
    _grabberCritsect(CriticalSectionWrapper::CreateCriticalSection())
{
}

VideoCaptureMacQuickTimeInfo::~VideoCaptureMacQuickTimeInfo()
{
}

int32_t VideoCaptureMacQuickTimeInfo::Init()
{

    return 0;
}

uint32_t VideoCaptureMacQuickTimeInfo::NumberOfDevices()
{
    int numOfDevices = 0;

    
    const int kNameLength = 1024;
    char deviceNameUTF8[kNameLength] = "";
    char deviceUniqueIdUTF8[kNameLength] = "";
    char productUniqueIdUTF8[kNameLength] = "";

    if (GetCaptureDevices(0, deviceNameUTF8, kNameLength, deviceUniqueIdUTF8,
                          kNameLength, productUniqueIdUTF8, kNameLength,
                          numOfDevices) != 0)
    {
        return 0;
    }

    return numOfDevices;
}

int32_t VideoCaptureMacQuickTimeInfo::GetDeviceName(
    uint32_t deviceNumber, char* deviceNameUTF8,
    uint32_t deviceNameUTF8Length, char* deviceUniqueIdUTF8,
    uint32_t deviceUniqueIdUTF8Length, char* productUniqueIdUTF8,
    uint32_t productUniqueIdUTF8Length)
{

    int numOfDevices = 0; 
    return GetCaptureDevices(deviceNumber, deviceNameUTF8,
                             deviceNameUTF8Length, deviceUniqueIdUTF8,
                             deviceUniqueIdUTF8Length, productUniqueIdUTF8,
                             productUniqueIdUTF8Length, numOfDevices);
}

int32_t VideoCaptureMacQuickTimeInfo::NumberOfCapabilities(
    const char* deviceUniqueIdUTF8)
{
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, _id,
                 "NumberOfCapabilities is not supported on the Mac platform.");
    return -1;
}

int32_t VideoCaptureMacQuickTimeInfo::GetCapability(
    const char* deviceUniqueIdUTF8,
    const uint32_t deviceCapabilityNumber,
    VideoCaptureCapability& capability)
{
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, _id,
                 "NumberOfCapabilities is not supported on the Mac platform.");
    return -1;
}

int32_t VideoCaptureMacQuickTimeInfo::GetBestMatchedCapability(
    const char*deviceUniqueIdUTF8,
    const VideoCaptureCapability& requested, VideoCaptureCapability& resulting)
{
    WEBRTC_TRACE(webrtc::kTraceInfo, webrtc::kTraceVideoCapture, _id,
                 "NumberOfCapabilities is not supported on the Mac platform.");
    return -1;
}

int32_t VideoCaptureMacQuickTimeInfo::DisplayCaptureSettingsDialogBox(
    const char* deviceUniqueIdUTF8,
    const char* dialogTitleUTF8, void* parentWindow,
    uint32_t positionX, uint32_t positionY)
{
     return -1;
}

int32_t VideoCaptureMacQuickTimeInfo::CreateCapabilityMap(
    const char* deviceUniqueIdUTF8)
{
    WEBRTC_TRACE(webrtc::kTraceInfo, webrtc::kTraceVideoCapture, _id,
                 "NumberOfCapabilities is not supported on the Mac platform.");
    return -1;
}

int VideoCaptureMacQuickTimeInfo::GetCaptureDevices(
    uint32_t deviceNumber, char* deviceNameUTF8,
    uint32_t deviceNameUTF8Length, char* deviceUniqueIdUTF8,
    uint32_t deviceUniqueIdUTF8Length, char* productUniqueIdUTF8,
    uint32_t productUniqueIdUTF8Length, int& numberOfDevices)
{


    numberOfDevices = 0;
    memset(deviceNameUTF8, 0, deviceNameUTF8Length);
    memset(deviceUniqueIdUTF8, 0, deviceUniqueIdUTF8Length);
    memset(productUniqueIdUTF8, 0, productUniqueIdUTF8Length);

    if (deviceNumber < 0)
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, _id,
                     "%s:%d Invalid deviceNumber", __FUNCTION__, __LINE__);
        return -1;
    }

    Component captureDevice = NULL;
    SeqGrabComponent captureGrabber = NULL;
    SGChannel captureChannel = NULL;
    bool closeChannel = false;

    ComponentDescription compCaptureType;

    compCaptureType.componentType = SeqGrabComponentType;
    compCaptureType.componentSubType = 0;
    compCaptureType.componentManufacturer = 0;
    compCaptureType.componentFlags = 0;
    compCaptureType.componentFlagsMask = 0;

    
    long numSequenceGrabbers = CountComponents(&compCaptureType);

    if (deviceNumber > numSequenceGrabbers)
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, _id,
                     "%s:%d Invalid deviceNumber", __FUNCTION__, __LINE__);
        return -1;
    }

    if (numSequenceGrabbers <= 0)
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, _id,
                     "%s:%d No sequence grabbers available", __FUNCTION__,
                     __LINE__);
        return -1;
    }

    
    for (int seqGrabberIndex = 0;
         seqGrabberIndex < numSequenceGrabbers;
         seqGrabberIndex++)
    {
        captureDevice = FindNextComponent(0, &compCaptureType);
        captureGrabber = OpenComponent(captureDevice);
        if (captureGrabber != NULL)
        {
            
            if (SGInitialize(captureGrabber) != noErr)
            {
                CloseComponent(captureGrabber);
                WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture,
                             _id, "%s:%d Could not init the sequence grabber",
                             __FUNCTION__, __LINE__);
                return -1;
            }
            break;
        }
        if (seqGrabberIndex == numSequenceGrabbers - 1)
        {
            
            WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, _id,
                         "%s:%d Could not open a sequence grabber",
                         __FUNCTION__, __LINE__);
            return -1;
        }
    }

    
    
    if (SGNewChannel(captureGrabber, VideoMediaType, &captureChannel) != noErr)
    {
        
        SGRelease(captureGrabber);
        CloseComponent(captureGrabber);
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, _id,
                     "%s:%d Could not create a sequence grabber video channel",
                     __FUNCTION__, __LINE__);
        return -1;
    }
    closeChannel = true;

    
    SGDeviceList deviceList = NULL;
    if (SGGetChannelDeviceList(captureChannel, sgDeviceListIncludeInputs,
                               &deviceList) != noErr)
    {
        if (closeChannel)
            SGDisposeChannel(captureGrabber, captureChannel);
        if (captureGrabber)
        {
            SGRelease(captureGrabber);
            CloseComponent(captureGrabber);
        }
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, _id,
                     "%s:%d Could not create a device list", __FUNCTION__,
                     __LINE__);
        return -1;
    }

    
    
    int numDevices = (*deviceList)->count;
    WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCapture, _id,
                 "%s:%d Found %d devices", __FUNCTION__, __LINE__, numDevices);

    for (int i = 0; i < numDevices; i++)
    {

        SGDeviceName sgDeviceName = (*deviceList)->entry[i];
        
        if (sgDeviceName.inputs)
        {
            SGDeviceInputList inputList = sgDeviceName.inputs;
            int numInputDev = (*inputList)->count;

            for (int inputDevIndex = 0;
                 inputDevIndex < numInputDev;
                 inputDevIndex++)
            {
                
                SGDeviceInputName deviceInputName =
                    (*inputList)->entry[inputDevIndex];

                VideoCaptureMacName* deviceName = new VideoCaptureMacName();

                deviceName->_size = PascalStringToCString(
                    deviceInputName.name, deviceName->_name,
                    sizeof(deviceName->_name));

                if (deviceName->_size > 0)
                {
                    WEBRTC_TRACE(webrtc::kTraceDebug,webrtc::kTraceVideoCapture,
                                 _id,
                                 "%s:%d Capture device %d: %s was successfully "
                                 "set", __FUNCTION__, __LINE__, numberOfDevices,
                                 deviceName->_name);

                    if (numberOfDevices == deviceNumber)
                    {
                        strcpy((char*) deviceNameUTF8, deviceName->_name);
                        strcpy((char*) deviceUniqueIdUTF8, deviceName->_name);
                    }
                    numberOfDevices++;
                }
                else
                {
                    delete deviceName;

                    if (deviceName->_size < 0)
                    {
                        WEBRTC_TRACE(webrtc::kTraceError,
                                     webrtc::kTraceVideoCapture, _id,
                                     "%s:%d Error in PascalStringToCString",
                                     __FUNCTION__, __LINE__);
                        return -1;
                    }
                }
            }
        }
    }

    
    SGDisposeDeviceList(captureGrabber, deviceList);
    if (closeChannel)
    {
        SGDisposeChannel(captureGrabber, captureChannel);
    }
    if (captureGrabber)
    {
        SGRelease(captureGrabber);
        CloseComponent(captureGrabber);
    }

    WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCapture, _id,
                 "%s:%d End function successfully", __FUNCTION__, __LINE__);
    return 0;
}
















CFIndex VideoCaptureMacQuickTimeInfo::PascalStringToCString(
    const unsigned char* pascalString, char* cString, CFIndex bufferSize)
{
    if (pascalString == NULL)
    {
        return -1;
    }

    if (cString == NULL)
    {
        return -1;
    }

    if (bufferSize == 0)
    {
        return -1;
    }

    CFIndex cStringLength = 0;
    CFIndex maxStringLength = bufferSize - 1;

    CFStringRef cfString = CFStringCreateWithPascalString(
        NULL, pascalString, kCFStringEncodingMacRoman);
    if (cfString == NULL)
    {
        CFRelease(cfString);
        return -1;
    }

    CFIndex cfLength = CFStringGetLength(cfString);
    cStringLength = cfLength;
    if (cfLength > maxStringLength)
    {
        cStringLength = maxStringLength;
    }

    Boolean success = CFStringGetCString(cfString, cString, bufferSize,
                                         kCFStringEncodingMacRoman);

    
    
    if (success == false && cfLength <= maxStringLength)
    {
        CFRelease(cfString);
        return -1;
    }

    CFRelease(cfString);
    return cStringLength;
}







VideoCaptureMacQuickTimeInfo::VideoCaptureMacName::VideoCaptureMacName() :
    _size(0)
{
    memset(_name, 0, kVideoCaptureMacNameMaxSize);
}
}  
