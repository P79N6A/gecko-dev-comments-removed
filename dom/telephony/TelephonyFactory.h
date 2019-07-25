





#ifndef mozilla_dom_telephony_telephonyfactory_h__
#define mozilla_dom_telephony_telephonyfactory_h__

#include "nsIDOMTelephony.h"
#include "nsIDOMVoicemail.h"
#include "nsPIDOMWindow.h"



nsresult
NS_NewTelephony(nsPIDOMWindow* aWindow, nsIDOMTelephony** aTelephony);

nsresult
NS_NewVoicemail(nsPIDOMWindow* aWindow, nsIDOMMozVoicemail** aVoicemail);

#endif 
