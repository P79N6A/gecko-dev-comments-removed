





#pragma once

#include "nsAutoPtr.h"
#include "nsISynthVoiceRegistry.h"
#include "nsRefPtrHashtable.h"
#include "nsTArray.h"

namespace mozilla {
namespace dom {

class RemoteVoice;
class SpeechSynthesisUtterance;
class nsSpeechTask;
class VoiceData;

class nsSynthVoiceRegistry : public nsISynthVoiceRegistry
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISYNTHVOICEREGISTRY

  nsSynthVoiceRegistry();
  virtual ~nsSynthVoiceRegistry();

  static nsSynthVoiceRegistry* GetInstance();

  static already_AddRefed<nsSynthVoiceRegistry> GetInstanceForService();

  static void Shutdown();

private:
  nsresult AddVoiceImpl(nsISupports* aService,
                        const nsAString& aUri,
                        const nsAString& aName,
                        const nsAString& aLang,
                        bool aLocalService);

  nsTArray<nsRefPtr<VoiceData> > mVoices;

  nsTArray<nsRefPtr<VoiceData> > mDefaultVoices;

  nsRefPtrHashtable<nsStringHashKey, VoiceData> mUriVoiceMap;
};

} 
} 
