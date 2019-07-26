



pref("datareporting.healthreport.currentDaySubmissionFailureCount", 0);
pref("datareporting.healthreport.documentServerURI", "https://data.mozilla.com/");
pref("datareporting.healthreport.documentServerNamespace", "metrics");
pref("datareporting.healthreport.infoURL", "https://www.mozilla.org/legal/privacy/firefox.html#health-report");
pref("datareporting.healthreport.logging.consoleEnabled", true);
pref("datareporting.healthreport.logging.consoleLevel", "Warn");
pref("datareporting.healthreport.lastDataSubmissionFailureTime", "0");
pref("datareporting.healthreport.lastDataSubmissionRequestedTime", "0");
pref("datareporting.healthreport.lastDataSubmissionSuccessfulTime", "0");
pref("datareporting.healthreport.nextDataSubmissionTime", "0");
pref("datareporting.healthreport.pendingDeleteRemoteData", false);


pref("datareporting.healthreport.uploadEnabled", true);

pref("datareporting.healthreport.service.enabled", true);
pref("datareporting.healthreport.service.loadDelayMsec", 10000);
pref("datareporting.healthreport.service.loadDelayFirstRunMsec", 60000);

pref("datareporting.healthreport.service.providerCategories",
#if MOZ_UPDATE_CHANNEL == release
    "healthreport-js-provider-default"
#else
    "healthreport-js-provider-default,healthreport-js-provider-@MOZ_UPDATE_CHANNEL@"
#endif
    );

pref("datareporting.healthreport.about.reportUrl",   "https://fhr.cdn.mozilla.net/%LOCALE%/");
