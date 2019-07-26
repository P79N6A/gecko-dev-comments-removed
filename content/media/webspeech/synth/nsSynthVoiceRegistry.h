





#pragma once

#include "nsAutoPtr.h"
#include "nsISynthVoiceRegistry.h"
#include "nsRefPtrHashtable.h"
#include "nsTArray.h"

class nsISpeechService;

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

  already_AddRefed<nsSpeechTask> SpeakUtterance(SpeechSynthesisUtterance& aUtterance,
                                                const nsAString& aDocLang);

  void Speak(const nsAString& aText, const nsAString& aLang,
             const nsAString& aUri, const float& aRate, const float& aPitch,
             nsSpeechTask* aTask);

  static nsSynthVoiceRegistry* GetInstance();

  static already_AddRefed<nsSynthVoiceRegistry> GetInstanceForService();

  static void Shutdown();

private:
  VoiceData* FindBestMatch(const nsAString& aUri, const nsAString& lang);

  bool FindVoiceByLang(const nsAString& aLang, VoiceData** aRetval);

  nsresult AddVoiceImpl(nsISpeechService* aService,
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
