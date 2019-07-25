



































#filter substitution


pref("toolkit.browser.cachePixelX", 580);
pref("toolkit.browser.cachePixelY", 1000);
pref("toolkit.browser.recacheRatio", 60);

pref("toolkit.defaultChromeURI", "chrome://browser/content/browser.xul");
pref("general.useragent.compatMode.firefox", true);
pref("browser.chromeURL", "chrome://browser/content/");

pref("browser.tabs.warnOnClose", true);
#ifdef MOZ_IPC
pref("browser.tabs.remote", true);
#else
pref("browser.tabs.remote", false);
#endif

pref("toolkit.screen.lock", false);


pref("zoom.minPercent", 20);
pref("zoom.maxPercent", 400);
pref("toolkit.zoomManager.zoomValues", ".2,.3,.5,.67,.8,.9,1,1.1,1.2,1.33,1.5,1.7,2,2.4,3,4");
pref("zoom.dpiScale", 150);


pref("ui.use_native_popup_windows", true);


pref("ui.click_hold_context_menus", true);


pref("browser.cache.disk.enable", false);
pref("browser.cache.disk.capacity", 0); 

pref("browser.cache.memory.enable", true);
pref("browser.cache.memory.capacity", 1024); 


pref("image.cache.size", 1048576); 


pref("browser.offline-apps.notify", true);
pref("browser.cache.offline.enable", true);
pref("browser.cache.offline.capacity", 5120); 
pref("offline-apps.quota.max", 2048); 
pref("offline-apps.quota.warn", 1024); 


pref("network.protocol-handler.warn-external.tel", false);
pref("network.protocol-handler.warn-external.mailto", false);


pref("network.http.pipelining", true);
pref("network.http.pipelining.ssl", true);
pref("network.http.proxy.pipelining", true);
pref("network.http.pipelining.maxrequests" , 6);
pref("network.http.keep-alive.timeout", 600);
pref("network.http.max-connections", 6);
pref("network.http.max-connections-per-server", 4);
pref("network.http.max-persistent-connections-per-server", 4);
pref("network.http.max-persistent-connections-per-proxy", 4);
#ifdef MOZ_PLATFORM_MAEMO
pref("network.autodial-helper.enabled", true);
#endif


pref("browser.display.history.maxresults", 100);


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


pref("formhelper.enabled", true);
pref("formhelper.autozoom", true);
pref("formhelper.restore", false);


pref("findhelper.autozoom", true);


pref("browser.formfill.enable", true);


pref("browser.microsummary.enabled", false);
pref("browser.microsummary.updateGenerators", false);


pref("layout.spellcheckDefault", 1);


pref("xpinstall.whitelist.add", "addons.mozilla.org");

pref("extensions.autoupdate.enabled", true);
pref("extensions.autoupdate.interval", 86400);
pref("extensions.update.enabled", false);
pref("extensions.update.interval", 86400);
pref("extensions.dss.enabled", false);
pref("extensions.dss.switchPending", false);
pref("extensions.ignoreMTimeChanges", false);
pref("extensions.logging.enabled", false);
pref("extensions.hideInstallButton", true);
pref("extensions.showMismatchUI", false);
pref("extensions.hideUpdateButton", false);

pref("extensions.update.url", "https://versioncheck.addons.mozilla.org/update/VersionCheck.php?reqVersion=%REQ_VERSION%&id=%ITEM_ID%&version=%ITEM_VERSION%&maxAppVersion=%ITEM_MAXAPPVERSION%&status=%ITEM_STATUS%&appID=%APP_ID%&appVersion=%APP_VERSION%&appOS=%APP_OS%&appABI=%APP_ABI%&locale=%APP_LOCALE%&currentAppVersion=%CURRENT_APP_VERSION%&updateType=%UPDATE_TYPE%");


pref("extensions.getAddons.cache.enabled", false);
pref("extensions.getAddons.maxResults", 15);
pref("extensions.getAddons.recommended.browseURL", "https://addons.mozilla.org/%LOCALE%/mobile/recommended/");
pref("extensions.getAddons.recommended.url", "https://services.addons.mozilla.org/%LOCALE%/mobile/api/%API_VERSION%/list/featured/all/%MAX_RESULTS%/%OS%/%VERSION%");
pref("extensions.getAddons.search.browseURL", "https://addons.mozilla.org/%LOCALE%/mobile/search?q=%TERMS%");
pref("extensions.getAddons.search.url", "https://services.addons.mozilla.org/%LOCALE%/mobile/api/%API_VERSION%/search/%TERMS%/all/%MAX_RESULTS%/%OS%/%VERSION%");
pref("extensions.getAddons.browseAddons", "https://addons.mozilla.org/%LOCALE%/mobile/");


pref("extensions.blocklist.enabled", true);
pref("extensions.blocklist.interval", 86400);
pref("extensions.blocklist.url", "https://addons.mozilla.org/blocklist/3/%APP_ID%/%APP_VERSION%/%PRODUCT%/%BUILD_ID%/%BUILD_TARGET%/%LOCALE%/%CHANNEL%/%OS_VERSION%/%DISTRIBUTION%/%DISTRIBUTION_VERSION%/%PING_COUNT%/");
pref("extensions.blocklist.detailsURL", "https://www.mozilla.com/%LOCALE%/blocklist/");


pref("dom.disable_open_during_load", true);
pref("privacy.popups.showBrowserMessage", true);

pref("keyword.enabled", true);
pref("keyword.URL", "http://www.google.com/search?ie=UTF-8&oe=UTF-8&sourceid=navclient&gfns=1&q=");

pref("accessibility.typeaheadfind", false);
pref("accessibility.typeaheadfind.timeout", 5000);
pref("accessibility.typeaheadfind.flashBar", 1);
pref("accessibility.typeaheadfind.linksonly", false);
pref("accessibility.typeaheadfind.casesensitive", 0);

pref("accessibility.browsewithcaret_shortcut.enabled", false);



pref("app.update.showInstalledUI", false);


pref("browser.search.defaultenginename", "chrome://browser/locale/region.properties");

pref("browser.ssl_override_behavior", 2);
pref("browser.xul.error_pages.expert_bad_cert", false);


pref("browser.search.log", false);


pref("browser.search.order.1", "chrome://browser/locale/region.properties");
pref("browser.search.order.2", "chrome://browser/locale/region.properties");


pref("browser.search.update", false);
pref("browser.search.update.log", false);
pref("browser.search.updateinterval", 6);


pref("browser.search.suggest.enabled", true);


pref("browser.search.loadFromJars", true);
pref("browser.search.jarURIs", "chrome://browser/locale/searchplugins/");


pref("browser.search.noCurrentEngine", true);


pref("browser.xul.error_pages.enabled", true);


pref("browser.urlbar.default.behavior", 0);
pref("browser.urlbar.default.behavior.emptyRestriction", 0);



pref("places.favicons.optimizeToDimension", 32);



pref("browser.urlbar.clickSelectsAll", true);
pref("browser.urlbar.doubleClickSelectsAll", true);
pref("browser.urlbar.autoFill", false);
pref("browser.urlbar.matchOnlyTyped", false);
pref("browser.urlbar.matchBehavior", 1);
pref("browser.urlbar.filter.javascript", true);
pref("browser.urlbar.maxRichResults", 24); 
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
pref("privacy.item.history", true);
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

#ifdef MOZ_PLATFORM_MAEMO
pref("plugins.force.wmode", "opaque");
#endif


pref("browser.geolocation.warning.infoURL", "http://www.mozilla.com/%LOCALE%/firefox/geolocation/");


pref("geo.wifi.uri", "https://www.google.com/loc/json");


pref("geo.enabled", true);



pref("content.sink.enable_perf_mode",  2); 
pref("content.sink.pending_event_mode", 0);
pref("content.sink.perf_deflect_count", 1000000);
pref("content.sink.perf_parse_time", 50000000);

pref("javascript.options.mem.gc_frequency", 300);
pref("javascript.options.methodjit.chrome", true);

pref("dom.max_chrome_script_run_time", 0); 
pref("dom.max_script_run_time", 20);


pref("browser.console.showInPanel", false);


pref("browser.ui.kinetic.updateInterval", 30);
pref("browser.ui.kinetic.decelerationRate", 20);
pref("browser.ui.kinetic.speedSensitivity", 80);
pref("browser.ui.kinetic.swipeLength", 160);


pref("browser.ui.zoom.pageFitGranularity", 5); 
pref("browser.ui.zoom.animationDuration", 200); 


pref("browser.ui.pinch.maxGrowth", 150);     
pref("browser.ui.pinch.maxShrink", 200);     
pref("browser.ui.pinch.scalingFactor", 500); 


pref("browser.ui.touch.left", 8);
pref("browser.ui.touch.right", 8);
pref("browser.ui.touch.top", 12);
pref("browser.ui.touch.bottom", 4);
pref("browser.ui.touch.weight.visited", 120); 


pref("plugin.disable", true);
pref("dom.ipc.plugins.enabled", false);



pref("breakpad.reportURL", "http://crash-stats.mozilla.com/report/index/");
pref("app.releaseNotesURL", "http://www.mozilla.com/%LOCALE%/mobile/%VERSION%/releasenotes/");

pref("app.support.baseURL", "http://mobile.support.mozilla.com/");
pref("app.privacyURL", "https://www.mozilla.com/%LOCALE%/legal/privacy/firefox/mobile/");
pref("app.creditsURL", "http://www.mozilla.com/%LOCALE%/mobile/credits/");
#if MOZ_UPDATE_CHANNEL == beta
pref("app.featuresURL", "http://www.mozilla.com/%LOCALE%/mobile/beta/features/");
pref("app.faqURL", "http://www.mozilla.com/%LOCALE%/mobile/beta/faq/");
#else
pref("app.featuresURL", "http://www.mozilla.com/%LOCALE%/mobile/features/");
pref("app.faqURL", "http://www.mozilla.com/%LOCALE%/mobile/faq/");
#endif


pref("security.alternate_certificate_error_page", "certerror");


pref("ui.-moz-dialog", "#efebe7");
pref("ui.-moz-dialogtext", "#101010");
pref("ui.-moz-field", "#fff");
pref("ui.-moz-fieldtext", "#1a1a1a");
pref("ui.-moz-buttonhoverface", "#f3f0ed");
pref("ui.-moz-buttonhovertext", "#101010");
pref("ui.-moz-combobox", "#fff");
pref("ui.-moz-comboboxtext", "#101010");
pref("ui.buttonface", "#ece7e2");
pref("ui.buttonhighlight", "#fff");
pref("ui.buttonshadow", "#aea194");
pref("ui.buttontext", "#101010");
pref("ui.captiontext", "#101010");
pref("ui.graytext", "#b1a598");
pref("ui.highlight", "#fad184");
pref("ui.highlighttext", "#1a1a1a");
pref("ui.infobackground", "#f5f5b5");
pref("ui.infotext", "#000");
pref("ui.menu", "#f7f5f3");
pref("ui.menutext", "#101010");
pref("ui.threeddarkshadow", "#000");
pref("ui.threedface", "#ece7e2");
pref("ui.threedhighlight", "#fff");
pref("ui.threedlightshadow", "#ece7e2");
pref("ui.threedshadow", "#aea194");
pref("ui.window", "#efebe7");
pref("ui.windowtext", "#101010");
pref("ui.windowframe", "#efebe7");

#ifdef MOZ_OFFICIAL_BRANDING
pref("browser.search.param.yahoo-fr", "moz35");
pref("browser.search.param.yahoo-fr-cjkt", "moz35");
pref("browser.search.param.yahoo-fr-ja", "mozff");
#endif


pref("app.update.timer", 60000); 

#ifdef MOZ_UPDATER
pref("app.update.enabled", true);
pref("app.update.timerFirstInterval", 20000); 
pref("app.update.auto", false);
pref("app.update.channel", "@MOZ_UPDATE_CHANNEL@");
pref("app.update.mode", 1);
pref("app.update.silent", false);
pref("app.update.url", "https://aus2.mozilla.org/update/4/%PRODUCT%/%VERSION%/%BUILD_ID%/%BUILD_TARGET%/%LOCALE%/%CHANNEL%/%OS_VERSION%/%DISTRIBUTION%/%DISTRIBUTION_VERSION%/%PLATFORM_VERSION%/update.xml");
pref("app.update.nagTimer.restart", 86400);
pref("app.update.promptWaitTime", 43200);
pref("app.update.idletime", 60);
pref("app.update.showInstalledUI", false);
pref("app.update.incompatible.mode", 0);
pref("app.update.download.backgroundInterval", 0);

#ifdef MOZ_OFFICIAL_BRANDING
pref("app.update.interval", 86400);
pref("app.update.url.manual", "http://www.mozilla.com/%LOCALE%/m/");
pref("app.update.url.details", "http://www.mozilla.com/%LOCALE%/mobile/releases/");
#else
pref("app.update.interval", 28800);
#endif
#endif


pref("editor.singleLine.pasteNewlines", 2);

#ifdef MOZ_PLATFORM_MAEMO

pref("font.default.x-baltic", "SwissA");
pref("font.default.x-central-euro", "SwissA");
pref("font.default.x-cyrillic", "SwissA");
pref("font.default.x-unicode", "SwissA");
pref("font.default.x-user-def", "SwissA");
pref("font.default.x-western", "SwissA");
#endif


pref("network.buffer.cache.count", 24);
pref("network.buffer.cache.size",  16384);


pref("services.sync.client.type", "mobile");
pref("services.sync.registerEngines", "Tab,Bookmarks,Form,History,Password");
pref("services.sync.autoconnectDelay", 5);


pref("ui.dragThresholdX", 25);
pref("ui.dragThresholdY", 25);


pref("layers.accelerate-all", false);
