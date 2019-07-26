



pref("browser.chromeURL", "chrome://webapprt/content/webapp.xul");
pref("browser.download.folderList", 1);


pref("extensions.enabledScopes", 1);

pref("extensions.autoDisableScopes", 1);

pref("xpinstall.enabled", false);

pref("extensions.installDistroAddons", false);

pref("extensions.showMismatchUI", false);


pref("webapprt.firstrun", false);


pref("extensions.blocklist.enabled", true);
pref("extensions.blocklist.interval", 86400);


pref("extensions.blocklist.level", 2);
pref("extensions.blocklist.url", "https://addons.mozilla.org/blocklist/3/%APP_ID%/%APP_VERSION%/%PRODUCT%/%BUILD_ID%/%BUILD_TARGET%/%LOCALE%/%CHANNEL%/%OS_VERSION%/%DISTRIBUTION%/%DISTRIBUTION_VERSION%/%PING_COUNT%/%TOTAL_PING_COUNT%/%DAYS_SINCE_LAST_PING%/");
pref("extensions.blocklist.detailsURL", "https://www.mozilla.com/%LOCALE%/blocklist/");
pref("extensions.blocklist.itemURL", "https://addons.mozilla.org/%LOCALE%/%APP%/blocked/%blockID%");

pref("full-screen-api.enabled", true);


pref("dom.indexedDB.enabled", true);
pref("indexedDB.feature.enabled", true);
pref("dom.indexedDB.warningQuota", 50);


pref("browser.offline-apps.notify", false);
pref("browser.cache.offline.enable", true);
pref("offline-apps.allow_by_default", true);


pref("dom.mozTCPSocket.enabled", true);


pref("general.smoothScroll", true);


pref("dom.always_allow_move_resize_window", true);

pref("plugin.allowed_types", "application/x-shockwave-flash,application/futuresplash");



#ifdef XP_MACOSX

pref("dom.ipc.plugins.enabled.i386", false);
pref("dom.ipc.plugins.enabled.i386.flash player.plugin", true);

pref("dom.ipc.plugins.enabled.x86_64", true);
#else
pref("dom.ipc.plugins.enabled", true);
#endif

