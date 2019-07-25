







































#ifdef WINCE_WINDOWS_MOBILE
#include <windows.h>
#include <phone.h>
#endif

#ifdef MOZ_PLATFORM_HILDON
#include <dbus/dbus.h>
#endif

#include "nsStringAPI.h"
#include "nsIPhoneSupport.h"
#include "nsIGenericFactory.h"

class nsPhoneSupport : public nsIPhoneSupport
{
public:

  NS_DECL_ISUPPORTS
  NS_DECL_NSIPHONESUPPORT

  nsPhoneSupport() {};
  ~nsPhoneSupport(){};

};

NS_IMPL_ISUPPORTS1(nsPhoneSupport, nsIPhoneSupport)

NS_IMETHODIMP
nsPhoneSupport::MakeCall(const PRUnichar *telephoneNumber, const PRUnichar *telephoneDescription, PRBool aPrompt)
{
  long result = -1;

#ifdef WINCE_WINDOWS_MOBILE
  typedef LONG (*__PhoneMakeCall)(PHONEMAKECALLINFO *ppmci);

  HMODULE hPhoneDLL = LoadLibraryW(L"phone.dll");
  if (hPhoneDLL) {
    __PhoneMakeCall MakeCall = (__PhoneMakeCall) GetProcAddress(hPhoneDLL,
                                                                "PhoneMakeCall");
    if (MakeCall) {
      PHONEMAKECALLINFO callInfo;
      callInfo.cbSize          = sizeof(PHONEMAKECALLINFO);
      callInfo.dwFlags         = aPrompt ? PMCF_PROMPTBEFORECALLING : PMCF_DEFAULT;
      callInfo.pszDestAddress  = telephoneNumber;
      callInfo.pszAppName      = nsnull;
      callInfo.pszCalledParty  = telephoneDescription;
      callInfo.pszComment      = nsnull;
      result = MakeCall(&callInfo);
    }
    FreeLibrary(hPhoneDLL);
  }
#endif

  return (result == 0) ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsPhoneSupport::SwitchTask()
{
#ifdef MOZ_PLATFORM_HILDON
  DBusError error;
  dbus_error_init(&error);

  DBusConnection *conn = dbus_bus_get(DBUS_BUS_SESSION, &error);

  DBusMessage *msg = dbus_message_new_signal("/com/nokia/hildon_desktop",
                                             "com.nokia.hildon_desktop",
                                             "exit_app_view");

  if (msg) {
      dbus_connection_send(conn, msg, NULL);
      dbus_message_unref(msg);
      dbus_connection_flush(conn);
  }
  return NS_OK;
#else
  return NS_ERROR_NOT_IMPLEMENTED;
#endif
}





#define nsPhoneSupport_CID                          \
{ 0x2a08c9e4, 0xf853, 0x4f02,                       \
{0x88, 0xd8, 0xd6, 0x2f, 0x27, 0xca, 0x06, 0x85} }

#define nsPhoneSupport_ContractID "@mozilla.org/phone/support;1"

NS_GENERIC_FACTORY_CONSTRUCTOR(nsPhoneSupport)

static const nsModuleComponentInfo components[] =
{
  { "Phone Support",
    nsPhoneSupport_CID,
    nsPhoneSupport_ContractID,
    nsPhoneSupportConstructor,
    nsnull,
    nsnull
  }
};

NS_IMPL_NSGETMODULE(nsPhoneSupportModule, components)
