



#filter substitution

pref("toolkit.defaultChromeURI", "chrome://browser/content/shell.html");
pref("browser.chromeURL", "chrome://browser/content/");


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
pref("canvas.image.cache.limit", 10485760); 


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
pref("network.http.keep-alive.timeout", 600);
pref("network.http.max-connections", 20);
pref("network.http.max-persistent-connections-per-server", 6);
pref("network.http.max-persistent-connections-per-proxy", 20);


pref("network.http.spdy.push-allowance", 32768);


pref("network.buffer.cache.count", 24);
pref("network.buffer.cache.size",  16384);


pref("browser.sessionhistory.max_total_viewers", 1);
pref("browser.sessionhistory.max_entries", 50);


pref("browser.sessionstore.resume_session_once", false);
pref("browser.sessionstore.resume_from_crash", true);
pref("browser.sessionstore.resume_from_crash_timeout", 60); 
pref("browser.sessionstore.interval", 10000); 
pref("browser.sessionstore.max_tabs_undo", 1);


pref("mozilla.widget.force-24bpp", true);
pref("mozilla.widget.use-buffer-pixmap", true);
pref("mozilla.widget.disable-native-theme", true);
pref("layout.reflow.synthMouseMove", false);
pref("layers.force-tiles", false);


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


pref("browser.formfill.enable", true);


pref("layout.spellcheckDefault", 0);


pref("dom.disable_open_during_load", true);
pref("privacy.popups.showBrowserMessage", true);

pref("keyword.enabled", true);

pref("accessibility.typeaheadfind", false);
pref("accessibility.typeaheadfind.timeout", 5000);
pref("accessibility.typeaheadfind.flashBar", 1);
pref("accessibility.typeaheadfind.linksonly", false);
pref("accessibility.typeaheadfind.casesensitive", 0);


pref("browser.search.defaultenginename", "chrome://browser/locale/region.properties");


pref("browser.ssl_override_behavior", 2);
pref("browser.xul.error_pages.expert_bad_cert", false);


pref("browser.search.log", false);


pref("browser.search.update", false);
pref("browser.search.update.log", false);
pref("browser.search.updateinterval", 6);


pref("browser.search.suggest.enabled", true);


pref("browser.search.noCurrentEngine", true);


pref("chrome.override_package.global", "b2g-l10n");
pref("chrome.override_package.mozapps", "b2g-l10n");
pref("chrome.override_package.passwordmgr", "b2g-l10n");


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


pref("browser.geolocation.warning.infoURL", "http://www.mozilla.com/%LOCALE%/firefox/geolocation/");


pref("geo.wifi.uri", "https://maps.googleapis.com/maps/api/browserlocation/json");


pref("geo.enabled", true);



pref("content.sink.enable_perf_mode",  2); 
pref("content.sink.pending_event_mode", 0);
pref("content.sink.perf_deflect_count", 1000000);
pref("content.sink.perf_parse_time", 50000000);



pref("dom.use_watchdog", false);



pref("dom.max_script_run_time", 0);
pref("dom.max_chrome_script_run_time", 0);


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


pref("editor.singleLine.pasteNewlines", 2);



pref("ui.dragThresholdX", 25);
pref("ui.dragThresholdY", 25);



#ifndef MOZ_WIDGET_GONK
pref("dom.ipc.tabs.disabled", true);
pref("layers.offmainthreadcomposition.enabled", false);
pref("layers.offmainthreadcomposition.async-animations", false);
pref("layers.async-video.enabled", false);
#else
pref("dom.ipc.tabs.disabled", false);
pref("layers.offmainthreadcomposition.enabled", true);
pref("layers.acceleration.disabled", false);
pref("layers.offmainthreadcomposition.async-animations", true);
pref("layers.async-video.enabled", true);
pref("layers.async-pan-zoom.enabled", true);
pref("gfx.content.azure.enabled", true);
pref("gfx.content.azure.backends", "cairo");
#endif


pref("notification.feature.enabled", true);


pref("dom.indexedDB.warningQuota", 5);


pref("media.preload.default", 1); 
pref("media.preload.auto", 2);    
pref("media.cache_size", 4096);    



pref("media.video-queue.default-size", 3);


pref("image.mem.decodeondraw", true);
pref("image.mem.allow_locking_in_content_processes", false); 
pref("image.mem.min_discard_timeout_ms", 86400000); 
pref("image.mem.max_decoded_image_kb", 30000); 
pref("image.onload.decode.limit", 24); 



#ifdef MOZ_WIDGET_GONK

pref("dom.w3c_touch_events.enabled", 1);
pref("dom.w3c_touch_events.safetyX", 0); 
pref("dom.w3c_touch_events.safetyY", 120); 
#endif

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


pref("browser.safebrowsing.warning.infoURL", "http://www.mozilla.com/%LOCALE%/%APP%/phishing-protection/");
pref("browser.geolocation.warning.infoURL", "http://www.mozilla.com/%LOCALE%/%APP%/geolocation/");



pref("urlclassifier.alternate_error_page", "blocked");


pref("urlclassifier.gethashnoise", 4);




pref("urlclassifier.max-complete-age", 2700);


pref("browser.safebrowsing.malware.reportURL", "http://safebrowsing.clients.google.com/safebrowsing/diagnostic?client=%NAME%&hl=%LOCALE%&site=");
#endif


pref("browser.firstrun.show.uidiscovery", true);
pref("browser.firstrun.show.localepicker", true);


pref("content.ime.strict_policy", true);









pref("browser.dom.window.dump.enabled", false);


pref("security.apps.privileged.CSP.default", "default-src *; script-src 'self'; object-src 'none'; style-src 'self' 'unsafe-inline'");
pref("security.apps.certified.CSP.default", "default-src *; script-src 'self'; object-src 'none'; style-src 'self'");




pref("layers.acceleration.force-enabled", true);



pref("browser.link.open_newwindow", 3);




pref("browser.link.open_newwindow.restriction", 0);



pref("dom.mozBrowserFramesEnabled", true);



pref("dom.ipc.processCount", 100000);

pref("dom.ipc.browser_frames.oop_by_default", false);


pref("dom.sms.enabled", true);
pref("dom.sms.strict7BitEncoding", false); 
pref("dom.sms.requestStatusReport", true); 
pref("dom.mms.requestStatusReport", true); 


pref("network.gonk.ms-release-mms-connection", 30000);


pref("dom.mozContacts.enabled", true);
pref("dom.navigator-property.disable.mozContacts", false);
pref("dom.global-constructor.disable.mozContact", false);



pref("dom.phonenumber.substringmatching.BR", 8);
pref("dom.phonenumber.substringmatching.CO", 10);
pref("dom.phonenumber.substringmatching.VE", 7);


pref("dom.mozAlarms.enabled", true);


pref("services.push.enabled", true);

pref("services.push.debug", false);


pref("services.push.connection.enabled", true);

pref("services.push.serverURL", "wss://push.services.mozilla.com/");
pref("services.push.userAgentID", "");


pref("services.push.retryBaseInterval", 5000);



pref("services.push.pingInterval", 1800000); 

pref("services.push.requestTimeout", 10000);

pref("services.push.udp.wakeupEnabled", true);


#ifdef MOZ_B2G_RIL
pref("dom.mozNetworkStats.enabled", true);
pref("ril.cellbroadcast.disabled", false);
pref("dom.webapps.firstRunWithSIM", true);
#endif


pref("dom.mozSettings.enabled", true);
pref("dom.navigator-property.disable.mozSettings", false);
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


pref("shutdown.watchdog.timeoutSecs", 5);

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


pref("app.update.url", "http://update.boot2gecko.org/%CHANNEL%/update.xml");
pref("app.update.channel", "@MOZ_UPDATE_CHANNEL@");


pref("app.update.interval", 86400); 

pref("app.update.download.backgroundInterval", 0);


pref("app.update.socket.retryTimeout", 30000);




pref("app.update.socket.maxErrors", 20);



pref("app.update.log", true);
#else


pref("shutdown.watchdog.timeoutSecs", -1);
#endif


pref("webapps.update.interval", 86400);


pref("extensions.update.enabled", false);
pref("extensions.getAddons.cache.enabled", false);


pref("ui.click_hold_context_menus", true);
pref("ui.click_hold_context_menus.delay", 750);


pref("device.storage.enabled", true);


pref("dom.webapps.useCurrentProfile", true);


pref("dom.sysmsg.enabled", true);
pref("media.plugins.enabled", false);
pref("media.omx.enabled", true);


pref("dom.disable_window_print", true);


pref("dom.disable_window_showModalDialog", true);


pref("dom.experimental_forms", true);
pref("dom.forms.number", true);


pref("gfx.gralloc.enabled", false);


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


pref("ui.showHideScrollbars", 1);




pref("dom.ipc.processPriorityManager.enabled", true);
pref("dom.ipc.processPriorityManager.backgroundGracePeriodMS", 1000);
pref("dom.ipc.processPriorityManager.temporaryPriorityLockMS", 5000);




pref("dom.ipc.processPriorityManager.backgroundLRUPoolLevels", 5);
















pref("hal.processPriorityManager.gonk.MASTER.OomScoreAdjust", 0);
pref("hal.processPriorityManager.gonk.MASTER.KillUnderMB", 4);
pref("hal.processPriorityManager.gonk.MASTER.Nice", 0);

pref("hal.processPriorityManager.gonk.FOREGROUND_HIGH.OomScoreAdjust", 67);
pref("hal.processPriorityManager.gonk.FOREGROUND_HIGH.KillUnderMB", 5);
pref("hal.processPriorityManager.gonk.FOREGROUND_HIGH.Nice", 0);

pref("hal.processPriorityManager.gonk.FOREGROUND.OomScoreAdjust", 134);
pref("hal.processPriorityManager.gonk.FOREGROUND.KillUnderMB", 6);
pref("hal.processPriorityManager.gonk.FOREGROUND.Nice", 1);

pref("hal.processPriorityManager.gonk.FOREGROUND_KEYBOARD.OomScoreAdjust", 200);
pref("hal.processPriorityManager.gonk.FOREGROUND_KEYBOARD.Nice", 1);

pref("hal.processPriorityManager.gonk.BACKGROUND_PERCEIVABLE.OomScoreAdjust", 400);
pref("hal.processPriorityManager.gonk.BACKGROUND_PERCEIVABLE.KillUnderMB", 7);
pref("hal.processPriorityManager.gonk.BACKGROUND_PERCEIVABLE.Nice", 7);

pref("hal.processPriorityManager.gonk.BACKGROUND_HOMESCREEN.OomScoreAdjust", 534);
pref("hal.processPriorityManager.gonk.BACKGROUND_HOMESCREEN.KillUnderMB", 8);
pref("hal.processPriorityManager.gonk.BACKGROUND_HOMESCREEN.Nice", 18);

pref("hal.processPriorityManager.gonk.BACKGROUND.OomScoreAdjust", 667);
pref("hal.processPriorityManager.gonk.BACKGROUND.KillUnderMB", 20);
pref("hal.processPriorityManager.gonk.BACKGROUND.Nice", 18);


pref("hal.processPriorityManager.gonk.LowCPUNice", 18);








pref("hal.processPriorityManager.gonk.notifyLowMemUnderMB", 14);




pref("gonk.systemMemoryPressureRecoveryPollMS", 5000);

#ifndef DEBUG


pref("dom.ipc.processPrelaunch.enabled", true);

pref("dom.ipc.processPrelaunch.delayMs", 5000);
#endif




pref("dom.ipc.systemMessageCPULockTimeoutSec", 30);


pref("dom.disable_window_open_dialog_feature", true);


pref("accessibility.accessfu.activate", 2);
pref("accessibility.accessfu.quicknav_modes", "Link,Heading,FormElement,Landmark,ListItem");

pref("accessibility.accessfu.utterance", 1);

pref("accessibility.accessfu.skip_empty_images", true);


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

pref("jsloader.reuseGlobal", true);


pref("font.size.inflation.minTwips", 120);

pref("font.size.inflation.disabledInMasterProcess", true);



pref("memory.free_dirty_pages", true);


pref("wap.UAProf.url", "");
pref("wap.UAProf.tagname", "x-wap-profile");

pref("layout.imagevisibility.enabled", false);
pref("layout.imagevisibility.numscrollportwidths", 1);
pref("layout.imagevisibility.numscrollportheights", 1);


pref("dom.identity.enabled", true);


pref("layers.orientation.sync.timeout", 1000);



pref("webgl.can-lose-context-in-foreground", false);



pref("memory_info_dumper.watch_fifo.enabled", true);
pref("memory_info_dumper.watch_fifo.directory", "/data/local");

pref("general.useragent.enable_overrides", true);

pref("general.useragent.updates.enabled", true);
pref("general.useragent.updates.url", "");
pref("general.useragent.updates.interval", 604800); 
pref("general.useragent.updates.retry", 86400); 


pref("media.useAudioChannelService", true);

pref("b2g.version", @MOZ_B2G_VERSION@);


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


pref("dom.promise.enabled", false);


pref("dom.datastore.enabled", false);


#ifdef MOZ_WIDGET_GONK

pref("dom.inter-app-communication-api.enabled", true);
#endif




pref("b2g.adb.timeout-hours", 12);


pref("dom.mozInputMethod.enabled", true);



pref("devtools.debugger.unix-domain-socket", "/data/local/debugger-socket");



pref("gfx.canvas.azure.backends", "skia");
pref("gfx.canvas.azure.accelerated", true);


pref("dom.telephony.enabled", true);


pref("b2g.neterror.url", "app://system.gaiamobile.org/net_error.html");


pref("media.webspeech.synth.enabled", true);
