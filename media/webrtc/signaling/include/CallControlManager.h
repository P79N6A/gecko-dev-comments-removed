



#pragma once

#include "CC_Common.h"

#include "CC_Observer.h"
#include "ECC_Observer.h"
#include "ECC_Types.h"

#include <string>
#include <vector>




















namespace CSF
{
	DECLARE_NS_PTR(CallControlManager)
	









    class ECC_API CallControlManager
    {
    public:
        NS_INLINE_DECL_THREADSAFE_REFCOUNTING(CallControlManager)
		






        static CallControlManagerPtr create();
        virtual bool destroy() = 0;

        virtual ~CallControlManager();

        









        virtual void addCCObserver ( CC_Observer * observer ) = 0;
        virtual void removeCCObserver ( CC_Observer * observer ) = 0;

        virtual void addECCObserver ( ECC_Observer * observer ) = 0;
        virtual void removeECCObserver ( ECC_Observer * observer ) = 0;

        virtual void setMultiClusterMode(bool allowMultipleClusters) = 0;
        virtual void setSIPCCLoggingMask(const cc_int32_t mask) = 0;
        virtual void setAuthenticationString(const std::string &authString) = 0;
        virtual void setSecureCachePath(const std::string &secureCachePath) = 0;

        
        virtual void setAudioCodecs(int codecMask) = 0;
        virtual void setVideoCodecs(int codecMask) = 0;

        virtual bool registerUser(const std::string& deviceName, const std::string& user, const std::string& password, const std::string& domain) = 0;
        virtual bool disconnect() = 0;
        virtual std::string getPreferredDeviceName() = 0;
        virtual std::string getPreferredLineDN() = 0;
        virtual ConnectionStatusEnum::ConnectionStatus getConnectionStatus() = 0;
        virtual std::string getCurrentServer() = 0;

        
        virtual bool startP2PMode(const std::string& user) = 0;

        
        virtual bool startSDPMode() = 0;

        



        virtual CC_DevicePtr getActiveDevice() = 0;
        virtual PhoneDetailsVtrPtr getAvailablePhoneDetails() = 0;
        virtual PhoneDetailsPtr getAvailablePhoneDetails(const std::string& deviceName) = 0;

        






        virtual VideoControlPtr getVideoControl() = 0;
        virtual AudioControlPtr getAudioControl() = 0;

        virtual bool setProperty(ConfigPropertyKeysEnum::ConfigPropertyKeys key, std::string& value) = 0;
        virtual std::string getProperty(ConfigPropertyKeysEnum::ConfigPropertyKeys key) = 0;

    protected:
        CallControlManager() {}
    private:
        CallControlManager(const CallControlManager&);
        CallControlManager& operator=(const CallControlManager&);
    };
} 
