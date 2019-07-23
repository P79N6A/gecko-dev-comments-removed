# -*- Mode: Java; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
# ***** BEGIN LICENSE BLOCK *****
# Version: MPL 1.1/GPL 2.0/LGPL 2.1
#
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http:
#
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
#
# The Original Code is mozilla.org code.
#
# The Initial Developer of the Original Code is 
# Netscape Communications Corporation.
# Portions created by the Initial Developer are Copyright (C) 1998
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#
# Alternatively, the contents of this file may be used under the terms of
# either the GNU General Public License Version 2 or later (the "GPL"), or 
# the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
# in which case the provisions of the GPL or the LGPL are applicable instead
# of those above. If you wish to allow use of your version of this file only
# under the terms of either the GPL or the LGPL, and not to allow others to
# use your version of this file under the terms of the MPL, indicate your
# decision by deleting the provisions above and replace them with the notice
# and other provisions required by the GPL or the LGPL. If you do not delete
# the provisions above, a recipient may use your version of this file under
# the terms of any one of the MPL, the GPL or the LGPL.
#
# ***** END LICENSE BLOCK *****



#filter substitution

# SYNTAX HINTS:  dashes are delimiters.  Use underscores instead.
#  The first character after a period must be alphabetic.

#ifdef XP_UNIX
#ifndef XP_MACOSX
#define UNIX_BUT_NOT_MAC
#endif
#endif

pref("general.startup.browser", true);

pref("browser.chromeURL","chrome://browser/content/");
pref("browser.hiddenWindowChromeURL", "chrome://browser/content/hiddenWindow.xul");
pref("xpinstall.dialog.confirm", "chrome://mozapps/content/xpinstall/xpinstallConfirm.xul");
pref("xpinstall.dialog.progress.skin", "chrome://mozapps/content/extensions/extensions.xul");
pref("xpinstall.dialog.progress.chrome", "chrome://mozapps/content/extensions/extensions.xul");
pref("xpinstall.dialog.progress.type.skin", "Extension:Manager");
pref("xpinstall.dialog.progress.type.chrome", "Extension:Manager");





pref("extensions.ignoreMTimeChanges", false);

pref("extensions.logging.enabled", false);

pref("extensions.hideInstallButton", true);


pref("extensions.getAddons.showPane", true);
pref("extensions.getAddons.browseAddons", "https://%LOCALE%.add-ons.mozilla.com/%LOCALE%/%APP%");
pref("extensions.getAddons.maxResults", 5);
pref("extensions.getAddons.recommended.browseURL", "https://%LOCALE%.add-ons.mozilla.com/%LOCALE%/%APP%/recommended");
pref("extensions.getAddons.recommended.url", "https://services.addons.mozilla.org/%LOCALE%/%APP%/api/%API_VERSION%/list/featured/all/10/%OS%/%VERSION%");
pref("extensions.getAddons.search.browseURL", "https://%LOCALE%.add-ons.mozilla.com/%LOCALE%/%APP%/search?q=%TERMS%");
pref("extensions.getAddons.search.url", "https://services.addons.mozilla.org/%LOCALE%/%APP%/api/%API_VERSION%/search/%TERMS%/all/10/%OS%/%VERSION%");


pref("extensions.blocklist.enabled", true);
pref("extensions.blocklist.interval", 86400);


pref("extensions.blocklist.level", 2);
pref("extensions.blocklist.url", "https://addons.mozilla.org/blocklist/3/%APP_ID%/%APP_VERSION%/%PRODUCT%/%BUILD_ID%/%BUILD_TARGET%/%LOCALE%/%CHANNEL%/%OS_VERSION%/%DISTRIBUTION%/%DISTRIBUTION_VERSION%/");
pref("extensions.blocklist.detailsURL", "https://www.mozilla.com/%LOCALE%/blocklist/");


pref("browser.dictionaries.download.url", "https://%LOCALE%.add-ons.mozilla.com/%LOCALE%/firefox/%VERSION%/dictionaries/");




pref("app.update.enabled", true);




pref("app.update.auto", true);










pref("app.update.mode", 1);


pref("app.update.silent", false);


pref("app.update.url", "https://aus2.mozilla.org/update/3/%PRODUCT%/%VERSION%/%BUILD_ID%/%BUILD_TARGET%/%LOCALE%/%CHANNEL%/%OS_VERSION%/%DISTRIBUTION%/%DISTRIBUTION_VERSION%/update.xml");










pref("app.update.nagTimer.restart", 86400);



pref("app.update.timer", 600000);

pref("app.update.promptWaitTime", 43200);

pref("app.update.idletime", 60);






pref("app.update.showInstalledUI", false);






pref("app.update.incompatible.mode", 0);








pref("extensions.update.enabled", true);
pref("extensions.update.url", "https://versioncheck.addons.mozilla.org/update/VersionCheck.php?reqVersion=%REQ_VERSION%&id=%ITEM_ID%&version=%ITEM_VERSION%&maxAppVersion=%ITEM_MAXAPPVERSION%&status=%ITEM_STATUS%&appID=%APP_ID%&appVersion=%APP_VERSION%&appOS=%APP_OS%&appABI=%APP_ABI%&locale=%APP_LOCALE%&currentAppVersion=%CURRENT_APP_VERSION%&updateType=%UPDATE_TYPE%");
pref("extensions.update.interval", 86400);  
                                            

pref("extensions.getMoreExtensionsURL", "https://%LOCALE%.add-ons.mozilla.com/%LOCALE%/%APP%/%VERSION%/extensions/");
pref("extensions.getMoreThemesURL", "https://%LOCALE%.add-ons.mozilla.com/%LOCALE%/%APP%/%VERSION%/themes/");
pref("extensions.getMorePluginsURL", "https://%LOCALE%.add-ons.mozilla.com/%LOCALE%/%APP%/%VERSION%/plugins/");
pref("extensions.dss.enabled", false);          
pref("extensions.dss.switchPending", false);    
                                                

pref("extensions.{972ce4c6-7e08-4474-a285-3208198ce6fd}.name", "chrome://browser/locale/browser.properties");
pref("extensions.{972ce4c6-7e08-4474-a285-3208198ce6fd}.description", "chrome://browser/locale/browser.properties");

pref("xpinstall.whitelist.add", "update.mozilla.org");
pref("xpinstall.whitelist.add.103", "addons.mozilla.org");

pref("keyword.enabled", true);
pref("keyword.URL", "chrome://browser-region/locale/region.properties");

pref("general.useragent.locale", "@AB_CD@");
pref("general.skins.selectedSkin", "classic/1.0");
pref("general.useragent.extra.firefox", "@APP_UA_NAME@/@APP_VERSION@");

pref("general.smoothScroll", false);
#ifdef UNIX_BUT_NOT_MAC
pref("general.autoScroll", false);
#else
pref("general.autoScroll", true);
#endif



pref("browser.shell.checkDefaultBrowser", true);



pref("browser.startup.page",                1);
pref("browser.startup.homepage",            "resource:/browserconfig.properties");

pref("browser.enable_automatic_image_resizing", true);
pref("browser.chrome.site_icons", true);
pref("browser.chrome.favicons", true);
pref("browser.warnOnQuit", true);
pref("browser.warnOnRestart", true);
pref("browser.fullscreen.autohide", true);
pref("browser.fullscreen.animateUp", 1);

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
pref("browser.urlbar.autoFill", false);




pref("browser.urlbar.matchBehavior", 1);
pref("browser.urlbar.filter.javascript", true);


pref("browser.urlbar.maxRichResults", 12);



pref("browser.urlbar.delay", 50);




pref("browser.urlbar.restrict.history", "^");
pref("browser.urlbar.restrict.bookmark", "*");
pref("browser.urlbar.restrict.tag", "+");
pref("browser.urlbar.restrict.typed", "~");
pref("browser.urlbar.match.title", "#");
pref("browser.urlbar.match.url", "@");







pref("browser.urlbar.default.behavior", 0);





pref("browser.download.saveLinkAsFilenameTimeout", 1000);

pref("browser.download.useDownloadDir", true);

#ifdef WINCE
pref("browser.download.folderList", 2);

pref("browser.download.dir", "\\Hard Disk");
#else
pref("browser.download.folderList", 1);
#endif
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


pref("browser.search.searchEnginesURL",      "https://%LOCALE%.add-ons.mozilla.com/%LOCALE%/firefox/%VERSION%/search-engines/");


pref("browser.search.defaultenginename",      "chrome://browser-region/locale/region.properties");


pref("browser.search.log", false);


pref("browser.search.order.1",                "chrome://browser-region/locale/region.properties");
pref("browser.search.order.2",                "chrome://browser-region/locale/region.properties");


pref("browser.search.openintab", false);


pref("browser.search.update", true);


pref("browser.search.update.log", false);


pref("browser.search.updateinterval", 6);


pref("browser.microsummary.enabled", true);
pref("browser.microsummary.updateGenerators", true);


pref("browser.search.suggest.enabled", true);

pref("browser.history.showSessions", false);
pref("browser.sessionhistory.max_entries", 50);
pref("browser.history_expire_days", 180);
pref("browser.history_expire_days_min", 90);
pref("browser.history_expire_sites", 40000);



pref("browser.link.open_newwindow", 3);




pref("browser.link.open_newwindow.restriction", 2);


#ifndef WINCE
pref("browser.tabs.autoHide", false);
#else
pref("browser.tabs.autoHide", true);
#endif
pref("browser.tabs.closeWindowWithLastTab", true);
pref("browser.tabs.insertRelatedAfterCurrent", true);
pref("browser.tabs.warnOnClose", true);
pref("browser.tabs.warnOnOpen", true);
pref("browser.tabs.maxOpenBeforeWarn", 15);
pref("browser.tabs.loadInBackground", true);
pref("browser.tabs.loadFolderAndReplace", true);
pref("browser.tabs.opentabfor.middleclick", true);
pref("browser.tabs.loadDivertedInBackground", false);
pref("browser.tabs.loadBookmarksInBackground", false);
pref("browser.tabs.tabMinWidth", 100);
pref("browser.tabs.tabMaxWidth", 250);
pref("browser.tabs.tabClipWidth", 140);






pref("browser.tabs.closeButtons", 1);






pref("browser.tabs.selectOwnerOnClose", true);

pref("browser.allTabs.previews", false);
pref("browser.ctrlTab.previews", false);
pref("browser.ctrlTab.recentlyUsedLimit", 7);


pref("browser.bookmarks.sort.direction", "descending");
pref("browser.bookmarks.sort.resource", "rdf:http://home.netscape.com/NC-rdf#Name");




pref("browser.bookmarks.autoExportHTML",          false);





pref("browser.bookmarks.max_backups",             5);


pref("dom.disable_open_during_load",              true);
#ifdef DEBUG
pref("javascript.options.showInConsole",          true);
pref("general.warnOnAboutConfig",                 false);
#else
pref("javascript.options.showInConsole",          false);
#endif

#ifdef WINCE

pref("dom.max_script_run_time",                   20);
#endif


pref("dom.disable_window_open_feature.status",    true);





pref("dom.disable_window_open_feature.location",  true);
pref("dom.disable_window_status_change",          true);

pref("dom.disable_window_move_resize",            false);

pref("dom.disable_window_flip",                   true);


pref("privacy.popups.policy",               1);
pref("privacy.popups.usecustom",            true);
pref("privacy.popups.firstTime",            true);
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

pref("network.cookie.cookieBehavior", 0); 


pref("intl.accept_languages", "chrome://global/locale/intl.properties");
pref("intl.charsetmenu.browser.static", "chrome://global/locale/intl.properties");
pref("intl.charsetmenu.browser.more1",  "chrome://global/locale/intl.properties");
pref("intl.charsetmenu.browser.more2",  "chrome://global/locale/intl.properties");
pref("intl.charsetmenu.browser.more3",  "chrome://global/locale/intl.properties");
pref("intl.charsetmenu.browser.more4",  "chrome://global/locale/intl.properties");
pref("intl.charsetmenu.browser.more5",  "chrome://global/locale/intl.properties");
pref("intl.charsetmenu.browser.unicode",  "UTF-8, UTF-16LE, UTF-16BE, UTF-32, UTF-32LE, UTF-32BE");
pref("intl.charset.detector", "chrome://global/locale/intl.properties");
pref("intl.charset.default",  "chrome://global-platform/locale/intl.properties");
pref("font.language.group", "chrome://global/locale/intl.properties");
pref("intl.menuitems.alwaysappendaccesskeys","chrome://global/locale/intl.properties");
pref("intl.menuitems.insertseparatorbeforeaccesskeys","chrome://global/locale/intl.properties");


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
pref("browser.gesture.pinch.out", "cmd_fullZoomEnlarge");
pref("browser.gesture.pinch.in", "cmd_fullZoomReduce");
pref("browser.gesture.pinch.out.shift", "cmd_fullZoomReset");
pref("browser.gesture.pinch.in.shift", "cmd_fullZoomReset");
pref("browser.gesture.twist.latched", false);
pref("browser.gesture.twist.threshold", 25);
pref("browser.gesture.twist.right", "");
pref("browser.gesture.twist.left", "");
pref("browser.gesture.tap", "cmd_fullZoomReset");


#ifdef XP_MACOSX



pref("mousewheel.withshiftkey.action",0);
pref("mousewheel.withshiftkey.sysnumlines",true);
pref("mousewheel.withshiftkey.numlines",1);
pref("mousewheel.withaltkey.action",2);
pref("mousewheel.withaltkey.sysnumlines",false);
pref("mousewheel.withaltkey.numlines",1);
pref("mousewheel.withmetakey.action",0);
pref("mousewheel.withmetakey.sysnumlines",false);
pref("mousewheel.withmetakey.numlines",1);
#else
pref("mousewheel.withshiftkey.action",2);
pref("mousewheel.withshiftkey.sysnumlines",false);
pref("mousewheel.withshiftkey.numlines",1);
pref("mousewheel.withaltkey.action",0);
pref("mousewheel.withaltkey.sysnumlines",false);
pref("mousewheel.withaltkey.numlines",1);
pref("mousewheel.withmetakey.action",0);
pref("mousewheel.withmetakey.sysnumlines",true);
pref("mousewheel.withmetakey.numlines",1);
#endif
pref("mousewheel.withcontrolkey.action",3);
pref("mousewheel.withcontrolkey.sysnumlines",false);
pref("mousewheel.withcontrolkey.numlines",1);

pref("profile.allow_automigration", false);   


pref("custtoolbar.personal_toolbar_folder", "");


pref("alerts.slideIncrement", 1);
pref("alerts.slideIncrementTime", 10);
pref("alerts.totalOpenTime", 4000);

pref("browser.xul.error_pages.enabled", true);
pref("browser.xul.error_pages.expert_bad_cert", false);


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


pref("security.warn_entering_secure.show_once", false);
pref("security.warn_entering_weak.show_once", true);
pref("security.warn_leaving_secure.show_once", false);
pref("security.warn_viewing_mixed.show_once", true);
pref("security.warn_submit_insecure.show_once", false);

pref("accessibility.typeaheadfind", false);
pref("accessibility.typeaheadfind.timeout", 5000);
pref("accessibility.typeaheadfind.linksonly", false);
pref("accessibility.typeaheadfind.flashBar", 1);


pref("plugin.default_plugin_disabled", true);


pref("pfs.datasource.url", "https://pfs.mozilla.org/plugins/PluginFinderService.php?mimetype=%PLUGIN_MIMETYPE%&appID=%APP_ID%&appVersion=%APP_VERSION%&clientOS=%CLIENT_OS%&chromeLocale=%CHROME_LOCALE%&appRelease=%APP_RELEASE%");


pref("plugins.hide_infobar_for_missing_plugin", false);

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


pref("browser.safebrowsing.provider.0.updateURL", "http://safebrowsing.clients.google.com/safebrowsing/downloads?client={moz:client}&appver={moz:version}&pver=2.2");

pref("browser.safebrowsing.dataProvider", 0);


pref("browser.safebrowsing.provider.0.name", "Google");
pref("browser.safebrowsing.provider.0.lookupURL", "http://safebrowsing.clients.google.com/safebrowsing/lookup?sourceid=firefox-antiphish&features=TrustRank&client={moz:client}&appver={moz:version}&");
pref("browser.safebrowsing.provider.0.keyURL", "https://sb-ssl.google.com/safebrowsing/newkey?client={moz:client}&appver={moz:version}&pver=2.2");
pref("browser.safebrowsing.provider.0.reportURL", "http://safebrowsing.clients.google.com/safebrowsing/report?");
pref("browser.safebrowsing.provider.0.gethashURL", "http://safebrowsing.clients.google.com/safebrowsing/gethash?client={moz:client}&appver={moz:version}&pver=2.2");


pref("browser.safebrowsing.provider.0.reportGenericURL", "http://{moz:locale}.phish-generic.mozilla.com/?hl={moz:locale}");
pref("browser.safebrowsing.provider.0.reportErrorURL", "http://{moz:locale}.phish-error.mozilla.com/?hl={moz:locale}");
pref("browser.safebrowsing.provider.0.reportPhishURL", "http://{moz:locale}.phish-report.mozilla.com/?hl={moz:locale}");
pref("browser.safebrowsing.provider.0.reportMalwareURL", "http://{moz:locale}.malware-report.mozilla.com/?hl={moz:locale}");
pref("browser.safebrowsing.provider.0.reportMalwareErrorURL", "http://{moz:locale}.malware-error.mozilla.com/?hl={moz:locale}");


pref("browser.safebrowsing.warning.infoURL", "http://%LOCALE%.www.mozilla.com/%LOCALE%/firefox/phishing-protection/");
pref("browser.geolocation.warning.infoURL", "http://%LOCALE%.www.mozilla.com/%LOCALE%/firefox/geolocation/");



pref("urlclassifier.alternate_error_page", "blocked");


pref("urlclassifier.gethashnoise", 4);


pref("urlclassifier.gethashtables", "goog-phish-shavar,goog-malware-shavar");




pref("urlclassifier.confirm-age", 2700);

#ifdef MOZ_WIDGET_GTK2
#define RESTRICT_CACHEMAX
#endif
#ifdef XP_OS2
#define RESTRICT_CACHEMAX
#endif


#ifdef RESTRICT_CACHEMAX
pref("urlclassifier.updatecachemax", 104857600);
#else
pref("urlclassifier.updatecachemax", -1);
#endif


pref("browser.safebrowsing.malware.reportURL", "http://safebrowsing.clients.google.com/safebrowsing/diagnostic?client=%NAME%&hl=%LOCALE%&site=");

#endif

pref("browser.EULA.version", 3);
pref("browser.rights.version", 3);
pref("browser.rights.3.shown", false);

#ifdef DEBUG

pref("browser.rights.override", true);
#endif

pref("browser.sessionstore.resume_from_crash", true);
pref("browser.sessionstore.resume_session_once", false);


pref("browser.sessionstore.interval", 10000);


pref("browser.sessionstore.postdata", 0);


pref("browser.sessionstore.privacy_level", 1);

pref("browser.sessionstore.max_tabs_undo", 10);


pref("browser.sessionstore.max_windows_undo", 3);


pref("browser.sessionstore.max_resumed_crashes", 1);


pref("accessibility.blockautorefresh", false);



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





pref("browser.identity.ssl_domain_display", 1);



pref("browser.offline-apps.notify", true);


pref("browser.zoom.full", true);


pref("browser.zoom.siteSpecific", true);


pref("browser.zoom.updateBackgroundTabs", true);


pref("editor.singleLine.pasteNewlines", 2);


pref("breakpad.reportURL", "http://crash-stats.mozilla.com/report/index/");


pref("app.support.baseURL", "http://support.mozilla.com/1/%APP%/%VERSION%/%OS%/%LOCALE%/");


pref("security.alternate_certificate_error_page", "certerror");


pref("browser.privatebrowsing.autostart", false);


pref("browser.privatebrowsing.dont_prompt_on_enter", false);



pref("browser.bookmarks.editDialog.firstEditField", "namePicker");


pref("geo.wifi.uri", "https://www.google.com/loc/json");

#ifdef WINCE


pref("browser.urlbar.delay", 250);


pref("browser.safebrowsing.enabled", false);
pref("browser.safebrowsing.malware.enabled", false);


pref("browser.shell.checkDefaultBrowser", false);


pref("browser.sessionhistory.max_total_viewers", 0);


pref("content.sink.interactive_deflect_count", 10); 
pref("content.sink.perf_deflect_count", 50); 
pref("content.sink.interactive_parse_time", 5000); 
pref("content.sink.perf_parse_time", 150000); 
pref("content.sink.pending_event_mode", 0); 
pref("content.sink.event_probe_rate", 1); 
pref("content.sink.interactive_time", 750000); 
pref("content.sink.initial_perf_time", 500000); 
pref("content.sink.enable_perf_mode", 0); 


pref("browser.sessionstore.interval", 60000);

#endif 


#ifdef XP_MACOSX
pref("toolbar.customization.usesheet", true);
#else
pref("toolbar.customization.usesheet", false);
#endif
