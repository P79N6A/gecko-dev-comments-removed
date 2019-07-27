



#filter substitution

#ifndef MOZ_MULET
pref("toolkit.defaultChromeURI", "chrome://b2g/content/shell.html");
pref("browser.chromeURL", "chrome://b2g/content/");
#endif

#ifdef MOZ_MULET




pref("browser.startup.homepage", "data:text/plain,browser.startup.homepage=chrome://b2g/content/shell.html");
pref("b2g.is_mulet", true);

pref("startup.homepage_welcome_url", "");
pref("browser.shell.checkDefaultBrowser", false);

pref("devtools.toolbox.host", "side");
pref("devtools.toolbox.sidebar.width", 800);

pref("browser.sessionstore.max_tabs_undo", 0);
pref("browser.sessionstore.max_windows_undo", 0);
pref("browser.sessionstore.restore_on_demand", false);
pref("browser.sessionstore.resume_from_crash", false);

pref("browser.tabs.remote.autostart.1", false);
#endif


pref("toolkit.defaultChromeFeatures", "chrome,dialog=no,close,resizable,scrollbars,extrachrome");


pref("browser.display.focus_ring_width", 0);


pref("browser.viewport.scaleRatio", -1);


pref("browser.ignoreNativeFrameTextSelection", true);


#ifdef MOZ_WIDGET_GONK
pref("browser.cache.disk.enable", true);
pref("browser.cache.disk.capacity", 55000); 
pref("browser.cache.disk.parent_directory", "/cache");
#endif
pref("browser.cache.disk.smart_size.enabled", false);
pref("browser.cache.disk.smart_size.first_run", false);

pref("browser.cache.memory.enable", true);
pref("browser.cache.memory.capacity", 1024); 

pref("browser.cache.memory_limit", 2048); 


pref("image.cache.size", 1048576); 
pref("image.high_quality_downscaling.enabled", false);
pref("canvas.image.cache.limit", 20971520); 


pref("browser.offline-apps.notify", false);
pref("browser.cache.offline.enable", true);
pref("offline-apps.allow_by_default", true);


pref("network.protocol-handler.warn-external.tel", false);
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



pref("network.cookie.cookieBehavior", 0);


pref("network.http.spdy.enabled.http2draft", true);
pref("network.http.spdy.push-allowance", 32768);


pref("network.buffer.cache.count", 24);
pref("network.buffer.cache.size",  16384);


pref("network.predictor.enabled", false); 
pref("network.predictor.max-db-size", 2097152); 
pref("network.predictor.preserve", 50); 


pref("browser.sessionhistory.max_total_viewers", 1);
pref("browser.sessionhistory.max_entries", 50);
pref("browser.sessionhistory.contentViewerTimeout", 360);


pref("browser.sessionstore.resume_session_once", false);
pref("browser.sessionstore.resume_from_crash", true);
pref("browser.sessionstore.resume_from_crash_timeout", 60); 
pref("browser.sessionstore.interval", 10000); 
pref("browser.sessionstore.max_tabs_undo", 1);


pref("mozilla.widget.force-24bpp", true);
pref("mozilla.widget.use-buffer-pixmap", true);
pref("mozilla.widget.disable-native-theme", true);
pref("layout.reflow.synthMouseMove", false);
pref("layers.enable-tiles", true);
pref("layers.low-precision-buffer", true);
pref("layers.low-precision-opacity", "0.5");
pref("layers.progressive-paint", true);


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


pref("browser.formfill.enable", true);


pref("layout.spellcheckDefault", 0);


pref("dom.disable_open_during_load", true);
pref("privacy.popups.showBrowserMessage", true);

pref("keyword.enabled", true);
pref("browser.fixup.domainwhitelist.localhost", true);

pref("accessibility.typeaheadfind", false);
pref("accessibility.typeaheadfind.timeout", 5000);
pref("accessibility.typeaheadfind.flashBar", 1);
pref("accessibility.typeaheadfind.linksonly", false);
pref("accessibility.typeaheadfind.casesensitive", 0);


pref("browser.ssl_override_behavior", 2);
pref("browser.xul.error_pages.expert_bad_cert", false);


pref("browser.search.update", false);


pref("browser.search.noCurrentEngine", true);


pref("browser.xul.error_pages.enabled", true);


pref("gfx.color_management.mode", 0);


pref("dom.disable_window_move_resize", true);


pref("browser.enable_click_image_resizing", false);


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


pref("geo.provider.use_mls", false);
pref("geo.cell.scan", true);
pref("geo.wifi.uri", "https://location.services.mozilla.com/v1/geolocate?key=%MOZILLA_API_KEY%");


pref("geo.enabled", true);



pref("content.sink.enable_perf_mode",  2); 
pref("content.sink.pending_event_mode", 0);
pref("content.sink.perf_deflect_count", 1000000);
pref("content.sink.perf_parse_time", 50000000);



pref("dom.use_watchdog", false);



pref("dom.max_script_run_time", 0);
pref("dom.max_chrome_script_run_time", 0);
pref("dom.max_child_script_run_time", 0);


pref("plugin.disable", true);
pref("dom.ipc.plugins.enabled", true);



pref("breakpad.reportURL", "https://crash-stats.mozilla.com/report/index/");
pref("app.releaseNotesURL", "http://www.mozilla.com/%LOCALE%/b2g/%VERSION%/releasenotes/");
pref("app.support.baseURL", "http://support.mozilla.com/b2g");
pref("app.privacyURL", "http://www.mozilla.com/%LOCALE%/m/privacy.html");
pref("app.creditsURL", "http://www.mozilla.org/credits/");
pref("app.featuresURL", "http://www.mozilla.com/%LOCALE%/b2g/features/");
pref("app.faqURL", "http://www.mozilla.com/%LOCALE%/b2g/faq/");


pref("security.alternate_certificate_error_page", "certerror");

pref("security.warn_viewing_mixed", false); 







pref("security.cert_pinning.enforcement_level", 2);



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
pref("ui.highlighttext", "#1a1a1a");
pref("ui.threeddarkshadow", "#000");
pref("ui.threedface", "#ece7e2");
pref("ui.threedhighlight", "#fff");
pref("ui.threedlightshadow", "#ece7e2");
pref("ui.threedshadow", "#aea194");
pref("ui.windowframe", "#efebe7");


pref("ui.menu", "#f97c17");
pref("ui.menutext", "#ffffff");
pref("ui.infobackground", "#343e40");
pref("ui.infotext", "#686868");
pref("ui.window", "#ffffff");
pref("ui.windowtext", "#000000");
pref("ui.highlight", "#b2f2ff");


pref("editor.singleLine.pasteNewlines", 2);



pref("ui.dragThresholdX", 25);
pref("ui.dragThresholdY", 25);



pref("layers.offmainthreadcomposition.enabled", true);
pref("layers.offmainthreadcomposition.async-animations", true);
#ifndef MOZ_WIDGET_GONK
pref("dom.ipc.tabs.disabled", true);
#else
pref("dom.ipc.tabs.disabled", false);
pref("layers.acceleration.disabled", false);
pref("layers.async-pan-zoom.enabled", true);
pref("gfx.content.azure.backends", "cairo");
#endif


pref("notification.feature.enabled", true);


pref("media.preload.default", 1); 
pref("media.preload.auto", 2);    
pref("media.cache_size", 4096);    


pref("media.cache_resume_threshold", 10);
pref("media.cache_readahead_limit", 30);

#ifdef MOZ_FMP4

pref("media.fragmented-mp4.gonk.enabled", true);
#endif


pref("media.video-queue.default-size", 3);


pref("image.decode-only-on-draw.enabled", true);
pref("image.mem.allow_locking_in_content_processes", true);




pref("image.mem.surfacecache.max_size_kb", 131072);  
pref("image.mem.surfacecache.size_factor", 8);  
pref("image.mem.surfacecache.discard_factor", 2);  
pref("image.mem.surfacecache.min_expiration_ms", 86400000); 
pref("image.onload.decode.limit", 24); 




pref("dom.w3c_touch_events.enabled", 1);
pref("dom.w3c_touch_events.safetyX", 0); 
pref("dom.w3c_touch_events.safetyY", 120); 

#ifdef MOZ_SAFE_BROWSING

pref("browser.safebrowsing.enabled", false);


pref("browser.safebrowsing.malware.enabled", false);

pref("browser.safebrowsing.debug", false);
pref("browser.safebrowsing.updateURL", "https://safebrowsing.google.com/safebrowsing/downloads?client=SAFEBROWSING_ID&appver=%VERSION%&pver=2.2&key=%GOOGLE_API_KEY%");
pref("browser.safebrowsing.gethashURL", "https://safebrowsing.google.com/safebrowsing/gethash?client=SAFEBROWSING_ID&appver=%VERSION%&pver=2.2");
pref("browser.safebrowsing.reportURL", "https://safebrowsing.google.com/safebrowsing/report?");
pref("browser.safebrowsing.reportGenericURL", "http://%LOCALE%.phish-generic.mozilla.com/?hl=%LOCALE%");
pref("browser.safebrowsing.reportErrorURL", "http://%LOCALE%.phish-error.mozilla.com/?hl=%LOCALE%");
pref("browser.safebrowsing.reportPhishURL", "http://%LOCALE%.phish-report.mozilla.com/?hl=%LOCALE%");
pref("browser.safebrowsing.reportMalwareURL", "http://%LOCALE%.malware-report.mozilla.com/?hl=%LOCALE%");
pref("browser.safebrowsing.reportMalwareErrorURL", "http://%LOCALE%.malware-error.mozilla.com/?hl=%LOCALE%");
pref("browser.safebrowsing.appRepURL", "https://sb-ssl.google.com/safebrowsing/clientreport/download?key=%GOOGLE_API_KEY%");

pref("browser.safebrowsing.id", "Firefox");


pref("urlclassifier.downloadBlockTable", "goog-badbinurl-shavar");


pref("browser.safebrowsing.provider.0.updateURL", "https://safebrowsing.google.com/safebrowsing/downloads?client={moz:client}&appver={moz:version}&pver=2.2&key=%GOOGLE_API_KEY%");

pref("browser.safebrowsing.dataProvider", 0);


pref("browser.safebrowsing.provider.0.name", "Google");
pref("browser.safebrowsing.provider.0.reportURL", "https://safebrowsing.google.com/safebrowsing/report?");
pref("browser.safebrowsing.provider.0.gethashURL", "https://safebrowsing.google.com/safebrowsing/gethash?client={moz:client}&appver={moz:version}&pver=2.2");


pref("browser.safebrowsing.provider.0.reportGenericURL", "http://{moz:locale}.phish-generic.mozilla.com/?hl={moz:locale}");
pref("browser.safebrowsing.provider.0.reportErrorURL", "http://{moz:locale}.phish-error.mozilla.com/?hl={moz:locale}");
pref("browser.safebrowsing.provider.0.reportPhishURL", "http://{moz:locale}.phish-report.mozilla.com/?hl={moz:locale}");
pref("browser.safebrowsing.provider.0.reportMalwareURL", "http://{moz:locale}.malware-report.mozilla.com/?hl={moz:locale}");
pref("browser.safebrowsing.provider.0.reportMalwareErrorURL", "http://{moz:locale}.malware-error.mozilla.com/?hl={moz:locale}");




pref("urlclassifier.gethashnoise", 4);


pref("urlclassifier.gethash.timeout_ms", 5000);




pref("urlclassifier.max-complete-age", 2700);


pref("browser.safebrowsing.malware.reportURL", "https://safebrowsing.google.com/safebrowsing/diagnostic?client=%NAME%&hl=%LOCALE%&site=");
#endif


pref("browser.firstrun.show.uidiscovery", true);
pref("browser.firstrun.show.localepicker", true);


pref("content.ime.strict_policy", true);









pref("browser.dom.window.dump.enabled", false);



pref("security.apps.certified.CSP.default", "default-src *; script-src 'self'; object-src 'none'; style-src 'self' 'unsafe-inline' app://theme.gaiamobile.org");

pref("security.apps.trusted.CSP.default", "default-src *; object-src 'none'; frame-src 'none'");




pref("layers.acceleration.force-enabled", true);



pref("browser.link.open_newwindow", 3);




pref("browser.link.open_newwindow.restriction", 0);



pref("dom.mozBrowserFramesEnabled", true);



pref("dom.ipc.processCount", 100000);

pref("dom.ipc.browser_frames.oop_by_default", false);

pref("dom.meta-viewport.enabled", true);


pref("dom.sms.enabled", true);


pref("network.gonk.ms-release-mms-connection", 30000);


pref("dom.mozContacts.enabled", true);



pref("dom.phonenumber.substringmatching.BR", 8);
pref("dom.phonenumber.substringmatching.CO", 10);
pref("dom.phonenumber.substringmatching.VE", 7);
pref("dom.phonenumber.substringmatching.CL", 8);
pref("dom.phonenumber.substringmatching.PE", 7);


pref("dom.mozAlarms.enabled", true);


pref("services.push.enabled", true);

pref("services.push.debug", false);


pref("services.push.connection.enabled", true);

pref("services.push.serverURL", "wss://push.services.mozilla.com/");
pref("services.push.userAgentID", "");


pref("services.push.retryBaseInterval", 5000);



pref("services.push.pingInterval", 1800000); 

pref("services.push.requestTimeout", 10000);
pref("services.push.pingInterval.default", 180000);
pref("services.push.pingInterval.mobile", 180000); 
pref("services.push.pingInterval.wifi", 180000);  

pref("services.push.adaptive.enabled", true);
pref("services.push.adaptive.lastGoodPingInterval", 180000);
pref("services.push.adaptive.lastGoodPingInterval.mobile", 180000);
pref("services.push.adaptive.lastGoodPingInterval.wifi", 180000);

pref("services.push.adaptive.gap", 60000); 

pref("services.push.adaptive.upperLimit", 1740000); 

pref("services.push.udp.wakeupEnabled", true);


#ifdef MOZ_WIDGET_GONK
pref("dom.mozNetworkStats.enabled", true);
pref("dom.webapps.firstRunWithSIM", true);
#endif


#ifdef MOZ_WIDGET_GONK
pref("dom.resource_stats.enabled", true);
#endif

#ifdef MOZ_B2G_RIL

pref("dom.mozApps.single_variant_sourcedir", "/persist/svoperapps");
#endif


pref("dom.mozSettings.enabled", true);
pref("dom.mozPermissionSettings.enabled", true);


pref("device.camera.enabled", true);
pref("media.realtime_decoder.enabled", true);


pref("dom.mozTCPSocket.enabled", true);


pref("dom.mozPay.enabled", true);








pref("layout.frame_rate.precise", true);


pref("b2g.keys.menu.enabled", true);


pref("power.screen.timeout", 60);

pref("full-screen-api.enabled", true);

#ifndef MOZ_WIDGET_GONK






pref("full-screen-api.ignore-widgets", true);
#endif

pref("media.volume.steps", 10);

#ifdef ENABLE_MARIONETTE

pref("marionette.defaultPrefs.enabled", true);
pref("marionette.defaultPrefs.port", 2828);
#ifndef MOZ_WIDGET_GONK

pref("marionette.force-local", true);
#endif
#endif

#ifdef MOZ_UPDATER


pref("shutdown.watchdog.timeoutSecs", 10);

pref("b2g.update.apply-prompt-timeout", 60000); 

pref("b2g.update.apply-idle-timeout", 600000); 

pref("b2g.update.download-watchdog-timeout", 120000); 
pref("b2g.update.download-watchdog-max-retries", 5);

pref("app.update.enabled", true);
pref("app.update.auto", false);
pref("app.update.silent", false);
pref("app.update.mode", 0);
pref("app.update.incompatible.mode", 0);
pref("app.update.staging.enabled", true);
pref("app.update.service.enabled", true);

pref("app.update.url", "https://aus4.mozilla.org/update/3/%PRODUCT%/%VERSION%/%BUILD_ID%/%PRODUCT_DEVICE%/%LOCALE%/%CHANNEL%/%OS_VERSION%/%DISTRIBUTION%/%DISTRIBUTION_VERSION%/update.xml");
pref("app.update.channel", "@MOZ_UPDATE_CHANNEL@");


pref("app.update.interval", 86400); 

pref("app.update.download.backgroundInterval", 0);


pref("app.update.socket.retryTimeout", 30000);




pref("app.update.socket.maxErrors", 20);



pref("app.update.log", true);
#else


pref("shutdown.watchdog.timeoutSecs", -1);
#endif


pref("webapps.update.enabled", true);


pref("webapps.update.interval", 86400);


pref("extensions.update.enabled", false);
pref("extensions.getAddons.cache.enabled", false);


pref("ui.click_hold_context_menus", true);
pref("ui.click_hold_context_menus.delay", 400);


pref("device.storage.enabled", true);


pref("dom.webapps.useCurrentProfile", true);


pref("dom.sysmsg.enabled", true);
pref("media.plugins.enabled", false);
pref("media.omx.enabled", true);
pref("media.rtsp.enabled", true);
pref("media.rtsp.video.enabled", true);


pref("dom.disable_window_print", true);


pref("dom.disable_window_showModalDialog", true);


pref("dom.experimental_forms", true);
pref("dom.forms.number", true);



pref("dom.forms.color", false);


pref("gfx.gralloc.enabled", false);





pref("javascript.options.discardSystemSource", true);


pref("javascript.options.mem.log", false);


pref("javascript.options.mem.gc_incremental_slice_ms", 30);


pref("javascript.options.mem.gc_high_frequency_time_limit_ms", 1500);

pref("javascript.options.mem.gc_high_frequency_heap_growth_max", 300);
pref("javascript.options.mem.gc_high_frequency_heap_growth_min", 120);
pref("javascript.options.mem.gc_high_frequency_high_limit_mb", 40);
pref("javascript.options.mem.gc_high_frequency_low_limit_mb", 0);
pref("javascript.options.mem.gc_low_frequency_heap_growth", 120);
pref("javascript.options.mem.high_water_mark", 6);
pref("javascript.options.mem.gc_allocation_threshold_mb", 1);
pref("javascript.options.mem.gc_decommit_threshold_mb", 1);
pref("javascript.options.mem.gc_min_empty_chunk_count", 1);
pref("javascript.options.mem.gc_max_empty_chunk_count", 2);


pref("ui.showHideScrollbars", 1);
pref("ui.useOverlayScrollbars", 1);
pref("ui.scrollbarFadeBeginDelay", 450);
pref("ui.scrollbarFadeDuration", 200);


pref("layout.scrollbar.side", 1);


pref("layout.css.scroll-snap.enabled", true);





pref("dom.ipc.processPriorityManager.enabled", true);
pref("dom.ipc.processPriorityManager.backgroundGracePeriodMS", 1000);
pref("dom.ipc.processPriorityManager.backgroundPerceivableGracePeriodMS", 5000);
pref("dom.ipc.processPriorityManager.temporaryPriorityLockMS", 5000);




pref("dom.ipc.processPriorityManager.BACKGROUND.LRUPoolLevels", 5);
pref("dom.ipc.processPriorityManager.FOREGROUND.LRUPoolLevels", 3);








pref("hal.processPriorityManager.gonk.MASTER.OomScoreAdjust", 0);
pref("hal.processPriorityManager.gonk.MASTER.KillUnderKB", 4096);
pref("hal.processPriorityManager.gonk.MASTER.cgroup", "");

pref("hal.processPriorityManager.gonk.PREALLOC.OomScoreAdjust", 67);
pref("hal.processPriorityManager.gonk.PREALLOC.cgroup", "apps/bg_non_interactive");

pref("hal.processPriorityManager.gonk.FOREGROUND_HIGH.OomScoreAdjust", 67);
pref("hal.processPriorityManager.gonk.FOREGROUND_HIGH.KillUnderKB", 5120);
pref("hal.processPriorityManager.gonk.FOREGROUND_HIGH.cgroup", "apps/critical");

pref("hal.processPriorityManager.gonk.FOREGROUND.OomScoreAdjust", 134);
pref("hal.processPriorityManager.gonk.FOREGROUND.KillUnderKB", 6144);
pref("hal.processPriorityManager.gonk.FOREGROUND.cgroup", "apps");

pref("hal.processPriorityManager.gonk.FOREGROUND_KEYBOARD.OomScoreAdjust", 200);
pref("hal.processPriorityManager.gonk.FOREGROUND_KEYBOARD.cgroup", "apps");

pref("hal.processPriorityManager.gonk.BACKGROUND_PERCEIVABLE.OomScoreAdjust", 400);
pref("hal.processPriorityManager.gonk.BACKGROUND_PERCEIVABLE.KillUnderKB", 7168);
pref("hal.processPriorityManager.gonk.BACKGROUND_PERCEIVABLE.cgroup", "apps/bg_perceivable");

pref("hal.processPriorityManager.gonk.BACKGROUND_HOMESCREEN.OomScoreAdjust", 534);
pref("hal.processPriorityManager.gonk.BACKGROUND_HOMESCREEN.KillUnderKB", 8192);
pref("hal.processPriorityManager.gonk.BACKGROUND_HOMESCREEN.cgroup", "apps/bg_non_interactive");

pref("hal.processPriorityManager.gonk.BACKGROUND.OomScoreAdjust", 667);
pref("hal.processPriorityManager.gonk.BACKGROUND.KillUnderKB", 20480);
pref("hal.processPriorityManager.gonk.BACKGROUND.cgroup", "apps/bg_non_interactive");








pref("hal.processPriorityManager.gonk.cgroups.apps.cpu_shares", 1024);
pref("hal.processPriorityManager.gonk.cgroups.apps.cpu_notify_on_migrate", 0);
pref("hal.processPriorityManager.gonk.cgroups.apps.memory_swappiness", 10);


pref("hal.processPriorityManager.gonk.cgroups.apps/critical.cpu_shares", 16384);
pref("hal.processPriorityManager.gonk.cgroups.apps/critical.cpu_notify_on_migrate", 0);
pref("hal.processPriorityManager.gonk.cgroups.apps/critical.memory_swappiness", 0);


pref("hal.processPriorityManager.gonk.cgroups.apps/bg_perceivable.cpu_shares", 103);
pref("hal.processPriorityManager.gonk.cgroups.apps/bg_perceivable.cpu_notify_on_migrate", 0);
pref("hal.processPriorityManager.gonk.cgroups.apps/bg_perceivable.memory_swappiness", 60);


pref("hal.processPriorityManager.gonk.cgroups.apps/bg_non_interactive.cpu_shares", 52);
pref("hal.processPriorityManager.gonk.cgroups.apps/bg_non_interactive.cpu_notify_on_migrate", 0);
pref("hal.processPriorityManager.gonk.cgroups.apps/bg_non_interactive.memory_swappiness", 100);











pref("hal.gonk.COMPOSITOR.rt_priority", 0);
pref("hal.gonk.COMPOSITOR.nice", -4);








pref("hal.processPriorityManager.gonk.notifyLowMemUnderKB", 14336);




pref("gonk.systemMemoryPressureRecoveryPollMS", 5000);

#ifndef DEBUG


pref("dom.ipc.processPrelaunch.enabled", true);

pref("dom.ipc.processPrelaunch.delayMs", 5000);
#endif

pref("dom.ipc.reuse_parent_app", false);




pref("dom.ipc.systemMessageCPULockTimeoutSec", 30);


pref("dom.disable_window_open_dialog_feature", true);


pref("dom.beforeAfterKeyboardEvent.enabled", true);


pref("accessibility.accessfu.activate", 2);
pref("accessibility.accessfu.quicknav_modes", "Link,Heading,FormElement,Landmark,ListItem");

pref("accessibility.accessfu.quicknav_index", 0);

pref("accessibility.accessfu.utterance", 1);

pref("accessibility.accessfu.skip_empty_images", true);


pref("accessibility.accessfu.keyboard_echo", 3);


pref("ui.touch.radius.enabled", true);
pref("ui.touch.radius.leftmm", 3);
pref("ui.touch.radius.topmm", 5);
pref("ui.touch.radius.rightmm", 3);
pref("ui.touch.radius.bottommm", 2);

pref("ui.mouse.radius.enabled", true);
pref("ui.mouse.radius.leftmm", 3);
pref("ui.mouse.radius.topmm", 5);
pref("ui.mouse.radius.rightmm", 3);
pref("ui.mouse.radius.bottommm", 2);


pref("browser.prompt.allowNative", false);





pref("network.activity.blipIntervalMilliseconds", 250);






pref("network.gonk.manage-offline-status", true);



#ifndef MOZ_MULET

#ifndef DEBUG
pref("jsloader.reuseGlobal", true);
#endif
#endif


pref("font.size.inflation.minTwips", 120);

pref("font.size.inflation.disabledInMasterProcess", true);



pref("memory.free_dirty_pages", true);


pref("memory.system_memory_reporter", true);


pref("memory.dump_reports_on_oom", false);

pref("layout.imagevisibility.numscrollportwidths", 1);
pref("layout.imagevisibility.numscrollportheights", 1);


pref("dom.identity.enabled", true);


pref("layers.orientation.sync.timeout", 1000);


pref("b2g.orientation.animate", true);



pref("webgl.can-lose-context-in-foreground", false);



pref("memory_info_dumper.watch_fifo.enabled", true);
pref("memory_info_dumper.watch_fifo.directory", "/data/local");


pref("general.useragent.updates.enabled", true);
pref("general.useragent.updates.url", "https://dynamicua.cdn.mozilla.net/0/%APP_ID%");
pref("general.useragent.updates.interval", 604800); 
pref("general.useragent.updates.retry", 86400); 

pref("general.useragent.device_id", "");


pref("media.useAudioChannelService", true);

pref("b2g.version", @MOZ_B2G_VERSION@);
pref("b2g.osName", @MOZ_B2G_OS_NAME@);


pref("consoleservice.buffered", false);

#ifdef MOZ_WIDGET_GONK

pref("toolkit.storage.pageSize", 2048);
#endif


pref("captivedetect.canonicalURL", "http://detectportal.firefox.com/success.txt");
pref("captivedetect.canonicalContent", "success\n");


pref("ping.manifestURL", "https://marketplace.firefox.com/packaged.webapp");


pref("disk_space_watcher.enabled", true);


pref("network.sntp.maxRetryCount", 10);
pref("network.sntp.refreshPeriod", 86400); 
pref("network.sntp.pools", 
     "0.pool.ntp.org;1.pool.ntp.org;2.pool.ntp.org;3.pool.ntp.org");
pref("network.sntp.port", 123);
pref("network.sntp.timeout", 30); 


pref("dom.datastore.enabled", true);


pref("dom.datastore.sysMsgOnChangeShortTimeoutSec", 10);
pref("dom.datastore.sysMsgOnChangeLongTimeoutSec", 60);


pref("dom.inter-app-communication-api.enabled", true);




pref("b2g.adb.timeout-hours", 12);


pref("dom.mozInputMethod.enabled", true);



pref("devtools.debugger.unix-domain-socket", "/data/local/debugger-socket");



#ifdef MOZ_WIDGET_GONK
pref("gfx.canvas.azure.backends", "skia");
pref("gfx.canvas.azure.accelerated", true);
#endif


pref("gfx.canvas.skiagl.dynamic-cache", true);


pref("gfx.canvas.max-size-for-skia-gl", -1);


pref("gfx.gralloc.fence-with-readpixels", true);


pref("b2g.neterror.url", "app://system.gaiamobile.org/net_error.html");


pref("b2g.theme.origin", "app://theme.gaiamobile.org");
pref("dom.mozApps.themable", true);
pref("dom.mozApps.selected_theme", "default_theme.gaiamobile.org");


pref("network.proxy.pac_generator", true);



pref("network.proxy.browsing.app_origins", "app://system.gaiamobile.org");


pref("media.webspeech.synth.enabled", true);


pref("dom.mozDownloads.enabled", true);
pref("dom.downloads.max_retention_days", 7);








pref("security.exthelperapp.disable_background_handling", true);


pref("osfile.reset_worker_delay", 5000);





pref("apz.asyncscroll.throttle", 40);
pref("apz.pan_repaint_interval", 16);


pref("apz.fling_curve_function_x1", "0.41");
pref("apz.fling_curve_function_y1", "0.0");
pref("apz.fling_curve_function_x2", "0.80");
pref("apz.fling_curve_function_y2", "1.0");
pref("apz.fling_curve_threshold_inches_per_ms", "0.01");
pref("apz.fling_friction", "0.0019");
pref("apz.max_velocity_inches_per_ms", "0.07");
pref("apz.touch_start_tolerance", "0.1");



pref("apz.x_skate_size_multiplier", "1.25");
pref("apz.y_skate_size_multiplier", "1.5");
pref("apz.x_stationary_size_multiplier", "1.5");
pref("apz.y_stationary_size_multiplier", "1.8");
pref("apz.enlarge_displayport_when_clipped", true);

pref("apz.axis_lock.mode", 2);


pref("apz.overscroll.enabled", true);
pref("apz.overscroll.stretch_factor", "0.35");
pref("apz.overscroll.spring_stiffness", "0.0018");
pref("apz.overscroll.spring_friction", "0.015");
pref("apz.overscroll.stop_distance_threshold", "5.0");
pref("apz.overscroll.stop_velocity_threshold", "0.01");


pref("layout.event-regions.enabled", true);









pref("gfx.canvas.willReadFrequently.enable", true);



pref("browser.autofocus", false);


pref("dom.wakelock.enabled", true);


pref("dom.apps.customization.enabled", true);


pref("touchcaret.enabled", true);


pref("selectioncaret.enabled", true);


pref("services.sync.fxaccounts.enabled", true);
pref("identity.fxaccounts.enabled", true);


pref("services.mobileid.server.uri", "https://msisdn.services.mozilla.com");


#ifndef XP_WIN
pref("dom.mapped_arraybuffer.enabled", true);
#endif


pref("dom.broadcastChannel.enabled", true);


pref("dom.udpsocket.enabled", true);


pref("dom.tv.enabled", true);

pref("dom.mozSettings.SettingsDB.debug.enabled", true);
pref("dom.mozSettings.SettingsManager.debug.enabled", true);
pref("dom.mozSettings.SettingsRequestManager.debug.enabled", true);
pref("dom.mozSettings.SettingsService.debug.enabled", true);

pref("dom.mozSettings.SettingsDB.verbose.enabled", false);
pref("dom.mozSettings.SettingsManager.verbose.enabled", false);
pref("dom.mozSettings.SettingsRequestManager.verbose.enabled", false);
pref("dom.mozSettings.SettingsService.verbose.enabled", false);




pref("dom.mozSettings.allowForceReadOnly", false);


pref("dom.requestSync.enabled", true);




#if ANDROID_VERSION == 19 || ANDROID_VERSION == 21 || ANDROID_VERSION == 15
pref("gfx.vsync.hw-vsync.enabled", true);
pref("gfx.vsync.compositor", true);
pref("gfx.touch.resample", true);
#else
pref("gfx.vsync.hw-vsync.enabled", false);
pref("gfx.vsync.compositor", false);
pref("gfx.touch.resample", false);
#endif



#if ANDROID_VERSION == 19 || ANDROID_VERSION == 15
pref("gfx.vsync.refreshdriver", true);
#else
pref("gfx.vsync.refreshdriver", false);
#endif
