#filter substitution




"use strict";

this.EXPORTED_SYMBOLS = ["AppConstants"];


this.AppConstants = Object.freeze({
  
  
  NIGHTLY_BUILD:
#ifdef NIGHTLY_BUILD
  true,
#else
  false,
#endif

  RELEASE_BUILD:
#ifdef RELEASE_BUILD
  true,
#else
  false,
#endif

  ACCESSIBILITY:
#ifdef ACCESSIBILITY
  true,
#else
  false,
#endif

  
  
  
  MOZILLA_OFFICIAL:
#ifdef MOZILLA_OFFICIAL
  true,
#else
  false,
#endif

  MOZ_OFFICIAL_BRANDING:
#ifdef MOZ_OFFICIAL_BRANDING
  true,
#else
  false,
#endif

  MOZ_SERVICES_HEALTHREPORT:
#ifdef MOZ_SERVICES_HEALTHREPORT
  true,
#else
  false,
#endif

  MOZ_DEVICES:
#ifdef MOZ_DEVICES
  true,
#else
  false,
#endif

  MOZ_SAFE_BROWSING:
#ifdef MOZ_SAFE_BROWSING
  true,
#else
  false,
#endif

  MOZ_SANDBOX:
#ifdef MOZ_SANDBOX
  true,
#else
  false,
#endif

  MOZ_SHARK:
#ifdef XP_MACOSX
#ifdef MOZ_SHARK
  true,
#else
  false,
#endif
#else
  false,
#endif

  MOZ_TELEMETRY_REPORTING:
#ifdef MOZ_TELEMETRY_REPORTING
  true,
#else
  false,
#endif

  MOZ_UPDATER:
#ifdef MOZ_UPDATER
  true,
#else
  false,
#endif

  MOZ_WEBRTC:
#ifdef MOZ_WEBRTC
  true,
#else
  false,
#endif

# NOTE! XP_LINUX has to go after MOZ_WIDGET_ANDROID otherwise Android
# builds will be misidentified as linux.
  platform:
#ifdef MOZ_WIDGET_GTK
  "linux",
#elif MOZ_WIDGET_QT
  "linux",
#elif XP_WIN
  "win",
#elif XP_MACOSX
  "macosx",
#elif MOZ_WIDGET_ANDROID
  "android",
#elif MOZ_WIDGET_GONK
  "gonk",
#elif XP_LINUX
  "linux",
#else
  "other",
#endif

  MOZ_CRASHREPORTER:
#ifdef MOZ_CRASHREPORTER
  true,
#else
  false,
#endif

  MOZ_MAINTENANCE_SERVICE:
#ifdef MOZ_MAINTENANCE_SERVICE
  true,
#else
  false,
#endif

  E10S_TESTING_ONLY:
#ifdef E10S_TESTING_ONLY
  true,
#else
  false,
#endif

  DEBUG:
#ifdef DEBUG
  true,
#else
  false,
#endif

  MOZ_APP_NAME: "@MOZ_APP_NAME@",
  MOZ_APP_VERSION: "@MOZ_APP_VERSION@",
  MOZ_BUILD_APP: "@MOZ_BUILD_APP@",
  MOZ_UPDATE_CHANNEL: "@MOZ_UPDATE_CHANNEL@",
  MOZ_WIDGET_TOOLKIT: "@MOZ_WIDGET_TOOLKIT@",
  ANDROID_PACKAGE_NAME: "@ANDROID_PACKAGE_NAME@",
});
