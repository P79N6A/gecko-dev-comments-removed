






































#pragma once


#include <string>

namespace csf
{

namespace ProviderStateEnum
{
	enum ProviderState
	{
		Ready,
		Registering,
		AwaitingIpAddress,
		FetchingDeviceConfig,
		Idle,
		RecoveryPending,
		Connected
	};
	const std::string toString(ProviderState);
}
namespace LoginErrorStatusEnum
{
	enum LoginErrorStatus {
		Ok,								
		Unknown,						
		NoCallManagerConfigured,		
		NoDevicesFound,					
		NoCsfDevicesFound,				
		PhoneConfigGenError,			
		SipProfileGenError,			    
		ConfigNotSet,					
		CreateConfigProviderFailed,		
		CreateSoftPhoneProviderFailed,	
		MissingUsername,				
		ManualLogout,			        
		LoggedInElseWhere,				
		AuthenticationFailure,			
		CtiCouldNotConnect,				
		InvalidServerSearchList
	};
	const std::string toString(LoginErrorStatus);
}

namespace ErrorCodeEnum
{
	enum ErrorCode
	{
		Ok,
		Unknown,
		InvalidState,
		InvalidArgument
	};
	const std::string toString(ErrorCode);
}

} 

