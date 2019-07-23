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


pref("extensions.blocklist.enabled", true);
pref("extensions.blocklist.interval", 86400);
pref("extensions.blocklist.url", "https://addons.mozilla.org/blocklist/1/%APP_ID%/%APP_VERSION%/");
pref("extensions.blocklist.detailsURL", "http://%LOCALE%.www.mozilla.com/%LOCALE%/blocklist/");


pref("browser.dictionaries.download.url", "https://%LOCALE%.add-ons.mozilla.com/%LOCALE%/firefox/%VERSION%/dictionaries/");




pref("app.update.enabled", true);




pref("app.update.auto", true);










pref("app.update.mode", 1);


pref("app.update.silent", false);


pref("app.update.url", "https://aus2.mozilla.org/update/2/%PRODUCT%/%VERSION%/%BUILD_ID%/%BUILD_TARGET%/%LOCALE%/%CHANNEL%/%OS_VERSION%/update.xml");








pref("app.update.interval", 86400);


pref("app.update.nagTimer.download", 86400);


pref("app.update.nagTimer.restart", 1800);


pref("app.update.timer", 600000);






pref("app.update.showInstalledUI", false);






pref("app.update.incompatible.mode", 0);








pref("extensions.update.enabled", true);
pref("extensions.update.url", "chrome://mozapps/locale/extensions/extensions.properties");
pref("extensions.update.interval", 86400);  
                                            

pref("extensions.getMoreExtensionsURL", "https://%LOCALE%.add-ons.mozilla.com/%LOCALE%/%APP%/%VERSION%/extensions/");
pref("extensions.getMoreThemesURL", "https://%LOCALE%.add-ons.mozilla.com/%LOCALE%/%APP%/%VERSION%/themes/");
pref("extensions.dss.enabled", false);          
pref("extensions.dss.switchPending", false);    
                                                

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

pref("browser.cache.disk.capacity",         50000);
pref("browser.enable_automatic_image_resizing", true);
pref("browser.chrome.site_icons", true);
pref("browser.chrome.favicons", true);
pref("browser.formfill.enable", true);
pref("browser.warnOnQuit", true);

#ifdef XP_UNIX
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
pref("browser.urlbar.matchOnlyTyped", false);
pref("browser.urlbar.hideProtocols", "");
pref("browser.urlbar.animateBlend", true);

pref("browser.download.useDownloadDir", true);
pref("browser.download.folderList", 0);
pref("browser.download.manager.showAlertOnComplete", true);
pref("browser.download.manager.showAlertInterval", 2000);
pref("browser.download.manager.retention", 2);
pref("browser.download.manager.showWhenStarting", true);
pref("browser.download.manager.useWindow", true);
pref("browser.download.manager.closeWhenDone", false);
pref("browser.download.manager.openDelay", 0);
pref("browser.download.manager.focusWhenStarting", false);
pref("browser.download.manager.flashCount", 2);
pref("browser.download.manager.displayedHistoryDays", 7);


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

pref("browser.history.grouping", "day");
pref("browser.history.showSessions", false);
pref("browser.sessionhistory.max_entries", 50);
pref("browser.history_expire_days", 180);



pref("browser.link.open_external", 3);


pref("browser.link.open_newwindow", 3);




pref("browser.link.open_newwindow.restriction", 2);


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


pref("browser.bookmarks.sort.direction", "descending");
pref("browser.bookmarks.sort.resource", "rdf:http://home.netscape.com/NC-rdf#Name");


pref("dom.disable_open_during_load",              true);
#ifdef DEBUG
pref("javascript.options.showInConsole",          true);
pref("general.warnOnAboutConfig",                 false);
#else
pref("javascript.options.showInConsole",          false);
#endif


pref("dom.disable_window_open_feature.status",    true);




pref("dom.disable_window_open_feature.location",  false);
pref("dom.disable_window_status_change",          true);

pref("dom.disable_window_move_resize",            false);

pref("dom.disable_window_flip",                   true);


pref("privacy.popups.policy",               1);
pref("privacy.popups.usecustom",            true);
pref("privacy.popups.firstTime",            true);
pref("privacy.popups.showBrowserMessage",   true);
 
pref("privacy.item.history",    true);
pref("privacy.item.formdata",   true);
pref("privacy.item.passwords",  false);
pref("privacy.item.downloads",  true);
pref("privacy.item.cookies",    false);
pref("privacy.item.cache",      true);
pref("privacy.item.siteprefs",  false);
pref("privacy.item.sessions",   true);

pref("privacy.sanitize.sanitizeOnShutdown", false);
pref("privacy.sanitize.promptOnSanitize", true);

pref("network.proxy.share_proxy_settings",  false); 

pref("network.cookie.cookieBehavior",       0); 
pref("network.cookie.enableForCurrentSessionOnly", false);


pref("intl.accept_languages", "chrome://global/locale/intl.properties");


pref("intl.collationOption",  "chrome://global-platform/locale/intl.properties");
pref("intl.charsetmenu.browser.static", "chrome://global/locale/intl.properties");
pref("intl.charsetmenu.browser.more1",  "chrome://global/locale/intl.properties");
pref("intl.charsetmenu.browser.more2",  "chrome://global/locale/intl.properties");
pref("intl.charsetmenu.browser.more3",  "chrome://global/locale/intl.properties");
pref("intl.charsetmenu.browser.more4",  "chrome://global/locale/intl.properties");
pref("intl.charsetmenu.browser.more5",  "chrome://global/locale/intl.properties");
pref("intl.charsetmenu.browser.unicode",  "chrome://global/locale/intl.properties");
pref("intl.charset.detector", "chrome://global/locale/intl.properties");
pref("intl.charset.default",  "chrome://global-platform/locale/intl.properties");
pref("font.language.group", "chrome://global/locale/intl.properties");
pref("intl.menuitems.alwaysappendaccesskeys","chrome://global/locale/intl.properties");
pref("intl.menuitems.insertseparatorbeforeaccesskeys","chrome://global/locale/intl.properties");


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


pref("pfs.datasource.url", "https://pfs.mozilla.org/plugins/PluginFinderService.php?mimetype=%PLUGIN_MIMETYPE%&appID=%APP_ID%&appVersion=%APP_VERSION%&clientOS=%CLIENT_OS%&chromeLocale=%CHROME_LOCALE%");

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



pref("bidi.browser.ui", false);





#ifdef UNIX_BUT_NOT_MAC
pref("browser.backspace_action", 2);
#else
pref("browser.backspace_action", 0);
#endif






pref("layout.spellcheckDefault", 1);

pref("view_source.editor.path", "");
pref("view_source.editor.external", false);

pref("browser.send_pings", true);


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

#ifdef MOZ_SAFE_BROWSING

pref("browser.safebrowsing.enabled", true);
pref("browser.safebrowsing.remoteLookups", false);


pref("browser.safebrowsing.malware.enabled", true);


pref("browser.safebrowsing.provider.0.updateURL", "http://sb.google.com/safebrowsing/downloads?client={moz:client}&appver={moz:version}&pver=2.0");

pref("browser.safebrowsing.dataProvider", 0);


pref("browser.safebrowsing.provider.0.name", "Google");
pref("browser.safebrowsing.provider.0.lookupURL", "http://sb.google.com/safebrowsing/lookup?sourceid=firefox-antiphish&features=TrustRank&client={moz:client}&appver={moz:version}&");
pref("browser.safebrowsing.provider.0.keyURL", "https://sb-ssl.google.com/safebrowsing/getkey?client={moz:client}&");
pref("browser.safebrowsing.provider.0.reportURL", "http://sb.google.com/safebrowsing/report?");



pref("browser.safebrowsing.provider.0.privacy.url", "http://www.google.com/tools/firefox/firefox_privacy.html?hl=%LOCALE%");
pref("browser.safebrowsing.provider.0.privacy.fallbackurl", "chrome://browser/content/preferences/phishEULA.xhtml");


pref("browser.safebrowsing.provider.0.reportGenericURL", "http://{moz:locale}.phish-generic.mozilla.com/?hl={moz:locale}");
pref("browser.safebrowsing.provider.0.reportErrorURL", "http://{moz:locale}.phish-error.mozilla.com/?hl={moz:locale}");
pref("browser.safebrowsing.provider.0.reportPhishURL", "http://{moz:locale}.phish-report.mozilla.com/?hl={moz:locale}");


pref("browser.safebrowsing.warning.infoURL", "http://%LOCALE%.www.mozilla.com/%LOCALE%/firefox/phishing-protection/");
#endif


pref("browser.EULA.2.accepted", true);


pref("browser.EULA.version", 2);

pref("browser.sessionstore.enabled", true);
pref("browser.sessionstore.resume_from_crash", true);
pref("browser.sessionstore.resume_session_once", false);
 

pref("browser.sessionstore.interval", 10000);


pref("browser.sessionstore.postdata", 0);


pref("browser.sessionstore.privacy_level", 1);

pref("browser.sessionstore.max_tabs_undo", 10);


pref("accessibility.blockautorefresh", false);


pref("browser.places.importBookmarksHTML", true);


pref("browser.warn_chromeless_window.infobar", false);
