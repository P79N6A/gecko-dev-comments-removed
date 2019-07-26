





#pragma once

#include "mozilla/Mutex.h"
#include "nsAutoPtr.h"
#include "nsTArray.h"
#include "nsIThread.h"
#include "nsISpeechService.h"
#include "nsRefPtrHashtable.h"
#include "mozilla/StaticPtr.h"
#include "mozilla/Monitor.h"

namespace mozilla {
namespace dom {

class PicoVoice;
class PicoCallbackRunnable;

typedef void* pico_System;
typedef void* pico_Resource;
typedef void* pico_Engine;

class nsPicoService : public nsISpeechService
{
  friend class PicoCallbackRunnable;
  friend class PicoInitRunnable;

public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSISPEECHSERVICE

  nsPicoService();

  virtual ~nsPicoService();

  static nsPicoService* GetInstance();

  static already_AddRefed<nsPicoService> GetInstanceForService();

  static void Shutdown();

private:

  void Init();

  void RegisterVoices();

  bool GetVoiceFileLanguage(const nsACString& aFileName, nsAString& aLang);

  void LoadEngine(PicoVoice* aVoice);

  void UnloadEngine();

  PicoVoice* CurrentVoice();

  bool mInitialized;

  nsCOMPtr<nsIThread> mThread;

  nsRefPtrHashtable<nsStringHashKey, PicoVoice> mVoices;

  Monitor mVoicesMonitor;

  PicoVoice* mCurrentVoice;

  Atomic<nsISpeechTask*> mCurrentTask;

  pico_System mPicoSystem;

  pico_Engine mPicoEngine;

  pico_Resource mSgResource;

  pico_Resource mTaResource;

  nsAutoPtr<uint8_t> mPicoMemArea;

  static StaticRefPtr<nsPicoService> sSingleton;
};

} 
} 
