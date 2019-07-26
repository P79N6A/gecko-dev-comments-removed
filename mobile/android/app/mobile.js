



#filter substitution













pref("toolkit.browser.cacheRatioWidth", 2000);
pref("toolkit.browser.cacheRatioHeight", 3000);



pref("toolkit.browser.contentViewExpire", 3000);

pref("toolkit.defaultChromeURI", "chrome://browser/content/browser.xul");
pref("browser.chromeURL", "chrome://browser/content/");

pref("browser.tabs.remote", false);




pref("browser.tabs.expireTime", 3600);


pref("zoom.minPercent", 20);
pref("zoom.maxPercent", 400);
pref("toolkit.zoomManager.zoomValues", ".2,.3,.5,.67,.8,.9,1,1.1,1.2,1.33,1.5,1.7,2,2.4,3,4");


pref("toolkit.storage.synchronous", 0);


pref("browser.viewport.scaleRatio", -1);
pref("browser.viewport.desktopWidth", 980);


pref("browser.viewport.defaultZoom", -1);


pref("ui.scrollbarsCanOverlapContent", 1);


pref("browser.cache.disk.enable", true);
pref("browser.cache.disk.capacity", 20480); 
pref("browser.cache.disk.max_entry_size", 4096); 
pref("browser.cache.disk.smart_size.enabled", true);
pref("browser.cache.disk.smart_size.first_run", true);

#ifdef MOZ_PKG_SPECIAL

pref("browser.cache.memory.enable", false);
#else
pref("browser.cache.memory.enable", true);
#endif
pref("browser.cache.memory.capacity", 1024); 


pref("image.cache.size", 1048576); 
pref("image.high_quality_downscaling.enabled", false);


pref("browser.offline-apps.notify", true);
pref("browser.cache.offline.enable", true);
pref("browser.cache.offline.capacity", 5120); 
pref("offline-apps.quota.warn", 1024); 


pref("browser.cache.compression_level", 0);


pref("network.protocol-handler.warn-external.tel", false);
pref("network.protocol-handler.warn-external.sms", false);
pref("network.protocol-handler.warn-external.mailto", false);
pref("network.protocol-handler.warn-external.vnd.youtube", false);


pref("network.http.pipelining", true);
pref("network.http.pipelining.ssl", true);
pref("network.http.proxy.pipelining", true);
pref("network.http.pipelining.maxrequests" , 6);
pref("network.http.keep-alive.timeout", 600);
pref("network.http.max-connections", 20);
pref("network.http.max-persistent-connections-per-server", 6);
pref("network.http.max-persistent-connections-per-proxy", 20);


pref("network.buffer.cache.count", 24);
pref("network.buffer.cache.size",  16384);


pref("browser.display.history.maxresults", 100);


pref("browser.display.remotetabs.timeout", 10);


pref("browser.sessionhistory.max_total_viewers", 1);
pref("browser.sessionhistory.max_entries", 50);


pref("browser.sessionstore.resume_session_once", false);
pref("browser.sessionstore.resume_from_crash", true);
pref("browser.sessionstore.interval", 10000); 
pref("browser.sessionstore.max_tabs_undo", 1);
pref("browser.sessionstore.max_resumed_crashes", 1);
pref("browser.sessionstore.recent_crashes", 0);


pref("mozilla.widget.force-24bpp", true);
pref("mozilla.widget.use-buffer-pixmap", true);
pref("mozilla.widget.disable-native-theme", true);
pref("layout.reflow.synthMouseMove", false);
pref("layout.css.report_errors", false);


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


pref("browser.helperApps.deleteTempFileOnExit", false);


pref("signon.rememberSignons", true);
pref("signon.expireMasterPassword", false);
pref("signon.SignonFileName", "signons.txt");
pref("signon.debug", false);



pref("formhelper.mode", 2);
pref("formhelper.autozoom", true);
pref("formhelper.autozoom.caret", true);
pref("formhelper.restore", false);


pref("findhelper.autozoom", true);


pref("browser.formfill.enable", true);


pref("layout.spellcheckDefault", 0);


pref("dom.experimental_forms", true);


pref("xpinstall.whitelist.add", "addons.mozilla.org");
pref("xpinstall.whitelist.add.180", "marketplace.firefox.com");

pref("extensions.enabledScopes", 1);
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
pref("extensions.strictCompatibility", false);
pref("extensions.minCompatibleAppVersion", "11.0");

pref("extensions.update.url", "https://versioncheck.addons.mozilla.org/update/VersionCheck.php?reqVersion=%REQ_VERSION%&id=%ITEM_ID%&version=%ITEM_VERSION%&maxAppVersion=%ITEM_MAXAPPVERSION%&status=%ITEM_STATUS%&appID=%APP_ID%&appVersion=%APP_VERSION%&appOS=%APP_OS%&appABI=%APP_ABI%&locale=%APP_LOCALE%&currentAppVersion=%CURRENT_APP_VERSION%&updateType=%UPDATE_TYPE%&compatMode=%COMPATIBILITY_MODE%");
pref("extensions.update.background.url", "https://versioncheck-bg.addons.mozilla.org/update/VersionCheck.php?reqVersion=%REQ_VERSION%&id=%ITEM_ID%&version=%ITEM_VERSION%&maxAppVersion=%ITEM_MAXAPPVERSION%&status=%ITEM_STATUS%&appID=%APP_ID%&appVersion=%APP_VERSION%&appOS=%APP_OS%&appABI=%APP_ABI%&locale=%APP_LOCALE%&currentAppVersion=%CURRENT_APP_VERSION%&updateType=%UPDATE_TYPE%&compatMode=%COMPATIBILITY_MODE%");


pref("extensions.getAddons.cache.enabled", true);
pref("extensions.getAddons.maxResults", 15);
pref("extensions.getAddons.recommended.browseURL", "https://addons.mozilla.org/%LOCALE%/android/recommended/");
pref("extensions.getAddons.recommended.url", "https://services.addons.mozilla.org/%LOCALE%/android/api/%API_VERSION%/list/featured/all/%MAX_RESULTS%/%OS%/%VERSION%");
pref("extensions.getAddons.search.browseURL", "https://addons.mozilla.org/%LOCALE%/android/search?q=%TERMS%&platform=%OS%&appver=%VERSION%");
pref("extensions.getAddons.search.url", "https://services.addons.mozilla.org/%LOCALE%/android/api/%API_VERSION%/search/%TERMS%/all/%MAX_RESULTS%/%OS%/%VERSION%/%COMPATIBILITY_MODE%");
pref("extensions.getAddons.browseAddons", "https://addons.mozilla.org/%LOCALE%/android/");
pref("extensions.getAddons.get.url", "https://services.addons.mozilla.org/%LOCALE%/android/api/%API_VERSION%/search/guid:%IDS%?src=mobile&appOS=%OS%&appVersion=%VERSION%");
pref("extensions.getAddons.getWithPerformance.url", "https://services.addons.mozilla.org/%LOCALE%/android/api/%API_VERSION%/search/guid:%IDS%?src=mobile&appOS=%OS%&appVersion=%VERSION%&tMain=%TIME_MAIN%&tFirstPaint=%TIME_FIRST_PAINT%&tSessionRestored=%TIME_SESSION_RESTORED%");


pref("extensions.getLocales.get.url", "");
pref("extensions.compatability.locales.buildid", "0");


pref("extensions.blocklist.enabled", true);
pref("extensions.blocklist.interval", 86400);
pref("extensions.blocklist.url", "https://addons.mozilla.org/blocklist/3/%APP_ID%/%APP_VERSION%/%PRODUCT%/%BUILD_ID%/%BUILD_TARGET%/%LOCALE%/%CHANNEL%/%OS_VERSION%/%DISTRIBUTION%/%DISTRIBUTION_VERSION%/%PING_COUNT%/%TOTAL_PING_COUNT%/%DAYS_SINCE_LAST_PING%/");
pref("extensions.blocklist.detailsURL", "https://www.mozilla.com/%LOCALE%/blocklist/");


pref("dom.disable_open_during_load", true);
pref("privacy.popups.showBrowserMessage", true);


pref("dom.disable_window_open_dialog_feature", true);
pref("dom.disable_window_showModalDialog", true);
pref("dom.disable_window_print", true);
pref("dom.disable_window_find", true);

pref("keyword.enabled", true);
pref("keyword.URL", "");

pref("accessibility.typeaheadfind", false);
pref("accessibility.typeaheadfind.timeout", 5000);
pref("accessibility.typeaheadfind.flashBar", 1);
pref("accessibility.typeaheadfind.linksonly", false);
pref("accessibility.typeaheadfind.casesensitive", 0);

pref("accessibility.browsewithcaret_shortcut.enabled", false);



pref("browser.menu.showCharacterEncoding", "chrome://browser/locale/browser.properties");
pref("intl.charsetmenu.browser.static", "chrome://browser/locale/browser.properties");


pref("browser.search.defaultenginename", "chrome://browser/locale/region.properties");

pref("browser.ssl_override_behavior", 2);
pref("browser.xul.error_pages.expert_bad_cert", false);


pref("browser.search.log", false);


pref("browser.search.order.1", "chrome://browser/locale/region.properties");
pref("browser.search.order.2", "chrome://browser/locale/region.properties");


pref("browser.search.update", false);
pref("browser.search.update.log", false);
pref("browser.search.updateinterval", 6);


pref("browser.search.suggest.enabled", false);
pref("browser.search.suggest.prompted", false);


pref("browser.search.loadFromJars", true);
pref("browser.search.jarURIs", "chrome://browser/locale/searchplugins/");


pref("browser.search.noCurrentEngine", true);

#ifdef MOZ_OFFICIAL_BRANDING

pref("browser.search.official", true);
#endif


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
pref("browser.urlbar.autocomplete.search_threshold", 5);
pref("browser.history.grouping", "day");
pref("browser.history.showSessions", false);
pref("browser.sessionhistory.max_entries", 50);
pref("browser.history_expire_days", 180);
pref("browser.history_expire_days_min", 90);
pref("browser.history_expire_sites", 40000);
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


pref("gfx.displayport.strategy", 1);




pref("gfx.displayport.strategy_fm.multiplier", -1); 
pref("gfx.displayport.strategy_fm.danger_x", -1); 
pref("gfx.displayport.strategy_fm.danger_y", -1); 


pref("gfx.displayport.strategy_vb.multiplier", -1); 
pref("gfx.displayport.strategy_vb.threshold", -1); 
pref("gfx.displayport.strategy_vb.reverse_buffer", -1); 
pref("gfx.displayport.strategy_vb.danger_x_base", -1); 
pref("gfx.displayport.strategy_vb.danger_y_base", -1); 
pref("gfx.displayport.strategy_vb.danger_x_incr", -1); 
pref("gfx.displayport.strategy_vb.danger_y_incr", -1); 


pref("gfx.displayport.strategy_pb.threshold", -1); 




pref("gfx.font_rendering.graphite.enabled", false);


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
pref("privacy.item.syncAccount", true);


pref("geo.enabled", true);









pref("javascript.options.methodjit.chrome",  false);



pref("javascript.options.gc_on_memory_pressure", false);

#ifdef MOZ_PKG_SPECIAL

pref("javascript.options.mem.gc_high_frequency_heap_growth_max", 120);
pref("javascript.options.mem.gc_high_frequency_heap_growth_min", 101);
pref("javascript.options.mem.gc_high_frequency_high_limit_mb", 40);
pref("javascript.options.mem.gc_high_frequency_low_limit_mb", 10);
pref("javascript.options.mem.gc_low_frequency_heap_growth", 105);
pref("javascript.options.mem.high_water_mark", 16);
pref("javascript.options.mem.gc_allocation_threshold_mb", 3);
#else
pref("javascript.options.mem.high_water_mark", 32);
#endif

pref("dom.max_chrome_script_run_time", 0); 
pref("dom.max_script_run_time", 20);


pref("devtools.errorconsole.enabled", false);

pref("font.size.inflation.minTwips", 120);


pref("browser.ui.zoom.force-user-scalable", false);



pref("browser.ui.touch.left", 32);
pref("browser.ui.touch.right", 32);
pref("browser.ui.touch.top", 48);
pref("browser.ui.touch.bottom", 16);
pref("browser.ui.touch.weight.visited", 120); 


pref("plugin.disable", false);
pref("dom.ipc.plugins.enabled", false);

pref("plugins.click_to_play", true);



pref("breakpad.reportURL", "https://crash-stats.mozilla.com/report/index/");
pref("app.support.baseURL", "http://support.mozilla.org/1/mobile/%VERSION%/%OS%/%LOCALE%/");

pref("app.feedback.postURL", "http://m.input.mozilla.org/%LOCALE%/feedback");
pref("app.privacyURL", "http://www.mozilla.com/%LOCALE%/m/privacy.html");
pref("app.creditsURL", "http://www.mozilla.org/credits/");
pref("app.channelURL", "http://www.mozilla.org/%LOCALE%/firefox/channel/");
#if MOZ_UPDATE_CHANNEL == aurora
pref("app.releaseNotesURL", "http://www.mozilla.com/%LOCALE%/mobile/%VERSION%/auroranotes/");
#else
pref("app.releaseNotesURL", "http://www.mozilla.com/%LOCALE%/mobile/%VERSION%/releasenotes/");
#endif
#if MOZ_UPDATE_CHANNEL == beta
pref("app.faqURL", "http://www.mozilla.com/%LOCALE%/mobile/beta/faq/");
#else
pref("app.faqURL", "http://www.mozilla.com/%LOCALE%/mobile/faq/");
#endif
pref("app.marketplaceURL", "https://marketplace.mozilla.org/");


pref("security.alternate_certificate_error_page", "certerror");

pref("security.warn_viewing_mixed", false); 


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


pref("app.update.timerFirstInterval", 30000); 
pref("app.update.timerMinimumDelay", 30); 



pref("app.update.autodownload", "wifi");

#ifdef MOZ_UPDATER

pref("app.update.enabled", false);
pref("app.update.channel", "@MOZ_UPDATE_CHANNEL@");



#endif


pref("editor.singleLine.pasteNewlines", 2);



pref("ui.dragThresholdX", 25);
pref("ui.dragThresholdY", 25);

pref("layers.acceleration.disabled", false);
pref("layers.offmainthreadcomposition.enabled", true);
pref("layers.async-video.enabled", true);
pref("layers.progressive-paint", true);
pref("layers.low-precision-buffer", true);
pref("layers.low-precision-resolution", 250);

pref("notification.feature.enabled", true);
pref("dom.webnotifications.enabled", true);


pref("browser.chrome.toolbar_tips", false);
pref("indexedDB.feature.enabled", true);
pref("dom.indexedDB.warningQuota", 5);


pref("media.preload.default", 1); 
pref("media.preload.auto", 2);    


pref("image.mem.decodeondraw", true);
pref("content.image.allow_locking", false);
pref("image.mem.min_discard_timeout_ms", 10000);


pref("dom.w3c_touch_events.enabled", 1);

#ifdef MOZ_SAFE_BROWSING
pref("browser.safebrowsing.enabled", true);
pref("browser.safebrowsing.malware.enabled", true);
pref("browser.safebrowsing.debug", false);

pref("browser.safebrowsing.updateURL", "http://safebrowsing.clients.google.com/safebrowsing/downloads?client=SAFEBROWSING_ID&appver=%VERSION%&pver=2.2");
pref("browser.safebrowsing.keyURL", "https://sb-ssl.google.com/safebrowsing/newkey?client=SAFEBROWSING_ID&appver=%VERSION%&pver=2.2");
pref("browser.safebrowsing.gethashURL", "http://safebrowsing.clients.google.com/safebrowsing/gethash?client=SAFEBROWSING_ID&appver=%VERSION%&pver=2.2");
pref("browser.safebrowsing.reportURL", "http://safebrowsing.clients.google.com/safebrowsing/report?");
pref("browser.safebrowsing.reportGenericURL", "http://%LOCALE%.phish-generic.mozilla.com/?hl=%LOCALE%");
pref("browser.safebrowsing.reportErrorURL", "http://%LOCALE%.phish-error.mozilla.com/?hl=%LOCALE%");
pref("browser.safebrowsing.reportPhishURL", "http://%LOCALE%.phish-report.mozilla.com/?hl=%LOCALE%");
pref("browser.safebrowsing.reportMalwareURL", "http://%LOCALE%.malware-report.mozilla.com/?hl=%LOCALE%");
pref("browser.safebrowsing.reportMalwareErrorURL", "http://%LOCALE%.malware-error.mozilla.com/?hl=%LOCALE%");

pref("browser.safebrowsing.warning.infoURL", "http://www.mozilla.com/%LOCALE%/firefox/phishing-protection/");
pref("browser.safebrowsing.malware.reportURL", "http://safebrowsing.clients.google.com/safebrowsing/diagnostic?client=%NAME%&hl=%LOCALE%&site=");

pref("browser.safebrowsing.id", @MOZ_APP_UA_NAME@);



pref("urlclassifier.alternate_error_page", "blocked");


pref("urlclassifier.gethashnoise", 4);


pref("urlclassifier.gethashtables", "goog-phish-shavar,goog-malware-shavar");




pref("urlclassifier.max-complete-age", 2700);
#endif


pref("browser.firstrun.show.uidiscovery", true);
pref("browser.firstrun.show.localepicker", false);









pref("browser.dom.window.dump.enabled", true);


pref("device.camera.enabled", true);
pref("media.realtime_decoder.enabled", true);

pref("dom.report_all_js_exceptions", true);
pref("javascript.options.showInConsole", true);

pref("full-screen-api.enabled", true);

pref("direct-texture.force.enabled", false);
pref("direct-texture.force.disabled", false);


pref("ui.scrolling.friction_slow", -1);

pref("ui.scrolling.friction_fast", -1);


pref("ui.scrolling.max_event_acceleration", -1);

pref("ui.scrolling.overscroll_decel_rate", -1);

pref("ui.scrolling.overscroll_snap_limit", -1);


pref("ui.scrolling.min_scrollable_distance", -1);


pref("accessibility.accessfu.activate", 2);
pref("accessibility.accessfu.quicknav_modes", "Link,Heading,FormElement,ListItem");


pref("network.manage-offline-status", true);


pref("dom.min_background_timeout_value", 900000);


pref("reader.font_size", 4);


pref("reader.margin_size", 5);


pref("reader.color_scheme", "light");


pref("reader.has_used_toolbar", false);


pref("media.plugins.enabled", true);





pref("media.stagefright.omxcodec.flags", 0);


pref("dom.event.touch.coalescing.enabled", false);



pref("app.orientation.default", "");



pref("memory.free_dirty_pages", true);

pref("layout.imagevisibility.enabled", false);


pref("browser.chrome.dynamictoolbar", true);

#ifdef MOZ_PKG_SPECIAL


pref("webgl.disabled", true);
#endif

#ifndef RELEASE_BUILD

pref("media.webaudio.enabled", true);
#endif
