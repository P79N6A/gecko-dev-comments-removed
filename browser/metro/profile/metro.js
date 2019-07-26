



#filter substitution

#ifdef DEBUG

pref("nglayout.debug.disable_xul_cache", true);
pref("nglayout.debug.disable_xul_fastload", true);
pref("devtools.errorconsole.enabled", true);
#endif


#ifdef RELEASE_BUILD
pref("app.crashreporter.autosubmit", false);
#else

pref("app.crashreporter.autosubmit", true);
#endif

pref("app.crashreporter.prompted", false);


pref("metro.debug.treatmouseastouch", false);
pref("metro.debug.colorizeInputOverlay", false);
pref("metro.debug.selection.displayRanges", false);
pref("metro.debug.selection.dumpRanges", false);
pref("metro.debug.selection.dumpEvents", false);


pref("prompts.tab_modal.enabled", true);



pref("layers.offmainthreadcomposition.enabled", true);
pref("layers.async-pan-zoom.enabled", true);
pref("layers.componentalpha.enabled", false);
pref("gfx.azpc.touch_start_tolerance", "0.1"); 
pref("gfx.azpc.pan_repaint_interval", "50");   
pref("gfx.azpc.fling_repaint_interval", "50"); 
pref("gfx.axis.fling_friction", "0.002");
pref("gfx.axis.fling_stopped_threshold", "0.2");


pref("intl.enable_tsf_support", true);

pref("general.autoScroll", true);
pref("general.smoothScroll", true);
pref("general.smoothScroll.durationToIntervalRatio", 200);
pref("mousewheel.enable_pixel_scrolling", true);













pref("toolkit.browser.cacheRatioWidth", 2000);
pref("toolkit.browser.cacheRatioHeight", 3000);



pref("toolkit.browser.contentViewExpire", 3000);


pref("toolkit.defaultChromeURI", "chrome://browser/content/browser.xul");
pref("browser.chromeURL", "chrome://browser/content/");

pref("browser.tabs.remote", false);


pref("toolkit.telemetry.enabled", true);
pref("toolkit.telemetry.prompted", 2);

pref("toolkit.screen.lock", false);


pref("zoom.minPercent", 20);
pref("zoom.maxPercent", 400);
pref("toolkit.zoomManager.zoomValues", ".2,.3,.5,.67,.8,.9,1,1.1,1.2,1.33,1.5,1.7,2,2.4,3,4");


pref("browser.viewport.scaleRatio", -1);


pref("ui.click_hold_context_menus", false);


pref("browser.offline-apps.notify", true);


pref("network.protocol-handler.warn-external.tel", false);
pref("network.protocol-handler.warn-external.mailto", false);
pref("network.protocol-handler.warn-external.vnd.youtube", false);
pref("network.protocol-handler.warn-external.ms-windows-store", false);
pref("network.protocol-handler.external.ms-windows-store", true);


pref("browser.display.overlaynavbuttons", true);


pref("browser.display.history.maxresults", 100);


pref("browser.display.startUI.maxresults", 16);





pref("browser.backspace_action", 0);


pref("browser.sessionhistory.max_entries", 50);


pref("browser.startup.sessionRestore", false);


pref("browser.sessionstore.resume_from_crash", true);
pref("browser.sessionstore.resume_session_once", false);
pref("browser.sessionstore.resume_from_crash_timeout", 60); 

pref("browser.sessionstore.interval", 15000); 


pref("browser.sessionstore.postdata", 0);


pref("browser.sessionstore.privacy_level", 0);

pref("browser.sessionstore.privacy_level_deferred", 1);

pref("browser.sessionstore.max_tabs_undo", 10);


pref("browser.sessionstore.max_resumed_crashes", 1);





pref("browser.sessionstore.restore_on_demand", true);


pref("mozilla.widget.force-24bpp", true);
pref("mozilla.widget.use-buffer-pixmap", true);
pref("mozilla.widget.disable-native-theme", false);
pref("layout.reflow.synthMouseMove", false);


pref("layout.frame_rate.precise", true);


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
pref("browser.download.manager.addToRecentDocs", true);
pref("browser.download.manager.displayedHistoryDays", 7);
pref("browser.download.manager.resumeOnWakeDelay", 10000);
pref("browser.download.manager.quitBehavior", 0);


pref("alerts.totalOpenTime", 6000);


pref("browser.helperApps.deleteTempFileOnExit", false);


pref("signon.rememberSignons", true);
pref("signon.SignonFileName", "signons.txt");


pref("findhelper.autozoom", true);






pref("layout.spellcheckDefault", 1);



pref("extensions.enabledScopes", 1);

pref("extensions.autoDisableScopes", 1);

pref("xpinstall.enabled", false);
pref("xpinstall.whitelist.add", "addons.mozilla.org");
pref("extensions.autoupdate.enabled", false);
pref("extensions.update.enabled", false);


pref("extensions.blocklist.enabled", true);
pref("extensions.blocklist.interval", 86400);
pref("extensions.blocklist.url", "https://addons.mozilla.org/blocklist/3/%APP_ID%/%APP_VERSION%/%PRODUCT%/%BUILD_ID%/%BUILD_TARGET%/%LOCALE%/%CHANNEL%/%OS_VERSION%/%DISTRIBUTION%/%DISTRIBUTION_VERSION%/%PING_COUNT%/%TOTAL_PING_COUNT%/%DAYS_SINCE_LAST_PING%/");
pref("extensions.blocklist.detailsURL", "https://www.mozilla.org/%LOCALE%/blocklist/");


pref("dom.disable_open_during_load", true);
pref("privacy.popups.showBrowserMessage", true);


pref("privacy.donottrackheader.value", -1);


pref("dom.disable_window_open_dialog_feature", true);

pref("keyword.enabled", true);

pref("accessibility.typeaheadfind", false);
pref("accessibility.typeaheadfind.timeout", 5000);
pref("accessibility.typeaheadfind.flashBar", 1);
pref("accessibility.typeaheadfind.linksonly", false);
pref("accessibility.typeaheadfind.casesensitive", 0);


pref("accessibility.browsewithcaret_shortcut.enabled", true);
pref("accessibility.browsewithcaret", false);



pref("app.update.showInstalledUI", false);


pref("browser.search.defaultenginename", "chrome://browser/locale/region.properties");


pref("browser.ssl_override_behavior", 2);
pref("browser.xul.error_pages.expert_bad_cert", false);


pref("browser.search.log", false);


pref("browser.search.order.1", "chrome://browser/locale/region.properties");
pref("browser.search.order.2", "chrome://browser/locale/region.properties");
pref("browser.search.order.3", "chrome://browser/locale/region.properties");


pref("browser.search.update", true);


pref("browser.search.update.log", false);


pref("browser.search.update.interval", 21600);


pref("browser.search.suggest.enabled", true);


pref("browser.search.noCurrentEngine", true);

#ifdef MOZ_OFFICIAL_BRANDING

pref("browser.search.official", true);
#endif


pref("browser.xul.error_pages.enabled", true);


pref("browser.urlbar.default.behavior", 0);
pref("browser.urlbar.default.behavior.emptyRestriction", 0);



pref("places.favicons.optimizeToDimension", 25);



pref("browser.urlbar.trimURLs", true);
pref("browser.urlbar.formatting.enabled", true);
pref("browser.urlbar.clickSelectsAll", true);
pref("browser.urlbar.doubleClickSelectsAll", true);
pref("browser.urlbar.autoFill", false);
pref("browser.urlbar.matchOnlyTyped", false);
pref("browser.urlbar.matchBehavior", 1);
pref("browser.urlbar.filter.javascript", true);
pref("browser.urlbar.maxRichResults", 8);
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
pref("privacy.item.syncAccount", true);

pref("plugins.force.wmode", "opaque");







pref("privacy.sanitize.timeSpan", 1);
pref("privacy.sanitize.sanitizeOnShutdown", false);
pref("privacy.sanitize.migrateFx3Prefs",    false);


pref("geo.enabled", true);
pref("geo.wifi.uri", "https://www.googleapis.com/geolocation/v1/geolocate?key=%GOOGLE_API_KEY%");


pref("devtools.errorconsole.enabled", false);


pref("browser.ui.snapped.maxWidth", 600);


pref("browser.ui.kinetic.updateInterval", 16);
pref("browser.ui.kinetic.exponentialC", 1400);
pref("browser.ui.kinetic.polynomialC", 100);
pref("browser.ui.kinetic.swipeLength", 160);
pref("browser.ui.zoom.animationDuration", 200); 

pref("ui.mouse.radius.enabled", true);
pref("ui.touch.radius.enabled", true);


pref("plugin.disable", true);
pref("dom.ipc.plugins.enabled", true);



pref("dom.ipc.content.nice", 1);



pref("breakpad.reportURL", "https://crash-stats.mozilla.com/report/index/");

pref("app.sync.tutorialURL", "https://support.mozilla.org/kb/sync-firefox-between-desktop-and-mobile");
pref("app.support.baseURL", "https://support.mozilla.org/1/firefox/%VERSION%/%OS%/%LOCALE%/");
pref("app.privacyURL", "http://www.mozilla.org/%LOCALE%/legal/privacy/firefox.html");
pref("app.creditsURL", "http://www.mozilla.org/credits/");
pref("app.channelURL", "http://www.mozilla.org/%LOCALE%/firefox/channel/");


pref("security.alternate_certificate_error_page", "certerror");

pref("security.warn_viewing_mixed", false); 





#ifdef MOZ_UPDATER


pref("app.update.enabled", true);




pref("app.update.auto", true);


pref("app.update.mode", 0);



pref("app.update.metro.enabled", true);


pref("app.update.silent", true);



pref("app.update.staging.enabled", true);


#ifdef NIGHTLY_BUILD
pref("app.update.url", "https://aus4.mozilla.org/update/3/%PRODUCT%/%VERSION%/%BUILD_ID%/%BUILD_TARGET%/%LOCALE%/%CHANNEL%/%OS_VERSION%/%DISTRIBUTION%/%DISTRIBUTION_VERSION%/update.xml");
#else
pref("app.update.url", "https://aus3.mozilla.org/update/3/%PRODUCT%/%VERSION%/%BUILD_ID%/%BUILD_TARGET%/%LOCALE%/%CHANNEL%/%OS_VERSION%/%DISTRIBUTION%/%DISTRIBUTION_VERSION%/update.xml");
#endif


pref("app.update.idletime", 60);






pref("app.update.showInstalledUI", false);






pref("app.update.incompatible.mode", 0);


#ifdef MOZ_MAINTENANCE_SERVICE
pref("app.update.service.enabled", true);
#endif



pref("app.update.timerMinimumDelay", 120);


pref("app.update.log", false);




pref("app.update.backgroundMaxErrors", 10);





pref("app.update.cert.requireBuiltIn", true);





pref("app.update.cert.checkAttributes", true);




pref("app.update.cert.maxErrors", 5);


















#ifdef NIGHTLY_BUILD
pref("app.update.certs.1.issuerName", "CN=DigiCert Secure Server CA,O=DigiCert Inc,C=US");
pref("app.update.certs.1.commonName", "aus4.mozilla.org");
pref("app.update.certs.2.issuerName", "CN=Thawte SSL CA,O=\"Thawte, Inc.\",C=US");
pref("app.update.certs.2.commonName", "aus4.mozilla.org");
#else
pref("app.update.certs.1.issuerName", "OU=Equifax Secure Certificate Authority,O=Equifax,C=US");
pref("app.update.certs.1.commonName", "aus3.mozilla.org");
pref("app.update.certs.2.issuerName", "CN=Thawte SSL CA,O=\"Thawte, Inc.\",C=US");
pref("app.update.certs.2.commonName", "aus3.mozilla.org");
#endif





pref("editor.singleLine.pasteNewlines", 2);

#ifdef MOZ_SERVICES_SYNC

pref("services.sync.registerEngines", "Tab,Bookmarks,Form,History,Password,Prefs");


pref("services.sync.prefs.sync.browser.startup.sessionRestore", true);
pref("services.sync.prefs.sync.browser.tabs.warnOnClose", true);
pref("services.sync.prefs.sync.devtools.errorconsole.enabled", true);
pref("services.sync.prefs.sync.lightweightThemes.isThemeSelected", true);
pref("services.sync.prefs.sync.lightweightThemes.usedThemes", true);
pref("services.sync.prefs.sync.privacy.donottrackheader.enabled", true);
pref("services.sync.prefs.sync.privacy.donottrackheader.value", true);
pref("services.sync.prefs.sync.signon.rememberSignons", true);
#endif



pref("ui.dragThresholdX", 50);
pref("ui.dragThresholdY", 50);


pref("browser.chrome.toolbar_tips", false);




pref("pdfjs.disabled", true);


pref("pdfjs.firstRun", false);


pref("pdfjs.previousHandler.preferredAction", 0);
pref("pdfjs.previousHandler.alwaysAskBeforeHandling", false);




pref("image.mem.max_decoded_image_kb", 256000);


pref("dom.w3c_touch_events.enabled", 1);
pref("dom.w3c_touch_events.safetyX", 5); 
pref("dom.w3c_touch_events.safetyY", 20); 

#ifdef MOZ_SAFE_BROWSING

pref("browser.safebrowsing.enabled", true);


pref("browser.safebrowsing.malware.enabled", true);


pref("browser.safebrowsing.provider.0.updateURL", "http://safebrowsing.clients.google.com/safebrowsing/downloads?client={moz:client}&appver={moz:version}&pver=2.2");

pref("browser.safebrowsing.dataProvider", 0);


pref("browser.safebrowsing.provider.0.name", "Google");
pref("browser.safebrowsing.provider.0.keyURL", "https://sb-ssl.google.com/safebrowsing/newkey?client={moz:client}&appver={moz:version}&pver=2.2");
pref("browser.safebrowsing.provider.0.reportURL", "http://safebrowsing.clients.google.com/safebrowsing/report?");
pref("browser.safebrowsing.provider.0.gethashURL", "http://safebrowsing.clients.google.com/safebrowsing/gethash?client={moz:client}&appver={moz:version}&pver=2.2");


pref("browser.safebrowsing.provider.0.reportGenericURL", "http://{moz:locale}.phish-generic.mozilla.com/?hl={moz:locale}");
pref("browser.safebrowsing.provider.0.reportErrorURL", "http://{moz:locale}.phish-error.mozilla.com/?hl={moz:locale}");
pref("browser.safebrowsing.provider.0.reportPhishURL", "http://{moz:locale}.phish-report.mozilla.com/?hl={moz:locale}");
pref("browser.safebrowsing.provider.0.reportMalwareURL", "http://{moz:locale}.malware-report.mozilla.com/?hl={moz:locale}");
pref("browser.safebrowsing.provider.0.reportMalwareErrorURL", "http://{moz:locale}.malware-error.mozilla.com/?hl={moz:locale}");


pref("browser.safebrowsing.warning.infoURL", "https://www.mozilla.org/%LOCALE%/firefox/phishing-protection/");
pref("browser.geolocation.warning.infoURL", "https://www.mozilla.org/%LOCALE%/firefox/geolocation/");



pref("urlclassifier.alternate_error_page", "blocked");


pref("urlclassifier.gethashnoise", 4);




pref("urlclassifier.max-complete-age", 2700);


pref("urlclassifier.updatecachemax", 41943040);


pref("browser.safebrowsing.malware.reportURL", "http://safebrowsing.clients.google.com/safebrowsing/diagnostic?client=%NAME%&hl=%LOCALE%&site=");
#endif


pref("browser.firstrun.show.localepicker", false);









pref("javascript.options.showInConsole", true);
pref("browser.dom.window.dump.enabled", true);


pref("device.camera.enabled", true);
pref("media.realtime_decoder.enabled", true);


pref("network.manage-offline-status", true);


pref("full-screen-api.enabled", true);


pref("full-screen-api.approval-required", false);


pref("full-screen-api.content-only", true);





pref("full-screen-api.ignore-widgets", true);




pref("layout.imagevisibility.enabled", true);
pref("layout.imagevisibility.numscrollportwidths", 1);
pref("layout.imagevisibility.numscrollportheights", 1);
