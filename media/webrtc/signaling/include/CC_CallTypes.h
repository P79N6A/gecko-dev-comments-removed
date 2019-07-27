



#pragma once

#include "CC_Common.h"
#include <string>
#include <set>

namespace CSF
{
	namespace CC_CallCapabilityEnum
	{
		typedef enum
		{
			canSetRemoteWindow,
			canSetLocalWindow,
			canSendIFrame,
			canOriginateCall,
			canAnswerCall,
			canHold,
			canResume,
			canEndCall,
			canSendDigit,
			canBackspace,
			canRedial,
			canInitiateCallForwardAll,
			canEndConsultativeCall,
			canConferenceStart,
			canConferenceComplete,
			canTransferStart,
			canTransferComplete,
			canCancelTransferOrConferenceFeature,
			canDirectTransfer,
			canJoinAcrossLine,
			canBlfCallPickup,
			canSelect,
			canUpdateVideoMediaCap,
			canSendInfo,
			canMuteAudio,
			canUnmuteAudio,
			canMuteVideo,
			canUnmuteVideo,
			canSetVolume
		} CC_CallCapability;
		std::string ECC_API toString(CC_CallCapability cap);
		std::string ECC_API toString(std::set<CC_CallCapability>& caps);
	};
};
