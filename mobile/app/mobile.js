



































#filter substitution

pref("toolkit.defaultChromeURI", "chrome://browser/content/browser.xul");
pref("general.useragent.extra.mobile", "@APP_UA_NAME@/@APP_VERSION@");
pref("browser.chromeURL", "chrome://browser/content/");

pref("browser.startup.homepage", "about:firstrun");

#if MOZ_PLATFORM_HILDON
pref("browser.ui.cursor", false);
#elifdef WINCE
pref("browser.ui.cursor", false);
#else
pref("browser.ui.cursor", true);
#endif

#ifdef MOZ_PLATFORM_HILDON
pref("browser.ui.panning.fixup.mousemove", true);
#else
pref("browser.ui.panning.fixup.mousemove", false);
#endif


pref("ui.use_native_popup_windows", true);

pref("javascript.options.showInConsole", false);
pref("browser.dom.window.dump.enabled", false);


#ifdef MOZ_PLATFORM_HILDON
pref("browser.cache.disk.enable", true);
pref("browser.cache.disk.capacity", 10240); 
pref("browser.cache.disk.parent_directory", "/media/mmc2/.mozilla/@APP_NAME@");
#endif
#ifdef WINCE
pref("browser.cache.disk.enable", false);
pref("browser.cache.disk.capacity", 0); 
#endif
pref("browser.cache.memory.enable", true);
pref("browser.cache.memory.capacity", 1024); 


pref("image.cache.size", 1048576); 


pref("browser.cache.offline.enable", true);
pref("browser.cache.offline.capacity", 5120); 
pref("offline-apps.quota.max", 2048); 
pref("offline-apps.quota.warn", 1024); 
#ifdef MOZ_PLATFORM_HILDON
pref("browser.cache.offline.parent_directory", "/media/mmc2/.mozilla/@APP_NAME@");
#endif


pref("network.http.pipelining", true);
pref("network.http.pipelining.ssl", true);
pref("network.http.proxy.pipelining", true);
pref("network.http.pipelining.maxrequests" , 6);
pref("network.http.keep-alive.timeout", 600);
pref("network.http.max-connections", 6);
pref("network.http.max-connections-per-server", 4);
pref("network.http.max-persistent-connections-per-server", 4);
pref("network.http.max-persistent-connections-per-proxy", 4);


pref("browser.sessionhistory.max_total_viewers", 1);
pref("browser.sessionhistory.max_entries", 50);


pref("mozilla.widget.force-24bpp", true);
pref("mozilla.widget.use-buffer-pixmap", true);
pref("mozilla.widget.disable-native-theme", true);


pref("browser.download.useDownloadDir", true);
pref("browser.download.folderList", 1); 
pref("browser.download.manager.showAlertOnComplete", false);
pref("browser.download.manager.showAlertInterval", 2000);
pref("browser.download.manager.retention", 2);
pref("browser.download.manager.showWhenStarting", false);
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
pref("xpinstall.dialog.progress.skin", "chrome://browser/content/browser.xul");
pref("xpinstall.dialog.progress.chrome", "chrome://browser/content/browser.xul");
pref("xpinstall.dialog.progress.type.skin", "navigator:browser");
pref("xpinstall.dialog.progress.type.chrome", "navigator:browser");
pref("xpinstall.whitelist.add", "addons.mozilla.org");
pref("extensions.update.enabled", true);
pref("extensions.update.interval", 86400);
pref("extensions.dss.enabled", false);
pref("extensions.dss.switchPending", false);
pref("extensions.ignoreMTimeChanges", false);
pref("extensions.logging.enabled", false);
pref("extensions.hideInstallButton", true);
pref("extensions.disabledAddons.showUI", true);


pref("extensions.update.url", "chrome://mozapps/locale/extensions/extensions.properties");
pref("extensions.getMoreExtensionsURL", "chrome://mozapps/locale/extensions/extensions.properties");
pref("extensions.getMoreThemesURL", "chrome://mozapps/locale/extensions/extensions.properties");


pref("extensions.getAddons.showPane", true);
pref("extensions.getAddons.browseAddons", "https://%LOCALE%.add-ons.mozilla.com/%LOCALE%/%APP%");
pref("extensions.getAddons.maxResults", 5);
pref("extensions.getAddons.recommended.browseURL", "https://%LOCALE%.add-ons.mozilla.com/%LOCALE%/%APP%/recommended");
pref("extensions.getAddons.recommended.url", "https://services.addons.mozilla.org/%LOCALE%/%APP%/api/%API_VERSION%/list/featured/all/10/%OS%/%VERSION%");
pref("extensions.getAddons.search.browseURL", "https://%LOCALE%.add-ons.mozilla.com/%LOCALE%/%APP%/search?q=%TERMS%");
pref("extensions.getAddons.search.url", "https://services.addons.mozilla.org/%LOCALE%/%APP%/api/%API_VERSION%/search/%TERMS%/all/10/%OS%/%VERSION%");


pref("extensions.blocklist.enabled", true);
pref("extensions.blocklist.interval", 86400);
pref("extensions.blocklist.url", "https://addons.mozilla.org/blocklist/2/%APP_ID%/%APP_VERSION%/%PRODUCT%/%BUILD_ID%/%BUILD_TARGET%/%LOCALE%/%CHANNEL%/%OS_VERSION%/%DISTRIBUTION%/%DISTRIBUTION_VERSION%/");
pref("extensions.blocklist.detailsURL", "http://%LOCALE%.www.mozilla.com/%LOCALE%/blocklist/");


pref("browser.dictionaries.download.url", "https://%LOCALE%.add-ons.mozilla.com/%LOCALE%/firefox/%VERSION%/dictionaries/");


pref("browser.display.use_focus_colors", true);
pref("browser.display.focus_background_color", "#ffffa0");
pref("browser.display.focus_text_color", "#00000");
pref("browser.display.focus_ring_on_anything", true);
pref("browser.display.focus_ring_style", 0);


pref("dom.disable_open_during_load", true);
pref("privacy.popups.showBrowserMessage", true);

pref("keyword.enabled", true);
pref("keyword.URL", "http://www.google.com/search?ie=UTF-8&oe=UTF-8&sourceid=navclient&gfns=1&q=");

pref("snav.enabled", false);

pref("accessibility.typeaheadfind", false);
pref("accessibility.typeaheadfind.timeout", 5000);
pref("accessibility.typeaheadfind.flashBar", 1);
pref("accessibility.typeaheadfind.linksonly", false);
pref("accessibility.typeaheadfind.casesensitive", 0);

pref("accessibility.browsewithcaret_shortcut.enabled", false);



pref("app.update.showInstalledUI", false);


pref("browser.search.defaultenginename",      "chrome://browser/locale/region.properties");

pref("browser.ssl_override_behavior", 2);
pref("browser.xul.error_pages.expert_bad_cert", false);


pref("browser.search.log", false);


pref("browser.search.order.1", "chrome://browser/locale/region.properties");
pref("browser.search.order.2", "chrome://browser/locale/region.properties");


pref("browser.search.update", false);
pref("browser.search.update.log", false);
pref("browser.search.updateinterval", 6);


pref("browser.search.suggest.enabled", true);


pref("browser.xul.error_pages.enabled", true);


pref("browser.urlbar.default.behavior", 0);
pref("browser.urlbar.default.behavior.emptyRestriction", 0);



pref("browser.urlbar.clickSelectsAll", true);
pref("browser.urlbar.doubleClickSelectsAll", true);
pref("browser.urlbar.autoFill", false);
pref("browser.urlbar.matchOnlyTyped", false);
pref("browser.urlbar.matchBehavior", 1);
pref("browser.urlbar.filter.javascript", true);
pref("browser.urlbar.maxRichResults", 12);
pref("browser.urlbar.search.chunkSize", 1000);
pref("browser.urlbar.search.timeout", 100);
pref("browser.urlbar.restrict.history", "^");
pref("browser.urlbar.restrict.bookmark", "*");
pref("browser.urlbar.restrict.tag", "+");
pref("browser.urlbar.match.title", "#");
pref("browser.urlbar.match.url", "@");
pref("browser.history.grouping", "day");
pref("browser.history.showSessions", false);
pref("browser.sessionhistory.max_entries", 50);
pref("browser.history_expire_days", 180);
pref("browser.history_expire_days_min", 90);
pref("browser.history_expire_sites", 40000);
pref("privacy.item.history",     true);
pref("browser.places.migratePostDataAnnotations", true);
pref("browser.places.updateRecentTagsUri", true);
pref("places.frecency.numVisits", 10);
pref("places.frecency.numCalcOnIdle", 50);
pref("places.frecency.numCalcOnMigrate", 50);
pref("places.frecency.updateIdleTime", 60000);
pref("places.frecency.firstBucketCutoff", 4);
pref("places.frecency.secondBucketCutoff", 14);
pref("places.frecency.thirdBucketCutoff", 31);
pref("places.frecency.fourthBucketCutoff", 90);
pref("places.frecency.firstBucketWeight", 100);
pref("places.frecency.secondBucketWeight", 70);
pref("places.frecency.thirdBucketWeight", 50);
pref("places.frecency.fourthBucketWeight", 30);
pref("places.frecency.defaultBucketWeight", 10);
pref("places.frecency.embedVisitBonus", 0);
pref("places.frecency.linkVisitBonus", 100);
pref("places.frecency.typedVisitBonus", 2000);
pref("places.frecency.bookmarkVisitBonus", 150);
pref("places.frecency.downloadVisitBonus", 0);
pref("places.frecency.permRedirectVisitBonus", 0);
pref("places.frecency.tempRedirectVisitBonus", 0);
pref("places.frecency.defaultVisitBonus", 0);
pref("places.frecency.unvisitedBookmarkBonus", 140);
pref("places.frecency.unvisitedTypedBonus", 200);


pref("gfx.color_management.mode", 0);


pref("dom.disable_window_move_resize", true);


pref("browser.enable_click_image_resizing", false);



pref("browser.link.open_external", 3);
pref("browser.link.open_newwindow", 3);

pref("browser.link.open_newwindow.restriction", 0);


pref("privacy.sanitize.promptOnSanitize", false);
pref("privacy.item.cache", true);
pref("privacy.item.cookies", true);
pref("privacy.item.offlineApps", true);
pref("privacy.item.history", true);
pref("privacy.item.formdata", true);
pref("privacy.item.downloads", true);
pref("privacy.item.passwords", true);
pref("privacy.item.sessions", true);
pref("privacy.item.geolocation", true);
pref("privacy.item.siteSettings", true);


pref("plugins.enabled", true);


pref("browser.geolocation.warning.infoURL", "http://%LOCALE%.www.mozilla.com/%LOCALE%/firefox/geolocation/");


pref("geo.wifi.uri", "https://www.google.com/loc/json");


pref("geo.enabled", true);

#ifdef WINCE
pref("layout.css.devPixelsPerPx", 1);
#endif


pref("content.max.deflected.tokens", 10);
pref("content.max.tokenizing.time", 50000);

pref("javascript.options.jit.content", true);
pref("javascript.options.jit.chrome", true);
pref("javascript.options.mem.gc_frequency", 300);

pref("dom.max_chrome_script_run_time", 30);
pref("dom.max_script_run_time", 20);


pref("browser.console.showInPanel", false);


pref("browser.ui.kinetic.updateInterval", 33);
pref("browser.ui.kinetic.ema.alphaValue", 8);
pref("browser.ui.kinetic.decelerationRate", 15);


pref("plugin.default_plugin_disabled", true);


pref("app.releaseNotesURL", "http://www.mozilla.com/%LOCALE%/%APP%/%VERSION%/releasenotes/");
pref("app.support.baseURL", "http://support.mozilla.com/1/%APP%/%VERSION%/%OS%/%LOCALE%/");


pref("security.alternate_certificate_error_page", "certerror");
