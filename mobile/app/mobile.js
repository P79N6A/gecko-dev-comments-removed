



































#filter substitution

pref("toolkit.defaultChromeURI", "chrome://browser/content/browser.xul");
pref("general.useragent.extra.mobile", "@APP_UA_NAME@/@APP_VERSION@");
pref("browser.chromeURL", "chrome://browser/content/");

pref("browser.startup.homepage", "http://www.mozilla.org/");
pref("browser.ui.cursor", false);


pref("browser.cache.disk.enable", false);
pref("browser.cache.disk.capacity", 0);
pref("browser.cache.memory.enable", true);
pref("browser.cache.memory.capacity", 1024);


pref("network.http.pipelining", true);
pref("network.http.pipelining.ssl", true);
pref("network.http.proxy.pipelining", true);
pref("network.http.pipelining.maxrequests" , 2);
pref("network.http.keep-alive.timeout", 600);
pref("network.http.max-connections", 4);
pref("network.http.max-connections-per-server", 1);
pref("network.http.max-persistent-connections-per-server", 1);
pref("network.http.max-persistent-connections-per-proxy", 1);


pref("browser.sessionhistory.max_total_viewers", 0);
pref("browser.sessionhistory.max_entries", 50);


pref("browser.dom.window.dump.enabled", true);
pref("javascript.options.showInConsole", true);
pref("javascript.options.strict", true);
pref("nglayout.debug.disable_xul_cache", false);
pref("nglayout.debug.disable_xul_fastload", false);


pref("browser.download.useDownloadDir", true);
pref("browser.download.folderList", 0);
pref("browser.download.manager.showAlertOnComplete", false);
pref("browser.download.manager.showAlertInterval", 2000);
pref("browser.download.manager.retention", 2);
pref("browser.download.manager.showWhenStarting", true);
pref("browser.download.manager.useWindow", false);
pref("browser.download.manager.closeWhenDone", true);
pref("browser.download.manager.openDelay", 0);
pref("browser.download.manager.focusWhenStarting", false);
pref("browser.download.manager.flashCount", 2);
pref("browser.download.manager.displayedHistoryDays", 7);


pref("alerts.slideIncrement", 1);
pref("alerts.slideIncrementTime", 10);
pref("alerts.totalOpenTime", 6000);
pref("alerts.height", 50);


pref("signon.rememberSignons", true);
pref("signon.expireMasterPassword", false);
pref("signon.SignonFileName", "signons.txt");


pref("browser.formfill.enable", true);


pref("layout.spellcheckDefault", 1);


pref("xpinstall.dialog.confirm", "chrome://mozapps/content/xpinstall/xpinstallConfirm.xul");
pref("xpinstall.dialog.progress.skin", "chrome://mozapps/content/extensions/extensions.xul?type=themes");
pref("xpinstall.dialog.progress.chrome", "chrome://mozapps/content/extensions/extensions.xul?type=extensions");
pref("xpinstall.dialog.progress.type.skin", "Extension:Manager-themes");
pref("xpinstall.dialog.progress.type.chrome", "Extension:Manager-extensions");
pref("extensions.update.enabled", true);
pref("extensions.update.interval", 86400);
pref("extensions.dss.enabled", false);
pref("extensions.dss.switchPending", false);
pref("extensions.ignoreMTimeChanges", false);
pref("extensions.logging.enabled", false);


pref("extensions.update.url", "chrome://mozapps/locale/extensions/extensions.properties");
pref("extensions.getMoreExtensionsURL", "chrome://mozapps/locale/extensions/extensions.properties");
pref("extensions.getMoreThemesURL", "chrome://mozapps/locale/extensions/extensions.properties");


pref("browser.display.use_focus_colors", true);
pref("browser.display.focus_background_color", "#ffffa0");
pref("browser.display.focus_text_color", "#00000");


pref("dom.disable_open_during_load", true);
pref("privacy.popups.showBrowserMessage", true);

pref("snav.enabled", true);
