






































#pragma once

#include "CC_Common.h"
#include "ECC_Types.h"

namespace CSF
{
	





	class ECC_API ECC_Observer
	{
	public:
		virtual void onAvailablePhoneEvent (AvailablePhoneEventType::AvailablePhoneEvent event,
											const PhoneDetailsPtr availablePhoneDetails) = 0;

		virtual void onAuthenticationStatusChange (AuthenticationStatusEnum::AuthenticationStatus) = 0;
		virtual void onConnectionStatusChange(ConnectionStatusEnum::ConnectionStatus status) = 0;
	};
}
