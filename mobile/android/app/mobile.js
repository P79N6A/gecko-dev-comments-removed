



#filter substitution













pref("toolkit.browser.cacheRatioWidth", 2000);
pref("toolkit.browser.cacheRatioHeight", 3000);



pref("toolkit.browser.contentViewExpire", 3000);

pref("toolkit.defaultChromeURI", "chrome://browser/content/browser.xul");
pref("browser.chromeURL", "chrome://browser/content/");




pref("browser.tabs.expireTime", 900);


pref("zoom.minPercent", 20);
pref("zoom.maxPercent", 400);
pref("toolkit.zoomManager.zoomValues", ".2,.3,.5,.67,.8,.9,1,1.1,1.2,1.33,1.5,1.7,2,2.4,3,4");


pref("toolkit.storage.synchronous", 0);

pref("browser.viewport.desktopWidth", 980);


pref("browser.viewport.defaultZoom", -1);


pref("ui.scrollbarsCanOverlapContent", 1);


pref("ui.caretBlinkCount", 10);


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

pref("browser.cache.memory_limit", 5120); 


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
pref("network.http.keep-alive.timeout", 109);
pref("network.http.max-connections", 20);
pref("network.http.max-persistent-connections-per-server", 6);
pref("network.http.max-persistent-connections-per-proxy", 20);


pref("network.http.spdy.push-allowance", 32768);


pref("network.buffer.cache.count", 24);
pref("network.buffer.cache.size",  16384);


pref("network.predictor.enabled", true);
pref("network.predictor.max-db-size", 2097152); 
pref("network.predictor.preserve", 50); 


pref("browser.display.history.maxresults", 100);


pref("browser.display.remotetabs.timeout", 10);


pref("browser.sessionhistory.max_total_viewers", 1);
pref("browser.sessionhistory.max_entries", 50);
pref("browser.sessionhistory.contentViewerTimeout", 360);


pref("browser.sessionstore.resume_session_once", false);
pref("browser.sessionstore.resume_from_crash", true);
pref("browser.sessionstore.interval", 10000); 
pref("browser.sessionstore.max_tabs_undo", 5);
pref("browser.sessionstore.max_resumed_crashes", 1);
pref("browser.sessionstore.recent_crashes", 0);
pref("browser.sessionstore.privacy_level", 0); 


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
pref("browser.download.manager.addToRecentDocs", true);


pref("browser.helperApps.deleteTempFileOnExit", false);


pref("signon.rememberSignons", true);
pref("signon.expireMasterPassword", false);
pref("signon.debug", false);


pref("formhelper.mode", 2);  
pref("formhelper.autozoom", true);


pref("findhelper.autozoom", true);


pref("browser.formfill.enable", true);


pref("layout.spellcheckDefault", 0);


pref("dom.experimental_forms", true);
pref("dom.forms.number", true);


pref("xpinstall.whitelist.directRequest", false);
pref("xpinstall.whitelist.fileRequest", false);
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

pref("extensions.hotfix.id", "firefox-android-hotfix@mozilla.org");
pref("extensions.hotfix.cert.checkAttributes", true);
pref("extensions.hotfix.certs.1.sha1Fingerprint", "91:53:98:0C:C1:86:DF:47:8F:35:22:9E:11:C9:A7:31:04:49:A1:AA");


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
pref("extensions.blocklist.url", "https://blocklist.addons.mozilla.org/blocklist/3/%APP_ID%/%APP_VERSION%/%PRODUCT%/%BUILD_ID%/%BUILD_TARGET%/%LOCALE%/%CHANNEL%/%OS_VERSION%/%DISTRIBUTION%/%DISTRIBUTION_VERSION%/%PING_COUNT%/%TOTAL_PING_COUNT%/%DAYS_SINCE_LAST_PING%/");
pref("extensions.blocklist.detailsURL", "https://www.mozilla.com/%LOCALE%/blocklist/");


pref("dom.disable_open_during_load", true);
pref("privacy.popups.showBrowserMessage", true);


pref("dom.disable_window_open_dialog_feature", true);
pref("dom.disable_window_showModalDialog", true);
pref("dom.disable_window_print", true);
pref("dom.disable_window_find", true);

pref("keyword.enabled", true);
pref("browser.fixup.domainwhitelist.localhost", true);

pref("accessibility.typeaheadfind", false);
pref("accessibility.typeaheadfind.timeout", 5000);
pref("accessibility.typeaheadfind.flashBar", 1);
pref("accessibility.typeaheadfind.linksonly", false);
pref("accessibility.typeaheadfind.casesensitive", 0);
pref("accessibility.browsewithcaret_shortcut.enabled", false);



pref("browser.menu.showCharacterEncoding", "chrome://browser/locale/browser.properties");


pref("browser.search.defaultenginename", "chrome://browser/locale/region.properties");

pref("browser.search.param.maxSuggestions", "4");

pref("browser.ssl_override_behavior", 2);
pref("browser.xul.error_pages.expert_bad_cert", false);


pref("browser.search.order.1", "chrome://browser/locale/region.properties");
pref("browser.search.order.2", "chrome://browser/locale/region.properties");
pref("browser.search.order.3", "chrome://browser/locale/region.properties");


pref("browser.search.geoSpecificDefaults", true);
pref("browser.search.defaultenginename.US", "chrome://browser/locale/region.properties");
pref("browser.search.order.US.1", "chrome://browser/locale/region.properties");
pref("browser.search.order.US.2", "chrome://browser/locale/region.properties");
pref("browser.search.order.US.3", "chrome://browser/locale/region.properties");


pref("browser.search.update", false);


pref("browser.search.suggest.enabled", false);
pref("browser.search.suggest.prompted", false);


pref("browser.search.loadFromJars", true);
pref("browser.search.jarURIs", "chrome://browser/locale/searchplugins/");


pref("browser.search.noCurrentEngine", true);


pref("browser.casting.enabled", true);
#ifdef RELEASE_BUILD

pref("browser.mirroring.enabled.roku", false);

pref("browser.mirroring.enabled", false);
#else
pref("browser.mirroring.enabled.roku", true);
pref("browser.mirroring.enabled", true);
#endif


pref("chrome.override_package.global", "browser");
pref("chrome.override_package.mozapps", "browser");
pref("chrome.override_package.passwordmgr", "browser");


pref("browser.xul.error_pages.enabled", true);


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


pref("gfx.android.rgb16.force", false);


pref("dom.disable_window_move_resize", true);


pref("browser.enable_click_image_resizing", false);



pref("browser.link.open_external", 3);
pref("browser.link.open_newwindow", 3);

pref("browser.link.open_newwindow.restriction", 0);


pref("privacy.item.cache", true);
pref("privacy.item.cookies", true);
pref("privacy.item.offlineApps", true);
pref("privacy.item.history", true);
pref("privacy.item.searchHistory", true);
pref("privacy.item.formdata", true);
pref("privacy.item.downloads", true);
pref("privacy.item.passwords", true);
pref("privacy.item.sessions", true);
pref("privacy.item.geolocation", true);
pref("privacy.item.siteSettings", true);
pref("privacy.item.syncAccount", true);


pref("geo.enabled", true);










pref("javascript.options.gc_on_memory_pressure", false);

#ifdef MOZ_PKG_SPECIAL

pref("javascript.options.mem.gc_high_frequency_heap_growth_max", 120);
pref("javascript.options.mem.gc_high_frequency_heap_growth_min", 120);
pref("javascript.options.mem.gc_high_frequency_high_limit_mb", 40);
pref("javascript.options.mem.gc_high_frequency_low_limit_mb", 10);
pref("javascript.options.mem.gc_low_frequency_heap_growth", 120);
pref("javascript.options.mem.high_water_mark", 16);
pref("javascript.options.mem.gc_allocation_threshold_mb", 3);
pref("javascript.options.mem.gc_decommit_threshold_mb", 1);
pref("javascript.options.mem.gc_min_empty_chunk_count", 1);
pref("javascript.options.mem.gc_max_empty_chunk_count", 2);
#else
pref("javascript.options.mem.high_water_mark", 32);
#endif

pref("dom.max_chrome_script_run_time", 0); 
pref("dom.max_script_run_time", 20);


pref("devtools.errorconsole.enabled", false);


pref("devtools.debugger.unix-domain-socket", "/data/data/@ANDROID_PACKAGE_NAME@/firefox-debugger-socket");

pref("font.size.inflation.minTwips", 0);


pref("browser.ui.zoom.force-user-scalable", false);

pref("ui.zoomedview.enabled", false);
pref("ui.zoomedview.limitReadableSize", 8);  

pref("ui.touch.radius.enabled", false);
pref("ui.touch.radius.leftmm", 3);
pref("ui.touch.radius.topmm", 5);
pref("ui.touch.radius.rightmm", 3);
pref("ui.touch.radius.bottommm", 2);
pref("ui.touch.radius.visitedWeight", 120);

pref("ui.mouse.radius.enabled", true);
pref("ui.mouse.radius.leftmm", 3);
pref("ui.mouse.radius.topmm", 5);
pref("ui.mouse.radius.rightmm", 3);
pref("ui.mouse.radius.bottommm", 2);
pref("ui.mouse.radius.visitedWeight", 120);
pref("ui.mouse.radius.reposition", true);


pref("browser.ui.show-margins-threshold", 10);



pref("browser.ui.selection.distance", 250);


pref("plugin.disable", false);
pref("dom.ipc.plugins.enabled", false);



pref("plugins.click_to_play", true);

pref("plugin.default.state", 1);



pref("breakpad.reportURL", "https://crash-stats.mozilla.com/report/index/");
pref("app.support.baseURL", "http://support.mozilla.org/1/mobile/%VERSION%/%OS%/%LOCALE%/");

pref("app.feedback.postURL", "https://input.mozilla.org/api/v1/feedback/");
pref("app.privacyURL", "https://www.mozilla.org/privacy/firefox/");
pref("app.creditsURL", "http://www.mozilla.org/credits/");
pref("app.channelURL", "http://www.mozilla.org/%LOCALE%/firefox/channel/");
#if MOZ_UPDATE_CHANNEL == aurora
pref("app.releaseNotesURL", "http://www.mozilla.com/%LOCALE%/mobile/%VERSION%/auroranotes/");
#elif MOZ_UPDATE_CHANNEL == beta
pref("app.releaseNotesURL", "http://www.mozilla.com/%LOCALE%/mobile/%VERSION%beta/releasenotes/");
#else
pref("app.releaseNotesURL", "http://www.mozilla.com/%LOCALE%/mobile/%VERSION%/releasenotes/");
#endif
#if MOZ_UPDATE_CHANNEL == beta
pref("app.faqURL", "http://www.mozilla.com/%LOCALE%/mobile/beta/faq/");
#else
pref("app.faqURL", "http://www.mozilla.com/%LOCALE%/mobile/faq/");
#endif
pref("app.marketplaceURL", "https://marketplace.firefox.com/");


pref("security.alternate_certificate_error_page", "certerror");

pref("security.warn_viewing_mixed", false); 


pref("security.mixed_content.block_active_content", true);


pref("security.cert_pinning.enforcement_level", 1);


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


pref("app.update.timerFirstInterval", 30000); 
pref("app.update.timerMinimumDelay", 30); 



pref("app.update.autodownload", "wifi");
pref("app.update.url.android", "https://aus4.mozilla.org/update/4/%PRODUCT%/%VERSION%/%BUILD_ID%/%BUILD_TARGET%/%LOCALE%/%CHANNEL%/%OS_VERSION%/%DISTRIBUTION%/%DISTRIBUTION_VERSION%/%MOZ_VERSION%/update.xml");

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
#ifdef MOZ_ANDROID_APZ
pref("layers.async-pan-zoom.enabled", true);
#endif
pref("layers.progressive-paint", true);
pref("layers.low-precision-buffer", true);
pref("layers.low-precision-resolution", "0.25");
pref("layers.low-precision-opacity", "1.0");





pref("layers.max-active", 20);

pref("notification.feature.enabled", true);
pref("dom.webnotifications.enabled", true);


pref("browser.chrome.toolbar_tips", false);


pref("media.preload.default", 1); 
pref("media.preload.auto", 2);    
pref("media.cache_size", 32768);    


pref("media.cache_resume_threshold", 10);
pref("media.cache_readahead_limit", 30);







pref("media.video-queue.default-size", 3);


pref("media.fragmented-mp4.exposed", true);
pref("media.fragmented-mp4.enabled", true);
pref("media.fragmented-mp4.android-media-codec.enabled", true);
pref("media.fragmented-mp4.android-media-codec.preferred", true);


pref("image.decode-only-on-draw.enabled", true);

#ifdef NIGHTLY_BUILD

pref("shumway.disabled", true);
#endif


pref("dom.w3c_touch_events.enabled", 1);

#ifdef MOZ_SAFE_BROWSING
pref("browser.safebrowsing.enabled", true);
pref("browser.safebrowsing.malware.enabled", true);
pref("browser.safebrowsing.debug", false);

pref("browser.safebrowsing.updateURL", "https://safebrowsing.google.com/safebrowsing/downloads?client=SAFEBROWSING_ID&appver=%VERSION%&pver=2.2&key=%GOOGLE_API_KEY%");
pref("browser.safebrowsing.gethashURL", "https://safebrowsing.google.com/safebrowsing/gethash?client=SAFEBROWSING_ID&appver=%VERSION%&pver=2.2");
pref("browser.safebrowsing.reportURL", "https://safebrowsing.google.com/safebrowsing/report?");
pref("browser.safebrowsing.reportGenericURL", "http://%LOCALE%.phish-generic.mozilla.com/?hl=%LOCALE%");
pref("browser.safebrowsing.reportErrorURL", "http://%LOCALE%.phish-error.mozilla.com/?hl=%LOCALE%");
pref("browser.safebrowsing.reportPhishURL", "http://%LOCALE%.phish-report.mozilla.com/?hl=%LOCALE%");
pref("browser.safebrowsing.reportMalwareURL", "http://%LOCALE%.malware-report.mozilla.com/?hl=%LOCALE%");
pref("browser.safebrowsing.reportMalwareErrorURL", "http://%LOCALE%.malware-error.mozilla.com/?hl=%LOCALE%");

pref("browser.safebrowsing.malware.reportURL", "https://safebrowsing.google.com/safebrowsing/diagnostic?client=%NAME%&hl=%LOCALE%&site=");

pref("browser.safebrowsing.id", @MOZ_APP_UA_NAME@);



pref("urlclassifier.alternate_error_page", "blocked");


pref("urlclassifier.gethashnoise", 4);


pref("urlclassifier.gethash.timeout_ms", 5000);




pref("urlclassifier.max-complete-age", 2700);
#endif


#ifdef RELEASE_BUILD
pref("browser.tiles.reportURL", "https://tiles.services.mozilla.com/v2/links/click");
#endif


pref("browser.firstrun.show.uidiscovery", true);
pref("browser.firstrun.show.localepicker", false);









pref("browser.dom.window.dump.enabled", true);


pref("services.push.enabled", false);


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

pref("ui.scrolling.axis_lock_mode", "standard");

pref("ui.scrolling.negate_wheel_scrollY", true);


pref("ui.scrolling.gamepad_dead_zone", 115);


pref("ui.scrolling.fling_accel_interval", -1);
pref("ui.scrolling.fling_accel_base_multiplier", -1);
pref("ui.scrolling.fling_accel_supplemental_multiplier", -1);


pref("ui.scrolling.fling_curve_function_x1", -1);
pref("ui.scrolling.fling_curve_function_y1", -1);
pref("ui.scrolling.fling_curve_function_x2", -1);
pref("ui.scrolling.fling_curve_function_y2", -1);
pref("ui.scrolling.fling_curve_threshold_velocity", -1);
pref("ui.scrolling.fling_curve_max_velocity", -1);
pref("ui.scrolling.fling_curve_newton_iterations", -1);


pref("accessibility.accessfu.activate", 2);
pref("accessibility.accessfu.quicknav_modes", "Link,Heading,FormElement,Landmark,ListItem");

pref("accessibility.accessfu.quicknav_index", 0);

pref("accessibility.accessfu.utterance", 1);

pref("accessibility.accessfu.skip_empty_images", true);



pref("network.tickle-wifi.enabled", true);


pref("network.manage-offline-status", true);


pref("dom.min_background_timeout_value", 900000);


pref("media.plugins.enabled", true);





pref("media.stagefright.omxcodec.flags", 0);


pref("dom.event.touch.coalescing.enabled", false);



pref("app.orientation.default", "");



pref("memory.free_dirty_pages", true);

pref("layout.imagevisibility.numscrollportwidths", 1);
pref("layout.imagevisibility.numscrollportheights", 1);

pref("layers.enable-tiles", true);


pref("browser.chrome.dynamictoolbar", true);




pref("browser.chrome.titlebarMode", 1);


pref("browser.urlbar.trimURLs", true);

#ifdef MOZ_PKG_SPECIAL


pref("webgl.disabled", true);
#endif


pref("browser.contentHandlers.types.0.title", "chrome://browser/locale/region.properties");
pref("browser.contentHandlers.types.0.uri", "chrome://browser/locale/region.properties");
pref("browser.contentHandlers.types.0.type", "application/vnd.mozilla.maybe.feed");
pref("browser.contentHandlers.types.1.title", "chrome://browser/locale/region.properties");
pref("browser.contentHandlers.types.1.uri", "chrome://browser/locale/region.properties");
pref("browser.contentHandlers.types.1.type", "application/vnd.mozilla.maybe.feed");
pref("browser.contentHandlers.types.2.title", "chrome://browser/locale/region.properties");
pref("browser.contentHandlers.types.2.uri", "chrome://browser/locale/region.properties");
pref("browser.contentHandlers.types.2.type", "application/vnd.mozilla.maybe.feed");
pref("browser.contentHandlers.types.3.title", "chrome://browser/locale/region.properties");
pref("browser.contentHandlers.types.3.uri", "chrome://browser/locale/region.properties");
pref("browser.contentHandlers.types.3.type", "application/vnd.mozilla.maybe.feed");


pref("dom.mozPay.enabled", true);

#ifndef RELEASE_BUILD
pref("dom.payment.provider.0.name", "Firefox Marketplace");
pref("dom.payment.provider.0.description", "marketplace.firefox.com");
pref("dom.payment.provider.0.uri", "https://marketplace.firefox.com/mozpay/?req=");
pref("dom.payment.provider.0.type", "mozilla/payments/pay/v1");
pref("dom.payment.provider.0.requestMethod", "GET");
#endif



pref("dom.phonenumber.substringmatching.BR", 8);
pref("dom.phonenumber.substringmatching.CO", 10);
pref("dom.phonenumber.substringmatching.VE", 7);


pref("media.useAudioChannelService", false);


pref("gfx.canvas.azure.backends", "skia");
pref("gfx.canvas.azure.accelerated", true);

pref("general.useragent.override.youtube.com", "Android; Tablet;#Android; Mobile;");


pref("browser.ui.linkify.phone", false);


pref("snav.enabled", true);




pref("browser.snippets.updateUrl", "https://snippets.mozilla.com/json/%SNIPPETS_VERSION%/%NAME%/%VERSION%/%APPBUILDID%/%BUILD_TARGET%/%LOCALE%/%CHANNEL%/%OS_VERSION%/%DISTRIBUTION%/%DISTRIBUTION_VERSION%/");


pref("browser.snippets.updateInterval", 86400);


pref("browser.snippets.geoUrl", "https://geo.mozilla.org/country.json");


pref("browser.snippets.statsUrl", "https://snippets-stats.mozilla.org/mobile");


pref("browser.snippets.enabled", true);
pref("browser.snippets.syncPromo.enabled", true);
pref("browser.snippets.firstrunHomepage.enabled", true);


pref("browser.webapps.apkFactoryUrl", "https://controller.apk.firefox.com/application.apk");


pref("browser.webapps.updateInterval", 86400);












pref("browser.webapps.checkForUpdates", 1);




pref("browser.webapps.updateCheckUrl", "https://controller.apk.firefox.com/app_updates");




pref("home.sync.updateMode", 0);


pref("home.sync.checkIntervalSecs", 3600);


pref("device.storage.enabled", true);


pref("dom.meta-viewport.enabled", true);


pref("media.gmp-provider.enabled", true);




pref("reader.color_scheme", "auto");


pref("reader.color_scheme.values", "[\"dark\",\"auto\",\"light\"]");


pref("reader.toolbar.vertical", false);


pref("browser.readinglist.enabled", true);


pref("selectioncaret.detects.longtap", false);


#ifdef NIGHTLY_BUILD
pref("dom.serviceWorkers.enabled", true);
#endif
