# -*- indent-tabs-mode: nil; js-indent-level: 2 -*-
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http:

let Ci = Components.interfaces;
let Cu = Components.utils;
let Cc = Components.classes;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/NotificationDB.jsm");
Cu.import("resource:///modules/RecentWindow.jsm");


XPCOMUtils.defineLazyModuleGetter(this, "Preferences",
                                  "resource://gre/modules/Preferences.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Deprecated",
                                  "resource://gre/modules/Deprecated.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "BrowserUITelemetry",
                                  "resource:///modules/BrowserUITelemetry.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "E10SUtils",
                                  "resource:///modules/E10SUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "BrowserUtils",
                                  "resource://gre/modules/BrowserUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
                                  "resource://gre/modules/Task.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PromiseUtils",
                                  "resource://gre/modules/PromiseUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "CharsetMenu",
                                  "resource://gre/modules/CharsetMenu.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "ShortcutUtils",
                                  "resource://gre/modules/ShortcutUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "GMPInstallManager",
                                  "resource://gre/modules/GMPInstallManager.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "NewTabUtils",
                                  "resource://gre/modules/NewTabUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "ContentSearch",
                                  "resource:///modules/ContentSearch.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "AboutHome",
                                  "resource:///modules/AboutHome.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Log",
                                  "resource://gre/modules/Log.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "AppConstants",
                                  "resource://gre/modules/AppConstants.jsm");
XPCOMUtils.defineLazyServiceGetter(this, "Favicons",
                                   "@mozilla.org/browser/favicon-service;1",
                                   "mozIAsyncFavicons");
XPCOMUtils.defineLazyServiceGetter(this, "gDNSService",
                                   "@mozilla.org/network/dns-service;1",
                                   "nsIDNSService");
XPCOMUtils.defineLazyModuleGetter(this, "LightweightThemeManager",
                                  "resource://gre/modules/LightweightThemeManager.jsm");


const nsIWebNavigation = Ci.nsIWebNavigation;

var gLastBrowserCharset = null;
var gProxyFavIcon = null;
var gLastValidURLStr = "";
var gInPrintPreviewMode = false;
var gContextMenu = null; 
var gMultiProcessBrowser =
  window.QueryInterface(Ci.nsIInterfaceRequestor)
        .getInterface(Ci.nsIWebNavigation)
        .QueryInterface(Ci.nsILoadContext)
        .useRemoteTabs;
var gAppInfo = Cc["@mozilla.org/xre/app-info;1"]
                  .getService(Ci.nsIXULAppInfo)
                  .QueryInterface(Ci.nsIXULRuntime);

#ifndef XP_MACOSX
var gEditUIVisible = true;
#endif

[
  ["gBrowser",            "content"],
  ["gNavToolbox",         "navigator-toolbox"],
  ["gURLBar",             "urlbar"],
  ["gNavigatorBundle",    "bundle_browser"]
].forEach(function (elementGlobal) {
  var [name, id] = elementGlobal;
  window.__defineGetter__(name, function () {
    var element = document.getElementById(id);
    if (!element)
      return null;
    delete window[name];
    return window[name] = element;
  });
  window.__defineSetter__(name, function (val) {
    delete window[name];
    return window[name] = val;
  });
});




this.__defineGetter__("gFindBar", function() {
  return window.gBrowser.getFindBar();
});

this.__defineGetter__("gFindBarInitialized", function() {
  return window.gBrowser.isFindBarInitialized();
});

XPCOMUtils.defineLazyGetter(this, "gPrefService", function() {
  return Services.prefs;
});

this.__defineGetter__("AddonManager", function() {
  let tmp = {};
  Cu.import("resource://gre/modules/AddonManager.jsm", tmp);
  return this.AddonManager = tmp.AddonManager;
});
this.__defineSetter__("AddonManager", function (val) {
  delete this.AddonManager;
  return this.AddonManager = val;
});

XPCOMUtils.defineLazyModuleGetter(this, "PluralForm",
  "resource://gre/modules/PluralForm.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "TelemetryStopwatch",
  "resource://gre/modules/TelemetryStopwatch.jsm");

XPCOMUtils.defineLazyGetter(this, "gCustomizeMode", function() {
  let scope = {};
  Cu.import("resource:///modules/CustomizeMode.jsm", scope);
  return new scope.CustomizeMode(window);
});

#ifdef MOZ_SERVICES_SYNC
XPCOMUtils.defineLazyModuleGetter(this, "Weave",
  "resource://services-sync/main.js");
#endif

XPCOMUtils.defineLazyGetter(this, "PopupNotifications", function () {
  let tmp = {};
  Cu.import("resource://gre/modules/PopupNotifications.jsm", tmp);
  try {
    return new tmp.PopupNotifications(gBrowser,
                                      document.getElementById("notification-popup"),
                                      document.getElementById("notification-popup-box"));
  } catch (ex) {
    Cu.reportError(ex);
    return null;
  }
});

XPCOMUtils.defineLazyGetter(this, "DeveloperToolbar", function() {
  let tmp = {};
  Cu.import("resource:///modules/devtools/DeveloperToolbar.jsm", tmp);
  return new tmp.DeveloperToolbar(window, document.getElementById("developer-toolbar"));
});

XPCOMUtils.defineLazyGetter(this, "BrowserToolboxProcess", function() {
  let tmp = {};
  Cu.import("resource:///modules/devtools/ToolboxProcess.jsm", tmp);
  return tmp.BrowserToolboxProcess;
});

XPCOMUtils.defineLazyModuleGetter(this, "Social",
  "resource:///modules/Social.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "PageThumbs",
  "resource://gre/modules/PageThumbs.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "ProcessHangMonitor",
  "resource:///modules/ProcessHangMonitor.jsm");

#ifdef MOZ_SAFE_BROWSING
XPCOMUtils.defineLazyModuleGetter(this, "SafeBrowsing",
  "resource://gre/modules/SafeBrowsing.jsm");
#endif

XPCOMUtils.defineLazyModuleGetter(this, "gCustomizationTabPreloader",
  "resource:///modules/CustomizationTabPreloader.jsm", "CustomizationTabPreloader");

XPCOMUtils.defineLazyModuleGetter(this, "PrivateBrowsingUtils",
  "resource://gre/modules/PrivateBrowsingUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Translation",
  "resource:///modules/translation/Translation.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "SitePermissions",
  "resource:///modules/SitePermissions.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "SessionStore",
  "resource:///modules/sessionstore/SessionStore.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "TabState",
  "resource:///modules/sessionstore/TabState.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "fxAccounts",
  "resource://gre/modules/FxAccounts.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "gWebRTCUI",
  "resource:///modules/webrtcUI.jsm", "webrtcUI");

#ifdef MOZ_CRASHREPORTER
XPCOMUtils.defineLazyModuleGetter(this, "TabCrashReporter",
  "resource:///modules/ContentCrashReporters.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PluginCrashReporter",
  "resource:///modules/ContentCrashReporters.jsm");
#endif

XPCOMUtils.defineLazyModuleGetter(this, "FormValidationHandler",
  "resource:///modules/FormValidationHandler.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "UITour",
  "resource:///modules/UITour.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "CastingApps",
  "resource:///modules/CastingApps.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "SimpleServiceDiscovery",
  "resource://gre/modules/SimpleServiceDiscovery.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "ReaderMode",
  "resource://gre/modules/ReaderMode.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "ReaderParent",
  "resource:///modules/ReaderParent.jsm");

let gInitialPages = [
  "about:blank",
  "about:newtab",
  "about:home",
  "about:privatebrowsing",
  "about:welcomeback",
  "about:sessionrestore"
];

#include browser-addons.js
#include browser-ctrlTab.js
#include browser-customization.js
#include browser-devedition.js
#include browser-eme.js
#include browser-feeds.js
#include browser-fullScreen.js
#include browser-fullZoom.js
#include browser-gestureSupport.js
#include browser-loop.js
#include browser-places.js
#include browser-plugins.js
#include browser-readinglist.js
#include browser-safebrowsing.js
#include browser-sidebar.js
#include browser-social.js
#include browser-tabview.js
#include browser-thumbnails.js

#ifdef MOZ_DATA_REPORTING
#include browser-data-submission-info-bar.js
#endif

#ifdef MOZ_SERVICES_SYNC
#include browser-syncui.js
#endif

#include browser-fxaccounts.js

XPCOMUtils.defineLazyGetter(this, "Win7Features", function () {
#ifdef XP_WIN
  
  if (gMultiProcessBrowser)
    return null;

  const WINTASKBAR_CONTRACTID = "@mozilla.org/windows-taskbar;1";
  if (WINTASKBAR_CONTRACTID in Cc &&
      Cc[WINTASKBAR_CONTRACTID].getService(Ci.nsIWinTaskbar).available) {
    let AeroPeek = Cu.import("resource:///modules/WindowsPreviewPerTab.jsm", {}).AeroPeek;
    return {
      onOpenWindow: function () {
        AeroPeek.onOpenWindow(window);
      },
      onCloseWindow: function () {
        AeroPeek.onCloseWindow(window);
      }
    };
  }
#endif
  return null;
});

#ifdef MOZ_CRASHREPORTER
XPCOMUtils.defineLazyServiceGetter(this, "gCrashReporter",
                                   "@mozilla.org/xre/app-info;1",
                                   "nsICrashReporter");
#endif

XPCOMUtils.defineLazyGetter(this, "PageMenuParent", function() {
  let tmp = {};
  Cu.import("resource://gre/modules/PageMenu.jsm", tmp);
  return new tmp.PageMenuParent();
});

function* browserWindows() {
  let windows = Services.wm.getEnumerator("navigator:browser");
  while (windows.hasMoreElements())
    yield windows.getNext();
}





function pageShowEventHandlers(persisted) {
  XULBrowserWindow.asyncUpdateUI();
}

function UpdateBackForwardCommands(aWebNavigation) {
  var backBroadcaster = document.getElementById("Browser:Back");
  var forwardBroadcaster = document.getElementById("Browser:Forward");

  
  
  
  

  var backDisabled = backBroadcaster.hasAttribute("disabled");
  var forwardDisabled = forwardBroadcaster.hasAttribute("disabled");
  if (backDisabled == aWebNavigation.canGoBack) {
    if (backDisabled)
      backBroadcaster.removeAttribute("disabled");
    else
      backBroadcaster.setAttribute("disabled", true);
  }

  if (forwardDisabled == aWebNavigation.canGoForward) {
    if (forwardDisabled)
      forwardBroadcaster.removeAttribute("disabled");
    else
      forwardBroadcaster.setAttribute("disabled", true);
  }
}





function SetClickAndHoldHandlers() {
  var timer;

  function openMenu(aButton) {
    cancelHold(aButton);
    aButton.firstChild.hidden = false;
    aButton.open = true;
  }

  function mousedownHandler(aEvent) {
    if (aEvent.button != 0 ||
        aEvent.currentTarget.open ||
        aEvent.currentTarget.disabled)
      return;

    
    aEvent.currentTarget.firstChild.hidden = true;

    aEvent.currentTarget.addEventListener("mouseout", mouseoutHandler, false);
    aEvent.currentTarget.addEventListener("mouseup", mouseupHandler, false);
    timer = setTimeout(openMenu, 500, aEvent.currentTarget);
  }

  function mouseoutHandler(aEvent) {
    let buttonRect = aEvent.currentTarget.getBoundingClientRect();
    if (aEvent.clientX >= buttonRect.left &&
        aEvent.clientX <= buttonRect.right &&
        aEvent.clientY >= buttonRect.bottom)
      openMenu(aEvent.currentTarget);
    else
      cancelHold(aEvent.currentTarget);
  }

  function mouseupHandler(aEvent) {
    cancelHold(aEvent.currentTarget);
  }

  function cancelHold(aButton) {
    clearTimeout(timer);
    aButton.removeEventListener("mouseout", mouseoutHandler, false);
    aButton.removeEventListener("mouseup", mouseupHandler, false);
  }

  function clickHandler(aEvent) {
    if (aEvent.button == 0 &&
        aEvent.target == aEvent.currentTarget &&
        !aEvent.currentTarget.open &&
        !aEvent.currentTarget.disabled) {
      let cmdEvent = document.createEvent("xulcommandevent");
      cmdEvent.initCommandEvent("command", true, true, window, 0,
                                aEvent.ctrlKey, aEvent.altKey, aEvent.shiftKey,
                                aEvent.metaKey, null);
      aEvent.currentTarget.dispatchEvent(cmdEvent);
    }
  }

  function _addClickAndHoldListenersOnElement(aElm) {
    aElm.addEventListener("mousedown", mousedownHandler, true);
    aElm.addEventListener("click", clickHandler, true);
  }

  
  let popup = document.getElementById("backForwardMenu").cloneNode(true);
  popup.removeAttribute("id");
  
  popup.setAttribute("context", "");

  let backButton = document.getElementById("back-button");
  backButton.setAttribute("type", "menu");
  backButton.appendChild(popup);
  _addClickAndHoldListenersOnElement(backButton);

  let forwardButton = document.getElementById("forward-button");
  popup = popup.cloneNode(true);
  forwardButton.setAttribute("type", "menu");
  forwardButton.appendChild(popup);
  _addClickAndHoldListenersOnElement(forwardButton);
}

const gSessionHistoryObserver = {
  observe: function(subject, topic, data)
  {
    if (topic != "browser:purge-session-history")
      return;

    var backCommand = document.getElementById("Browser:Back");
    backCommand.setAttribute("disabled", "true");
    var fwdCommand = document.getElementById("Browser:Forward");
    fwdCommand.setAttribute("disabled", "true");

    
    window.messageManager.broadcastAsyncMessage("Browser:HideSessionRestoreButton");

    if (gURLBar) {
      
      gURLBar.editor.transactionManager.clear()
    }
  }
};













function findChildShell(aDocument, aDocShell, aSoughtURI) {
  aDocShell.QueryInterface(Components.interfaces.nsIWebNavigation);
  aDocShell.QueryInterface(Components.interfaces.nsIInterfaceRequestor);
  var doc = aDocShell.getInterface(Components.interfaces.nsIDOMDocument);
  if ((aDocument && doc == aDocument) ||
      (aSoughtURI && aSoughtURI.spec == aDocShell.currentURI.spec))
    return aDocShell;

  var node = aDocShell.QueryInterface(Components.interfaces.nsIDocShellTreeItem);
  for (var i = 0; i < node.childCount; ++i) {
    var docShell = node.getChildAt(i);
    docShell = findChildShell(aDocument, docShell, aSoughtURI);
    if (docShell)
      return docShell;
  }
  return null;
}

var gPopupBlockerObserver = {
  _reportButton: null,

  onReportButtonClick: function (aEvent)
  {
    if (aEvent.button != 0 || aEvent.target != this._reportButton)
      return;

    document.getElementById("blockedPopupOptions")
            .openPopup(this._reportButton, "after_end", 0, 2, false, false, aEvent);
  },

  handleEvent: function (aEvent)
  {
    if (aEvent.originalTarget != gBrowser.selectedBrowser)
      return;

    if (!this._reportButton && gURLBar)
      this._reportButton = document.getElementById("page-report-button");

    if (!gBrowser.selectedBrowser.blockedPopups) {
      
      if (gURLBar)
        this._reportButton.hidden = true;

      
      var notificationBox = gBrowser.getNotificationBox();
      var notification = notificationBox.getNotificationWithValue("popup-blocked");
      if (notification) {
        notificationBox.removeNotification(notification, false);
      }
      return;
    }

    if (gURLBar)
      this._reportButton.hidden = false;

    
    
    
    if (!gBrowser.selectedBrowser.blockedPopups.reported) {
      if (gPrefService.getBoolPref("privacy.popups.showBrowserMessage")) {
        var brandBundle = document.getElementById("bundle_brand");
        var brandShortName = brandBundle.getString("brandShortName");
        var popupCount = gBrowser.selectedBrowser.blockedPopups.length;
#ifdef XP_WIN
        var popupButtonText = gNavigatorBundle.getString("popupWarningButton");
        var popupButtonAccesskey = gNavigatorBundle.getString("popupWarningButton.accesskey");
#else
        var popupButtonText = gNavigatorBundle.getString("popupWarningButtonUnix");
        var popupButtonAccesskey = gNavigatorBundle.getString("popupWarningButtonUnix.accesskey");
#endif
        var messageBase = gNavigatorBundle.getString("popupWarning.message");
        var message = PluralForm.get(popupCount, messageBase)
                                .replace("#1", brandShortName)
                                .replace("#2", popupCount);

        var notificationBox = gBrowser.getNotificationBox();
        var notification = notificationBox.getNotificationWithValue("popup-blocked");
        if (notification) {
          notification.label = message;
        }
        else {
          var buttons = [{
            label: popupButtonText,
            accessKey: popupButtonAccesskey,
            popup: "blockedPopupOptions",
            callback: null
          }];

          const priority = notificationBox.PRIORITY_WARNING_MEDIUM;
          notificationBox.appendNotification(message, "popup-blocked",
                                             "chrome://browser/skin/Info.png",
                                             priority, buttons);
        }
      }

      
      
      gBrowser.selectedBrowser.blockedPopups.reported = true;
    }
  },

  toggleAllowPopupsForSite: function (aEvent)
  {
    var pm = Services.perms;
    var shouldBlock = aEvent.target.getAttribute("block") == "true";
    var perm = shouldBlock ? pm.DENY_ACTION : pm.ALLOW_ACTION;
    pm.add(gBrowser.currentURI, "popup", perm);

    if (!shouldBlock)
      this.showAllBlockedPopups(gBrowser.selectedBrowser);

    gBrowser.getNotificationBox().removeCurrentNotification();
  },

  fillPopupList: function (aEvent)
  {
    
    
    
    
    
    
    
    
    
    let browser = gBrowser.selectedBrowser;
    var uri = browser.currentURI;
    var blockedPopupAllowSite = document.getElementById("blockedPopupAllowSite");
    try {
      blockedPopupAllowSite.removeAttribute("hidden");

      var pm = Services.perms;
      if (pm.testPermission(uri, "popup") == pm.ALLOW_ACTION) {
        
        
        let blockString = gNavigatorBundle.getFormattedString("popupBlock", [uri.host || uri.spec]);
        blockedPopupAllowSite.setAttribute("label", blockString);
        blockedPopupAllowSite.setAttribute("block", "true");
      }
      else {
        
        let allowString = gNavigatorBundle.getFormattedString("popupAllow", [uri.host || uri.spec]);
        blockedPopupAllowSite.setAttribute("label", allowString);
        blockedPopupAllowSite.removeAttribute("block");
      }
    }
    catch (e) {
      blockedPopupAllowSite.setAttribute("hidden", "true");
    }

    if (PrivateBrowsingUtils.isWindowPrivate(window))
      blockedPopupAllowSite.setAttribute("disabled", "true");
    else
      blockedPopupAllowSite.removeAttribute("disabled");

    var foundUsablePopupURI = false;
    var blockedPopups = browser.blockedPopups;
    if (blockedPopups) {
      for (let i = 0; i < blockedPopups.length; i++) {
        let blockedPopup = blockedPopups[i];

        
        
        if (!blockedPopup.popupWindowURI)
          continue;

        var popupURIspec = blockedPopup.popupWindowURI.spec;

        
        
        
        
        
        if (popupURIspec == "" || popupURIspec == "about:blank" ||
            popupURIspec == uri.spec)
          continue;

        
        
        
        
        
        foundUsablePopupURI = true;

        var menuitem = document.createElement("menuitem");
        var label = gNavigatorBundle.getFormattedString("popupShowPopupPrefix",
                                                        [popupURIspec]);
        menuitem.setAttribute("label", label);
        menuitem.setAttribute("oncommand", "gPopupBlockerObserver.showBlockedPopup(event);");
        menuitem.setAttribute("popupReportIndex", i);
        menuitem.popupReportBrowser = browser;
        aEvent.target.appendChild(menuitem);
      }
    }

    
    
    var blockedPopupsSeparator =
      document.getElementById("blockedPopupsSeparator");
    if (foundUsablePopupURI)
      blockedPopupsSeparator.removeAttribute("hidden");
    else
      blockedPopupsSeparator.setAttribute("hidden", true);

    var blockedPopupDontShowMessage = document.getElementById("blockedPopupDontShowMessage");
    var showMessage = gPrefService.getBoolPref("privacy.popups.showBrowserMessage");
    blockedPopupDontShowMessage.setAttribute("checked", !showMessage);
    if (aEvent.target.anchorNode.id == "page-report-button") {
      aEvent.target.anchorNode.setAttribute("open", "true");
      blockedPopupDontShowMessage.setAttribute("label", gNavigatorBundle.getString("popupWarningDontShowFromLocationbar"));
    } else
      blockedPopupDontShowMessage.setAttribute("label", gNavigatorBundle.getString("popupWarningDontShowFromMessage"));
  },

  onPopupHiding: function (aEvent) {
    if (aEvent.target.anchorNode.id == "page-report-button")
      aEvent.target.anchorNode.removeAttribute("open");

    let item = aEvent.target.lastChild;
    while (item && item.getAttribute("observes") != "blockedPopupsSeparator") {
      let next = item.previousSibling;
      item.parentNode.removeChild(item);
      item = next;
    }
  },

  showBlockedPopup: function (aEvent)
  {
    var target = aEvent.target;
    var popupReportIndex = target.getAttribute("popupReportIndex");
    let browser = target.popupReportBrowser;
    browser.unblockPopup(popupReportIndex);
  },

  showAllBlockedPopups: function (aBrowser)
  {
    let popups = aBrowser.blockedPopups;
    if (!popups)
      return;

    for (let i = 0; i < popups.length; i++) {
      if (popups[i].popupWindowURI)
        aBrowser.unblockPopup(i);
    }
  },

  editPopupSettings: function ()
  {
    var host = "";
    try {
      host = gBrowser.currentURI.host;
    }
    catch (e) { }

    var bundlePreferences = document.getElementById("bundle_preferences");
    var params = { blockVisible   : false,
                   sessionVisible : false,
                   allowVisible   : true,
                   prefilledHost  : host,
                   permissionType : "popup",
                   windowTitle    : bundlePreferences.getString("popuppermissionstitle"),
                   introText      : bundlePreferences.getString("popuppermissionstext") };
    var existingWindow = Services.wm.getMostRecentWindow("Browser:Permissions");
    if (existingWindow) {
      existingWindow.initWithParams(params);
      existingWindow.focus();
    }
    else
      window.openDialog("chrome://browser/content/preferences/permissions.xul",
                        "_blank", "resizable,dialog=no,centerscreen", params);
  },

  dontShowMessage: function ()
  {
    var showMessage = gPrefService.getBoolPref("privacy.popups.showBrowserMessage");
    gPrefService.setBoolPref("privacy.popups.showBrowserMessage", !showMessage);
    gBrowser.getNotificationBox().removeCurrentNotification();
  }
};

function gKeywordURIFixup({ target: browser, data: fixupInfo }) {
  let deserializeURI = (spec) => spec ? makeURI(spec) : null;

  
  
  
  let alternativeURI = deserializeURI(fixupInfo.fixedURI);
  if (!fixupInfo.keywordProviderName  || !alternativeURI || !alternativeURI.host) {
    return;
  }

  
  
  
  
  
  
  
  
  let previousURI = browser.currentURI;
  let preferredURI = deserializeURI(fixupInfo.preferredURI);

  
  
  let weakBrowser = Cu.getWeakReference(browser);
  browser = null;

  
  let hostName = alternativeURI.host;
  
  let asciiHost = alternativeURI.asciiHost;
  
  
  
  
  if (asciiHost.indexOf('.') == asciiHost.length - 1) {
    asciiHost = asciiHost.slice(0, -1);
  }

  
  if (/^\d+$/.test(asciiHost))
    return;

  let onLookupComplete = (request, record, status) => {
    let browser = weakBrowser.get();
    if (!Components.isSuccessCode(status) || !browser)
      return;

    let currentURI = browser.currentURI;
    
    if (!currentURI.equals(previousURI) &&
        !currentURI.equals(preferredURI)) {
      return;
    }

    
    let notificationBox = gBrowser.getNotificationBox(browser);
    if (notificationBox.getNotificationWithValue("keyword-uri-fixup"))
      return;

    let message = gNavigatorBundle.getFormattedString(
      "keywordURIFixup.message", [hostName]);
    let yesMessage = gNavigatorBundle.getFormattedString(
      "keywordURIFixup.goTo", [hostName])

    let buttons = [
      {
        label: yesMessage,
        accessKey: gNavigatorBundle.getString("keywordURIFixup.goTo.accesskey"),
        callback: function() {
          
          if (!PrivateBrowsingUtils.isWindowPrivate(window)) {
            let pref = "browser.fixup.domainwhitelist." + asciiHost;
            Services.prefs.setBoolPref(pref, true);
          }
          openUILinkIn(alternativeURI.spec, "current");
        }
      },
      {
        label: gNavigatorBundle.getString("keywordURIFixup.dismiss"),
        accessKey: gNavigatorBundle.getString("keywordURIFixup.dismiss.accesskey"),
        callback: function() {
          let notification = notificationBox.getNotificationWithValue("keyword-uri-fixup");
          notificationBox.removeNotification(notification, true);
        }
      }
    ];
    let notification =
      notificationBox.appendNotification(message,"keyword-uri-fixup", null,
                                         notificationBox.PRIORITY_INFO_HIGH,
                                         buttons);
    notification.persistence = 1;
  };

  try {
    gDNSService.asyncResolve(hostName, 0, onLookupComplete, Services.tm.mainThread);
  } catch (ex) {
    
    if (ex.result != Cr.NS_ERROR_UNKNOWN_HOST) {
      
      Cu.reportError(ex);
    }
  }
}



function _loadURIWithFlags(browser, uri, params) {
  if (!uri) {
    uri = "about:blank";
  }
  let flags = params.flags || 0;
  let referrer = params.referrerURI;
  let referrerPolicy = ('referrerPolicy' in params ? params.referrerPolicy :
                        Ci.nsIHttpChannel.REFERRER_POLICY_DEFAULT);
  let charset = params.charset;
  let postdata = params.postData;

  if (!(flags & browser.webNavigation.LOAD_FLAGS_FROM_EXTERNAL)) {
    browser.userTypedClear++;
  }

  let process = browser.isRemoteBrowser ? Ci.nsIXULRuntime.PROCESS_TYPE_CONTENT
                                        : Ci.nsIXULRuntime.PROCESS_TYPE_DEFAULT;
  let mustChangeProcess = gMultiProcessBrowser &&
                          !E10SUtils.canLoadURIInProcess(uri, process);
  try {
    if (!mustChangeProcess) {
      browser.webNavigation.loadURIWithOptions(uri, flags,
                                               referrer, referrerPolicy,
                                               postdata, null, null);
    } else {
      LoadInOtherProcess(browser, {
        uri: uri,
        flags: flags,
        referrer: referrer ? referrer.spec : null,
        referrerPolicy: referrerPolicy,
      });
    }
  } catch (e) {
    
    
    
    
    gBrowser.updateBrowserRemotenessByURL(browser, uri);
    browser.webNavigation.loadURIWithOptions(uri, flags, referrer, referrerPolicy,
                                             postdata, null, null);
  } finally {
    if (browser.userTypedClear) {
      browser.userTypedClear--;
    }
  }
}



function LoadInOtherProcess(browser, loadOptions, historyIndex = -1) {
  let tab = gBrowser.getTabForBrowser(browser);
  
  TabState.flush(browser);
  let tabState = JSON.parse(SessionStore.getTabState(tab));

  if (historyIndex < 0) {
    tabState.userTypedValue = null;
    
    SessionStore._restoreTabAndLoad(tab, JSON.stringify(tabState), loadOptions);
  }
  else {
    
    tabState.index = historyIndex + 1;
    
    
    SessionStore.setTabState(tab, JSON.stringify(tabState));
  }
}



function RedirectLoad({ target: browser, data }) {
  
  
  if (gBrowserInit.delayedStartupFinished) {
    LoadInOtherProcess(browser, data.loadOptions, data.historyIndex);
  } else {
    let delayedStartupFinished = (subject, topic) => {
      if (topic == "browser-delayed-startup-finished" &&
          subject == window) {
        Services.obs.removeObserver(delayedStartupFinished, topic);
        LoadInOtherProcess(browser, data.loadOptions, data.historyIndex);
      }
    };
    Services.obs.addObserver(delayedStartupFinished,
                             "browser-delayed-startup-finished",
                             false);
  }
}

var gBrowserInit = {
  delayedStartupFinished: false,

  onLoad: function() {
    gBrowser.addEventListener("DOMUpdatePageReport", gPopupBlockerObserver, false);

    Services.obs.addObserver(gPluginHandler.NPAPIPluginCrashed, "plugin-crashed", false);

    window.addEventListener("AppCommand", HandleAppCommandEvent, true);

    
    
    
    
    DOMLinkHandler.init();
    gPageStyleMenu.init();
    LanguageDetectionListener.init();
    BrowserOnClick.init();
    DevEdition.init();
    AboutPrivateBrowsingListener.init();

    let mm = window.getGroupMessageManager("browsers");
    mm.loadFrameScript("chrome://browser/content/tab-content.js", true);
    mm.loadFrameScript("chrome://browser/content/content.js", true);
    mm.loadFrameScript("chrome://browser/content/content-UITour.js", true);
    mm.loadFrameScript("chrome://global/content/manifestMessages.js", true);

    window.messageManager.addMessageListener("Browser:LoadURI", RedirectLoad);

    
    
    XULBrowserWindow.init();
    window.QueryInterface(Ci.nsIInterfaceRequestor)
          .getInterface(nsIWebNavigation)
          .QueryInterface(Ci.nsIDocShellTreeItem).treeOwner
          .QueryInterface(Ci.nsIInterfaceRequestor)
          .getInterface(Ci.nsIXULWindow)
          .XULBrowserWindow = window.XULBrowserWindow;
    window.QueryInterface(Ci.nsIDOMChromeWindow).browserDOMWindow =
      new nsBrowserAccess();

    if (!gMultiProcessBrowser) {
      
      Cc["@mozilla.org/eventlistenerservice;1"]
        .getService(Ci.nsIEventListenerService)
        .addSystemEventListener(gBrowser, "click", contentAreaClick, true);
    }

    
    gBrowser.addProgressListener(window.XULBrowserWindow);
    gBrowser.addTabsProgressListener(window.TabsProgressListener);

    
    gGestureSupport.init(true);

    
    gHistorySwipeAnimation.init();

    SidebarUI.init();

    
    
    
    
    Services.obs.notifyObservers(window, "browser-window-before-show", "");

    
    if (!document.documentElement.hasAttribute("width")) {
      let defaultWidth;
      let defaultHeight;

      
      
      
      
      
      if (screen.availHeight <= 600) {
        document.documentElement.setAttribute("sizemode", "maximized");
        defaultWidth = 610;
        defaultHeight = 450;
      }
      else {
        if (screen.availWidth <= screen.availHeight) {
          defaultWidth = screen.availWidth * .9;
          defaultHeight = screen.availHeight * .75;
        }
        else if (screen.availWidth >= 2048) {
          defaultWidth = (screen.availWidth / 2) - 20;
          defaultHeight = screen.availHeight - 10;
        }
        else {
          defaultWidth = screen.availWidth * .75;
          defaultHeight = screen.availHeight * .75;
        }

#if MOZ_WIDGET_GTK == 2
        
        
        defaultHeight -= 28;
#endif
      }
      document.documentElement.setAttribute("width", defaultWidth);
      document.documentElement.setAttribute("height", defaultHeight);
    }

    if (!window.toolbar.visible) {
      
      if (gURLBar) {
        gURLBar.setAttribute("readonly", "true");
        gURLBar.setAttribute("enablehistory", "false");
      }
      goSetCommandEnabled("cmd_newNavigatorTab", false);
    }

    
    CombinedStopReload.init();
    gPrivateBrowsingUI.init();
    TabsInTitlebar.init();

#ifdef XP_WIN
    if (window.matchMedia("(-moz-os-version: windows-win8)").matches &&
        window.matchMedia("(-moz-windows-default-theme)").matches) {
      let windowFrameColor = Cu.import("resource:///modules/Windows8WindowFrameColor.jsm", {})
                               .Windows8WindowFrameColor.get();

      
      
      windowFrameColor = windowFrameColor.map((color) => {
        if (color <= 10) {
          return color / 255 / 12.92;
        }
        return Math.pow(((color / 255) + 0.055) / 1.055, 2.4);
      });
      let backgroundLuminance = windowFrameColor[0] * 0.2126 +
                                windowFrameColor[1] * 0.7152 +
                                windowFrameColor[2] * 0.0722;
      let foregroundLuminance = 0; 
      let contrastRatio = (backgroundLuminance + 0.05) / (foregroundLuminance + 0.05);
      if (contrastRatio < 3) {
        document.documentElement.setAttribute("darkwindowframe", "true");
      }
    }
#endif

    ToolbarIconColor.init();

    
    this._boundDelayedStartup = this._delayedStartup.bind(this);
    window.addEventListener("MozAfterPaint", this._boundDelayedStartup);

    this._loadHandled = true;
  },

  _cancelDelayedStartup: function () {
    window.removeEventListener("MozAfterPaint", this._boundDelayedStartup);
    this._boundDelayedStartup = null;
  },

  _delayedStartup: function() {
    let tmp = {};
    Cu.import("resource://gre/modules/TelemetryTimestamps.jsm", tmp);
    let TelemetryTimestamps = tmp.TelemetryTimestamps;
    TelemetryTimestamps.add("delayedStartupStarted");

    this._cancelDelayedStartup();

    
    
    
    
    gBrowser.addEventListener("MozApplicationManifest",
                              OfflineApps, false);
    
    let socialBrowser = document.getElementById("social-sidebar-browser");
    socialBrowser.addEventListener("MozApplicationManifest",
                              OfflineApps, false);

    
    
    let mm = window.messageManager;
    mm.addMessageListener("PageVisibility:Show", function(message) {
      if (message.target == gBrowser.selectedBrowser) {
        setTimeout(pageShowEventHandlers, 0, message.data.persisted);
      }
    });

    gBrowser.addEventListener("AboutTabCrashedLoad", function(event) {
#ifdef MOZ_CRASHREPORTER
      TabCrashReporter.onAboutTabCrashedLoad(gBrowser.getBrowserForDocument(event.target));
#endif
    }, false, true);

    gBrowser.addEventListener("AboutTabCrashedMessage", function(event) {
      let ownerDoc = event.originalTarget;

      if (!ownerDoc.documentURI.startsWith("about:tabcrashed")) {
        return;
      }

      let isTopFrame = (ownerDoc.defaultView.parent === ownerDoc.defaultView);
      if (!isTopFrame) {
        return;
      }

      let browser = gBrowser.getBrowserForDocument(ownerDoc);
#ifdef MOZ_CRASHREPORTER
      if (event.detail.sendCrashReport) {
        TabCrashReporter.submitCrashReport(browser);
      }
#endif

      let tab = gBrowser.getTabForBrowser(browser);
      switch (event.detail.message) {
      case "closeTab":
        gBrowser.removeTab(tab, { animate: true });
        break;
      case "restoreTab":
        SessionStore.reviveCrashedTab(tab);
        break;
      case "restoreAll":
        for (let browserWin of browserWindows()) {
          for (let tab of window.gBrowser.tabs) {
            SessionStore.reviveCrashedTab(tab);
          }
        }
        break;
      }
    }, false, true);

    let uriToLoad = this._getUriToLoad();
    if (uriToLoad && uriToLoad != "about:blank") {
      if (uriToLoad instanceof Ci.nsISupportsArray) {
        let count = uriToLoad.Count();
        let specs = [];
        for (let i = 0; i < count; i++) {
          let urisstring = uriToLoad.GetElementAt(i).QueryInterface(Ci.nsISupportsString);
          specs.push(urisstring.data);
        }

        
        
        try {
          gBrowser.loadTabs(specs, false, true);
        } catch (e) {}
      }
      else if (uriToLoad instanceof XULElement) {
        
        
        let tabToOpen = uriToLoad;

        
        gBrowser.stop();
        
        gBrowser.docShell;

        
        
        
        try {
          if (tabToOpen.linkedBrowser.isRemoteBrowser) {
            if (!gMultiProcessBrowser) {
              throw new Error("Cannot drag a remote browser into a window " +
                              "without the remote tabs load context.");
            }

            gBrowser.updateBrowserRemoteness(gBrowser.selectedBrowser, true);
          }
          gBrowser.swapBrowsersAndCloseOther(gBrowser.selectedTab, tabToOpen);
        } catch(e) {
          Cu.reportError(e);
        }
      }
      
      
      
      
      else if (window.arguments.length >= 3) {
        let referrerURI = window.arguments[2];
        if (typeof(referrerURI) == "string") {
          try {
            referrerURI = makeURI(referrerURI);
          } catch (e) {
            referrerURI = null;
          }
        }
        let referrerPolicy = (window.arguments[5] != undefined ?
            window.arguments[5] : Ci.nsIHttpChannel.REFERRER_POLICY_DEFAULT);
        loadURI(uriToLoad, referrerURI, window.arguments[3] || null,
                window.arguments[4] || false, referrerPolicy);
        window.focus();
      }
      
      
      else {
        loadOneOrMoreURIs(uriToLoad);
      }
    }

#ifdef MOZ_SAFE_BROWSING
    
    setTimeout(function() { SafeBrowsing.init(); }, 2000);
#endif

    Services.obs.addObserver(gSessionHistoryObserver, "browser:purge-session-history", false);
    Services.obs.addObserver(gXPInstallObserver, "addon-install-disabled", false);
    Services.obs.addObserver(gXPInstallObserver, "addon-install-started", false);
    Services.obs.addObserver(gXPInstallObserver, "addon-install-blocked", false);
    Services.obs.addObserver(gXPInstallObserver, "addon-install-failed", false);
    Services.obs.addObserver(gXPInstallObserver, "addon-install-confirmation", false);
    Services.obs.addObserver(gXPInstallObserver, "addon-install-complete", false);
    window.messageManager.addMessageListener("Browser:URIFixup", gKeywordURIFixup);

    BrowserOffline.init();
    OfflineApps.init();
    IndexedDBPromptHelper.init();
#ifdef E10S_TESTING_ONLY
    gRemoteTabsUI.init();
#endif
    ReadingListUI.init();

    
    
    
    FullZoom.init();
    PanelUI.init();
    LightweightThemeListener.init();

    Services.telemetry.getHistogramById("E10S_WINDOW").add(gMultiProcessBrowser);

    SidebarUI.startDelayedLoad();

    UpdateUrlbarSearchSplitterState();

    if (!(isBlankPageURL(uriToLoad) || uriToLoad == "about:privatebrowsing") ||
        !focusAndSelectUrlBar()) {
      gBrowser.selectedBrowser.focus();
    }

    
    this._initializeSanitizer();

    
    gBrowser.tabContainer.updateVisibility();

    BookmarkingUI.init();

    gPrefService.addObserver(gHomeButton.prefDomain, gHomeButton, false);

    var homeButton = document.getElementById("home-button");
    gHomeButton.updateTooltip(homeButton);
    gHomeButton.updatePersonalToolbarStyle(homeButton);

    
    gBidiUI = isBidiEnabled();
    if (gBidiUI) {
      document.getElementById("documentDirection-separator").hidden = false;
      document.getElementById("documentDirection-swap").hidden = false;
      document.getElementById("textfieldDirection-separator").hidden = false;
      document.getElementById("textfieldDirection-swap").hidden = false;
    }

    
    
    if (!getBoolPref("ui.click_hold_context_menus", false))
      SetClickAndHoldHandlers();

    let NP = {};
    Cu.import("resource:///modules/NetworkPrioritizer.jsm", NP);
    NP.trackBrowserWindow(window);

    PlacesToolbarHelper.init();

    ctrlTab.readPref();
    gPrefService.addObserver(ctrlTab.prefName, ctrlTab, false);

    
    
    
    
    
    setTimeout(function() {
      try {
        Cu.import("resource:///modules/DownloadsCommon.jsm", {})
          .DownloadsCommon.initializeAllDataLinks();
        Cu.import("resource:///modules/DownloadsTaskbar.jsm", {})
          .DownloadsTaskbar.registerIndicator(window);
      } catch (ex) {
        Cu.reportError(ex);
      }
    }, 10000);

    
    
    
    
    setTimeout(function() {
      try {
        Services.logins;
      } catch (ex) {
        Cu.reportError(ex);
      }
    }, 3000);

    
    
    
    DownloadsButton.initializeIndicator();

#ifndef XP_MACOSX
    updateEditUIVisibility();
    let placesContext = document.getElementById("placesContext");
    placesContext.addEventListener("popupshowing", updateEditUIVisibility, false);
    placesContext.addEventListener("popuphiding", updateEditUIVisibility, false);
#endif

    gBrowser.mPanelContainer.addEventListener("InstallBrowserTheme", LightWeightThemeWebInstaller, false, true);
    gBrowser.mPanelContainer.addEventListener("PreviewBrowserTheme", LightWeightThemeWebInstaller, false, true);
    gBrowser.mPanelContainer.addEventListener("ResetBrowserThemePreview", LightWeightThemeWebInstaller, false, true);

    if (Win7Features)
      Win7Features.onOpenWindow();

    FullScreen.init();

#ifdef MOZ_SERVICES_SYNC
    
    gSyncUI.init();
    gFxAccounts.init();
#endif

#ifdef MOZ_DATA_REPORTING
    gDataNotificationInfoBar.init();
#endif

    LoopUI.init();

    gBrowserThumbnails.init();

    
    gDevToolsBrowser.registerBrowserWindow(window);

    gMenuButtonUpdateBadge.init();

    window.addEventListener("mousemove", MousePosTracker, false);
    window.addEventListener("dragover", MousePosTracker, false);

    gNavToolbox.addEventListener("customizationstarting", CustomizationHandler);
    gNavToolbox.addEventListener("customizationchange", CustomizationHandler);
    gNavToolbox.addEventListener("customizationending", CustomizationHandler);

    
    
    try {
      const startupCrashEndDelay = 30 * 1000;
      setTimeout(Services.startup.trackStartupCrashEnd, startupCrashEndDelay);
    } catch (ex) {
      Cu.reportError("Could not end startup crash tracking: " + ex);
    }

    if (typeof WindowsPrefSync !== 'undefined') {
      
      WindowsPrefSync.init();
    }

    
    setTimeout(() => {
      this.gmpInstallManager = new GMPInstallManager();
      
      
      this.gmpInstallManager.simpleCheckAndInstall().then(null, () => {});
    }, 1000 * 60);

    SessionStore.promiseInitialized.then(() => {
      
      if (window.closed) {
        return;
      }

      
      RestoreLastSessionObserver.init();

      SocialUI.init();
      TabView.init();

      
      
      setTimeout(() => {
        if (window.closed) {
          return;
        }
        let secmodDB = Cc["@mozilla.org/security/pkcs11moduledb;1"]
                       .getService(Ci.nsIPKCS11ModuleDB);
        let slot = secmodDB.findSlotByName("");
        let mpEnabled = slot &&
                        slot.status != Ci.nsIPKCS11Slot.SLOT_UNINITIALIZED &&
                        slot.status != Ci.nsIPKCS11Slot.SLOT_READY;
        if (mpEnabled) {
          Services.telemetry.getHistogramById("MASTER_PASSWORD_ENABLED").add(mpEnabled);
        }
      }, 5000);

      
      let tpEnabled = gPrefService
                      .getBoolPref("privacy.trackingprotection.enabled");
      Services.telemetry.getHistogramById("TRACKING_PROTECTION_ENABLED")
        .add(tpEnabled);

      PanicButtonNotifier.init();
    });
    this.delayedStartupFinished = true;

    Services.obs.notifyObservers(window, "browser-delayed-startup-finished", "");
    TelemetryTimestamps.add("delayedStartupFinished");
  },

  
  _getUriToLoad: function () {
    
    
    
    
    
    if (!window.arguments || !window.arguments[0])
      return null;

    let uri = window.arguments[0];
    let sessionStartup = Cc["@mozilla.org/browser/sessionstartup;1"]
                           .getService(Ci.nsISessionStartup);
    let defaultArgs = Cc["@mozilla.org/browser/clh;1"]
                        .getService(Ci.nsIBrowserHandler)
                        .defaultArgs;

    
    
    if (uri == defaultArgs && sessionStartup.willOverrideHomepage)
      return null;

    return uri;
  },

  onUnload: function() {
    
    
    
    if (!this._loadHandled)
      return;

    gDevToolsBrowser.forgetBrowserWindow(window);

    let desc = Object.getOwnPropertyDescriptor(window, "DeveloperToolbar");
    if (desc && !desc.get) {
      DeveloperToolbar.destroy();
    }

    
    

    CombinedStopReload.uninit();

    gGestureSupport.init(false);

    gHistorySwipeAnimation.uninit();

    FullScreen.uninit();

#ifdef MOZ_SERVICES_SYNC
    gFxAccounts.uninit();
#endif

    Services.obs.removeObserver(gPluginHandler.NPAPIPluginCrashed, "plugin-crashed");

    try {
      gBrowser.removeProgressListener(window.XULBrowserWindow);
      gBrowser.removeTabsProgressListener(window.TabsProgressListener);
    } catch (ex) {
    }

    PlacesToolbarHelper.uninit();

    BookmarkingUI.uninit();

    TabsInTitlebar.uninit();

    ToolbarIconColor.uninit();

    BrowserOnClick.uninit();

    DevEdition.uninit();

    gMenuButtonUpdateBadge.uninit();

    ReadingListUI.uninit();

    SidebarUI.uninit();

    
    
    if (this._boundDelayedStartup) {
      this._cancelDelayedStartup();
    } else {
      if (Win7Features)
        Win7Features.onCloseWindow();

      gPrefService.removeObserver(ctrlTab.prefName, ctrlTab);
      ctrlTab.uninit();
      TabView.uninit();
      SocialUI.uninit();
      gBrowserThumbnails.uninit();
      LoopUI.uninit();
      FullZoom.destroy();

      Services.obs.removeObserver(gSessionHistoryObserver, "browser:purge-session-history");
      Services.obs.removeObserver(gXPInstallObserver, "addon-install-disabled");
      Services.obs.removeObserver(gXPInstallObserver, "addon-install-started");
      Services.obs.removeObserver(gXPInstallObserver, "addon-install-blocked");
      Services.obs.removeObserver(gXPInstallObserver, "addon-install-failed");
      Services.obs.removeObserver(gXPInstallObserver, "addon-install-confirmation");
      Services.obs.removeObserver(gXPInstallObserver, "addon-install-complete");
      window.messageManager.removeMessageListener("Browser:URIFixup", gKeywordURIFixup);
      window.messageManager.removeMessageListener("Browser:LoadURI", RedirectLoad);

      try {
        gPrefService.removeObserver(gHomeButton.prefDomain, gHomeButton);
      } catch (ex) {
        Cu.reportError(ex);
      }

      if (typeof WindowsPrefSync !== 'undefined') {
        WindowsPrefSync.uninit();
      }
      if (this.gmpInstallManager) {
        this.gmpInstallManager.uninit();
      }

      BrowserOffline.uninit();
      OfflineApps.uninit();
      IndexedDBPromptHelper.uninit();
      LightweightThemeListener.uninit();
      PanelUI.uninit();
    }

    
    window.XULBrowserWindow = null;
    window.QueryInterface(Ci.nsIInterfaceRequestor)
          .getInterface(Ci.nsIWebNavigation)
          .QueryInterface(Ci.nsIDocShellTreeItem).treeOwner
          .QueryInterface(Ci.nsIInterfaceRequestor)
          .getInterface(Ci.nsIXULWindow)
          .XULBrowserWindow = null;
    window.QueryInterface(Ci.nsIDOMChromeWindow).browserDOMWindow = null;
  },

#ifdef XP_MACOSX
  
  
  
  nonBrowserWindowStartup: function() {
    
    var disabledItems = ['Browser:SavePage',
                         'Browser:SendLink', 'cmd_pageSetup', 'cmd_print', 'cmd_find', 'cmd_findAgain',
                         'viewToolbarsMenu', 'viewSidebarMenuMenu', 'Browser:Reload',
                         'viewFullZoomMenu', 'pageStyleMenu', 'charsetMenu', 'View:PageSource', 'View:FullScreen',
                         'viewHistorySidebar', 'Browser:AddBookmarkAs', 'Browser:BookmarkAllTabs',
                         'View:PageInfo', 'Browser:ToggleTabView'];
    var element;

    for (let disabledItem of disabledItems) {
      element = document.getElementById(disabledItem);
      if (element)
        element.setAttribute("disabled", "true");
    }

    
    
    if (window.location.href == "chrome://browser/content/hiddenWindow.xul") {
      var hiddenWindowDisabledItems = ['cmd_close', 'minimizeWindow', 'zoomWindow'];
      for (let hiddenWindowDisabledItem of hiddenWindowDisabledItems) {
        element = document.getElementById(hiddenWindowDisabledItem);
        if (element)
          element.setAttribute("disabled", "true");
      }

      
      element = document.getElementById("sep-window-list");
      element.setAttribute("hidden", "true");

      
      let dockMenuElement = document.getElementById("menu_mac_dockmenu");
      if (dockMenuElement != null) {
        let nativeMenu = Cc["@mozilla.org/widget/standalonenativemenu;1"]
                         .createInstance(Ci.nsIStandaloneNativeMenu);

        try {
          nativeMenu.init(dockMenuElement);

          let dockSupport = Cc["@mozilla.org/widget/macdocksupport;1"]
                            .getService(Ci.nsIMacDockSupport);
          dockSupport.dockMenu = nativeMenu;
        }
        catch (e) {
        }
      }
    }

    if (PrivateBrowsingUtils.permanentPrivateBrowsing) {
      document.getElementById("macDockMenuNewWindow").hidden = true;
    }

    this._delayedStartupTimeoutId = setTimeout(this.nonBrowserWindowDelayedStartup.bind(this), 0);
  },

  nonBrowserWindowDelayedStartup: function() {
    this._delayedStartupTimeoutId = null;

    
    BrowserOffline.init();

    
    this._initializeSanitizer();

    
    gPrivateBrowsingUI.init();

#ifdef MOZ_SERVICES_SYNC
    
    gSyncUI.init();
#endif

#ifdef E10S_TESTING_ONLY
    gRemoteTabsUI.init();
#endif
  },

  nonBrowserWindowShutdown: function() {
    
    
    if (this._delayedStartupTimeoutId) {
      clearTimeout(this._delayedStartupTimeoutId);
      return;
    }

    BrowserOffline.uninit();
  },
#endif

  _initializeSanitizer: function() {
    const kDidSanitizeDomain = "privacy.sanitize.didShutdownSanitize";
    if (gPrefService.prefHasUserValue(kDidSanitizeDomain)) {
      gPrefService.clearUserPref(kDidSanitizeDomain);
      
      
      gPrefService.savePrefFile(null);
    }

    





    if (!gPrefService.getBoolPref("privacy.sanitize.migrateFx3Prefs")) {
      let itemBranch = gPrefService.getBranch("privacy.item.");
      let itemArray = itemBranch.getChildList("");

      
      let doMigrate = itemArray.some(function (name) itemBranch.prefHasUserValue(name));
      
      if (!doMigrate)
        doMigrate = gPrefService.getBoolPref("privacy.sanitize.sanitizeOnShutdown");

      if (doMigrate) {
        let cpdBranch = gPrefService.getBranch("privacy.cpd.");
        let clearOnShutdownBranch = gPrefService.getBranch("privacy.clearOnShutdown.");
        for (let name of itemArray) {
          try {
            
            
            if (name != "passwords" && name != "offlineApps")
              cpdBranch.setBoolPref(name, itemBranch.getBoolPref(name));
            clearOnShutdownBranch.setBoolPref(name, itemBranch.getBoolPref(name));
          }
          catch(e) {
            Cu.reportError("Exception thrown during privacy pref migration: " + e);
          }
        }
      }

      gPrefService.setBoolPref("privacy.sanitize.migrateFx3Prefs", true);
    }
  },
}



var BrowserStartup        = gBrowserInit.onLoad.bind(gBrowserInit);
var BrowserShutdown       = gBrowserInit.onUnload.bind(gBrowserInit);
#ifdef XP_MACOSX
var nonBrowserWindowStartup        = gBrowserInit.nonBrowserWindowStartup.bind(gBrowserInit);
var nonBrowserWindowDelayedStartup = gBrowserInit.nonBrowserWindowDelayedStartup.bind(gBrowserInit);
var nonBrowserWindowShutdown       = gBrowserInit.nonBrowserWindowShutdown.bind(gBrowserInit);
#endif

function HandleAppCommandEvent(evt) {
  switch (evt.command) {
  case "Back":
    BrowserBack();
    break;
  case "Forward":
    BrowserForward();
    break;
  case "Reload":
    BrowserReloadSkipCache();
    break;
  case "Stop":
    if (XULBrowserWindow.stopCommand.getAttribute("disabled") != "true")
      BrowserStop();
    break;
  case "Search":
    BrowserSearch.webSearch();
    break;
  case "Bookmarks":
    SidebarUI.toggle("viewBookmarksSidebar");
    break;
  case "Home":
    BrowserHome();
    break;
  case "New":
    BrowserOpenTab();
    break;
  case "Close":
    BrowserCloseTabOrWindow();
    break;
  case "Find":
    gFindBar.onFindCommand();
    break;
  case "Help":
    openHelpLink('firefox-help');
    break;
  case "Open":
    BrowserOpenFileWindow();
    break;
  case "Print":
    PrintUtils.print(gBrowser.selectedBrowser.contentWindowAsCPOW,
                     gBrowser.selectedBrowser);
    break;
  case "Save":
    saveDocument(gBrowser.selectedBrowser.contentDocumentAsCPOW);
    break;
  case "SendMail":
    MailIntegration.sendLinkForBrowser(gBrowser.selectedBrowser);
    break;
  default:
    return;
  }
  evt.stopPropagation();
  evt.preventDefault();
}

function gotoHistoryIndex(aEvent) {
  let index = aEvent.target.getAttribute("index");
  if (!index)
    return false;

  let where = whereToOpenLink(aEvent);

  if (where == "current") {
    

    try {
      gBrowser.gotoIndex(index);
    }
    catch(ex) {
      return false;
    }
    return true;
  }
  

  duplicateTabIn(gBrowser.selectedTab, where, index - gBrowser.sessionHistory.index);
  return true;
}

function BrowserForward(aEvent) {
  let where = whereToOpenLink(aEvent, false, true);

  if (where == "current") {
    try {
      gBrowser.goForward();
    }
    catch(ex) {
    }
  }
  else {
    duplicateTabIn(gBrowser.selectedTab, where, 1);
  }
}

function BrowserBack(aEvent) {
  let where = whereToOpenLink(aEvent, false, true);

  if (where == "current") {
    try {
      gBrowser.goBack();
    }
    catch(ex) {
    }
  }
  else {
    duplicateTabIn(gBrowser.selectedTab, where, -1);
  }
}

function BrowserHandleBackspace()
{
  switch (gPrefService.getIntPref("browser.backspace_action")) {
  case 0:
    BrowserBack();
    break;
  case 1:
    goDoCommand("cmd_scrollPageUp");
    break;
  }
}

function BrowserHandleShiftBackspace()
{
  switch (gPrefService.getIntPref("browser.backspace_action")) {
  case 0:
    BrowserForward();
    break;
  case 1:
    goDoCommand("cmd_scrollPageDown");
    break;
  }
}

function BrowserStop() {
  const stopFlags = nsIWebNavigation.STOP_ALL;
  gBrowser.webNavigation.stop(stopFlags);
}

function BrowserReloadOrDuplicate(aEvent) {
  var backgroundTabModifier = aEvent.button == 1 ||
#ifdef XP_MACOSX
    aEvent.metaKey;
#else
    aEvent.ctrlKey;
#endif
  if (aEvent.shiftKey && !backgroundTabModifier) {
    BrowserReloadSkipCache();
    return;
  }

  let where = whereToOpenLink(aEvent, false, true);
  if (where == "current")
    BrowserReload();
  else
    duplicateTabIn(gBrowser.selectedTab, where);
}

function BrowserReload() {
  const reloadFlags = nsIWebNavigation.LOAD_FLAGS_NONE;
  BrowserReloadWithFlags(reloadFlags);
}

function BrowserReloadSkipCache() {
  
  const reloadFlags = nsIWebNavigation.LOAD_FLAGS_BYPASS_PROXY | nsIWebNavigation.LOAD_FLAGS_BYPASS_CACHE;
  BrowserReloadWithFlags(reloadFlags);
}

var BrowserHome = BrowserGoHome;
function BrowserGoHome(aEvent) {
  if (aEvent && "button" in aEvent &&
      aEvent.button == 2) 
    return;

  var homePage = gHomeButton.getHomePage();
  var where = whereToOpenLink(aEvent, false, true);
  var urls;

  
  if (where == "current" &&
      gBrowser &&
      gBrowser.selectedTab.pinned)
    where = "tab";

  
  switch (where) {
  case "current":
    loadOneOrMoreURIs(homePage);
    break;
  case "tabshifted":
  case "tab":
    urls = homePage.split("|");
    var loadInBackground = getBoolPref("browser.tabs.loadBookmarksInBackground", false);
    gBrowser.loadTabs(urls, loadInBackground);
    break;
  case "window":
    OpenBrowserWindow();
    break;
  }
}

function loadOneOrMoreURIs(aURIString)
{
#ifdef XP_MACOSX
  
  if (window.location.href != getBrowserURL())
  {
    window.openDialog(getBrowserURL(), "_blank", "all,dialog=no", aURIString);
    return;
  }
#endif
  
  
  try {
    gBrowser.loadTabs(aURIString.split("|"), false, true);
  }
  catch (e) {
  }
}

function focusAndSelectUrlBar() {
  if (gURLBar) {
    if (window.fullScreen)
      FullScreen.mouseoverToggle(true);

    gURLBar.select();
    if (document.activeElement == gURLBar.inputField)
      return true;
  }
  return false;
}

function openLocation() {
  if (focusAndSelectUrlBar())
    return;

#ifdef XP_MACOSX
  if (window.location.href != getBrowserURL()) {
    var win = getTopWin();
    if (win) {
      
      win.focus()
      win.openLocation();
    }
    else {
      
      window.openDialog("chrome://browser/content/", "_blank",
                        "chrome,all,dialog=no", BROWSER_NEW_TAB_URL);
    }
  }
#endif
}

function BrowserOpenTab()
{
  openUILinkIn(BROWSER_NEW_TAB_URL, "tab");
}





function delayedOpenWindow(chrome, flags, href, postData)
{
  
  
  
  
  
  setTimeout(function() { openDialog(chrome, "_blank", flags, href, null, null, postData); }, 10);
}



function delayedOpenTab(aUrl, aReferrer, aCharset, aPostData, aAllowThirdPartyFixup)
{
  gBrowser.loadOneTab(aUrl, {
                      referrerURI: aReferrer,
                      charset: aCharset,
                      postData: aPostData,
                      inBackground: false,
                      allowThirdPartyFixup: aAllowThirdPartyFixup});
}

var gLastOpenDirectory = {
  _lastDir: null,
  get path() {
    if (!this._lastDir || !this._lastDir.exists()) {
      try {
        this._lastDir = gPrefService.getComplexValue("browser.open.lastDir",
                                                     Ci.nsILocalFile);
        if (!this._lastDir.exists())
          this._lastDir = null;
      }
      catch(e) {}
    }
    return this._lastDir;
  },
  set path(val) {
    try {
      if (!val || !val.isDirectory())
        return;
    } catch(e) {
      return;
    }
    this._lastDir = val.clone();

    
    if (!PrivateBrowsingUtils.isWindowPrivate(window))
      gPrefService.setComplexValue("browser.open.lastDir", Ci.nsILocalFile,
                                   this._lastDir);
  },
  reset: function() {
    this._lastDir = null;
  }
};

function BrowserOpenFileWindow()
{
  
  try {
    const nsIFilePicker = Ci.nsIFilePicker;
    let fp = Cc["@mozilla.org/filepicker;1"].createInstance(nsIFilePicker);
    let fpCallback = function fpCallback_done(aResult) {
      if (aResult == nsIFilePicker.returnOK) {
        try {
          if (fp.file) {
            gLastOpenDirectory.path =
              fp.file.parent.QueryInterface(Ci.nsILocalFile);
          }
        } catch (ex) {
        }
        openUILinkIn(fp.fileURL.spec, "current");
      }
    };

    fp.init(window, gNavigatorBundle.getString("openFile"),
            nsIFilePicker.modeOpen);
    fp.appendFilters(nsIFilePicker.filterAll | nsIFilePicker.filterText |
                     nsIFilePicker.filterImages | nsIFilePicker.filterXML |
                     nsIFilePicker.filterHTML);
    fp.displayDirectory = gLastOpenDirectory.path;
    fp.open(fpCallback);
  } catch (ex) {
  }
}

function BrowserCloseTabOrWindow() {
#ifdef XP_MACOSX
  
  if (window.location.href != getBrowserURL()) {
    closeWindow(true);
    return;
  }
#endif

  
  gBrowser.removeCurrentTab({animate: true});
}

function BrowserTryToCloseWindow()
{
  if (WindowIsClosing())
    window.close();     
}

function loadURI(uri, referrer, postData, allowThirdPartyFixup, referrerPolicy) {
  try {
    openLinkIn(uri, "current",
               { referrerURI: referrer,
                 referrerPolicy: referrerPolicy,
                 postData: postData,
                 allowThirdPartyFixup: allowThirdPartyFixup });
  } catch (e) {}
}












function getShortcutOrURIAndPostData(url, callback = null) {
  if (callback) {
    Deprecated.warning("Please use the Promise returned by " +
                       "getShortcutOrURIAndPostData() instead of passing a " +
                       "callback",
                       "https://bugzilla.mozilla.org/show_bug.cgi?id=1100294");
  }

  return Task.spawn(function* () {
    let mayInheritPrincipal = false;
    let postData = null;
    let shortcutURL = null;
    let keyword = url;
    let param = "";

    let offset = url.indexOf(" ");
    if (offset > 0) {
      keyword = url.substr(0, offset);
      param = url.substr(offset + 1);
    }

    let engine = Services.search.getEngineByAlias(keyword);
    if (engine) {
      let submission = engine.getSubmission(param, null, "keyword");
      postData = submission.postData;
      return { postData: submission.postData, url: submission.uri.spec,
               mayInheritPrincipal };
    }

    let entry = yield PlacesUtils.keywords.fetch(keyword);
    if (entry) {
      shortcutURL = entry.url.href;
      postData = entry.postData;
    }

    if (!shortcutURL) {
      return { postData, url, mayInheritPrincipal };
    }

    let escapedPostData = "";
    if (postData)
      escapedPostData = unescape(postData);

    if (/%s/i.test(shortcutURL) || /%s/i.test(escapedPostData)) {
      let charset = "";
      const re = /^(.*)\&mozcharset=([a-zA-Z][_\-a-zA-Z0-9]+)\s*$/;
      let matches = shortcutURL.match(re);

      if (matches) {
        [, shortcutURL, charset] = matches;
      } else {
        let uri;
        try {
          
          uri = makeURI(shortcutURL);
        } catch (ex) {}

        if (uri) {
          
          
          charset = yield PlacesUtils.getCharsetForURI(uri);
        }
      }

      
      
      
      
      
      let encodedParam = "";
      if (charset && charset != "UTF-8")
        encodedParam = escape(convertFromUnicode(charset, param)).
                       replace(/[+@\/]+/g, encodeURIComponent);
      else 
        encodedParam = encodeURIComponent(param);

      shortcutURL = shortcutURL.replace(/%s/g, encodedParam).replace(/%S/g, param);

      if (/%s/i.test(escapedPostData)) 
        postData = getPostDataStream(escapedPostData, param, encodedParam,
                                               "application/x-www-form-urlencoded");

      
      
      mayInheritPrincipal = true;

      return { postData, url: shortcutURL, mayInheritPrincipal };
    }

    if (param) {
      
      
      postData = null;

      return { postData, url, mayInheritPrincipal };
    }

    
    
    mayInheritPrincipal = true;

    return { postData, url: shortcutURL, mayInheritPrincipal };
  }).then(data => {
    if (callback) {
      callback(data);
    }

    return data;
  });
}

function getPostDataStream(aStringData, aKeyword, aEncKeyword, aType) {
  var dataStream = Cc["@mozilla.org/io/string-input-stream;1"].
                   createInstance(Ci.nsIStringInputStream);
  aStringData = aStringData.replace(/%s/g, aEncKeyword).replace(/%S/g, aKeyword);
  dataStream.data = aStringData;

  var mimeStream = Cc["@mozilla.org/network/mime-input-stream;1"].
                   createInstance(Ci.nsIMIMEInputStream);
  mimeStream.addHeader("Content-Type", aType);
  mimeStream.addContentLength = true;
  mimeStream.setData(dataStream);
  return mimeStream.QueryInterface(Ci.nsIInputStream);
}

function getLoadContext() {
  return window.QueryInterface(Ci.nsIInterfaceRequestor)
               .getInterface(Ci.nsIWebNavigation)
               .QueryInterface(Ci.nsILoadContext);
}

function readFromClipboard()
{
  var url;

  try {
    
    var trans = Components.classes["@mozilla.org/widget/transferable;1"]
                          .createInstance(Components.interfaces.nsITransferable);
    trans.init(getLoadContext());

    trans.addDataFlavor("text/unicode");

    
    if (Services.clipboard.supportsSelectionClipboard())
      Services.clipboard.getData(trans, Services.clipboard.kSelectionClipboard);
    else
      Services.clipboard.getData(trans, Services.clipboard.kGlobalClipboard);

    var data = {};
    var dataLen = {};
    trans.getTransferData("text/unicode", data, dataLen);

    if (data) {
      data = data.value.QueryInterface(Components.interfaces.nsISupportsString);
      url = data.data.substring(0, dataLen.value / 2);
    }
  } catch (ex) {
  }

  return url;
}

function BrowserViewSourceOfDocument(aDocument)
{
  var pageCookie;
  var webNav;

  
  var docCharset = "charset=" + aDocument.characterSet;

  
  try {
      var win;
      var ifRequestor;

      
      
      
      
      win = aDocument.defaultView;
      if (win == window) {
        win = content;
      }
      ifRequestor = win.QueryInterface(Components.interfaces.nsIInterfaceRequestor);

      webNav = ifRequestor.getInterface(nsIWebNavigation);
  } catch(err) {
      
      
      webNav = gBrowser.webNavigation;
  }
  
  
  
  
  
  try {

#ifdef E10S_TESTING_ONLY
    
    
    
    
    
    
    if (!Cu.isCrossProcessWrapper(aDocument)) {
#endif
      var PageLoader = webNav.QueryInterface(Components.interfaces.nsIWebPageDescriptor);

      pageCookie = PageLoader.currentDescriptor;
#ifdef E10S_TESTING_ONLY
    }
#endif
  } catch(err) {
    
  }

  top.gViewSourceUtils.viewSource(webNav.currentURI.spec, pageCookie, aDocument);
}




function BrowserPageInfo(doc, initialTab, imageElement) {
  var args = {doc: doc, initialTab: initialTab, imageElement: imageElement};
  var windows = Services.wm.getEnumerator("Browser:page-info");

  var documentURL = doc ? doc.location : window.gBrowser.selectedBrowser.contentDocumentAsCPOW.location;

  
  while (windows.hasMoreElements()) {
    var currentWindow = windows.getNext();
    if (currentWindow.closed) {
      continue;
    }
    if (currentWindow.document.documentElement.getAttribute("relatedUrl") == documentURL) {
      currentWindow.focus();
      currentWindow.resetPageInfo(args);
      return currentWindow;
    }
  }

  
  return openDialog("chrome://browser/content/pageinfo/pageInfo.xul", "",
                    "chrome,toolbar,dialog=no,resizable", args);
}

function URLBarSetURI(aURI) {
  var value = gBrowser.userTypedValue;
  var valid = false;

  if (value == null) {
    let uri = aURI || gBrowser.currentURI;
    
    try {
      uri = Services.uriFixup.createExposableURI(uri);
    } catch (e) {}

    
    
    
    if (gInitialPages.indexOf(uri.spec) != -1)
      value = !gMultiProcessBrowser && content.opener ? uri.spec : "";
    else
      value = losslessDecodeURI(uri);

    valid = !isBlankPageURL(uri.spec);
  }

  gURLBar.value = value;
  gURLBar.valueIsTyped = !valid;
  SetPageProxyState(valid ? "valid" : "invalid");
}

function losslessDecodeURI(aURI) {
  var value = aURI.spec;
  if (aURI.schemeIs("moz-action"))
    throw new Error("losslessDecodeURI should never get a moz-action URI");
  
  if (!/%25(?:3B|2F|3F|3A|40|26|3D|2B|24|2C|23)/i.test(value))
    try {
      value = decodeURI(value)
                
                
                
                
                
                
                
                .replace(/%(?!3B|2F|3F|3A|40|26|3D|2B|24|2C|23)|[\r\n\t]/ig,
                         encodeURIComponent);
    } catch (e) {}

  
  
  
  value = value.replace(/[\u0000-\u001f\u007f-\u00a0\u2028\u2029\ufffc]/g,
                        encodeURIComponent);

  
  
  
  
  value = value.replace(/[\u00ad\u034f\u061c\u115f-\u1160\u17b4-\u17b5\u180b-\u180d\u200b\u200e-\u200f\u202a-\u202e\u2060-\u206f\u3164\ufe00-\ufe0f\ufeff\uffa0\ufff0-\ufff8]|\ud834[\udd73-\udd7a]|[\udb40-\udb43][\udc00-\udfff]/g,
                        encodeURIComponent);
  return value;
}

function UpdateUrlbarSearchSplitterState()
{
  var splitter = document.getElementById("urlbar-search-splitter");
  var urlbar = document.getElementById("urlbar-container");
  var searchbar = document.getElementById("search-container");

  if (document.documentElement.getAttribute("customizing") == "true") {
    if (splitter) {
      splitter.remove();
    }
    return;
  }

  
  if (splitter &&
      ((splitter.nextSibling == searchbar && splitter.previousSibling == urlbar) ||
       (splitter.nextSibling == urlbar && splitter.previousSibling == searchbar))) {
    return;
  }

  var ibefore = null;
  if (urlbar && searchbar) {
    if (urlbar.nextSibling == searchbar)
      ibefore = searchbar;
    else if (searchbar.nextSibling == urlbar)
      ibefore = urlbar;
  }

  if (ibefore) {
    if (!splitter) {
      splitter = document.createElement("splitter");
      splitter.id = "urlbar-search-splitter";
      splitter.setAttribute("resizebefore", "flex");
      splitter.setAttribute("resizeafter", "flex");
      splitter.setAttribute("skipintoolbarset", "true");
      splitter.setAttribute("overflows", "false");
      splitter.className = "chromeclass-toolbar-additional";
    }
    urlbar.parentNode.insertBefore(splitter, ibefore);
  } else if (splitter)
    splitter.parentNode.removeChild(splitter);
}

function UpdatePageProxyState()
{
  if (gURLBar && gURLBar.value != gLastValidURLStr)
    SetPageProxyState("invalid");
}

function SetPageProxyState(aState)
{
  BookmarkingUI.onPageProxyStateChanged(aState);
  ReadingListUI.onPageProxyStateChanged(aState);

  if (!gURLBar)
    return;

  if (!gProxyFavIcon)
    gProxyFavIcon = document.getElementById("page-proxy-favicon");

  gURLBar.setAttribute("pageproxystate", aState);
  gProxyFavIcon.setAttribute("pageproxystate", aState);

  
  
  if (aState == "valid") {
    gLastValidURLStr = gURLBar.value;
    gURLBar.addEventListener("input", UpdatePageProxyState, false);
  } else if (aState == "invalid") {
    gURLBar.removeEventListener("input", UpdatePageProxyState, false);
  }
}

function PageProxyClickHandler(aEvent)
{
  if (aEvent.button == 1 && gPrefService.getBoolPref("middlemouse.paste"))
    middleMousePaste(aEvent);
}


let gMenuButtonUpdateBadge = {
  enabled: false,

  init: function () {
    try {
      this.enabled = Services.prefs.getBoolPref("app.update.badge");
    } catch (e) {}
    if (this.enabled) {
      PanelUI.menuButton.classList.add("badged-button");
      Services.obs.addObserver(this, "update-staged", false);
    }
  },

  uninit: function () {
    if (this.enabled) {
      Services.obs.removeObserver(this, "update-staged");
      PanelUI.panel.removeEventListener("popupshowing", this, true);
      this.enabled = false;
    }
  },

  onMenuPanelCommand: function(event) {
    if (event.originalTarget.getAttribute("update-status") === "succeeded") {
      
      let cancelQuit = Cc["@mozilla.org/supports-PRBool;1"]
                       .createInstance(Ci.nsISupportsPRBool);
      Services.obs.notifyObservers(cancelQuit, "quit-application-requested", "restart");

      if (!cancelQuit.data) {
        Services.startup.quit(Services.startup.eAttemptQuit | Services.startup.eRestart);
      }
    } else {
      
      let url = Services.urlFormatter.formatURLPref("app.update.url.manual");
      openUILinkIn(url, "tab");
    }
  },

  observe: function (subject, topic, status) {
    const STATE_DOWNLOADING     = "downloading";
    const STATE_PENDING         = "pending";
    const STATE_PENDING_SVC     = "pending-service";
    const STATE_APPLIED         = "applied";
    const STATE_APPLIED_SVC     = "applied-service";
    const STATE_FAILED          = "failed";

    let updateButton = document.getElementById("PanelUI-update-status");

    let updateButtonText;
    let stringId;

    
    switch (status) {
      case STATE_APPLIED:
      case STATE_APPLIED_SVC:
      case STATE_PENDING:
      case STATE_PENDING_SVC:
        
        
        
        PanelUI.menuButton.setAttribute("update-status", "succeeded");

        let brandBundle = document.getElementById("bundle_brand");
        let brandShortName = brandBundle.getString("brandShortName");
        stringId = "appmenu.restartNeeded.description";
        updateButtonText = gNavigatorBundle.getFormattedString(stringId,
                                                               [brandShortName]);

        updateButton.setAttribute("label", updateButtonText);
        updateButton.setAttribute("update-status", "succeeded");
        updateButton.hidden = false;

        PanelUI.panel.addEventListener("popupshowing", this, true);

        break;
      case STATE_FAILED:
        
        
        PanelUI.menuButton.setAttribute("update-status", "failed");
        PanelUI.menuButton.setAttribute("badge", "!");

        stringId = "appmenu.updateFailed.description";
        updateButtonText = gNavigatorBundle.getString(stringId);

        updateButton.setAttribute("label", updateButtonText);
        updateButton.setAttribute("update-status", "failed");
        updateButton.hidden = false;

        PanelUI.panel.addEventListener("popupshowing", this, true);

        break;
      case STATE_DOWNLOADING:
        
        
        
        return;
    }
    this.uninit();
  },

  handleEvent: function(e) {
    if (e.type === "popupshowing") {
      PanelUI.menuButton.removeAttribute("badge");
      PanelUI.panel.removeEventListener("popupshowing", this, true);
    }
  }
};






let BrowserOnClick = {
  init: function () {
    let mm = window.messageManager;
    mm.addMessageListener("Browser:CertExceptionError", this);
    mm.addMessageListener("Browser:SiteBlockedError", this);
    mm.addMessageListener("Browser:EnableOnlineMode", this);
    mm.addMessageListener("Browser:SendSSLErrorReport", this);
    mm.addMessageListener("Browser:SetSSLErrorReportAuto", this);
  },

  uninit: function () {
    let mm = window.messageManager;
    mm.removeMessageListener("Browser:CertExceptionError", this);
    mm.removeMessageListener("Browser:SiteBlockedError", this);
    mm.removeMessageListener("Browser:EnableOnlineMode", this);
    mm.removeMessageListener("Browser:SendSSLErrorReport", this);
    mm.removeMessageListener("Browser:SetSSLErrorReportAuto", this);
  },

  handleEvent: function (event) {
    if (!event.isTrusted || 
        event.button == 2) {
      return;
    }

    let originalTarget = event.originalTarget;
    let ownerDoc = originalTarget.ownerDocument;
    if (!ownerDoc) {
      return;
    }

    if (gMultiProcessBrowser &&
        ownerDoc.documentURI.toLowerCase() == "about:newtab") {
      this.onE10sAboutNewTab(event, ownerDoc);
    }
  },

  receiveMessage: function (msg) {
    switch (msg.name) {
      case "Browser:CertExceptionError":
        this.onAboutCertError(msg.target, msg.data.elementId,
                              msg.data.isTopFrame, msg.data.location,
                              msg.data.sslStatusAsString);
      break;
      case "Browser:SiteBlockedError":
        this.onAboutBlocked(msg.data.elementId, msg.data.reason,
                            msg.data.isTopFrame, msg.data.location);
      break;
      case "Browser:EnableOnlineMode":
        if (Services.io.offline) {
          
          Services.io.offline = false;
          msg.target.reload();
        }
      break;
      case "Browser:SendSSLErrorReport":
        this.onSSLErrorReport(msg.target, msg.data.elementId,
                              msg.data.documentURI,
                              msg.data.location,
                              msg.data.securityInfo);
      break;
      case "Browser:SetSSLErrorReportAuto":
        Services.prefs.setBoolPref("security.ssl.errorReporting.automatic", msg.json.automatic);
      break;
    }
  },

  onSSLErrorReport: function(browser, elementId, documentURI, location, securityInfo) {
    function showReportStatus(reportStatus) {
      gBrowser.selectedBrowser
          .messageManager
          .sendAsyncMessage("Browser:SSLErrorReportStatus",
                            {
                              reportStatus: reportStatus,
                              documentURI: documentURI
                            });
    }

    if (!Services.prefs.getBoolPref("security.ssl.errorReporting.enabled")) {
      showReportStatus("error");
      Cu.reportError("User requested certificate error report sending, but certificate error reporting is disabled");
      return;
    }

    let serhelper = Cc["@mozilla.org/network/serialization-helper;1"]
                           .getService(Ci.nsISerializationHelper);
    let transportSecurityInfo = serhelper.deserializeObject(securityInfo);
    transportSecurityInfo.QueryInterface(Ci.nsITransportSecurityInfo)

    showReportStatus("activity");

    










    
    
    function getDERString(cert)
    {
      var length = {};
      var derArray = cert.getRawDER(length);
      var derString = '';
      for (var i = 0; i < derArray.length; i++) {
        derString += String.fromCharCode(derArray[i]);
      }
      return derString;
    }

    
    
    let asciiCertChain = [];

    if (transportSecurityInfo.failedCertChain) {
      let certs = transportSecurityInfo.failedCertChain.getEnumerator();
      while (certs.hasMoreElements()) {
        let cert = certs.getNext();
        cert.QueryInterface(Ci.nsIX509Cert);
        asciiCertChain.push(btoa(getDERString(cert)));
      }
    }

    let report = {
      hostname: location.hostname,
      port: location.port,
      timestamp: Math.round(Date.now() / 1000),
      errorCode: transportSecurityInfo.errorCode,
      failedCertChain: asciiCertChain,
      userAgent: window.navigator.userAgent,
      version: 1,
      build: gAppInfo.appBuildID,
      product: gAppInfo.name,
      channel: Services.prefs.getCharPref("app.update.channel")
    }

    let reportURL = Services.prefs.getCharPref("security.ssl.errorReporting.url");

    let xhr = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
        .createInstance(Ci.nsIXMLHttpRequest);
    try {
      xhr.open("POST", reportURL);
    } catch (e) {
      Cu.reportError("xhr.open exception", e);
      showReportStatus("error");
    }

    xhr.onerror = function (e) {
      
      Cu.reportError("xhr onerror", e);
      showReportStatus("error");
    };

    xhr.onload = function (event) {
      if (xhr.status !== 201 && xhr.status !== 0) {
        
        Cu.reportError("xhr returned failure code", xhr.status);
        showReportStatus("error");
      } else {
        showReportStatus("complete");
      }
    };

    xhr.send(JSON.stringify(report));
  },

  onAboutCertError: function (browser, elementId, isTopFrame, location, sslStatusAsString) {
    let secHistogram = Services.telemetry.getHistogramById("SECURITY_UI");

    switch (elementId) {
      case "exceptionDialogButton":
        if (isTopFrame) {
          secHistogram.add(Ci.nsISecurityUITelemetry.WARNING_BAD_CERT_TOP_CLICK_ADD_EXCEPTION);
        }

        let serhelper = Cc["@mozilla.org/network/serialization-helper;1"]
                           .getService(Ci.nsISerializationHelper);
        let sslStatus = serhelper.deserializeObject(sslStatusAsString);
        sslStatus.QueryInterface(Components.interfaces.nsISSLStatus);
        let params = { exceptionAdded : false,
                       sslStatus : sslStatus };

        try {
          switch (Services.prefs.getIntPref("browser.ssl_override_behavior")) {
            case 2 : 
              params.prefetchCert = true;
            case 1 : 
              params.location = location;
          }
        } catch (e) {
          Components.utils.reportError("Couldn't get ssl_override pref: " + e);
        }

        window.openDialog('chrome://pippki/content/exceptionDialog.xul',
                          '','chrome,centerscreen,modal', params);

        
        if (params.exceptionAdded) {
          browser.reload();
        }
        break;

      case "getMeOutOfHereButton":
        if (isTopFrame) {
          secHistogram.add(Ci.nsISecurityUITelemetry.WARNING_BAD_CERT_TOP_GET_ME_OUT_OF_HERE);
        }
        getMeOutOfHere();
        break;

      case "technicalContent":
        if (isTopFrame) {
          secHistogram.add(Ci.nsISecurityUITelemetry.WARNING_BAD_CERT_TOP_TECHNICAL_DETAILS);
        }
        break;

      case "expertContent":
        if (isTopFrame) {
          secHistogram.add(Ci.nsISecurityUITelemetry.WARNING_BAD_CERT_TOP_UNDERSTAND_RISKS);
        }
        break;

    }
  },

  onAboutBlocked: function (elementId, reason, isTopFrame, location) {
    
    
    let bucketName = "WARNING_PHISHING_PAGE_";
    if (reason === 'malware') {
      bucketName = "WARNING_MALWARE_PAGE_";
    } else if (reason === 'unwanted') {
      bucketName = "WARNING_UNWANTED_PAGE_";
    }
    let secHistogram = Services.telemetry.getHistogramById("SECURITY_UI");
    let nsISecTel = Ci.nsISecurityUITelemetry;
    bucketName += isTopFrame ? "TOP_" : "FRAME_";
    switch (elementId) {
      case "getMeOutButton":
        secHistogram.add(nsISecTel[bucketName + "GET_ME_OUT_OF_HERE"]);
        getMeOutOfHere();
        break;

      case "reportButton":
        
        

        
        
        secHistogram.add(nsISecTel[bucketName + "WHY_BLOCKED"]);

        openHelpLink("phishing-malware", false, "current");
        break;

      case "ignoreWarningButton":
        secHistogram.add(nsISecTel[bucketName + "IGNORE_WARNING"]);
        this.ignoreWarningButton(reason);
        break;
    }
  },

  





  onE10sAboutNewTab: function(event, ownerDoc) {
    let isTopFrame = (ownerDoc.defaultView.parent === ownerDoc.defaultView);
    if (!isTopFrame) {
      return;
    }

    let anchorTarget = event.originalTarget.parentNode;

    if (anchorTarget instanceof HTMLAnchorElement &&
        anchorTarget.classList.contains("newtab-link")) {
      event.preventDefault();
      let where = whereToOpenLink(event, false, false);
      openLinkIn(anchorTarget.href, where, { charset: ownerDoc.characterSet });
    }
  },

  ignoreWarningButton: function (reason) {
    
    
    
    gBrowser.loadURIWithFlags(gBrowser.currentURI.spec,
                              nsIWebNavigation.LOAD_FLAGS_BYPASS_CLASSIFIER,
                              null, null, null);

    Services.perms.add(gBrowser.currentURI, "safe-browsing",
                       Ci.nsIPermissionManager.ALLOW_ACTION,
                       Ci.nsIPermissionManager.EXPIRE_SESSION);

    let buttons = [{
      label: gNavigatorBundle.getString("safebrowsing.getMeOutOfHereButton.label"),
      accessKey: gNavigatorBundle.getString("safebrowsing.getMeOutOfHereButton.accessKey"),
      callback: function() { getMeOutOfHere(); }
    }];

    let title;
    if (reason === 'malware') {
      title = gNavigatorBundle.getString("safebrowsing.reportedAttackSite");
      buttons[1] = {
        label: gNavigatorBundle.getString("safebrowsing.notAnAttackButton.label"),
        accessKey: gNavigatorBundle.getString("safebrowsing.notAnAttackButton.accessKey"),
        callback: function() {
          openUILinkIn(gSafeBrowsing.getReportURL('MalwareError'), 'tab');
        }
      };
    } else if (reason === 'phishing') {
      title = gNavigatorBundle.getString("safebrowsing.reportedWebForgery");
      buttons[1] = {
        label: gNavigatorBundle.getString("safebrowsing.notAForgeryButton.label"),
        accessKey: gNavigatorBundle.getString("safebrowsing.notAForgeryButton.accessKey"),
        callback: function() {
          openUILinkIn(gSafeBrowsing.getReportURL('Error'), 'tab');
        }
      };
    } else if (reason === 'unwanted') {
      title = gNavigatorBundle.getString("safebrowsing.reportedUnwantedSite");
      
      
    }

    let notificationBox = gBrowser.getNotificationBox();
    let value = "blocked-badware-page";

    let previousNotification = notificationBox.getNotificationWithValue(value);
    if (previousNotification) {
      notificationBox.removeNotification(previousNotification);
    }

    let notification = notificationBox.appendNotification(
      title,
      value,
      "chrome://global/skin/icons/blacklist_favicon.png",
      notificationBox.PRIORITY_CRITICAL_HIGH,
      buttons
    );
    
    
    notification.persistence = -1;
  },
};








function getMeOutOfHere() {
  
  var prefs = Services.prefs.getDefaultBranch(null);
  var url = BROWSER_NEW_TAB_URL;
  try {
    url = prefs.getComplexValue("browser.startup.homepage",
                                Ci.nsIPrefLocalizedString).data;
    
    if (url.contains("|"))
      url = url.split("|")[0];
  } catch(e) {
    Components.utils.reportError("Couldn't get homepage pref: " + e);
  }
  gBrowser.loadURI(url);
}

function BrowserFullScreen()
{
  window.fullScreen = !window.fullScreen;
}

function mirrorShow(popup) {
  let services = [];
  if (Services.prefs.getBoolPref("browser.casting.enabled")) {
    services = CastingApps.getServicesForMirroring();
  }
  popup.ownerDocument.getElementById("menu_mirrorTabCmd").hidden = !services.length;
}

function mirrorMenuItemClicked(event) {
  gBrowser.selectedBrowser.messageManager.sendAsyncMessage("SecondScreen:tab-mirror",
                                                           {service: event.originalTarget._service});
}

function populateMirrorTabMenu(popup) {
  popup.innerHTML = null;
  if (!Services.prefs.getBoolPref("browser.casting.enabled")) {
    return;
  }
  let videoEl = this.target;
  let doc = popup.ownerDocument;
  let services = CastingApps.getServicesForMirroring();
  services.forEach(service => {
    let item = doc.createElement("menuitem");
    item.setAttribute("label", service.friendlyName);
    item._service = service;
    item.addEventListener("command", mirrorMenuItemClicked);
    popup.appendChild(item);
  });
};

function _checkDefaultAndSwitchToMetro() {
#ifdef HAVE_SHELL_SERVICE
#ifdef XP_WIN
#ifdef MOZ_METRO
  let shell = Components.classes["@mozilla.org/browser/shell-service;1"].
    getService(Components.interfaces.nsIShellService);
  let isDefault = shell.isDefaultBrowser(false, false);

  if (isDefault) {
    let appStartup = Components.classes["@mozilla.org/toolkit/app-startup;1"].
    getService(Components.interfaces.nsIAppStartup);

    Services.prefs.setBoolPref('browser.sessionstore.resume_session_once', true);

    let cancelQuit = Cc["@mozilla.org/supports-PRBool;1"]
                     .createInstance(Ci.nsISupportsPRBool);
    Services.obs.notifyObservers(cancelQuit, "quit-application-requested", "restart");

    if (!cancelQuit.data) {
      appStartup.quit(Components.interfaces.nsIAppStartup.eAttemptQuit |
                      Components.interfaces.nsIAppStartup.eRestartTouchEnvironment);
    }
    return true;
  }
  return false;
#endif
#endif
#endif
}

function SwitchToMetro() {
#ifdef HAVE_SHELL_SERVICE
#ifdef XP_WIN
#ifdef MOZ_METRO
  if (this._checkDefaultAndSwitchToMetro()) {
    return;
  }

  let shell = Components.classes["@mozilla.org/browser/shell-service;1"].
    getService(Components.interfaces.nsIShellService);

  shell.setDefaultBrowser(false, false);

  let intervalID = window.setInterval(this._checkDefaultAndSwitchToMetro, 1000);
  window.setTimeout(function() { window.clearInterval(intervalID); }, 10000);
#endif
#endif
#endif
}

function getWebNavigation()
{
  return gBrowser.webNavigation;
}

function BrowserReloadWithFlags(reloadFlags) {
  let url = gBrowser.currentURI.spec;
  if (gBrowser.updateBrowserRemotenessByURL(gBrowser.selectedBrowser, url)) {
    
    
    
    gBrowser.loadURIWithFlags(url, reloadFlags);
    return;
  }

  let windowUtils = window.QueryInterface(Ci.nsIInterfaceRequestor)
                          .getInterface(Ci.nsIDOMWindowUtils);

  gBrowser.selectedBrowser
          .messageManager
          .sendAsyncMessage("Browser:Reload",
                            { flags: reloadFlags,
                              handlingUserInput: windowUtils.isHandlingUserInput });
}

var PrintPreviewListener = {
  _printPreviewTab: null,
  _tabBeforePrintPreview: null,

  getPrintPreviewBrowser: function () {
    if (!this._printPreviewTab) {
      this._tabBeforePrintPreview = gBrowser.selectedTab;
      this._printPreviewTab = gBrowser.loadOneTab("about:blank",
                                                  { inBackground: false });
      gBrowser.selectedTab = this._printPreviewTab;
    }
    return gBrowser.getBrowserForTab(this._printPreviewTab);
  },
  getSourceBrowser: function () {
    return this._tabBeforePrintPreview ?
      this._tabBeforePrintPreview.linkedBrowser : gBrowser.selectedBrowser;
  },
  getNavToolbox: function () {
    return gNavToolbox;
  },
  onEnter: function () {
    gInPrintPreviewMode = true;
    this._toggleAffectedChrome();
  },
  onExit: function () {
    gBrowser.selectedTab = this._tabBeforePrintPreview;
    this._tabBeforePrintPreview = null;
    gInPrintPreviewMode = false;
    this._toggleAffectedChrome();
    gBrowser.removeTab(this._printPreviewTab);
    this._printPreviewTab = null;
  },
  _toggleAffectedChrome: function () {
    gNavToolbox.collapsed = gInPrintPreviewMode;

    if (gInPrintPreviewMode)
      this._hideChrome();
    else
      this._showChrome();

    TabsInTitlebar.allowedBy("print-preview", !gInPrintPreviewMode);
  },
  _hideChrome: function () {
    this._chromeState = {};

    this._chromeState.sidebarOpen = SidebarUI.isOpen;
    this._sidebarCommand = SidebarUI.currentID;
    SidebarUI.hide();

    var notificationBox = gBrowser.getNotificationBox();
    this._chromeState.notificationsOpen = !notificationBox.notificationsHidden;
    notificationBox.notificationsHidden = true;

    gBrowser.updateWindowResizers();

    this._chromeState.findOpen = gFindBarInitialized && !gFindBar.hidden;
    if (gFindBarInitialized)
      gFindBar.close();

    var globalNotificationBox = document.getElementById("global-notificationbox");
    this._chromeState.globalNotificationsOpen = !globalNotificationBox.notificationsHidden;
    globalNotificationBox.notificationsHidden = true;

    this._chromeState.syncNotificationsOpen = false;
    var syncNotifications = document.getElementById("sync-notifications");
    if (syncNotifications) {
      this._chromeState.syncNotificationsOpen = !syncNotifications.notificationsHidden;
      syncNotifications.notificationsHidden = true;
    }
  },
  _showChrome: function () {
    if (this._chromeState.notificationsOpen)
      gBrowser.getNotificationBox().notificationsHidden = false;

    if (this._chromeState.findOpen)
      gFindBar.open();

    if (this._chromeState.globalNotificationsOpen)
      document.getElementById("global-notificationbox").notificationsHidden = false;

    if (this._chromeState.syncNotificationsOpen)
      document.getElementById("sync-notifications").notificationsHidden = false;

    if (this._chromeState.sidebarOpen)
      SidebarUI.show(this._sidebarCommand);
  }
}

function getMarkupDocumentViewer()
{
  return gBrowser.markupDocumentViewer;
}


function FillInHTMLTooltip(tipElement)
{
  document.getElementById("aHTMLTooltip").fillInPageTooltip(tipElement);
}

var browserDragAndDrop = {
  canDropLink: function (aEvent) Services.droppedLinkHandler.canDropLink(aEvent, true),

  dragOver: function (aEvent)
  {
    if (this.canDropLink(aEvent)) {
      aEvent.preventDefault();
    }
  },

  drop: function (aEvent, aName, aDisallowInherit) {
    return Services.droppedLinkHandler.dropLink(aEvent, aName, aDisallowInherit);
  }
};

var homeButtonObserver = {
  onDrop: function (aEvent)
    {
      
      let url = browserDragAndDrop.drop(aEvent, {}, true);
      setTimeout(openHomeDialog, 0, url);
    },

  onDragOver: function (aEvent)
    {
      browserDragAndDrop.dragOver(aEvent);
      aEvent.dropEffect = "link";
    },
  onDragExit: function (aEvent)
    {
    }
}

function openHomeDialog(aURL)
{
  var promptTitle = gNavigatorBundle.getString("droponhometitle");
  var promptMsg   = gNavigatorBundle.getString("droponhomemsg");
  var pressedVal  = Services.prompt.confirmEx(window, promptTitle, promptMsg,
                          Services.prompt.STD_YES_NO_BUTTONS,
                          null, null, null, null, {value:0});

  if (pressedVal == 0) {
    try {
      var str = Components.classes["@mozilla.org/supports-string;1"]
                          .createInstance(Components.interfaces.nsISupportsString);
      str.data = aURL;
      gPrefService.setComplexValue("browser.startup.homepage",
                                   Components.interfaces.nsISupportsString, str);
    } catch (ex) {
      dump("Failed to set the home page.\n"+ex+"\n");
    }
  }
}

var newTabButtonObserver = {
  onDragOver: function (aEvent)
  {
    browserDragAndDrop.dragOver(aEvent);
  },

  onDragExit: function (aEvent)
  {
  },

  onDrop: function (aEvent)
  {
    let url = browserDragAndDrop.drop(aEvent, { });
    getShortcutOrURIAndPostData(url).then(data => {
      if (data.url) {
        
        openNewTabWith(data.url, null, data.postData, aEvent, true);
      }
    });
  }
}

var newWindowButtonObserver = {
  onDragOver: function (aEvent)
  {
    browserDragAndDrop.dragOver(aEvent);
  },
  onDragExit: function (aEvent)
  {
  },
  onDrop: function (aEvent)
  {
    let url = browserDragAndDrop.drop(aEvent, { });
    getShortcutOrURIAndPostData(url).then(data => {
      if (data.url) {
        
        openNewWindowWith(data.url, null, data.postData, true);
      }
    });
  }
}

const DOMLinkHandler = {
  init: function() {
    let mm = window.messageManager;
    mm.addMessageListener("Link:AddFeed", this);
    mm.addMessageListener("Link:SetIcon", this);
    mm.addMessageListener("Link:AddSearch", this);
  },

  receiveMessage: function (aMsg) {
    switch (aMsg.name) {
      case "Link:AddFeed":
        let link = {type: aMsg.data.type, href: aMsg.data.href, title: aMsg.data.title};
        FeedHandler.addFeed(link, aMsg.target);
        break;

      case "Link:SetIcon":
        return this.setIcon(aMsg.target, aMsg.data.url);
        break;

      case "Link:AddSearch":
        this.addSearch(aMsg.target, aMsg.data.engine, aMsg.data.url);
        break;
    }
  },

  setIcon: function(aBrowser, aURL) {
    if (gBrowser.isFailedIcon(aURL))
      return false;

    let tab = gBrowser.getTabForBrowser(aBrowser);
    if (!tab)
      return false;

    gBrowser.setIcon(tab, aURL);
    return true;
  },

  addSearch: function(aBrowser, aEngine, aURL) {
    let tab = gBrowser.getTabForBrowser(aBrowser);
    if (!tab)
      return false;

    BrowserSearch.addEngine(aBrowser, aEngine, makeURI(aURL));
  },
}

const BrowserSearch = {
  addEngine: function(browser, engine, uri) {
    if (!this.searchBar)
      return;

    
    if (browser.engines) {
      if (browser.engines.some(function (e) e.title == engine.title))
        return;
    }

    var hidden = false;
    
    
    
    
    if (Services.search.getEngineByName(engine.title))
      hidden = true;

    var engines = (hidden ? browser.hiddenEngines : browser.engines) || [];

    engines.push({ uri: engine.href,
                   title: engine.title,
                   get icon() { return browser.mIconURL; }
                 });

    if (hidden)
      browser.hiddenEngines = engines;
    else {
      browser.engines = engines;
      if (browser == gBrowser.selectedBrowser)
        this.updateSearchButton();
    }
  },

  




  updateSearchButton: function() {
    var searchBar = this.searchBar;

    
    
    
    if (!searchBar || !searchBar.searchButton)
      return;

    var engines = gBrowser.selectedBrowser.engines;
    if (engines && engines.length > 0)
      searchBar.setAttribute("addengines", "true");
    else
      searchBar.removeAttribute("addengines");
  },

  




  webSearch: function BrowserSearch_webSearch() {
#ifdef XP_MACOSX
    if (window.location.href != getBrowserURL()) {
      var win = getTopWin();
      if (win) {
        
        win.focus();
        win.BrowserSearch.webSearch();
      } else {
        
        var observer = function observer(subject, topic, data) {
          if (subject == win) {
            BrowserSearch.webSearch();
            Services.obs.removeObserver(observer, "browser-delayed-startup-finished");
          }
        }
        win = window.openDialog(getBrowserURL(), "_blank",
                                "chrome,all,dialog=no", "about:blank");
        Services.obs.addObserver(observer, "browser-delayed-startup-finished", false);
      }
      return;
    }
#endif
    let openSearchPageIfFieldIsNotActive = function(aSearchBar) {
      if (!aSearchBar || document.activeElement != aSearchBar.textbox.inputField) {
        let url = gBrowser.currentURI.spec.toLowerCase();
        let mm = gBrowser.selectedBrowser.messageManager;
        if (url === "about:home") {
          AboutHome.focusInput(mm);
        } else if (url === "about:newtab" && NewTabUtils.allPages.enabled) {
          ContentSearch.focusInput(mm);
        } else {
          openUILinkIn("about:home", "current");
        }
      }
    };

    let searchBar = this.searchBar;
    let placement = CustomizableUI.getPlacementOfWidget("search-container");
    let focusSearchBar = () => {
      searchBar = this.searchBar;
      searchBar.select();
      openSearchPageIfFieldIsNotActive(searchBar);
    };
    if (placement && placement.area == CustomizableUI.AREA_PANEL) {
      
      PanelUI.show().then(focusSearchBar);
      return;
    }
    if (placement && placement.area == CustomizableUI.AREA_NAVBAR && searchBar &&
        searchBar.parentNode.getAttribute("overflowedItem") == "true") {
      let navBar = document.getElementById(CustomizableUI.AREA_NAVBAR);
      navBar.overflowable.show().then(() => {
        focusSearchBar();
      });
      return;
    }
    if (searchBar) {
      if (window.fullScreen)
        FullScreen.mouseoverToggle(true);
      searchBar.select();
    }
    openSearchPageIfFieldIsNotActive(searchBar);
  },

  


















  _loadSearch: function (searchText, useNewTab, purpose) {
    let engine;

    
    
    if (isElementVisible(this.searchBar))
      engine = Services.search.currentEngine;
    else
      engine = Services.search.defaultEngine;

    let submission = engine.getSubmission(searchText, null, purpose); 

    
    
    
    
    if (!submission) {
      return null;
    }

    let inBackground = Services.prefs.getBoolPref("browser.search.context.loadInBackground");
    openLinkIn(submission.uri.spec,
               useNewTab ? "tab" : "current",
               { postData: submission.postData,
                 inBackground: inBackground,
                 relatedToCurrent: true });

    return engine;
  },

  





  loadSearch: function BrowserSearch_search(searchText, useNewTab, purpose) {
    let engine = BrowserSearch._loadSearch(searchText, useNewTab, purpose);
    if (!engine) {
      return null;
    }
    return engine.name;
  },

  





  loadSearchFromContext: function (terms) {
    let engine = BrowserSearch._loadSearch(terms, true, "contextmenu");
    if (engine) {
      BrowserSearch.recordSearchInHealthReport(engine, "contextmenu");
    }
  },

  pasteAndSearch: function (event) {
    BrowserSearch.searchBar.select();
    goDoCommand("cmd_paste");
    BrowserSearch.searchBar.handleSearchCommand(event);
  },

  


  get searchBar() {
    return document.getElementById("searchbar");
  },

  loadAddEngines: function BrowserSearch_loadAddEngines() {
    var newWindowPref = gPrefService.getIntPref("browser.link.open_newwindow");
    var where = newWindowPref == 3 ? "tab" : "window";
    var searchEnginesURL = formatURL("browser.search.searchEnginesURL", true);
    openUILinkIn(searchEnginesURL, where);
  },

  














  recordSearchInHealthReport: function (engine, source, selection) {
    BrowserUITelemetry.countSearchEvent(source, null, selection);
    this.recordSearchInTelemetry(engine, source);
#ifdef MOZ_SERVICES_HEALTHREPORT
    let reporter = Cc["@mozilla.org/datareporting/service;1"]
                     .getService()
                     .wrappedJSObject
                     .healthReporter;

    
    
    if (!reporter) {
      return;
    }

    reporter.onInit().then(function record() {
      try {
        reporter.getProvider("org.mozilla.searches").recordSearch(engine, source);
      } catch (ex) {
        Cu.reportError(ex);
      }
    });
#endif
  },

  _getSearchEngineId: function (engine) {
    if (!engine) {
      return "other";
    }

    if (engine.identifier) {
      return engine.identifier;
    }

    return "other-" + engine.name;
  },

  recordSearchInTelemetry: function (engine, source) {
    const SOURCES = [
      "abouthome",
      "contextmenu",
      "newtab",
      "searchbar",
      "urlbar",
    ];

    if (SOURCES.indexOf(source) == -1) {
      Cu.reportError("Unknown source for search: " + source);
      return;
    }

    let countId = this._getSearchEngineId(engine) + "." + source;

    let count = Services.telemetry.getKeyedHistogramById("SEARCH_COUNTS");
    count.add(countId);
  },

  recordOneoffSearchInTelemetry: function (engine, source, type, where) {
    let id = this._getSearchEngineId(engine) + "." + source;
    BrowserUITelemetry.countOneoffSearchEvent(id, type, where);
  }
};

function FillHistoryMenu(aParent) {
  
  if (!aParent.hasStatusListener) {
    
    aParent.addEventListener("DOMMenuItemActive", function(aEvent) {
      
      if (!aEvent.target.hasAttribute("checked"))
        XULBrowserWindow.setOverLink(aEvent.target.getAttribute("uri"));
    }, false);
    aParent.addEventListener("DOMMenuItemInactive", function() {
      XULBrowserWindow.setOverLink("");
    }, false);

    aParent.hasStatusListener = true;
  }

  
  var children = aParent.childNodes;
  for (var i = children.length - 1; i >= 0; --i) {
    if (children[i].hasAttribute("index"))
      aParent.removeChild(children[i]);
  }

  var webNav = gBrowser.webNavigation;
  var sessionHistory = webNav.sessionHistory;

  var count = sessionHistory.count;
  if (count <= 1) 
    return false;

  const MAX_HISTORY_MENU_ITEMS = 15;
  var index = sessionHistory.index;
  var half_length = Math.floor(MAX_HISTORY_MENU_ITEMS / 2);
  var start = Math.max(index - half_length, 0);
  var end = Math.min(start == 0 ? MAX_HISTORY_MENU_ITEMS : index + half_length + 1, count);
  if (end == count)
    start = Math.max(count - MAX_HISTORY_MENU_ITEMS, 0);

  var tooltipBack = gNavigatorBundle.getString("tabHistory.goBack");
  var tooltipCurrent = gNavigatorBundle.getString("tabHistory.current");
  var tooltipForward = gNavigatorBundle.getString("tabHistory.goForward");

  for (var j = end - 1; j >= start; j--) {
    let item = document.createElement("menuitem");
    let entry = sessionHistory.getEntryAtIndex(j, false);
    let uri = entry.URI.spec;
    let entryURI = BrowserUtils.makeURIFromCPOW(entry.URI);

    item.setAttribute("uri", uri);
    item.setAttribute("label", entry.title || uri);
    item.setAttribute("index", j);

    if (j != index) {
      PlacesUtils.favicons.getFaviconURLForPage(entryURI, function (aURI) {
        if (aURI) {
          let iconURL = PlacesUtils.favicons.getFaviconLinkForIcon(aURI).spec;
          iconURL = PlacesUtils.getImageURLForResolution(window, iconURL);
          item.style.listStyleImage = "url(" + iconURL + ")";
        }
      });
    }

    if (j < index) {
      item.className = "unified-nav-back menuitem-iconic menuitem-with-favicon";
      item.setAttribute("tooltiptext", tooltipBack);
    } else if (j == index) {
      item.setAttribute("type", "radio");
      item.setAttribute("checked", "true");
      item.className = "unified-nav-current";
      item.setAttribute("tooltiptext", tooltipCurrent);
    } else {
      item.className = "unified-nav-forward menuitem-iconic menuitem-with-favicon";
      item.setAttribute("tooltiptext", tooltipForward);
    }

    aParent.appendChild(item);
  }
  return true;
}

function addToUrlbarHistory(aUrlToAdd) {
  if (!PrivateBrowsingUtils.isWindowPrivate(window) &&
      aUrlToAdd &&
      !aUrlToAdd.contains(" ") &&
      !/[\x00-\x1F]/.test(aUrlToAdd))
    PlacesUIUtils.markPageAsTyped(aUrlToAdd);
}

function toJavaScriptConsole()
{
  toOpenWindowByType("global:console", "chrome://global/content/console.xul");
}

function BrowserDownloadsUI()
{
  if (PrivateBrowsingUtils.isWindowPrivate(window)) {
    openUILinkIn("about:downloads", "tab");
  } else {
    PlacesCommandHook.showPlacesOrganizer("Downloads");
  }
}

function toOpenWindowByType(inType, uri, features)
{
  var topWindow = Services.wm.getMostRecentWindow(inType);

  if (topWindow)
    topWindow.focus();
  else if (features)
    window.open(uri, "_blank", features);
  else
    window.open(uri, "_blank", "chrome,extrachrome,menubar,resizable,scrollbars,status,toolbar");
}

function OpenBrowserWindow(options)
{
  var telemetryObj = {};
  TelemetryStopwatch.start("FX_NEW_WINDOW_MS", telemetryObj);

  function newDocumentShown(doc, topic, data) {
    if (topic == "document-shown" &&
        doc != document &&
        doc.defaultView == win) {
      Services.obs.removeObserver(newDocumentShown, "document-shown");
      Services.obs.removeObserver(windowClosed, "domwindowclosed");
      TelemetryStopwatch.finish("FX_NEW_WINDOW_MS", telemetryObj);
    }
  }

  function windowClosed(subject) {
    if (subject == win) {
      Services.obs.removeObserver(newDocumentShown, "document-shown");
      Services.obs.removeObserver(windowClosed, "domwindowclosed");
    }
  }

  
  
  Services.obs.addObserver(newDocumentShown, "document-shown", false);
  Services.obs.addObserver(windowClosed, "domwindowclosed", false);

  var charsetArg = new String();
  var handler = Components.classes["@mozilla.org/browser/clh;1"]
                          .getService(Components.interfaces.nsIBrowserHandler);
  var defaultArgs = handler.defaultArgs;
  var wintype = document.documentElement.getAttribute('windowtype');

  var extraFeatures = "";
  if (options && options.private) {
    extraFeatures = ",private";
    if (!PrivateBrowsingUtils.permanentPrivateBrowsing) {
      
      defaultArgs = "about:privatebrowsing";
    }
  } else {
    extraFeatures = ",non-private";
  }

  if (options && options.remote) {
    
    
    let omtcEnabled = gPrefService.getBoolPref("layers.offmainthreadcomposition.enabled")
                      || Services.appinfo.browserTabsRemoteAutostart;
    if (!omtcEnabled) {
      alert("To use out-of-process tabs, you must set the layers.offmainthreadcomposition.enabled preference and restart. Opening a normal window instead.");
    } else {
      extraFeatures += ",remote";
    }
  } else if (options && options.remote === false) {
    extraFeatures += ",non-remote";
  }

  
  
  
  var win;
  if (window && (wintype == "navigator:browser") && window.content && window.content.document)
  {
    var DocCharset = window.content.document.characterSet;
    charsetArg = "charset="+DocCharset;

    
    win = window.openDialog("chrome://browser/content/", "_blank", "chrome,all,dialog=no" + extraFeatures, defaultArgs, charsetArg);
  }
  else 
  {
    win = window.openDialog("chrome://browser/content/", "_blank", "chrome,all,dialog=no" + extraFeatures, defaultArgs);
  }

  return win;
}


function BrowserCustomizeToolbar() {
  gCustomizeMode.enter();
}





















function updateEditUIVisibility()
{
#ifndef XP_MACOSX
  let editMenuPopupState = document.getElementById("menu_EditPopup").state;
  let contextMenuPopupState = document.getElementById("contentAreaContextMenu").state;
  let placesContextMenuPopupState = document.getElementById("placesContext").state;

  
  
  
  gEditUIVisible = editMenuPopupState == "showing" ||
                   editMenuPopupState == "open" ||
                   contextMenuPopupState == "showing" ||
                   contextMenuPopupState == "open" ||
                   placesContextMenuPopupState == "showing" ||
                   placesContextMenuPopupState == "open" ||
                   document.getElementById("edit-controls") ? true : false;

  
  
  if (gEditUIVisible)
    goUpdateGlobalEditMenuItems();

  
  
  
  else {
    goSetCommandEnabled("cmd_undo", true);
    goSetCommandEnabled("cmd_redo", true);
    goSetCommandEnabled("cmd_cut", true);
    goSetCommandEnabled("cmd_copy", true);
    goSetCommandEnabled("cmd_paste", true);
    goSetCommandEnabled("cmd_selectAll", true);
    goSetCommandEnabled("cmd_delete", true);
    goSetCommandEnabled("cmd_switchTextDirection", true);
  }
#endif
}





function updateCharacterEncodingMenuState()
{
  let charsetMenu = document.getElementById("charsetMenu");
  
  
  
  if (gBrowser && gBrowser.selectedBrowser.mayEnableCharacterEncodingMenu) {
    if (charsetMenu) {
      charsetMenu.removeAttribute("disabled");
    }
  } else {
    if (charsetMenu) {
      charsetMenu.setAttribute("disabled", "true");
    }
  }
}

var XULBrowserWindow = {
  
  status: "",
  defaultStatus: "",
  overLink: "",
  startTime: 0,
  statusText: "",
  isBusy: false,
  
  inContentWhitelist: [],

  QueryInterface: function (aIID) {
    if (aIID.equals(Ci.nsIWebProgressListener) ||
        aIID.equals(Ci.nsIWebProgressListener2) ||
        aIID.equals(Ci.nsISupportsWeakReference) ||
        aIID.equals(Ci.nsIXULBrowserWindow) ||
        aIID.equals(Ci.nsISupports))
      return this;
    throw Cr.NS_NOINTERFACE;
  },

  get stopCommand () {
    delete this.stopCommand;
    return this.stopCommand = document.getElementById("Browser:Stop");
  },
  get reloadCommand () {
    delete this.reloadCommand;
    return this.reloadCommand = document.getElementById("Browser:Reload");
  },
  get statusTextField () {
    return gBrowser.getStatusPanel();
  },
  get isImage () {
    delete this.isImage;
    return this.isImage = document.getElementById("isImage");
  },

  init: function () {
    
    var securityUI = gBrowser.securityUI;
    this.onSecurityChange(null, null, securityUI.state);
  },

  setJSStatus: function () {
    
  },

  forceInitialBrowserRemote: function() {
    let initBrowser =
      document.getAnonymousElementByAttribute(gBrowser, "anonid", "initialBrowser");
    gBrowser.updateBrowserRemoteness(initBrowser, true);
    return initBrowser.frameLoader.tabParent;
  },

  setDefaultStatus: function (status) {
    this.defaultStatus = status;
    this.updateStatusField();
  },

  setOverLink: function (url, anchorElt) {
    
    
    url = url.replace(/[\u200e\u200f\u202a\u202b\u202c\u202d\u202e]/g,
                      encodeURIComponent);

    if (gURLBar && gURLBar._mayTrimURLs )
      url = trimURL(url);

    this.overLink = url;
    LinkTargetDisplay.update();
  },

  showTooltip: function (x, y, tooltip) {
    if (Cc["@mozilla.org/widget/dragservice;1"].getService(Ci.nsIDragService).
        getCurrentSession()) {
      return;
    }

    
    
    let elt = document.getElementById("remoteBrowserTooltip");
    elt.label = tooltip;

    let anchor = gBrowser.selectedBrowser;
    elt.openPopupAtScreen(anchor.boxObject.screenX + x, anchor.boxObject.screenY + y, false, null);
  },

  hideTooltip: function () {
    let elt = document.getElementById("remoteBrowserTooltip");
    elt.hidePopup();
  },

  updateStatusField: function () {
    var text, type, types = ["overLink"];
    if (this._busyUI)
      types.push("status");
    types.push("defaultStatus");
    for (type of types) {
      text = this[type];
      if (text)
        break;
    }

    
    
    if (this.statusText != text) {
      let field = this.statusTextField;
      field.setAttribute("previoustype", field.getAttribute("type"));
      field.setAttribute("type", type);
      field.label = text;
      field.setAttribute("crop", type == "overLink" ? "center" : "end");
      this.statusText = text;
    }
  },

  
  onBeforeLinkTraversal: function(originalTarget, linkURI, linkNode, isAppTab) {
    let target = BrowserUtils.onBeforeLinkTraversal(originalTarget, linkURI, linkNode, isAppTab);
    SocialUI.closeSocialPanelForLinkTraversal(target, linkNode);
    return target;
  },

  
  shouldLoadURI: function(aDocShell, aURI, aReferrer) {
    if (!gMultiProcessBrowser)
      return true;

    let browser = aDocShell.QueryInterface(Ci.nsIDocShellTreeItem)
                           .sameTypeRootTreeItem
                           .QueryInterface(Ci.nsIDocShell)
                           .chromeEventHandler;

    
    if (browser.localName != "browser" || !browser.getTabBrowser || browser.getTabBrowser() != gBrowser)
      return true;

    if (!E10SUtils.shouldLoadURI(aDocShell, aURI, aReferrer)) {
      E10SUtils.redirectLoad(aDocShell, aURI, aReferrer);
      return false;
    }

    return true;
  },

  onProgressChange: function (aWebProgress, aRequest,
                              aCurSelfProgress, aMaxSelfProgress,
                              aCurTotalProgress, aMaxTotalProgress) {
    
  },

  onProgressChange64: function (aWebProgress, aRequest,
                                aCurSelfProgress, aMaxSelfProgress,
                                aCurTotalProgress, aMaxTotalProgress) {
    return this.onProgressChange(aWebProgress, aRequest,
      aCurSelfProgress, aMaxSelfProgress, aCurTotalProgress,
      aMaxTotalProgress);
  },

  
  onStateChange: function (aWebProgress, aRequest, aStateFlags, aStatus) {
    const nsIWebProgressListener = Ci.nsIWebProgressListener;
    const nsIChannel = Ci.nsIChannel;

    let browser = gBrowser.selectedBrowser;

    if (aStateFlags & nsIWebProgressListener.STATE_START &&
        aStateFlags & nsIWebProgressListener.STATE_IS_NETWORK) {

      if (aRequest && aWebProgress.isTopLevel) {
        
        browser.feeds = null;

        
        browser.engines = null;
      }

      this.isBusy = true;

      if (!(aStateFlags & nsIWebProgressListener.STATE_RESTORING)) {
        this._busyUI = true;

        
        this.stopCommand.removeAttribute("disabled");
        CombinedStopReload.switchToStop();
      }
    }
    else if (aStateFlags & nsIWebProgressListener.STATE_STOP) {
      
      
      
      if (aRequest) {
        let msg = "";
        let location;
        
        if (aRequest instanceof nsIChannel || "URI" in aRequest) {
          location = aRequest.URI;

          
          if (location.scheme == "keyword" && aWebProgress.isTopLevel)
            gBrowser.userTypedValue = null;

          if (location.spec != "about:blank") {
            switch (aStatus) {
              case Components.results.NS_ERROR_NET_TIMEOUT:
                msg = gNavigatorBundle.getString("nv_timeout");
                break;
            }
          }
        }

        this.status = "";
        this.setDefaultStatus(msg);

        
        if (browser.documentContentType && BrowserUtils.mimeTypeIsTextBased(browser.documentContentType))
          this.isImage.removeAttribute('disabled');
        else
          this.isImage.setAttribute('disabled', 'true');
      }

      this.isBusy = false;

      if (this._busyUI) {
        this._busyUI = false;

        this.stopCommand.setAttribute("disabled", "true");
        CombinedStopReload.switchToReload(aRequest instanceof Ci.nsIRequest);
      }
    }
  },

  onLocationChange: function (aWebProgress, aRequest, aLocationURI, aFlags) {
    var location = aLocationURI ? aLocationURI.spec : "";

    
    FormValidationHandler.hidePopup();

    let pageTooltip = document.getElementById("aHTMLTooltip");
    let tooltipNode = pageTooltip.triggerNode;
    if (tooltipNode) {
      
      if (aWebProgress.isTopLevel) {
        pageTooltip.hidePopup();
      }
      else {
        for (let tooltipWindow = tooltipNode.ownerDocument.defaultView;
             tooltipWindow != tooltipWindow.parent;
             tooltipWindow = tooltipWindow.parent) {
          if (tooltipWindow == aWebProgress.DOMWindow) {
            pageTooltip.hidePopup();
            break;
          }
        }
      }
    }

    let browser = gBrowser.selectedBrowser;

    
    if (browser.documentContentType && BrowserUtils.mimeTypeIsTextBased(browser.documentContentType))
      this.isImage.removeAttribute('disabled');
    else
      this.isImage.setAttribute('disabled', 'true');

    this.hideOverLinkImmediately = true;
    this.setOverLink("", null);
    this.hideOverLinkImmediately = false;

    
    
    
    

    if (aWebProgress.isTopLevel) {
      if ((location == "about:blank" && (gMultiProcessBrowser || !content.opener)) ||
          location == "") {  
                             
        this.reloadCommand.setAttribute("disabled", "true");
      } else {
        this.reloadCommand.removeAttribute("disabled");
      }

      if (gURLBar) {
        URLBarSetURI(aLocationURI);

        BookmarkingUI.onLocationChange();
        SocialUI.updateState(location);
        UITour.onLocationChange(location);
      }

      
      var shouldDisableFind = function shouldDisableFind(aDocument) {
        let docElt = aDocument.documentElement;
        return docElt && docElt.getAttribute("disablefastfind") == "true";
      }

      var disableFindCommands = function disableFindCommands(aDisable) {
        let findCommands = [document.getElementById("cmd_find"),
                            document.getElementById("cmd_findAgain"),
                            document.getElementById("cmd_findPrevious")];
        for (let elt of findCommands) {
          if (aDisable)
            elt.setAttribute("disabled", "true");
          else
            elt.removeAttribute("disabled");
        }
      }

      var onContentRSChange = function onContentRSChange(e) {
        if (e.target.readyState != "interactive" && e.target.readyState != "complete")
          return;

        e.target.removeEventListener("readystatechange", onContentRSChange);
        disableFindCommands(shouldDisableFind(e.target));
      }

      
      if (!gMultiProcessBrowser && aLocationURI &&
          (aLocationURI.schemeIs("about") || aLocationURI.schemeIs("chrome"))) {
        
        
        if (!(aFlags & Ci.nsIWebProgressListener.LOCATION_CHANGE_SAME_DOCUMENT)) {
          if (content.document.readyState == "interactive" || content.document.readyState == "complete")
            disableFindCommands(shouldDisableFind(content.document));
          else {
            content.document.addEventListener("readystatechange", onContentRSChange);
          }
        }
      } else
        disableFindCommands(false);

      
      
      let customizingURI = "about:customizing";
      if (location == customizingURI) {
        gCustomizeMode.enter();
      } else if (location != customizingURI &&
                 (CustomizationHandler.isEnteringCustomizeMode ||
                  CustomizationHandler.isCustomizing())) {
        gCustomizeMode.exit();
      }
    }
    UpdateBackForwardCommands(gBrowser.webNavigation);
    ReaderParent.updateReaderButton(gBrowser.selectedBrowser);

    gGestureSupport.restoreRotationState();

    
    
    if (aRequest)
      setTimeout(function () { XULBrowserWindow.asyncUpdateUI(); }, 0);
    else
      this.asyncUpdateUI();

#ifdef MOZ_CRASHREPORTER
    if (aLocationURI) {
      let uri = aLocationURI.clone();
      try {
        
        uri.userPass = "";
      } catch (ex) {  }

      try {
        gCrashReporter.annotateCrashReport("URL", uri.spec);
      } catch (ex if ex.result == Components.results.NS_ERROR_NOT_INITIALIZED) {
        
      }
    }
#endif
  },

  asyncUpdateUI: function () {
    FeedHandler.updateFeeds();
    BrowserSearch.updateSearchButton();
  },

  
  hideChromeForLocation: function() {},

  onStatusChange: function (aWebProgress, aRequest, aStatus, aMessage) {
    this.status = aMessage;
    this.updateStatusField();
  },

  
  _state: null,
  _lastLocation: null,

  onSecurityChange: function (aWebProgress, aRequest, aState) {
    
    
    let uri = gBrowser.currentURI;
    let spec = uri.spec;
    if (this._state == aState &&
        this._lastLocation == spec)
      return;
    this._state = aState;
    this._lastLocation = spec;

    
    
    const wpl = Components.interfaces.nsIWebProgressListener;
    const wpl_security_bits = wpl.STATE_IS_SECURE |
                              wpl.STATE_IS_BROKEN |
                              wpl.STATE_IS_INSECURE;
    var level;

    switch (this._state & wpl_security_bits) {
      case wpl.STATE_IS_SECURE:
        level = "high";
        break;
      case wpl.STATE_IS_BROKEN:
        level = "broken";
        break;
    }

    if (level) {
      
      
      if (gURLBar)
        gURLBar.setAttribute("level", level);
    } else {
      if (gURLBar)
        gURLBar.removeAttribute("level");
    }

    try {
      uri = Services.uriFixup.createExposableURI(uri);
    } catch (e) {}
    gIdentityHandler.checkIdentity(this._state, uri);
  },

  
  onUpdateCurrentBrowser: function XWB_onUpdateCurrentBrowser(aStateFlags, aStatus, aMessage, aTotalProgress) {
    if (FullZoom.updateBackgroundTabs)
      FullZoom.onLocationChange(gBrowser.currentURI, true);
    var nsIWebProgressListener = Components.interfaces.nsIWebProgressListener;
    var loadingDone = aStateFlags & nsIWebProgressListener.STATE_STOP;
    
    
    
    this.onStateChange(
      gBrowser.webProgress,
      { URI: gBrowser.currentURI },
      loadingDone ? nsIWebProgressListener.STATE_STOP : nsIWebProgressListener.STATE_START,
      aStatus
    );
    
    if (loadingDone)
      return;
    this.onStatusChange(gBrowser.webProgress, null, 0, aMessage);
  }
};

var LinkTargetDisplay = {
  get DELAY_SHOW() {
     delete this.DELAY_SHOW;
     return this.DELAY_SHOW = Services.prefs.getIntPref("browser.overlink-delay");
  },

  DELAY_HIDE: 250,
  _timer: 0,

  get _isVisible () XULBrowserWindow.statusTextField.label != "",

  update: function () {
    clearTimeout(this._timer);
    window.removeEventListener("mousemove", this, true);

    if (!XULBrowserWindow.overLink) {
      if (XULBrowserWindow.hideOverLinkImmediately)
        this._hide();
      else
        this._timer = setTimeout(this._hide.bind(this), this.DELAY_HIDE);
      return;
    }

    if (this._isVisible) {
      XULBrowserWindow.updateStatusField();
    } else {
      
      this._showDelayed();
      window.addEventListener("mousemove", this, true);
    }
  },

  handleEvent: function (event) {
    switch (event.type) {
      case "mousemove":
        
        clearTimeout(this._timer);
        this._showDelayed();
        break;
    }
  },

  _showDelayed: function () {
    this._timer = setTimeout(function (self) {
      XULBrowserWindow.updateStatusField();
      window.removeEventListener("mousemove", self, true);
    }, this.DELAY_SHOW, this);
  },

  _hide: function () {
    clearTimeout(this._timer);

    XULBrowserWindow.updateStatusField();
  }
};

var CombinedStopReload = {
  init: function () {
    if (this._initialized)
      return;

    let reload = document.getElementById("urlbar-reload-button");
    let stop = document.getElementById("urlbar-stop-button");
    if (!stop || !reload || reload.nextSibling != stop)
      return;

    this._initialized = true;
    if (XULBrowserWindow.stopCommand.getAttribute("disabled") != "true")
      reload.setAttribute("displaystop", "true");
    stop.addEventListener("click", this, false);
    this.reload = reload;
    this.stop = stop;
  },

  uninit: function () {
    if (!this._initialized)
      return;

    this._cancelTransition();
    this._initialized = false;
    this.stop.removeEventListener("click", this, false);
    this.reload = null;
    this.stop = null;
  },

  handleEvent: function (event) {
    
    if (event.button == 0 &&
        !this.stop.disabled)
      this._stopClicked = true;
  },

  switchToStop: function () {
    if (!this._initialized)
      return;

    this._cancelTransition();
    this.reload.setAttribute("displaystop", "true");
  },

  switchToReload: function (aDelay) {
    if (!this._initialized)
      return;

    this.reload.removeAttribute("displaystop");

    if (!aDelay || this._stopClicked) {
      this._stopClicked = false;
      this._cancelTransition();
      this.reload.disabled = XULBrowserWindow.reloadCommand
                                             .getAttribute("disabled") == "true";
      return;
    }

    if (this._timer)
      return;

    
    
    this.reload.disabled = true;
    this._timer = setTimeout(function (self) {
      self._timer = 0;
      self.reload.disabled = XULBrowserWindow.reloadCommand
                                             .getAttribute("disabled") == "true";
    }, 650, this);
  },

  _cancelTransition: function () {
    if (this._timer) {
      clearTimeout(this._timer);
      this._timer = 0;
    }
  }
};

var TabsProgressListener = {
  onStateChange: function (aBrowser, aWebProgress, aRequest, aStateFlags, aStatus) {
    
    if (aWebProgress.isTopLevel) {
      if (aStateFlags & Ci.nsIWebProgressListener.STATE_IS_WINDOW) {
        if (aStateFlags & Ci.nsIWebProgressListener.STATE_START) {
          TelemetryStopwatch.start("FX_PAGE_LOAD_MS", aBrowser);
          Services.telemetry.getHistogramById("FX_TOTAL_TOP_VISITS").add(true);
        } else if (aStateFlags & Ci.nsIWebProgressListener.STATE_STOP) {
          TelemetryStopwatch.finish("FX_PAGE_LOAD_MS", aBrowser);
        }
      } else if (aStateFlags & Ci.nsIWebProgressListener.STATE_STOP &&
                 aStatus == Cr.NS_BINDING_ABORTED) {
        TelemetryStopwatch.cancel("FX_PAGE_LOAD_MS", aBrowser);
      }
    }

    
    
    
    
    
    

    let isRemoteBrowser = aBrowser.isRemoteBrowser;
    
    let doc = isRemoteBrowser ? null : aWebProgress.DOMWindow.document;

    if (!isRemoteBrowser &&
        aStateFlags & Ci.nsIWebProgressListener.STATE_STOP &&
        Components.isSuccessCode(aStatus) &&
        doc.documentURI.startsWith("about:") &&
        !doc.documentURI.toLowerCase().startsWith("about:blank") &&
        !doc.documentURI.toLowerCase().startsWith("about:home") &&
        !doc.documentElement.hasAttribute("hasBrowserHandlers")) {
      
      
      doc.documentElement.setAttribute("hasBrowserHandlers", "true");
      aBrowser.addEventListener("click", BrowserOnClick, true);
      aBrowser.addEventListener("pagehide", function onPageHide(event) {
        if (event.target.defaultView.frameElement)
          return;
        aBrowser.removeEventListener("click", BrowserOnClick, true);
        aBrowser.removeEventListener("pagehide", onPageHide, true);
        if (event.target.documentElement)
          event.target.documentElement.removeAttribute("hasBrowserHandlers");
      }, true);
    }
  },

  onLocationChange: function (aBrowser, aWebProgress, aRequest, aLocationURI,
                              aFlags) {
    
    
    if (aFlags & Ci.nsIWebProgressListener.LOCATION_CHANGE_SAME_DOCUMENT) {
      
      let mm = gBrowser.selectedBrowser.messageManager;
      mm.sendAsyncMessage("Reader:PushState");
      return;
    }

    
    if (!aWebProgress.isTopLevel)
      return;

    
    
    
    if (!Object.getOwnPropertyDescriptor(window, "PopupNotifications").get)
      PopupNotifications.locationChange(aBrowser);

    gBrowser.getNotificationBox(aBrowser).removeTransientNotifications();

    FullZoom.onLocationChange(aLocationURI, false, aBrowser);
  },

  onRefreshAttempted: function (aBrowser, aWebProgress, aURI, aDelay, aSameURI) {
    if (gPrefService.getBoolPref("accessibility.blockautorefresh")) {
      let brandBundle = document.getElementById("bundle_brand");
      let brandShortName = brandBundle.getString("brandShortName");
      let refreshButtonText =
        gNavigatorBundle.getString("refreshBlocked.goButton");
      let refreshButtonAccesskey =
        gNavigatorBundle.getString("refreshBlocked.goButton.accesskey");
      let message =
        gNavigatorBundle.getFormattedString(aSameURI ? "refreshBlocked.refreshLabel"
                                                     : "refreshBlocked.redirectLabel",
                                            [brandShortName]);
      let docShell = aWebProgress.DOMWindow
                                 .QueryInterface(Ci.nsIInterfaceRequestor)
                                 .getInterface(Ci.nsIWebNavigation)
                                 .QueryInterface(Ci.nsIDocShell);
      let notificationBox = gBrowser.getNotificationBox(aBrowser);
      let notification = notificationBox.getNotificationWithValue("refresh-blocked");
      if (notification) {
        notification.label = message;
        notification.refreshURI = aURI;
        notification.delay = aDelay;
        notification.docShell = docShell;
      } else {
        let buttons = [{
          label: refreshButtonText,
          accessKey: refreshButtonAccesskey,
          callback: function (aNotification, aButton) {
            var refreshURI = aNotification.docShell
                                          .QueryInterface(Ci.nsIRefreshURI);
            refreshURI.forceRefreshURI(aNotification.refreshURI,
                                       aNotification.delay, true);
          }
        }];
        notification =
          notificationBox.appendNotification(message, "refresh-blocked",
                                             "chrome://browser/skin/Info.png",
                                             notificationBox.PRIORITY_INFO_MEDIUM,
                                             buttons);
        notification.refreshURI = aURI;
        notification.delay = aDelay;
        notification.docShell = docShell;
      }
      return false;
    }
    return true;
  }
}

function nsBrowserAccess() { }

nsBrowserAccess.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIBrowserDOMWindow, Ci.nsISupports]),

  _openURIInNewTab: function(aURI, aReferrer, aReferrerPolicy, aIsPrivate,
                             aIsExternal, aForceNotRemote=false) {
    let win, needToFocusWin;

    
    if (window.toolbar.visible)
      win = window;
    else {
      win = RecentWindow.getMostRecentBrowserWindow({private: aIsPrivate});
      needToFocusWin = true;
    }

    if (!win) {
      
      return null;
    }

    if (aIsExternal && (!aURI || aURI.spec == "about:blank")) {
      win.BrowserOpenTab(); 
      win.focus();
      return win.gBrowser.selectedBrowser;
    }

    let loadInBackground = gPrefService.getBoolPref("browser.tabs.loadDivertedInBackground");

    let tab = win.gBrowser.loadOneTab(aURI ? aURI.spec : "about:blank", {
                                      referrerURI: aReferrer,
                                      referrerPolicy: aReferrerPolicy,
                                      fromExternal: aIsExternal,
                                      inBackground: loadInBackground,
                                      forceNotRemote: aForceNotRemote});
    let browser = win.gBrowser.getBrowserForTab(tab);

    if (needToFocusWin || (!loadInBackground && aIsExternal))
      win.focus();

    return browser;
  },

  openURI: function (aURI, aOpener, aWhere, aContext) {
    
    
    if (aOpener && Cu.isCrossProcessWrapper(aOpener)) {
      Cu.reportError("nsBrowserAccess.openURI was passed a CPOW for aOpener. " +
                     "openURI should only ever be called from non-remote browsers.");
      throw Cr.NS_ERROR_FAILURE;
    }

    var newWindow = null;
    var isExternal = (aContext == Ci.nsIBrowserDOMWindow.OPEN_EXTERNAL);

    if (aOpener && isExternal) {
      Cu.reportError("nsBrowserAccess.openURI did not expect an opener to be " +
                     "passed if the context is OPEN_EXTERNAL.");
      throw Cr.NS_ERROR_FAILURE;
    }

    if (isExternal && aURI && aURI.schemeIs("chrome")) {
      dump("use --chrome command-line option to load external chrome urls\n");
      return null;
    }

    if (aWhere == Ci.nsIBrowserDOMWindow.OPEN_DEFAULTWINDOW) {
      if (isExternal &&
          gPrefService.prefHasUserValue("browser.link.open_newwindow.override.external"))
        aWhere = gPrefService.getIntPref("browser.link.open_newwindow.override.external");
      else
        aWhere = gPrefService.getIntPref("browser.link.open_newwindow");
    }

    let referrer = aOpener ? makeURI(aOpener.location.href) : null;
    let referrerPolicy = Ci.nsIHttpChannel.REFERRER_POLICY_DEFAULT;
    if (aOpener && aOpener.document) {
      referrerPolicy = aOpener.document.referrerPolicy;
    }
    let isPrivate = PrivateBrowsingUtils.isWindowPrivate(aOpener || window);

    switch (aWhere) {
      case Ci.nsIBrowserDOMWindow.OPEN_NEWWINDOW :
        
        
        var url = aURI ? aURI.spec : "about:blank";
        let features = "all,dialog=no";
        if (isPrivate) {
          features += ",private";
        }
        
        
        newWindow = openDialog(getBrowserURL(), "_blank", features, url, null, null, null);
        break;
      case Ci.nsIBrowserDOMWindow.OPEN_NEWTAB :
        
        
        
        
        
        
        let forceNotRemote = !!aOpener;
        let browser = this._openURIInNewTab(aURI, referrer, referrerPolicy,
                                            isPrivate, isExternal,
                                            forceNotRemote);
        if (browser)
          newWindow = browser.contentWindow;
        break;
      default : 
        newWindow = content;
        if (aURI) {
          let loadflags = isExternal ?
                            Ci.nsIWebNavigation.LOAD_FLAGS_FROM_EXTERNAL :
                            Ci.nsIWebNavigation.LOAD_FLAGS_NONE;
          gBrowser.loadURIWithFlags(aURI.spec, {
                                    flags: loadflags,
                                    referrerURI: referrer,
                                    referrerPolicy: referrerPolicy,
                                    });
        }
        if (!gPrefService.getBoolPref("browser.tabs.loadDivertedInBackground"))
          window.focus();
    }
    return newWindow;
  },

  openURIInFrame: function browser_openURIInFrame(aURI, aParams, aWhere, aContext) {
    if (aWhere != Ci.nsIBrowserDOMWindow.OPEN_NEWTAB) {
      dump("Error: openURIInFrame can only open in new tabs");
      return null;
    }

    var isExternal = (aContext == Ci.nsIBrowserDOMWindow.OPEN_EXTERNAL);
    let browser = this._openURIInNewTab(aURI, aParams.referrer,
                                        aParams.referrerPolicy,
                                        aParams.isPrivate, isExternal);
    if (browser)
      return browser.QueryInterface(Ci.nsIFrameLoaderOwner);

    return null;
  },

  isTabContentWindow: function (aWindow) {
    return gBrowser.browsers.some(function (browser) browser.contentWindow == aWindow);
  },
}

function getTogglableToolbars() {
  let toolbarNodes = Array.slice(gNavToolbox.childNodes);
  toolbarNodes = toolbarNodes.concat(gNavToolbox.externalToolbars);
  toolbarNodes = toolbarNodes.filter(node => node.getAttribute("toolbarname"));
  return toolbarNodes;
}

function onViewToolbarsPopupShowing(aEvent, aInsertPoint) {
  var popup = aEvent.target;
  if (popup != aEvent.currentTarget)
    return;

  
  for (var i = popup.childNodes.length-1; i >= 0; --i) {
    var deadItem = popup.childNodes[i];
    if (deadItem.hasAttribute("toolbarId"))
      popup.removeChild(deadItem);
  }

  var firstMenuItem = aInsertPoint || popup.firstChild;

  let toolbarNodes = getTogglableToolbars();

  for (let toolbar of toolbarNodes) {
    let menuItem = document.createElement("menuitem");
    let hidingAttribute = toolbar.getAttribute("type") == "menubar" ?
                          "autohide" : "collapsed";
    menuItem.setAttribute("id", "toggle_" + toolbar.id);
    menuItem.setAttribute("toolbarId", toolbar.id);
    menuItem.setAttribute("type", "checkbox");
    menuItem.setAttribute("label", toolbar.getAttribute("toolbarname"));
    menuItem.setAttribute("checked", toolbar.getAttribute(hidingAttribute) != "true");
    menuItem.setAttribute("accesskey", toolbar.getAttribute("accesskey"));
    if (popup.id != "toolbar-context-menu")
      menuItem.setAttribute("key", toolbar.getAttribute("key"));

    popup.insertBefore(menuItem, firstMenuItem);

    menuItem.addEventListener("command", onViewToolbarCommand, false);
  }


  let moveToPanel = popup.querySelector(".customize-context-moveToPanel");
  let removeFromToolbar = popup.querySelector(".customize-context-removeFromToolbar");
  
  if (!moveToPanel || !removeFromToolbar) {
    return;
  }

  
  let toolbarItem = popup.triggerNode;

  if (toolbarItem && toolbarItem.localName == "toolbarpaletteitem") {
    toolbarItem = toolbarItem.firstChild;
  } else if (toolbarItem && toolbarItem.localName != "toolbar") {
    while (toolbarItem && toolbarItem.parentNode) {
      let parent = toolbarItem.parentNode;
      if ((parent.classList && parent.classList.contains("customization-target")) ||
          parent.getAttribute("overflowfortoolbar") || 
          parent.localName == "toolbarpaletteitem" ||
          parent.localName == "toolbar")
        break;
      toolbarItem = parent;
    }
  } else {
    toolbarItem = null;
  }

  let showTabStripItems = toolbarItem && toolbarItem.id == "tabbrowser-tabs";
  for (let node of popup.querySelectorAll('menuitem[contexttype="toolbaritem"]')) {
    node.hidden = showTabStripItems;
  }

  for (let node of popup.querySelectorAll('menuitem[contexttype="tabbar"]')) {
    node.hidden = !showTabStripItems;
  }

  if (showTabStripItems) {
    PlacesCommandHook.updateBookmarkAllTabsCommand();

    let haveMultipleTabs = gBrowser.visibleTabs.length > 1;
    document.getElementById("toolbar-context-reloadAllTabs").disabled = !haveMultipleTabs;

    document.getElementById("toolbar-context-undoCloseTab").disabled =
      SessionStore.getClosedTabCount(window) == 0;
    return;
  }

  
  
  
  let movable = toolbarItem && toolbarItem.parentNode &&
                CustomizableUI.isWidgetRemovable(toolbarItem);
  if (movable) {
    moveToPanel.removeAttribute("disabled");
    removeFromToolbar.removeAttribute("disabled");
  } else {
    moveToPanel.setAttribute("disabled", true);
    removeFromToolbar.setAttribute("disabled", true);
  }
}

function onViewToolbarCommand(aEvent) {
  var toolbarId = aEvent.originalTarget.getAttribute("toolbarId");
  var isVisible = aEvent.originalTarget.getAttribute("checked") == "true";
  CustomizableUI.setToolbarVisibility(toolbarId, isVisible);
}

function setToolbarVisibility(toolbar, isVisible, persist=true) {
  let hidingAttribute;
  if (toolbar.getAttribute("type") == "menubar") {
    hidingAttribute = "autohide";
#ifdef MOZ_WIDGET_GTK
    Services.prefs.setBoolPref("ui.key.menuAccessKeyFocuses", !isVisible);
#endif
  } else {
    hidingAttribute = "collapsed";
  }

  toolbar.setAttribute(hidingAttribute, !isVisible);
  if (persist) {
    document.persist(toolbar.id, hidingAttribute);
  }

  let eventParams = {
    detail: {
      visible: isVisible
    },
    bubbles: true
  };
  let event = new CustomEvent("toolbarvisibilitychange", eventParams);
  toolbar.dispatchEvent(event);

  PlacesToolbarHelper.init();
  BookmarkingUI.onToolbarVisibilityChange();
  gBrowser.updateWindowResizers();
  if (isVisible)
    ToolbarIconColor.inferFromText();
}

var TabsInTitlebar = {
  init: function () {
#ifdef CAN_DRAW_IN_TITLEBAR
    this._readPref();
    Services.prefs.addObserver(this._prefName, this, false);

    
    
    
    
    
    
    
    
    
    
    let menu = document.getElementById("toolbar-menubar");
    this._menuObserver = new MutationObserver(this._onMenuMutate);
    this._menuObserver.observe(menu, {attributes: true});

    this.onAreaReset = function(aArea) {
      if (aArea == CustomizableUI.AREA_TABSTRIP || aArea == CustomizableUI.AREA_MENUBAR)
        this._update(true);
    };
    this.onWidgetAdded = this.onWidgetRemoved = function(aWidgetId, aArea) {
      if (aArea == CustomizableUI.AREA_TABSTRIP || aArea == CustomizableUI.AREA_MENUBAR)
        this._update(true);
    };
    CustomizableUI.addListener(this);

    this._initialized = true;
#endif
  },

  allowedBy: function (condition, allow) {
#ifdef CAN_DRAW_IN_TITLEBAR
    if (allow) {
      if (condition in this._disallowed) {
        delete this._disallowed[condition];
        this._update(true);
      }
    } else {
      if (!(condition in this._disallowed)) {
        this._disallowed[condition] = null;
        this._update(true);
      }
    }
#endif
  },

  updateAppearance: function updateAppearance(aForce) {
#ifdef CAN_DRAW_IN_TITLEBAR
    this._update(aForce);
#endif
  },

  get enabled() {
    return document.documentElement.getAttribute("tabsintitlebar") == "true";
  },

#ifdef CAN_DRAW_IN_TITLEBAR
  observe: function (subject, topic, data) {
    if (topic == "nsPref:changed")
      this._readPref();
  },

  _onMenuMutate: function (aMutations) {
    for (let mutation of aMutations) {
      if (mutation.attributeName == "inactive" ||
          mutation.attributeName == "autohide") {
        TabsInTitlebar._update(true);
        return;
      }
    }
  },

  _initialized: false,
  _disallowed: {},
  _prefName: "browser.tabs.drawInTitlebar",
  _lastSizeMode: null,

  _readPref: function () {
    this.allowedBy("pref",
                   Services.prefs.getBoolPref(this._prefName));
  },

  _update: function (aForce=false) {
    function $(id) document.getElementById(id);
    function rect(ele) ele.getBoundingClientRect();
    function verticalMargins(cstyle) parseFloat(cstyle.marginBottom) + parseFloat(cstyle.marginTop);

    if (!this._initialized || window.fullScreen)
      return;

    let allowed = true;

    if (!aForce) {
      
      
      
      
      let sizemode = document.documentElement.getAttribute("sizemode");
      if (this._lastSizeMode == sizemode) {
        return;
      }
      this._lastSizeMode = sizemode;
    }

    for (let something in this._disallowed) {
      allowed = false;
      break;
    }

    let titlebar = $("titlebar");
    let titlebarContent = $("titlebar-content");
    let menubar = $("toolbar-menubar");

    if (allowed) {
      
      
      document.documentElement.setAttribute("tabsintitlebar", "true");
      updateTitlebarDisplay();

      
      

      
      let tabsToolbar = $("TabsToolbar");
      let tabsStyles = window.getComputedStyle(tabsToolbar);
      let fullTabsHeight = rect(tabsToolbar).height + verticalMargins(tabsStyles);
      
      let captionButtonsBoxWidth = rect($("titlebar-buttonbox-container")).width;

#ifdef XP_MACOSX
      let secondaryButtonsWidth = rect($("titlebar-secondary-buttonbox")).width;
      
      let menuHeight = 0;
      let fullMenuHeight = 0;
#else
      
      let menuHeight = rect(menubar).height;
      let menuStyles = window.getComputedStyle(menubar);
      let fullMenuHeight = verticalMargins(menuStyles) + menuHeight;
#endif

      
      let titlebarContentHeight = rect(titlebarContent).height;

      

      
      
      
      if (menuHeight) {
        
        let menuTitlebarDelta = titlebarContentHeight - fullMenuHeight;
        let paddingBottom;
        
        if (menuTitlebarDelta > 0) {
          fullMenuHeight += menuTitlebarDelta;
          
          
          if ((paddingBottom = menuStyles.paddingBottom)) {
            menuTitlebarDelta += parseFloat(paddingBottom);
          }
          menubar.style.paddingBottom = menuTitlebarDelta + "px";
        
        } else if (menuTitlebarDelta < 0 && (paddingBottom = menuStyles.paddingBottom)) {
          let existingPadding = parseFloat(paddingBottom);
          
          let desiredPadding = Math.max(0, existingPadding + menuTitlebarDelta);
          menubar.style.paddingBottom = desiredPadding + "px";
          
          fullMenuHeight += desiredPadding - existingPadding;
        }
      }

      
      
      let tabAndMenuHeight = fullTabsHeight + fullMenuHeight;

      if (tabAndMenuHeight > titlebarContentHeight) {
        
        
        let extraMargin = tabAndMenuHeight - titlebarContentHeight;
#ifndef XP_MACOSX
        titlebarContent.style.marginBottom = extraMargin + "px";
#endif
        titlebarContentHeight += extraMargin;
      }

      
      titlebar.style.marginBottom = "-" + titlebarContentHeight + "px";


      
#ifdef XP_MACOSX
      this._sizePlaceholder("fullscreen-button", secondaryButtonsWidth);
#endif
      this._sizePlaceholder("caption-buttons", captionButtonsBoxWidth);

      if (!this._draghandles) {
        this._draghandles = {};
        let tmp = {};
        Components.utils.import("resource://gre/modules/WindowDraggingUtils.jsm", tmp);

        let mouseDownCheck = function () {
          return !this._dragBindingAlive && TabsInTitlebar.enabled;
        };

        this._draghandles.tabsToolbar = new tmp.WindowDraggingElement(tabsToolbar);
        this._draghandles.tabsToolbar.mouseDownCheck = mouseDownCheck;

        this._draghandles.navToolbox = new tmp.WindowDraggingElement(gNavToolbox);
        this._draghandles.navToolbox.mouseDownCheck = mouseDownCheck;
      }
    } else {
      document.documentElement.removeAttribute("tabsintitlebar");
      updateTitlebarDisplay();

#ifdef XP_MACOSX
      let secondaryButtonsWidth = rect($("titlebar-secondary-buttonbox")).width;
      this._sizePlaceholder("fullscreen-button", secondaryButtonsWidth);
#endif
      
      titlebarContent.style.marginTop = "";
      titlebarContent.style.marginBottom = "";
      titlebar.style.marginBottom = "";
      menubar.style.paddingBottom = "";
    }

    ToolbarIconColor.inferFromText();
    if (CustomizationHandler.isCustomizing()) {
      gCustomizeMode.updateLWTStyling();
    }
  },

  _sizePlaceholder: function (type, width) {
    Array.forEach(document.querySelectorAll(".titlebar-placeholder[type='"+ type +"']"),
                  function (node) { node.width = width; });
  },
#endif

  uninit: function () {
#ifdef CAN_DRAW_IN_TITLEBAR
    this._initialized = false;
    Services.prefs.removeObserver(this._prefName, this);
    this._menuObserver.disconnect();
    CustomizableUI.removeListener(this);
#endif
  }
};

#ifdef CAN_DRAW_IN_TITLEBAR
function updateTitlebarDisplay() {

#ifdef XP_MACOSX
  
  
  
  
  
  if (TabsInTitlebar.enabled) {
    document.documentElement.setAttribute("chromemargin-nonlwtheme", "0,-1,-1,-1");
    document.documentElement.setAttribute("chromemargin", "0,-1,-1,-1");
    document.documentElement.removeAttribute("drawtitle");
  } else {
    
    
    
    document.documentElement.setAttribute("chromemargin-nonlwtheme", "");
    let isCustomizing = document.documentElement.hasAttribute("customizing");
    let hasLWTheme = document.documentElement.hasAttribute("lwtheme");
    let isPrivate = PrivateBrowsingUtils.isWindowPrivate(window);
    if ((!hasLWTheme || isCustomizing) && !isPrivate) {
      document.documentElement.removeAttribute("chromemargin");
    }
    document.documentElement.setAttribute("drawtitle", "true");
  }

#else

  if (TabsInTitlebar.enabled)
    document.documentElement.setAttribute("chromemargin", "0,2,2,2");
  else
    document.documentElement.removeAttribute("chromemargin");
#endif
}
#endif

#ifdef CAN_DRAW_IN_TITLEBAR
function onTitlebarMaxClick() {
  if (window.windowState == window.STATE_MAXIMIZED)
    window.restore();
  else
    window.maximize();
}
#endif

function displaySecurityInfo()
{
  BrowserPageInfo(null, "securityTab");
}


var gHomeButton = {
  prefDomain: "browser.startup.homepage",
  observe: function (aSubject, aTopic, aPrefName)
  {
    if (aTopic != "nsPref:changed" || aPrefName != this.prefDomain)
      return;

    this.updateTooltip();
  },

  updateTooltip: function (homeButton)
  {
    if (!homeButton)
      homeButton = document.getElementById("home-button");
    if (homeButton) {
      var homePage = this.getHomePage();
      homePage = homePage.replace(/\|/g,', ');
      if (homePage.toLowerCase() == "about:home")
        homeButton.setAttribute("tooltiptext", homeButton.getAttribute("aboutHomeOverrideTooltip"));
      else
        homeButton.setAttribute("tooltiptext", homePage);
    }
  },

  getHomePage: function ()
  {
    var url;
    try {
      url = gPrefService.getComplexValue(this.prefDomain,
                                Components.interfaces.nsIPrefLocalizedString).data;
    } catch (e) {
    }

    
    if (!url) {
      var configBundle = Services.strings
                                 .createBundle("chrome://branding/locale/browserconfig.properties");
      url = configBundle.GetStringFromName(this.prefDomain);
    }

    return url;
  },

  updatePersonalToolbarStyle: function (homeButton)
  {
    if (!homeButton)
      homeButton = document.getElementById("home-button");
    if (homeButton)
      homeButton.className = homeButton.parentNode.id == "PersonalToolbar"
                               || homeButton.parentNode.parentNode.id == "PersonalToolbar" ?
                             homeButton.className.replace("toolbarbutton-1", "bookmark-item") :
                             homeButton.className.replace("bookmark-item", "toolbarbutton-1");
  },
};

const nodeToTooltipMap = {
  "bookmarks-menu-button": "bookmarksMenuButton.tooltip",
#ifdef XP_MACOSX
  "print-button": "printButton.tooltip",
#endif
  "new-window-button": "newWindowButton.tooltip",
  "new-tab-button": "newTabButton.tooltip",
  "tabs-newtab-button": "newTabButton.tooltip",
  "fullscreen-button": "fullscreenButton.tooltip",
  "tabview-button": "tabviewButton.tooltip",
  "downloads-button": "downloads.tooltip",
};
const nodeToShortcutMap = {
  "bookmarks-menu-button": "manBookmarkKb",
#ifdef XP_MACOSX
  "print-button": "printKb",
#endif
  "new-window-button": "key_newNavigator",
  "new-tab-button": "key_newNavigatorTab",
  "tabs-newtab-button": "key_newNavigatorTab",
  "fullscreen-button": "key_fullScreen",
  "tabview-button": "key_tabview",
  "downloads-button": "key_openDownloads"
};
const gDynamicTooltipCache = new Map();
function UpdateDynamicShortcutTooltipText(aTooltip) {
  let nodeId = aTooltip.triggerNode.id || aTooltip.triggerNode.getAttribute("anonid");
  if (!gDynamicTooltipCache.has(nodeId) && nodeId in nodeToTooltipMap) {
    let strId = nodeToTooltipMap[nodeId];
    let args = [];
    if (nodeId in nodeToShortcutMap) {
      let shortcutId = nodeToShortcutMap[nodeId];
      let shortcut = document.getElementById(shortcutId);
      if (shortcut) {
        args.push(ShortcutUtils.prettifyShortcut(shortcut));
      }
    }
    gDynamicTooltipCache.set(nodeId, gNavigatorBundle.getFormattedString(strId, args));
  }
  aTooltip.setAttribute("label", gDynamicTooltipCache.get(nodeId));
}










function getBrowserSelection(aCharLen) {
  
  const kMaxSelectionLen = 150;
  const charLen = Math.min(aCharLen || kMaxSelectionLen, kMaxSelectionLen);

  let [element, focusedWindow] = BrowserUtils.getFocusSync(document);
  var selection = focusedWindow.getSelection().toString();
  
  if (!selection) {
    var isOnTextInput = function isOnTextInput(elem) {
      
      
      return elem instanceof HTMLTextAreaElement ||
             (elem instanceof HTMLInputElement && elem.mozIsTextField(true));
    };

    if (isOnTextInput(element)) {
      selection = element.QueryInterface(Ci.nsIDOMNSEditableElement)
                         .editor.selection.toString();
    }
  }

  if (selection) {
    if (selection.length > charLen) {
      
      var pattern = new RegExp("^(?:\\s*.){0," + charLen + "}");
      pattern.test(selection);
      selection = RegExp.lastMatch;
    }

    selection = selection.trim().replace(/\s+/g, " ");

    if (selection.length > charLen)
      selection = selection.substr(0, charLen);
  }
  return selection;
}

var gWebPanelURI;
function openWebPanel(title, uri) {
  
  SidebarUI.show("viewWebPanelsSidebar");

  
  SidebarUI.title = title;

  
  if (SidebarUI.browser.docShell && SidebarUI.browser.contentDocument &&
      SidebarUI.browser.contentDocument.getElementById("web-panels-browser")) {
    SidebarUI.browser.contentWindow.loadWebPanel(uri);
    if (gWebPanelURI) {
      gWebPanelURI = "";
      SidebarUI.browser.removeEventListener("load", asyncOpenWebPanel, true);
    }
  } else {
    
    if (!gWebPanelURI) {
      SidebarUI.browser.addEventListener("load", asyncOpenWebPanel, true);
    }
    gWebPanelURI = uri;
  }
}

function asyncOpenWebPanel(event) {
  if (gWebPanelURI && SidebarUI.browser.contentDocument &&
      SidebarUI.browser.contentDocument.getElementById("web-panels-browser")) {
    SidebarUI.browser.contentWindow.loadWebPanel(gWebPanelURI);
  }
  gWebPanelURI = "";
  SidebarUI.browser.removeEventListener("load", asyncOpenWebPanel, true);
}

















function hrefAndLinkNodeForClickEvent(event)
{
  function isHTMLLink(aNode)
  {
    
    return ((aNode instanceof HTMLAnchorElement && aNode.href) ||
            (aNode instanceof HTMLAreaElement && aNode.href) ||
            aNode instanceof HTMLLinkElement);
  }

  let node = event.target;
  while (node && !isHTMLLink(node)) {
    node = node.parentNode;
  }

  if (node)
    return [node.href, node];

  
  let href, baseURI;
  node = event.target;
  while (node && !href) {
    if (node.nodeType == Node.ELEMENT_NODE) {
      href = node.getAttributeNS("http://www.w3.org/1999/xlink", "href");
      if (href)
        baseURI = node.baseURI;
    }
    node = node.parentNode;
  }

  
  
  return [href ? makeURLAbsolute(baseURI, href) : null, null];
}










function contentAreaClick(event, isPanelClick)
{
  if (!event.isTrusted || event.defaultPrevented || event.button == 2)
    return;

  let [href, linkNode] = hrefAndLinkNodeForClickEvent(event);
  if (!href) {
    
    if (event.button == 1 &&
        gPrefService.getBoolPref("middlemouse.contentLoadURL") &&
        !gPrefService.getBoolPref("general.autoScroll")) {
      middleMousePaste(event);
      event.preventDefault();
    }
    return;
  }

  
  
  if (linkNode && event.button == 0 &&
      !event.ctrlKey && !event.shiftKey && !event.altKey && !event.metaKey) {
    
    
    
    let target = linkNode.target;
    let mainTarget = !target || target == "_content" || target  == "_main";
    if (isPanelClick && mainTarget) {
      
      if (linkNode.getAttribute("onclick") ||
          href.startsWith("javascript:") ||
          href.startsWith("data:"))
        return;

      try {
        urlSecurityCheck(href, linkNode.ownerDocument.nodePrincipal);
      }
      catch(ex) {
        
        event.preventDefault();
        return;
      }

      loadURI(href, null, null, false);
      event.preventDefault();
      return;
    }

    if (linkNode.getAttribute("rel") == "sidebar") {
      
      
      
      PlacesUIUtils.showBookmarkDialog({ action: "add"
                                       , type: "bookmark"
                                       , uri: makeURI(href)
                                       , title: linkNode.getAttribute("title")
                                       , loadBookmarkInSidebar: true
                                       , hiddenRows: [ "description"
                                                     , "location"
                                                     , "keyword" ]
                                       }, window);
      event.preventDefault();
      return;
    }
  }

  handleLinkClick(event, href, linkNode);

  
  
  
  
  try {
    if (!PrivateBrowsingUtils.isWindowPrivate(window))
      PlacesUIUtils.markPageAsFollowedLink(href);
  } catch (ex) {  }
}






function handleLinkClick(event, href, linkNode) {
  if (event.button == 2) 
    return false;

  var where = whereToOpenLink(event);
  if (where == "current")
    return false;

  var doc = event.target.ownerDocument;

  if (where == "save") {
    saveURL(href, linkNode ? gatherTextUnder(linkNode) : "", null, true,
            true, doc.documentURIObject, doc);
    event.preventDefault();
    return true;
  }

  var referrerURI = doc.documentURIObject;
  
  
  
  var persistAllowMixedContentInChildTab = false;

  if (where == "tab" && gBrowser.docShell.mixedContentChannel) {
    const sm = Services.scriptSecurityManager;
    try {
      var targetURI = makeURI(href);
      sm.checkSameOriginURI(referrerURI, targetURI, false);
      persistAllowMixedContentInChildTab = true;
    }
    catch (e) { }
  }

  urlSecurityCheck(href, doc.nodePrincipal);
  let params = { charset: doc.characterSet,
                 allowMixedContent: persistAllowMixedContentInChildTab,
                 referrerURI: referrerURI,
                 referrerPolicy: doc.referrerPolicy,
                 noReferrer: BrowserUtils.linkHasNoReferrer(linkNode) };
  openLinkIn(href, where, params);
  event.preventDefault();
  return true;
}

function middleMousePaste(event) {
  let clipboard = readFromClipboard();
  if (!clipboard)
    return;

  
  
  clipboard = clipboard.replace(/\s*\n\s*/g, "");

  clipboard = stripUnsafeProtocolOnPaste(clipboard);

  
  
  let where = whereToOpenLink(event, true, false);
  let lastLocationChange;
  if (where == "current") {
    lastLocationChange = gBrowser.selectedBrowser.lastLocationChange;
  }

  getShortcutOrURIAndPostData(clipboard).then(data => {
    try {
      makeURI(data.url);
    } catch (ex) {
      
      return;
    }

    try {
      addToUrlbarHistory(data.url);
    } catch (ex) {
      
      
      Cu.reportError(ex);
    }

    if (where != "current" ||
        lastLocationChange == gBrowser.selectedBrowser.lastLocationChange) {
      openUILink(data.url, event,
                 { ignoreButton: true,
                   disallowInheritPrincipal: !data.mayInheritPrincipal });
    }
  });

  event.stopPropagation();
}

function stripUnsafeProtocolOnPaste(pasteData) {
  
  
  return pasteData.replace(/^(?:\s*javascript:)+/i, "");
}

function handleDroppedLink(event, url, name)
{
  let lastLocationChange = gBrowser.selectedBrowser.lastLocationChange;

  getShortcutOrURIAndPostData(url).then(data => {
    if (data.url &&
        lastLocationChange == gBrowser.selectedBrowser.lastLocationChange)
      loadURI(data.url, null, data.postData, false);
  });

  
  
  event.preventDefault();
};

function BrowserSetForcedCharacterSet(aCharset)
{
  if (aCharset) {
    gBrowser.selectedBrowser.characterSet = aCharset;
    
    if (!PrivateBrowsingUtils.isWindowPrivate(window))
      PlacesUtils.setCharsetForURI(getWebNavigation().currentURI, aCharset);
  }
  BrowserCharsetReload();
}

function BrowserCharsetReload()
{
  BrowserReloadWithFlags(nsIWebNavigation.LOAD_FLAGS_CHARSET_CHANGE);
}

function UpdateCurrentCharset(target) {
  for (let menuItem of target.getElementsByTagName("menuitem")) {
    let isSelected = menuItem.getAttribute("charset") ===
                     CharsetMenu.foldCharset(gBrowser.selectedBrowser.characterSet);
    menuItem.setAttribute("checked", isSelected);
  }
}

var gPageStyleMenu = {

  
  
  
  
  _pageStyleSyncHandlers: new WeakMap(),

  init: function() {
    let mm = window.messageManager;
    mm.addMessageListener("PageStyle:SetSyncHandler", (msg) => {
      this._pageStyleSyncHandlers.set(msg.target.permanentKey, msg.objects.syncHandler);
    });
  },

  getAllStyleSheets: function () {
    let handler = this._pageStyleSyncHandlers.get(gBrowser.selectedBrowser.permanentKey);
    try {
      return handler.getAllStyleSheets();
    } catch (ex) {
      
      return [];
    }
  },

  _getStyleSheetInfo: function (browser) {
    let handler = this._pageStyleSyncHandlers.get(gBrowser.selectedBrowser.permanentKey);
    try {
      return handler.getStyleSheetInfo();
    } catch (ex) {
      
      return {styleSheets: [], authorStyleDisabled: false, preferredStyleSheetSet: true};
    }
  },

  fillPopup: function (menuPopup) {
    let styleSheetInfo = this._getStyleSheetInfo(gBrowser.selectedBrowser);
    var noStyle = menuPopup.firstChild;
    var persistentOnly = noStyle.nextSibling;
    var sep = persistentOnly.nextSibling;
    while (sep.nextSibling)
      menuPopup.removeChild(sep.nextSibling);

    let styleSheets = styleSheetInfo.styleSheets;
    var currentStyleSheets = {};
    var styleDisabled = styleSheetInfo.authorStyleDisabled;
    var haveAltSheets = false;
    var altStyleSelected = false;

    for (let currentStyleSheet of styleSheets) {
      if (!currentStyleSheet.disabled)
        altStyleSelected = true;

      haveAltSheets = true;

      let lastWithSameTitle = null;
      if (currentStyleSheet.title in currentStyleSheets)
        lastWithSameTitle = currentStyleSheets[currentStyleSheet.title];

      if (!lastWithSameTitle) {
        let menuItem = document.createElement("menuitem");
        menuItem.setAttribute("type", "radio");
        menuItem.setAttribute("label", currentStyleSheet.title);
        menuItem.setAttribute("data", currentStyleSheet.title);
        menuItem.setAttribute("checked", !currentStyleSheet.disabled && !styleDisabled);
        menuItem.setAttribute("oncommand", "gPageStyleMenu.switchStyleSheet(this.getAttribute('data'));");
        menuPopup.appendChild(menuItem);
        currentStyleSheets[currentStyleSheet.title] = menuItem;
      } else if (currentStyleSheet.disabled) {
        lastWithSameTitle.removeAttribute("checked");
      }
    }

    noStyle.setAttribute("checked", styleDisabled);
    persistentOnly.setAttribute("checked", !altStyleSelected && !styleDisabled);
    persistentOnly.hidden = styleSheetInfo.preferredStyleSheetSet ? haveAltSheets : false;
    sep.hidden = (noStyle.hidden && persistentOnly.hidden) || !haveAltSheets;
  },

  switchStyleSheet: function (title) {
    let mm = gBrowser.selectedBrowser.messageManager;
    mm.sendAsyncMessage("PageStyle:Switch", {title: title});
  },

  disableStyle: function () {
    let mm = gBrowser.selectedBrowser.messageManager;
    mm.sendAsyncMessage("PageStyle:Disable");
  },
};


var getAllStyleSheets   = gPageStyleMenu.getAllStyleSheets.bind(gPageStyleMenu);
var stylesheetFillPopup = gPageStyleMenu.fillPopup.bind(gPageStyleMenu);
function stylesheetSwitchAll(contentWindow, title) {
  
  
  
  gPageStyleMenu.switchStyleSheet(title);
}
function setStyleDisabled(disabled) {
  if (disabled)
    gPageStyleMenu.disableStyle();
}


var LanguageDetectionListener = {
  init: function() {
    window.messageManager.addMessageListener("Translation:DocumentState", msg => {
      Translation.documentStateReceived(msg.target, msg.data);
    });
  }
};


var BrowserOffline = {
  _inited: false,

  
  
  init: function ()
  {
    if (!this._uiElement)
      this._uiElement = document.getElementById("workOfflineMenuitemState");

    Services.obs.addObserver(this, "network:offline-status-changed", false);

    this._updateOfflineUI(Services.io.offline);

    this._inited = true;
  },

  uninit: function ()
  {
    if (this._inited) {
      Services.obs.removeObserver(this, "network:offline-status-changed");
    }
  },

  toggleOfflineStatus: function ()
  {
    var ioService = Services.io;

    
    try {
      ioService.manageOfflineStatus = false;
    } catch (ex) {
    }

    if (!ioService.offline && !this._canGoOffline()) {
      this._updateOfflineUI(false);
      return;
    }

    ioService.offline = !ioService.offline;
  },

  
  
  observe: function (aSubject, aTopic, aState)
  {
    if (aTopic != "network:offline-status-changed")
      return;

    this._updateOfflineUI(aState == "offline");
  },

  
  
  _canGoOffline: function ()
  {
    try {
      var cancelGoOffline = Cc["@mozilla.org/supports-PRBool;1"].createInstance(Ci.nsISupportsPRBool);
      Services.obs.notifyObservers(cancelGoOffline, "offline-requested", null);

      
      if (cancelGoOffline.data)
        return false;
    }
    catch (ex) {
    }

    return true;
  },

  _uiElement: null,
  _updateOfflineUI: function (aOffline)
  {
    var offlineLocked = gPrefService.prefIsLocked("network.online");
    if (offlineLocked)
      this._uiElement.setAttribute("disabled", "true");

    this._uiElement.setAttribute("checked", aOffline);
  }
};

var OfflineApps = {
  
  
  init: function ()
  {
    Services.obs.addObserver(this, "offline-cache-update-completed", false);
  },

  uninit: function ()
  {
    Services.obs.removeObserver(this, "offline-cache-update-completed");
  },

  handleEvent: function(event) {
    if (event.type == "MozApplicationManifest") {
      this.offlineAppRequested(event.originalTarget.defaultView);
    }
  },

  
  

  
  
  _getBrowserWindowForContentWindow: function(aContentWindow) {
    return aContentWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                         .getInterface(Ci.nsIWebNavigation)
                         .QueryInterface(Ci.nsIDocShellTreeItem)
                         .rootTreeItem
                         .QueryInterface(Ci.nsIInterfaceRequestor)
                         .getInterface(Ci.nsIDOMWindow)
                         .wrappedJSObject;
  },

  _getBrowserForContentWindow: function(aBrowserWindow, aContentWindow) {
    
    aContentWindow = aContentWindow.top;
    var browsers = aBrowserWindow.gBrowser.browsers;
    for (let browser of browsers) {
      if (browser.contentWindow == aContentWindow)
        return browser;
    }
    
    let browser = aContentWindow
                          .QueryInterface(Ci.nsIInterfaceRequestor)
                          .getInterface(Ci.nsIWebNavigation)
                          .QueryInterface(Ci.nsIDocShell)
                          .chromeEventHandler;
    if (browser.getAttribute("popupnotificationanchor"))
      return browser;
    return null;
  },

  _getManifestURI: function(aWindow) {
    if (!aWindow.document.documentElement)
      return null;

    var attr = aWindow.document.documentElement.getAttribute("manifest");
    if (!attr)
      return null;

    try {
      var contentURI = makeURI(aWindow.location.href, null, null);
      return makeURI(attr, aWindow.document.characterSet, contentURI);
    } catch (e) {
      return null;
    }
  },

  
  
  _getBrowserForCacheUpdate: function(aCacheUpdate) {
    
    var uri = this._getManifestURI(content);
    if (uri && uri.equals(aCacheUpdate.manifestURI)) {
      return gBrowser.selectedBrowser;
    }

    var browsers = gBrowser.browsers;
    for (let browser of browsers) {
      uri = this._getManifestURI(browser.contentWindow);
      if (uri && uri.equals(aCacheUpdate.manifestURI)) {
        return browser;
      }
    }

    
    browsers = document.querySelectorAll("iframe[popupnotificationanchor] | browser[popupnotificationanchor]");
    for (let browser of browsers) {
      uri = this._getManifestURI(browser.contentWindow);
      if (uri && uri.equals(aCacheUpdate.manifestURI)) {
        return browser;
      }
    }

    return null;
  },

  _warnUsage: function(aBrowser, aURI) {
    if (!aBrowser)
      return;

    let mainAction = {
      label: gNavigatorBundle.getString("offlineApps.manageUsage"),
      accessKey: gNavigatorBundle.getString("offlineApps.manageUsageAccessKey"),
      callback: OfflineApps.manage
    };

    let warnQuota = gPrefService.getIntPref("offline-apps.quota.warn");
    let message = gNavigatorBundle.getFormattedString("offlineApps.usage",
                                                      [ aURI.host,
                                                        warnQuota / 1024 ]);

    let anchorID = "indexedDB-notification-icon";
    PopupNotifications.show(aBrowser, "offline-app-usage", message,
                            anchorID, mainAction);

    
    
    Services.perms.add(aURI, "offline-app",
                       Ci.nsIOfflineCacheUpdateService.ALLOW_NO_WARN);
  },

  
  _getOfflineAppUsage: function (host, groups)
  {
    var cacheService = Cc["@mozilla.org/network/application-cache-service;1"].
                       getService(Ci.nsIApplicationCacheService);
    if (!groups)
      groups = cacheService.getGroups();

    var usage = 0;
    for (let group of groups) {
      var uri = Services.io.newURI(group, null, null);
      if (uri.asciiHost == host) {
        var cache = cacheService.getActiveCache(group);
        usage += cache.usage;
      }
    }

    return usage;
  },

  _checkUsage: function(aURI) {
    
    if (Services.perms.testExactPermission(aURI, "offline-app") !=
        Ci.nsIOfflineCacheUpdateService.ALLOW_NO_WARN) {
      var usage = this._getOfflineAppUsage(aURI.asciiHost);
      var warnQuota = gPrefService.getIntPref("offline-apps.quota.warn");
      if (usage >= warnQuota * 1024) {
        return true;
      }
    }

    return false;
  },

  offlineAppRequested: function(aContentWindow) {
    if (!gPrefService.getBoolPref("browser.offline-apps.notify")) {
      return;
    }

    let browserWindow = this._getBrowserWindowForContentWindow(aContentWindow);
    let browser = this._getBrowserForContentWindow(browserWindow,
                                                   aContentWindow);

    let currentURI = aContentWindow.document.documentURIObject;

    
    if (Services.perms.testExactPermission(currentURI, "offline-app") != Services.perms.UNKNOWN_ACTION)
      return;

    try {
      if (gPrefService.getBoolPref("offline-apps.allow_by_default")) {
        
        return;
      }
    } catch(e) {
      
    }

    let host = currentURI.asciiHost;
    let notificationID = "offline-app-requested-" + host;
    let notification = PopupNotifications.getNotification(notificationID, browser);

    if (notification) {
      notification.options.documents.push(aContentWindow.document);
    } else {
      let mainAction = {
        label: gNavigatorBundle.getString("offlineApps.allow"),
        accessKey: gNavigatorBundle.getString("offlineApps.allowAccessKey"),
        callback: function() {
          for (let document of notification.options.documents) {
            OfflineApps.allowSite(document);
          }
        }
      };
      let secondaryActions = [{
        label: gNavigatorBundle.getString("offlineApps.never"),
        accessKey: gNavigatorBundle.getString("offlineApps.neverAccessKey"),
        callback: function() {
          for (let document of notification.options.documents) {
            OfflineApps.disallowSite(document);
          }
        }
      }];
      let message = gNavigatorBundle.getFormattedString("offlineApps.available",
                                                        [ host ]);
      let anchorID = "indexedDB-notification-icon";
      let options= {
        documents : [ aContentWindow.document ]
      };
      notification = PopupNotifications.show(browser, notificationID, message,
                                             anchorID, mainAction,
                                             secondaryActions, options);
    }
  },

  allowSite: function(aDocument) {
    Services.perms.add(aDocument.documentURIObject, "offline-app", Services.perms.ALLOW_ACTION);

    
    
    
    this._startFetching(aDocument);
  },

  disallowSite: function(aDocument) {
    Services.perms.add(aDocument.documentURIObject, "offline-app", Services.perms.DENY_ACTION);
  },

  manage: function() {
    openAdvancedPreferences("networkTab");
  },

  _startFetching: function(aDocument) {
    if (!aDocument.documentElement)
      return;

    var manifest = aDocument.documentElement.getAttribute("manifest");
    if (!manifest)
      return;

    var manifestURI = makeURI(manifest, aDocument.characterSet,
                              aDocument.documentURIObject);

    var updateService = Cc["@mozilla.org/offlinecacheupdate-service;1"].
                        getService(Ci.nsIOfflineCacheUpdateService);
    updateService.scheduleUpdate(manifestURI, aDocument.documentURIObject, window);
  },

  
  
  observe: function (aSubject, aTopic, aState)
  {
    if (aTopic == "offline-cache-update-completed") {
      var cacheUpdate = aSubject.QueryInterface(Ci.nsIOfflineCacheUpdate);

      var uri = cacheUpdate.manifestURI;
      if (OfflineApps._checkUsage(uri)) {
        var browser = this._getBrowserForCacheUpdate(cacheUpdate);
        if (browser) {
          OfflineApps._warnUsage(browser, cacheUpdate.manifestURI);
        }
      }
    }
  }
};

var IndexedDBPromptHelper = {
  _permissionsPrompt: "indexedDB-permissions-prompt",
  _permissionsResponse: "indexedDB-permissions-response",

  _notificationIcon: "indexedDB-notification-icon",

  init:
  function IndexedDBPromptHelper_init() {
    Services.obs.addObserver(this, this._permissionsPrompt, false);
  },

  uninit:
  function IndexedDBPromptHelper_uninit() {
    Services.obs.removeObserver(this, this._permissionsPrompt);
  },

  observe:
  function IndexedDBPromptHelper_observe(subject, topic, data) {
    if (topic != this._permissionsPrompt) {
      throw new Error("Unexpected topic!");
    }

    var requestor = subject.QueryInterface(Ci.nsIInterfaceRequestor);

    var browser = requestor.getInterface(Ci.nsIDOMNode);
    if (browser.ownerDocument.defaultView != window) {
      
      return;
    }

    var host = browser.currentURI.asciiHost;

    var message;
    var responseTopic;
    if (topic == this._permissionsPrompt) {
      message = gNavigatorBundle.getFormattedString("offlineApps.available",
                                                    [ host ]);
      responseTopic = this._permissionsResponse;
    }

    const hiddenTimeoutDuration = 30000; 
    const firstTimeoutDuration = 300000; 

    var timeoutId;

    var observer = requestor.getInterface(Ci.nsIObserver);

    var mainAction = {
      label: gNavigatorBundle.getString("offlineApps.allow"),
      accessKey: gNavigatorBundle.getString("offlineApps.allowAccessKey"),
      callback: function() {
        clearTimeout(timeoutId);
        observer.observe(null, responseTopic,
                         Ci.nsIPermissionManager.ALLOW_ACTION);
      }
    };

    var secondaryActions = [
      {
        label: gNavigatorBundle.getString("offlineApps.never"),
        accessKey: gNavigatorBundle.getString("offlineApps.neverAccessKey"),
        callback: function() {
          clearTimeout(timeoutId);
          observer.observe(null, responseTopic,
                           Ci.nsIPermissionManager.DENY_ACTION);
        }
      }
    ];

    
    var notification;

    function timeoutNotification() {
      
      if (notification) {
        notification.remove();
      }

      
      
      clearTimeout(timeoutId);

      
      observer.observe(null, responseTopic,
                       Ci.nsIPermissionManager.UNKNOWN_ACTION);
    }

    var options = {
      eventCallback: function(state) {
        
        if (!timeoutId) {
          return;
        }

        
        if (state == "dismissed") {
          clearTimeout(timeoutId);
          timeoutId = setTimeout(timeoutNotification, hiddenTimeoutDuration);
          return;
        }

        
        
        if (state == "shown") {
          clearTimeout(timeoutId);
        }
      }
    };

    notification = PopupNotifications.show(browser, topic, message,
                                           this._notificationIcon, mainAction,
                                           secondaryActions, options);

    
    
    
    timeoutId = setTimeout(timeoutNotification, firstTimeoutDuration);
  }
};

function WindowIsClosing()
{
  if (TabView.isVisible()) {
    TabView.hide();
    return false;
  }

  if (!closeWindow(false, warnAboutClosingWindow))
    return false;

  
  if (gMultiProcessBrowser)
    return true;

  for (let browser of gBrowser.browsers) {
    let ds = browser.docShell;
    
    
    
    
    if (ds.contentViewer && !ds.contentViewer.permitUnload(true)) {
      
      
      
      
      window.getInterface(Ci.nsIDocShell).contentViewer.resetCloseWindow();
      return false;
    }
  }

  return true;
}






function warnAboutClosingWindow() {
  
  let isPBWindow = PrivateBrowsingUtils.isWindowPrivate(window) &&
        !PrivateBrowsingUtils.permanentPrivateBrowsing;
  if (!isPBWindow && !toolbar.visible)
    return gBrowser.warnAboutClosingTabs(gBrowser.closingTabsEnum.ALL);

  
  let otherPBWindowExists = false;
  let nonPopupPresent = false;
  for (let win of browserWindows()) {
    if (!win.closed && win != window) {
      if (isPBWindow && PrivateBrowsingUtils.isWindowPrivate(win))
        otherPBWindowExists = true;
      if (win.toolbar.visible)
        nonPopupPresent = true;
      
      
      
      
      
      if ((!isPBWindow || otherPBWindowExists) && nonPopupPresent)
        break;
    }
  }

  if (isPBWindow && !otherPBWindowExists) {
    let exitingCanceled = Cc["@mozilla.org/supports-PRBool;1"].
                          createInstance(Ci.nsISupportsPRBool);
    exitingCanceled.data = false;
    Services.obs.notifyObservers(exitingCanceled,
                                 "last-pb-context-exiting",
                                 null);
    if (exitingCanceled.data)
      return false;
  }

  if (nonPopupPresent) {
    return isPBWindow || gBrowser.warnAboutClosingTabs(gBrowser.closingTabsEnum.ALL);
  }

  let os = Services.obs;

  let closingCanceled = Cc["@mozilla.org/supports-PRBool;1"].
                        createInstance(Ci.nsISupportsPRBool);
  os.notifyObservers(closingCanceled,
                     "browser-lastwindow-close-requested", null);
  if (closingCanceled.data)
    return false;

  os.notifyObservers(null, "browser-lastwindow-close-granted", null);

#ifdef XP_MACOSX
  
  
  
  return isPBWindow || gBrowser.warnAboutClosingTabs(gBrowser.closingTabsEnum.ALL);
#else
  return true;
#endif
}

var MailIntegration = {
  sendLinkForBrowser: function (aBrowser) {
    this.sendMessage(aBrowser.currentURI.spec, aBrowser.contentTitle);
  },

  sendMessage: function (aBody, aSubject) {
    
    var mailtoUrl = "mailto:";
    if (aBody) {
      mailtoUrl += "?body=" + encodeURIComponent(aBody);
      mailtoUrl += "&subject=" + encodeURIComponent(aSubject);
    }

    var uri = makeURI(mailtoUrl);

    
    this._launchExternalUrl(uri);
  },

  
  
  
  _launchExternalUrl: function (aURL) {
    var extProtocolSvc =
       Cc["@mozilla.org/uriloader/external-protocol-service;1"]
         .getService(Ci.nsIExternalProtocolService);
    if (extProtocolSvc)
      extProtocolSvc.loadUrl(aURL);
  }
};

function BrowserOpenAddonsMgr(aView) {
  if (aView) {
    let emWindow;
    let browserWindow;

    var receivePong = function receivePong(aSubject, aTopic, aData) {
      let browserWin = aSubject.QueryInterface(Ci.nsIInterfaceRequestor)
                               .getInterface(Ci.nsIWebNavigation)
                               .QueryInterface(Ci.nsIDocShellTreeItem)
                               .rootTreeItem
                               .QueryInterface(Ci.nsIInterfaceRequestor)
                               .getInterface(Ci.nsIDOMWindow);
      if (!emWindow || browserWin == window ) {
        emWindow = aSubject;
        browserWindow = browserWin;
      }
    }
    Services.obs.addObserver(receivePong, "EM-pong", false);
    Services.obs.notifyObservers(null, "EM-ping", "");
    Services.obs.removeObserver(receivePong, "EM-pong");

    if (emWindow) {
      emWindow.loadView(aView);
      browserWindow.gBrowser.selectedTab =
        browserWindow.gBrowser._getTabForContentWindow(emWindow);
      emWindow.focus();
      return;
    }
  }

  var newLoad = !switchToTabHavingURI("about:addons", true);

  if (aView) {
    
    
    Services.obs.addObserver(function observer(aSubject, aTopic, aData) {
      Services.obs.removeObserver(observer, aTopic);
      aSubject.loadView(aView);
    }, "EM-loaded", false);
  }
}

function BrowserOpenApps() {
  let appsURL = Services.urlFormatter.formatURLPref("browser.apps.URL");
  switchToTabHavingURI(appsURL, true)
}

function GetSearchFieldBookmarkData(node) {
  var charset = node.ownerDocument.characterSet;

  var formBaseURI = makeURI(node.form.baseURI,
                            charset);

  var formURI = makeURI(node.form.getAttribute("action"),
                        charset,
                        formBaseURI);

  var spec = formURI.spec;

  var isURLEncoded =
               (node.form.method.toUpperCase() == "POST"
                && (node.form.enctype == "application/x-www-form-urlencoded" ||
                    node.form.enctype == ""));

  var title = gNavigatorBundle.getFormattedString("addKeywordTitleAutoFill",
                                                  [node.ownerDocument.title]);
  var description = PlacesUIUtils.getDescriptionFromDocument(node.ownerDocument);

  var formData = [];

  function escapeNameValuePair(aName, aValue, aIsFormUrlEncoded) {
    if (aIsFormUrlEncoded)
      return escape(aName + "=" + aValue);
    else
      return escape(aName) + "=" + escape(aValue);
  }

  for (let el of node.form.elements) {
    if (!el.type) 
      continue;

    if (el == node) {
      formData.push((isURLEncoded) ? escapeNameValuePair(el.name, "%s", true) :
                                     
                                     escapeNameValuePair(el.name, "", false) + "%s");
      continue;
    }

    let type = el.type.toLowerCase();

    if (((el instanceof HTMLInputElement && el.mozIsTextField(true)) ||
        type == "hidden" || type == "textarea") ||
        ((type == "checkbox" || type == "radio") && el.checked)) {
      formData.push(escapeNameValuePair(el.name, el.value, isURLEncoded));
    } else if (el instanceof HTMLSelectElement && el.selectedIndex >= 0) {
      for (var j=0; j < el.options.length; j++) {
        if (el.options[j].selected)
          formData.push(escapeNameValuePair(el.name, el.options[j].value,
                                            isURLEncoded));
      }
    }
  }

  var postData;

  if (isURLEncoded)
    postData = formData.join("&");
  else {
    let separator = spec.contains("?") ? "&" : "?";
    spec += separator + formData.join("&");
  }

  return {
    spec: spec,
    title: title,
    description: description,
    postData: postData,
    charSet: charset
  };
}


function AddKeywordForSearchField() {
  let bookmarkData = GetSearchFieldBookmarkData(gContextMenu.target);

  PlacesUIUtils.showBookmarkDialog({ action: "add"
                                   , type: "bookmark"
                                   , uri: makeURI(bookmarkData.spec)
                                   , title: bookmarkData.title
                                   , description: bookmarkData.description
                                   , keyword: ""
                                   , postData: bookmarkData.postData
                                   , charSet: bookmarkData.charset
                                   , hiddenRows: [ "location"
                                                 , "description"
                                                 , "tags"
                                                 , "loadInSidebar" ]
                                   }, window);
}

function convertFromUnicode(charset, str)
{
  try {
    var unicodeConverter = Components
       .classes["@mozilla.org/intl/scriptableunicodeconverter"]
       .createInstance(Components.interfaces.nsIScriptableUnicodeConverter);
    unicodeConverter.charset = charset;
    str = unicodeConverter.ConvertFromUnicode(str);
    return str + unicodeConverter.Finish();
  } catch(ex) {
    return null;
  }
}







function undoCloseTab(aIndex) {
  
  var blankTabToRemove = null;
  if (gBrowser.tabs.length == 1 && isTabEmpty(gBrowser.selectedTab))
    blankTabToRemove = gBrowser.selectedTab;

  var tab = null;
  if (SessionStore.getClosedTabCount(window) > (aIndex || 0)) {
    TabView.prepareUndoCloseTab(blankTabToRemove);
    tab = SessionStore.undoCloseTab(window, aIndex || 0);
    TabView.afterUndoCloseTab();

    if (blankTabToRemove)
      gBrowser.removeTab(blankTabToRemove);
  }

  return tab;
}







function undoCloseWindow(aIndex) {
  let window = null;
  if (SessionStore.getClosedWindowCount() > (aIndex || 0))
    window = SessionStore.undoCloseWindow(aIndex || 0);

  return window;
}





function isTabEmpty(aTab) {
  if (aTab.hasAttribute("busy"))
    return false;

  let browser = aTab.linkedBrowser;
  if (!isBlankPageURL(browser.currentURI.spec))
    return false;

  
  if (!gMultiProcessBrowser && browser.contentWindow.opener)
    return false;

  if (browser.canGoForward || browser.canGoBack)
    return false;

  return true;
}

#ifdef MOZ_SERVICES_SYNC
function BrowserOpenSyncTabs() {
  switchToTabHavingURI("about:sync-tabs", true);
}
#endif









function formatURL(aFormat, aIsPref) {
  var formatter = Cc["@mozilla.org/toolkit/URLFormatterService;1"].getService(Ci.nsIURLFormatter);
  return aIsPref ? formatter.formatURLPref(aFormat) : formatter.formatURL(aFormat);
}




var gIdentityHandler = {
  
  IDENTITY_MODE_IDENTIFIED                             : "verifiedIdentity", 
  IDENTITY_MODE_DOMAIN_VERIFIED                        : "verifiedDomain",   
  IDENTITY_MODE_UNKNOWN                                : "unknownIdentity",  
  IDENTITY_MODE_MIXED_DISPLAY_LOADED                   : "unknownIdentity mixedContent mixedDisplayContent",  
  IDENTITY_MODE_MIXED_ACTIVE_LOADED                    : "unknownIdentity mixedContent mixedActiveContent",  
  IDENTITY_MODE_MIXED_DISPLAY_LOADED_ACTIVE_BLOCKED    : "unknownIdentity mixedContent mixedDisplayContentLoadedActiveBlocked",  
  IDENTITY_MODE_CHROMEUI                               : "chromeUI",         

  
  _lastStatus : null,
  _lastUri : null,
  _mode : "unknownIdentity",

  
  get _encryptionLabel () {
    delete this._encryptionLabel;
    this._encryptionLabel = {};
    this._encryptionLabel[this.IDENTITY_MODE_DOMAIN_VERIFIED] =
      gNavigatorBundle.getString("identity.encrypted2");
    this._encryptionLabel[this.IDENTITY_MODE_IDENTIFIED] =
      gNavigatorBundle.getString("identity.encrypted2");
    this._encryptionLabel[this.IDENTITY_MODE_UNKNOWN] =
      gNavigatorBundle.getString("identity.unencrypted");
    this._encryptionLabel[this.IDENTITY_MODE_MIXED_DISPLAY_LOADED] =
      gNavigatorBundle.getString("identity.broken_loaded");
    this._encryptionLabel[this.IDENTITY_MODE_MIXED_ACTIVE_LOADED] =
      gNavigatorBundle.getString("identity.mixed_active_loaded2");
    this._encryptionLabel[this.IDENTITY_MODE_MIXED_DISPLAY_LOADED_ACTIVE_BLOCKED] =
      gNavigatorBundle.getString("identity.broken_loaded");
    return this._encryptionLabel;
  },
  get _identityPopup () {
    delete this._identityPopup;
    return this._identityPopup = document.getElementById("identity-popup");
  },
  get _identityBox () {
    delete this._identityBox;
    return this._identityBox = document.getElementById("identity-box");
  },
  get _identityPopupContentBox () {
    delete this._identityPopupContentBox;
    return this._identityPopupContentBox =
      document.getElementById("identity-popup-content-box");
  },
  get _identityPopupChromeLabel () {
    delete this._identityPopupChromeLabel;
    return this._identityPopupChromeLabel =
      document.getElementById("identity-popup-chromeLabel");
  },
  get _identityPopupContentHost () {
    delete this._identityPopupContentHost;
    return this._identityPopupContentHost =
      document.getElementById("identity-popup-content-host");
  },
  get _identityPopupContentOwner () {
    delete this._identityPopupContentOwner;
    return this._identityPopupContentOwner =
      document.getElementById("identity-popup-content-owner");
  },
  get _identityPopupContentSupp () {
    delete this._identityPopupContentSupp;
    return this._identityPopupContentSupp =
      document.getElementById("identity-popup-content-supplemental");
  },
  get _identityPopupContentVerif () {
    delete this._identityPopupContentVerif;
    return this._identityPopupContentVerif =
      document.getElementById("identity-popup-content-verifier");
  },
  get _identityPopupEncLabel () {
    delete this._identityPopupEncLabel;
    return this._identityPopupEncLabel =
      document.getElementById("identity-popup-encryption-label");
  },
  get _identityIconLabel () {
    delete this._identityIconLabel;
    return this._identityIconLabel = document.getElementById("identity-icon-label");
  },
  get _overrideService () {
    delete this._overrideService;
    return this._overrideService = Cc["@mozilla.org/security/certoverride;1"]
                                     .getService(Ci.nsICertOverrideService);
  },
  get _identityIconCountryLabel () {
    delete this._identityIconCountryLabel;
    return this._identityIconCountryLabel = document.getElementById("identity-icon-country-label");
  },
  get _identityIcon () {
    delete this._identityIcon;
    return this._identityIcon = document.getElementById("page-proxy-favicon");
  },
  get _permissionsContainer () {
    delete this._permissionsContainer;
    return this._permissionsContainer = document.getElementById("identity-popup-permissions");
  },
  get _permissionList () {
    delete this._permissionList;
    return this._permissionList = document.getElementById("identity-popup-permission-list");
  },

  



  _cacheElements : function() {
    delete this._identityBox;
    delete this._identityIconLabel;
    delete this._identityIconCountryLabel;
    delete this._identityIcon;
    delete this._permissionsContainer;
    delete this._permissionList;
    this._identityBox = document.getElementById("identity-box");
    this._identityIconLabel = document.getElementById("identity-icon-label");
    this._identityIconCountryLabel = document.getElementById("identity-icon-country-label");
    this._identityIcon = document.getElementById("page-proxy-favicon");
    this._permissionsContainer = document.getElementById("identity-popup-permissions");
    this._permissionList = document.getElementById("identity-popup-permission-list");
  },

  


  handleHelpCommand : function(event) {
    openHelpLink("secure-connection");
    this._identityPopup.hidePopup();
  },

  



  handleMoreInfoClick : function(event) {
    displaySecurityInfo();
    event.stopPropagation();
    this._identityPopup.hidePopup();
  },

  



  getIdentityData : function() {
    var result = {};
    var status = this._lastStatus.QueryInterface(Components.interfaces.nsISSLStatus);
    var cert = status.serverCert;

    
    result.subjectOrg = cert.organization;

    
    if (cert.subjectName) {
      result.subjectNameFields = {};
      cert.subjectName.split(",").forEach(function(v) {
        var field = v.split("=");
        this[field[0]] = field[1];
      }, result.subjectNameFields);

      
      result.city = result.subjectNameFields.L;
      result.state = result.subjectNameFields.ST;
      result.country = result.subjectNameFields.C;
    }

    
    result.caOrg =  cert.issuerOrganization || cert.issuerCommonName;
    result.cert = cert;

    return result;
  },

  







  checkIdentity : function(state, uri) {
    var currentStatus = gBrowser.securityUI
                                .QueryInterface(Components.interfaces.nsISSLStatusProvider)
                                .SSLStatus;
    this._lastStatus = currentStatus;
    this._lastUri = uri;

    let nsIWebProgressListener = Ci.nsIWebProgressListener;

    
    
    let unknown = false;
    try {
      uri.host;
    } catch (e) { unknown = true; }

    
    
    let whitelist = /^about:(accounts|addons|app-manager|config|crashes|customizing|downloads|healthreport|home|license|newaddon|permissions|preferences|privatebrowsing|rights|sessionrestore|support|welcomeback)/i;
    let isChromeUI = uri.schemeIs("about") && whitelist.test(uri.spec);
    if (isChromeUI) {
      this.setMode(this.IDENTITY_MODE_CHROMEUI);
    } else if (unknown) {
      this.setMode(this.IDENTITY_MODE_UNKNOWN);
    } else if (state & nsIWebProgressListener.STATE_IDENTITY_EV_TOPLEVEL) {
      this.setMode(this.IDENTITY_MODE_IDENTIFIED);
    } else if (state & nsIWebProgressListener.STATE_IS_SECURE) {
      this.setMode(this.IDENTITY_MODE_DOMAIN_VERIFIED);
    } else if (state & nsIWebProgressListener.STATE_IS_BROKEN) {
      if (state & nsIWebProgressListener.STATE_LOADED_MIXED_ACTIVE_CONTENT) {
        this.setMode(this.IDENTITY_MODE_MIXED_ACTIVE_LOADED);
      } else if (state & nsIWebProgressListener.STATE_BLOCKED_MIXED_ACTIVE_CONTENT) {
        this.setMode(this.IDENTITY_MODE_MIXED_DISPLAY_LOADED_ACTIVE_BLOCKED);
      } else {
        this.setMode(this.IDENTITY_MODE_MIXED_DISPLAY_LOADED);
      }
    } else {
      this.setMode(this.IDENTITY_MODE_UNKNOWN);
    }

    
    
    
    
    
    if (state &
        (nsIWebProgressListener.STATE_BLOCKED_MIXED_ACTIVE_CONTENT |
         nsIWebProgressListener.STATE_LOADED_MIXED_ACTIVE_CONTENT  |
         nsIWebProgressListener.STATE_BLOCKED_TRACKING_CONTENT     |
         nsIWebProgressListener.STATE_LOADED_TRACKING_CONTENT)) {
      this.showBadContentDoorhanger(state);
    } else if (gPrefService.getBoolPref("privacy.trackingprotection.enabled")) {
      
      Services.telemetry.getHistogramById("TRACKING_PROTECTION_SHIELD")
        .add(0);
    }
  },

  showBadContentDoorhanger : function(state) {
    var currentNotification =
      PopupNotifications.getNotification("bad-content",
        gBrowser.selectedBrowser);

    
    if (currentNotification && currentNotification.options.state == state)
      return;

    let options = {
      
      dismissed: true,
      state: state
    };

    
    let iconState = "bad-content-blocked-notification-icon";

    
    let histogram = Services.telemetry.getHistogramById
                      ("TRACKING_PROTECTION_SHIELD");
    if (state & Ci.nsIWebProgressListener.STATE_LOADED_TRACKING_CONTENT) {
      histogram.add(1);
    } else if (state &
               Ci.nsIWebProgressListener.STATE_BLOCKED_TRACKING_CONTENT) {
      histogram.add(2);
    } else if (gPrefService.getBoolPref("privacy.trackingprotection.enabled")) {
      
      
      histogram.add(3);
    }
    if (state &
        (Ci.nsIWebProgressListener.STATE_LOADED_MIXED_ACTIVE_CONTENT |
         Ci.nsIWebProgressListener.STATE_LOADED_TRACKING_CONTENT)) {
      iconState = "bad-content-unblocked-notification-icon";
    }

    PopupNotifications.show(gBrowser.selectedBrowser, "bad-content",
                            "", iconState, null, null, options);
  },

  


  getEffectiveHost : function() {
    if (!this._IDNService)
      this._IDNService = Cc["@mozilla.org/network/idn-service;1"]
                         .getService(Ci.nsIIDNService);
    try {
      let baseDomain =
        Services.eTLD.getBaseDomainFromHost(this._lastUri.host);
      return this._IDNService.convertToDisplayIDN(baseDomain, {});
    } catch (e) {
      
      
      return this._lastUri.host;
    }
  },

  



  setMode : function(newMode) {
    if (!this._identityBox) {
      
      
      return;
    }

    this._identityPopup.className = newMode;
    this._identityBox.className = newMode;
    this.setIdentityMessages(newMode);

    
    if (this._identityPopup.state == "open")
      this.setPopupMessages(newMode);

    this._mode = newMode;
  },

  





  setIdentityMessages : function(newMode) {
    let icon_label = "";
    let tooltip = "";
    let icon_country_label = "";
    let icon_labels_dir = "ltr";

    switch (newMode) {
    case this.IDENTITY_MODE_DOMAIN_VERIFIED: {
      let iData = this.getIdentityData();

      
      
      tooltip = gNavigatorBundle.getFormattedString("identity.identified.verifier",
                                                    [iData.caOrg]);

      
      let host = this._lastUri.host;
      let port = 443;
      try {
        if (this._lastUri.port > 0)
          port = this._lastUri.port;
      } catch (e) {}

      if (this._overrideService.hasMatchingOverride(host, port, iData.cert, {}, {}))
        tooltip = gNavigatorBundle.getString("identity.identified.verified_by_you");

      break; }
    case this.IDENTITY_MODE_IDENTIFIED: {
      
      let iData = this.getIdentityData();
      tooltip = gNavigatorBundle.getFormattedString("identity.identified.verifier",
                                                    [iData.caOrg]);
      icon_label = iData.subjectOrg;
      if (iData.country)
        icon_country_label = "(" + iData.country + ")";

      
      
      
      
      
      
      icon_labels_dir = /^[\u0590-\u08ff\ufb1d-\ufdff\ufe70-\ufefc]/.test(icon_label) ?
                        "rtl" : "ltr";
      break; }
    case this.IDENTITY_MODE_CHROMEUI:
      let brandBundle = document.getElementById("bundle_brand");
      icon_label = brandBundle.getString("brandShorterName");
      break;
    default:
      tooltip = gNavigatorBundle.getString("identity.unknown.tooltip");
    }

    
    this._identityBox.tooltipText = tooltip;
    this._identityIconLabel.value = icon_label;
    this._identityIconCountryLabel.value = icon_country_label;
    
    this._identityIconLabel.crop = icon_country_label ? "end" : "center";
    this._identityIconLabel.parentNode.style.direction = icon_labels_dir;
    
    this._identityIconLabel.parentNode.collapsed = icon_label ? false : true;
  },

  






  setPopupMessages : function(newMode) {

    this._identityPopup.className = newMode;
    this._identityPopupContentBox.className = newMode;

    
    this._identityPopupEncLabel.textContent = this._encryptionLabel[newMode];

    
    let supplemental = "";
    let verifier = "";
    let host = "";
    let owner = "";

    switch (newMode) {
    case this.IDENTITY_MODE_DOMAIN_VERIFIED:
      host = this.getEffectiveHost();
      verifier = this._identityBox.tooltipText;
      break;
    case this.IDENTITY_MODE_IDENTIFIED: {
      
      let iData = this.getIdentityData();
      host = this.getEffectiveHost();
      owner = iData.subjectOrg;
      verifier = this._identityBox.tooltipText;

      
      if (iData.city)
        supplemental += iData.city + "\n";
      if (iData.state && iData.country)
        supplemental += gNavigatorBundle.getFormattedString("identity.identified.state_and_country",
                                                            [iData.state, iData.country]);
      else if (iData.state) 
        supplemental += iData.state;
      else if (iData.country) 
        supplemental += iData.country;
      break; }
    case this.IDENTITY_MODE_CHROMEUI: {
      let brandBundle = document.getElementById("bundle_brand");
      let brandShortName = brandBundle.getString("brandShortName");
      this._identityPopupChromeLabel.textContent = gNavigatorBundle.getFormattedString("identity.chrome",
                                                                                       [brandShortName]);
      break; }
    }

    
    this._identityPopupContentHost.textContent = host;
    this._identityPopupContentOwner.textContent = owner;
    this._identityPopupContentSupp.textContent = supplemental;
    this._identityPopupContentVerif.textContent = verifier;
  },

  


  handleIdentityButtonEvent : function(event) {
    event.stopPropagation();

    if ((event.type == "click" && event.button != 0) ||
        (event.type == "keypress" && event.charCode != KeyEvent.DOM_VK_SPACE &&
         event.keyCode != KeyEvent.DOM_VK_RETURN)) {
      return; 
    }

    
    if (gURLBar.getAttribute("pageproxystate") != "valid") {
      return;
    }

    
    
    this._identityPopup.hidden = false;

    
    this.setPopupMessages(this._identityBox.className);

    this.updateSitePermissions();

    
    this._identityBox.setAttribute("open", "true");
    var self = this;
    this._identityPopup.addEventListener("popuphidden", function onPopupHidden(e) {
      e.currentTarget.removeEventListener("popuphidden", onPopupHidden, false);
      self._identityBox.removeAttribute("open");
    }, false);

    
    this._identityPopup.openPopup(this._identityIcon, "bottomcenter topleft");
  },

  onPopupShown : function(event) {
    document.getElementById('identity-popup-more-info-button').focus();

    this._identityPopup.addEventListener("blur", this, true);
    this._identityPopup.addEventListener("popuphidden", this);
  },

  onDragStart: function (event) {
    if (gURLBar.getAttribute("pageproxystate") != "valid")
      return;

    let value = gBrowser.currentURI.spec;
    let urlString = value + "\n" + gBrowser.contentTitle;
    let htmlString = "<a href=\"" + value + "\">" + value + "</a>";

    let dt = event.dataTransfer;
    dt.setData("text/x-moz-url", urlString);
    dt.setData("text/uri-list", value);
    dt.setData("text/plain", value);
    dt.setData("text/html", htmlString);
    dt.setDragImage(gProxyFavIcon, 16, 16);
  },
 
  handleEvent: function (event) {
    switch (event.type) {
      case "blur":
        
        setTimeout(() => {
          if (document.activeElement &&
              document.activeElement.compareDocumentPosition(this._identityPopup) &
                Node.DOCUMENT_POSITION_CONTAINS)
            return;

          this._identityPopup.hidePopup();
        }, 0);
        break;
      case "popuphidden":
        this._identityPopup.removeEventListener("blur", this, true);
        this._identityPopup.removeEventListener("popuphidden", this);
        break;
    }
  },

  updateSitePermissions: function () {
    while (this._permissionList.hasChildNodes())
      this._permissionList.removeChild(this._permissionList.lastChild);

    let uri = gBrowser.currentURI;

    for (let permission of SitePermissions.listPermissions()) {
      let state = SitePermissions.get(uri, permission);

      if (state == SitePermissions.UNKNOWN)
        continue;

      let item = this._createPermissionItem(permission, state);
      this._permissionList.appendChild(item);
    }

    this._permissionsContainer.hidden = !this._permissionList.hasChildNodes();
  },

  setPermission: function (aPermission, aState) {
    if (aState == SitePermissions.getDefault(aPermission))
      SitePermissions.remove(gBrowser.currentURI, aPermission);
    else
      SitePermissions.set(gBrowser.currentURI, aPermission, aState);
  },

  _createPermissionItem: function (aPermission, aState) {
    let menulist = document.createElement("menulist");
    let menupopup = document.createElement("menupopup");
    for (let state of SitePermissions.getAvailableStates(aPermission)) {
      let menuitem = document.createElement("menuitem");
      menuitem.setAttribute("value", state);
      menuitem.setAttribute("label", SitePermissions.getStateLabel(aPermission, state));
      menupopup.appendChild(menuitem);
    }
    menulist.appendChild(menupopup);
    menulist.setAttribute("value", aState);
    menulist.setAttribute("oncommand", "gIdentityHandler.setPermission('" +
                                       aPermission + "', this.value)");
    menulist.setAttribute("id", "identity-popup-permission:" + aPermission);

    let label = document.createElement("label");
    label.setAttribute("flex", "1");
    label.setAttribute("control", menulist.getAttribute("id"));
    label.setAttribute("value", SitePermissions.getPermissionLabel(aPermission));

    let container = document.createElement("hbox");
    container.setAttribute("align", "center");
    container.appendChild(label);
    container.appendChild(menulist);
    return container;
  }
};

function getNotificationBox(aWindow) {
  var foundBrowser = gBrowser.getBrowserForDocument(aWindow.document);
  if (foundBrowser)
    return gBrowser.getNotificationBox(foundBrowser)
  return null;
};

function getTabModalPromptBox(aWindow) {
  var foundBrowser = gBrowser.getBrowserForDocument(aWindow.document);
  if (foundBrowser)
    return gBrowser.getTabModalPromptBox(foundBrowser);
  return null;
};


function getBrowser() gBrowser;
function getNavToolbox() gNavToolbox;

let gPrivateBrowsingUI = {
  init: function PBUI_init() {
    
    if (!PrivateBrowsingUtils.isWindowPrivate(window)) {
      return;
    }

    
    
    document.getElementById("Tools:Sanitize").setAttribute("disabled", "true");

    if (window.location.href == getBrowserURL()) {
      
      let docElement = document.documentElement;
      if (!PrivateBrowsingUtils.permanentPrivateBrowsing) {
        docElement.setAttribute("title",
          docElement.getAttribute("title_privatebrowsing"));
        docElement.setAttribute("titlemodifier",
          docElement.getAttribute("titlemodifier_privatebrowsing"));
      }
      docElement.setAttribute("privatebrowsingmode",
        PrivateBrowsingUtils.permanentPrivateBrowsing ? "permanent" : "temporary");
      gBrowser.updateTitlebar();

      if (PrivateBrowsingUtils.permanentPrivateBrowsing) {
        
        [
          { normal: "menu_newNavigator", private: "menu_newPrivateWindow" },
        ].forEach(function(menu) {
          let newWindow = document.getElementById(menu.normal);
          let newPrivateWindow = document.getElementById(menu.private);
          if (newWindow && newPrivateWindow) {
            newPrivateWindow.hidden = true;
            newWindow.label = newPrivateWindow.label;
            newWindow.accessKey = newPrivateWindow.accessKey;
            newWindow.command = newPrivateWindow.command;
          }
        });
      }
    }

    if (gURLBar &&
        !PrivateBrowsingUtils.permanentPrivateBrowsing) {
      
      
      let value = gURLBar.getAttribute("autocompletesearchparam") || "";
      if (!value.contains("disable-private-actions")) {
        gURLBar.setAttribute("autocompletesearchparam",
                             value + " disable-private-actions");
      }
    }
  }
};

let gRemoteTabsUI = {
  init: function() {
    if (window.location.href != getBrowserURL() &&
        
        window.location.href != "chrome://browser/content/hiddenWindow.xul") {
      return;
    }

    if (Services.appinfo.inSafeMode) {
      
      return;
    }

#ifdef XP_MACOSX
    if (Services.prefs.getBoolPref("layers.acceleration.disabled")) {
      
      
      return;
    }
#endif

    let newRemoteWindow = document.getElementById("menu_newRemoteWindow");
    let newNonRemoteWindow = document.getElementById("menu_newNonRemoteWindow");
    let autostart = Services.appinfo.browserTabsRemoteAutostart;
    newRemoteWindow.hidden = autostart;
    newNonRemoteWindow.hidden = !autostart;
  }
};


























function switchToTabHavingURI(aURI, aOpenNew, aOpenParams={}) {
  
  
  const kPrivateBrowsingWhitelist = new Set([
    "about:addons",
    "about:customizing",
  ]);

  let ignoreFragment = aOpenParams.ignoreFragment;
  let ignoreQueryString = aOpenParams.ignoreQueryString;
  let replaceQueryString = aOpenParams.replaceQueryString;

  
  
  delete aOpenParams.ignoreFragment;
  delete aOpenParams.ignoreQueryString;
  delete aOpenParams.replaceQueryString;

  
  function switchIfURIInWindow(aWindow) {
    
    
    if (!kPrivateBrowsingWhitelist.has(aURI.spec) &&
        (PrivateBrowsingUtils.isWindowPrivate(window) ||
         PrivateBrowsingUtils.isWindowPrivate(aWindow)) &&
        !PrivateBrowsingUtils.permanentPrivateBrowsing) {
      return false;
    }

    let browsers = aWindow.gBrowser.browsers;
    for (let i = 0; i < browsers.length; i++) {
      let browser = browsers[i];
      if (ignoreFragment ? browser.currentURI.equalsExceptRef(aURI) :
                           browser.currentURI.equals(aURI)) {
        
        aWindow.focus();
        if (ignoreFragment) {
          let spec = aURI.spec;
          if (!aURI.ref)
            spec += "#";
          browser.loadURI(spec);
        }
        aWindow.gBrowser.tabContainer.selectedIndex = i;
        return true;
      }
      if (ignoreQueryString || replaceQueryString) {
        if (browser.currentURI.spec.split("?")[0] == aURI.spec.split("?")[0]) {
          
          aWindow.focus();
          if (replaceQueryString) {
            browser.loadURI(aURI.spec);
          }
          aWindow.gBrowser.tabContainer.selectedIndex = i;
          return true;
        }
      }
    }
    return false;
  }

  
  if (!(aURI instanceof Ci.nsIURI))
    aURI = Services.io.newURI(aURI, null, null);

  let isBrowserWindow = !!window.gBrowser;

  
  if (isBrowserWindow && switchIfURIInWindow(window))
    return true;

  for (let browserWin of browserWindows()) {
    
    
    if (browserWin.closed || browserWin == window)
      continue;
    if (switchIfURIInWindow(browserWin))
      return true;
  }

  
  if (aOpenNew) {
    if (isBrowserWindow && isTabEmpty(gBrowser.selectedTab))
      openUILinkIn(aURI.spec, "current", aOpenParams);
    else
      openUILinkIn(aURI.spec, "tab", aOpenParams);
  }

  return false;
}

let RestoreLastSessionObserver = {
  init: function () {
    if (SessionStore.canRestoreLastSession &&
        !PrivateBrowsingUtils.isWindowPrivate(window)) {
      Services.obs.addObserver(this, "sessionstore-last-session-cleared", true);
      goSetCommandEnabled("Browser:RestoreLastSession", true);
    }
  },

  observe: function () {
    
    
    Services.obs.removeObserver(this, "sessionstore-last-session-cleared");
    goSetCommandEnabled("Browser:RestoreLastSession", false);
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference])
};

function restoreLastSession() {
  SessionStore.restoreLastSession();
}

var TabContextMenu = {
  contextTab: null,
  updateContextMenu: function updateContextMenu(aPopupMenu) {
    this.contextTab = aPopupMenu.triggerNode.localName == "tab" ?
                      aPopupMenu.triggerNode : gBrowser.selectedTab;
    let disabled = gBrowser.tabs.length == 1;

    var menuItems = aPopupMenu.getElementsByAttribute("tbattr", "tabbrowser-multiple");
    for (let menuItem of menuItems)
      menuItem.disabled = disabled;

#ifdef E10S_TESTING_ONLY
    menuItems = aPopupMenu.getElementsByAttribute("tbattr", "tabbrowser-remote");
    for (let menuItem of menuItems)
      menuItem.hidden = !gMultiProcessBrowser;
#endif

    disabled = gBrowser.visibleTabs.length == 1;
    menuItems = aPopupMenu.getElementsByAttribute("tbattr", "tabbrowser-multiple-visible");
    for (let menuItem of menuItems)
      menuItem.disabled = disabled;

    
    document.getElementById("context_undoCloseTab").disabled =
      SessionStore.getClosedTabCount(window) == 0;

    
    document.getElementById("context_pinTab").hidden = this.contextTab.pinned;
    document.getElementById("context_unpinTab").hidden = !this.contextTab.pinned;

    
    
    
    document.getElementById("context_closeTabsToTheEnd").disabled =
      gBrowser.getTabsToTheEndFrom(this.contextTab).length == 0;
    document.getElementById("context_closeTabsToTheEnd").hidden = this.contextTab.pinned;

    
    
    let unpinnedTabs = gBrowser.visibleTabs.length - gBrowser._numPinnedTabs;
    document.getElementById("context_closeOtherTabs").disabled = unpinnedTabs <= 1;
    document.getElementById("context_closeOtherTabs").hidden = this.contextTab.pinned;

    
    let bookmarkAllTabs = document.getElementById("context_bookmarkAllTabs");
    bookmarkAllTabs.hidden = this.contextTab.pinned;
    if (!bookmarkAllTabs.hidden)
      PlacesCommandHook.updateBookmarkAllTabsCommand();

    
    document.getElementById("context_tabViewMenu").hidden =
      (this.contextTab.pinned || !TabView.firstUseExperienced);
  }
};

XPCOMUtils.defineLazyModuleGetter(this, "gDevTools",
                                  "resource:///modules/devtools/gDevTools.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "gDevToolsBrowser",
                                  "resource:///modules/devtools/gDevTools.jsm");

Object.defineProperty(this, "HUDService", {
  get: function HUDService_getter() {
    let devtools = Cu.import("resource://gre/modules/devtools/Loader.jsm", {}).devtools;
    return devtools.require("devtools/webconsole/hudservice");
  },
  configurable: true,
  enumerable: true
});


function safeModeRestart() {
  Services.obs.notifyObservers(null, "restart-in-safe-mode", "");
}











function duplicateTabIn(aTab, where, delta) {
  let newTab = SessionStore.duplicateTab(window, aTab, delta);

  switch (where) {
    case "window":
      gBrowser.hideTab(newTab);
      gBrowser.replaceTabWithWindow(newTab);
      break;
    case "tabshifted":
      
      break;
    case "tab":
      gBrowser.selectedTab = newTab;
      break;
  }
}

var Scratchpad = {
  openScratchpad: function SP_openScratchpad() {
    return this.ScratchpadManager.openScratchpad();
  }
};

XPCOMUtils.defineLazyGetter(Scratchpad, "ScratchpadManager", function() {
  let tmp = {};
  Cu.import("resource:///modules/devtools/scratchpad-manager.jsm", tmp);
  return tmp.ScratchpadManager;
});

var ResponsiveUI = {
  toggle: function RUI_toggle() {
    this.ResponsiveUIManager.toggle(window, gBrowser.selectedTab);
  }
};

XPCOMUtils.defineLazyGetter(ResponsiveUI, "ResponsiveUIManager", function() {
  let tmp = {};
  Cu.import("resource:///modules/devtools/responsivedesign.jsm", tmp);
  return tmp.ResponsiveUIManager;
});

function openEyedropper() {
  var eyedropper = new this.Eyedropper(this, { context: "menu",
                                               copyOnSelect: true });
  eyedropper.open();
}

Object.defineProperty(this, "Eyedropper", {
  get: function() {
    let devtools = Cu.import("resource://gre/modules/devtools/Loader.jsm", {}).devtools;
    return devtools.require("devtools/eyedropper/eyedropper").Eyedropper;
  },
  configurable: true,
  enumerable: true
});

XPCOMUtils.defineLazyGetter(window, "gShowPageResizers", function () {
#ifdef XP_WIN
  
  return parseFloat(Services.sysinfo.getProperty("version")) < 6;
#else
  return false;
#endif
});

var MousePosTracker = {
  _listeners: new Set(),
  _x: 0,
  _y: 0,
  get _windowUtils() {
    delete this._windowUtils;
    return this._windowUtils = window.getInterface(Ci.nsIDOMWindowUtils);
  },

  addListener: function (listener) {
    if (this._listeners.has(listener))
      return;

    listener._hover = false;
    this._listeners.add(listener);

    this._callListener(listener);
  },

  removeListener: function (listener) {
    this._listeners.delete(listener);
  },

  handleEvent: function (event) {
    var fullZoom = this._windowUtils.fullZoom;
    this._x = event.screenX / fullZoom - window.mozInnerScreenX;
    this._y = event.screenY / fullZoom - window.mozInnerScreenY;

    this._listeners.forEach(function (listener) {
      try {
        this._callListener(listener);
      } catch (e) {
        Cu.reportError(e);
      }
    }, this);
  },

  _callListener: function (listener) {
    let rect = listener.getMouseTargetRect();
    let hover = this._x >= rect.left &&
                this._x <= rect.right &&
                this._y >= rect.top &&
                this._y <= rect.bottom;

    if (hover == listener._hover)
      return;

    listener._hover = hover;

    if (hover) {
      if (listener.onMouseEnter)
        listener.onMouseEnter();
    } else {
      if (listener.onMouseLeave)
        listener.onMouseLeave();
    }
  }
};

function focusNextFrame(event) {
  let fm = Services.focus;
  let dir = event.shiftKey ? fm.MOVEFOCUS_BACKWARDDOC : fm.MOVEFOCUS_FORWARDDOC;
  let element = fm.moveFocus(window, null, dir, fm.FLAG_BYKEY);
  let panelOrNotificationSelector = "popupnotification " + element.localName + ", " +
                                    "panel " + element.localName;
  if (element.ownerDocument == document && !element.matches(panelOrNotificationSelector))
    focusAndSelectUrlBar();
}

function BrowserOpenNewTabOrWindow(event) {
  if (event.shiftKey) {
    OpenBrowserWindow();
  } else {
    BrowserOpenTab();
  }
}

let ToolbarIconColor = {
  init: function () {
    this._initialized = true;

    window.addEventListener("activate", this);
    window.addEventListener("deactivate", this);
    Services.obs.addObserver(this, "lightweight-theme-styling-update", false);

    
    
    
    if (Services.focus.activeWindow == window)
      this.inferFromText();
  },

  uninit: function () {
    this._initialized = false;

    window.removeEventListener("activate", this);
    window.removeEventListener("deactivate", this);
    Services.obs.removeObserver(this, "lightweight-theme-styling-update");
  },

  handleEvent: function (event) {
    switch (event.type) {
      case "activate":
      case "deactivate":
        this.inferFromText();
        break;
    }
  },

  observe: function (aSubject, aTopic, aData) {
    switch (aTopic) {
      case "lightweight-theme-styling-update":
        
        
        setTimeout(() => { this.inferFromText(); }, 0);
        break;
    }
  },

  inferFromText: function () {
    if (!this._initialized)
      return;

    function parseRGB(aColorString) {
      let rgb = aColorString.match(/^rgba?\((\d+), (\d+), (\d+)/);
      rgb.shift();
      return rgb.map(x => parseInt(x));
    }

    let toolbarSelector = "#navigator-toolbox > toolbar:not([collapsed=true]):not(#addon-bar)";
#ifdef XP_MACOSX
    toolbarSelector += ":not([type=menubar])";
#endif

    
    

    let luminances = new Map;
    for (let toolbar of document.querySelectorAll(toolbarSelector)) {
      let [r, g, b] = parseRGB(getComputedStyle(toolbar).color);
      let luminance = 0.2125 * r + 0.7154 * g + 0.0721 * b;
      luminances.set(toolbar, luminance);
    }

    for (let [toolbar, luminance] of luminances) {
      if (luminance <= 110)
        toolbar.removeAttribute("brighttext");
      else
        toolbar.setAttribute("brighttext", "true");
    }
  }
}

let PanicButtonNotifier = {
  init: function() {
    this._initialized = true;
    if (window.PanicButtonNotifierShouldNotify) {
      delete window.PanicButtonNotifierShouldNotify;
      this.notify();
    }
  },
  notify: function() {
    if (!this._initialized) {
      window.PanicButtonNotifierShouldNotify = true;
      return;
    }
    
    try {
      let popup = document.getElementById("panic-button-success-notification");
      popup.hidden = false;
      let widget = CustomizableUI.getWidget("panic-button").forWindow(window);
      let anchor = widget.anchor;
      anchor = document.getAnonymousElementByAttribute(anchor, "class", "toolbarbutton-icon");
      popup.openPopup(anchor, popup.getAttribute("position"));
    } catch (ex) {
      Cu.reportError(ex);
    }
  },
  close: function() {
    let popup = document.getElementById("panic-button-success-notification");
    popup.hidePopup();
  },
};

let AboutPrivateBrowsingListener = {
  init: function () {
    window.messageManager.addMessageListener(
      "AboutPrivateBrowsing:OpenPrivateWindow",
      msg => {
        OpenBrowserWindow({private: true});
    });
  }
};
