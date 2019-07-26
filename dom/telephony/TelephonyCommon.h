





#ifndef mozilla_dom_telephony_telephonycommon_h__
#define mozilla_dom_telephony_telephonycommon_h__

#include "mozilla/Attributes.h"
#include "mozilla/ErrorResult.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsDebug.h"
#include "nsDOMEventTargetHelper.h"
#include "nsStringGlue.h"
#include "nsTArray.h"

#define BEGIN_TELEPHONY_NAMESPACE \
  namespace mozilla { namespace dom { namespace telephony {
#define END_TELEPHONY_NAMESPACE \
  } /* namespace telephony */ } /* namespace dom */ } /* namespace mozilla */
#define USING_TELEPHONY_NAMESPACE \
  using namespace mozilla::dom::telephony;

BEGIN_TELEPHONY_NAMESPACE

enum {
  kOutgoingPlaceholderCallIndex = UINT32_MAX
};

class CallsList;
class Telephony;
class TelephonyCall;
class TelephonyCallGroup;

END_TELEPHONY_NAMESPACE

#endif
