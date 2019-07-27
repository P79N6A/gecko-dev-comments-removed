



#include "mozilla/dom/MozMobileConnectionBinding.h"
#include "nsIMobileConnectionService.h"

namespace mozilla {
namespace dom {

#define ASSERT_NETWORK_SELECTION_MODE_EQUALITY(webidlState, xpidlState) \
  static_assert(static_cast<int32_t>(MobileNetworkSelectionMode::webidlState) == nsIMobileConnection::xpidlState, \
                "MobileNetworkSelectionMode::" #webidlState " should equal to nsIMobileConnection::" #xpidlState)

ASSERT_NETWORK_SELECTION_MODE_EQUALITY(Automatic, NETWORK_SELECTION_MODE_AUTOMATIC);
ASSERT_NETWORK_SELECTION_MODE_EQUALITY(Manual, NETWORK_SELECTION_MODE_MANUAL);

#undef ASSERT_NETWORK_SELECTION_MODE_EQUALITY

#define ASSERT_MOBILE_RADIO_STATE_EQUALITY(webidlState, xpidlState) \
  static_assert(static_cast<int32_t>(MobileRadioState::webidlState) == nsIMobileConnection::xpidlState, \
                "MobileRadioState::" #webidlState " should equal to nsIMobileConnection::" #xpidlState)

ASSERT_MOBILE_RADIO_STATE_EQUALITY(Enabling, MOBILE_RADIO_STATE_ENABLING);
ASSERT_MOBILE_RADIO_STATE_EQUALITY(Enabled, MOBILE_RADIO_STATE_ENABLED);
ASSERT_MOBILE_RADIO_STATE_EQUALITY(Disabling, MOBILE_RADIO_STATE_DISABLING);
ASSERT_MOBILE_RADIO_STATE_EQUALITY(Disabled, MOBILE_RADIO_STATE_DISABLED);

#undef ASSERT_MOBILE_RADIO_STATE_EQUALITY

} 
} 
