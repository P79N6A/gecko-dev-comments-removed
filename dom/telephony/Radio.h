






































#ifndef mozilla_dom_telephony_radio_h__
#define mozilla_dom_telephony_radio_h__

#include "jsapi.h"
#include "nsComponentManagerUtils.h"
#include "nsCOMPtr.h"
#include "nsDebug.h"
#include "nsServiceManagerUtils.h"

#include "nsIObserver.h"
#include "mozilla/ipc/Ril.h"
#include "nsIRadioInterface.h"

#define TELEPHONYRADIO_CONTRACTID "@mozilla.org/telephony/radio;1"
#define TELEPHONYRADIOINTERFACE_CONTRACTID "@mozilla.org/telephony/radio-interface;1"

#define BEGIN_TELEPHONY_NAMESPACE \
  namespace mozilla { namespace dom { namespace telephony {
#define END_TELEPHONY_NAMESPACE \
  } /* namespace telephony */ } /* namespace dom */ } /* namespace mozilla */
#define USING_TELEPHONY_NAMESPACE \
  using namespace mozilla::dom::telephony;


#define TELEPHONYRADIO_CID \
  {0xa5c3a6de, 0x84c4, 0x4b15, {0x86, 0x11, 0x8a, 0xeb, 0x8d, 0x97, 0xf8, 0xba}}


#define TELEPHONYRADIOINTERFACE_CID \
  {0xd66e7ece, 0x41b1, 0x4608, {0x82, 0x80, 0x72, 0x50, 0xa6, 0x44, 0xe6, 0x6f}}


class nsIXPConnectJSObjectHolder;

BEGIN_TELEPHONY_NAMESPACE

class Radio : public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  nsresult Init();
  void Shutdown();

  static already_AddRefed<Radio>
  FactoryCreate();

  static already_AddRefed<nsIRadioInterface>
  GetRadioInterface();

protected:
  Radio();
  ~Radio();

  nsCOMPtr<nsIRadioInterface> mRadioInterface;
  bool mShutdown;
};

END_TELEPHONY_NAMESPACE

#endif 
