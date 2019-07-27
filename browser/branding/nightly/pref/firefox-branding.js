



pref("startup.homepage_override_url", "https://www.mozilla.org/projects/firefox/%VERSION%/whatsnew/?oldversion=%OLD_VERSION%");
pref("startup.homepage_welcome_url", "https://www.mozilla.org/projects/firefox/%VERSION%/firstrun/");

pref("app.update.interval", 7200); 



pref("app.update.download.backgroundInterval", 0);

pref("app.update.promptWaitTime", 43200);


pref("app.update.url.manual", "https://nightly.mozilla.org");


pref("app.update.url.details", "https://nightly.mozilla.org");




pref("app.update.checkInstallTime.days", 2);


pref("browser.search.param.yahoo-fr", "moz35");
pref("browser.search.param.yahoo-fr-ja", "mozff");
#ifdef MOZ_METRO
pref("browser.search.param.yahoo-fr-metro", "");
#endif



pref("devtools.selfxss.count", 5);