




pref("webapprt.app_update_interval", 86400);

pref("browser.chromeURL", "chrome://webapprt/content/webapp.xul");
pref("browser.download.folderList", 1);


pref("extensions.enabledScopes", 1);

pref("extensions.autoDisableScopes", 1);

pref("xpinstall.enabled", false);

pref("extensions.installDistroAddons", false);

pref("extensions.showMismatchUI", false);


pref("breakpad.reportURL", "https://crash-stats.mozilla.com/report/index/");


pref("extensions.blocklist.enabled", true);
pref("extensions.blocklist.interval", 86400);


pref("extensions.blocklist.level", 2);
pref("extensions.blocklist.url", "https://blocklist.addons.mozilla.org/blocklist/3/%APP_ID%/%APP_VERSION%/%PRODUCT%/%BUILD_ID%/%BUILD_TARGET%/%LOCALE%/%CHANNEL%/%OS_VERSION%/%DISTRIBUTION%/%DISTRIBUTION_VERSION%/%PING_COUNT%/%TOTAL_PING_COUNT%/%DAYS_SINCE_LAST_PING%/");
pref("extensions.blocklist.detailsURL", "https://www.mozilla.com/%LOCALE%/blocklist/");
pref("extensions.blocklist.itemURL", "https://blocklist.addons.mozilla.org/%LOCALE%/%APP%/blocked/%blockID%");

pref("full-screen-api.enabled", true);


pref("dom.indexedDB.enabled", true);


pref("browser.offline-apps.notify", false);
pref("browser.cache.offline.enable", true);
pref("offline-apps.allow_by_default", true);


pref("dom.mozTCPSocket.enabled", true);


pref("general.smoothScroll", true);


pref("dom.mozPay.enabled", true);


pref("dom.sysmsg.enabled", true);


pref("dom.mozAlarms.enabled", true);


pref("dom.max_script_run_time", 0);
pref("dom.max_chrome_script_run_time", 0);


pref("geo.wifi.uri", "https://location.services.mozilla.com/v1/geolocate?key=%MOZILLA_API_KEY%");

#ifndef RELEASE_BUILD

pref("dom.payment.provider.0.name", "Firefox Marketplace");
pref("dom.payment.provider.0.description", "marketplace.firefox.com");
pref("dom.payment.provider.0.uri", "https://marketplace.firefox.com/mozpay/?req=");
pref("dom.payment.provider.0.type", "mozilla/payments/pay/v1");
pref("dom.payment.provider.0.requestMethod", "GET");
#endif


pref("dom.always_allow_move_resize_window", true);



pref("plugin.allowed_types", " ");

pref("extensions.blocklist.suppressUI", true);

pref("devtools.debugger.remote-enabled", true);
pref("devtools.debugger.force-local", true);



#ifdef XP_MACOSX

pref("dom.ipc.plugins.enabled.i386", false);
pref("dom.ipc.plugins.enabled.i386.flash player.plugin", true);

pref("dom.ipc.plugins.enabled.x86_64", true);
#else
pref("dom.ipc.plugins.enabled", true);
#endif

pref("places.database.growthIncrementKiB", 0);
