# -*- Mode: JavaScript; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http:



#filter substitution

#
# SYNTAX HINTS:
#
#  - Dashes are delimiters; use underscores instead.
#  - The first character after a period must be alphabetic.
#  - Computed values (e.g. 50 * 1024) don't work.
#

#ifdef XP_UNIX
#ifndef XP_MACOSX
#define UNIX_BUT_NOT_MAC
#endif
#endif

pref("browser.chromeURL","chrome://browser/content/");
pref("browser.hiddenWindowChromeURL", "chrome://browser/content/hiddenWindow.xul");


pref("extensions.logging.enabled", false);


pref("extensions.strictCompatibility", false);



pref("extensions.minCompatibleAppVersion", "4.0");


pref("extensions.getAddons.cache.enabled", true);
pref("extensions.getAddons.maxResults", 15);
pref("extensions.getAddons.get.url", "https://services.addons.mozilla.org/%LOCALE%/firefox/api/%API_VERSION%/search/guid:%IDS%?src=firefox&appOS=%OS%&appVersion=%VERSION%");
pref("extensions.getAddons.getWithPerformance.url", "https://services.addons.mozilla.org/%LOCALE%/firefox/api/%API_VERSION%/search/guid:%IDS%?src=firefox&appOS=%OS%&appVersion=%VERSION%&tMain=%TIME_MAIN%&tFirstPaint=%TIME_FIRST_PAINT%&tSessionRestored=%TIME_SESSION_RESTORED%");
pref("extensions.getAddons.search.browseURL", "https://addons.mozilla.org/%LOCALE%/firefox/search?q=%TERMS%&platform=%OS%&appver=%VERSION%");
pref("extensions.getAddons.search.url", "https://services.addons.mozilla.org/%LOCALE%/firefox/api/%API_VERSION%/search/%TERMS%/all/%MAX_RESULTS%/%OS%/%VERSION%/%COMPATIBILITY_MODE%?src=firefox");
pref("extensions.webservice.discoverURL", "https://services.addons.mozilla.org/%LOCALE%/firefox/discovery/pane/%VERSION%/%OS%/%COMPATIBILITY_MODE%");


pref("extensions.blocklist.enabled", true);
pref("extensions.blocklist.interval", 86400);


pref("extensions.blocklist.level", 2);
pref("extensions.blocklist.url", "https://addons.mozilla.org/blocklist/3/%APP_ID%/%APP_VERSION%/%PRODUCT%/%BUILD_ID%/%BUILD_TARGET%/%LOCALE%/%CHANNEL%/%OS_VERSION%/%DISTRIBUTION%/%DISTRIBUTION_VERSION%/%PING_COUNT%/%TOTAL_PING_COUNT%/%DAYS_SINCE_LAST_PING%/");
pref("extensions.blocklist.detailsURL", "https://www.mozilla.com/%LOCALE%/blocklist/");
pref("extensions.blocklist.itemURL", "https://addons.mozilla.org/%LOCALE%/%APP%/blocked/%blockID%");

pref("extensions.update.autoUpdateDefault", true);

pref("extensions.hotfix.id", "firefox-hotfix@mozilla.org");
pref("extensions.hotfix.cert.checkAttributes", true);
pref("extensions.hotfix.certs.1.sha1Fingerprint", "CA:C4:7D:BF:63:4D:24:E9:DC:93:07:2F:E3:C8:EA:6D:C3:94:6E:89");



pref("extensions.autoDisableScopes", 15);


pref("browser.dictionaries.download.url", "https://addons.mozilla.org/%LOCALE%/firefox/dictionaries/");



pref("app.update.timerMinimumDelay", 120);









pref("app.update.altwindowtype", "Browser:About");


pref("app.update.log", false);




pref("app.update.backgroundMaxErrors", 10);





pref("app.update.cert.requireBuiltIn", true);





pref("app.update.cert.checkAttributes", true);




pref("app.update.cert.maxErrors", 5);















pref("app.update.certs.1.issuerName", "OU=Equifax Secure Certificate Authority,O=Equifax,C=US");
pref("app.update.certs.1.commonName", "aus3.mozilla.org");

pref("app.update.certs.2.issuerName", "CN=Thawte SSL CA,O=\"Thawte, Inc.\",C=US");
pref("app.update.certs.2.commonName", "aus3.mozilla.org");


pref("app.update.enabled", true);




pref("app.update.auto", true);










pref("app.update.mode", 1);


pref("app.update.silent", false);



pref("app.update.staging.enabled", true);


pref("app.update.url", "https://aus3.mozilla.org/update/3/%PRODUCT%/%VERSION%/%BUILD_ID%/%BUILD_TARGET%/%LOCALE%/%CHANNEL%/%OS_VERSION%/%DISTRIBUTION%/%DISTRIBUTION_VERSION%/update.xml");










pref("app.update.idletime", 60);






pref("app.update.showInstalledUI", false);






pref("app.update.incompatible.mode", 0);


#ifdef MOZ_MAINTENANCE_SERVICE
pref("app.update.service.enabled", true);
#endif







pref("extensions.update.enabled", true);
pref("extensions.update.url", "https://versioncheck.addons.mozilla.org/update/VersionCheck.php?reqVersion=%REQ_VERSION%&id=%ITEM_ID%&version=%ITEM_VERSION%&maxAppVersion=%ITEM_MAXAPPVERSION%&status=%ITEM_STATUS%&appID=%APP_ID%&appVersion=%APP_VERSION%&appOS=%APP_OS%&appABI=%APP_ABI%&locale=%APP_LOCALE%&currentAppVersion=%CURRENT_APP_VERSION%&updateType=%UPDATE_TYPE%&compatMode=%COMPATIBILITY_MODE%");
pref("extensions.update.background.url", "https://versioncheck-bg.addons.mozilla.org/update/VersionCheck.php?reqVersion=%REQ_VERSION%&id=%ITEM_ID%&version=%ITEM_VERSION%&maxAppVersion=%ITEM_MAXAPPVERSION%&status=%ITEM_STATUS%&appID=%APP_ID%&appVersion=%APP_VERSION%&appOS=%APP_OS%&appABI=%APP_ABI%&locale=%APP_LOCALE%&currentAppVersion=%CURRENT_APP_VERSION%&updateType=%UPDATE_TYPE%&compatMode=%COMPATIBILITY_MODE%");
pref("extensions.update.interval", 86400);  
                                            

pref("extensions.dss.enabled", false);          
pref("extensions.dss.switchPending", false);    
                                                

pref("extensions.{972ce4c6-7e08-4474-a285-3208198ce6fd}.name", "chrome://browser/locale/browser.properties");
pref("extensions.{972ce4c6-7e08-4474-a285-3208198ce6fd}.description", "chrome://browser/locale/browser.properties");

pref("xpinstall.whitelist.add", "addons.mozilla.org");
pref("xpinstall.whitelist.add.36", "getpersonas.com");
pref("xpinstall.whitelist.add.180", "marketplace.firefox.com");

pref("lightweightThemes.update.enabled", true);

pref("keyword.enabled", true);


pref("keyword.URL", "");

pref("general.useragent.locale", "@AB_CD@");
pref("general.skins.selectedSkin", "classic/1.0");

pref("general.smoothScroll", true);
#ifdef UNIX_BUT_NOT_MAC
pref("general.autoScroll", false);
#else
pref("general.autoScroll", true);
#endif

pref("general.useragent.complexOverride.moodle", false); 


pref("browser.shell.checkDefaultBrowser", true);



pref("browser.startup.page",                1);
pref("browser.startup.homepage",            "chrome://branding/locale/browserconfig.properties");

pref("browser.slowStartup.notificationDisabled", false);
pref("browser.slowStartup.timeThreshold", 60000);
pref("browser.slowStartup.maxSamples", 5);




pref("browser.aboutHomeSnippets.updateUrl", "https://snippets.mozilla.com/%STARTPAGE_VERSION%/%NAME%/%VERSION%/%APPBUILDID%/%BUILD_TARGET%/%LOCALE%/%CHANNEL%/%OS_VERSION%/%DISTRIBUTION%/%DISTRIBUTION_VERSION%/");

pref("browser.enable_automatic_image_resizing", true);
pref("browser.chrome.site_icons", true);
pref("browser.chrome.favicons", true);

pref("browser.warnOnQuit", true);


pref("browser.showQuitWarning", false);
pref("browser.fullscreen.autohide", true);
pref("browser.fullscreen.animateUp", 1);
pref("browser.overlink-delay", 80);

#ifdef UNIX_BUT_NOT_MAC
pref("browser.urlbar.clickSelectsAll", false);
#else
pref("browser.urlbar.clickSelectsAll", true);
#endif
#ifdef UNIX_BUT_NOT_MAC
pref("browser.urlbar.doubleClickSelectsAll", true);
#else
pref("browser.urlbar.doubleClickSelectsAll", false);
#endif
pref("browser.urlbar.autoFill", true);
pref("browser.urlbar.autoFill.typed", true);




pref("browser.urlbar.matchBehavior", 1);
pref("browser.urlbar.filter.javascript", true);


pref("browser.urlbar.maxRichResults", 12);



pref("browser.urlbar.delay", 50);




pref("browser.urlbar.restrict.history", "^");
pref("browser.urlbar.restrict.bookmark", "*");
pref("browser.urlbar.restrict.tag", "+");
pref("browser.urlbar.restrict.openpage", "%");
pref("browser.urlbar.restrict.typed", "~");
pref("browser.urlbar.match.title", "#");
pref("browser.urlbar.match.url", "@");








pref("browser.urlbar.default.behavior", 0);

pref("browser.urlbar.formatting.enabled", true);
pref("browser.urlbar.trimURLs", true);

pref("browser.altClickSave", false);


pref("browser.download.debug", false);





pref("browser.download.saveLinkAsFilenameTimeout", 4000);

pref("browser.download.useDownloadDir", true);

pref("browser.download.folderList", 1);
pref("browser.download.manager.showAlertOnComplete", true);
pref("browser.download.manager.showAlertInterval", 2000);
pref("browser.download.manager.retention", 2);
pref("browser.download.manager.showWhenStarting", true);
pref("browser.download.manager.closeWhenDone", false);
pref("browser.download.manager.focusWhenStarting", false);
pref("browser.download.manager.flashCount", 2);
pref("browser.download.manager.addToRecentDocs", true);
pref("browser.download.manager.quitBehavior", 0);
pref("browser.download.manager.scanWhenDone", true);
pref("browser.download.manager.resumeOnWakeDelay", 10000);


pref("browser.download.useToolkitUI", false);


pref("browser.download.panel.shown", false);



pref("browser.download.panel.firstSessionCompleted", false);


pref("browser.search.searchEnginesURL",      "https://addons.mozilla.org/%LOCALE%/firefox/search-engines/");


pref("browser.search.defaultenginename",      "chrome://browser-region/locale/region.properties");


pref("browser.search.log", false);


pref("browser.search.order.1",                "chrome://browser-region/locale/region.properties");
pref("browser.search.order.2",                "chrome://browser-region/locale/region.properties");
pref("browser.search.order.3",                "chrome://browser-region/locale/region.properties");


pref("browser.search.openintab", false);


pref("browser.search.context.loadInBackground", false);


pref("browser.search.update", true);


pref("browser.search.update.log", false);


pref("browser.search.update.interval", 21600);


pref("browser.search.suggest.enabled", true);

#ifdef MOZ_OFFICIAL_BRANDING

pref("browser.search.official", true);
#endif

pref("browser.sessionhistory.max_entries", 50);



pref("browser.link.open_newwindow", 3);




pref("browser.link.open_newwindow.override.external", -1);




pref("browser.link.open_newwindow.restriction", 2);


#ifdef XP_MACOSX
pref("browser.link.open_newwindow.disabled_in_fullscreen", true);
#else
pref("browser.link.open_newwindow.disabled_in_fullscreen", false);
#endif


pref("browser.tabs.autoHide", false);
pref("browser.tabs.closeWindowWithLastTab", true);
pref("browser.tabs.insertRelatedAfterCurrent", true);
pref("browser.tabs.warnOnClose", true);
pref("browser.tabs.warnOnCloseOtherTabs", true);
pref("browser.tabs.warnOnOpen", true);
pref("browser.tabs.maxOpenBeforeWarn", 15);
pref("browser.tabs.loadInBackground", true);
pref("browser.tabs.opentabfor.middleclick", true);
pref("browser.tabs.loadDivertedInBackground", false);
pref("browser.tabs.loadBookmarksInBackground", false);
pref("browser.tabs.tabClipWidth", 140);
pref("browser.tabs.animate", true);
pref("browser.tabs.onTop", true);
#ifdef XP_WIN
pref("browser.tabs.drawInTitlebar", true);
#else
pref("browser.tabs.drawInTitlebar", false);
#endif






pref("browser.tabs.closeButtons", 1);






pref("browser.tabs.selectOwnerOnClose", true);

pref("browser.ctrlTab.previews", false);
pref("browser.ctrlTab.recentlyUsedLimit", 7);




pref("browser.bookmarks.autoExportHTML",          false);





pref("browser.bookmarks.max_backups",             10);


pref("dom.disable_open_during_load",              true);
pref("javascript.options.showInConsole",          true);
#ifdef DEBUG
pref("general.warnOnAboutConfig",                 false);
#endif






pref("dom.disable_window_open_feature.location",  true);

pref("dom.disable_window_status_change",          true);

pref("dom.disable_window_move_resize",            false);

pref("dom.disable_window_flip",                   true);


pref("privacy.popups.policy",               1);
pref("privacy.popups.usecustom",            true);
pref("privacy.popups.showBrowserMessage",   true);

pref("privacy.item.cookies",                false);

pref("privacy.clearOnShutdown.history",     true);
pref("privacy.clearOnShutdown.formdata",    true);
pref("privacy.clearOnShutdown.passwords",   false);
pref("privacy.clearOnShutdown.downloads",   true);
pref("privacy.clearOnShutdown.cookies",     true);
pref("privacy.clearOnShutdown.cache",       true);
pref("privacy.clearOnShutdown.sessions",    true);
pref("privacy.clearOnShutdown.offlineApps", false);
pref("privacy.clearOnShutdown.siteSettings", false);

pref("privacy.cpd.history",                 true);
pref("privacy.cpd.formdata",                true);
pref("privacy.cpd.passwords",               false);
pref("privacy.cpd.downloads",               true);
pref("privacy.cpd.cookies",                 true);
pref("privacy.cpd.cache",                   true);
pref("privacy.cpd.sessions",                true);
pref("privacy.cpd.offlineApps",             false);
pref("privacy.cpd.siteSettings",            false);







pref("privacy.sanitize.timeSpan", 1);
pref("privacy.sanitize.sanitizeOnShutdown", false);

pref("privacy.sanitize.migrateFx3Prefs",    false);

pref("network.proxy.share_proxy_settings",  false); 


pref("browser.gesture.swipe.left", "Browser:BackOrBackDuplicate");
pref("browser.gesture.swipe.right", "Browser:ForwardOrForwardDuplicate");
pref("browser.gesture.swipe.up", "cmd_scrollTop");
pref("browser.gesture.swipe.down", "cmd_scrollBottom");
#ifdef XP_MACOSX
pref("browser.gesture.pinch.latched", true);
pref("browser.gesture.pinch.threshold", 150);
#else
pref("browser.gesture.pinch.latched", false);
pref("browser.gesture.pinch.threshold", 25);
#endif
#ifdef XP_WIN

pref("browser.gesture.pinch.out", "cmd_fullZoomEnlarge");
pref("browser.gesture.pinch.in", "cmd_fullZoomReduce");
pref("browser.gesture.pinch.out.shift", "cmd_fullZoomReset");
pref("browser.gesture.pinch.in.shift", "cmd_fullZoomReset");
#else

pref("browser.gesture.pinch.out", "");
pref("browser.gesture.pinch.in", "");
pref("browser.gesture.pinch.out.shift", "");
pref("browser.gesture.pinch.in.shift", "");
#endif
pref("browser.gesture.twist.latched", false);
pref("browser.gesture.twist.threshold", 0);
pref("browser.gesture.twist.right", "cmd_gestureRotateRight");
pref("browser.gesture.twist.left", "cmd_gestureRotateLeft");
pref("browser.gesture.twist.end", "cmd_gestureRotateEnd");
pref("browser.gesture.tap", "cmd_fullZoomReset");





#ifdef XP_MACOSX



pref("mousewheel.with_alt.action", 2);
pref("mousewheel.with_shift.action", 1);




pref("mousewheel.with_meta.action", 3); 



pref("mousewheel.with_control.action.override_x", 0);
pref("mousewheel.with_meta.action.override_x", 0);
#else
pref("mousewheel.with_alt.action", 1);
pref("mousewheel.with_shift.action", 2);
pref("mousewheel.with_meta.action", 1); 
#endif
pref("mousewheel.with_control.action",3);
pref("mousewheel.with_win.action", 1);

pref("browser.xul.error_pages.enabled", true);
pref("browser.xul.error_pages.expert_bad_cert", false);


pref("network.manage-offline-status", false);


pref("network.protocol-handler.external.mailto", true); 
pref("network.protocol-handler.external.news", true);   
pref("network.protocol-handler.external.snews", true);  
pref("network.protocol-handler.external.nntp", true);   

pref("network.protocol-handler.warn-external.mailto", false);
pref("network.protocol-handler.warn-external.news", false);
pref("network.protocol-handler.warn-external.snews", false);
pref("network.protocol-handler.warn-external.nntp", false);





pref("network.protocol-handler.expose-all", true);
pref("network.protocol-handler.expose.mailto", false);
pref("network.protocol-handler.expose.news", false);
pref("network.protocol-handler.expose.snews", false);
pref("network.protocol-handler.expose.nntp", false);

pref("accessibility.typeaheadfind", false);
pref("accessibility.typeaheadfind.timeout", 5000);
pref("accessibility.typeaheadfind.linksonly", false);
pref("accessibility.typeaheadfind.flashBar", 1);


pref("pfs.datasource.url", "https://pfs.mozilla.org/plugins/PluginFinderService.php?mimetype=%PLUGIN_MIMETYPE%&appID=%APP_ID%&appVersion=%APP_VERSION%&clientOS=%CLIENT_OS%&chromeLocale=%CHROME_LOCALE%&appRelease=%APP_RELEASE%");


pref("plugins.hide_infobar_for_missing_plugin", false);
pref("plugins.hide_infobar_for_outdated_plugin", false);

#ifdef XP_MACOSX
pref("plugins.hide_infobar_for_carbon_failure_plugin", false);
#endif

pref("plugins.update.url", "https://www.mozilla.org/%LOCALE%/plugincheck/");
pref("plugins.update.notifyUser", false);

pref("plugins.click_to_play", false);

#ifdef XP_WIN
pref("browser.preferences.instantApply", false);
#else
pref("browser.preferences.instantApply", true);
#endif
#ifdef XP_MACOSX
pref("browser.preferences.animateFadeIn", true);
#else
pref("browser.preferences.animateFadeIn", false);
#endif


pref("browser.preferences.inContent", false);

pref("browser.download.show_plugins_in_list", true);
pref("browser.download.hide_plugins_without_extensions", true);





#ifdef UNIX_BUT_NOT_MAC
pref("browser.backspace_action", 2);
#else
pref("browser.backspace_action", 0);
#endif






pref("layout.spellcheckDefault", 1);

pref("browser.send_pings", false);


pref("browser.contentHandlers.types.0.title", "chrome://browser-region/locale/region.properties");
pref("browser.contentHandlers.types.0.uri", "chrome://browser-region/locale/region.properties");
pref("browser.contentHandlers.types.0.type", "application/vnd.mozilla.maybe.feed");
pref("browser.contentHandlers.types.1.title", "chrome://browser-region/locale/region.properties");
pref("browser.contentHandlers.types.1.uri", "chrome://browser-region/locale/region.properties");
pref("browser.contentHandlers.types.1.type", "application/vnd.mozilla.maybe.feed");
pref("browser.contentHandlers.types.2.title", "chrome://browser-region/locale/region.properties");
pref("browser.contentHandlers.types.2.uri", "chrome://browser-region/locale/region.properties");
pref("browser.contentHandlers.types.2.type", "application/vnd.mozilla.maybe.feed");
pref("browser.contentHandlers.types.3.title", "chrome://browser-region/locale/region.properties");
pref("browser.contentHandlers.types.3.uri", "chrome://browser-region/locale/region.properties");
pref("browser.contentHandlers.types.3.type", "application/vnd.mozilla.maybe.feed");
pref("browser.contentHandlers.types.4.title", "chrome://browser-region/locale/region.properties");
pref("browser.contentHandlers.types.4.uri", "chrome://browser-region/locale/region.properties");
pref("browser.contentHandlers.types.4.type", "application/vnd.mozilla.maybe.feed");
pref("browser.contentHandlers.types.5.title", "chrome://browser-region/locale/region.properties");
pref("browser.contentHandlers.types.5.uri", "chrome://browser-region/locale/region.properties");
pref("browser.contentHandlers.types.5.type", "application/vnd.mozilla.maybe.feed");

pref("browser.feeds.handler", "ask");
pref("browser.videoFeeds.handler", "ask");
pref("browser.audioFeeds.handler", "ask");





pref("gecko.handlerService.defaultHandlersVersion", "chrome://browser-region/locale/region.properties");







pref("gecko.handlerService.schemes.webcal.0.name", "chrome://browser-region/locale/region.properties");
pref("gecko.handlerService.schemes.webcal.0.uriTemplate", "chrome://browser-region/locale/region.properties");
pref("gecko.handlerService.schemes.webcal.1.name", "chrome://browser-region/locale/region.properties");
pref("gecko.handlerService.schemes.webcal.1.uriTemplate", "chrome://browser-region/locale/region.properties");
pref("gecko.handlerService.schemes.webcal.2.name", "chrome://browser-region/locale/region.properties");
pref("gecko.handlerService.schemes.webcal.2.uriTemplate", "chrome://browser-region/locale/region.properties");
pref("gecko.handlerService.schemes.webcal.3.name", "chrome://browser-region/locale/region.properties");
pref("gecko.handlerService.schemes.webcal.3.uriTemplate", "chrome://browser-region/locale/region.properties");


pref("gecko.handlerService.schemes.mailto.0.name", "chrome://browser-region/locale/region.properties");
pref("gecko.handlerService.schemes.mailto.0.uriTemplate", "chrome://browser-region/locale/region.properties");
pref("gecko.handlerService.schemes.mailto.1.name", "chrome://browser-region/locale/region.properties");
pref("gecko.handlerService.schemes.mailto.1.uriTemplate", "chrome://browser-region/locale/region.properties");
pref("gecko.handlerService.schemes.mailto.2.name", "chrome://browser-region/locale/region.properties");
pref("gecko.handlerService.schemes.mailto.2.uriTemplate", "chrome://browser-region/locale/region.properties");
pref("gecko.handlerService.schemes.mailto.3.name", "chrome://browser-region/locale/region.properties");
pref("gecko.handlerService.schemes.mailto.3.uriTemplate", "chrome://browser-region/locale/region.properties");


pref("gecko.handlerService.schemes.irc.0.name", "chrome://browser-region/locale/region.properties");
pref("gecko.handlerService.schemes.irc.0.uriTemplate", "chrome://browser-region/locale/region.properties");
pref("gecko.handlerService.schemes.irc.1.name", "chrome://browser-region/locale/region.properties");
pref("gecko.handlerService.schemes.irc.1.uriTemplate", "chrome://browser-region/locale/region.properties");
pref("gecko.handlerService.schemes.irc.2.name", "chrome://browser-region/locale/region.properties");
pref("gecko.handlerService.schemes.irc.2.uriTemplate", "chrome://browser-region/locale/region.properties");
pref("gecko.handlerService.schemes.irc.3.name", "chrome://browser-region/locale/region.properties");
pref("gecko.handlerService.schemes.irc.3.uriTemplate", "chrome://browser-region/locale/region.properties");


pref("gecko.handlerService.schemes.ircs.0.name", "chrome://browser-region/locale/region.properties");
pref("gecko.handlerService.schemes.ircs.0.uriTemplate", "chrome://browser-region/locale/region.properties");
pref("gecko.handlerService.schemes.ircs.1.name", "chrome://browser-region/locale/region.properties");
pref("gecko.handlerService.schemes.ircs.1.uriTemplate", "chrome://browser-region/locale/region.properties");
pref("gecko.handlerService.schemes.ircs.2.name", "chrome://browser-region/locale/region.properties");
pref("gecko.handlerService.schemes.ircs.2.uriTemplate", "chrome://browser-region/locale/region.properties");
pref("gecko.handlerService.schemes.ircs.3.name", "chrome://browser-region/locale/region.properties");
pref("gecko.handlerService.schemes.ircs.3.uriTemplate", "chrome://browser-region/locale/region.properties");


pref("gecko.handlerService.allowRegisterFromDifferentHost", false);

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

#ifdef MOZILLA_OFFICIAL


pref("browser.safebrowsing.id", "navclient-auto-ffox");
#endif



pref("urlclassifier.alternate_error_page", "blocked");


pref("urlclassifier.gethashnoise", 4);


pref("urlclassifier.gethashtables", "goog-phish-shavar,goog-malware-shavar");




pref("urlclassifier.max-complete-age", 2700);
#endif

pref("browser.geolocation.warning.infoURL", "http://www.mozilla.com/%LOCALE%/firefox/geolocation/");
pref("browser.mixedcontent.warning.infoURL", "http://support.mozilla.org/1/%APP%/%VERSION%/%OS%/%LOCALE%/mixed-content/");

pref("browser.EULA.version", 3);
pref("browser.rights.version", 3);
pref("browser.rights.3.shown", false);

#ifdef DEBUG

pref("browser.rights.override", true);
#endif

pref("browser.sessionstore.resume_from_crash", true);
pref("browser.sessionstore.resume_session_once", false);


pref("browser.sessionstore.interval", 15000);


pref("browser.sessionstore.postdata", 0);


pref("browser.sessionstore.privacy_level", 0);

pref("browser.sessionstore.privacy_level_deferred", 1);

pref("browser.sessionstore.max_tabs_undo", 10);


pref("browser.sessionstore.max_windows_undo", 3);


pref("browser.sessionstore.max_resumed_crashes", 1);





pref("browser.sessionstore.restore_on_demand", true);

pref("browser.sessionstore.restore_hidden_tabs", false);



pref("browser.sessionstore.restore_pinned_tabs_on_demand", false);


pref("accessibility.blockautorefresh", false);


pref("places.history.enabled", true);



pref("places.frecency.numVisits", 10);


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
pref("places.frecency.framedLinkVisitBonus", 0);
pref("places.frecency.linkVisitBonus", 100);
pref("places.frecency.typedVisitBonus", 2000);
pref("places.frecency.bookmarkVisitBonus", 75);
pref("places.frecency.downloadVisitBonus", 0);
pref("places.frecency.permRedirectVisitBonus", 0);
pref("places.frecency.tempRedirectVisitBonus", 0);
pref("places.frecency.defaultVisitBonus", 0);


pref("places.frecency.unvisitedBookmarkBonus", 140);
pref("places.frecency.unvisitedTypedBonus", 200);





pref("browser.ssl_override_behavior", 2);



pref("browser.offline-apps.notify", true);


pref("browser.zoom.full", true);


pref("browser.zoom.siteSpecific", true);


pref("browser.zoom.updateBackgroundTabs", true);


pref("breakpad.reportURL", "https://crash-stats.mozilla.com/report/index/");

#ifndef RELEASE_BUILD


pref("toolkit.crashreporter.pluginHangSubmitURL",
     "https://hang-reports.mozilla.org/submit");
#endif


pref("toolkit.crashreporter.infoURL",
     "http://www.mozilla.com/legal/privacy/firefox.html#crash-reporter");


pref("app.support.baseURL", "http://support.mozilla.org/1/firefox/%VERSION%/%OS%/%LOCALE%/");


pref("security.alternate_certificate_error_page", "certerror");


pref("browser.privatebrowsing.autostart", false);



pref("browser.bookmarks.editDialog.firstEditField", "namePicker");


#ifdef XP_MACOSX
pref("toolbar.customization.usesheet", true);
#else
pref("toolbar.customization.usesheet", false);
#endif



#ifdef XP_MACOSX

pref("dom.ipc.plugins.enabled.i386", false);
pref("dom.ipc.plugins.enabled.i386.flash player.plugin", true);
pref("dom.ipc.plugins.enabled.i386.javaplugin2_npapi.plugin", true);
pref("dom.ipc.plugins.enabled.i386.javaappletplugin.plugin", true);
pref("dom.ipc.plugins.enabled.i386.silverlight.plugin", true);

pref("dom.ipc.plugins.enabled.x86_64", true);
#else
pref("dom.ipc.plugins.enabled", true);
#endif

#ifdef MOZ_E10S_COMPAT
pref("browser.tabs.remote", true);
#endif








#ifdef XP_MACOSX
pref("dom.ipc.plugins.nativeCursorSupport", true);
#endif

#ifdef XP_WIN
pref("browser.taskbar.previews.enable", false);
pref("browser.taskbar.previews.max", 20);
pref("browser.taskbar.previews.cachetime", 5);
pref("browser.taskbar.lists.enabled", true);
pref("browser.taskbar.lists.frequent.enabled", true);
pref("browser.taskbar.lists.recent.enabled", false);
pref("browser.taskbar.lists.maxListItemCount", 7);
pref("browser.taskbar.lists.tasks.enabled", true);
pref("browser.taskbar.lists.refreshInSeconds", 120);
#endif

#ifdef MOZ_SERVICES_SYNC

pref("services.sync.registerEngines", "Bookmarks,Form,History,Password,Prefs,Tab,Addons");

pref("services.sync.prefs.sync.accessibility.blockautorefresh", true);
pref("services.sync.prefs.sync.accessibility.browsewithcaret", true);
pref("services.sync.prefs.sync.accessibility.typeaheadfind", true);
pref("services.sync.prefs.sync.accessibility.typeaheadfind.linksonly", true);
pref("services.sync.prefs.sync.addons.ignoreUserEnabledChanges", true);





pref("services.sync.prefs.sync.app.update.mode", true);
pref("services.sync.prefs.sync.browser.download.manager.closeWhenDone", true);
pref("services.sync.prefs.sync.browser.download.manager.retention", true);
pref("services.sync.prefs.sync.browser.download.manager.scanWhenDone", true);
pref("services.sync.prefs.sync.browser.download.manager.showWhenStarting", true);
pref("services.sync.prefs.sync.browser.formfill.enable", true);
pref("services.sync.prefs.sync.browser.link.open_newwindow", true);
pref("services.sync.prefs.sync.browser.offline-apps.notify", true);
pref("services.sync.prefs.sync.browser.safebrowsing.enabled", true);
pref("services.sync.prefs.sync.browser.safebrowsing.malware.enabled", true);
pref("services.sync.prefs.sync.browser.search.selectedEngine", true);
pref("services.sync.prefs.sync.browser.search.update", true);
pref("services.sync.prefs.sync.browser.sessionstore.restore_on_demand", true);
pref("services.sync.prefs.sync.browser.startup.homepage", true);
pref("services.sync.prefs.sync.browser.startup.page", true);
pref("services.sync.prefs.sync.browser.tabs.autoHide", true);
pref("services.sync.prefs.sync.browser.tabs.closeButtons", true);
pref("services.sync.prefs.sync.browser.tabs.loadInBackground", true);
pref("services.sync.prefs.sync.browser.tabs.warnOnClose", true);
pref("services.sync.prefs.sync.browser.tabs.warnOnOpen", true);
pref("services.sync.prefs.sync.browser.urlbar.autocomplete.enabled", true);
pref("services.sync.prefs.sync.browser.urlbar.default.behavior", true);
pref("services.sync.prefs.sync.browser.urlbar.maxRichResults", true);
pref("services.sync.prefs.sync.dom.disable_open_during_load", true);
pref("services.sync.prefs.sync.dom.disable_window_flip", true);
pref("services.sync.prefs.sync.dom.disable_window_move_resize", true);
pref("services.sync.prefs.sync.dom.event.contextmenu.enabled", true);
pref("services.sync.prefs.sync.extensions.personas.current", true);
pref("services.sync.prefs.sync.extensions.update.enabled", true);
pref("services.sync.prefs.sync.intl.accept_languages", true);
pref("services.sync.prefs.sync.javascript.enabled", true);
pref("services.sync.prefs.sync.layout.spellcheckDefault", true);
pref("services.sync.prefs.sync.lightweightThemes.isThemeSelected", true);
pref("services.sync.prefs.sync.lightweightThemes.usedThemes", true);
pref("services.sync.prefs.sync.network.cookie.cookieBehavior", true);
pref("services.sync.prefs.sync.network.cookie.lifetimePolicy", true);
pref("services.sync.prefs.sync.permissions.default.image", true);
pref("services.sync.prefs.sync.pref.advanced.images.disable_button.view_image", true);
pref("services.sync.prefs.sync.pref.advanced.javascript.disable_button.advanced", true);
pref("services.sync.prefs.sync.pref.downloads.disable_button.edit_actions", true);
pref("services.sync.prefs.sync.pref.privacy.disable_button.cookie_exceptions", true);
pref("services.sync.prefs.sync.privacy.clearOnShutdown.cache", true);
pref("services.sync.prefs.sync.privacy.clearOnShutdown.cookies", true);
pref("services.sync.prefs.sync.privacy.clearOnShutdown.downloads", true);
pref("services.sync.prefs.sync.privacy.clearOnShutdown.formdata", true);
pref("services.sync.prefs.sync.privacy.clearOnShutdown.history", true);
pref("services.sync.prefs.sync.privacy.clearOnShutdown.offlineApps", true);
pref("services.sync.prefs.sync.privacy.clearOnShutdown.passwords", true);
pref("services.sync.prefs.sync.privacy.clearOnShutdown.sessions", true);
pref("services.sync.prefs.sync.privacy.clearOnShutdown.siteSettings", true);
pref("services.sync.prefs.sync.privacy.donottrackheader.enabled", true);
pref("services.sync.prefs.sync.privacy.donottrackheader.value", true);
pref("services.sync.prefs.sync.privacy.sanitize.sanitizeOnShutdown", true);
pref("services.sync.prefs.sync.security.OCSP.disable_button.managecrl", true);
pref("services.sync.prefs.sync.security.OCSP.enabled", true);
pref("services.sync.prefs.sync.security.OCSP.require", true);
pref("services.sync.prefs.sync.security.default_personal_cert", true);
pref("services.sync.prefs.sync.security.enable_ssl3", true);
pref("services.sync.prefs.sync.security.enable_tls", true);
pref("services.sync.prefs.sync.signon.rememberSignons", true);
pref("services.sync.prefs.sync.spellchecker.dictionary", true);
pref("services.sync.prefs.sync.xpinstall.whitelist.required", true);
#endif


pref("devtools.errorconsole.enabled", false);


pref("devtools.toolbar.enabled", true);
pref("devtools.toolbar.visible", false);
pref("devtools.gcli.allowSet", false);
pref("devtools.commands.dir", "");


pref("devtools.toolbox.footer.height", 250);
pref("devtools.toolbox.sidebar.width", 500);
pref("devtools.toolbox.host", "bottom");
pref("devtools.toolbox.selectedTool", "webconsole");
pref("devtools.toolbox.toolbarSpec", '["paintflashing toggle","tilt toggle","scratchpad","resize toggle"]');
pref("devtools.toolbox.sideEnabled", true);


pref("devtools.inspector.enabled", true);
pref("devtools.inspector.activeSidebar", "ruleview");
pref("devtools.inspector.markupPreview", false);


pref("devtools.layoutview.enabled", true);
pref("devtools.layoutview.open", false);


pref("devtools.responsiveUI.enabled", true);


pref("devtools.debugger.enabled", true);
pref("devtools.debugger.chrome-enabled", true);
pref("devtools.debugger.remote-host", "localhost");
pref("devtools.debugger.remote-autoconnect", false);
pref("devtools.debugger.remote-connection-retries", 3);
pref("devtools.debugger.remote-timeout", 20000);


pref("devtools.debugger.ui.win-x", 0);
pref("devtools.debugger.ui.win-y", 0);
pref("devtools.debugger.ui.win-width", 900);
pref("devtools.debugger.ui.win-height", 400);
pref("devtools.debugger.ui.panes-sources-width", 200);
pref("devtools.debugger.ui.panes-instruments-width", 300);
pref("devtools.debugger.ui.pause-on-exceptions", false);
pref("devtools.debugger.ui.panes-visible-on-startup", false);
pref("devtools.debugger.ui.variables-sorting-enabled", true);
pref("devtools.debugger.ui.variables-only-enum-visible", false);
pref("devtools.debugger.ui.variables-searchbox-visible", false);


pref("devtools.profiler.enabled", true);


pref("devtools.tilt.enabled", true);
pref("devtools.tilt.intro_transition", true);
pref("devtools.tilt.outro_transition", true);


pref("devtools.scratchpad.enabled", true);




pref("devtools.scratchpad.recentFilesMax", 10);


pref("devtools.styleeditor.enabled", true);
pref("devtools.styleeditor.transitions", true);


pref("devtools.chrome.enabled", false);


pref("devtools.gcli.hideIntro", false);


pref("devtools.gcli.eagerHelper", 2);


pref("devtools.gcli.allowSet", false);


pref("devtools.webconsole.filter.network", true);
pref("devtools.webconsole.filter.networkinfo", true);
pref("devtools.webconsole.filter.csserror", true);
pref("devtools.webconsole.filter.cssparser", true);
pref("devtools.webconsole.filter.exception", true);
pref("devtools.webconsole.filter.jswarn", true);
pref("devtools.webconsole.filter.error", true);
pref("devtools.webconsole.filter.warn", true);
pref("devtools.webconsole.filter.info", true);
pref("devtools.webconsole.filter.log", true);


pref("devtools.webconsole.fontSize", 0);



pref("devtools.hud.loglimit.network", 200);
pref("devtools.hud.loglimit.cssparser", 200);
pref("devtools.hud.loglimit.exception", 200);
pref("devtools.hud.loglimit.console", 200);




pref("devtools.editor.tabsize", 4);
pref("devtools.editor.expandtab", true);







pref("devtools.editor.component", "orion");


pref("devtools.fontinspector.enabled", true);



pref("browser.menu.showCharacterEncoding", "chrome://browser/locale/browser.properties");


pref("prompts.tab_modal.enabled", true);

pref("browser.panorama.animate_zoom", true);


pref("browser.newtab.url", "about:newtab");

pref("browser.newtab.preload", false);


pref("browser.newtabpage.enabled", true);


pref("browser.newtabpage.rows", 3);


pref("browser.newtabpage.columns", 3);


pref("full-screen-api.enabled", true);




pref("full-screen-api.approval-required", true);




pref("toolkit.startup.max_resumed_crashes", 3);




pref("pdfjs.disabled", false);


pref("pdfjs.firstRun", true);


pref("pdfjs.previousHandler.preferredAction", 0);
pref("pdfjs.previousHandler.alwaysAskBeforeHandling", false);




pref("image.mem.max_decoded_image_kb", 256000);


pref("social.manifest.facebook", "{\"origin\":\"https://www.facebook.com\",\"name\":\"Facebook Messenger\",\"workerURL\":\"https://www.facebook.com/desktop/fbdesktop2/socialfox/fbworker.js.php\",\"iconURL\":\"data:image/x-icon;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8%2F9hAAAAX0lEQVQ4jWP4%2F%2F8%2FAyUYTFhHzjgDxP9JxGeQDSBVMxgTbUBCxer%2Fr999%2BQ8DJBuArJksA9A10s8AXIBoA0B%2BR%2FY%2FjD%2BEwoBoA1yT5v3PbdmCE8MAshhID%2FUMoDgzUYIBj0Cgi7ar4coAAAAASUVORK5CYII%3D\",\"sidebarURL\":\"https://www.facebook.com/desktop/fbdesktop2/?socialfox=true\",\"icon32URL\":\"data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAYAAABzenr0AAAACXBIWXMAAAsTAAALEwEAmpwYAAADbklEQVRYCc1Xv08UQRj99tctexAuCEFjRE0kGBEtLDSGqIWNxkYKbTAxNlY2JhaGWltNtNFeKgsKKxITK43/gCYW+IsoRhA4D47bH7fn9+bcvdm5JR7sefolC3Ozu9978+bNN7PayUv3HN3umdY0Y6IWBtSJ0HSTarXqTOiuTep6Lj+tdxAcA8RAgSmwdd2aCDs0clldYALb/FvgYVhjmfliVA2XpjEgWo0Attn42Z6WH1RFor5ehwo9XQIUZMoVn4qlCoVMSo62EvD8Kh0b3U2Xz43R2PBO6mUCGDlAf65V6MadZzT/rUimoccc2kYA4BfPHqJb105RzjJigKhRq9kEJUBIjgYVuXeL7SAI6eD+Abp5dTwVHOmEHxT50d8WBYJqSOdPj5BjW8gZR8UNqFR2xagx/65XFYaMH+BGWwiYpi4UkBPPLxTp9v1Z+lHc4DWvCQXWmIy6EjITgKowVd5Jjv7N3Hd6y5esigoOwpkJIAmMpZpLJGdiaaC4F0UmAj6bD84GCEwmB/qxMmRilmnwb/mpjAocHh4UEoNAt5NLZB7oy9OJo0PxqkAtePdhiSqunyC1LQUwWMPQaOr6GRre258Ajn4cP7KHcEXhsxpXbj+lT19X2TMNGTLVAcjcalS8gDwsQ2UOMhH4k8FkcrEn5E5ub2sKohxLK2VR77Hl9RUcsrgeRIEiVOT6z+tDbIeLy+vk+kGTCbXxycet6xhl//3f6bJEkdHYhA+mLtDIvoH4ieev5+juoxdk5+pjhALYEdXIpEB5w+NlSKSzqVQ/+H7IO6BLtl3fngGMiqhGJgIwlM6qpyUGFjySdk8m0Zg0ubeD7X9OIDEFajltRQgUJaUKx69tdgaQa0FMADuahZPMFtcEwNPm2hA7ZI5sK4aoE2NvYI+o8hkCIe7CwTv68zS0q9Dk5vpbm/8FXxitSzmMFHpsGj0wyLUheTwD2Y9fVgh1Ae0EPUgD9241ZEnld+v5kgnVZ/8fE0brVh5BK+1oCqKKF72Dk7HwBsssB/pklU1dfChy3S659H5+uelgIb+8WRv1/uGTV9Sdb5wJFlfW6fPCalMhwhSU1j2xKwKbP838GcOwJja4TqO0bjdmXxYTy1EYjFdCWoCEYZhseH/GDL3yJPHnuW6YmT7P1SlIA4768Hke4vOcsX8BE346lLHhDUQAAAAASUVORK5CYII=\", \"icon64URL\":\"data:image/png;base64,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\", \"description\":\"Keep up with friends wherever you go on the web.\",\"author\":\"Facebook\",\"homepageURL\":\"https://www.facebook.com/about/messenger-for-firefox\"}");




pref("social.whitelist", "");


pref("social.directories", "https://addons.mozilla.org");



pref("social.remote-install.enabled", true);

pref("social.sidebar.open", true);
pref("social.sidebar.unload_timeout_ms", 10000);
pref("social.toast-notifications.enabled", true);

pref("dom.identity.enabled", false);


pref("plain_text.wrap_long_lines", true);
