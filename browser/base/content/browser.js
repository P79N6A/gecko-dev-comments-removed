# -*- Mode: javascript; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
#   Blake Ross <blake@cs.stanford.edu>
#   David Hyatt <hyatt@mozilla.org>
#   Peter Annema <disttsc@bart.nl>
#   Dean Tessman <dean_tessman@hotmail.com>
#   Kevin Puetz <puetzk@iastate.edu>
#   Ben Goodger <ben@netscape.com>
#   Pierre Chanial <chanial@noos.fr>
#   Jason Eager <jce2@po.cwru.edu>
#   Joe Hewitt <hewitt@netscape.com>
#   Alec Flett <alecf@netscape.com>
#   Asaf Romano <mozilla.mano@sent.com>
#   Jason Barnabe <jason_barnabe@fastmail.fm>
#   Peter Parente <parente@cs.unc.edu>
#   Giorgio Maone <g.maone@informaction.com>
#   Tom Germeau <tom.germeau@epigoon.com>
#   Jesse Ruderman <jruderman@gmail.com>
#   Joe Hughes <joe@retrovirus.com>
#   Pamela Greene <pamg.bugs@gmail.com>
#   Michael Ventnor <m.ventnor@gmail.com>
#   Simon Bünzli <zeniko@gmail.com>
#   Johnathan Nightingale <johnath@mozilla.com>
#   Ehsan Akhgari <ehsan.akhgari@gmail.com>
#   Dão Gottwald <dao@mozilla.com>
#   Thomas K. Dyas <tdyas@zecador.org>
#   Edward Lee <edward.lee@engineering.uiuc.edu>
#   Paul O’Shannessy <paul@oshannessy.com>
#   Nils Maier <maierman@web.de>
#   Rob Arnold <robarnold@cmu.edu>
#   Dietrich Ayala <dietrich@mozilla.com>
#   Gavin Sharp <gavin@gavinsharp.com>
#   Justin Dolske <dolske@mozilla.com>
#   Rob Campbell <rcampbell@mozilla.com>
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

let Ci = Components.interfaces;
let Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

const nsIWebNavigation = Ci.nsIWebNavigation;

var gCharsetMenu = null;
var gLastBrowserCharset = null;
var gPrevCharset = null;
var gProxyFavIcon = null;
var gLastValidURLStr = "";
var gInPrintPreviewMode = false;
var gDownloadMgr = null;
var gContextMenu = null; 

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



var gFindBarInitialized = false;
XPCOMUtils.defineLazyGetter(window, "gFindBar", function() {
  let XULNS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
  let findbar = document.createElementNS(XULNS, "findbar");
  findbar.setAttribute("browserid", "content");
  findbar.id = "FindToolbar";

  let browserBottomBox = document.getElementById("browser-bottombox");
  browserBottomBox.insertBefore(findbar, browserBottomBox.firstChild);

  
  findbar.clientTop;
  window.gFindBarInitialized = true;
  return findbar;
});

__defineGetter__("gPrefService", function() {
  delete this.gPrefService;
  return this.gPrefService = Services.prefs;
});

__defineGetter__("AddonManager", function() {
  Cu.import("resource://gre/modules/AddonManager.jsm");
  return this.AddonManager;
});
__defineSetter__("AddonManager", function (val) {
  delete this.AddonManager;
  return this.AddonManager = val;
});

__defineGetter__("PluralForm", function() {
  Cu.import("resource://gre/modules/PluralForm.jsm");
  return this.PluralForm;
});
__defineSetter__("PluralForm", function (val) {
  delete this.PluralForm;
  return this.PluralForm = val;
});

XPCOMUtils.defineLazyGetter(this, "PopupNotifications", function () {
  let tmp = {};
  Cu.import("resource://gre/modules/PopupNotifications.jsm", tmp);
  return new tmp.PopupNotifications(gBrowser,
                                    document.getElementById("notification-popup"),
                                    document.getElementById("notification-popup-box"));
});

let gInitialPages = [
  "about:blank",
  "about:privatebrowsing",
  "about:sessionrestore"
];

#include browser-fullZoom.js
#include inspector.js
#include browser-places.js
#include browser-tabPreviews.js

XPCOMUtils.defineLazyGetter(this, "Win7Features", function () {
#ifdef XP_WIN
#ifndef WINCE
  const WINTASKBAR_CONTRACTID = "@mozilla.org/windows-taskbar;1";
  if (WINTASKBAR_CONTRACTID in Cc &&
      Cc[WINTASKBAR_CONTRACTID].getService(Ci.nsIWinTaskbar).available) {
    let temp = {};
    Cu.import("resource://gre/modules/WindowsPreviewPerTab.jsm", temp);
    let AeroPeek = temp.AeroPeek;
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
#endif
  return null;
});

#ifdef MOZ_CRASHREPORTER
XPCOMUtils.defineLazyServiceGetter(this, "gCrashReporter",
                                   "@mozilla.org/xre/app-info;1",
                                   "nsICrashReporter");
#endif





function pageShowEventHandlers(event) {
  
  if (event.originalTarget == content.document) {
    charsetLoadListener(event);
    XULBrowserWindow.asyncUpdateUI();
  }
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

#ifdef XP_MACOSX




function SetClickAndHoldHandlers() {
  var timer;

  function timerCallback(aButton) {
    aButton.firstChild.hidden = false;
    aButton.open = true;
    timer = null;
  }

  function mousedownHandler(aEvent) {
    if (aEvent.button != 0 ||
        aEvent.currentTarget.open ||
        aEvent.currentTarget.disabled)
      return;

    
    aEvent.currentTarget.firstChild.hidden = true;

    timer = setTimeout(timerCallback, 500, aEvent.currentTarget);
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

  function stopTimer(aEvent) {
    if (timer) {
      clearTimeout(timer);
      timer = null;
    }
  }

  function _addClickAndHoldListenersOnElement(aElm) {
    aElm.addEventListener("mousedown", mousedownHandler, true);
    aElm.addEventListener("mouseup", stopTimer, false);
    aElm.addEventListener("mouseout", stopTimer, false);
    aElm.addEventListener("click", clickHandler, true);
  }

  
  
  var unifiedButton = document.getElementById("unified-back-forward-button");
  if (unifiedButton && !unifiedButton._clickHandlersAttached) {
    var popup = document.getElementById("back-forward-dropmarker")
                        .firstChild.cloneNode(true);
    var backButton = document.getElementById("back-button");
    backButton.setAttribute("type", "menu");
    backButton.appendChild(popup);
    _addClickAndHoldListenersOnElement(backButton);
    var forwardButton = document.getElementById("forward-button");
    popup = popup.cloneNode(true);
    forwardButton.setAttribute("type", "menu");
    forwardButton.appendChild(popup);
    _addClickAndHoldListenersOnElement(forwardButton);
    unifiedButton._clickHandlersAttached = true;
  }
}
#endif

function BookmarkThisTab(aTab) {
  PlacesCommandHook.bookmarkPage(aTab.linkedBrowser,
                                 PlacesUtils.bookmarksMenuFolderId, true);
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

  var node = aDocShell.QueryInterface(Components.interfaces.nsIDocShellTreeNode);
  for (var i = 0; i < node.childCount; ++i) {
    var docShell = node.getChildAt(i);
    docShell = findChildShell(aDocument, docShell, aSoughtURI);
    if (docShell)
      return docShell;
  }
  return null;
}

const gPopupBlockerObserver = {
  _reportButton: null,

  onUpdatePageReport: function (aEvent)
  {
    if (aEvent.originalTarget != gBrowser.selectedBrowser)
      return;

    if (!this._reportButton)
      this._reportButton = document.getElementById("page-report-button");

    if (!gBrowser.pageReport) {
      
      this._reportButton.hidden = true;

      return;
    }

    this._reportButton.hidden = false;

    
    
    
    if (!gBrowser.pageReport.reported) {
      if (gPrefService.getBoolPref("privacy.popups.showBrowserMessage")) {
        var brandBundle = document.getElementById("bundle_brand");
        var brandShortName = brandBundle.getString("brandShortName");
        var message;
        var popupCount = gBrowser.pageReport.length;
#ifdef XP_WIN
        var popupButtonText = gNavigatorBundle.getString("popupWarningButton");
        var popupButtonAccesskey = gNavigatorBundle.getString("popupWarningButton.accesskey");
#else
        var popupButtonText = gNavigatorBundle.getString("popupWarningButtonUnix");
        var popupButtonAccesskey = gNavigatorBundle.getString("popupWarningButtonUnix.accesskey");
#endif
        if (popupCount > 1)
          message = gNavigatorBundle.getFormattedString("popupWarningMultiple", [brandShortName, popupCount]);
        else
          message = gNavigatorBundle.getFormattedString("popupWarning", [brandShortName]);

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

      
      
      gBrowser.pageReport.reported = true;
    }
  },

  toggleAllowPopupsForSite: function (aEvent)
  {
    var pm = Services.perms;
    var shouldBlock = aEvent.target.getAttribute("block") == "true";
    var perm = shouldBlock ? pm.DENY_ACTION : pm.ALLOW_ACTION;
    pm.add(gBrowser.currentURI, "popup", perm);

    gBrowser.getNotificationBox().removeCurrentNotification();
  },

  fillPopupList: function (aEvent)
  {
    
    
    
    
    
    
    
    
    
    var uri = gBrowser.currentURI;
    var blockedPopupAllowSite = document.getElementById("blockedPopupAllowSite");
    try {
      blockedPopupAllowSite.removeAttribute("hidden");

      var pm = Services.perms;
      if (pm.testPermission(uri, "popup") == pm.ALLOW_ACTION) {
        
        
        let blockString = gNavigatorBundle.getFormattedString("popupBlock", [uri.host]);
        blockedPopupAllowSite.setAttribute("label", blockString);
        blockedPopupAllowSite.setAttribute("block", "true");
      }
      else {
        
        let allowString = gNavigatorBundle.getFormattedString("popupAllow", [uri.host]);
        blockedPopupAllowSite.setAttribute("label", allowString);
        blockedPopupAllowSite.removeAttribute("block");
      }
    }
    catch (e) {
      blockedPopupAllowSite.setAttribute("hidden", "true");
    }

    if (gPrivateBrowsingUI.privateBrowsingEnabled)
      blockedPopupAllowSite.setAttribute("disabled", "true");
    else
      blockedPopupAllowSite.removeAttribute("disabled");

    var item = aEvent.target.lastChild;
    while (item && item.getAttribute("observes") != "blockedPopupsSeparator") {
      var next = item.previousSibling;
      item.parentNode.removeChild(item);
      item = next;
    }

    var foundUsablePopupURI = false;
    var pageReport = gBrowser.pageReport;
    if (pageReport) {
      for (var i = 0; i < pageReport.length; ++i) {
        var popupURIspec = pageReport[i].popupWindowURI.spec;

        
        
        
        
        
        if (popupURIspec == "" || popupURIspec == "about:blank" ||
            popupURIspec == uri.spec)
          continue;

        
        
        
        
        
        foundUsablePopupURI = true;

        var menuitem = document.createElement("menuitem");
        var label = gNavigatorBundle.getFormattedString("popupShowPopupPrefix",
                                                        [popupURIspec]);
        menuitem.setAttribute("label", label);
        menuitem.setAttribute("popupWindowURI", popupURIspec);
        menuitem.setAttribute("popupWindowFeatures", pageReport[i].popupWindowFeatures);
        menuitem.setAttribute("popupWindowName", pageReport[i].popupWindowName);
        menuitem.setAttribute("oncommand", "gPopupBlockerObserver.showBlockedPopup(event);");
        menuitem.requestingWindow = pageReport[i].requestingWindow;
        menuitem.requestingDocument = pageReport[i].requestingDocument;
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
    if (aEvent.target.localName == "popup")
      blockedPopupDontShowMessage.setAttribute("label", gNavigatorBundle.getString("popupWarningDontShowFromMessage"));
    else
      blockedPopupDontShowMessage.setAttribute("label", gNavigatorBundle.getString("popupWarningDontShowFromStatusbar"));
  },

  showBlockedPopup: function (aEvent)
  {
    var target = aEvent.target;
    var popupWindowURI = target.getAttribute("popupWindowURI");
    var features = target.getAttribute("popupWindowFeatures");
    var name = target.getAttribute("popupWindowName");

    var dwi = target.requestingWindow;

    
    
    if (dwi && dwi.document == target.requestingDocument) {
      dwi.open(popupWindowURI, name, features);
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
    var firstTime = gPrefService.getBoolPref("privacy.popups.firstTime");

    
    
    
    if (showMessage && firstTime)
      this._displayPageReportFirstTime();

    gPrefService.setBoolPref("privacy.popups.showBrowserMessage", !showMessage);

    gBrowser.getNotificationBox().removeCurrentNotification();
  },

  _displayPageReportFirstTime: function ()
  {
    window.openDialog("chrome://browser/content/pageReportFirstTime.xul", "_blank",
                      "dependent");
  }
};

const gXPInstallObserver = {
  _findChildShell: function (aDocShell, aSoughtShell)
  {
    if (aDocShell == aSoughtShell)
      return aDocShell;

    var node = aDocShell.QueryInterface(Components.interfaces.nsIDocShellTreeNode);
    for (var i = 0; i < node.childCount; ++i) {
      var docShell = node.getChildAt(i);
      docShell = this._findChildShell(docShell, aSoughtShell);
      if (docShell == aSoughtShell)
        return docShell;
    }
    return null;
  },

  _getBrowser: function (aDocShell)
  {
    for (var i = 0; i < gBrowser.browsers.length; ++i) {
      var browser = gBrowser.getBrowserAtIndex(i);
      if (this._findChildShell(browser.docShell, aDocShell))
        return browser;
    }
    return null;
  },

  observe: function (aSubject, aTopic, aData)
  {
    var brandBundle = document.getElementById("bundle_brand");
    var installInfo = aSubject.QueryInterface(Components.interfaces.amIWebInstallInfo);
    var win = installInfo.originatingWindow;
    var shell = win.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
                   .getInterface(Components.interfaces.nsIWebNavigation)
                   .QueryInterface(Components.interfaces.nsIDocShell);
    var browser = this._getBrowser(shell);
    if (!browser)
      return;
    const anchorID = "addons-notification-icon";
    var messageString, action;
    var brandShortName = brandBundle.getString("brandShortName");
    var host = installInfo.originatingURI ? installInfo.originatingURI.host : browser.currentURI.host;

    var notificationID = aTopic;

    switch (aTopic) {
    case "addon-install-blocked":
      var enabled = true;
      try {
        enabled = gPrefService.getBoolPref("xpinstall.enabled");
      }
      catch (e) {
      }

      if (!enabled) {
        notificationID = "xpinstall-disabled"
        if (PopupNotifications.getNotification(notificationID, browser))
          return;

        if (gPrefService.prefIsLocked("xpinstall.enabled")) {
          messageString = gNavigatorBundle.getString("xpinstallDisabledMessageLocked");
          buttons = [];
        }
        else {
          messageString = gNavigatorBundle.getFormattedString("xpinstallDisabledMessage",
                                                              [brandShortName, host]);

          action = {
            label: gNavigatorBundle.getString("xpinstallDisabledButton"),
            accessKey: gNavigatorBundle.getString("xpinstallDisabledButton.accesskey"),
            callback: function editPrefs() {
              gPrefService.setBoolPref("xpinstall.enabled", true);
            }
          };
        }
      }
      else {
        if (PopupNotifications.getNotification(notificationID, browser))
          return;

        messageString = gNavigatorBundle.getFormattedString("xpinstallPromptWarning",
                                                            [brandShortName, host]);

        action = {
          label: gNavigatorBundle.getString("xpinstallPromptAllowButton"),
          accessKey: gNavigatorBundle.getString("xpinstallPromptAllowButton.accesskey"),
          callback: function() {
            installInfo.install();
          }
        };
      }

      PopupNotifications.show(browser, notificationID, messageString, anchorID,
                              action);
      break;
    case "addon-install-failed":
      
      installInfo.installs.forEach(function(aInstall) {
        var error = "addonError";
        if (aInstall.error != 0)
          error += aInstall.error;
        else if (aInstall.addon.blocklistState == Ci.nsIBlocklistService.STATE_BLOCKED)
          error += "Blocklisted";
        else
          error += "Incompatible";

        messageString = gNavigatorBundle.getString(error);
        messageString = messageString.replace("#1", aInstall.name);
        messageString = messageString.replace("#2", host);
        messageString = messageString.replace("#3", brandShortName);
        messageString = messageString.replace("#4", Services.appinfo.version);

        PopupNotifications.show(browser, notificationID, messageString, anchorID,
                                action);
      });
      break;
    case "addon-install-complete":
      var notification = PopupNotifications.getNotification(notificationID, browser);
      if (notification)
        PopupNotifications.remove(notification);

      var needsRestart = installInfo.installs.some(function(i) {
        return (i.addon.pendingOperations & AddonManager.PENDING_INSTALL) != 0;
      });

      if (needsRestart) {
        messageString = gNavigatorBundle.getString("addonsInstalledNeedsRestart");
        action = {
          label: gNavigatorBundle.getString("addonInstallRestartButton"),
          accessKey: gNavigatorBundle.getString("addonInstallRestartButton.accesskey"),
          callback: function() {
            Application.restart();
          }
        };
      }
      else {
        messageString = gNavigatorBundle.getString("addonsInstalled");
        action = {
          label: gNavigatorBundle.getString("addonInstallManage"),
          accessKey: gNavigatorBundle.getString("addonInstallManage.accesskey"),
          callback: function() {
            
            
            var types = {};
            var bestType = null;
            installInfo.installs.forEach(function(aInstall) {
              if (aInstall.type in types)
                types[aInstall.type]++;
              else
                types[aInstall.type] = 1;
              if (!bestType || types[aInstall.type] > types[bestType])
                bestType = aInstall.type;
            });

            BrowserOpenAddonsMgr("addons://list/" + bestType);
          }
        };
      }

      messageString = PluralForm.get(installInfo.installs.length, messageString);
      messageString = messageString.replace("#1", installInfo.installs[0].name);
      messageString = messageString.replace("#2", installInfo.installs.length);
      messageString = messageString.replace("#3", brandShortName);

      PopupNotifications.show(browser, notificationID, messageString, anchorID,
                              action);
      break;
    }
  }
};












let gGestureSupport = {
  





  init: function GS_init(aAddListener) {
    const gestureEvents = ["SwipeGesture",
      "MagnifyGestureStart", "MagnifyGestureUpdate", "MagnifyGesture",
      "RotateGestureStart", "RotateGestureUpdate", "RotateGesture",
      "TapGesture", "PressTapGesture"];

    let addRemove = aAddListener ? window.addEventListener :
      window.removeEventListener;

    gestureEvents.forEach(function (event) addRemove("Moz" + event, this, true),
                          this);
  },

  







  handleEvent: function GS_handleEvent(aEvent) {
    aEvent.stopPropagation();

    
    let def = function(aThreshold, aLatched)
      ({ threshold: aThreshold, latched: !!aLatched });

    switch (aEvent.type) {
      case "MozSwipeGesture":
        aEvent.preventDefault();
        return this.onSwipe(aEvent);
      case "MozMagnifyGestureStart":
        aEvent.preventDefault();
#ifdef XP_WIN
        return this._setupGesture(aEvent, "pinch", def(25, 0), "out", "in");
#else
        return this._setupGesture(aEvent, "pinch", def(150, 1), "out", "in");
#endif
      case "MozRotateGestureStart":
        aEvent.preventDefault();
        return this._setupGesture(aEvent, "twist", def(25, 0), "right", "left");
      case "MozMagnifyGestureUpdate":
      case "MozRotateGestureUpdate":
        aEvent.preventDefault();
        return this._doUpdate(aEvent);
      case "MozTapGesture":
        aEvent.preventDefault();
        return this._doAction(aEvent, ["tap"]);
      case "MozPressTapGesture":
      
      return;
    }
  },

  














  _setupGesture: function GS__setupGesture(aEvent, aGesture, aPref, aInc, aDec) {
    
    for (let [pref, def] in Iterator(aPref))
      aPref[pref] = this._getPref(aGesture + "." + pref, def);

    
    let offset = 0;
    let latchDir = aEvent.delta > 0 ? 1 : -1;
    let isLatched = false;

    
    this._doUpdate = function GS__doUpdate(aEvent) {
      
      offset += aEvent.delta;

      
      if (Math.abs(offset) > aPref["threshold"]) {
        
        
        
        let sameDir = (latchDir ^ offset) >= 0;
        if (!aPref["latched"] || (isLatched ^ sameDir)) {
          this._doAction(aEvent, [aGesture, offset > 0 ? aInc : aDec]);

          
          isLatched = !isLatched;
        }

        
        offset = 0;
      }
    };

    
    this._doUpdate(aEvent);
  },

  







  _power: function GS__power(aArray) {
    
    let num = 1 << aArray.length;
    while (--num >= 0) {
      
      yield aArray.reduce(function (aPrev, aCurr, aIndex) {
        if (num & 1 << aIndex)
          aPrev.push(aCurr);
        return aPrev;
      }, []);
    }
  },

  










  _doAction: function GS__doAction(aEvent, aGesture) {
    
    
    
    let keyCombos = [];
    ["shift", "alt", "ctrl", "meta"].forEach(function (key) {
      if (aEvent[key + "Key"])
        keyCombos.push(key);
    });

    
    for each (let subCombo in this._power(keyCombos)) {
      
      
      
      let command;
      try {
        command = this._getPref(aGesture.concat(subCombo).join("."));
      } catch (e) {}

      if (!command)
        continue;

      let node = document.getElementById(command);
      if (node) {
        if (node.getAttribute("disabled") != "true") {
          let cmdEvent = document.createEvent("xulcommandevent");
          cmdEvent.initCommandEvent("command", true, true, window, 0,
                                    aEvent.ctrlKey, aEvent.altKey, aEvent.shiftKey,
                                    aEvent.metaKey, null);
          node.dispatchEvent(cmdEvent);
        }
      } else {
        goDoCommand(command);
      }

      return command;
    }
    return null;
  },

  







  _doUpdate: function(aEvent) {},

  





  onSwipe: function GS_onSwipe(aEvent) {
    
    ["UP", "RIGHT", "DOWN", "LEFT"].forEach(function (dir) {
      if (aEvent.direction == aEvent["DIRECTION_" + dir])
        return this._doAction(aEvent, ["swipe", dir.toLowerCase()]);
    }, this);
  },

  







  _getPref: function GS__getPref(aPref, aDef) {
    
    const branch = "browser.gesture.";

    try {
      
      let type = typeof aDef;
      let getFunc = "get" + (type == "boolean" ? "Bool" :
                             type == "number" ? "Int" : "Char") + "Pref";
      return gPrefService[getFunc](branch + aPref);
    }
    catch (e) {
      return aDef;
    }
  },
};

function BrowserStartup() {
  var uriToLoad = null;

  
  
  
  
  
  
  
  
  
  if ("arguments" in window && window.arguments[0])
    uriToLoad = window.arguments[0];

  var isLoadingBlank = uriToLoad == "about:blank";
  var mustLoadSidebar = false;

  prepareForStartup();

  if (uriToLoad && !isLoadingBlank) {
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
      
      

      
      gBrowser.stop();
      
      gBrowser.docShell;

      gBrowser.swapBrowsersAndCloseOther(gBrowser.selectedTab, uriToLoad);
    }
    else if (window.arguments.length >= 3) {
      loadURI(uriToLoad, window.arguments[2], window.arguments[3] || null,
              window.arguments[4] || false);
      content.focus();
    }
    
    
    else
      loadOneOrMoreURIs(uriToLoad);
  }

  if (window.opener && !window.opener.closed) {
    let openerSidebarBox = window.opener.document.getElementById("sidebar-box");
    
    
    
    if (openerSidebarBox && !openerSidebarBox.hidden) {
      let sidebarCmd = openerSidebarBox.getAttribute("sidebarcommand");
      let sidebarCmdElem = document.getElementById(sidebarCmd);

      
      if (sidebarCmdElem) {
        let sidebarBox = document.getElementById("sidebar-box");
        let sidebarTitle = document.getElementById("sidebar-title");

        sidebarTitle.setAttribute(
          "value", window.opener.document.getElementById("sidebar-title").getAttribute("value"));
        sidebarBox.setAttribute("width", openerSidebarBox.boxObject.width);

        sidebarBox.setAttribute("sidebarcommand", sidebarCmd);
        
        
        
        sidebarBox.setAttribute(
          "src", window.opener.document.getElementById("sidebar").getAttribute("src"));
        mustLoadSidebar = true;

        sidebarBox.hidden = false;
        document.getElementById("sidebar-splitter").hidden = false;
        sidebarCmdElem.setAttribute("checked", "true");
      }
    }
  }
  else {
    let box = document.getElementById("sidebar-box");
    if (box.hasAttribute("sidebarcommand")) {
      let commandID = box.getAttribute("sidebarcommand");
      if (commandID) {
        let command = document.getElementById(commandID);
        if (command) {
          mustLoadSidebar = true;
          box.hidden = false;
          document.getElementById("sidebar-splitter").hidden = false;
          command.setAttribute("checked", "true");
        }
        else {
          
          
          
          box.removeAttribute("sidebarcommand");
        }
      }
    }
  }

  
  
  Services.obs.notifyObservers(null, "browser-window-before-show", "");

  
  if (!document.documentElement.hasAttribute("width")) {
    let defaultWidth = 994;
    let defaultHeight;
    if (screen.availHeight <= 600) {
      document.documentElement.setAttribute("sizemode", "maximized");
      defaultWidth = 610;
      defaultHeight = 450;
    }
    else {
      
      
      if (screen.availWidth >= 1600)
        defaultWidth = (screen.availWidth / 2) - 20;
      defaultHeight = screen.availHeight - 10;
#ifdef MOZ_WIDGET_GTK2
      
      
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
    goSetCommandEnabled("Browser:OpenLocation", false);
    goSetCommandEnabled("cmd_newNavigatorTab", false);
  }

#ifdef MENUBAR_CAN_AUTOHIDE
  updateAppButtonDisplay();
#endif

  CombinedStopReload.init();

  allTabs.readPref();

  TabsOnTop.syncCommand();

  BookmarksMenuButton.init();

  setTimeout(delayedStartup, 0, isLoadingBlank, mustLoadSidebar);
}

function HandleAppCommandEvent(evt) {
  evt.stopPropagation();
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
    BrowserStop();
    break;
  case "Search":
    BrowserSearch.webSearch();
    break;
  case "Bookmarks":
    toggleSidebar('viewBookmarksSidebar');
    break;
  case "Home":
    BrowserHome();
    break;
  default:
    break;
  }
}

function prepareForStartup() {
  gBrowser.addEventListener("DOMUpdatePageReport", gPopupBlockerObserver.onUpdatePageReport, false);

  gBrowser.addEventListener("PluginNotFound",     gPluginHandler, true);
  gBrowser.addEventListener("PluginCrashed",      gPluginHandler, true);
  gBrowser.addEventListener("PluginBlocklisted",  gPluginHandler, true);
  gBrowser.addEventListener("PluginOutdated",     gPluginHandler, true);
  gBrowser.addEventListener("PluginDisabled",     gPluginHandler, true);
  gBrowser.addEventListener("NewPluginInstalled", gPluginHandler.newPluginInstalled, true);

  Services.obs.addObserver(gPluginHandler.pluginCrashed, "plugin-crashed", false);

  window.addEventListener("AppCommand", HandleAppCommandEvent, true);

  var webNavigation;
  try {
    webNavigation = getWebNavigation();
    if (!webNavigation)
      throw "no XBL binding for browser";
  } catch (e) {
    alert("Error launching browser window:" + e);
    window.close(); 
    return;
  }

  
  
  XULBrowserWindow.init();
  window.QueryInterface(Ci.nsIInterfaceRequestor)
        .getInterface(nsIWebNavigation)
        .QueryInterface(Ci.nsIDocShellTreeItem).treeOwner
        .QueryInterface(Ci.nsIInterfaceRequestor)
        .getInterface(Ci.nsIXULWindow)
        .XULBrowserWindow = window.XULBrowserWindow;
  window.QueryInterface(Ci.nsIDOMChromeWindow).browserDOMWindow =
    new nsBrowserAccess();

  
  if ("arguments" in window && window.arguments.length > 1 && window.arguments[1]) {
    if (window.arguments[1].indexOf("charset=") != -1) {
      var arrayArgComponents = window.arguments[1].split("=");
      if (arrayArgComponents) {
        
        getMarkupDocumentViewer().defaultCharacterSet = arrayArgComponents[1];
      }
    }
  }

  
  
  
  
  
  webNavigation.sessionHistory = Components.classes["@mozilla.org/browser/shistory;1"]
                                           .createInstance(Components.interfaces.nsISHistory);
  Services.obs.addObserver(gBrowser.browsers[0], "browser:purge-session-history", false);

  
  
  gBrowser.browsers[0].removeAttribute("disablehistory");

  
  try {
    gBrowser.docShell.QueryInterface(Components.interfaces.nsIDocShellHistory).useGlobalHistory = true;
  } catch(ex) {
    Components.utils.reportError("Places database may be locked: " + ex);
  }

  
  gBrowser.addProgressListener(window.XULBrowserWindow, Components.interfaces.nsIWebProgress.NOTIFY_ALL);
  gBrowser.addTabsProgressListener(window.TabsProgressListener);

  
  gBrowser.addEventListener("DOMLinkAdded", DOMLinkHandler, false);

  
  gBrowser.addEventListener("MozApplicationManifest",
                            OfflineApps, false);

  
  gGestureSupport.init(true);
}

function delayedStartup(isLoadingBlank, mustLoadSidebar) {
  Services.obs.addObserver(gSessionHistoryObserver, "browser:purge-session-history", false);
  Services.obs.addObserver(gXPInstallObserver, "addon-install-blocked", false);
  Services.obs.addObserver(gXPInstallObserver, "addon-install-failed", false);
  Services.obs.addObserver(gXPInstallObserver, "addon-install-complete", false);

  BrowserOffline.init();
  OfflineApps.init();

  gBrowser.addEventListener("pageshow", function(evt) { setTimeout(pageShowEventHandlers, 0, evt); }, true);

  
  Cc["@mozilla.org/login-manager;1"].getService(Ci.nsILoginManager);

  if (mustLoadSidebar) {
    let sidebar = document.getElementById("sidebar");
    let sidebarBox = document.getElementById("sidebar-box");
    sidebar.setAttribute("src", sidebarBox.getAttribute("src"));
  }

  UpdateUrlbarSearchSplitterState();

  PlacesStarButton.init();

  
  
  window.addEventListener("fullscreen", onFullScreen, true);

  if (isLoadingBlank && gURLBar && isElementVisible(gURLBar))
    gURLBar.focus();
  else
    gBrowser.selectedBrowser.focus();

  gNavToolbox.customizeDone = BrowserToolboxCustomizeDone;
  gNavToolbox.customizeChange = BrowserToolboxCustomizeChange;

  
  initializeSanitizer();

  
  gBrowser.tabContainer.updateVisibility();

  gPrefService.addObserver(gHomeButton.prefDomain, gHomeButton, false);

  var homeButton = document.getElementById("home-button");
  gHomeButton.updateTooltip(homeButton);
  gHomeButton.updatePersonalToolbarStyle(homeButton);

#ifdef HAVE_SHELL_SERVICE
  
  var shell = getShellService();
  if (shell) {
    var shouldCheck = shell.shouldCheckDefaultBrowser;
    var willRecoverSession = false;
    try {
      var ss = Cc["@mozilla.org/browser/sessionstartup;1"].
               getService(Ci.nsISessionStartup);
      willRecoverSession =
        (ss.sessionType == Ci.nsISessionStartup.RECOVER_SESSION);
    }
    catch (ex) {  }
    if (shouldCheck && !shell.isDefaultBrowser(true) && !willRecoverSession) {
      var brandBundle = document.getElementById("bundle_brand");
      var shellBundle = document.getElementById("bundle_shell");

      var brandShortName = brandBundle.getString("brandShortName");
      var promptTitle = shellBundle.getString("setDefaultBrowserTitle");
      var promptMessage = shellBundle.getFormattedString("setDefaultBrowserMessage",
                                                         [brandShortName]);
      var checkboxLabel = shellBundle.getFormattedString("setDefaultBrowserDontAsk",
                                                         [brandShortName]);
      var checkEveryTime = { value: shouldCheck };
      var ps = Services.prompt;
      var rv = ps.confirmEx(window, promptTitle, promptMessage,
                            ps.STD_YES_NO_BUTTONS,
                            null, null, null, checkboxLabel, checkEveryTime);
      if (rv == 0)
        shell.setDefaultBrowser(true, false);
      shell.shouldCheckDefaultBrowser = checkEveryTime.value;
    }
  }
#endif

  
  gBidiUI = isBidiEnabled();
  if (gBidiUI) {
    document.getElementById("documentDirection-separator").hidden = false;
    document.getElementById("documentDirection-swap").hidden = false;
    document.getElementById("textfieldDirection-separator").hidden = false;
    document.getElementById("textfieldDirection-swap").hidden = false;
  }

#ifdef XP_MACOSX
  
  
  if (!getBoolPref("ui.click_hold_context_menus", false))
    SetClickAndHoldHandlers();
#endif

  
  
  
  try {
    FullZoom.init();
  }
  catch(ex) {
    Components.utils.reportError("Failed to init content pref service:\n" + ex);
  }

  let NP = {};
  Cu.import("resource:///modules/NetworkPrioritizer.jsm", NP);
  NP.trackBrowserWindow(window);

  
  try {
    Cc["@mozilla.org/browser/sessionstore;1"]
      .getService(Ci.nsISessionStore)
      .init(window);
  } catch (ex) {
    dump("nsSessionStore could not be initialized: " + ex + "\n");
  }

  PlacesToolbarHelper.updateState();

  
  gBookmarkAllTabsHandler.init();

  
  
  
  
  gBrowser.addEventListener("command", BrowserOnCommand, false);

  ctrlTab.readPref();
  gPrefService.addObserver(ctrlTab.prefName, ctrlTab, false);
  gPrefService.addObserver(allTabs.prefName, allTabs, false);

  
  
  
  setTimeout(function() {
    try {
      Cc["@mozilla.org/microsummary/service;1"].getService(Ci.nsIMicrosummaryService);
    } catch (ex) {
      Components.utils.reportError("Failed to init microsummary service:\n" + ex);
    }
  }, 4000);

  
  
  
  
  
  setTimeout(function() PlacesUtils.livemarks.start(), 5000);

  
  
  
  
  
  setTimeout(function() {
    gDownloadMgr = Cc["@mozilla.org/download-manager;1"].
                   getService(Ci.nsIDownloadManager);

    
    DownloadMonitorPanel.init();

    if (Win7Features) {
      let tempScope = {};
      Cu.import("resource://gre/modules/DownloadTaskbarProgress.jsm",
                tempScope);
      tempScope.DownloadTaskbarProgress.onBrowserWindowLoad(window);
    }
  }, 10000);

  
  
  
  setTimeout(function() PlacesUtils.startPlacesDBUtils(), 15000);

#ifndef XP_MACOSX
  updateEditUIVisibility();
  let placesContext = document.getElementById("placesContext");
  placesContext.addEventListener("popupshowing", updateEditUIVisibility, false);
  placesContext.addEventListener("popuphiding", updateEditUIVisibility, false);
#endif

  
  gPrivateBrowsingUI.init();

  gBrowser.mPanelContainer.addEventListener("InstallBrowserTheme", LightWeightThemeWebInstaller, false, true);
  gBrowser.mPanelContainer.addEventListener("PreviewBrowserTheme", LightWeightThemeWebInstaller, false, true);
  gBrowser.mPanelContainer.addEventListener("ResetBrowserThemePreview", LightWeightThemeWebInstaller, false, true);

  if (Win7Features)
    Win7Features.onOpenWindow();

  Services.obs.notifyObservers(window, "browser-delayed-startup-finished", "");
}

function BrowserShutdown()
{
  if (Win7Features)
    Win7Features.onCloseWindow();

  gPrefService.removeObserver(ctrlTab.prefName, ctrlTab);
  gPrefService.removeObserver(allTabs.prefName, allTabs);
  ctrlTab.uninit();
  allTabs.uninit();

  CombinedStopReload.uninit();

  gGestureSupport.init(false);

  FullScreen.cleanup();

  try {
    FullZoom.destroy();
  }
  catch(ex) {
    Components.utils.reportError(ex);
  }

  Services.obs.removeObserver(gSessionHistoryObserver, "browser:purge-session-history");
  Services.obs.removeObserver(gXPInstallObserver, "addon-install-blocked");
  Services.obs.removeObserver(gXPInstallObserver, "addon-install-failed");
  Services.obs.removeObserver(gXPInstallObserver, "addon-install-complete");
  Services.obs.removeObserver(gPluginHandler.pluginCrashed, "plugin-crashed");

  try {
    gBrowser.removeProgressListener(window.XULBrowserWindow);
    gBrowser.removeTabsProgressListener(window.TabsProgressListener);
  } catch (ex) {
  }

  PlacesStarButton.uninit();

  try {
    gPrefService.removeObserver(gHomeButton.prefDomain, gHomeButton);
  } catch (ex) {
    Components.utils.reportError(ex);
  }

  BrowserOffline.uninit();
  OfflineApps.uninit();
  DownloadMonitorPanel.uninit();
  gPrivateBrowsingUI.uninit();

  var enumerator = Services.wm.getEnumerator(null);
  enumerator.getNext();
  if (!enumerator.hasMoreElements()) {
    document.persist("sidebar-box", "sidebarcommand");
    document.persist("sidebar-box", "width");
    document.persist("sidebar-box", "src");
    document.persist("sidebar-title", "value");
  }

  window.XULBrowserWindow.destroy();
  window.XULBrowserWindow = null;
  window.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
        .getInterface(Components.interfaces.nsIWebNavigation)
        .QueryInterface(Components.interfaces.nsIDocShellTreeItem).treeOwner
        .QueryInterface(Components.interfaces.nsIInterfaceRequestor)
        .getInterface(Components.interfaces.nsIXULWindow)
        .XULBrowserWindow = null;
  window.QueryInterface(Ci.nsIDOMChromeWindow).browserDOMWindow = null;
}

#ifdef XP_MACOSX



function nonBrowserWindowStartup()
{
  
  var disabledItems = ['Browser:SavePage',
                       'Browser:SendLink', 'cmd_pageSetup', 'cmd_print', 'cmd_find', 'cmd_findAgain',
                       'viewToolbarsMenu', 'cmd_toggleTaskbar', 'viewSidebarMenuMenu', 'Browser:Reload',
                       'viewFullZoomMenu', 'pageStyleMenu', 'charsetMenu', 'View:PageSource', 'View:FullScreen',
                       'viewHistorySidebar', 'Browser:AddBookmarkAs', 'View:PageInfo', 'Tasks:InspectPage'];
  var element;

  for (var id in disabledItems)
  {
    element = document.getElementById(disabledItems[id]);
    if (element)
      element.setAttribute("disabled", "true");
  }

  
  
  if (window.location.href == "chrome://browser/content/hiddenWindow.xul")
  {
    var hiddenWindowDisabledItems = ['cmd_close', 'minimizeWindow', 'zoomWindow'];
    for (var id in hiddenWindowDisabledItems)
    {
      element = document.getElementById(hiddenWindowDisabledItems[id]);
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


  setTimeout(nonBrowserWindowDelayedStartup, 0);
}

function nonBrowserWindowDelayedStartup()
{
  
  BrowserOffline.init();

  
  initializeSanitizer();

  
  gPrivateBrowsingUI.init();
}

function nonBrowserWindowShutdown()
{
  BrowserOffline.uninit();

  gPrivateBrowsingUI.uninit();
}
#endif

function initializeSanitizer()
{
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
      itemArray.forEach(function (name) {
        try {
          
          
          if (name != "passwords" && name != "offlineApps")
            cpdBranch.setBoolPref(name, itemBranch.getBoolPref(name));
          clearOnShutdownBranch.setBoolPref(name, itemBranch.getBoolPref(name));
        }
        catch(e) {
          Cu.reportError("Exception thrown during privacy pref migration: " + e);
        }
      });
    }

    gPrefService.setBoolPref("privacy.sanitize.migrateFx3Prefs", true);
  }
}

function gotoHistoryIndex(aEvent)
{
  var index = aEvent.target.getAttribute("index");
  if (!index)
    return false;

  var where = whereToOpenLink(aEvent);

  if (where == "current") {
    

    try {
      gBrowser.gotoIndex(index);
    }
    catch(ex) {
      return false;
    }
    return true;
  }
  else {
    
    

    var sessionHistory = getWebNavigation().sessionHistory;
    var entry = sessionHistory.getEntryAtIndex(index, false);
    var url = entry.URI.spec;
    openUILinkIn(url, where, {relatedToCurrent: true});
    return true;
  }
}

function BrowserForward(aEvent) {
  var where = whereToOpenLink(aEvent, false, true);

  if (where == "current") {
    try {
      gBrowser.goForward();
    }
    catch(ex) {
    }
  }
  else {
    var sessionHistory = getWebNavigation().sessionHistory;
    var currentIndex = sessionHistory.index;
    var entry = sessionHistory.getEntryAtIndex(currentIndex + 1, false);
    var url = entry.URI.spec;
    openUILinkIn(url, where, {relatedToCurrent: true});
  }
}

function BrowserBack(aEvent) {
  var where = whereToOpenLink(aEvent, false, true);

  if (where == "current") {
    try {
      gBrowser.goBack();
    }
    catch(ex) {
    }
  }
  else {
    var sessionHistory = getWebNavigation().sessionHistory;
    var currentIndex = sessionHistory.index;
    var entry = sessionHistory.getEntryAtIndex(currentIndex - 1, false);
    var url = entry.URI.spec;
    openUILinkIn(url, where, {relatedToCurrent: true});
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

function BrowserStop()
{
  try {
    const stopFlags = nsIWebNavigation.STOP_ALL;
    getWebNavigation().stop(stopFlags);
  }
  catch(ex) {
  }
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

  var where = whereToOpenLink(aEvent, false, true);
  if (where == "current")
    BrowserReload();
  else
    openUILinkIn(getWebNavigation().currentURI.spec, where,
                 {relatedToCurrent: true});
}

function BrowserReload() {
  const reloadFlags = nsIWebNavigation.LOAD_FLAGS_NONE;
  BrowserReloadWithFlags(reloadFlags);
}

function BrowserReloadSkipCache() {
  
  const reloadFlags = nsIWebNavigation.LOAD_FLAGS_BYPASS_PROXY | nsIWebNavigation.LOAD_FLAGS_BYPASS_CACHE;
  BrowserReloadWithFlags(reloadFlags);
}

function BrowserHome()
{
  var homePage = gHomeButton.getHomePage();
  loadOneOrMoreURIs(homePage);
}

function BrowserGoHome(aEvent) {
  if (aEvent && "button" in aEvent &&
      aEvent.button == 2) 
    return;

  var homePage = gHomeButton.getHomePage();
  var where = whereToOpenLink(aEvent, false, true);
  var urls;

  
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
  if (gURLBar && !gURLBar.readOnly) {
    if (window.fullScreen)
      FullScreen.mouseoverToggle(true);
    if (isElementVisible(gURLBar)) {
      gURLBar.focus();
      gURLBar.select();
      return true;
    }
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
      
      win = window.openDialog("chrome://browser/content/", "_blank",
                              "chrome,all,dialog=no", "about:blank");
      win.addEventListener("load", openLocationCallback, false);
    }
    return;
  }
#endif
  openDialog("chrome://browser/content/openLocation.xul", "_blank",
             "chrome,modal,titlebar", window);
}

function openLocationCallback()
{
  
  setTimeout(function() { this.openLocation(); }, 0);
}

function BrowserOpenTab()
{
  if (!gBrowser) {
    
    window.openDialog("chrome://browser/content/", "_blank",
                      "chrome,all,dialog=no", "about:blank");
    return;
  }
  gBrowser.loadOneTab("about:blank", {inBackground: false});
  focusAndSelectUrlBar();
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
    if (!val || !val.exists() || !val.isDirectory())
      return;
    this._lastDir = val.clone();

    
    if (!gPrivateBrowsingUI.privateBrowsingEnabled)
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
    const nsIFilePicker = Components.interfaces.nsIFilePicker;
    var fp = Components.classes["@mozilla.org/filepicker;1"].createInstance(nsIFilePicker);
    fp.init(window, gNavigatorBundle.getString("openFile"), nsIFilePicker.modeOpen);
    fp.appendFilters(nsIFilePicker.filterAll | nsIFilePicker.filterText | nsIFilePicker.filterImages |
                     nsIFilePicker.filterXML | nsIFilePicker.filterHTML);
    fp.displayDirectory = gLastOpenDirectory.path;

    if (fp.show() == nsIFilePicker.returnOK) {
      if (fp.file && fp.file.exists())
        gLastOpenDirectory.path = fp.file.parent.QueryInterface(Ci.nsILocalFile);
      openTopWin(fp.fileURL.spec);
    }
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

  
  gBrowser.removeCurrentTab();
}

function BrowserTryToCloseWindow()
{
  if (WindowIsClosing())
    window.close();     
}

function loadURI(uri, referrer, postData, allowThirdPartyFixup)
{
  try {
    if (postData === undefined)
      postData = null;
    var flags = nsIWebNavigation.LOAD_FLAGS_NONE;
    if (allowThirdPartyFixup) {
      flags = nsIWebNavigation.LOAD_FLAGS_ALLOW_THIRD_PARTY_FIXUP;
    }
    gBrowser.loadURIWithFlags(uri, flags, referrer, null, postData);
  } catch (e) {
  }
}

function getShortcutOrURI(aURL, aPostDataRef) {
  var shortcutURL = null;
  var keyword = aURL;
  var param = "";

  var offset = aURL.indexOf(" ");
  if (offset > 0) {
    keyword = aURL.substr(0, offset);
    param = aURL.substr(offset + 1);
  }

  if (!aPostDataRef)
    aPostDataRef = {};

  var engine = Services.search.getEngineByAlias(keyword);
  if (engine) {
    var submission = engine.getSubmission(param);
    aPostDataRef.value = submission.postData;
    return submission.uri.spec;
  }

  [shortcutURL, aPostDataRef.value] =
    PlacesUtils.getURLAndPostDataForKeyword(keyword);

  if (!shortcutURL)
    return aURL;

  var postData = "";
  if (aPostDataRef.value)
    postData = unescape(aPostDataRef.value);

  if (/%s/i.test(shortcutURL) || /%s/i.test(postData)) {
    var charset = "";
    const re = /^(.*)\&mozcharset=([a-zA-Z][_\-a-zA-Z0-9]+)\s*$/;
    var matches = shortcutURL.match(re);
    if (matches)
      [, shortcutURL, charset] = matches;
    else {
      
      try {
        
        
        charset = PlacesUtils.history.getCharsetForURI(makeURI(shortcutURL));
      } catch (e) {}
    }

    var encodedParam = "";
    if (charset)
      encodedParam = escape(convertFromUnicode(charset, param));
    else 
      encodedParam = encodeURIComponent(param);

    shortcutURL = shortcutURL.replace(/%s/g, encodedParam).replace(/%S/g, param);

    if (/%s/i.test(postData)) 
      aPostDataRef.value = getPostDataStream(postData, param, encodedParam,
                                             "application/x-www-form-urlencoded");
  }
  else if (param) {
    
    
    aPostDataRef.value = null;

    return aURL;
  }

  return shortcutURL;
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

function readFromClipboard()
{
  var url;

  try {
    
    var clipboard = Components.classes["@mozilla.org/widget/clipboard;1"]
                              .getService(Components.interfaces.nsIClipboard);

    
    var trans = Components.classes["@mozilla.org/widget/transferable;1"]
                          .createInstance(Components.interfaces.nsITransferable);

    trans.addDataFlavor("text/unicode");

    
    if (clipboard.supportsSelectionClipboard())
      clipboard.getData(trans, clipboard.kSelectionClipboard);
    else
      clipboard.getData(trans, clipboard.kGlobalClipboard);

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
      
      
      webNav = getWebNavigation();
  }
  
  
  
  
  
  try{
    var PageLoader = webNav.QueryInterface(Components.interfaces.nsIWebPageDescriptor);

    pageCookie = PageLoader.currentDescriptor;
  } catch(err) {
    
  }

  top.gViewSourceUtils.viewSource(webNav.currentURI.spec, pageCookie, aDocument);
}




function BrowserPageInfo(doc, initialTab, imageElement) {
  var args = {doc: doc, initialTab: initialTab, imageElement: imageElement};
  var windows = Cc['@mozilla.org/appshell/window-mediator;1']
                  .getService(Ci.nsIWindowMediator)
                  .getEnumerator("Browser:page-info");

  var documentURL = doc ? doc.location : window.content.document.location;

  
  while (windows.hasMoreElements()) {
    var currentWindow = windows.getNext();
    if (currentWindow.document.documentElement.getAttribute("relatedUrl") == documentURL) {
      currentWindow.focus();
      currentWindow.resetPageInfo(args);
      return currentWindow;
    }
  }

  
  return openDialog("chrome://browser/content/pageinfo/pageInfo.xul", "",
                    "chrome,toolbar,dialog=no,resizable", args);
}

#ifdef DEBUG

function LeakDetector(verbose)
{
  this.verbose = verbose;
}

const NS_LEAKDETECTOR_CONTRACTID = "@mozilla.org/xpcom/leakdetector;1";

if (NS_LEAKDETECTOR_CONTRACTID in Components.classes) {
  try {
    LeakDetector.prototype = Components.classes[NS_LEAKDETECTOR_CONTRACTID]
                                       .createInstance(Components.interfaces.nsILeakDetector);
  } catch (err) {
    LeakDetector.prototype = Object.prototype;
  }
} else {
  LeakDetector.prototype = Object.prototype;
}

var leakDetector = new LeakDetector(false);


function dumpMemoryLeaks()
{
  leakDetector.dumpLeaks();
}


function traceChrome()
{
  leakDetector.traceObject(document, leakDetector.verbose);
}


function traceDocument()
{
  
  leakDetector.markObject(document, true);
  leakDetector.traceObject(content, leakDetector.verbose);
  leakDetector.markObject(document, false);
}


function traceVerbose(verbose)
{
  leakDetector.verbose = (verbose == "true");
}
#endif

function URLBarSetURI(aURI) {
  var value = gBrowser.userTypedValue;
  var valid = false;

  if (value == null) {
    let uri = aURI || getWebNavigation().currentURI;

    
    
    if (gInitialPages.indexOf(uri.spec) != -1)
      value = content.opener ? uri.spec : "";
    else
      value = losslessDecodeURI(uri);

    valid = (uri.spec != "about:blank");
  }

  gURLBar.value = value;
  SetPageProxyState(valid ? "valid" : "invalid");
}

function losslessDecodeURI(aURI) {
  var value = aURI.spec;
  
  if (!/%25(?:3B|2F|3F|3A|40|26|3D|2B|24|2C|23)/i.test(value))
    try {
      value = decodeURI(value)
                
                
                
                
                
                
                
                .replace(/%(?!3B|2F|3F|3A|40|26|3D|2B|24|2C|23)|[\r\n\t]/ig,
                         encodeURIComponent);
    } catch (e) {}

  
  
  value = value.replace(/[\v\x0c\x1c\x1d\x1e\x1f\u2028\u2029\ufffc]/g,
                        encodeURIComponent);

  
  
  
  value = value.replace(/[\u00ad\u034f\u115f-\u1160\u17b4-\u17b5\u180b-\u180d\u200b-\u200f\u202a-\u202e\u2060-\u206f\u3164\ufe00-\ufe0f\ufeff\uffa0\ufff0-\ufff8]|\ud834[\udd73-\udd7a]|[\udb40-\udb43][\udc00-\udfff]/g,
                        encodeURIComponent);
  return value;
}

function UpdateUrlbarSearchSplitterState()
{
  var splitter = document.getElementById("urlbar-search-splitter");
  var urlbar = document.getElementById("urlbar-container");
  var searchbar = document.getElementById("search-container");

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
      splitter.className = "chromeclass-toolbar-additional";
    }
    urlbar.parentNode.insertBefore(splitter, ibefore);
  } else if (splitter)
    splitter.parentNode.removeChild(splitter);
}

var LocationBarHelpers = {
  _timeoutID: null,

  _searchBegin: function LocBar_searchBegin() {
    function delayedBegin(self) {
      self._timeoutID = null;
      document.getElementById("urlbar-throbber").setAttribute("busy", "true");
    }

    this._timeoutID = setTimeout(delayedBegin, 500, this);
  },

  _searchComplete: function LocBar_searchComplete() {
    
    if (this._timeoutID) {
      clearTimeout(this._timeoutID);
      this._timeoutID = null;
    }
    document.getElementById("urlbar-throbber").removeAttribute("busy");
  }
};

function UpdatePageProxyState()
{
  if (gURLBar && gURLBar.value != gLastValidURLStr)
    SetPageProxyState("invalid");
}

function SetPageProxyState(aState)
{
  if (!gURLBar)
    return;

  if (!gProxyFavIcon)
    gProxyFavIcon = document.getElementById("page-proxy-favicon");

  gURLBar.setAttribute("pageproxystate", aState);
  gProxyFavIcon.setAttribute("pageproxystate", aState);

  
  
  if (aState == "valid") {
    gLastValidURLStr = gURLBar.value;
    gURLBar.addEventListener("input", UpdatePageProxyState, false);

    PageProxySetIcon(gBrowser.getIcon());
  } else if (aState == "invalid") {
    gURLBar.removeEventListener("input", UpdatePageProxyState, false);
    PageProxyClearIcon();
  }
}

function PageProxySetIcon (aURL)
{
  if (!gProxyFavIcon)
    return;

  if (!aURL)
    PageProxyClearIcon();
  else if (gProxyFavIcon.getAttribute("src") != aURL)
    gProxyFavIcon.setAttribute("src", aURL);
}

function PageProxyClearIcon ()
{
  gProxyFavIcon.removeAttribute("src");
}

function PageProxyClickHandler(aEvent)
{
  if (aEvent.button == 1 && gPrefService.getBoolPref("middlemouse.paste"))
    middleMousePaste(aEvent);
}

function BrowserImport()
{
#ifdef XP_MACOSX
  var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                     .getService(Components.interfaces.nsIWindowMediator);
  var win = wm.getMostRecentWindow("Browser:MigrationWizard");
  if (win)
    win.focus();
  else {
    window.openDialog("chrome://browser/content/migration/migration.xul",
                      "migration", "centerscreen,chrome,resizable=no");
  }
#else
  window.openDialog("chrome://browser/content/migration/migration.xul",
                    "migration", "modal,centerscreen,chrome,resizable=no");
#endif
}




function BrowserOnCommand(event) {
    
    if (!event.isTrusted)
      return;

    var ot = event.originalTarget;
    var errorDoc = ot.ownerDocument;

    
    
    if (/^about:certerror/.test(errorDoc.documentURI)) {
      if (ot == errorDoc.getElementById('exceptionDialogButton')) {
        var params = { exceptionAdded : false, handlePrivateBrowsing : true };

        try {
          switch (gPrefService.getIntPref("browser.ssl_override_behavior")) {
            case 2 : 
              params.prefetchCert = true;
            case 1 : 
              params.location = errorDoc.location.href;
          }
        } catch (e) {
          Components.utils.reportError("Couldn't get ssl_override pref: " + e);
        }

        window.openDialog('chrome://pippki/content/exceptionDialog.xul',
                          '','chrome,centerscreen,modal', params);

        
        if (params.exceptionAdded)
          errorDoc.location.reload();
      }
      else if (ot == errorDoc.getElementById('getMeOutOfHereButton')) {
        getMeOutOfHere();
      }
    }
    else if (/^about:blocked/.test(errorDoc.documentURI)) {
      
      
      
      var isMalware = /e=malwareBlocked/.test(errorDoc.documentURI);

      if (ot == errorDoc.getElementById('getMeOutButton')) {
        getMeOutOfHere();
      }
      else if (ot == errorDoc.getElementById('reportButton')) {
        
        
        

        if (isMalware) {
          
          
          try {
            let reportURL = formatURL("browser.safebrowsing.malware.reportURL", true);
            reportURL += errorDoc.location.href;
            content.location = reportURL;
          } catch (e) {
            Components.utils.reportError("Couldn't get malware report URL: " + e);
          }
        }
        else { 
          try {
            content.location = formatURL("browser.safebrowsing.warning.infoURL", true);
          } catch (e) {
            Components.utils.reportError("Couldn't get phishing info URL: " + e);
          }
        }
      }
      else if (ot == errorDoc.getElementById('ignoreWarningButton')) {
        
        
        
        gBrowser.loadURIWithFlags(content.location.href,
                                  nsIWebNavigation.LOAD_FLAGS_BYPASS_CLASSIFIER,
                                  null, null, null);
        let buttons = [{
          label: gNavigatorBundle.getString("safebrowsing.getMeOutOfHereButton.label"),
          accessKey: gNavigatorBundle.getString("safebrowsing.getMeOutOfHereButton.accessKey"),
          callback: function() { getMeOutOfHere(); }
        }];

        let title;
        if (isMalware) {
          title = gNavigatorBundle.getString("safebrowsing.reportedAttackSite");
          buttons[1] = {
            label: gNavigatorBundle.getString("safebrowsing.notAnAttackButton.label"),
            accessKey: gNavigatorBundle.getString("safebrowsing.notAnAttackButton.accessKey"),
            callback: function() {
              openUILinkIn(safebrowsing.getReportURL('MalwareError'), 'tab');
            }
          };
        } else {
          title = gNavigatorBundle.getString("safebrowsing.reportedWebForgery");
          buttons[1] = {
            label: gNavigatorBundle.getString("safebrowsing.notAForgeryButton.label"),
            accessKey: gNavigatorBundle.getString("safebrowsing.notAForgeryButton.accessKey"),
            callback: function() {
              openUILinkIn(safebrowsing.getReportURL('Error'), 'tab');
            }
          };
        }

        let notificationBox = gBrowser.getNotificationBox();
        let value = "blocked-badware-page";

        let previousNotification = notificationBox.getNotificationWithValue(value);
        if (previousNotification)
          notificationBox.removeNotification(previousNotification);

        notificationBox.appendNotification(
          title,
          value,
          "chrome://global/skin/icons/blacklist_favicon.png",
          notificationBox.PRIORITY_CRITICAL_HIGH,
          buttons
        );
      }
    }
    else if (/^about:privatebrowsing/.test(errorDoc.documentURI)) {
      if (ot == errorDoc.getElementById("startPrivateBrowsing")) {
        gPrivateBrowsingUI.toggleMode();
      }
    }
}








function getMeOutOfHere() {
  
  var prefs = Cc["@mozilla.org/preferences-service;1"]
             .getService(Ci.nsIPrefService).getDefaultBranch(null);
  var url = "about:blank";
  try {
    url = prefs.getComplexValue("browser.startup.homepage",
                                Ci.nsIPrefLocalizedString).data;
    
    if (url.indexOf("|") != -1)
      url = url.split("|")[0];
  } catch(e) {
    Components.utils.reportError("Couldn't get homepage pref: " + e);
  }
  content.location = url;
}

function BrowserFullScreen()
{
  window.fullScreen = !window.fullScreen;
}

function onFullScreen()
{
  FullScreen.toggle();
}

function getWebNavigation()
{
  try {
    return gBrowser.webNavigation;
  } catch (e) {
    return null;
  }
}

function BrowserReloadWithFlags(reloadFlags) {
  





  var webNav = getWebNavigation();
  try {
    var sh = webNav.sessionHistory;
    if (sh)
      webNav = sh.QueryInterface(nsIWebNavigation);
  } catch (e) {
  }

  try {
    webNav.reload(reloadFlags);
  } catch (e) {
  }
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
    gBrowser.removeTab(this._printPreviewTab);
    this._printPreviewTab = null;
    gInPrintPreviewMode = false;
    this._toggleAffectedChrome();
  },
  _toggleAffectedChrome: function () {
    
    
    
    
    
    
    
    
    

    gNavToolbox.hidden = gInPrintPreviewMode;

    if (gInPrintPreviewMode)
      this._hideChrome();
    else
      this._showChrome();

    if (this._chromeState.sidebarOpen)
      toggleSidebar(this._sidebarCommand);
  },
  _hideChrome: function () {
    this._chromeState = {};

    var sidebar = document.getElementById("sidebar-box");
    this._chromeState.sidebarOpen = !sidebar.hidden;
    this._sidebarCommand = sidebar.getAttribute("sidebarcommand");

    var notificationBox = gBrowser.getNotificationBox();
    this._chromeState.notificationsOpen = !notificationBox.notificationsHidden;
    notificationBox.notificationsHidden = true;

    document.getElementById("sidebar").setAttribute("src", "about:blank");
    var statusbar = document.getElementById("status-bar");
    this._chromeState.statusbarOpen = !statusbar.hidden;
    statusbar.hidden = true;

    this._chromeState.findOpen = gFindBarInitialized && !gFindBar.hidden;
    if (gFindBarInitialized)
      gFindBar.close();
  },
  _showChrome: function () {
    if (this._chromeState.notificationsOpen)
      gBrowser.getNotificationBox().notificationsHidden = false;

    if (this._chromeState.statusbarOpen)
      document.getElementById("status-bar").hidden = false;

    if (this._chromeState.findOpen)
      gFindBar.open();
  }
}

function getMarkupDocumentViewer()
{
  return gBrowser.markupDocumentViewer;
}














function FillInHTMLTooltip(tipElement)
{
  var retVal = false;
  
  if (tipElement.namespaceURI == "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul" ||
      !tipElement.ownerDocument)
    return retVal;

  const XLinkNS = "http://www.w3.org/1999/xlink";


  var titleText = null;
  var XLinkTitleText = null;
  var SVGTitleText = null;
#ifdef MOZ_SVG
  var lookingForSVGTitle = true;
#else
  var lookingForSVGTitle = false;
#endif 
  var direction = tipElement.ownerDocument.dir;

  while (!titleText && !XLinkTitleText && !SVGTitleText && tipElement) {
    if (tipElement.nodeType == Node.ELEMENT_NODE) {
      titleText = tipElement.getAttribute("title");
      if ((tipElement instanceof HTMLAnchorElement && tipElement.href) ||
          (tipElement instanceof HTMLAreaElement && tipElement.href) ||
          (tipElement instanceof HTMLLinkElement && tipElement.href)
#ifdef MOZ_SVG
          || (tipElement instanceof SVGAElement && tipElement.hasAttributeNS(XLinkNS, "href"))
#endif 
          ) {
        XLinkTitleText = tipElement.getAttributeNS(XLinkNS, "title");
      }
      if (lookingForSVGTitle &&
          !(tipElement instanceof SVGElement &&
            tipElement.parentNode instanceof SVGElement &&
            !(tipElement.parentNode instanceof SVGForeignObjectElement))) {
        lookingForSVGTitle = false;
      }
      if (lookingForSVGTitle) {
        let length = tipElement.childNodes.length;
        for (let i = 0; i < length; i++) {
          let childNode = tipElement.childNodes[i];
          if (childNode instanceof SVGTitleElement) {
            SVGTitleText = childNode.textContent;
            break;
          }
        }
      }
      var defView = tipElement.ownerDocument.defaultView;
      
      
      if (!defView)
        return retVal;
      direction = defView.getComputedStyle(tipElement, "")
        .getPropertyValue("direction");
    }
    tipElement = tipElement.parentNode;
  }

  var tipNode = document.getElementById("aHTMLTooltip");
  tipNode.style.direction = direction;

  [titleText, XLinkTitleText, SVGTitleText].forEach(function (t) {
    if (t && /\S/.test(t)) {

      
      
      
      
      
      t = t.replace(/[\r\t]/g, ' ');
      t = t.replace(/\n/g, '');

      tipNode.setAttribute("label", t);
      retVal = true;
    }
  });

  return retVal;
}

var browserDragAndDrop = {
  canDropLink: function (aEvent) Services.droppedLinkHandler.canDropLink(aEvent, true),

  dragOver: function (aEvent, statusString)
  {
    if (this.canDropLink(aEvent)) {
      aEvent.preventDefault();

      if (statusString) {
        var statusTextFld = document.getElementById("statusbar-display");
        statusTextFld.label = gNavigatorBundle.getString(statusString);
      }
    }
  },

  drop: function (aEvent, aName) Services.droppedLinkHandler.dropLink(aEvent, aName)
}

var proxyIconDNDObserver = {
  onDragStart: function (aEvent, aXferData, aDragAction)
    {
      if (gProxyFavIcon.getAttribute("pageproxystate") != "valid")
        return;

      var value = content.location.href;
      var urlString = value + "\n" + content.document.title;
      var htmlString = "<a href=\"" + value + "\">" + value + "</a>";

      var dt = aEvent.dataTransfer;
      dt.setData("text/x-moz-url", urlString);
      dt.setData("text/uri-list", value);
      dt.setData("text/plain", value);
      dt.setData("text/html", htmlString);
    }
}

var homeButtonObserver = {
  onDrop: function (aEvent)
    {
      setTimeout(openHomeDialog, 0, browserDragAndDrop.drop(aEvent, { }));
    },

  onDragOver: function (aEvent)
    {
      browserDragAndDrop.dragOver(aEvent, "droponhomebutton");
      aEvent.dropEffect = "link";
    },
  onDragLeave: function (aEvent)
    {
      var statusTextFld = document.getElementById("statusbar-display");
      statusTextFld.label = "";
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
      var homeButton = document.getElementById("home-button");
      homeButton.setAttribute("tooltiptext", aURL);
    } catch (ex) {
      dump("Failed to set the home page.\n"+ex+"\n");
    }
  }
}

var bookmarksButtonObserver = {
  onDrop: function (aEvent)
  {
    let name = { };
    let url = browserDragAndDrop.drop(aEvent, name);
    try {
      PlacesUIUtils.showMinimalAddBookmarkUI(makeURI(url), name);
    } catch(ex) { }
  },

  onDragOver: function (aEvent)
  {
    browserDragAndDrop.dragOver(aEvent, "droponbookmarksbutton");
    aEvent.dropEffect = "link";
  },

  onDragLeave: function (aEvent)
  {
    var statusTextFld = document.getElementById("statusbar-display");
    statusTextFld.label = "";
  }
}

var newTabButtonObserver = {
  onDragOver: function (aEvent)
  {
    browserDragAndDrop.dragOver(aEvent, "droponnewtabbutton");
  },

  onDragLeave: function (aEvent)
  {
    var statusTextFld = document.getElementById("statusbar-display");
    statusTextFld.label = "";
  },

  onDrop: function (aEvent)
  {
    let url = browserDragAndDrop.drop(aEvent, { });
    var postData = {};
    url = getShortcutOrURI(url, postData);
    if (url) {
      
      openNewTabWith(url, null, postData.value, aEvent, true);
    }
  }
}

var newWindowButtonObserver = {
  onDragOver: function (aEvent)
  {
    browserDragAndDrop.dragOver(aEvent, "droponnewwindowbutton");
  },
  onDragLeave: function (aEvent)
  {
    var statusTextFld = document.getElementById("statusbar-display");
    statusTextFld.label = "";
  },
  onDrop: function (aEvent)
  {
    let url = browserDragAndDrop.drop(aEvent, { });
    var postData = {};
    url = getShortcutOrURI(url, postData);
    if (url) {
      
      openNewWindowWith(url, null, postData.value, true);
    }
  }
}

var DownloadsButtonDNDObserver = {
  onDragOver: function (aEvent)
  {
    var statusTextFld = document.getElementById("statusbar-display");
    statusTextFld.label = gNavigatorBundle.getString("dropondownloadsbutton");
    var types = aEvent.dataTransfer.types;
    if (types.contains("text/x-moz-url") ||
        types.contains("text/uri-list") ||
        types.contains("text/plain"))
      aEvent.preventDefault();
  },

  onDragLeave: function (aEvent)
  {
    var statusTextFld = document.getElementById("statusbar-display");
    statusTextFld.label = "";
  },

  onDrop: function (aEvent)
  {
    let name = { };
    let url = browserDragAndDrop.drop(aEvent, name);
    if (url)
      saveURL(url, name, null, true, true);
  }
}

const DOMLinkHandler = {
  handleEvent: function (event) {
    switch (event.type) {
      case "DOMLinkAdded":
        this.onLinkAdded(event);
        break;
    }
  },
  onLinkAdded: function (event) {
    var link = event.originalTarget;
    var rel = link.rel && link.rel.toLowerCase();
    if (!link || !link.ownerDocument || !rel || !link.href)
      return;

    var feedAdded = false;
    var iconAdded = false;
    var searchAdded = false;
    var relStrings = rel.split(/\s+/);
    var rels = {};
    for (let i = 0; i < relStrings.length; i++)
      rels[relStrings[i]] = true;

    for (let relVal in rels) {
      switch (relVal) {
        case "feed":
        case "alternate":
          if (!feedAdded) {
            if (!rels.feed && rels.alternate && rels.stylesheet)
              break;

            if (isValidFeed(link, link.ownerDocument.nodePrincipal, rels.feed)) {
              FeedHandler.addFeed(link, link.ownerDocument);
              feedAdded = true;
            }
          }
          break;
        case "icon":
          if (!iconAdded) {
            if (!gPrefService.getBoolPref("browser.chrome.site_icons"))
              break;

            var targetDoc = link.ownerDocument;
            var uri = makeURI(link.href, targetDoc.characterSet);

            if (gBrowser.isFailedIcon(uri))
              break;

            
            
            
            const aboutNeterr = /^about:neterror\?/;
            const aboutBlocked = /^about:blocked\?/;
            const aboutCert = /^about:certerror\?/;
            if (!(aboutNeterr.test(targetDoc.documentURI) ||
                  aboutBlocked.test(targetDoc.documentURI) ||
                  aboutCert.test(targetDoc.documentURI)) ||
                !uri.schemeIs("chrome")) {
              var ssm = Cc["@mozilla.org/scriptsecuritymanager;1"].
                        getService(Ci.nsIScriptSecurityManager);
              try {
                ssm.checkLoadURIWithPrincipal(targetDoc.nodePrincipal, uri,
                                              Ci.nsIScriptSecurityManager.DISALLOW_SCRIPT);
              } catch(e) {
                break;
              }
            }

            try {
              var contentPolicy = Cc["@mozilla.org/layout/content-policy;1"].
                                  getService(Ci.nsIContentPolicy);
            } catch(e) {
              break; 
            }

            
            if (contentPolicy.shouldLoad(Ci.nsIContentPolicy.TYPE_IMAGE,
                                         uri, targetDoc.documentURIObject,
                                         link, link.type, null)
                                         != Ci.nsIContentPolicy.ACCEPT)
              break;

            var browserIndex = gBrowser.getBrowserIndexForDocument(targetDoc);
            
            if (browserIndex == -1)
              break;

            let tab = gBrowser.tabs[browserIndex];
            gBrowser.setIcon(tab, link.href);
            iconAdded = true;
          }
          break;
        case "search":
          if (!searchAdded) {
            var type = link.type && link.type.toLowerCase();
            type = type.replace(/^\s+|\s*(?:;.*)?$/g, "");

            if (type == "application/opensearchdescription+xml" && link.title &&
                /^(?:https?|ftp):/i.test(link.href)) {
              var engine = { title: link.title, href: link.href };
              BrowserSearch.addEngine(engine, link.ownerDocument);
              searchAdded = true;
            }
          }
          break;
      }
    }
  }
}

const BrowserSearch = {
  addEngine: function(engine, targetDoc) {
    if (!this.searchBar)
      return;

    var browser = gBrowser.getBrowserForDocument(targetDoc);
    
    if (!browser)
      return;

    
    if (browser.engines) {
      if (browser.engines.some(function (e) e.title == engine.title))
        return;
    }

    
    
    
    var iconURL = null;
    if (gBrowser.shouldLoadFavIcon(targetDoc.documentURIObject))
      iconURL = targetDoc.documentURIObject.prePath + "/favicon.ico";

    var hidden = false;
    
    
    
    
    if (Services.search.getEngineByName(engine.title))
      hidden = true;

    var engines = (hidden ? browser.hiddenEngines : browser.engines) || [];

    engines.push({ uri: engine.href,
                   title: engine.title,
                   icon: iconURL });

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
      searchBar.searchButton.setAttribute("addengines", "true");
    else
      searchBar.searchButton.removeAttribute("addengines");
  },

  




  webSearch: function BrowserSearch_webSearch() {
#ifdef XP_MACOSX
    if (window.location.href != getBrowserURL()) {
      var win = getTopWin();
      if (win) {
        
        win.focus()
        win.BrowserSearch.webSearch();
      } else {
        

        
        
        function webSearchCallback() {
          setTimeout(BrowserSearch.webSearch, 0);
        }

        win = window.openDialog("chrome://browser/content/", "_blank",
                                "chrome,all,dialog=no", "about:blank");
        win.addEventListener("load", webSearchCallback, false);
      }
      return;
    }
#endif
    var searchBar = this.searchBar;
    if (searchBar && window.fullScreen)
      FullScreen.mouseoverToggle(true);

    if (isElementVisible(searchBar)) {
      searchBar.select();
      searchBar.focus();
    } else {
      openUILinkIn(Services.search.defaultEngine.searchForm, "current");
    }
  },

  










  loadSearch: function BrowserSearch_search(searchText, useNewTab) {
    var engine;

    
    
    if (isElementVisible(this.searchBar))
      engine = Services.search.currentEngine;
    else
      engine = Services.search.defaultEngine;

    var submission = engine.getSubmission(searchText); 

    
    
    
    
    if (!submission)
      return;

    if (useNewTab) {
      gBrowser.loadOneTab(submission.uri.spec, {
                          postData: submission.postData,
                          relatedToCurrent: true});
    } else
      loadURI(submission.uri.spec, null, submission.postData, false);
  },

  


  get searchBar() {
    return document.getElementById("searchbar");
  },

  loadAddEngines: function BrowserSearch_loadAddEngines() {
    var newWindowPref = gPrefService.getIntPref("browser.link.open_newwindow");
    var where = newWindowPref == 3 ? "tab" : "window";
    var regionBundle = document.getElementById("bundle_browser_region");
    var searchEnginesURL = formatURL("browser.search.searchEnginesURL", true);
    openUILinkIn(searchEnginesURL, where);
  }
}

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

  var webNav = getWebNavigation();
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

    item.setAttribute("uri", uri);
    item.setAttribute("label", entry.title || uri);
    item.setAttribute("index", j);

    if (j != index) {
      try {
        let iconURL = Cc["@mozilla.org/browser/favicon-service;1"]
                         .getService(Ci.nsIFaviconService)
                         .getFaviconForPage(entry.URI).spec;
        item.style.listStyleImage = "url(" + iconURL + ")";
      } catch (ex) {}
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
  if (aUrlToAdd &&
      aUrlToAdd.indexOf(" ") == -1 &&
      !/[\x00-\x1F]/.test(aUrlToAdd))
    PlacesUIUtils.markPageAsTyped(aUrlToAdd);
}

function toJavaScriptConsole()
{
  toOpenWindowByType("global:console", "chrome://global/content/console.xul");
}

function BrowserDownloadsUI()
{
  Cc["@mozilla.org/download-manager-ui;1"].
  getService(Ci.nsIDownloadManagerUI).show(window);
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

function OpenBrowserWindow()
{
  var charsetArg = new String();
  var handler = Components.classes["@mozilla.org/browser/clh;1"]
                          .getService(Components.interfaces.nsIBrowserHandler);
  var defaultArgs = handler.defaultArgs;
  var wintype = document.documentElement.getAttribute('windowtype');

  
  
  
  var win;
  if (window && (wintype == "navigator:browser") && window.content && window.content.document)
  {
    var DocCharset = window.content.document.characterSet;
    charsetArg = "charset="+DocCharset;

    
    win = window.openDialog("chrome://browser/content/", "_blank", "chrome,all,dialog=no", defaultArgs, charsetArg);
  }
  else 
  {
    win = window.openDialog("chrome://browser/content/", "_blank", "chrome,all,dialog=no", defaultArgs);
  }

  return win;
}

var gCustomizeSheet = false;


function BrowserCustomizeToolbar()
{
  
  var menubar = document.getElementById("main-menubar");
  for (var i = 0; i < menubar.childNodes.length; ++i)
    menubar.childNodes[i].setAttribute("disabled", true);

  var cmd = document.getElementById("cmd_CustomizeToolbars");
  cmd.setAttribute("disabled", "true");

  var splitter = document.getElementById("urlbar-search-splitter");
  if (splitter)
    splitter.parentNode.removeChild(splitter);

  CombinedStopReload.uninit();

  BookmarksMenuButton.customizeStart();

  var customizeURL = "chrome://global/content/customizeToolbar.xul";
  gCustomizeSheet = getBoolPref("toolbar.customization.usesheet", false);

  if (gCustomizeSheet) {
    var sheetFrame = document.getElementById("customizeToolbarSheetIFrame");
    var panel = document.getElementById("customizeToolbarSheetPopup");
    sheetFrame.hidden = false;
    sheetFrame.toolbox = gNavToolbox;
    sheetFrame.panel = panel;

    
    
    
    if (sheetFrame.getAttribute("src") == customizeURL)
      sheetFrame.contentWindow.location.reload()
    else
      sheetFrame.setAttribute("src", customizeURL);

    
    
    panel.style.visibility = "hidden";
    gNavToolbox.addEventListener("beforecustomization", function () {
      gNavToolbox.removeEventListener("beforecustomization", arguments.callee, false);
      panel.style.removeProperty("visibility");
    }, false);
    panel.openPopup(gNavToolbox, "after_start", 0, 0);
    return sheetFrame.contentWindow;
  } else {
    return window.openDialog(customizeURL,
                             "CustomizeToolbar",
                             "chrome,titlebar,toolbar,location,resizable,dependent",
                             gNavToolbox);
  }
}

function BrowserToolboxCustomizeDone(aToolboxChanged) {
  if (gCustomizeSheet) {
    document.getElementById("customizeToolbarSheetIFrame").hidden = true;
    document.getElementById("customizeToolbarSheetPopup").hidePopup();
  }

  
  if (aToolboxChanged) {
    gURLBar = document.getElementById("urlbar");

    gProxyFavIcon = document.getElementById("page-proxy-favicon");
    gHomeButton.updateTooltip();
    gIdentityHandler._cacheElements();
    window.XULBrowserWindow.init();

    var backForwardDropmarker = document.getElementById("back-forward-dropmarker");
    if (backForwardDropmarker)
      backForwardDropmarker.disabled =
        document.getElementById('Browser:Back').hasAttribute('disabled') &&
        document.getElementById('Browser:Forward').hasAttribute('disabled');

#ifndef XP_MACOSX
    updateEditUIVisibility();
#endif
  }

  PlacesToolbarHelper.updateState();
  BookmarksMenuButton.customizeDone();

  UpdateUrlbarSearchSplitterState();

  CombinedStopReload.init();

  
  if (gURLBar) {
    URLBarSetURI();
    XULBrowserWindow.asyncUpdateUI();
    PlacesStarButton.updateState();
  }

  
  var menubar = document.getElementById("main-menubar");
  for (var i = 0; i < menubar.childNodes.length; ++i)
    menubar.childNodes[i].setAttribute("disabled", false);
  var cmd = document.getElementById("cmd_CustomizeToolbars");
  cmd.removeAttribute("disabled");

#ifdef XP_MACOSX
  
  if (!getBoolPref("ui.click_hold_context_menus", false))
    SetClickAndHoldHandlers();
#endif

  
  if (!gCustomizeSheet)
    window.focus();
}

function BrowserToolboxCustomizeChange() {
  gHomeButton.updatePersonalToolbarStyle();

  allTabs.readPref();
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
                   document.getElementById("cut-button") ||
                   document.getElementById("copy-button") ||
                   document.getElementById("paste-button") ? true : false;

  
  
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

var FullScreen =
{
  _XULNS: "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul",
  toggle: function()
  {
    
    this.showXULChrome("toolbar", window.fullScreen);
    this.showXULChrome("statusbar", window.fullScreen);
    document.getElementById("View:FullScreen").setAttribute("checked", !window.fullScreen);

    if (!window.fullScreen) {
      
      
      
      let fullScrToggler = document.getElementById("fullscr-toggler");
      if (!fullScrToggler) {
        fullScrToggler = document.createElement("hbox");
        fullScrToggler.id = "fullscr-toggler";
        fullScrToggler.collapsed = true;
        gNavToolbox.parentNode.insertBefore(fullScrToggler, gNavToolbox.nextSibling);
      }
      fullScrToggler.addEventListener("mouseover", this._expandCallback, false);
      fullScrToggler.addEventListener("dragenter", this._expandCallback, false);

      if (gPrefService.getBoolPref("browser.fullscreen.autohide"))
        gBrowser.mPanelContainer.addEventListener("mousemove",
                                                  this._collapseCallback, false);

      document.addEventListener("keypress", this._keyToggleCallback, false);
      document.addEventListener("popupshown", this._setPopupOpen, false);
      document.addEventListener("popuphidden", this._setPopupOpen, false);
      this._shouldAnimate = true;
      this.mouseoverToggle(false);

      
      gPrefService.addObserver("browser.fullscreen", this, false);
    }
    else {
      
      clearInterval(this._animationInterval);
      clearTimeout(this._animationTimeout);
      gNavToolbox.style.marginTop = "0px";
      if (this._isChromeCollapsed)
        this.mouseoverToggle(true);
      this._isAnimating = false;
      
      this._isPopupOpen = false;

      this.cleanup();
    }
  },

  cleanup: function () {
    if (window.fullScreen) {
      gBrowser.mPanelContainer.removeEventListener("mousemove",
                                                   this._collapseCallback, false);
      document.removeEventListener("keypress", this._keyToggleCallback, false);
      document.removeEventListener("popupshown", this._setPopupOpen, false);
      document.removeEventListener("popuphidden", this._setPopupOpen, false);
      gPrefService.removeObserver("browser.fullscreen", this);

      let fullScrToggler = document.getElementById("fullscr-toggler");
      if (fullScrToggler) {
        fullScrToggler.removeEventListener("mouseover", this._expandCallback, false);
        fullScrToggler.removeEventListener("dragenter", this._expandCallback, false);
      }
    }
  },

  observe: function(aSubject, aTopic, aData)
  {
    if (aData == "browser.fullscreen.autohide") {
      if (gPrefService.getBoolPref("browser.fullscreen.autohide")) {
        gBrowser.mPanelContainer.addEventListener("mousemove",
                                                  this._collapseCallback, false);
      }
      else {
        gBrowser.mPanelContainer.removeEventListener("mousemove",
                                                     this._collapseCallback, false);
      }
    }
  },

  
  _expandCallback: function()
  {
    FullScreen.mouseoverToggle(true);
  },
  _collapseCallback: function()
  {
    FullScreen.mouseoverToggle(false);
  },
  _keyToggleCallback: function(aEvent)
  {
    
    
    if (aEvent.keyCode == aEvent.DOM_VK_ESCAPE) {
      FullScreen._shouldAnimate = false;
      FullScreen.mouseoverToggle(false, true);
    }
    
    else if (aEvent.keyCode == aEvent.DOM_VK_F6)
      FullScreen.mouseoverToggle(true);
  },

  
  _isPopupOpen: false,
  _isChromeCollapsed: false,
  _safeToCollapse: function(forceHide)
  {
    if (!gPrefService.getBoolPref("browser.fullscreen.autohide"))
      return false;

    
    if (!forceHide && this._isPopupOpen)
      return false;

    
    if (document.commandDispatcher.focusedElement &&
        document.commandDispatcher.focusedElement.ownerDocument == document &&
        document.commandDispatcher.focusedElement.localName == "input") {
      if (forceHide)
        
        document.commandDispatcher.focusedElement.blur();
      else
        return false;
    }
    return true;
  },

  _setPopupOpen: function(aEvent)
  {
    
    
    
    
    if (aEvent.type == "popupshown" && !FullScreen._isChromeCollapsed &&
        aEvent.target.localName != "tooltip" && aEvent.target.localName != "window")
      FullScreen._isPopupOpen = true;
    else if (aEvent.type == "popuphidden" && aEvent.target.localName != "tooltip" &&
             aEvent.target.localName != "window")
      FullScreen._isPopupOpen = false;
  },

  
  getAutohide: function(aItem)
  {
    aItem.setAttribute("checked", gPrefService.getBoolPref("browser.fullscreen.autohide"));
  },
  setAutohide: function()
  {
    gPrefService.setBoolPref("browser.fullscreen.autohide", !gPrefService.getBoolPref("browser.fullscreen.autohide"));
  },

  
  _shouldAnimate: true,
  _isAnimating: false,
  _animationTimeout: null,
  _animationInterval: null,
  _animateUp: function()
  {
    
    if (!window.fullScreen || !FullScreen._safeToCollapse(false)) {
      FullScreen._isAnimating = false;
      FullScreen._shouldAnimate = true;
      return;
    }

    var animateFrameAmount = 2;
    function animateUpFrame() {
      animateFrameAmount *= 2;
      if (animateFrameAmount >= gNavToolbox.boxObject.height) {
        
        clearInterval(FullScreen._animationInterval);
        gNavToolbox.style.marginTop = "0px";
        FullScreen._isAnimating = false;
        FullScreen._shouldAnimate = false; 
        FullScreen.mouseoverToggle(false);
        return;
      }
      gNavToolbox.style.marginTop = (animateFrameAmount * -1) + "px";
    }

    FullScreen._animationInterval = setInterval(animateUpFrame, 70);
  },

  mouseoverToggle: function(aShow, forceHide)
  {
    
    
    
    
    if (aShow != this._isChromeCollapsed || (!aShow && this._isAnimating) ||
        (!aShow && !this._safeToCollapse(forceHide)))
      return;

    
    
    
    
    if (gPrefService.getIntPref("browser.fullscreen.animateUp") == 0)
      this._shouldAnimate = false;

    if (!aShow && this._shouldAnimate) {
      this._isAnimating = true;
      this._shouldAnimate = false;
      this._animationTimeout = setTimeout(this._animateUp, 800);
      return;
    }

    
    if (aShow) {
      gBrowser.mPanelContainer.addEventListener("mousemove",
                                                this._collapseCallback, false);
    }
    else {
      gBrowser.mPanelContainer.removeEventListener("mousemove",
                                                   this._collapseCallback, false);
    }

    
    
    gNavToolbox.style.marginTop = aShow ? "" : -gNavToolbox.clientHeight + "px";

    document.getElementById("fullscr-toggler").collapsed = aShow;
    this._isChromeCollapsed = !aShow;
    if (gPrefService.getIntPref("browser.fullscreen.animateUp") == 2)
      this._shouldAnimate = true;
  },

  showXULChrome: function(aTag, aShow)
  {
    var els = document.getElementsByTagNameNS(this._XULNS, aTag);

    for (var i = 0; i < els.length; ++i) {
      
      if (els[i].getAttribute("fullscreentoolbar") == "true") {
        if (!aShow) {

          var toolbarMode = els[i].getAttribute("mode");
          if (toolbarMode != "text") {
            els[i].setAttribute("saved-mode", toolbarMode);
            els[i].setAttribute("saved-iconsize",
                                els[i].getAttribute("iconsize"));
            els[i].setAttribute("mode", "icons");
            els[i].setAttribute("iconsize", "small");
          }

          
          
          els[i].setAttribute("saved-context",
                              els[i].getAttribute("context"));
          if (els[i].id == "nav-bar")
            els[i].setAttribute("context", "autohide-context");
          else
            els[i].removeAttribute("context");

          
          
          els[i].setAttribute("inFullscreen", true);
        }
        else {
          function restoreAttr(attrName) {
            var savedAttr = "saved-" + attrName;
            if (els[i].hasAttribute(savedAttr)) {
              els[i].setAttribute(attrName, els[i].getAttribute(savedAttr));
              els[i].removeAttribute(savedAttr);
            }
          }

          restoreAttr("mode");
          restoreAttr("iconsize");
          restoreAttr("context");

          els[i].removeAttribute("inFullscreen");
        }
      } else {
        
        
        if (aShow)
          els[i].removeAttribute("moz-collapsed");
        else
          els[i].setAttribute("moz-collapsed", "true");
      }
    }

    if (aShow) {
      gNavToolbox.removeAttribute("inFullscreen");
      document.documentElement.removeAttribute("inFullscreen");
    } else {
      gNavToolbox.setAttribute("inFullscreen", true);
      document.documentElement.setAttribute("inFullscreen", true);
    }

    var controls = document.getElementsByAttribute("fullscreencontrol", "true");
    for (var i = 0; i < controls.length; ++i)
      controls[i].hidden = aShow;
  }
};










function mimeTypeIsTextBased(aMimeType)
{
  return /^text\/|\+xml$/.test(aMimeType) ||
         aMimeType == "application/x-javascript" ||
         aMimeType == "application/javascript" ||
         aMimeType == "application/xml" ||
         aMimeType == "mozilla.application/cached-xul";
}

var XULBrowserWindow = {
  
  status: "",
  defaultStatus: "",
  jsStatus: "",
  jsDefaultStatus: "",
  overLink: "",
  startTime: 0,
  statusText: "",
  isBusy: false,

  _progressCollapseTimer: 0,

  QueryInterface: function (aIID) {
    if (aIID.equals(Ci.nsIWebProgressListener) ||
        aIID.equals(Ci.nsIWebProgressListener2) ||
        aIID.equals(Ci.nsISupportsWeakReference) ||
        aIID.equals(Ci.nsIXULBrowserWindow) ||
        aIID.equals(Ci.nsISupports))
      return this;
    throw Cr.NS_NOINTERFACE;
  },

  get statusMeter () {
    delete this.statusMeter;
    return this.statusMeter = document.getElementById("statusbar-icon");
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
    delete this.statusTextField;
    return this.statusTextField = document.getElementById("statusbar-display");
  },
  get securityButton () {
    delete this.securityButton;
    return this.securityButton = document.getElementById("security-button");
  },
  get isImage () {
    delete this.isImage;
    return this.isImage = document.getElementById("isImage");
  },
  get _uriFixup () {
    delete this._uriFixup;
    return this._uriFixup = Cc["@mozilla.org/docshell/urifixup;1"]
                              .getService(Ci.nsIURIFixup);
  },

  init: function () {
    this.throbberElement = document.getElementById("navigator-throbber");

    
    
    var securityUI = gBrowser.securityUI;
    this._hostChanged = true;
    this.onSecurityChange(null, null, securityUI.state);
  },

  destroy: function () {
    
    delete this.throbberElement;
    delete this.statusMeter;
    delete this.stopCommand;
    delete this.reloadCommand;
    delete this.statusTextField;
    delete this.securityButton;
    delete this.statusText;
  },

  setJSStatus: function (status) {
    this.jsStatus = status;
    this.updateStatusField();
  },

  setJSDefaultStatus: function (status) {
    this.jsDefaultStatus = status;
    this.updateStatusField();
  },

  setDefaultStatus: function (status) {
    this.defaultStatus = status;
    this.updateStatusField();
  },

  setOverLink: function (link, b) {
    
    
    this.overLink = link.replace(/[\u200e\u200f\u202a\u202b\u202c\u202d\u202e]/g,
                                 encodeURIComponent);
    this.updateStatusField();
  },

  updateStatusField: function () {
    var text = this.overLink || this.status || this.jsStatus || this.jsDefaultStatus || this.defaultStatus;

    
    
    if (this.statusText != text) {
      this.statusTextField.label = text;
      this.statusText = text;
    }
  },

  onLinkIconAvailable: function (aIconURL) {
    if (gProxyFavIcon && gBrowser.userTypedValue === null)
      PageProxySetIcon(aIconURL); 
  },

  onProgressChange: function (aWebProgress, aRequest,
                              aCurSelfProgress, aMaxSelfProgress,
                              aCurTotalProgress, aMaxTotalProgress) {
    
    
    if (aMaxTotalProgress > 0 && this._busyUI) {
      
      
      
      let percentage = (aCurTotalProgress * 100) / aMaxTotalProgress;
      this.statusMeter.value = percentage;
    }
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
    if (aStateFlags & nsIWebProgressListener.STATE_START &&
        aStateFlags & nsIWebProgressListener.STATE_IS_NETWORK) {

      if (aRequest && aWebProgress.DOMWindow == content)
        this.startDocumentLoad(aRequest);

      this.isBusy = true;

      if (!(aStateFlags & nsIWebProgressListener.STATE_RESTORING)) {
        this._busyUI = true;

        
        if (this.throbberElement)
          this.throbberElement.setAttribute("busy", "true");

        
        this.statusMeter.value = 0;  
        if (this._progressCollapseTimer) {
          clearTimeout(this._progressCollapseTimer);
          this._progressCollapseTimer = 0;
        }
        else
          this.statusMeter.parentNode.collapsed = false;

        
        this.stopCommand.removeAttribute("disabled");
        CombinedStopReload.switchToStop();
      }
    }
    else if (aStateFlags & nsIWebProgressListener.STATE_STOP) {
      if (aStateFlags & nsIWebProgressListener.STATE_IS_NETWORK) {
        if (aWebProgress.DOMWindow == content) {
          if (aRequest)
            this.endDocumentLoad(aRequest, aStatus);
          if (!gBrowser.mTabbedMode && !gBrowser.getIcon())
            gBrowser.useDefaultIcon(gBrowser.selectedTab);
        }
      }

      
      
      
      if (aRequest) {
        let msg = "";
        let location;
        
        if (aRequest instanceof nsIChannel || "URI" in aRequest) {
          location = aRequest.URI;

          
          if (location.scheme == "keyword" && aWebProgress.DOMWindow == content)
            gBrowser.userTypedValue = null;

          if (location.spec != "about:blank") {
            switch (aStatus) {
              case Components.results.NS_BINDING_ABORTED:
                msg = gNavigatorBundle.getString("nv_stopped");
                break;
              case Components.results.NS_ERROR_NET_TIMEOUT:
                msg = gNavigatorBundle.getString("nv_timeout");
                break;
            }
          }
        }
        
        
        if (!msg && (!location || location.spec != "about:blank"))
          msg = gNavigatorBundle.getString("nv_done");

        this.status = "";
        this.setDefaultStatus(msg);

        
        if (content.document && mimeTypeIsTextBased(content.document.contentType))
          this.isImage.removeAttribute('disabled');
        else
          this.isImage.setAttribute('disabled', 'true');
      }

      this.isBusy = false;

      if (this._busyUI) {
        this._busyUI = false;

        
        this._progressCollapseTimer = setTimeout(function (self) {
          self.statusMeter.parentNode.collapsed = true;
          self._progressCollapseTimer = 0;
        }, 100, this);

        if (this.throbberElement)
          this.throbberElement.removeAttribute("busy");

        this.stopCommand.setAttribute("disabled", "true");
        CombinedStopReload.switchToReload(aRequest instanceof Ci.nsIRequest);
      }
    }
  },

  onLocationChange: function (aWebProgress, aRequest, aLocationURI) {
    var location = aLocationURI ? aLocationURI.spec : "";
    this._hostChanged = true;

    if (document.tooltipNode) {
      
      if (aWebProgress.DOMWindow == content) {
        document.getElementById("aHTMLTooltip").hidePopup();
        document.tooltipNode = null;
      }
      else {
        for (let tooltipWindow =
               document.tooltipNode.ownerDocument.defaultView;
             tooltipWindow != tooltipWindow.parent;
             tooltipWindow = tooltipWindow.parent) {
          if (tooltipWindow == aWebProgress.DOMWindow) {
            document.getElementById("aHTMLTooltip").hidePopup();
            document.tooltipNode = null;
            break;
          }
        }
      }
    }

    
    
    
    
    
    
    
    
    var selectedBrowser = gBrowser.selectedBrowser;
    if (selectedBrowser.lastURI) {
      let oldSpec = selectedBrowser.lastURI.spec;
      let oldIndexOfHash = oldSpec.indexOf("#");
      if (oldIndexOfHash != -1)
        oldSpec = oldSpec.substr(0, oldIndexOfHash);
      let newSpec = location;
      let newIndexOfHash = newSpec.indexOf("#");
      if (newIndexOfHash != -1)
        newSpec = newSpec.substr(0, newSpec.indexOf("#"));
      if (newSpec != oldSpec) {
        
        
        let nBox = gBrowser.getNotificationBox(selectedBrowser);
        nBox.removeTransientNotifications();

        
        
        
        if (!__lookupGetter__("PopupNotifications"))
          PopupNotifications.locationChange();
      }
    }

    
    if (content.document && mimeTypeIsTextBased(content.document.contentType))
      this.isImage.removeAttribute('disabled');
    else
      this.isImage.setAttribute('disabled', 'true');

    this.setOverLink("", null);

    
    
    
    

    var browser = gBrowser.selectedBrowser;
    if (aWebProgress.DOMWindow == content) {
      if ((location == "about:blank" && !content.opener) ||
          location == "") {  
                             
        this.reloadCommand.setAttribute("disabled", "true");
      } else {
        this.reloadCommand.removeAttribute("disabled");
      }

      if (!gBrowser.mTabbedMode && aWebProgress.isLoadingDocument)
        gBrowser.setIcon(gBrowser.selectedTab, null);

      if (gURLBar) {
        
        let uri = aLocationURI;
        try {
          uri = this._uriFixup.createExposableURI(uri);
        } catch (e) {}
        URLBarSetURI(uri);

        
        PlacesStarButton.updateState();
      }
    }
    UpdateBackForwardCommands(gBrowser.webNavigation);

    if (gFindBarInitialized) {
      if (gFindBar.findMode != gFindBar.FIND_NORMAL) {
        
        gFindBar.close();
      }

      
      

      
      gFindBar.getElement("highlight").checked = false;      
    }

    
    
    if (aRequest)
      setTimeout(function () { XULBrowserWindow.asyncUpdateUI(); }, 0);
    else
      this.asyncUpdateUI();
  },

  asyncUpdateUI: function () {
    FeedHandler.updateFeeds();
    BrowserSearch.updateSearchButton();
  },

  onStatusChange: function (aWebProgress, aRequest, aStatus, aMessage) {
    this.status = aMessage;
    this.updateStatusField();
  },

  
  _state: null,
  _tooltipText: null,
  _hostChanged: false, 

  onSecurityChange: function (aWebProgress, aRequest, aState) {
    
    
    if (this._state == aState &&
        this._tooltipText == gBrowser.securityUI.tooltipText &&
        !this._hostChanged) {
#ifdef DEBUG
      try {
        var contentHost = gBrowser.contentWindow.location.host;
        if (this._host !== undefined && this._host != contentHost) {
            Components.utils.reportError(
              "ASSERTION: browser.js host is inconsistent. Content window has " +
              "<" + contentHost + "> but cached host is <" + this._host + ">.\n"
            );
        }
      } catch (ex) {}
#endif
      return;
    }
    this._state = aState;

#ifdef DEBUG
    try {
      this._host = gBrowser.contentWindow.location.host;
    } catch(ex) {
      this._host = null;
    }
#endif

    this._hostChanged = false;
    this._tooltipText = gBrowser.securityUI.tooltipText

    
    
    const wpl = Components.interfaces.nsIWebProgressListener;
    const wpl_security_bits = wpl.STATE_IS_SECURE |
                              wpl.STATE_IS_BROKEN |
                              wpl.STATE_IS_INSECURE |
                              wpl.STATE_SECURE_HIGH |
                              wpl.STATE_SECURE_MED |
                              wpl.STATE_SECURE_LOW;
    var level;

    switch (this._state & wpl_security_bits) {
      case wpl.STATE_IS_SECURE | wpl.STATE_SECURE_HIGH:
        level = "high";
        break;
      case wpl.STATE_IS_SECURE | wpl.STATE_SECURE_MED:
      case wpl.STATE_IS_SECURE | wpl.STATE_SECURE_LOW:
        level = "low";
        break;
      case wpl.STATE_IS_BROKEN:
        level = "broken";
        break;
    }

    if (level) {
      this.securityButton.setAttribute("level", level);
      this.securityButton.hidden = false;
      
      
      if (gURLBar)
        gURLBar.setAttribute("level", level);
    } else {
      this.securityButton.hidden = true;
      this.securityButton.removeAttribute("level");
      if (gURLBar)
        gURLBar.removeAttribute("level");
    }

    this.securityButton.setAttribute("tooltiptext", this._tooltipText);

    
    
    
    var location = gBrowser.contentWindow.location;
    var locationObj = {};
    try {
      locationObj.host = location.host;
      locationObj.hostname = location.hostname;
      locationObj.port = location.port;
    } catch (ex) {
      
      
      
    }
    gIdentityHandler.checkIdentity(this._state, locationObj);
  },

  
  onUpdateCurrentBrowser: function (aStateFlags, aStatus, aMessage, aTotalProgress) {
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
    this.onProgressChange(gBrowser.webProgress, 0, 0, aTotalProgress, 1);
  },

  startDocumentLoad: function (aRequest) {
    
    gBrowser.selectedBrowser.feeds = null;

    
    gBrowser.selectedBrowser.engines = null;

    var uri = aRequest.QueryInterface(Ci.nsIChannel).URI;
    try {
      Services.obs.notifyObservers(content, "StartDocumentLoad", uri.spec);
    } catch (e) {
    }
  },

  endDocumentLoad: function (aRequest, aStatus) {
    var urlStr = aRequest.QueryInterface(Ci.nsIChannel).originalURI.spec;

    var notification = Components.isSuccessCode(aStatus) ? "EndDocumentLoad" : "FailDocumentLoad";
    try {
      Services.obs.notifyObservers(content, notification, urlStr);
    } catch (e) {
    }
  }
};

var CombinedStopReload = {
  init: function () {
    if (this._initialized)
      return;

    var stop = document.getElementById("stop-button");
    if (!stop)
      return;

    var reload = document.getElementById("reload-button");
    if (!reload)
      return;

    if (!(reload.nextSibling == stop))
      return;

    this._initialized = true;
    if (XULBrowserWindow.stopCommand.getAttribute("disabled") != "true")
      reload.setAttribute("displaystop", "true");
    stop.addEventListener("click", this, false);
    this.stop = stop;
    this.reload = reload;
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

    if (!aDelay || this._stopClicked) {
      this._stopClicked = false;
      this._cancelTransition();
      this.reload.removeAttribute("displaystop");
      return;
    }

    if (this._timer)
      return;

    this._timer = setTimeout(function (self) {
      self._timer = 0;
      self.reload.removeAttribute("displaystop");
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
  onProgressChange: function (aBrowser, aWebProgress, aRequest,
                              aCurSelfProgress, aMaxSelfProgress,
                              aCurTotalProgress, aMaxTotalProgress) {
  },

  onStateChange: function (aBrowser, aWebProgress, aRequest, aStateFlags, aStatus) {
#ifdef MOZ_CRASHREPORTER
    if (aRequest instanceof Ci.nsIChannel &&
        aStateFlags & Ci.nsIWebProgressListener.STATE_START &&
        aStateFlags & Ci.nsIWebProgressListener.STATE_IS_DOCUMENT &&
        gCrashReporter.enabled) {
      gCrashReporter.annotateCrashReport("URL", aRequest.URI.spec);
    }
#endif
  },

  onLocationChange: function (aBrowser, aWebProgress, aRequest, aLocationURI) {
    
    if (aBrowser.contentWindow == aWebProgress.DOMWindow)
      FullZoom.onLocationChange(aLocationURI, false, aBrowser);
  },

  onStatusChange: function (aBrowser, aWebProgress, aRequest, aStatus, aMessage) {
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
  },

  onSecurityChange: function (aBrowser, aWebProgress, aRequest, aState) {
  }
}

function nsBrowserAccess() { }

nsBrowserAccess.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIBrowserDOMWindow, Ci.nsISupports]),

  openURI: function (aURI, aOpener, aWhere, aContext) {
    var newWindow = null;
    var isExternal = (aContext == Ci.nsIBrowserDOMWindow.OPEN_EXTERNAL);

    if (isExternal && aURI && aURI.schemeIs("chrome")) {
      dump("use -chrome command-line option to load external chrome urls\n");
      return null;
    }

    if (aWhere == Ci.nsIBrowserDOMWindow.OPEN_DEFAULTWINDOW)
      aWhere = gPrefService.getIntPref("browser.link.open_newwindow");
    switch (aWhere) {
      case Ci.nsIBrowserDOMWindow.OPEN_NEWWINDOW :
        
        
        var url = aURI ? aURI.spec : "about:blank";
        
        
        newWindow = openDialog(getBrowserURL(), "_blank", "all,dialog=no", url, null, null, null);
        break;
      case Ci.nsIBrowserDOMWindow.OPEN_NEWTAB :
        let win, needToFocusWin;

        
        if (!window.document.documentElement.getAttribute("chromehidden"))
          win = window;
        else {
          win = Cc["@mozilla.org/browser/browserglue;1"]
                  .getService(Ci.nsIBrowserGlue)
                  .getMostRecentBrowserWindow();
          needToFocusWin = true;
        }

        if (!win) {
          
          return null;
        }

        if (isExternal && (!aURI || aURI.spec == "about:blank")) {
          win.BrowserOpenTab(); 
          win.focus();
          newWindow = win.content;
          break;
        }

        let loadInBackground = gPrefService.getBoolPref("browser.tabs.loadDivertedInBackground");
        let referrer = aOpener ? makeURI(aOpener.location.href) : null;

        let tab = win.gBrowser.loadOneTab(aURI ? aURI.spec : "about:blank", {
                                          referrerURI: referrer,
                                          fromExternal: isExternal,
                                          inBackground: loadInBackground});
        let browser = win.gBrowser.getBrowserForTab(tab);

        newWindow = browser.contentWindow;
        if (needToFocusWin || (!loadInBackground && isExternal))
          newWindow.focus();
        break;
      default : 
        newWindow = content;
        if (aURI) {
          let referrer = aOpener ? makeURI(aOpener.location.href) : null;
          let loadflags = isExternal ?
                            Ci.nsIWebNavigation.LOAD_FLAGS_FROM_EXTERNAL :
                            Ci.nsIWebNavigation.LOAD_FLAGS_NONE;
          gBrowser.loadURIWithFlags(aURI.spec, loadflags, referrer, null, null);
        }
        if (!gPrefService.getBoolPref("browser.tabs.loadDivertedInBackground"))
          content.focus();
    }
    return newWindow;
  },

  isTabContentWindow: function (aWindow) {
    return gBrowser.browsers.some(function (browser) browser.contentWindow == aWindow);
  }
}

function onViewToolbarsPopupShowing(aEvent) {
  var popup = aEvent.target;
  if (popup != aEvent.currentTarget)
    return;

  var i;

  
  for (i = popup.childNodes.length-1; i >= 0; --i) {
    var deadItem = popup.childNodes[i];
    if (deadItem.hasAttribute("toolbarindex"))
      popup.removeChild(deadItem);
  }

  var firstMenuItem = popup.firstChild;

  for (i = 0; i < gNavToolbox.childNodes.length; ++i) {
    var toolbar = gNavToolbox.childNodes[i];
    var toolbarName = toolbar.getAttribute("toolbarname");
    if (toolbarName) {
      let menuItem = document.createElement("menuitem");
      let hidingAttribute = toolbar.getAttribute("type") == "menubar" ?
                            "autohide" : "collapsed";
      menuItem.setAttribute("toolbarindex", i);
      menuItem.setAttribute("type", "checkbox");
      menuItem.setAttribute("label", toolbarName);
      menuItem.setAttribute("checked", toolbar.getAttribute(hidingAttribute) != "true");
      if (popup.id != "appmenu_customizeMenu")
        menuItem.setAttribute("accesskey", toolbar.getAttribute("accesskey"));
      popup.insertBefore(menuItem, firstMenuItem);

      menuItem.addEventListener("command", onViewToolbarCommand, false);
    }
    toolbar = toolbar.nextSibling;
  }
}

function onViewToolbarCommand(aEvent) {
  var index = aEvent.originalTarget.getAttribute("toolbarindex");
  var toolbar = gNavToolbox.childNodes[index];
  var hidingAttribute = toolbar.getAttribute("type") == "menubar" ?
                        "autohide" : "collapsed";

  toolbar.setAttribute(hidingAttribute,
                       aEvent.originalTarget.getAttribute("checked") != "true");
  document.persist(toolbar.id, hidingAttribute);

  BookmarksMenuButton.updatePosition();

#ifdef MENUBAR_CAN_AUTOHIDE
  updateAppButtonDisplay();
#endif
}

var TabsOnTop = {
  toggle: function () {
    this.enabled = !this.enabled;
  },
  syncCommand: function () {
    let enabled = this.enabled;
    document.getElementById("cmd_ToggleTabsOnTop")
            .setAttribute("checked", enabled);
    document.documentElement.setAttribute("tabsontop", enabled);
  },
  get enabled () {
    return gNavToolbox.getAttribute("tabsontop") == "true";
  },
  set enabled (val) {
    gNavToolbox.setAttribute("tabsontop", !!val);
    this.syncCommand();

    return val;
  }
}

#ifdef MENUBAR_CAN_AUTOHIDE
function updateAppButtonDisplay() {
  var displayAppButton = window.menubar.visible &&
    document.getElementById("toolbar-menubar").getAttribute("autohide") == "true";

  document.getElementById("appmenu-button-container").hidden = !displayAppButton;

  if (displayAppButton)
    document.documentElement.setAttribute("chromemargin", "0,-1,-1,-1");
  else
    document.documentElement.removeAttribute("chromemargin");
}
#endif

function displaySecurityInfo()
{
  BrowserPageInfo(null, "securityTab");
}





















function toggleSidebar(commandID, forceOpen) {

  var sidebarBox = document.getElementById("sidebar-box");
  if (!commandID)
    commandID = sidebarBox.getAttribute("sidebarcommand");

  var sidebarBroadcaster = document.getElementById(commandID);
  var sidebar = document.getElementById("sidebar"); 
  var sidebarTitle = document.getElementById("sidebar-title");
  var sidebarSplitter = document.getElementById("sidebar-splitter");

  if (sidebarBroadcaster.getAttribute("checked") == "true") {
    if (!forceOpen) {
      sidebarBroadcaster.removeAttribute("checked");
      sidebarBox.setAttribute("sidebarcommand", "");
      sidebarTitle.value = "";
      sidebar.setAttribute("src", "about:blank");
      sidebarBox.hidden = true;
      sidebarSplitter.hidden = true;
      content.focus();
    } else {
      fireSidebarFocusedEvent();
    }
    return;
  }

  

  
  var broadcasters = document.getElementsByAttribute("group", "sidebar");
  for (var i = 0; i < broadcasters.length; ++i) {
    
    
    if (broadcasters[i].localName != "broadcaster")
      continue;

    if (broadcasters[i] != sidebarBroadcaster)
      broadcasters[i].removeAttribute("checked");
    else
      sidebarBroadcaster.setAttribute("checked", "true");
  }

  sidebarBox.hidden = false;
  sidebarSplitter.hidden = false;

  var url = sidebarBroadcaster.getAttribute("sidebarurl");
  var title = sidebarBroadcaster.getAttribute("sidebartitle");
  if (!title)
    title = sidebarBroadcaster.getAttribute("label");
  sidebar.setAttribute("src", url); 
  sidebarBox.setAttribute("sidebarcommand", sidebarBroadcaster.id);
  sidebarTitle.value = title;

  
  
  
  
  
  sidebarBox.setAttribute("src", url);

  if (sidebar.contentDocument.location.href != url)
    sidebar.addEventListener("load", sidebarOnLoad, true);
  else 
    fireSidebarFocusedEvent();
}

function sidebarOnLoad(event) {
  var sidebar = document.getElementById("sidebar");
  sidebar.removeEventListener("load", sidebarOnLoad, true);
  
  
  
  setTimeout(fireSidebarFocusedEvent, 0);
}







function fireSidebarFocusedEvent() {
  var sidebar = document.getElementById("sidebar");
  var event = document.createEvent("Events");
  event.initEvent("SidebarFocused", true, false);
  sidebar.contentWindow.dispatchEvent(event);
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
      var SBS = Cc["@mozilla.org/intl/stringbundle;1"].getService(Ci.nsIStringBundleService);
      var configBundle = SBS.createBundle("chrome://branding/locale/browserconfig.properties");
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
  }
};










function getBrowserSelection(aCharLen) {
  
  const kMaxSelectionLen = 150;
  const charLen = Math.min(aCharLen || kMaxSelectionLen, kMaxSelectionLen);

  var focusedWindow = document.commandDispatcher.focusedWindow;
  var selection = focusedWindow.getSelection().toString();

  if (selection) {
    if (selection.length > charLen) {
      
      var pattern = new RegExp("^(?:\\s*.){0," + charLen + "}");
      pattern.test(selection);
      selection = RegExp.lastMatch;
    }

    selection = selection.replace(/^\s+/, "")
                         .replace(/\s+$/, "")
                         .replace(/\s+/g, " ");

    if (selection.length > charLen)
      selection = selection.substr(0, charLen);
  }
  return selection;
}

var gWebPanelURI;
function openWebPanel(aTitle, aURI)
{
    
    toggleSidebar('viewWebPanelsSidebar', true);

    
    document.getElementById("sidebar-title").value = aTitle;

    
    var sidebar = document.getElementById("sidebar");
    if (sidebar.docShell && sidebar.contentDocument && sidebar.contentDocument.getElementById('web-panels-browser')) {
        sidebar.contentWindow.loadWebPanel(aURI);
        if (gWebPanelURI) {
            gWebPanelURI = "";
            sidebar.removeEventListener("load", asyncOpenWebPanel, true);
        }
    }
    else {
        
        if (!gWebPanelURI)
            sidebar.addEventListener("load", asyncOpenWebPanel, true);
        gWebPanelURI = aURI;
    }
}

function asyncOpenWebPanel(event)
{
    var sidebar = document.getElementById("sidebar");
    if (gWebPanelURI && sidebar.contentDocument && sidebar.contentDocument.getElementById('web-panels-browser'))
        sidebar.contentWindow.loadWebPanel(gWebPanelURI);
    gWebPanelURI = "";
    sidebar.removeEventListener("load", asyncOpenWebPanel, true);
}







 
 
 
 function contentAreaClick(event, fieldNormalClicks)
 {
   if (!event.isTrusted || event.getPreventDefault()) {
     return true;
   }

   var target = event.target;
   var linkNode;

   if (target instanceof HTMLAnchorElement ||
       target instanceof HTMLAreaElement ||
       target instanceof HTMLLinkElement) {
     if (target.hasAttribute("href"))
       linkNode = target;

     
     
     
     var parent = target.parentNode;
     while (parent) {
       if (parent instanceof HTMLAnchorElement ||
           parent instanceof HTMLAreaElement ||
           parent instanceof HTMLLinkElement) {
           if (parent.hasAttribute("href"))
             linkNode = parent;
       }
       parent = parent.parentNode;
     }
   }
   else {
     linkNode = event.originalTarget;
     while (linkNode && !(linkNode instanceof HTMLAnchorElement))
       linkNode = linkNode.parentNode;
     
     
     if (linkNode && !linkNode.hasAttribute("href"))
       linkNode = null;
   }
   var wrapper = null;
   if (linkNode) {
     wrapper = linkNode;
     if (event.button == 0 && !event.ctrlKey && !event.shiftKey &&
         !event.altKey && !event.metaKey) {
       
       
       
       
       
       target = wrapper.getAttribute("target");
       if (fieldNormalClicks &&
           (!target || target == "_content" || target  == "_main"))
         
       {
         if (!wrapper.href)
           return true;
         if (wrapper.getAttribute("onclick"))
           return true;
         
         if (wrapper.href.substr(0, 11) === "javascript:")
           return true;
         
         if (wrapper.href.substr(0, 5) === "data:")
           return true;

         try {
           urlSecurityCheck(wrapper.href, wrapper.ownerDocument.nodePrincipal);
         }
         catch(ex) {
           return false;
         }

         var postData = { };
         var url = getShortcutOrURI(wrapper.href, postData);
         if (!url)
           return true;
         loadURI(url, null, postData.value, false);
         event.preventDefault();
         return false;
       }
       else if (linkNode.getAttribute("rel") == "sidebar") {
         
         
         
         PlacesUIUtils.showMinimalAddBookmarkUI(makeURI(wrapper.href),
                                                wrapper.getAttribute("title"),
                                                null, null, true, true);
         event.preventDefault();
         return false;
       }
     }
     else {
       handleLinkClick(event, wrapper.href, linkNode);
     }

     return true;
   } else {
     
     var href, realHref, baseURI;
     linkNode = target;
     while (linkNode) {
       if (linkNode.nodeType == Node.ELEMENT_NODE) {
         wrapper = linkNode;

         realHref = wrapper.getAttributeNS("http://www.w3.org/1999/xlink", "href");
         if (realHref) {
           href = realHref;
           baseURI = wrapper.baseURI
         }
       }
       linkNode = linkNode.parentNode;
     }
     if (href) {
       href = makeURLAbsolute(baseURI, href);
       handleLinkClick(event, href, null);
       return true;
     }
   }
   if (event.button == 1 &&
       gPrefService.getBoolPref("middlemouse.contentLoadURL") &&
       !gPrefService.getBoolPref("general.autoScroll")) {
     middleMousePaste(event);
   }
   return true;
 }

function handleLinkClick(event, href, linkNode)
{
  var doc = event.target.ownerDocument;

  switch (event.button) {
    case 0:    
#ifdef XP_MACOSX
      if (event.metaKey) { 
#else
      if (event.ctrlKey) {
#endif
        openNewTabWith(href, doc, null, event, false);
        event.stopPropagation();
        return true;
      }

      if (event.shiftKey && event.altKey) {
        var feedService =
            Cc["@mozilla.org/browser/feeds/result-service;1"].
            getService(Ci.nsIFeedResultService);
        feedService.forcePreviewPage = true;
        loadURI(href, null, null, false);
        return false;
      }

      if (event.shiftKey) {
        openNewWindowWith(href, doc, null, false);
        event.stopPropagation();
        return true;
      }

      if (event.altKey) {
        saveURL(href, linkNode ? gatherTextUnder(linkNode) : "", null, true,
                true, doc.documentURIObject);
        return true;
      }

      return false;
    case 1:    
      var tab = gPrefService.getBoolPref("browser.tabs.opentabfor.middleclick");
      if (tab)
        openNewTabWith(href, doc, null, event, false);
      else
        openNewWindowWith(href, doc, null, false);
      event.stopPropagation();
      return true;
  }
  return false;
}

function middleMousePaste(event) {
  var url = getShortcutOrURI(readFromClipboard());
  try {
    makeURI(url);
  } catch (ex) {
    
    return;
  }

  try {
    addToUrlbarHistory(url);
  } catch (ex) {
    
    
    Cu.reportError(ex);
  }

  openUILink(url,
             event,
             true );

  event.stopPropagation();
}

function handleDroppedLink(event, url, name)
{
  let postData = { };
  let uri = getShortcutOrURI(url, postData);
  if (uri)
    loadURI(uri, null, postData.value, false);

  
  
  event.preventDefault();
};

function MultiplexHandler(event)
{ try {
    var node = event.target;
    var name = node.getAttribute('name');

    if (name == 'detectorGroup') {
        SetForcedDetector(true);
        SelectDetector(event, false);
    } else if (name == 'charsetGroup') {
        var charset = node.getAttribute('id');
        charset = charset.substring('charset.'.length, charset.length)
        SetForcedCharset(charset);
    } else if (name == 'charsetCustomize') {
        
    } else {
        SetForcedCharset(node.getAttribute('id'));
    }
    } catch(ex) { alert(ex); }
}

function SelectDetector(event, doReload)
{
    var uri =  event.target.getAttribute("id");
    var prefvalue = uri.substring('chardet.'.length, uri.length);
    if ("off" == prefvalue) { 
        prefvalue = "";
    }

    try {
        var str =  Cc["@mozilla.org/supports-string;1"].
                   createInstance(Ci.nsISupportsString);

        str.data = prefvalue;
        gPrefService.setComplexValue("intl.charset.detector", Ci.nsISupportsString, str);
        if (doReload)
          window.content.location.reload();
    }
    catch (ex) {
        dump("Failed to set the intl.charset.detector preference.\n");
    }
}

function SetForcedDetector(doReload)
{
    BrowserSetForcedDetector(doReload);
}

function SetForcedCharset(charset)
{
    BrowserSetForcedCharacterSet(charset);
}

function BrowserSetForcedCharacterSet(aCharset)
{
  var docCharset = gBrowser.docShell.QueryInterface(Ci.nsIDocCharset);
  docCharset.charset = aCharset;
  
  PlacesUtils.history.setCharsetForURI(getWebNavigation().currentURI, aCharset);
  BrowserReloadWithFlags(nsIWebNavigation.LOAD_FLAGS_CHARSET_CHANGE);
}

function BrowserSetForcedDetector(doReload)
{
  gBrowser.documentCharsetInfo.forcedDetector = true;
  if (doReload)
    BrowserReloadWithFlags(nsIWebNavigation.LOAD_FLAGS_CHARSET_CHANGE);
}

function UpdateCurrentCharset()
{
    
    var wnd = document.commandDispatcher.focusedWindow;
    if ((window == wnd) || (wnd == null)) wnd = window.content;

    
    if (gPrevCharset) {
        var pref_item = document.getElementById('charset.' + gPrevCharset);
        if (pref_item)
          pref_item.setAttribute('checked', 'false');
    }

    var menuitem = document.getElementById('charset.' + wnd.document.characterSet);
    if (menuitem) {
        menuitem.setAttribute('checked', 'true');
    }
}

function UpdateCharsetDetector() {
  var prefvalue = "off";

  try {
    prefvalue = gPrefService.getComplexValue("intl.charset.detector", Ci.nsIPrefLocalizedString).data;
  }
  catch (ex) {}

  prefvalue = "chardet." + prefvalue;

  var menuitem = document.getElementById(prefvalue);
  if (menuitem)
    menuitem.setAttribute("checked", "true");
}

function UpdateMenus(event) {
  
  
  
  UpdateCurrentCharset();
  setTimeout(UpdateCurrentCharset, 0);
  UpdateCharsetDetector();
  setTimeout(UpdateCharsetDetector, 0);
}

function CreateMenu(node) {
  Services.obs.notifyObservers(null, "charsetmenu-selected", node);
}

function charsetLoadListener(event) {
  var charset = window.content.document.characterSet;

  if (charset.length > 0 && (charset != gLastBrowserCharset)) {
    if (!gCharsetMenu)
      gCharsetMenu = Cc['@mozilla.org/rdf/datasource;1?name=charset-menu'].getService(Ci.nsICurrentCharsetListener);
    gCharsetMenu.SetCurrentCharset(charset);
    gPrevCharset = gLastBrowserCharset;
    gLastBrowserCharset = charset;
  }
}


function getAllStyleSheets(frameset) {
  var styleSheetsArray = Array.slice(frameset.document.styleSheets);
  for (let i = 0; i < frameset.frames.length; i++) {
    let frameSheets = getAllStyleSheets(frameset.frames[i]);
    styleSheetsArray = styleSheetsArray.concat(frameSheets);
  }
  return styleSheetsArray;
}

function stylesheetFillPopup(menuPopup) {
  var noStyle = menuPopup.firstChild;
  var persistentOnly = noStyle.nextSibling;
  var sep = persistentOnly.nextSibling;
  while (sep.nextSibling)
    menuPopup.removeChild(sep.nextSibling);

  var styleSheets = getAllStyleSheets(window.content);
  var currentStyleSheets = {};
  var styleDisabled = getMarkupDocumentViewer().authorStyleDisabled;
  var haveAltSheets = false;
  var altStyleSelected = false;

  for (let i = 0; i < styleSheets.length; ++i) {
    let currentStyleSheet = styleSheets[i];

    if (!currentStyleSheet.title)
      continue;

    
    if (currentStyleSheet.media.length > 0) {
      let media = currentStyleSheet.media.mediaText.split(", ");
      if (media.indexOf("screen") == -1 &&
          media.indexOf("all") == -1)
        continue;
    }

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
      menuPopup.appendChild(menuItem);
      currentStyleSheets[currentStyleSheet.title] = menuItem;
    } else if (currentStyleSheet.disabled) {
      lastWithSameTitle.removeAttribute("checked");
    }
  }

  noStyle.setAttribute("checked", styleDisabled);
  persistentOnly.setAttribute("checked", !altStyleSelected && !styleDisabled);
  persistentOnly.hidden = (window.content.document.preferredStyleSheetSet) ? haveAltSheets : false;
  sep.hidden = (noStyle.hidden && persistentOnly.hidden) || !haveAltSheets;
  return true;
}

function stylesheetInFrame(frame, title) {
  return Array.some(frame.document.styleSheets,
                    function (stylesheet) stylesheet.title == title);
}

function stylesheetSwitchFrame(frame, title) {
  var docStyleSheets = frame.document.styleSheets;

  for (let i = 0; i < docStyleSheets.length; ++i) {
    let docStyleSheet = docStyleSheets[i];

    if (title == "_nostyle")
      docStyleSheet.disabled = true;
    else if (docStyleSheet.title)
      docStyleSheet.disabled = (docStyleSheet.title != title);
    else if (docStyleSheet.disabled)
      docStyleSheet.disabled = false;
  }
}

function stylesheetSwitchAll(frameset, title) {
  if (!title || title == "_nostyle" || stylesheetInFrame(frameset, title))
    stylesheetSwitchFrame(frameset, title);

  for (let i = 0; i < frameset.frames.length; i++)
    stylesheetSwitchAll(frameset.frames[i], title);
}

function setStyleDisabled(disabled) {
  getMarkupDocumentViewer().authorStyleDisabled = disabled;
}


var BrowserOffline = {
  
  
  init: function ()
  {
    if (!this._uiElement)
      this._uiElement = document.getElementById("goOfflineMenuitem");

    Services.obs.addObserver(this, "network:offline-status-changed", false);

    this._updateOfflineUI(Services.io.offline);
  },

  uninit: function ()
  {
    try {
      Services.obs.removeObserver(this, "network:offline-status-changed");
    } catch (ex) {
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

    
    
    gPrefService.setBoolPref("browser.offline", ioService.offline);
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
    Services.obs.addObserver(this, "dom-storage-warn-quota-exceeded", false);
    Services.obs.addObserver(this, "offline-cache-update-completed", false);
  },

  uninit: function ()
  {
    Services.obs.removeObserver(this, "dom-storage-warn-quota-exceeded");
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
    for (var i = 0; i < browsers.length; ++i) {
      if (browsers[i].contentWindow == aContentWindow)
        return browsers[i];
    }
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
    for (var i = 0; i < browsers.length; ++i) {
      uri = this._getManifestURI(browsers[i].contentWindow);
      if (uri && uri.equals(aCacheUpdate.manifestURI)) {
        return browsers[i];
      }
    }

    return null;
  },

  _warnUsage: function(aBrowser, aURI) {
    if (!aBrowser)
      return;

    var notificationBox = gBrowser.getNotificationBox(aBrowser);
    var notification = notificationBox.getNotificationWithValue("offline-app-usage");
    if (!notification) {
      var buttons = [{
          label: gNavigatorBundle.getString("offlineApps.manageUsage"),
          accessKey: gNavigatorBundle.getString("offlineApps.manageUsageAccessKey"),
          callback: OfflineApps.manage
        }];

      var warnQuota = gPrefService.getIntPref("offline-apps.quota.warn");
      const priority = notificationBox.PRIORITY_WARNING_MEDIUM;
      var message = gNavigatorBundle.getFormattedString("offlineApps.usage",
                                                        [ aURI.host,
                                                          warnQuota / 1024 ]);

      notificationBox.appendNotification(message, "offline-app-usage",
                                         "chrome://browser/skin/Info.png",
                                         priority, buttons);
    }

    
    
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
    for (var i = 0; i < groups.length; i++) {
      var uri = Services.io.newURI(groups[i], null, null);
      if (uri.asciiHost == host) {
        var cache = cacheService.getActiveCache(groups[i]);
        usage += cache.usage;
      }
    }

    var storageManager = Cc["@mozilla.org/dom/storagemanager;1"].
                         getService(Ci.nsIDOMStorageManager);
    usage += storageManager.getUsage(host);

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

    var browserWindow = this._getBrowserWindowForContentWindow(aContentWindow);
    var browser = this._getBrowserForContentWindow(browserWindow,
                                                   aContentWindow);

    var currentURI = aContentWindow.document.documentURIObject;

    
    if (Services.perms.testExactPermission(currentURI, "offline-app") != Services.perms.UNKNOWN_ACTION)
      return;

    try {
      if (gPrefService.getBoolPref("offline-apps.allow_by_default")) {
        
        return;
      }
    } catch(e) {
      
    }

    var host = currentURI.asciiHost;
    var notificationBox = gBrowser.getNotificationBox(browser);
    var notificationID = "offline-app-requested-" + host;
    var notification = notificationBox.getNotificationWithValue(notificationID);

    if (notification) {
      notification.documents.push(aContentWindow.document);
    } else {
      var buttons = [{
        label: gNavigatorBundle.getString("offlineApps.allow"),
        accessKey: gNavigatorBundle.getString("offlineApps.allowAccessKey"),
        callback: function() {
          for (var i = 0; i < notification.documents.length; i++) {
            OfflineApps.allowSite(notification.documents[i]);
          }
        }
      },{
        label: gNavigatorBundle.getString("offlineApps.never"),
        accessKey: gNavigatorBundle.getString("offlineApps.neverAccessKey"),
        callback: function() {
          for (var i = 0; i < notification.documents.length; i++) {
            OfflineApps.disallowSite(notification.documents[i]);
          }
        }
      },{
        label: gNavigatorBundle.getString("offlineApps.notNow"),
        accessKey: gNavigatorBundle.getString("offlineApps.notNowAccessKey"),
        callback: function() {  }
      }];

      const priority = notificationBox.PRIORITY_INFO_LOW;
      var message = gNavigatorBundle.getFormattedString("offlineApps.available",
                                                        [ host ]);
      notification =
        notificationBox.appendNotification(message, notificationID,
                                           "chrome://browser/skin/Info.png",
                                           priority, buttons);
      notification.documents = [ aContentWindow.document ];
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
    updateService.scheduleUpdate(manifestURI, aDocument.documentURIObject);
  },

  
  
  observe: function (aSubject, aTopic, aState)
  {
    if (aTopic == "dom-storage-warn-quota-exceeded") {
      if (aSubject) {
        var uri = makeURI(aSubject.location.href);

        if (OfflineApps._checkUsage(uri)) {
          var browserWindow =
            this._getBrowserWindowForContentWindow(aSubject);
          var browser = this._getBrowserForContentWindow(browserWindow,
                                                         aSubject);
          OfflineApps._warnUsage(browser, uri);
        }
      }
    } else if (aTopic == "offline-cache-update-completed") {
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

function WindowIsClosing()
{
  var reallyClose = closeWindow(false, warnAboutClosingWindow);
  if (!reallyClose)
    return false;

  var numBrowsers = gBrowser.browsers.length;
  for (let i = 0; reallyClose && i < numBrowsers; ++i) {
    let ds = gBrowser.browsers[i].docShell;

    if (ds.contentViewer && !ds.contentViewer.permitUnload())
      reallyClose = false;
  }

  return reallyClose;
}






function warnAboutClosingWindow() {
  
  if (!toolbar.visible)
    return gBrowser.warnAboutClosingTabs(true);

  
  let foundOtherBrowserWindow = false;
  let e = Services.wm.getEnumerator("navigator:browser");
  while (e.hasMoreElements() && !foundOtherBrowserWindow) {
    let win = e.getNext();
    if (win != window && win.toolbar.visible)
      foundOtherBrowserWindow = true;
  }
  if (foundOtherBrowserWindow)
    return gBrowser.warnAboutClosingTabs(true);

  let os = Services.obs;

  let closingCanceled = Cc["@mozilla.org/supports-PRBool;1"].
                        createInstance(Ci.nsISupportsPRBool);
  os.notifyObservers(closingCanceled,
                     "browser-lastwindow-close-requested", null);
  if (closingCanceled.data)
    return false;

  os.notifyObservers(null, "browser-lastwindow-close-granted", null);

#ifdef XP_MACOSX
  
  
  
  return gBrowser.warnAboutClosingTabs(true);
#else
  return true;
#endif
}

var MailIntegration = {
  sendLinkForWindow: function (aWindow) {
    this.sendMessage(aWindow.location.href,
                     aWindow.document.title);
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
  switchToTabHavingURI("about:addons", true, function(browser) {
    if (aView)
      browser.contentWindow.wrappedJSObject.loadView(aView);
  });
}

function AddKeywordForSearchField() {
  var node = document.popupNode;

  var charset = node.ownerDocument.characterSet;

  var docURI = makeURI(node.ownerDocument.URL,
                       charset);

  var formURI = makeURI(node.form.getAttribute("action"),
                        charset,
                        docURI);

  var spec = formURI.spec;

  var isURLEncoded =
               (node.form.method.toUpperCase() == "POST"
                && (node.form.enctype == "application/x-www-form-urlencoded" ||
                    node.form.enctype == ""));

  var title = gNavigatorBundle.getFormattedString("addKeywordTitleAutoFill",
                                                  [node.ownerDocument.title]);
  var description = PlacesUIUtils.getDescriptionFromDocument(node.ownerDocument);

  var el, type;
  var formData = [];

  function escapeNameValuePair(aName, aValue, aIsFormUrlEncoded) {
    if (aIsFormUrlEncoded)
      return escape(aName + "=" + aValue);
    else
      return escape(aName) + "=" + escape(aValue);
  }

  for (var i=0; i < node.form.elements.length; i++) {
    el = node.form.elements[i];

    if (!el.type) 
      continue;

    if (el == node) {
      formData.push((isURLEncoded) ? escapeNameValuePair(el.name, "%s", true) :
                                     
                                     escapeNameValuePair(el.name, "", false) + "%s");
      continue;
    }

    type = el.type.toLowerCase();

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
  else
    spec += "?" + formData.join("&");

  PlacesUIUtils.showMinimalAddBookmarkUI(makeURI(spec), title, description, null,
                                         null, null, "", postData, charset);
}

function SwitchDocumentDirection(aWindow) {
  aWindow.document.dir = (aWindow.document.dir == "ltr" ? "rtl" : "ltr");
  for (var run = 0; run < aWindow.frames.length; run++)
    SwitchDocumentDirection(aWindow.frames[run]);
}

function getPluginInfo(pluginElement)
{
  var tagMimetype;
  var pluginsPage;
  if (pluginElement instanceof HTMLAppletElement) {
    tagMimetype = "application/x-java-vm";
  } else {
    if (pluginElement instanceof HTMLObjectElement) {
      pluginsPage = pluginElement.getAttribute("codebase");
    } else {
      pluginsPage = pluginElement.getAttribute("pluginspage");
    }

    
    if (pluginsPage) {
      var doc = pluginElement.ownerDocument;
      var docShell = findChildShell(doc, gBrowser.docShell, null);
      try {
        pluginsPage = makeURI(pluginsPage, doc.characterSet, docShell.currentURI).spec;
      } catch (ex) {
        pluginsPage = "";
      }
    }

    tagMimetype = pluginElement.QueryInterface(Components.interfaces.nsIObjectLoadingContent)
                               .actualType;

    if (tagMimetype == "") {
      tagMimetype = pluginElement.type;
    }
  }

  return {mimetype: tagMimetype, pluginsPage: pluginsPage};
}

var gPluginHandler = {

  get CrashSubmit() {
    delete this.CrashSubmit;
    Cu.import("resource://gre/modules/CrashSubmit.jsm", this);
    return this.CrashSubmit;
  },

  get crashReportHelpURL() {
    delete this.crashReportHelpURL;
    let url = formatURL("app.support.baseURL", true);
    url += "plugin-crashed";
    this.crashReportHelpURL = url;
    return this.crashReportHelpURL;
  },

  
  makeNicePluginName : function (aName, aFilename) {
    if (aName == "Shockwave Flash")
      return "Adobe Flash";

    
    
    let newName = aName.replace(/\bplug-?in\b/i, "").replace(/[\s\d\.\-\_\(\)]+$/, "");
    return newName;
  },

  isTooSmall : function (plugin, overlay) {
    
    let pluginRect = plugin.getBoundingClientRect();
    
    
    let overflows = (overlay.scrollWidth > pluginRect.width) ||
                    (overlay.scrollHeight - 5 > pluginRect.height);
    return overflows;
  },

  addLinkClickCallback: function (linkNode, callbackName ) {
    
    let self = this;
    let callbackArgs = Array.prototype.slice.call(arguments).slice(2);
    linkNode.addEventListener("click",
                              function(evt) {
                                if (!evt.isTrusted)
                                  return;
                                evt.preventDefault();
                                if (callbackArgs.length == 0)
                                  callbackArgs = [ evt ];
                                (self[callbackName]).apply(self, callbackArgs);
                              },
                              true);

    linkNode.addEventListener("keydown",
                              function(evt) {
                                if (!evt.isTrusted)
                                  return;
                                if (evt.keyCode == evt.DOM_VK_RETURN) {
                                  evt.preventDefault();
                                  if (callbackArgs.length == 0)
                                    callbackArgs = [ evt ];
                                  evt.preventDefault();
                                  (self[callbackName]).apply(self, callbackArgs);
                                }
                              },
                              true);
  },

  handleEvent : function(event) {
    let self = gPluginHandler;
    let plugin = event.target;

    
    if (!(plugin instanceof Ci.nsIObjectLoadingContent))
      return;

    switch (event.type) {
      case "PluginCrashed":
        self.pluginInstanceCrashed(plugin, event);
        break;

      case "PluginNotFound":
        
        
        
        if (!(plugin instanceof HTMLObjectElement))
          self.addLinkClickCallback(plugin, "installSinglePlugin");
        
      case "PluginBlocklisted":
      case "PluginOutdated":
        let hideBarPrefName = event.type == "PluginOutdated" ?
                                "plugins.hide_infobar_for_outdated_plugin" :
                                "plugins.hide_infobar_for_missing_plugin";
        if (gPrefService.getBoolPref(hideBarPrefName))
          return;

        self.pluginUnavailable(plugin, event.type);
        break;

      case "PluginDisabled":
        self.addLinkClickCallback(plugin, "managePlugins");
        break;
    }
  },

  newPluginInstalled : function(event) {
    
    var browser = event.originalTarget;
    
    browser.missingPlugins = null;

    var notificationBox = gBrowser.getNotificationBox(browser);
    var notification = notificationBox.getNotificationWithValue("missing-plugins");
    if (notification)
      notificationBox.removeNotification(notification);

    
    browser.reload();
  },

  
  installSinglePlugin: function (aEvent) {
    var missingPluginsArray = {};

    var pluginInfo = getPluginInfo(aEvent.target);
    missingPluginsArray[pluginInfo.mimetype] = pluginInfo;

    openDialog("chrome://mozapps/content/plugins/pluginInstallerWizard.xul",
               "PFSWindow", "chrome,centerscreen,resizable=yes",
               {plugins: missingPluginsArray, browser: gBrowser.selectedBrowser});
  },

  
  managePlugins: function (aEvent) {
    BrowserOpenAddonsMgr("addons://list/plugin");
  },

  
  submitReport : function(pluginDumpID, browserDumpID) {
    
    
    this.CrashSubmit.submit(pluginDumpID, gBrowser, null, null);
    if (browserDumpID)
      this.CrashSubmit.submit(browserDumpID, gBrowser, null, null);
  },

  
  reloadPage: function (browser) {
    browser.reload();
  },

  
  openHelpPage: function () {
    openHelpLink("plugin-crashed", false);
  },


  
  pluginUnavailable: function (plugin, eventType) {
    let browser = gBrowser.getBrowserForDocument(plugin.ownerDocument
                                                       .defaultView.top.document);
    if (!browser.missingPlugins)
      browser.missingPlugins = {};

    var pluginInfo = getPluginInfo(plugin);
    browser.missingPlugins[pluginInfo.mimetype] = pluginInfo;

    var notificationBox = gBrowser.getNotificationBox(browser);

    
    
    let outdatedNotification = notificationBox.getNotificationWithValue("outdated-plugins");
    let blockedNotification  = notificationBox.getNotificationWithValue("blocked-plugins");
    let missingNotification  = notificationBox.getNotificationWithValue("missing-plugins");

    
    if (outdatedNotification)
      return;

    function showBlocklistInfo() {
      var url = formatURL("extensions.blocklist.detailsURL", true);
      gBrowser.loadOneTab(url, {inBackground: false});
      return true;
    }

    function showOutdatedPluginsInfo() {
      gPrefService.setBoolPref("plugins.update.notifyUser", false);
      var url = formatURL("plugins.update.url", true);
      gBrowser.loadOneTab(url, {inBackground: false});
      return true;
    }

    function showPluginsMissing() {
      
      var missingPluginsArray = gBrowser.selectedBrowser.missingPlugins;
      if (missingPluginsArray) {
        openDialog("chrome://mozapps/content/plugins/pluginInstallerWizard.xul",
                   "PFSWindow", "chrome,centerscreen,resizable=yes",
                   {plugins: missingPluginsArray, browser: gBrowser.selectedBrowser});
      }
    }

    let notifications = {
      PluginBlocklisted : {
                            barID   : "blocked-plugins",
                            iconURL : "chrome://mozapps/skin/plugins/notifyPluginBlocked.png",
                            message : gNavigatorBundle.getString("blockedpluginsMessage.title"),
                            buttons : [{
                                         label     : gNavigatorBundle.getString("blockedpluginsMessage.infoButton.label"),
                                         accessKey : gNavigatorBundle.getString("blockedpluginsMessage.infoButton.accesskey"),
                                         popup     : null,
                                         callback  : showBlocklistInfo
                                       },
                                       {
                                         label     : gNavigatorBundle.getString("blockedpluginsMessage.searchButton.label"),
                                         accessKey : gNavigatorBundle.getString("blockedpluginsMessage.searchButton.accesskey"),
                                         popup     : null,
                                         callback  : showOutdatedPluginsInfo
                                      }],
                          },
      PluginOutdated    : {
                            barID   : "outdated-plugins",
                            iconURL : "chrome://mozapps/skin/plugins/notifyPluginOutdated.png",
                            message : gNavigatorBundle.getString("outdatedpluginsMessage.title"),
                            buttons : [{
                                         label     : gNavigatorBundle.getString("outdatedpluginsMessage.updateButton.label"),
                                         accessKey : gNavigatorBundle.getString("outdatedpluginsMessage.updateButton.accesskey"),
                                         popup     : null,
                                         callback  : showOutdatedPluginsInfo
                                      }],
                          },
      PluginNotFound    : {
                            barID   : "missing-plugins",
                            iconURL : "chrome://mozapps/skin/plugins/notifyPluginGeneric.png",
                            message : gNavigatorBundle.getString("missingpluginsMessage.title"),
                            buttons : [{
                                         label     : gNavigatorBundle.getString("missingpluginsMessage.button.label"),
                                         accessKey : gNavigatorBundle.getString("missingpluginsMessage.button.accesskey"),
                                         popup     : null,
                                         callback  : showPluginsMissing
                                      }],
                          }
    };

    if (eventType == "PluginBlocklisted") {
      if (blockedNotification || missingNotification)
        return;
    }
    else if (eventType == "PluginOutdated") {
      
      if (blockedNotification)
        blockedNotification.close();
      if (missingNotification)
        missingNotification.close();
    }
    else if (eventType == "PluginNotFound") {
      if (missingNotification)
        return;

      
      if (blockedNotification)
        blockedNotification.close();
    }

    let notify = notifications[eventType];
    notificationBox.appendNotification(notify.message, notify.barID, notify.iconURL,
                                       notificationBox.PRIORITY_WARNING_MEDIUM,
                                       notify.buttons);
  },

  
  
  pluginCrashed : function(subject, topic, data) {
    let propertyBag = subject;
    if (!(propertyBag instanceof Ci.nsIPropertyBag2) ||
        !(propertyBag instanceof Ci.nsIWritablePropertyBag2))
     return;

#ifdef MOZ_CRASHREPORTER
    let pluginDumpID = propertyBag.getPropertyAsAString("pluginDumpID");
    let browserDumpID= propertyBag.getPropertyAsAString("browserDumpID");
    let shouldSubmit = gCrashReporter.submitReports;
    let doPrompt     = true; 

    
    if (pluginDumpID && shouldSubmit && !doPrompt) {
      this.submitReport(pluginDumpID, browserDumpID);
      
      propertyBag.setPropertyAsBool("submittedCrashReport", true);
    }
#endif
  },

  
  
  pluginInstanceCrashed: function (plugin, aEvent) {
    
    if (!(aEvent instanceof Ci.nsIDOMDataContainerEvent))
      return;

    let submittedReport = aEvent.getData("submittedCrashReport");
    let doPrompt        = true; 
    let submitReports   = true; 
    let pluginName      = aEvent.getData("pluginName");
    let pluginFilename  = aEvent.getData("pluginFilename");
    let pluginDumpID    = aEvent.getData("pluginDumpID");
    let browserDumpID   = aEvent.getData("browserDumpID");

    
    pluginName = this.makeNicePluginName(pluginName, pluginFilename);

    
    plugin.clientTop;

    let messageString = gNavigatorBundle.getFormattedString("crashedpluginsMessage.title", [pluginName]);

    
    
    
    let doc = plugin.ownerDocument;
    let overlay = doc.getAnonymousElementByAttribute(plugin, "class", "mainBox");

    
    
    
    overlay.removeAttribute("role");

    let statusDiv = doc.getAnonymousElementByAttribute(plugin, "class", "submitStatus");
#ifdef MOZ_CRASHREPORTER
    let status;

    
    if (submittedReport) { 
      status = "submitted";
    }
    else if (!submitReports && !doPrompt) {
      status = "noSubmit";
    }
    else { 
      status = "please";
      
      let pleaseLink = doc.getAnonymousElementByAttribute(
                            plugin, "class", "pleaseSubmitLink");
      this.addLinkClickCallback(pleaseLink, "submitReport",
                                pluginDumpID, browserDumpID);
    }

    
    
    if (!pluginDumpID) {
        status = "noReport";
    }

    statusDiv.setAttribute("status", status);

    let bottomLinks = doc.getAnonymousElementByAttribute(plugin, "class", "msg msgBottomLinks");
    bottomLinks.style.display = "block";
    let helpIcon = doc.getAnonymousElementByAttribute(plugin, "class", "helpIcon");
    this.addLinkClickCallback(helpIcon, "openHelpPage");

    
    
    
    if (doPrompt) {
      let observer = {
        QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver,
                                               Ci.nsISupportsWeakReference]),
        observe : function(subject, topic, data) {
          let propertyBag = subject;
          if (!(propertyBag instanceof Ci.nsIPropertyBag2))
            return;
          
          if (propertyBag.get("minidumpID") != pluginDumpID)
            return;
          statusDiv.setAttribute("status", data);
        },

        handleEvent : function(event) {
            
        }
      }

      
      Services.obs.addObserver(observer, "crash-report-status", true);
      
      
      
      
      
      doc.addEventListener("mozCleverClosureHack", observer, false);
    }
#endif

    let crashText = doc.getAnonymousElementByAttribute(plugin, "class", "msg msgCrashed");
    crashText.textContent = messageString;

    let browser = gBrowser.getBrowserForDocument(doc.defaultView.top.document);

    let link = doc.getAnonymousElementByAttribute(plugin, "class", "reloadLink");
    this.addLinkClickCallback(link, "reloadPage", browser);

    let notificationBox = gBrowser.getNotificationBox(browser);

    
    if (this.isTooSmall(plugin, overlay)) {
        
        
        overlay.style.visibility = "hidden";
        
        
        if (!doc.mozNoPluginCrashedNotification)
          showNotificationBar(pluginDumpID, browserDumpID);
    } else {
        
        
        
        hideNotificationBar();
        doc.mozNoPluginCrashedNotification = true;
    }

    function hideNotificationBar() {
      let notification = notificationBox.getNotificationWithValue("plugin-crashed");
      if (notification)
        notificationBox.removeNotification(notification, true);
    }

    function showNotificationBar(pluginDumpID, browserDumpID) {
      
      let notification = notificationBox.getNotificationWithValue("plugin-crashed");
      if (notification)
        return;

      
      let priority = notificationBox.PRIORITY_WARNING_MEDIUM;
      let iconURL = "chrome://mozapps/skin/plugins/notifyPluginCrashed.png";
      let reloadLabel = gNavigatorBundle.getString("crashedpluginsMessage.reloadButton.label");
      let reloadKey   = gNavigatorBundle.getString("crashedpluginsMessage.reloadButton.accesskey");
      let submitLabel = gNavigatorBundle.getString("crashedpluginsMessage.submitButton.label");
      let submitKey   = gNavigatorBundle.getString("crashedpluginsMessage.submitButton.accesskey");

      let buttons = [{
        label: reloadLabel,
        accessKey: reloadKey,
        popup: null,
        callback: function() { browser.reload(); },
      }];
#ifdef MOZ_CRASHREPORTER
      let submitButton = {
        label: submitLabel,
        accessKey: submitKey,
        popup: null,
          callback: function() { gPluginHandler.submitReport(pluginDumpID, browserDumpID); },
      };
      if (pluginDumpID)
        buttons.push(submitButton);
#endif

      let notification = notificationBox.appendNotification(messageString, "plugin-crashed",
                                                            iconURL, priority, buttons);

      
      let XULNS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
      let link = notification.ownerDocument.createElementNS(XULNS, "label");
      link.className = "text-link";
      link.setAttribute("value", gNavigatorBundle.getString("crashedpluginsMessage.learnMore"));
      link.href = gPluginHandler.crashReportHelpURL;
      let description = notification.ownerDocument.getAnonymousElementByAttribute(notification, "anonid", "messageText");
      description.appendChild(link);

      
      doc.defaultView.top.addEventListener("unload", function() {
        notificationBox.removeNotification(notification);
      }, false);
    }

  }
};

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





var FeedHandler = {
  





  onFeedButtonClick: function(event) {
    event.stopPropagation();

    if (event.target.hasAttribute("feed") &&
        event.eventPhase == Event.AT_TARGET &&
        (event.button == 0 || event.button == 1)) {
        this.subscribeToFeed(null, event);
    }
  },

  









  buildFeedList: function(menuPopup) {
    var feeds = gBrowser.selectedBrowser.feeds;
    if (feeds == null) {
      
      
      
      
      
      
      menuPopup.parentNode.removeAttribute("open");
      return false;
    }

    while (menuPopup.firstChild)
      menuPopup.removeChild(menuPopup.firstChild);

    if (feeds.length == 1) {
      var feedButton = document.getElementById("feed-button");
      if (feedButton)
        feedButton.setAttribute("feed", feeds[0].href);
      return false;
    }

    
    for (var i = 0; i < feeds.length; ++i) {
      var feedInfo = feeds[i];
      var menuItem = document.createElement("menuitem");
      var baseTitle = feedInfo.title || feedInfo.href;
      var labelStr = gNavigatorBundle.getFormattedString("feedShowFeedNew", [baseTitle]);
      menuItem.setAttribute("class", "feed-menuitem");
      menuItem.setAttribute("label", labelStr);
      menuItem.setAttribute("feed", feedInfo.href);
      menuItem.setAttribute("tooltiptext", feedInfo.href);
      menuItem.setAttribute("crop", "center");
      menuPopup.appendChild(menuItem);
    }
    return true;
  },

  












  subscribeToFeed: function(href, event) {
    
    
    if (!href)
      href = event.target.getAttribute("feed");
    urlSecurityCheck(href, gBrowser.contentPrincipal,
                     Ci.nsIScriptSecurityManager.DISALLOW_INHERIT_PRINCIPAL);
    var feedURI = makeURI(href, document.characterSet);
    
    
    if (/^https?/.test(feedURI.scheme))
      href = "feed:" + href;
    this.loadFeed(href, event);
  },

  loadFeed: function(href, event) {
    var feeds = gBrowser.selectedBrowser.feeds;
    try {
      openUILink(href, event, false, true, false, null);
    }
    finally {
      
      
      gBrowser.selectedBrowser.feeds = feeds;
    }
  },

  



  updateFeeds: function() {
    var feedButton = document.getElementById("feed-button");
    if (!this._feedMenuitem)
      this._feedMenuitem = document.getElementById("subscribeToPageMenuitem");
    if (!this._feedMenupopup)
      this._feedMenupopup = document.getElementById("subscribeToPageMenupopup");

    var feeds = gBrowser.selectedBrowser.feeds;
    if (!feeds || feeds.length == 0) {
      if (feedButton) {
        feedButton.collapsed = true;
        feedButton.removeAttribute("feed");
      }
      this._feedMenuitem.setAttribute("disabled", "true");
      this._feedMenupopup.setAttribute("hidden", "true");
      this._feedMenuitem.removeAttribute("hidden");
    } else {
      if (feedButton)
        feedButton.collapsed = false;

      if (feeds.length > 1) {
        this._feedMenuitem.setAttribute("hidden", "true");
        this._feedMenupopup.removeAttribute("hidden");
        if (feedButton)
          feedButton.removeAttribute("feed");
      } else {
        if (feedButton)
          feedButton.setAttribute("feed", feeds[0].href);

        this._feedMenuitem.setAttribute("feed", feeds[0].href);
        this._feedMenuitem.removeAttribute("disabled");
        this._feedMenuitem.removeAttribute("hidden");
        this._feedMenupopup.setAttribute("hidden", "true");
      }
    }
  },

  addFeed: function(link, targetDoc) {
    
    var browserForLink = gBrowser.getBrowserForDocument(targetDoc);
    if (!browserForLink) {
      
      return;
    }

    if (!browserForLink.feeds)
      browserForLink.feeds = [];

    browserForLink.feeds.push({ href: link.href, title: link.title });

    if (browserForLink == gBrowser.selectedBrowser) {
      var feedButton = document.getElementById("feed-button");
      if (feedButton)
        feedButton.collapsed = false;
    }
  }
};







function undoCloseTab(aIndex) {
  
  var blankTabToRemove = null;
  if (gBrowser.tabs.length == 1 &&
      !gPrefService.getBoolPref("browser.tabs.autoHide") &&
      isTabEmpty(gBrowser.selectedTab))
    blankTabToRemove = gBrowser.selectedTab;

  var tab = null;
  var ss = Cc["@mozilla.org/browser/sessionstore;1"].
           getService(Ci.nsISessionStore);
  if (ss.getClosedTabCount(window) > (aIndex || 0)) {
    tab = ss.undoCloseTab(window, aIndex || 0);

    if (blankTabToRemove)
      gBrowser.removeTab(blankTabToRemove);
  }

  return tab;
}







function undoCloseWindow(aIndex) {
  let ss = Cc["@mozilla.org/browser/sessionstore;1"].
           getService(Ci.nsISessionStore);
  let window = null;
  if (ss.getClosedWindowCount() > (aIndex || 0))
    window = ss.undoCloseWindow(aIndex || 0);

  return window;
}





function isTabEmpty(aTab) {
  let browser = aTab.linkedBrowser;
  return browser.sessionHistory.count < 2 &&
         browser.currentURI.spec == "about:blank" &&
         !browser.contentDocument.body.hasChildNodes() &&
         !aTab.hasAttribute("busy");
}









function formatURL(aFormat, aIsPref) {
  var formatter = Cc["@mozilla.org/toolkit/URLFormatterService;1"].getService(Ci.nsIURLFormatter);
  return aIsPref ? formatter.formatURLPref(aFormat) : formatter.formatURL(aFormat);
}





var gBookmarkAllTabsHandler = {
  init: function () {
    this._command = document.getElementById("Browser:BookmarkAllTabs");
    gBrowser.tabContainer.addEventListener("TabOpen", this, true);
    gBrowser.tabContainer.addEventListener("TabClose", this, true);
    this._updateCommandState();
  },

  _updateCommandState: function BATH__updateCommandState(aTabClose) {
    var numTabs = gBrowser.tabs.length;

    
    if (aTabClose)
      numTabs--;

    if (numTabs > 1)
      this._command.removeAttribute("disabled");
    else
      this._command.setAttribute("disabled", "true");
  },

  doCommand: function BATH_doCommand() {
    PlacesCommandHook.bookmarkCurrentPages();
  },

  
  handleEvent: function(aEvent) {
    this._updateCommandState(aEvent.type == "TabClose");
  }
};




var gIdentityHandler = {
  
  IDENTITY_MODE_IDENTIFIED       : "verifiedIdentity", 
  IDENTITY_MODE_DOMAIN_VERIFIED  : "verifiedDomain",   
  IDENTITY_MODE_UNKNOWN          : "unknownIdentity",  
  IDENTITY_MODE_MIXED_CONTENT    : "unknownIdentity mixedContent",  

  
  _lastStatus : null,
  _lastLocation : null,

  
  get _encryptionLabel () {
    delete this._encryptionLabel;
    this._encryptionLabel = {};
    this._encryptionLabel[this.IDENTITY_MODE_DOMAIN_VERIFIED] =
      gNavigatorBundle.getString("identity.encrypted");
    this._encryptionLabel[this.IDENTITY_MODE_IDENTIFIED] =
      gNavigatorBundle.getString("identity.encrypted");
    this._encryptionLabel[this.IDENTITY_MODE_UNKNOWN] =
      gNavigatorBundle.getString("identity.unencrypted");
    this._encryptionLabel[this.IDENTITY_MODE_MIXED_CONTENT] =
      gNavigatorBundle.getString("identity.mixed_content");
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

  



  _cacheElements : function() {
    delete this._identityBox;
    delete this._identityIconLabel;
    delete this._identityIconCountryLabel;
    this._identityBox = document.getElementById("identity-box");
    this._identityIconLabel = document.getElementById("identity-icon-label");
    this._identityIconCountryLabel = document.getElementById("identity-icon-country-label");
  },

  



  handleMoreInfoClick : function(event) {
    displaySecurityInfo();
    event.stopPropagation();
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

  








  checkIdentity : function(state, location) {
    var currentStatus = gBrowser.securityUI
                                .QueryInterface(Components.interfaces.nsISSLStatusProvider)
                                .SSLStatus;
    this._lastStatus = currentStatus;
    this._lastLocation = location;

    let nsIWebProgressListener = Ci.nsIWebProgressListener;
    if (state & nsIWebProgressListener.STATE_IDENTITY_EV_TOPLEVEL)
      this.setMode(this.IDENTITY_MODE_IDENTIFIED);
    else if (state & nsIWebProgressListener.STATE_SECURE_HIGH)
      this.setMode(this.IDENTITY_MODE_DOMAIN_VERIFIED);
    else if (state & nsIWebProgressListener.STATE_IS_BROKEN)
      this.setMode(this.IDENTITY_MODE_MIXED_CONTENT);
    else
      this.setMode(this.IDENTITY_MODE_UNKNOWN);
  },

  


  getEffectiveHost : function() {
    
    if (!this._eTLDService)
      this._eTLDService = Cc["@mozilla.org/network/effective-tld-service;1"]
                         .getService(Ci.nsIEffectiveTLDService);
    try {
      return this._eTLDService.getBaseDomainFromHost(this._lastLocation.hostname);
    } catch (e) {
      
      
      return this._lastLocation.hostname;
    }
  },

  



  setMode : function(newMode) {
    if (!this._identityBox) {
      
      
      return;
    }

    this._identityBox.className = newMode;
    this.setIdentityMessages(newMode);

    
    if (this._identityPopup.state == "open")
      this.setPopupMessages(newMode);
  },

  





  setIdentityMessages : function(newMode) {
    if (newMode == this.IDENTITY_MODE_DOMAIN_VERIFIED) {
      var iData = this.getIdentityData();

      
      
      
      
      
      
      var icon_label = "";
      var icon_country_label = "";
      var icon_labels_dir = "ltr";
      switch (gPrefService.getIntPref("browser.identity.ssl_domain_display")) {
        case 2 : 
          icon_label = this._lastLocation.hostname;
          break;
        case 1 : 
          icon_label = this.getEffectiveHost();
      }

      
      
      var lookupHost = this._lastLocation.host;
      if (lookupHost.indexOf(':') < 0)
        lookupHost += ":443";

      
      
      var tooltip = gNavigatorBundle.getFormattedString("identity.identified.verifier",
                                                        [iData.caOrg]);

      
      
      
      
      if (this._overrideService.hasMatchingOverride(this._lastLocation.hostname,
                                                    (this._lastLocation.port || 443),
                                                    iData.cert, {}, {}))
        tooltip = gNavigatorBundle.getString("identity.identified.verified_by_you");
    }
    else if (newMode == this.IDENTITY_MODE_IDENTIFIED) {
      
      iData = this.getIdentityData();
      tooltip = gNavigatorBundle.getFormattedString("identity.identified.verifier",
                                                    [iData.caOrg]);
      icon_label = iData.subjectOrg;
      if (iData.country)
        icon_country_label = "(" + iData.country + ")";
      
      
      
      
      
      
      icon_labels_dir = /^[\u0590-\u08ff\ufb1d-\ufdff\ufe70-\ufefc]/.test(icon_label) ?
                        "rtl" : "ltr";
    }
    else {
      tooltip = gNavigatorBundle.getString("identity.unknown.tooltip");
      icon_label = "";
      icon_country_label = "";
      icon_labels_dir = "ltr";
    }

    
    this._identityBox.tooltipText = tooltip;
    this._identityIconLabel.value = icon_label;
    this._identityIconCountryLabel.value = icon_country_label;
    
    this._identityIconLabel.crop = icon_country_label ? "end" : "center";
    this._identityIconLabel.parentNode.style.direction = icon_labels_dir;
    
    this._identityIconLabel.parentNode.hidden = icon_label ? false : true;
  },

  






  setPopupMessages : function(newMode) {

    this._identityPopup.className = newMode;
    this._identityPopupContentBox.className = newMode;

    
    this._identityPopupEncLabel.textContent = this._encryptionLabel[newMode];

    
    var supplemental = "";
    var verifier = "";

    if (newMode == this.IDENTITY_MODE_DOMAIN_VERIFIED) {
      var iData = this.getIdentityData();
      var host = this.getEffectiveHost();
      var owner = gNavigatorBundle.getString("identity.ownerUnknown2");
      verifier = this._identityBox.tooltipText;
      supplemental = "";
    }
    else if (newMode == this.IDENTITY_MODE_IDENTIFIED) {
      
      iData = this.getIdentityData();
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
    }
    else {
      
      host = "";
      owner = "";
    }

    
    this._identityPopupContentHost.textContent = host;
    this._identityPopupContentOwner.textContent = owner;
    this._identityPopupContentSupp.textContent = supplemental;
    this._identityPopupContentVerif.textContent = verifier;
  },

  hideIdentityPopup : function() {
    this._identityPopup.hidePopup();
  },

  


  handleIdentityButtonEvent : function(event) {

    event.stopPropagation();

    if ((event.type == "click" && event.button != 0) ||
        (event.type == "keypress" && event.charCode != KeyEvent.DOM_VK_SPACE &&
         event.keyCode != KeyEvent.DOM_VK_RETURN))
      return; 

    
    gURLBar.handleRevert();

    
    
    this._identityPopup.hidden = false;

    
    this._identityPopup.popupBoxObject
        .setConsumeRollupEvent(Ci.nsIPopupBoxObject.ROLLUP_CONSUME);

    
    this.setPopupMessages(this._identityBox.className);

    
    
    var position = (getComputedStyle(gNavToolbox, "").direction == "rtl") ? 'after_end' : 'after_start';

    
    this._identityBox.setAttribute("open", "true");
    var self = this;
    this._identityPopup.addEventListener("popuphidden", function (e) {
      e.currentTarget.removeEventListener("popuphidden", arguments.callee, false);
      self._identityBox.removeAttribute("open");
    }, false);

    
    this._identityPopup.openPopup(this._identityBox, position);
  }
};

let DownloadMonitorPanel = {
  
  

  _panel: null,
  _activeStr: null,
  _pausedStr: null,
  _lastTime: Infinity,
  _listening: false,

  get DownloadUtils() {
    delete this.DownloadUtils;
    Cu.import("resource://gre/modules/DownloadUtils.jsm", this);
    return this.DownloadUtils;
  },

  
  

  


  init: function DMP_init() {
    
    this._panel = document.getElementById("download-monitor");

    
    this._activeStr = gNavigatorBundle.getString("activeDownloads1");
    this._pausedStr = gNavigatorBundle.getString("pausedDownloads1");

    gDownloadMgr.addListener(this);
    this._listening = true;

    this.updateStatus();
  },

  uninit: function DMP_uninit() {
    if (this._listening)
      gDownloadMgr.removeListener(this);
  },

  inited: function DMP_inited() {
    return this._panel != null;
  },

  


  updateStatus: function DMP_updateStatus() {
    if (!this.inited())
      return;

    let numActive = gDownloadMgr.activeDownloadCount;

    
    if (numActive == 0) {
      this._panel.hidden = true;
      this._lastTime = Infinity;

      return;
    }

    
    let numPaused = 0;
    let maxTime = -Infinity;
    let dls = gDownloadMgr.activeDownloads;
    while (dls.hasMoreElements()) {
      let dl = dls.getNext().QueryInterface(Ci.nsIDownload);
      if (dl.state == gDownloadMgr.DOWNLOAD_DOWNLOADING) {
        
        if (dl.speed > 0 && dl.size > 0)
          maxTime = Math.max(maxTime, (dl.size - dl.amountTransferred) / dl.speed);
        else
          maxTime = -1;
      }
      else if (dl.state == gDownloadMgr.DOWNLOAD_PAUSED)
        numPaused++;
    }

    
    let timeLeft;
    [timeLeft, this._lastTime] =
      this.DownloadUtils.getTimeLeft(maxTime, this._lastTime);

    
    let numDls = numActive - numPaused;
    let status = this._activeStr;

    
    if (numDls == 0) {
      numDls = numPaused;
      status = this._pausedStr;
    }

    
    
    status = PluralForm.get(numDls, status);
    status = status.replace("#1", numDls);
    status = status.replace("#2", timeLeft);

    
    this._panel.label = status;
    this._panel.hidden = false;
  },

  
  

  


  onProgressChange: function() {
    this.updateStatus();
  },

  


  onDownloadStateChange: function() {
    this.updateStatus();
  },

  onStateChange: function(aWebProgress, aRequest, aStateFlags, aStatus, aDownload) {
  },

  onSecurityChange: function(aWebProgress, aRequest, aState, aDownload) {
  },

  
  

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIDownloadProgressListener]),
};

function getNotificationBox(aWindow) {
  var foundBrowser = gBrowser.getBrowserForDocument(aWindow.document);
  if (foundBrowser)
    return gBrowser.getNotificationBox(foundBrowser)
  return null;
};


function getBrowser() gBrowser;
function getNavToolbox() gNavToolbox;

let gPrivateBrowsingUI = {
  _privateBrowsingService: null,
  _searchBarValue: null,
  _findBarValue: null,

  init: function PBUI_init() {
    Services.obs.addObserver(this, "private-browsing", false);
    Services.obs.addObserver(this, "private-browsing-transition-complete", false);

    this._privateBrowsingService = Cc["@mozilla.org/privatebrowsing;1"].
                                   getService(Ci.nsIPrivateBrowsingService);

    if (this.privateBrowsingEnabled)
      this.onEnterPrivateBrowsing(true);
  },

  uninit: function PBUI_unint() {
    Services.obs.removeObserver(this, "private-browsing");
    Services.obs.removeObserver(this, "private-browsing-transition-complete");
  },

  get _disableUIOnToggle() {
    if (this._privateBrowsingService.autoStarted)
      return false;

    try {
      return !gPrefService.getBoolPref("browser.privatebrowsing.keep_current_session");
    }
    catch (e) {
      return true;
    }
  },

  observe: function PBUI_observe(aSubject, aTopic, aData) {
    if (aTopic == "private-browsing") {
      if (aData == "enter")
        this.onEnterPrivateBrowsing();
      else if (aData == "exit")
        this.onExitPrivateBrowsing();
    }
    else if (aTopic == "private-browsing-transition-complete") {
      if (this._disableUIOnToggle) {
        
        setTimeout(function() {
          document.getElementById("Tools:PrivateBrowsing")
                  .removeAttribute("disabled");
        }, 0);
      }
    }
  },

  _shouldEnter: function PBUI__shouldEnter() {
    try {
      
      
      if (gPrefService.getBoolPref("browser.privatebrowsing.dont_prompt_on_enter") ||
          gPrefService.getBoolPref("browser.privatebrowsing.keep_current_session"))
        return true;
    }
    catch (ex) { }

    var bundleService = Cc["@mozilla.org/intl/stringbundle;1"].
                        getService(Ci.nsIStringBundleService);
    var pbBundle = bundleService.createBundle("chrome://browser/locale/browser.properties");
    var brandBundle = bundleService.createBundle("chrome://branding/locale/brand.properties");

    var appName = brandBundle.GetStringFromName("brandShortName");
# On Mac, use the header as the title.
#ifdef XP_MACOSX
    var dialogTitle = pbBundle.GetStringFromName("privateBrowsingMessageHeader");
    var header = "";
#else
    var dialogTitle = pbBundle.GetStringFromName("privateBrowsingDialogTitle");
    var header = pbBundle.GetStringFromName("privateBrowsingMessageHeader") + "\n\n";
#endif
    var message = pbBundle.formatStringFromName("privateBrowsingMessage", [appName], 1);

    var ps = Services.prompt;

    var flags = ps.BUTTON_TITLE_IS_STRING * ps.BUTTON_POS_0 +
                ps.BUTTON_TITLE_IS_STRING * ps.BUTTON_POS_1 +
                ps.BUTTON_POS_0_DEFAULT;

    var neverAsk = {value:false};
    var button0Title = pbBundle.GetStringFromName("privateBrowsingYesTitle");
    var button1Title = pbBundle.GetStringFromName("privateBrowsingNoTitle");
    var neverAskText = pbBundle.GetStringFromName("privateBrowsingNeverAsk");

    var result;
    var choice = ps.confirmEx(null, dialogTitle, header + message,
                              flags, button0Title, button1Title, null,
                              neverAskText, neverAsk);

    switch (choice) {
    case 0: 
      result = true;
      if (neverAsk.value)
        gPrefService.setBoolPref("browser.privatebrowsing.dont_prompt_on_enter", true);
      break;
    case 1: 
      result = false;
      break;
    }

    return result;
  },

  onEnterPrivateBrowsing: function PBUI_onEnterPrivateBrowsing(aOnWindowOpen) {
    if (BrowserSearch.searchBar)
      this._searchBarValue = BrowserSearch.searchBar.textbox.value;

    if (gFindBarInitialized)
      this._findBarValue = gFindBar.getElement("findbar-textbox").value;

    this._setPBMenuTitle("stop");

    document.getElementById("menu_import").setAttribute("disabled", "true");

    
    
    document.getElementById("Tools:Sanitize").setAttribute("disabled", "true");

    if (this._privateBrowsingService.autoStarted) {
      
      document.getElementById("privateBrowsingItem")
              .setAttribute("disabled", "true");
      document.getElementById("Tools:PrivateBrowsing")
              .setAttribute("disabled", "true");
    }
    else if (window.location.href == getBrowserURL()) {
      
      let docElement = document.documentElement;
      docElement.setAttribute("title",
        docElement.getAttribute("title_privatebrowsing"));
      docElement.setAttribute("titlemodifier",
        docElement.getAttribute("titlemodifier_privatebrowsing"));
      docElement.setAttribute("browsingmode", "private");
      gBrowser.updateTitlebar();
    }

    setTimeout(function () {
      DownloadMonitorPanel.updateStatus();
    }, 0);

    if (!aOnWindowOpen && this._disableUIOnToggle)
      document.getElementById("Tools:PrivateBrowsing")
              .setAttribute("disabled", "true");
  },

  onExitPrivateBrowsing: function PBUI_onExitPrivateBrowsing() {
    if (BrowserSearch.searchBar) {
      let searchBox = BrowserSearch.searchBar.textbox;
      searchBox.reset();
      if (this._searchBarValue) {
        searchBox.value = this._searchBarValue;
        this._searchBarValue = null;
      }
    }

    if (gURLBar) {
      gURLBar.editor.transactionManager.clear();
    }

    document.getElementById("menu_import").removeAttribute("disabled");

    
    
    document.getElementById("Tools:Sanitize").removeAttribute("disabled");

    if (gFindBarInitialized) {
      let findbox = gFindBar.getElement("findbar-textbox");
      findbox.reset();
      if (this._findBarValue) {
        findbox.value = this._findBarValue;
        this._findBarValue = null;
      }
    }

    this._setPBMenuTitle("start");

    if (window.location.href == getBrowserURL()) {
      
      let docElement = document.documentElement;
      docElement.setAttribute("title",
        docElement.getAttribute("title_normal"));
      docElement.setAttribute("titlemodifier",
        docElement.getAttribute("titlemodifier_normal"));
      docElement.setAttribute("browsingmode", "normal");
    }

    
    document.getElementById("privateBrowsingItem")
            .removeAttribute("disabled");
    document.getElementById("Tools:PrivateBrowsing")
            .removeAttribute("disabled");

    gLastOpenDirectory.reset();

    setTimeout(function () {
      DownloadMonitorPanel.updateStatus();
    }, 0);

    if (this._disableUIOnToggle)
      document.getElementById("Tools:PrivateBrowsing")
              .setAttribute("disabled", "true");
  },

  _setPBMenuTitle: function PBUI__setPBMenuTitle(aMode) {
    let pbMenuItem = document.getElementById("privateBrowsingItem");
    pbMenuItem.setAttribute("label", pbMenuItem.getAttribute(aMode + "label"));
    pbMenuItem.setAttribute("accesskey", pbMenuItem.getAttribute(aMode + "accesskey"));
  },

  toggleMode: function PBUI_toggleMode() {
    
    if (!this.privateBrowsingEnabled)
      if (!this._shouldEnter())
        return;

    this._privateBrowsingService.privateBrowsingEnabled =
      !this.privateBrowsingEnabled;
  },

  get privateBrowsingEnabled() {
    return this._privateBrowsingService.privateBrowsingEnabled;
  }
};

var LightWeightThemeWebInstaller = {
  handleEvent: function (event) {
    switch (event.type) {
      case "InstallBrowserTheme":
      case "PreviewBrowserTheme":
      case "ResetBrowserThemePreview":
        
        if (event.target.ownerDocument.defaultView.top != content)
          return;
    }
    switch (event.type) {
      case "InstallBrowserTheme":
        this._installRequest(event);
        break;
      case "PreviewBrowserTheme":
        this._preview(event);
        break;
      case "ResetBrowserThemePreview":
        this._resetPreview(event);
        break;
      case "pagehide":
      case "TabSelect":
        this._resetPreview();
        break;
    }
  },

  get _manager () {
    var temp = {};
    Cu.import("resource://gre/modules/LightweightThemeManager.jsm", temp);
    delete this._manager;
    return this._manager = temp.LightweightThemeManager;
  },

  _installRequest: function (event) {
    var node = event.target;
    var data = this._getThemeFromNode(node);
    if (!data)
      return;

    if (this._isAllowed(node)) {
      this._install(data);
      return;
    }

    var allowButtonText =
      gNavigatorBundle.getString("lwthemeInstallRequest.allowButton");
    var allowButtonAccesskey =
      gNavigatorBundle.getString("lwthemeInstallRequest.allowButton.accesskey");
    var message =
      gNavigatorBundle.getFormattedString("lwthemeInstallRequest.message",
                                          [node.ownerDocument.location.host]);
    var buttons = [{
      label: allowButtonText,
      accessKey: allowButtonAccesskey,
      callback: function () {
        LightWeightThemeWebInstaller._install(data);
      }
    }];

    this._removePreviousNotifications();

    var notificationBox = gBrowser.getNotificationBox();
    var notificationBar =
      notificationBox.appendNotification(message, "lwtheme-install-request", "",
                                         notificationBox.PRIORITY_INFO_MEDIUM,
                                         buttons);
    notificationBar.persistence = 1;
  },

  _install: function (newTheme) {
    var previousTheme = this._manager.currentTheme;
    this._manager.currentTheme = newTheme;
    if (this._manager.currentTheme &&
        this._manager.currentTheme.id == newTheme.id)
      this._postInstallNotification(newTheme, previousTheme);
  },

  _postInstallNotification: function (newTheme, previousTheme) {
    function text(id) {
      return gNavigatorBundle.getString("lwthemePostInstallNotification." + id);
    }

    var buttons = [{
      label: text("undoButton"),
      accessKey: text("undoButton.accesskey"),
      callback: function () {
        LightWeightThemeWebInstaller._manager.forgetUsedTheme(newTheme.id);
        LightWeightThemeWebInstaller._manager.currentTheme = previousTheme;
      }
    }, {
      label: text("manageButton"),
      accessKey: text("manageButton.accesskey"),
      callback: function () {
        BrowserOpenAddonsMgr("addons://list/theme");
      }
    }];

    this._removePreviousNotifications();

    var notificationBox = gBrowser.getNotificationBox();
    var notificationBar =
      notificationBox.appendNotification(text("message"),
                                         "lwtheme-install-notification", "",
                                         notificationBox.PRIORITY_INFO_MEDIUM,
                                         buttons);
    notificationBar.persistence = 1;
    notificationBar.timeout = Date.now() + 20000; 
  },

  _removePreviousNotifications: function () {
    var box = gBrowser.getNotificationBox();

    ["lwtheme-install-request",
     "lwtheme-install-notification"].forEach(function (value) {
        var notification = box.getNotificationWithValue(value);
        if (notification)
          box.removeNotification(notification);
      });
  },

  _previewWindow: null,
  _preview: function (event) {
    if (!this._isAllowed(event.target))
      return;

    var data = this._getThemeFromNode(event.target);
    if (!data)
      return;

    this._resetPreview();

    this._previewWindow = event.target.ownerDocument.defaultView;
    this._previewWindow.addEventListener("pagehide", this, true);
    gBrowser.tabContainer.addEventListener("TabSelect", this, false);

    this._manager.previewTheme(data);
  },

  _resetPreview: function (event) {
    if (!this._previewWindow ||
        event && !this._isAllowed(event.target))
      return;

    this._previewWindow.removeEventListener("pagehide", this, true);
    this._previewWindow = null;
    gBrowser.tabContainer.removeEventListener("TabSelect", this, false);

    this._manager.resetPreview();
  },

  _isAllowed: function (node) {
    var pm = Services.perms;

    var uri = node.ownerDocument.documentURIObject;
    return pm.testPermission(uri, "install") == pm.ALLOW_ACTION;
  },

  _getThemeFromNode: function (node) {
    return this._manager.parseTheme(node.getAttribute("data-browsertheme"),
                                    node.baseURI);
  }
}














function switchToTabHavingURI(aURI, aOpenNew, aCallback) {
  function switchIfURIInWindow(aWindow) {
    if (!("gBrowser" in aWindow))
      return false;
    let browsers = aWindow.gBrowser.browsers;
    for (let i = 0; i < browsers.length; i++) {
      let browser = browsers[i];
      if (browser.currentURI.equals(aURI)) {
        gURLBar.handleRevert();
        
        let prevTab = gBrowser.selectedTab;
        
        aWindow.focus();
        aWindow.gBrowser.tabContainer.selectedIndex = i;
        if (aCallback)
          aCallback(browser);
        
        if (isTabEmpty(prevTab))
          gBrowser.removeTab(prevTab);
        return true;
      }
    }
    return false;
  }

  
  if (!(aURI instanceof Ci.nsIURI))
    aURI = makeURI(aURI);

  
  if (switchIfURIInWindow(window))
    return true;

  let winEnum = Services.wm.getEnumerator("navigator:browser");
  while (winEnum.hasMoreElements()) {
    let browserWin = winEnum.getNext();
    
    
    if (browserWin.closed || browserWin == window)
      continue;
    if (switchIfURIInWindow(browserWin))
      return true;
  }

  
  if (aOpenNew) {
    gBrowser.selectedTab = gBrowser.addTab(aURI.spec);
    if (aCallback) {
      let browser = gBrowser.selectedBrowser;
      browser.addEventListener("pageshow", function(event) {
        if (event.target.location.href != aURI.spec)
          return;
        browser.removeEventListener("pageshow", arguments.callee, true);
        aCallback(browser);
      }, true);
    }
    return true;
  }

  return false;
}

var TabContextMenu = {
  contextTab: null,
  updateContextMenu: function updateContextMenu(aPopupMenu) {
    this.contextTab = document.popupNode.localName == "tab" ?
                      document.popupNode : gBrowser.selectedTab;
    var disabled = gBrowser.tabs.length == 1;
    var menuItems = aPopupMenu.getElementsByAttribute("tbattr", "tabbrowser-multiple");
    for (var i = 0; i < menuItems.length; i++)
      menuItems[i].disabled = disabled;

    
    
    
    document.getElementById("context_undoCloseTab").hidden =
      Cc["@mozilla.org/browser/sessionstore;1"].
      getService(Ci.nsISessionStore).
      getClosedTabCount(window) == 0;
      
    
    document.getElementById("context_pinTab").hidden = this.contextTab.pinned;
    document.getElementById("context_unpinTab").hidden = !this.contextTab.pinned;
  }
};

XPCOMUtils.defineLazyGetter(this, "HUDConsoleUI", function () {
  Cu.import("resource://gre/modules/HUDService.jsm");
  try {
    return HUDService.consoleUI;
  }
  catch (ex) {
    Components.utils.reportError(ex);
  }
});

