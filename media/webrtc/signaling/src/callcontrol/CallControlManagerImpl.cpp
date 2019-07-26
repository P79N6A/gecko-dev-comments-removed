






































#include "CC_SIPCCDevice.h"
#include "CC_SIPCCDeviceInfo.h"
#include "CC_SIPCCFeatureInfo.h"
#include "CC_SIPCCLine.h"
#include "CC_SIPCCLineInfo.h"
#include "CC_SIPCCCallInfo.h"
#include "CallControlManagerImpl.h"
#include "CSFLogStream.h"
#include "csf_common.h"

extern "C"
{
#include "config_api.h"
}

static const char* logTag = "CallControlManager";

static std::string logDestination = "CallControl.log";

using namespace std;
using namespace CSFUnified;


namespace CSF
{

CallControlManagerImpl::CallControlManagerImpl()
: multiClusterMode(false),
  sipccLoggingMask(0),
  authenticationStatus(AuthenticationStatusEnum::eNotAuthenticated),
  connectionState(ConnectionStatusEnum::eIdle)
{
    CSFLogInfoS(logTag, "CallControlManagerImpl()");
}

CallControlManagerImpl::~CallControlManagerImpl()
{
    CSFLogInfoS(logTag, "~CallControlManagerImpl()");
    destroy();
}

bool CallControlManagerImpl::destroy()
{
    CSFLogInfoS(logTag, "destroy()");
    bool retval = disconnect();
    if(retval == false)
	{
		return retval;
	}
	return retval;
}


void CallControlManagerImpl::addCCObserver ( CC_Observer * observer )
{
	AutoLock lock(m_lock);
    if (observer == NULL)
    {
        CSFLogErrorS(logTag, "NULL value for \"observer\" passed to addCCObserver().");
        return;
    }

    ccObservers.insert(observer);
}

void CallControlManagerImpl::removeCCObserver ( CC_Observer * observer )
{
	AutoLock lock(m_lock);
    ccObservers.erase(observer);
}

void CallControlManagerImpl::addECCObserver ( ECC_Observer * observer )
{
	AutoLock lock(m_lock);
    if (observer == NULL)
    {
        CSFLogErrorS(logTag, "NULL value for \"observer\" passed to addECCObserver().");
        return;
    }

    eccObservers.insert(observer);
}

void CallControlManagerImpl::removeECCObserver ( ECC_Observer * observer )
{
	AutoLock lock(m_lock);
    eccObservers.erase(observer);
}

void CallControlManagerImpl::setMultiClusterMode(bool allowMultipleClusters)
{
    CSFLogInfoS(logTag, "setMultiClusterMode(" << allowMultipleClusters << ")");
    multiClusterMode = allowMultipleClusters;
}

void CallControlManagerImpl::setSIPCCLoggingMask(const cc_int32_t mask)
{
    CSFLogInfoS(logTag, "setSIPCCLoggingMask(" << mask << ")");
    sipccLoggingMask = mask;
}

void CallControlManagerImpl::setAuthenticationString(const std::string &authString)
{
    CSFLogInfoS(logTag, "setAuthenticationString()");
    this->authString = authString;
}

void CallControlManagerImpl::setSecureCachePath(const std::string &secureCachePath)
{
    CSFLogInfoS(logTag, "setSecureCachePath(" << secureCachePath << ")");
    this->secureCachePath = secureCachePath;
}


void CallControlManagerImpl::setLocalIpAddressAndGateway(const std::string& localIpAddress, const std::string& defaultGW)
{
    CSFLogInfoS(logTag, "setLocalIpAddressAndGateway(" << localIpAddress << ", " << defaultGW << ")");
    this->localIpAddress = localIpAddress;
    this->defaultGW = defaultGW;

    if(softPhone != NULL)
    {
        softPhone->setLocalAddressAndGateway(this->localIpAddress, this->defaultGW);
    }
}

AuthenticationStatusEnum::AuthenticationStatus CallControlManagerImpl::getAuthenticationStatus()
{
    return authenticationStatus;
}

bool CallControlManagerImpl::registerUser( const std::string& deviceName, const std::string& user, const std::string& password, const std::string& domain )
{
	setConnectionState(ConnectionStatusEnum::eRegistering);

    CSFLogInfoS(logTag, "registerUser(" << user << ", " << domain << " )");
    if(phone != NULL)
    {
    	setConnectionState(ConnectionStatusEnum::eReady);

        CSFLogErrorS(logTag, "registerUser() failed - already connected!");
        return false;
    }

    
    if(localIpAddress.empty() || localIpAddress == "127.0.0.1")
    {
    	setConnectionState(ConnectionStatusEnum::eFailed);
    	CSFLogErrorS(logTag, "registerUser() failed - No local IP address set!");
    	return false;
    }

    softPhone = CC_SIPCCServicePtr(new CC_SIPCCService());
    phone = softPhone;
    phone->init(user, password, domain, deviceName);
    softPhone->setLoggingMask(sipccLoggingMask);
    softPhone->setLocalAddressAndGateway(localIpAddress, defaultGW);
    phone->addCCObserver(this);

    phone->setP2PMode(false);

    bool bStarted = phone->startService();
    if (!bStarted) {
        setConnectionState(ConnectionStatusEnum::eFailed);
    } else {
        setConnectionState(ConnectionStatusEnum::eReady);
    }

    return bStarted;
}

bool CallControlManagerImpl::startP2PMode(const std::string& user)
{
	setConnectionState(ConnectionStatusEnum::eRegistering);

    CSFLogInfoS(logTag, "startP2PMode(" << user << " )");
    if(phone != NULL)
    {
    	setConnectionState(ConnectionStatusEnum::eReady);

        CSFLogErrorS(logTag, "startP2PMode() failed - already started in p2p mode!");
        return false;
    }

    
    if(localIpAddress.empty() || localIpAddress == "127.0.0.1")
    {
    	setConnectionState(ConnectionStatusEnum::eFailed);
    	CSFLogErrorS(logTag, "startP2PMode() failed - No local IP address set!");
    	return false;
    }

    softPhone = CC_SIPCCServicePtr(new CC_SIPCCService());
    phone = softPhone;
    phone->init(user, "", "127.0.0.1", "sipdevice");
    softPhone->setLoggingMask(sipccLoggingMask);
    softPhone->setLocalAddressAndGateway(localIpAddress, defaultGW);
    phone->addCCObserver(this);

    phone->setP2PMode(true);

    bool bStarted = phone->startService();
    if (!bStarted) {
        setConnectionState(ConnectionStatusEnum::eFailed);
    } else {
        setConnectionState(ConnectionStatusEnum::eReady);
    }

    return bStarted;
}

bool CallControlManagerImpl::startROAPProxy( const std::string& deviceName, const std::string& user, const std::string& password, const std::string& domain )
{
	setConnectionState(ConnectionStatusEnum::eRegistering);

    CSFLogInfoS(logTag, "startROAPProxy(" << user << ", " << domain << " )");
    if(phone != NULL)
    {
    	setConnectionState(ConnectionStatusEnum::eReady);

        CSFLogErrorS(logTag, "startROAPProxy() failed - already connected!");
        return false;
    }

    
    if(localIpAddress.empty() || localIpAddress == "127.0.0.1")
    {
    	setConnectionState(ConnectionStatusEnum::eFailed);
    	CSFLogErrorS(logTag, "startROAPProxy() failed - No local IP address set!");
    	return false;
    }

    softPhone = CC_SIPCCServicePtr(new CC_SIPCCService());
    phone = softPhone;
    phone->init(user, password, domain, deviceName);
    softPhone->setLoggingMask(sipccLoggingMask);
    softPhone->setLocalAddressAndGateway(localIpAddress, defaultGW);
    phone->addCCObserver(this);

    phone->setP2PMode(false);
    phone->setROAPProxyMode(true);

    bool bStarted = phone->startService();
    if (!bStarted) {
        setConnectionState(ConnectionStatusEnum::eFailed);
    } else {
        setConnectionState(ConnectionStatusEnum::eReady);
    }

    return bStarted;
}

bool CallControlManagerImpl::disconnect()
{
    CSFLogInfoS(logTag, "disconnect()");
    if(phone == NULL)
        return true;

    connectionState = ConnectionStatusEnum::eIdle;
    phone->removeCCObserver(this);
    phone->stop();
    phone->destroy();
    phone.reset();
    softPhone.reset();

    return true;
}

std::string CallControlManagerImpl::getPreferredDeviceName()
{
    return preferredDevice;
}

std::string CallControlManagerImpl::getPreferredLineDN()
{
    return preferredLineDN;
}

ConnectionStatusEnum::ConnectionStatus CallControlManagerImpl::getConnectionStatus()
{
    return connectionState;
}

std::string CallControlManagerImpl::getCurrentServer()
{
    return "";
}


CC_DevicePtr CallControlManagerImpl::getActiveDevice()
{
    if(phone != NULL)
        return phone->getActiveDevice();

    return CC_DevicePtr();
}


PhoneDetailsVtrPtr CallControlManagerImpl::getAvailablePhoneDetails()
{
    PhoneDetailsVtrPtr result = PhoneDetailsVtrPtr(new PhoneDetailsVtr());
    for(PhoneDetailsMap::iterator it = phoneDetailsMap.begin(); it != phoneDetailsMap.end(); it++)
    {
        PhoneDetailsPtr details = it->second;
        result->push_back(details);
    }
    return result;
}
PhoneDetailsPtr CallControlManagerImpl::getAvailablePhoneDetails(const std::string& deviceName)
{
    PhoneDetailsMap::iterator it = phoneDetailsMap.find(deviceName);
    if(it != phoneDetailsMap.end())
    {
        return it->second;
    }
    return PhoneDetailsPtr();
}


VideoControlPtr CallControlManagerImpl::getVideoControl()
{
    if(phone != NULL)
        return phone->getVideoControl();

    return VideoControlPtr();
}

AudioControlPtr CallControlManagerImpl::getAudioControl()
{
    if(phone != NULL)
        return phone->getAudioControl();

    return AudioControlPtr();
}

bool CallControlManagerImpl::setProperty(ConfigPropertyKeysEnum::ConfigPropertyKeys key, std::string& value)
{
	CSFLogInfoS(logTag, "setProperty(" << value << " )");

	if (key == ConfigPropertyKeysEnum::eLocalVoipPort) {
		CCAPI_Config_set_local_voip_port(atoi(value.c_str()));
	} else if (key == ConfigPropertyKeysEnum::eRemoteVoipPort) {
		CCAPI_Config_set_remote_voip_port(atoi(value.c_str()));
	} else if (key == ConfigPropertyKeysEnum::eTransport) {
		if (value == "tcp")
			CCAPI_Config_set_transport_udp(false);
		else
			CCAPI_Config_set_transport_udp(true);
	}

	return true;
}

std::string CallControlManagerImpl::getProperty(ConfigPropertyKeysEnum::ConfigPropertyKeys key)
{
	CSFLogInfoS(logTag, "getProperty()");

	std::string retValue = "NONESET";
	if (key == ConfigPropertyKeysEnum::eLocalVoipPort) {
		int tmpValue = CCAPI_Config_get_local_voip_port();
		std::stringstream out;
		out << tmpValue;
		retValue = out.str();
	} else if (key == ConfigPropertyKeysEnum::eRemoteVoipPort) {
		int tmpValue = CCAPI_Config_get_remote_voip_port();
		std::stringstream out;
		out << tmpValue;
		retValue = out.str();
	} else if (key == ConfigPropertyKeysEnum::eVersion) {
		const char* version = CCAPI_Config_get_version();
		retValue = version;
	}

	return retValue;
}














































































void CallControlManagerImpl::onDeviceEvent(ccapi_device_event_e deviceEvent, CC_DevicePtr devicePtr, CC_DeviceInfoPtr info)
{
    notifyDeviceEventObservers(deviceEvent, devicePtr, info);
}
void CallControlManagerImpl::onFeatureEvent(ccapi_device_event_e deviceEvent, CC_DevicePtr devicePtr, CC_FeatureInfoPtr info)
{
    notifyFeatureEventObservers(deviceEvent, devicePtr, info);
}
void CallControlManagerImpl::onLineEvent(ccapi_line_event_e lineEvent,     CC_LinePtr linePtr, CC_LineInfoPtr info)
{
    notifyLineEventObservers(lineEvent, linePtr, info);
}
void CallControlManagerImpl::onCallEvent(ccapi_call_event_e callEvent,     CC_CallPtr callPtr, CC_CallInfoPtr info, char* sdp)
{
    notifyCallEventObservers(callEvent, callPtr, info, sdp);
}


void CallControlManagerImpl::notifyDeviceEventObservers (ccapi_device_event_e deviceEvent, CC_DevicePtr devicePtr, CC_DeviceInfoPtr info)
{
	AutoLock lock(m_lock);
    set<CC_Observer*>::const_iterator it = ccObservers.begin();
    for ( ; it != ccObservers.end(); it++ )
    {
        (*it)->onDeviceEvent(deviceEvent, devicePtr, info);
    }
}

void CallControlManagerImpl::notifyFeatureEventObservers (ccapi_device_event_e deviceEvent, CC_DevicePtr devicePtr, CC_FeatureInfoPtr info)
{
	AutoLock lock(m_lock);
    set<CC_Observer*>::const_iterator it = ccObservers.begin();
    for ( ; it != ccObservers.end(); it++ )
    {
        (*it)->onFeatureEvent(deviceEvent, devicePtr, info);
    }
}

void CallControlManagerImpl::notifyLineEventObservers (ccapi_line_event_e lineEvent, CC_LinePtr linePtr, CC_LineInfoPtr info)
{
	AutoLock lock(m_lock);
    set<CC_Observer*>::const_iterator it = ccObservers.begin();
    for ( ; it != ccObservers.end(); it++ )
    {
        (*it)->onLineEvent(lineEvent, linePtr, info);
    }
}

void CallControlManagerImpl::notifyCallEventObservers (ccapi_call_event_e callEvent, CC_CallPtr callPtr, CC_CallInfoPtr info, char* sdp)
{
	AutoLock lock(m_lock);
    set<CC_Observer*>::const_iterator it = ccObservers.begin();
    for ( ; it != ccObservers.end(); it++ )
    {
        (*it)->onCallEvent(callEvent, callPtr, info, sdp);
    }
}

void CallControlManagerImpl::notifyAvailablePhoneEvent (AvailablePhoneEventType::AvailablePhoneEvent event,
        const PhoneDetailsPtr availablePhoneDetails)
{
	AutoLock lock(m_lock);
    set<ECC_Observer*>::const_iterator it = eccObservers.begin();
    for ( ; it != eccObservers.end(); it++ )
    {
        (*it)->onAvailablePhoneEvent(event, availablePhoneDetails);
    }
}

void CallControlManagerImpl::notifyAuthenticationStatusChange (AuthenticationStatusEnum::AuthenticationStatus status)
{
	AutoLock lock(m_lock);
    set<ECC_Observer*>::const_iterator it = eccObservers.begin();
    for ( ; it != eccObservers.end(); it++ )
    {
        (*it)->onAuthenticationStatusChange(status);
    }
}

void CallControlManagerImpl::notifyConnectionStatusChange(ConnectionStatusEnum::ConnectionStatus status)
{
	AutoLock lock(m_lock);
    set<ECC_Observer*>::const_iterator it = eccObservers.begin();
    for ( ; it != eccObservers.end(); it++ )
    {
        (*it)->onConnectionStatusChange(status);
    }
}

void CallControlManagerImpl::setConnectionState(ConnectionStatusEnum::ConnectionStatus status)
{
	connectionState = status;
	notifyConnectionStatusChange(status);
}

}
