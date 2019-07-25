






































#ifndef mozilla_dom_telephony_telephonycommon_h__
#define mozilla_dom_telephony_telephonycommon_h__

#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsDebug.h"
#include "nsDOMEventTargetWrapperCache.h"
#include "nsStringGlue.h"
#include "nsTArray.h"

#define BEGIN_TELEPHONY_NAMESPACE \
  namespace mozilla { namespace dom { namespace telephony {
#define END_TELEPHONY_NAMESPACE \
  } /* namespace telephony */ } /* namespace dom */ } /* namespace mozilla */
#define USING_TELEPHONY_NAMESPACE \
  using namespace mozilla::dom::telephony;

class nsIDOMTelephony;
class nsIDOMTelephonyCall;

BEGIN_TELEPHONY_NAMESPACE

enum {
  kOutgoingPlaceholderCallIndex = PR_UINT32_MAX
};

class Telephony;
class TelephonyCall;

END_TELEPHONY_NAMESPACE

#endif
