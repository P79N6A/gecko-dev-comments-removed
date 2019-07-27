# -*- indent-tabs-mode: nil; js-indent-level: 2 -*-
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


pref("extensions.checkCompatibility.temporaryThemeOverride_minAppVersion", "29.0a1");

pref("xpinstall.customConfirmationUI", true);


pref("extensions.getAddons.cache.enabled", true);
pref("extensions.getAddons.maxResults", 15);
pref("extensions.getAddons.get.url", "https://services.addons.mozilla.org/%LOCALE%/firefox/api/%API_VERSION%/search/guid:%IDS%?src=firefox&appOS=%OS%&appVersion=%VERSION%");
pref("extensions.getAddons.getWithPerformance.url", "https://services.addons.mozilla.org/%LOCALE%/firefox/api/%API_VERSION%/search/guid:%IDS%?src=firefox&appOS=%OS%&appVersion=%VERSION%&tMain=%TIME_MAIN%&tFirstPaint=%TIME_FIRST_PAINT%&tSessionRestored=%TIME_SESSION_RESTORED%");
pref("extensions.getAddons.search.browseURL", "https://addons.mozilla.org/%LOCALE%/firefox/search?q=%TERMS%&platform=%OS%&appver=%VERSION%");
pref("extensions.getAddons.search.url", "https://services.addons.mozilla.org/%LOCALE%/firefox/api/%API_VERSION%/search/%TERMS%/all/%MAX_RESULTS%/%OS%/%VERSION%/%COMPATIBILITY_MODE%?src=firefox");
pref("extensions.webservice.discoverURL", "https://services.addons.mozilla.org/%LOCALE%/firefox/discovery/pane/%VERSION%/%OS%/%COMPATIBILITY_MODE%");
pref("extensions.getAddons.recommended.url", "https://services.addons.mozilla.org/%LOCALE%/%APP%/api/%API_VERSION%/list/recommended/all/%MAX_RESULTS%/%OS%/%VERSION%?src=firefox");


pref("extensions.blocklist.enabled", true);
pref("extensions.blocklist.interval", 86400);


pref("extensions.blocklist.level", 2);
pref("extensions.blocklist.url", "https://blocklist.addons.mozilla.org/blocklist/3/%APP_ID%/%APP_VERSION%/%PRODUCT%/%BUILD_ID%/%BUILD_TARGET%/%LOCALE%/%CHANNEL%/%OS_VERSION%/%DISTRIBUTION%/%DISTRIBUTION_VERSION%/%PING_COUNT%/%TOTAL_PING_COUNT%/%DAYS_SINCE_LAST_PING%/");
pref("extensions.blocklist.detailsURL", "https://www.mozilla.org/%LOCALE%/blocklist/");
pref("extensions.blocklist.itemURL", "https://blocklist.addons.mozilla.org/%LOCALE%/%APP%/blocked/%blockID%");

pref("extensions.update.autoUpdateDefault", true);

pref("extensions.hotfix.id", "firefox-hotfix@mozilla.org");
pref("extensions.hotfix.cert.checkAttributes", true);
pref("extensions.hotfix.certs.1.sha1Fingerprint", "91:53:98:0C:C1:86:DF:47:8F:35:22:9E:11:C9:A7:31:04:49:A1:AA");



pref("extensions.autoDisableScopes", 15);


pref("xpinstall.signatures.required", false);


pref("browser.dictionaries.download.url", "https://addons.mozilla.org/%LOCALE%/firefox/dictionaries/");



pref("app.update.checkInstallTime", true);






pref("app.update.timerMinimumDelay", 120);









pref("app.update.altwindowtype", "Browser:About");


pref("app.update.log", false);




pref("app.update.backgroundMaxErrors", 10);





#if defined(XP_WIN) || defined(XP_MACOSX)
pref("app.update.cert.requireBuiltIn", false);
pref("app.update.cert.checkAttributes", false);
#else




pref("app.update.cert.requireBuiltIn", true);





pref("app.update.cert.checkAttributes", true);




pref("app.update.cert.maxErrors", 5);




















pref("app.update.certs.1.issuerName", "CN=DigiCert Secure Server CA,O=DigiCert Inc,C=US");
pref("app.update.certs.1.commonName", "aus4.mozilla.org");

pref("app.update.certs.2.issuerName", "CN=Thawte SSL CA,O=\"Thawte, Inc.\",C=US");
pref("app.update.certs.2.commonName", "aus4.mozilla.org");
#endif


pref("app.update.enabled", true);




pref("app.update.auto", true);


pref("app.update.mode", 1);


pref("app.update.silent", false);


#ifndef RELEASE_BUILD
pref("app.update.badge", true);
#else
pref("app.update.badge", false);
#endif



pref("app.update.staging.enabled", true);


pref("app.update.url", "https://aus4.mozilla.org/update/3/%PRODUCT%/%VERSION%/%BUILD_ID%/%BUILD_TARGET%/%LOCALE%/%CHANNEL%/%OS_VERSION%/%DISTRIBUTION%/%DISTRIBUTION_VERSION%/update.xml");










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

pref("lightweightThemes.update.enabled", true);
pref("lightweightThemes.getMoreURL", "https://addons.mozilla.org/%LOCALE%/firefox/themes");
pref("lightweightThemes.recommendedThemes", "[{\"id\":\"recommended-1\",\"homepageURL\":\"https://addons.mozilla.org/firefox/addon/a-web-browser-renaissance/\",\"headerURL\":\"resource:///chrome/browser/content/browser/defaultthemes/1.header.jpg\",\"footerURL\":\"resource:///chrome/browser/content/browser/defaultthemes/1.footer.jpg\",\"textcolor\":\"#000000\",\"accentcolor\":\"#f2d9b1\",\"iconURL\":\"resource:///chrome/browser/content/browser/defaultthemes/1.icon.jpg\",\"previewURL\":\"resource:///chrome/browser/content/browser/defaultthemes/1.preview.jpg\",\"author\":\"Sean.Martell\",\"version\":\"0\"},{\"id\":\"recommended-2\",\"homepageURL\":\"https://addons.mozilla.org/firefox/addon/space-fantasy/\",\"headerURL\":\"resource:///chrome/browser/content/browser/defaultthemes/2.header.jpg\",\"footerURL\":\"resource:///chrome/browser/content/browser/defaultthemes/2.footer.jpg\",\"textcolor\":\"#ffffff\",\"accentcolor\":\"#d9d9d9\",\"iconURL\":\"resource:///chrome/browser/content/browser/defaultthemes/2.icon.jpg\",\"previewURL\":\"resource:///chrome/browser/content/browser/defaultthemes/2.preview.jpg\",\"author\":\"fx5800p\",\"version\":\"1.0\"},{\"id\":\"recommended-3\",\"homepageURL\":\"https://addons.mozilla.org/firefox/addon/linen-light/\",\"headerURL\":\"resource:///chrome/browser/content/browser/defaultthemes/3.header.png\",\"footerURL\":\"resource:///chrome/browser/content/browser/defaultthemes/3.footer.png\",\"accentcolor\":\"#ada8a8\",\"iconURL\":\"resource:///chrome/browser/content/browser/defaultthemes/3.icon.png\",\"previewURL\":\"resource:///chrome/browser/content/browser/defaultthemes/3.preview.png\",\"author\":\"DVemer\",\"version\":\"1.0\"},{\"id\":\"recommended-4\",\"homepageURL\":\"https://addons.mozilla.org/firefox/addon/pastel-gradient/\",\"headerURL\":\"resource:///chrome/browser/content/browser/defaultthemes/4.header.png\",\"footerURL\":\"resource:///chrome/browser/content/browser/defaultthemes/4.footer.png\",\"textcolor\":\"#000000\",\"accentcolor\":\"#000000\",\"iconURL\":\"resource:///chrome/browser/content/browser/defaultthemes/4.icon.png\",\"previewURL\":\"resource:///chrome/browser/content/browser/defaultthemes/4.preview.png\",\"author\":\"darrinhenein\",\"version\":\"1.0\"},{\"id\":\"recommended-5\",\"homepageURL\":\"https://addons.mozilla.org/firefox/addon/carbon-light/\",\"headerURL\":\"resource:///chrome/browser/content/browser/defaultthemes/5.header.png\",\"footerURL\":\"resource:///chrome/browser/content/browser/defaultthemes/5.footer.png\",\"textcolor\":\"#3b3b3b\",\"accentcolor\":\"#2e2e2e\",\"iconURL\":\"resource:///chrome/browser/content/browser/defaultthemes/5.icon.jpg\",\"previewURL\":\"resource:///chrome/browser/content/browser/defaultthemes/5.preview.jpg\",\"author\":\"Jaxivo\",\"version\":\"1.0\"}]");

pref("browser.eme.ui.enabled", false);


pref("browser.uitour.enabled", true);
pref("browser.uitour.loglevel", "Error");
pref("browser.uitour.requireSecure", true);
pref("browser.uitour.themeOrigin", "https://addons.mozilla.org/%LOCALE%/firefox/themes/");
pref("browser.uitour.url", "https://www.mozilla.org/%LOCALE%/firefox/%VERSION%/tour/");

pref("browser.uitour.readerViewTrigger", "^https:\\/\\/www\\.mozilla\\.org\\/[^\\/]+\\/firefox\\/reading\\/start");

pref("browser.customizemode.tip0.shown", false);
pref("browser.customizemode.tip0.learnMoreUrl", "https://support.mozilla.org/1/firefox/%VERSION%/%OS%/%LOCALE%/customize");

pref("keyword.enabled", true);
pref("browser.fixup.domainwhitelist.localhost", true);

pref("general.useragent.locale", "@AB_CD@");
pref("general.skins.selectedSkin", "classic/1.0");

pref("general.smoothScroll", true);
#ifdef UNIX_BUT_NOT_MAC
pref("general.autoScroll", false);
#else
pref("general.autoScroll", true);
#endif


pref("browser.shell.checkDefaultBrowser", true);
pref("browser.shell.shortcutFavicons",true);



pref("browser.startup.page",                1);
pref("browser.startup.homepage",            "chrome://branding/locale/browserconfig.properties");

pref("browser.slowStartup.notificationDisabled", false);
pref("browser.slowStartup.timeThreshold", 40000);
pref("browser.slowStartup.maxSamples", 5);




pref("browser.aboutHomeSnippets.updateUrl", "https://snippets.cdn.mozilla.net/%STARTPAGE_VERSION%/%NAME%/%VERSION%/%APPBUILDID%/%BUILD_TARGET%/%LOCALE%/%CHANNEL%/%OS_VERSION%/%DISTRIBUTION%/%DISTRIBUTION_VERSION%/");

pref("browser.enable_automatic_image_resizing", true);
pref("browser.casting.enabled", false);
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


pref("browser.urlbar.unifiedcomplete", false);





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



pref("browser.urlbar.suggest.history",              true);
pref("browser.urlbar.suggest.bookmark",             true);
pref("browser.urlbar.suggest.openpage",             true);



pref("browser.urlbar.suggest.history.onlyTyped",    false);

pref("browser.urlbar.formatting.enabled", true);
pref("browser.urlbar.trimURLs", true);

pref("browser.altClickSave", false);


pref("browser.download.debug", false);





pref("browser.download.saveLinkAsFilenameTimeout", 4000);

pref("browser.download.useDownloadDir", true);
pref("browser.download.folderList", 1);
pref("browser.download.manager.addToRecentDocs", true);
pref("browser.download.manager.resumeOnWakeDelay", 10000);



pref("browser.download.animateNotifications", true);


pref("browser.download.panel.shown", false);

#ifndef XP_MACOSX
pref("browser.helperApps.deleteTempFileOnExit", true);
#endif


pref("browser.search.searchEnginesURL",      "https://addons.mozilla.org/%LOCALE%/firefox/search-engines/");


pref("browser.search.defaultenginename",      "chrome://browser-region/locale/region.properties");


pref("browser.search.order.1",                "chrome://browser-region/locale/region.properties");
pref("browser.search.order.2",                "chrome://browser-region/locale/region.properties");
pref("browser.search.order.3",                "chrome://browser-region/locale/region.properties");


pref("browser.search.geoSpecificDefaults", true);
pref("browser.search.defaultenginename.US",      "data:text/plain,browser.search.defaultenginename.US=Yahoo");
pref("browser.search.order.US.1",                "data:text/plain,browser.search.order.US.1=Yahoo");
pref("browser.search.order.US.2",                "data:text/plain,browser.search.order.US.2=Google");
pref("browser.search.order.US.3",                "data:text/plain,browser.search.order.US.3=Bing");


pref("browser.search.openintab", false);


pref("browser.search.context.loadInBackground", false);

pref("browser.search.showOneOffButtons", true);


pref("browser.search.hiddenOneOffs", "");

pref("browser.sessionhistory.max_entries", 50);


pref("permissions.manager.defaultsUrl", "resource://app/defaults/permissions");



pref("browser.link.open_newwindow", 3);




pref("browser.link.open_newwindow.override.external", -1);




pref("browser.link.open_newwindow.restriction", 2);






#ifdef XP_MACOSX
pref("browser.link.open_newwindow.disabled_in_fullscreen", true);
#else
pref("browser.link.open_newwindow.disabled_in_fullscreen", false);
#endif


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
#ifdef UNIX_BUT_NOT_MAC
pref("browser.tabs.drawInTitlebar", false);
#else
pref("browser.tabs.drawInTitlebar", true);
#endif






pref("browser.tabs.selectOwnerOnClose", true);

pref("browser.ctrlTab.previews", false);




pref("browser.bookmarks.autoExportHTML",          false);





pref("browser.bookmarks.max_backups",             15);


pref("dom.disable_open_during_load",              true);
pref("javascript.options.showInConsole",          true);
#ifdef DEBUG
pref("general.warnOnAboutConfig",                 false);
#endif






pref("dom.disable_window_open_feature.location",  true);

pref("dom.disable_window_status_change",          true);

pref("dom.disable_window_move_resize",            false);

pref("dom.disable_window_flip",                   true);



pref("dom.w3c_touch_events.enabled",        0);


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
pref("privacy.clearOnShutdown.openWindows", false);

pref("privacy.cpd.history",                 true);
pref("privacy.cpd.formdata",                true);
pref("privacy.cpd.passwords",               false);
pref("privacy.cpd.downloads",               true);
pref("privacy.cpd.cookies",                 true);
pref("privacy.cpd.cache",                   true);
pref("privacy.cpd.sessions",                true);
pref("privacy.cpd.offlineApps",             false);
pref("privacy.cpd.siteSettings",            false);
pref("privacy.cpd.openWindows",             false);









pref("privacy.sanitize.timeSpan", 1);
pref("privacy.sanitize.sanitizeOnShutdown", false);

pref("privacy.sanitize.migrateFx3Prefs",    false);

pref("privacy.panicButton.enabled",         true);

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

pref("browser.snapshots.limit", 0);





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
#ifdef XP_WIN
pref("network.protocol-handler.external.ms-windows-store", true);
#endif


pref("network.protocol-handler.warn-external.mailto", false);
pref("network.protocol-handler.warn-external.news", false);
pref("network.protocol-handler.warn-external.snews", false);
pref("network.protocol-handler.warn-external.nntp", false);
#ifdef XP_WIN
pref("network.protocol-handler.warn-external.ms-windows-store", false);
#endif





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

pref("plugins.update.url", "https://www.mozilla.org/%LOCALE%/plugincheck/?utm_source=firefox-browser&utm_medium=firefox-browser&utm_campaign=plugincheck-update");
pref("plugins.update.notifyUser", false);

pref("plugins.click_to_play", true);
pref("plugins.testmode", false);

pref("plugin.default.state", 1);


pref("plugin.defaultXpi.state", 2);



pref("plugin.state.flash", 2);
pref("plugin.state.java", 1);




#ifdef XP_WIN
pref("plugin.state.npunity3d", 2);
#endif
#ifdef XP_MACOSX
pref("plugin.state.unity web player", 2);
#endif


#ifdef XP_WIN
pref("plugin.state.npciscowebcommunicator", 2);
#endif
#ifdef XP_MACOSX
pref("plugin.state.ciscowebcommunicator", 2);
#endif


#ifdef XP_WIN
pref("plugin.state.npmcafeemss", 2);
#endif


#ifdef XP_WIN
pref("plugin.state.npplayerplugin", 2);
#endif
#ifdef XP_MACOSX
pref("plugin.state.playerplugin", 2);
pref("plugin.state.playerplugin.dtv", 2);
pref("plugin.state.playerplugin.ciscodrm", 2);
pref("plugin.state.playerplugin.charter", 2);
#endif


#ifdef XP_WIN
pref("plugin.state.npchip", 2);
#endif
#ifdef XP_MACOSX
pref("plugin.state.cisco jabber guest plug-in", 2);
#endif


#ifdef XP_WIN
pref("plugin.state.npesteid-firefox-plugin", 2);
#endif
#ifdef XP_MACOSX
pref("plugin.state.esteidfirefoxplugin", 2);
#endif
#ifdef UNIX_BUT_NOT_MAC
pref("plugin.state.npesteid-firefox-plugin", 2);
#endif


#ifdef XP_WIN
pref("plugin.state.npmozcouponprinter", 2);
#endif
#ifdef XP_MACOSX
pref("plugin.state.couponprinter-firefox_v", 2);
#endif


pref("plugin.state.npbispbrowser", 2);


#ifdef XP_WIN
pref("plugin.state.npgcplugin", 2);
#endif
#ifdef XP_MACOSX
pref("plugin.state.gcplugin", 2);
#endif


#ifdef XP_WIN
pref("plugin.state.npwebcard", 2);
#endif


#ifdef XP_WIN
pref("plugin.state.npatgpc", 2);
#endif
#ifdef XP_MACOSX
pref("plugin.state.webex", 2);
#endif
#ifdef UNIX_BUT_NOT_MAC
pref("plugin.state.npatgpc", 2);
#endif


#ifdef XP_WIN
pref("plugin.state.npskypewebplugin", 2);
#endif
#ifdef XP_MACOSX
pref("plugin.state.skypewebplugin", 2);
#endif


#ifdef XP_WIN
pref("plugin.state.npfacebookvideocalling", 2);
#endif
#ifdef XP_MACOSX
pref("plugin.state.facebookvideocalling", 2);
#endif


#ifdef XP_WIN
pref("plugin.state.npmeetingjoinpluginoc", 2);
#endif
#ifdef XP_MACOSX
pref("plugin.state.lwaplugin", 2);
#endif


#ifdef XP_WIN
pref("plugin.state.npvidyoweb", 2);
#endif
#ifdef XP_MACOSX
pref("plugin.state.npvidyoweb", 2);
pref("plugin.state.vidyoweb", 2);
#endif


#ifdef XP_WIN
pref("plugin.state.npmvtplugin", 2);
#endif


#ifdef XP_WIN
pref("plugin.state.npviewright", 2);
#endif
#ifdef XP_MACOSX
pref("plugin.state.viewrightwebplayer", 2);
#endif


#ifdef XP_WIN
pref("plugin.state.npmcffplg", 2);
#endif


#ifdef XP_MACOSX
pref("plugin.state.f5 ssl vpn plugin", 2);
pref("plugin.state.f5 sam inspection host plugin", 2);
#endif


#ifdef XP_WIN
pref("plugin.state.nprobloxproxy", 2);
#endif
#ifdef XP_MACOSX
pref("plugin.state.nproblox", 2);
#endif


#ifdef XP_WIN
pref("plugin.state.npboxedit", 2);
#endif
#ifdef XP_MACOSX
pref("plugin.state.box edit", 2);
#endif


#ifdef XP_WIN
pref("plugin.state.np_prsnl", 2);
#endif
#ifdef XP_MACOSX
pref("plugin.state.personalplugin", 2);
#endif
#ifdef UNIX_BUT_NOT_MAC
pref("plugin.state.libplugins", 2);
#endif


#ifdef XP_WIN
pref("plugin.state.npnipp", 2);
pref("plugin.state.npnisp", 2);
#endif
#ifdef XP_MACOSX
pref("plugin.state.iprint", 2);
#endif

#ifdef XP_MACOSX
pref("browser.preferences.animateFadeIn", true);
#else
pref("browser.preferences.animateFadeIn", false);
#endif

#ifdef XP_WIN
pref("browser.preferences.instantApply", false);
#else
pref("browser.preferences.instantApply", true);
#endif


pref("browser.preferences.inContent", true);

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
pref("browser.safebrowsing.downloads.enabled", true);
pref("browser.safebrowsing.downloads.remote.enabled", true);
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

pref("browser.safebrowsing.appRepURL", "https://sb-ssl.google.com/safebrowsing/clientreport/download?key=%GOOGLE_API_KEY%");

#ifdef MOZILLA_OFFICIAL


pref("browser.safebrowsing.id", "navclient-auto-ffox");
#endif



pref("urlclassifier.alternate_error_page", "blocked");


pref("urlclassifier.gethashnoise", 4);


pref("urlclassifier.gethash.timeout_ms", 5000);




pref("urlclassifier.max-complete-age", 2700);

pref("urlclassifier.downloadBlockTable", "goog-badbinurl-shavar");
#ifdef XP_WIN



pref("urlclassifier.downloadAllowTable", "goog-downloadwhite-digest256");
#endif
#endif

pref("browser.geolocation.warning.infoURL", "https://www.mozilla.org/%LOCALE%/firefox/geolocation/");
pref("browser.push.warning.infoURL", "https://www.mozilla.org/%LOCALE%/firefox/push/");

pref("browser.EULA.version", 3);
pref("browser.rights.version", 3);
pref("browser.rights.3.shown", false);

#ifdef DEBUG

pref("browser.rights.override", true);
#endif

pref("browser.selfsupport.url", "https://self-repair.mozilla.org/%LOCALE%/repair");

pref("browser.sessionstore.resume_from_crash", true);
pref("browser.sessionstore.resume_session_once", false);


pref("browser.sessionstore.interval", 15000);


pref("browser.sessionstore.privacy_level", 0);

pref("browser.sessionstore.privacy_level_deferred", 1);

pref("browser.sessionstore.max_tabs_undo", 10);


pref("browser.sessionstore.max_windows_undo", 3);


pref("browser.sessionstore.max_resumed_crashes", 1);

pref("browser.sessionstore.max_serialize_back", 10);

pref("browser.sessionstore.max_serialize_forward", -1);





pref("browser.sessionstore.restore_on_demand", true);

pref("browser.sessionstore.restore_hidden_tabs", false);



pref("browser.sessionstore.restore_pinned_tabs_on_demand", false);

pref("browser.sessionstore.upgradeBackup.latestBuildID", "");

pref("browser.sessionstore.upgradeBackup.maxUpgradeBackups", 3);

pref("browser.sessionstore.debug", false);

pref("browser.sessionstore.cleanup.forget_closed_after", 1209600000);


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


pref("toolkit.crashreporter.infoURL",
     "https://www.mozilla.org/legal/privacy/firefox.html#crash-reporter");


pref("app.support.baseURL", "https://support.mozilla.org/1/firefox/%VERSION%/%OS%/%LOCALE%/");


#ifdef MOZ_DEV_EDITION
pref("app.feedback.baseURL", "https://input.mozilla.org/%LOCALE%/feedback/firefoxdev/%VERSION%/");
#else
pref("app.feedback.baseURL", "https://input.mozilla.org/%LOCALE%/feedback/%APP%/%VERSION%/");
#endif



pref("security.alternate_certificate_error_page", "certerror");


pref("browser.privatebrowsing.autostart", false);



pref("browser.bookmarks.editDialog.firstEditField", "namePicker");

pref("dom.ipc.plugins.flash.disable-protected-mode", false);


pref("browser.flash-protected-mode-flip.enable", false);


pref("browser.flash-protected-mode-flip.done", false);

#ifdef XP_MACOSX

pref("dom.ipc.plugins.enabled.i386", true);
pref("dom.ipc.plugins.enabled.x86_64", true);
#else
pref("dom.ipc.plugins.enabled", true);
#endif

pref("dom.ipc.shims.enabledWarnings", false);


pref("browser.tabs.remote.autostart", false);
pref("browser.tabs.remote.desktopbehavior", true);

#if defined(XP_WIN) && defined(MOZ_SANDBOX)




pref("security.sandbox.windows.log", false);










pref("dom.ipc.plugins.sandbox-level.default", 0);

#if defined(MOZ_CONTENT_SANDBOX)







#if defined(NIGHTLY_BUILD)
pref("security.sandbox.content.level", 1);
#else
pref("security.sandbox.content.level", 0);
#endif


#if defined(MOZ_STACKWALKING)




pref("security.sandbox.windows.log.stackTraceDepth", 0);
#endif
#endif
#endif

#if defined(XP_MACOSX) && defined(MOZ_SANDBOX) && defined(MOZ_CONTENT_SANDBOX)








pref("security.sandbox.content.level", 1);
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
pref("services.sync.prefs.sync.browser.formfill.enable", true);
pref("services.sync.prefs.sync.browser.link.open_newwindow", true);
pref("services.sync.prefs.sync.browser.newtabpage.enabled", true);
pref("services.sync.prefs.sync.browser.newtabpage.enhanced", true);
pref("services.sync.prefs.sync.browser.newtabpage.pinned", true);
pref("services.sync.prefs.sync.browser.offline-apps.notify", true);
pref("services.sync.prefs.sync.browser.safebrowsing.enabled", true);
pref("services.sync.prefs.sync.browser.safebrowsing.malware.enabled", true);
pref("services.sync.prefs.sync.browser.search.update", true);
pref("services.sync.prefs.sync.browser.sessionstore.restore_on_demand", true);
pref("services.sync.prefs.sync.browser.startup.homepage", true);
pref("services.sync.prefs.sync.browser.startup.page", true);
pref("services.sync.prefs.sync.browser.tabs.loadInBackground", true);
pref("services.sync.prefs.sync.browser.tabs.warnOnClose", true);
pref("services.sync.prefs.sync.browser.tabs.warnOnOpen", true);
pref("services.sync.prefs.sync.browser.urlbar.autocomplete.enabled", true);
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
pref("services.sync.prefs.sync.lightweightThemes.selectedThemeID", true);
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
pref("services.sync.prefs.sync.privacy.sanitize.sanitizeOnShutdown", true);
pref("services.sync.prefs.sync.privacy.trackingprotection.enabled", true);
pref("services.sync.prefs.sync.security.OCSP.enabled", true);
pref("services.sync.prefs.sync.security.OCSP.require", true);
pref("services.sync.prefs.sync.security.default_personal_cert", true);
pref("services.sync.prefs.sync.security.tls.version.min", true);
pref("services.sync.prefs.sync.security.tls.version.max", true);
pref("services.sync.prefs.sync.signon.rememberSignons", true);
pref("services.sync.prefs.sync.spellchecker.dictionary", true);
pref("services.sync.prefs.sync.xpinstall.whitelist.required", true);
#endif


#ifdef MOZ_DEV_EDITION
sticky_pref("lightweightThemes.selectedThemeID", "firefox-devedition@mozilla.org");
sticky_pref("browser.devedition.theme.enabled", true);
#endif


pref("devtools.devedition.promo.shown", false);
pref("devtools.devedition.promo.url", "https://www.mozilla.org/firefox/developer/?utm_source=firefox-dev-tools&utm_medium=firefox-browser&utm_content=betadoorhanger");


#if MOZ_UPDATE_CHANNEL == beta
  pref("devtools.devedition.promo.enabled", true);
#else
  pref("devtools.devedition.promo.enabled", false);
#endif


pref("devtools.errorconsole.enabled", false);


pref("devtools.toolbar.enabled", true);
pref("devtools.toolbar.visible", false);
pref("devtools.commands.dir", "");


pref("devtools.appmanager.enabled", true);
pref("devtools.appmanager.lastTab", "help");
pref("devtools.appmanager.manifestEditor.enabled", true);


pref("devtools.webide.enabled", true);


pref("devtools.toolbox.footer.height", 250);
pref("devtools.toolbox.sidebar.width", 500);
pref("devtools.toolbox.host", "bottom");
pref("devtools.toolbox.selectedTool", "webconsole");
pref("devtools.toolbox.toolbarSpec", '["splitconsole", "paintflashing toggle","tilt toggle","scratchpad","resize toggle","eyedropper","screenshot --fullpage", "rulers"]');
pref("devtools.toolbox.sideEnabled", true);
pref("devtools.toolbox.zoomValue", "1");
pref("devtools.toolbox.splitconsoleEnabled", false);
pref("devtools.toolbox.splitconsoleHeight", 100);


pref("devtools.command-button-pick.enabled", true);
pref("devtools.command-button-frames.enabled", false);
pref("devtools.command-button-splitconsole.enabled", true);
pref("devtools.command-button-paintflashing.enabled", false);
pref("devtools.command-button-tilt.enabled", false);
pref("devtools.command-button-scratchpad.enabled", false);
pref("devtools.command-button-responsive.enabled", true);
pref("devtools.command-button-eyedropper.enabled", false);
pref("devtools.command-button-screenshot.enabled", false);
pref("devtools.command-button-rulers.enabled", false);



pref("devtools.inspector.enabled", true);

pref("devtools.inspector.activeSidebar", "ruleview");

pref("devtools.inspector.markupPreview", false);
pref("devtools.inspector.remote", false);

pref("devtools.inspector.show_pseudo_elements", true);

pref("devtools.inspector.imagePreviewTooltipSize", 300);

pref("devtools.inspector.showUserAgentStyles", false);

pref("devtools.inspector.showAllAnonymousContent", false);


pref("devtools.defaultColorUnit", "hex");


pref("devtools.responsiveUI.no-reload-notification", false);


pref("devtools.debugger.enabled", true);
pref("devtools.debugger.chrome-debugging-host", "localhost");
pref("devtools.debugger.chrome-debugging-port", 6080);
pref("devtools.debugger.remote-host", "localhost");
pref("devtools.debugger.remote-timeout", 20000);
pref("devtools.debugger.pause-on-exceptions", false);
pref("devtools.debugger.ignore-caught-exceptions", true);
pref("devtools.debugger.source-maps-enabled", true);
pref("devtools.debugger.pretty-print-enabled", true);
pref("devtools.debugger.auto-pretty-print", false);
pref("devtools.debugger.auto-black-box", true);
pref("devtools.debugger.tracer", false);


pref("devtools.debugger.ui.panes-sources-width", 200);
pref("devtools.debugger.ui.panes-instruments-width", 300);
pref("devtools.debugger.ui.panes-visible-on-startup", false);
pref("devtools.debugger.ui.variables-sorting-enabled", true);
pref("devtools.debugger.ui.variables-only-enum-visible", false);
pref("devtools.debugger.ui.variables-searchbox-visible", false);


pref("devtools.performance.enabled", true);


pref("devtools.performance.memory.sample-probability", "0.05");
pref("devtools.performance.memory.max-log-length", 2147483647); 
pref("devtools.performance.timeline.hidden-markers", "[]");
pref("devtools.performance.profiler.buffer-size", 10000000);
pref("devtools.performance.profiler.sample-frequency-khz", 1);
pref("devtools.performance.ui.invert-call-tree", true);
pref("devtools.performance.ui.invert-flame-graph", false);
pref("devtools.performance.ui.flatten-tree-recursion", true);
pref("devtools.performance.ui.show-platform-data", false);
pref("devtools.performance.ui.show-idle-blocks", true);
pref("devtools.performance.ui.enable-memory", false);
pref("devtools.performance.ui.enable-framerate", true);
pref("devtools.performance.ui.show-jit-optimizations", false);


pref("devtools.cache.disabled", false);


pref("devtools.serviceWorkers.testing.enabled", false);


pref("devtools.netmonitor.enabled", true);


pref("devtools.netmonitor.panes-network-details-width", 550);
pref("devtools.netmonitor.panes-network-details-height", 450);
pref("devtools.netmonitor.statistics", true);
pref("devtools.netmonitor.filters", "[\"all\"]");


pref("devtools.tilt.enabled", true);
pref("devtools.tilt.intro_transition", true);
pref("devtools.tilt.outro_transition", true);











pref("devtools.scratchpad.recentFilesMax", 10);
pref("devtools.scratchpad.lineNumbers", true);
pref("devtools.scratchpad.wrapText", false);
pref("devtools.scratchpad.showTrailingSpace", false);
pref("devtools.scratchpad.editorFontSize", 12);
pref("devtools.scratchpad.enableAutocompletion", true);


pref("devtools.storage.enabled", false);


pref("devtools.styleeditor.enabled", true);
pref("devtools.styleeditor.source-maps-enabled", true);
pref("devtools.styleeditor.autocompletion-enabled", true);
pref("devtools.styleeditor.showMediaSidebar", true);
pref("devtools.styleeditor.mediaSidebarWidth", 238);
pref("devtools.styleeditor.navSidebarWidth", 245);
pref("devtools.styleeditor.transitions", true);


pref("devtools.shadereditor.enabled", false);


pref("devtools.canvasdebugger.enabled", false);


pref("devtools.webaudioeditor.enabled", false);


pref("devtools.webaudioeditor.inspectorWidth", 300);


#ifdef MOZ_DEV_EDITION
sticky_pref("devtools.theme", "dark");
#else
sticky_pref("devtools.theme", "light");
#endif


pref("devtools.gcli.hideIntro", false);


pref("devtools.gcli.eagerHelper", 2);


pref("devtools.gcli.jquerySrc", "https://ajax.googleapis.com/ajax/libs/jquery/2.1.1/jquery.min.js");
pref("devtools.gcli.lodashSrc", "https://cdnjs.cloudflare.com/ajax/libs/lodash.js/2.4.1/lodash.min.js");
pref("devtools.gcli.underscoreSrc", "https://cdnjs.cloudflare.com/ajax/libs/underscore.js/1.7.0/underscore-min.js");


pref("devtools.webconsole.filter.network", true);
pref("devtools.webconsole.filter.networkinfo", false);
pref("devtools.webconsole.filter.netwarn", true);
pref("devtools.webconsole.filter.netxhr", false);
pref("devtools.webconsole.filter.csserror", true);
pref("devtools.webconsole.filter.cssparser", false);
pref("devtools.webconsole.filter.csslog", false);
pref("devtools.webconsole.filter.exception", true);
pref("devtools.webconsole.filter.jswarn", true);
pref("devtools.webconsole.filter.jslog", false);
pref("devtools.webconsole.filter.error", true);
pref("devtools.webconsole.filter.warn", true);
pref("devtools.webconsole.filter.info", true);
pref("devtools.webconsole.filter.log", true);
pref("devtools.webconsole.filter.secerror", true);
pref("devtools.webconsole.filter.secwarn", true);
pref("devtools.webconsole.filter.serviceworkers", false);
pref("devtools.webconsole.filter.sharedworkers", false);
pref("devtools.webconsole.filter.windowlessworkers", false);


pref("devtools.browserconsole.filter.network", true);
pref("devtools.browserconsole.filter.networkinfo", false);
pref("devtools.browserconsole.filter.netwarn", true);
pref("devtools.browserconsole.filter.netxhr", false);
pref("devtools.browserconsole.filter.csserror", true);
pref("devtools.browserconsole.filter.cssparser", false);
pref("devtools.browserconsole.filter.csslog", false);
pref("devtools.browserconsole.filter.exception", true);
pref("devtools.browserconsole.filter.jswarn", true);
pref("devtools.browserconsole.filter.jslog", true);
pref("devtools.browserconsole.filter.error", true);
pref("devtools.browserconsole.filter.warn", true);
pref("devtools.browserconsole.filter.info", true);
pref("devtools.browserconsole.filter.log", true);
pref("devtools.browserconsole.filter.secerror", true);
pref("devtools.browserconsole.filter.secwarn", true);
pref("devtools.browserconsole.filter.serviceworkers", true);
pref("devtools.browserconsole.filter.sharedworkers", true);
pref("devtools.browserconsole.filter.windowlessworkers", true);


pref("devtools.webconsole.fontSize", 0);


pref("devtools.webconsole.inputHistoryCount", 50);




pref("devtools.webconsole.persistlog", false);




pref("devtools.webconsole.timestampMessages", false);



pref("devtools.hud.loglimit.network", 200);
pref("devtools.hud.loglimit.cssparser", 200);
pref("devtools.hud.loglimit.exception", 200);
pref("devtools.hud.loglimit.console", 200);


pref("devtools.eyedropper.zoom", 6);








pref("devtools.editor.tabsize", 2);
pref("devtools.editor.expandtab", true);
pref("devtools.editor.keymap", "default");
pref("devtools.editor.autoclosebrackets", true);
pref("devtools.editor.detectindentation", true);
pref("devtools.editor.enableCodeFolding", true);
pref("devtools.editor.autocomplete", true);


pref("devtools.fontinspector.enabled", true);




pref("devtools.telemetry.tools.opened.version", "{}");


pref("devtools.gcli.imgurClientID", '0df414e888d7240');

pref("devtools.gcli.imgurUploadURL", "https://api.imgur.com/3/image");



pref("browser.menu.showCharacterEncoding", "chrome://browser/locale/browser.properties");


pref("prompts.tab_modal.enabled", true);

pref("browser.panorama.animate_zoom", true);


pref("browser.newtab.url", "about:newtab");

pref("browser.newtab.preload", true);


pref("browser.newtabpage.introShown", false);


pref("browser.newtabpage.enabled", true);


pref("browser.newtabpage.rows", 3);


pref("browser.newtabpage.columns", 5);


pref("browser.newtabpage.directory.source", "https://tiles.services.mozilla.com/v3/links/fetch/%LOCALE%/%CHANNEL%");


pref("browser.newtabpage.directory.ping", "https://tiles.services.mozilla.com/v3/links/");


pref("full-screen-api.enabled", true);




pref("full-screen-api.approval-required", true);




pref("toolkit.startup.max_resumed_crashes", 3);




pref("pdfjs.disabled", false);


pref("pdfjs.firstRun", true);


pref("pdfjs.previousHandler.preferredAction", 0);
pref("pdfjs.previousHandler.alwaysAskBeforeHandling", false);


#ifdef NIGHTLY_BUILD

#ifdef UNIX_BUT_NOT_MAC
pref("shumway.disabled", true);
#else
pref("shumway.disabled", false);
pref("shumway.swf.whitelist", "http://g-ecx.images-amazon.com/*/AiryBasicRenderer*.swf,http://z-ecx.images-amazon.com/*/AiryFlashlsRenderer._TTW_.swf,http://ia.media-imdb.com/*/AiryFlashlsRenderer._TTW_.swf");
#endif
#endif




pref("image.mem.max_decoded_image_kb", 256000);

pref("loop.enabled", true);
pref("loop.server", "https://loop.services.mozilla.com/v0");
pref("loop.seenToS", "unseen");
pref("loop.showPartnerLogo", true);
pref("loop.gettingStarted.seen", false);
pref("loop.gettingStarted.url", "https://www.mozilla.org/%LOCALE%/firefox/%VERSION%/hello/start/");
pref("loop.gettingStarted.resumeOnFirstJoin", false);
pref("loop.learnMoreUrl", "https://www.firefox.com/hello/");
pref("loop.legal.ToS_url", "https://www.mozilla.org/about/legal/terms/firefox-hello/");
pref("loop.legal.privacy_url", "https://www.mozilla.org/privacy/firefox-hello/");
pref("loop.do_not_disturb", false);
pref("loop.ringtone", "chrome://browser/content/loop/shared/sounds/ringtone.ogg");
pref("loop.retry_delay.start", 60000);
pref("loop.retry_delay.limit", 300000);
pref("loop.ping.interval", 1800000);
pref("loop.ping.timeout", 10000);
pref("loop.feedback.baseUrl", "https://input.mozilla.org/api/v1/feedback");
pref("loop.feedback.product", "Loop");
pref("loop.debug.loglevel", "Error");
pref("loop.debug.dispatcher", false);
pref("loop.debug.websocket", false);
pref("loop.debug.sdk", false);
pref("loop.debug.twoWayMediaTelemetry", false);
#ifdef DEBUG
pref("loop.CSP", "default-src 'self' about: file: chrome: http://localhost:*; img-src * data:; font-src 'none'; connect-src wss://*.tokbox.com https://*.opentok.com https://*.tokbox.com wss://*.mozilla.com https://*.mozilla.org wss://*.mozaws.net http://localhost:* ws://localhost:*; media-src blob:");
#else
pref("loop.CSP", "default-src 'self' about: file: chrome:; img-src * data:; font-src 'none'; connect-src wss://*.tokbox.com https://*.opentok.com https://*.tokbox.com wss://*.mozilla.com https://*.mozilla.org wss://*.mozaws.net; media-src blob:");
#endif
pref("loop.oauth.google.redirect_uri", "urn:ietf:wg:oauth:2.0:oob:auto");
pref("loop.oauth.google.scope", "https://www.google.com/m8/feeds");
pref("loop.fxa_oauth.tokendata", "");
pref("loop.fxa_oauth.profile", "");
pref("loop.support_url", "https://support.mozilla.org/kb/group-conversations-firefox-hello-webrtc");
pref("loop.contacts.gravatars.show", false);
pref("loop.contacts.gravatars.promo", true);
pref("loop.browserSharing.showInfoBar", true);
pref("loop.contextInConverations.enabled", false);

pref("social.sidebar.unload_timeout_ms", 10000);



#ifdef EARLY_BETA_OR_EARLIER
pref("social.share.activationPanelEnabled", true);
#else
pref("social.share.activationPanelEnabled", false);
#endif
pref("social.shareDirectory", "https://activations.cdn.mozilla.net/sharePanel.html");

pref("dom.identity.enabled", false);


pref("security.mixed_content.block_active_content", true);


pref("security.cert_pinning.enforcement_level", 1);


pref("plain_text.wrap_long_lines", true);



pref("dom.debug.propagate_gesture_events_through_content", false);


#ifdef RELEASE_BUILD
pref("geo.wifi.uri", "https://www.googleapis.com/geolocation/v1/geolocate?key=%GOOGLE_API_KEY%");
#else
pref("geo.wifi.uri", "https://location.services.mozilla.com/v1/geolocate?key=%MOZILLA_API_KEY%");
#endif

#ifdef XP_MACOSX
#ifdef RELEASE_BUILD
pref("geo.provider.use_corelocation", false);
#else
pref("geo.provider.use_corelocation", true);
#endif
#endif

#ifdef XP_WIN
#ifdef RELEASE_BUILD
pref("geo.provider.ms-windows-location", false);
#else
pref("geo.provider.ms-windows-location", true);
#endif
#endif



pref("network.disable.ipc.security", true);


pref("browser.uiCustomization.debug", false);


pref("browser.uiCustomization.state", "");


pref("identity.fxaccounts.remote.signup.uri", "https://accounts.firefox.com/signup?service=sync&context=fx_desktop_v1");



pref("identity.fxaccounts.remote.force_auth.uri", "https://accounts.firefox.com/force_auth?service=sync&context=fx_desktop_v1");


pref("identity.fxaccounts.remote.signin.uri", "https://accounts.firefox.com/signin?service=sync&context=fx_desktop_v1");




pref("identity.fxaccounts.settings.uri", "https://accounts.firefox.com/settings");


pref("identity.fxaccounts.remote.profile.uri", "https://profile.accounts.firefox.com/v1");


pref("identity.fxaccounts.remote.oauth.uri", "https://oauth.accounts.firefox.com/v1");



#ifdef MOZ_DEV_EDITION
pref("identity.fxaccounts.migrateToDevEdition", true);
#else
pref("identity.fxaccounts.migrateToDevEdition", false);
#endif


#ifdef MOZ_WIDGET_GTK
pref("ui.key.menuAccessKeyFocuses", true);
#endif


pref("media.eme.enabled", true);
pref("media.eme.apiVisible", true);

#ifdef XP_WIN
pref("media.gmp-eme-adobe.enabled", true);
pref("browser.eme.ui.enabled", true);
#endif




pref("browser.cache.frecency_experiment", 0);

pref("browser.translation.detectLanguage", false);
pref("browser.translation.neverForLanguages", "");

pref("browser.translation.ui.show", false);



pref("toolkit.telemetry.archive.enabled", true);


pref("experiments.enabled", true);
pref("experiments.manifest.fetchIntervalSeconds", 86400);
pref("experiments.manifest.uri", "https://telemetry-experiment.cdn.mozilla.net/manifest/v1/firefox/%VERSION%/%CHANNEL%");

pref("experiments.supported", true);


pref("media.gmp-provider.enabled", true);

pref("browser.apps.URL", "https://marketplace.firefox.com/discovery/");

#ifdef NIGHTLY_BUILD
pref("browser.polaris.enabled", false);
pref("privacy.trackingprotection.ui.enabled", false);
#endif

#ifdef NIGHTLY_BUILD



pref("browser.tabs.remote.autostart.1", false);
pref("browser.tabs.remote.autostart.2", true);
#endif

#ifdef NIGHTLY_BUILD

pref("extensions.interposition.enabled", true);
pref("extensions.interposition.prefetching", true);
#endif

pref("browser.defaultbrowser.notificationbar", false);



pref("dom.ipc.cpow.timeout", 500);



pref("dom.ipc.processHangMonitor", true);

#ifdef DEBUG


pref("dom.ipc.reportProcessHangs", false);
#else
pref("dom.ipc.reportProcessHangs", true);
#endif

pref("browser.readinglist.enabled", false);
pref("browser.readinglist.sidebarEverOpened", false);
pref("readinglist.scheduler.enabled", false);
pref("readinglist.server", "https://readinglist.services.mozilla.com/v1");


pref("reader.parse-node-limit", 0);


#ifdef NIGHTLY_BUILD
pref("dom.serviceWorkers.enabled", true);
#endif

pref("browser.pocket.enabled", false);
pref("browser.pocket.hostname", "localhost");
pref("browser.pocket.removedByUser", false);
pref("browser.pocket.useLocaleList", true);
pref("browser.pocket.enabledLocales", "en-US");
