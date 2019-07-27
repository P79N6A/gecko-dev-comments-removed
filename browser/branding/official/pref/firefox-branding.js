



pref("startup.homepage_override_url","");
pref("startup.homepage_welcome_url","https://www.mozilla.org/%LOCALE%/firefox/%VERSION%/firstrun/");

pref("app.update.interval", 43200); 


pref("app.update.download.backgroundInterval", 60);

pref("app.update.promptWaitTime", 172800);


pref("app.update.url.manual", "https://www.mozilla.org/firefox/");


pref("app.update.url.details", "https://www.mozilla.org/%LOCALE%/firefox/notes");




pref("app.update.checkInstallTime.days", 63);


pref("browser.search.param.yahoo-fr", "moz35");
pref("browser.search.param.yahoo-fr-ja", "mozff");
#ifdef MOZ_METRO
pref("browser.search.param.ms-pc-metro", "MOZW");
pref("browser.search.param.yahoo-fr-metro", "mozilla_metro_search");
#endif



pref("devtools.selfxss.count", 0);