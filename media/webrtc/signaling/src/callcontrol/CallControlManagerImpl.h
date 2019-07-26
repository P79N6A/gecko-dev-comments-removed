






































#pragma once

#include "CallControlManager.h"
#include "PhoneDetailsImpl.h"
#include "CC_SIPCCService.h"

#include "base/lock.h"


#include <set>
#include <map>

namespace CSF
{
	class CallControlManagerImpl: public CallControlManager, public CC_Observer
	{
	public:
		CallControlManagerImpl();
        virtual bool destroy();
		virtual ~CallControlManagerImpl();

		
        virtual void addCCObserver ( CC_Observer * observer );
        virtual void removeCCObserver ( CC_Observer * observer );

        virtual void addECCObserver ( ECC_Observer * observer );
        virtual void removeECCObserver ( ECC_Observer * observer );

        
        virtual void setMultiClusterMode(bool allowMultipleClusters);
        virtual void setSIPCCLoggingMask(const cc_int32_t mask);
        virtual void setAuthenticationString(const std::string &authString);
        virtual void setSecureCachePath(const std::string &secureCachePath);

        
        virtual void setLocalIpAddressAndGateway(const std::string& localIpAddress, const std::string& defaultGW);

        virtual AuthenticationStatusEnum::AuthenticationStatus getAuthenticationStatus();

        virtual bool registerUser( const std::string& deviceName, const std::string& user, const std::string& password, const std::string& domain );

        virtual bool startP2PMode(const std::string& user);

        virtual bool startROAPProxy( const std::string& deviceName, const std::string& user, const std::string& password, const std::string& domain );

        virtual bool disconnect();
        virtual std::string getPreferredDeviceName();
        virtual std::string getPreferredLineDN();
        virtual ConnectionStatusEnum::ConnectionStatus getConnectionStatus();
        virtual std::string getCurrentServer();

        
        virtual CC_DevicePtr getActiveDevice();

        
        virtual PhoneDetailsVtrPtr getAvailablePhoneDetails();
        virtual PhoneDetailsPtr getAvailablePhoneDetails(const std::string& deviceName);

        
        virtual VideoControlPtr getVideoControl();
        virtual AudioControlPtr getAudioControl();

        virtual bool setProperty(ConfigPropertyKeysEnum::ConfigPropertyKeys key, std::string& value);
        virtual std::string getProperty(ConfigPropertyKeysEnum::ConfigPropertyKeys key);

	private: 

        
		Lock m_lock;;
		std::set<CC_Observer *> ccObservers;
		std::set<ECC_Observer *> eccObservers;

        
		std::string username;
		std::string password;
		std::string authString;
		std::string secureCachePath;
		bool multiClusterMode;
		cc_int32_t sipccLoggingMask;

        
		std::string localIpAddress;
		std::string defaultGW;

		AuthenticationStatusEnum::AuthenticationStatus authenticationStatus;

		std::string preferredDevice;
		std::string preferredLineDN;
		CC_ServicePtr phone;			
		CC_SIPCCServicePtr softPhone;	

        
		typedef std::map<std::string, PhoneDetailsImplPtr> PhoneDetailsMap;
		PhoneDetailsMap phoneDetailsMap;

		
		ConnectionStatusEnum::ConnectionStatus connectionState;

	public: 
		
		void onDeviceEvent  (ccapi_device_event_e deviceEvent, CC_DevicePtr devicePtr, CC_DeviceInfoPtr info);
		void onFeatureEvent (ccapi_device_event_e deviceEvent, CC_DevicePtr devicePtr, CC_FeatureInfoPtr info);
		void onLineEvent    (ccapi_line_event_e lineEvent,     CC_LinePtr linePtr, CC_LineInfoPtr info);
		void onCallEvent    (ccapi_call_event_e callEvent,     CC_CallPtr callPtr, CC_CallInfoPtr info, char* sdp);

	private: 

		
		void notifyDeviceEventObservers  (ccapi_device_event_e deviceEvent, CC_DevicePtr devicePtr, CC_DeviceInfoPtr info);
		void notifyFeatureEventObservers (ccapi_device_event_e deviceEvent, CC_DevicePtr devicePtr, CC_FeatureInfoPtr info);
		void notifyLineEventObservers    (ccapi_line_event_e lineEvent,     CC_LinePtr linePtr, CC_LineInfoPtr info);
		void notifyCallEventObservers    (ccapi_call_event_e callEvent,     CC_CallPtr callPtr, CC_CallInfoPtr info, char* sdp);

		
		void notifyAvailablePhoneEvent (AvailablePhoneEventType::AvailablePhoneEvent event,
											const PhoneDetailsPtr phoneDetails);
		void notifyAuthenticationStatusChange (AuthenticationStatusEnum::AuthenticationStatus);
		void notifyConnectionStatusChange(ConnectionStatusEnum::ConnectionStatus status);
		void setConnectionState(ConnectionStatusEnum::ConnectionStatus status);
	};

}
