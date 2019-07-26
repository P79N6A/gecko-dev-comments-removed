# -*- Mode: javascript; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http:

let Ci = Components.interfaces;
let Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/RecentWindow.jsm");

const nsIWebNavigation = Ci.nsIWebNavigation;

var gCharsetMenu = null;
var gLastBrowserCharset = null;
var gPrevCharset = null;
var gProxyFavIcon = null;
var gLastValidURLStr = "";
var gInPrintPreviewMode = false;
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
  findbar.id = "FindToolbar";

  let browserBottomBox = document.getElementById("browser-bottombox");
  browserBottomBox.insertBefore(findbar, browserBottomBox.firstChild);

  
  findbar.clientTop;
  findbar.browser = gBrowser;
  window.gFindBarInitialized = true;
  return findbar;
});

XPCOMUtils.defineLazyGetter(this, "gPrefService", function() {
  return Services.prefs;
});

__defineGetter__("AddonManager", function() {
  let tmp = {};
  Cu.import("resource://gre/modules/AddonManager.jsm", tmp);
  return this.AddonManager = tmp.AddonManager;
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

XPCOMUtils.defineLazyModuleGetter(this, "TelemetryStopwatch",
  "resource://gre/modules/TelemetryStopwatch.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "AboutHomeUtils",
  "resource:///modules/AboutHomeUtils.jsm");

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

XPCOMUtils.defineLazyGetter(this, "DebuggerUI", function() {
  let tmp = {};
  Cu.import("resource:///modules/devtools/DebuggerUI.jsm", tmp);
  return new tmp.DebuggerUI(window);
});

XPCOMUtils.defineLazyModuleGetter(this, "Social",
  "resource:///modules/Social.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "PageThumbs",
  "resource://gre/modules/PageThumbs.jsm");

#ifdef MOZ_SAFE_BROWSING
XPCOMUtils.defineLazyModuleGetter(this, "SafeBrowsing",
  "resource://gre/modules/SafeBrowsing.jsm");
#endif

XPCOMUtils.defineLazyModuleGetter(this, "gBrowserNewTabPreloader",
  "resource:///modules/BrowserNewTabPreloader.jsm", "BrowserNewTabPreloader");

XPCOMUtils.defineLazyModuleGetter(this, "PrivateBrowsingUtils",
  "resource://gre/modules/PrivateBrowsingUtils.jsm");

let gInitialPages = [
  "about:blank",
  "about:newtab",
  "about:home",
  "about:privatebrowsing",
  "about:sessionrestore"
];

#include browser-addons.js
#include browser-feeds.js
#include browser-fullScreen.js
#include browser-fullZoom.js
#include browser-places.js
#include browser-plugins.js
#include browser-safebrowsing.js
#include browser-social.js
#include browser-tabPreviews.js
#include browser-tabview.js
#include browser-thumbnails.js
#include browser-webrtcUI.js

#ifdef MOZ_DATA_REPORTING
#include browser-data-submission-info-bar.js
#endif

#ifdef MOZ_SERVICES_SYNC
#include browser-syncui.js
#endif

XPCOMUtils.defineLazyGetter(this, "Win7Features", function () {
#ifdef XP_WIN
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

XPCOMUtils.defineLazyGetter(this, "PageMenu", function() {
  let tmp = {};
  Cu.import("resource://gre/modules/PageMenu.jsm", tmp);
  return new tmp.PageMenu();
});





function pageShowEventHandlers(persisted) {
  charsetLoadListener();
  XULBrowserWindow.asyncUpdateUI();

  
  
  
  if (persisted)
    gPluginHandler.reshowClickToPlayNotification();
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

  
  
  var unifiedButton = document.getElementById("unified-back-forward-button");
  if (unifiedButton && !unifiedButton._clickHandlersAttached) {
    unifiedButton._clickHandlersAttached = true;

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

  var node = aDocShell.QueryInterface(Components.interfaces.nsIDocShellTreeNode);
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

    if (!gBrowser.pageReport) {
      
      if (gURLBar)
        this._reportButton.hidden = true;
      return;
    }

    if (gURLBar)
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
    var pageReports = gBrowser.pageReport;
    if (pageReports) {
      for (let pageReport of pageReports) {
        
        
        if (!pageReport.popupWindowURI)
          continue;
        var popupURIspec = pageReport.popupWindowURI.spec;

        
        
        
        
        
        if (popupURIspec == "" || popupURIspec == "about:blank" ||
            popupURIspec == uri.spec)
          continue;

        
        
        
        
        
        foundUsablePopupURI = true;

        var menuitem = document.createElement("menuitem");
        var label = gNavigatorBundle.getFormattedString("popupShowPopupPrefix",
                                                        [popupURIspec]);
        menuitem.setAttribute("label", label);
        menuitem.setAttribute("popupWindowURI", popupURIspec);
        menuitem.setAttribute("popupWindowFeatures", pageReport.popupWindowFeatures);
        menuitem.setAttribute("popupWindowName", pageReport.popupWindowName);
        menuitem.setAttribute("oncommand", "gPopupBlockerObserver.showBlockedPopup(event);");
        menuitem.requestingWindow = pageReport.requestingWindow;
        menuitem.requestingDocument = pageReport.requestingDocument;
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
    gPrefService.setBoolPref("privacy.popups.showBrowserMessage", !showMessage);
    gBrowser.getNotificationBox().removeCurrentNotification();
  }
};

const gFormSubmitObserver = {
  QueryInterface : XPCOMUtils.generateQI([Ci.nsIFormSubmitObserver]),

  panel: null,

  init: function()
  {
    this.panel = document.getElementById('invalid-form-popup');
  },

  notifyInvalidSubmit : function (aFormElement, aInvalidElements)
  {
    
    
    
    if (!aInvalidElements.length) {
      return;
    }

    
    if (gBrowser.contentDocument !=
        aFormElement.ownerDocument.defaultView.top.document) {
      return;
    }

    let element = aInvalidElements.queryElementAt(0, Ci.nsISupports);

    if (!(element instanceof HTMLInputElement ||
          element instanceof HTMLTextAreaElement ||
          element instanceof HTMLSelectElement ||
          element instanceof HTMLButtonElement)) {
      return;
    }

    this.panel.firstChild.textContent = element.validationMessage;

    element.focus();

    
    
    
    function blurHandler() {
      gFormSubmitObserver.panel.hidePopup();
    };
    function inputHandler(e) {
      if (e.originalTarget.validity.valid) {
        gFormSubmitObserver.panel.hidePopup();
      } else {
        
        
        if (gFormSubmitObserver.panel.firstChild.textContent !=
            e.originalTarget.validationMessage) {
          gFormSubmitObserver.panel.firstChild.textContent =
            e.originalTarget.validationMessage;
        }
      }
    };
    element.addEventListener("input", inputHandler, false);
    element.addEventListener("blur", blurHandler, false);

    
    this.panel.addEventListener("popuphiding", function onPopupHiding(aEvent) {
      aEvent.target.removeEventListener("popuphiding", onPopupHiding, false);
      element.removeEventListener("input", inputHandler, false);
      element.removeEventListener("blur", blurHandler, false);
    }, false);

    this.panel.hidden = false;

    
    
    let offset = 0;
    let position = "";

    if (element.tagName == 'INPUT' &&
        (element.type == 'radio' || element.type == 'checkbox')) {
      position = "bottomcenter topleft";
    } else {
      let win = element.ownerDocument.defaultView;
      let style = win.getComputedStyle(element, null);
      let utils = win.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
                     .getInterface(Components.interfaces.nsIDOMWindowUtils);

      if (style.direction == 'rtl') {
        offset = parseInt(style.paddingRight) + parseInt(style.borderRightWidth);
      } else {
        offset = parseInt(style.paddingLeft) + parseInt(style.borderLeftWidth);
      }

      offset = Math.round(offset * utils.fullZoom);

      position = "after_start";
    }

    this.panel.openPopup(element, position, offset, 0);
  }
};












let gGestureSupport = {
  _currentRotation: 0,
  _lastRotateDelta: 0,
  _rotateMomentumThreshold: .75,

  





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
        this.onSwipe(aEvent);
        break;
      case "MozMagnifyGestureStart":
        aEvent.preventDefault();
#ifdef XP_WIN
        this._setupGesture(aEvent, "pinch", def(25, 0), "out", "in");
#else
        this._setupGesture(aEvent, "pinch", def(150, 1), "out", "in");
#endif
        break;
      case "MozRotateGestureStart":
        aEvent.preventDefault();
        this._setupGesture(aEvent, "twist", def(25, 0), "right", "left");
        break;
      case "MozMagnifyGestureUpdate":
      case "MozRotateGestureUpdate":
        aEvent.preventDefault();
        this._doUpdate(aEvent);
        break;
      case "MozTapGesture":
        aEvent.preventDefault();
        this._doAction(aEvent, ["tap"]);
        break;
      case "MozRotateGesture":
        aEvent.preventDefault();
        this._doAction(aEvent, ["twist", "end"]);
        break;
      

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

    
    for (let subCombo of this._power(keyCombos)) {
      
      
      
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
                                    aEvent.metaKey, aEvent);
          node.dispatchEvent(cmdEvent);
        }
      } else {
        goDoCommand(command);
      }

      break;
    }
  },

  







  _doUpdate: function(aEvent) {},

  





  onSwipe: function GS_onSwipe(aEvent) {
    
    for (let dir of ["UP", "RIGHT", "DOWN", "LEFT"]) {
      if (aEvent.direction == aEvent["DIRECTION_" + dir]) {
        this._doAction(aEvent, ["swipe", dir.toLowerCase()]);
        break;
      }
    }
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

  





  rotate: function(aEvent) {
    if (!(content.document instanceof ImageDocument))
      return;

    let contentElement = content.document.body.firstElementChild;
    if (!contentElement)
      return;
    
    if (contentElement.classList.contains("completeRotation"))
      this._clearCompleteRotation();

    this.rotation = Math.round(this.rotation + aEvent.delta);
    contentElement.style.transform = "rotate(" + this.rotation + "deg)";
    this._lastRotateDelta = aEvent.delta;
  },

  


  rotateEnd: function() {
    if (!(content.document instanceof ImageDocument))
      return;

    let contentElement = content.document.body.firstElementChild;
    if (!contentElement)
      return;

    let transitionRotation = 0;

    
    
    
    if (this.rotation <= 45)
      transitionRotation = 0;
    else if (this.rotation > 45 && this.rotation <= 135)
      transitionRotation = 90;
    else if (this.rotation > 135 && this.rotation <= 225)
      transitionRotation = 180;
    else if (this.rotation > 225 && this.rotation <= 315)
      transitionRotation = 270;
    else
      transitionRotation = 360;

    
    
    if (this._lastRotateDelta > this._rotateMomentumThreshold &&
        this.rotation > transitionRotation)
      transitionRotation += 90;
    else if (this._lastRotateDelta < -1 * this._rotateMomentumThreshold &&
             this.rotation < transitionRotation)
      transitionRotation -= 90;

    
    if (transitionRotation != this.rotation) {
      contentElement.classList.add("completeRotation");
      contentElement.addEventListener("transitionend", this._clearCompleteRotation);
    }

    contentElement.style.transform = "rotate(" + transitionRotation + "deg)";
    this.rotation = transitionRotation;
  },

  


  get rotation() {
    return this._currentRotation;
  },

  






  set rotation(aVal) {
    this._currentRotation = aVal % 360;
    if (this._currentRotation < 0)
      this._currentRotation += 360;
    return this._currentRotation;
  },

  



  restoreRotationState: function() {
    if (!(content.document instanceof ImageDocument))
      return;

    let contentElement = content.document.body.firstElementChild;
    let transformValue = content.window.getComputedStyle(contentElement, null)
                                       .transform;

    if (transformValue == "none") {
      this.rotation = 0;
      return;
    }

    
    
    transformValue = transformValue.split("(")[1]
                                   .split(")")[0]
                                   .split(",");
    this.rotation = Math.round(Math.atan2(transformValue[1], transformValue[0]) *
                               (180 / Math.PI));
  },

  


  _clearCompleteRotation: function() {
    let contentElement = content.document &&
                         content.document instanceof ImageDocument &&
                         content.document.body &&
                         content.document.body.firstElementChild;
    if (!contentElement)
      return;
    contentElement.classList.remove("completeRotation");
    contentElement.removeEventListener("transitionend", this._clearCompleteRotation);
  },
};

var gBrowserInit = {
  onLoad: function() {
    
    
    
    
    
    
    
    
    
    if ("arguments" in window && window.arguments[0])
      var uriToLoad = window.arguments[0];

    var mustLoadSidebar = false;

    gBrowser.addEventListener("DOMUpdatePageReport", gPopupBlockerObserver, false);

    
    gBrowser.addEventListener("PluginBindingAttached", gPluginHandler, true, true);
    gBrowser.addEventListener("PluginScripted",        gPluginHandler, true);
    gBrowser.addEventListener("PluginCrashed",         gPluginHandler, true);
    gBrowser.addEventListener("PluginOutdated",        gPluginHandler, true);

    gBrowser.addEventListener("NewPluginInstalled", gPluginHandler.newPluginInstalled, true);

    Services.obs.addObserver(gPluginHandler.pluginCrashed, "plugin-crashed", false);

    window.addEventListener("AppCommand", HandleAppCommandEvent, true);

    messageManager.loadFrameScript("chrome://browser/content/content.js", true);

    
    
    gBrowser.init();
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
      if (window.arguments[1].startsWith("charset=")) {
        var arrayArgComponents = window.arguments[1].split("=");
        if (arrayArgComponents) {
          
          getMarkupDocumentViewer().defaultCharacterSet = arrayArgComponents[1];
        }
      }
    }

    
    
    
    
    
    gBrowser.webNavigation.sessionHistory = Cc["@mozilla.org/browser/shistory;1"].
                                            createInstance(Ci.nsISHistory);
    Services.obs.addObserver(gBrowser.browsers[0], "browser:purge-session-history", false);

    
    
    gBrowser.browsers[0].removeAttribute("disablehistory");

    
    try {
      gBrowser.docShell.QueryInterface(Ci.nsIDocShellHistory).useGlobalHistory = true;
    } catch(ex) {
      Cu.reportError("Places database may be locked: " + ex);
    }

#ifdef MOZ_E10S_COMPAT
    
#else
    
    gBrowser.addProgressListener(window.XULBrowserWindow);
    gBrowser.addTabsProgressListener(window.TabsProgressListener);
#endif

    
    gBrowser.addEventListener("DOMLinkAdded", DOMLinkHandler, false);

    
    gBrowser.addEventListener("MozApplicationManifest",
                              OfflineApps, false);

    
    gGestureSupport.init(true);

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

    if (!gShowPageResizers)
      document.getElementById("status-bar").setAttribute("hideresizer", "true");

    if (!window.toolbar.visible) {
      
      if (gURLBar) {
        gURLBar.setAttribute("readonly", "true");
        gURLBar.setAttribute("enablehistory", "false");
      }
      goSetCommandEnabled("cmd_newNavigatorTab", false);
    }

#ifdef MENUBAR_CAN_AUTOHIDE
    updateAppButtonDisplay();
#endif

    
    CombinedStopReload.init();
    TabsOnTop.init();
    BookmarksMenuButton.init();
    gPrivateBrowsingUI.init();
    TabsInTitlebar.init();
    retrieveToolbarIconsizesFromTheme();

    
    this._boundDelayedStartup = this._delayedStartup.bind(this, uriToLoad, mustLoadSidebar);
    window.addEventListener("MozAfterPaint", this._boundDelayedStartup);

    this._loadHandled = true;
  },

  _cancelDelayedStartup: function () {
    window.removeEventListener("MozAfterPaint", this._boundDelayedStartup);
    this._boundDelayedStartup = null;
  },

  _delayedStartup: function(uriToLoad, mustLoadSidebar) {
    let tmp = {};
    Cu.import("resource://gre/modules/TelemetryTimestamps.jsm", tmp);
    let TelemetryTimestamps = tmp.TelemetryTimestamps;
    TelemetryTimestamps.add("delayedStartupStarted");

    this._cancelDelayedStartup();

    var isLoadingBlank = isBlankPageURL(uriToLoad);

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
        
        

        
        gBrowser.stop();
        
        gBrowser.docShell;

        gBrowser.swapBrowsersAndCloseOther(gBrowser.selectedTab, uriToLoad);
      }
      else if (window.arguments.length >= 3) {
        loadURI(uriToLoad, window.arguments[2], window.arguments[3] || null,
                window.arguments[4] || false);
        window.focus();
      }
      
      
      else
        loadOneOrMoreURIs(uriToLoad);
    }

#ifdef MOZ_SAFE_BROWSING
    
    setTimeout(function() { SafeBrowsing.init(); }, 2000);
#endif

    Services.obs.addObserver(gSessionHistoryObserver, "browser:purge-session-history", false);
    Services.obs.addObserver(gXPInstallObserver, "addon-install-disabled", false);
    Services.obs.addObserver(gXPInstallObserver, "addon-install-started", false);
    Services.obs.addObserver(gXPInstallObserver, "addon-install-blocked", false);
    Services.obs.addObserver(gXPInstallObserver, "addon-install-failed", false);
    Services.obs.addObserver(gXPInstallObserver, "addon-install-complete", false);
    Services.obs.addObserver(gFormSubmitObserver, "invalidformsubmit", false);

    BrowserOffline.init();
    OfflineApps.init();
    IndexedDBPromptHelper.init();
    gFormSubmitObserver.init();
    SocialUI.init();
    AddonManager.addAddonListener(AddonsMgrListener);
    WebrtcIndicator.init();

    gBrowser.addEventListener("pageshow", function(event) {
      
      if (content && event.target == content.document)
        setTimeout(pageShowEventHandlers, 0, event.persisted);
    }, true);

    
    Services.logins;

    if (mustLoadSidebar) {
      let sidebar = document.getElementById("sidebar");
      let sidebarBox = document.getElementById("sidebar-box");
      sidebar.setAttribute("src", sidebarBox.getAttribute("src"));
    }

    UpdateUrlbarSearchSplitterState();

    if (!isLoadingBlank || !focusAndSelectUrlBar())
      gBrowser.selectedBrowser.focus();

    gNavToolbox.customizeDone = BrowserToolboxCustomizeDone;
    gNavToolbox.customizeChange = BrowserToolboxCustomizeChange;

    
    this._initializeSanitizer();

    
    gBrowser.tabContainer.updateVisibility();

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

    
    
    
    FullZoom.init();

#ifdef MOZ_E10S_COMPAT
    
#else
    let NP = {};
    Cu.import("resource:///modules/NetworkPrioritizer.jsm", NP);
    NP.trackBrowserWindow(window);
#endif

    
    let ss = Cc["@mozilla.org/browser/sessionstore;1"].getService(Ci.nsISessionStore);
    ss.init(window);

    
    if (ss.canRestoreLastSession &&
        !PrivateBrowsingUtils.isWindowPrivate(window))
      goSetCommandEnabled("Browser:RestoreLastSession", true);

    PlacesToolbarHelper.init();

    ctrlTab.readPref();
    gPrefService.addObserver(ctrlTab.prefName, ctrlTab, false);

    
    
    
    
    
    setTimeout(function() {
      Services.downloads;
      let DownloadTaskbarProgress =
        Cu.import("resource://gre/modules/DownloadTaskbarProgress.jsm", {}).DownloadTaskbarProgress;
      DownloadTaskbarProgress.onBrowserWindowLoad(window);
    }, 10000);

    
    
    
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

#ifdef MOZ_E10S_COMPAT
    
#else
    if (Win7Features)
      Win7Features.onOpenWindow();
#endif

   
    window.addEventListener("fullscreen", onFullScreen, true);

    
    
    window.addEventListener("MozEnteredDomFullscreen", onMozEnteredDomFullscreen, true);

    if (window.fullScreen)
      onFullScreen();
    if (document.mozFullScreen)
      onMozEnteredDomFullscreen();

#ifdef MOZ_SERVICES_SYNC
    
    gSyncUI.init();
#endif

#ifdef MOZ_DATA_REPORTING
    gDataNotificationInfoBar.init();
#endif

    gBrowserThumbnails.init();
    TabView.init();

    setUrlAndSearchBarWidthForConditionalForwardButton();
    window.addEventListener("resize", function resizeHandler(event) {
      if (event.target == window)
        setUrlAndSearchBarWidthForConditionalForwardButton();
    });

    
    let devToolbarEnabled = gPrefService.getBoolPref("devtools.toolbar.enabled");
    if (devToolbarEnabled) {
      let cmd = document.getElementById("Tools:DevToolbar");
      cmd.removeAttribute("disabled");
      cmd.removeAttribute("hidden");
      document.getElementById("Tools:DevToolbarFocus").removeAttribute("disabled");

      
      if (gPrefService.getBoolPref("devtools.toolbar.visible")) {
        DeveloperToolbar.show(false);
      }
    }

    
    let enabled = gPrefService.getBoolPref("devtools.chrome.enabled") &&
                  gPrefService.getBoolPref("devtools.debugger.chrome-enabled") &&
                  gPrefService.getBoolPref("devtools.debugger.remote-enabled");
    if (enabled) {
      let cmd = document.getElementById("Tools:ChromeDebugger");
      cmd.removeAttribute("disabled");
      cmd.removeAttribute("hidden");
    }

    
    
    let consoleEnabled = true || gPrefService.getBoolPref("devtools.errorconsole.enabled") ||
                         gPrefService.getBoolPref("devtools.chrome.enabled");
    if (consoleEnabled) {
      let cmd = document.getElementById("Tools:ErrorConsole");
      cmd.removeAttribute("disabled");
      cmd.removeAttribute("hidden");
    }

    
    let scratchpadEnabled = gPrefService.getBoolPref(Scratchpad.prefEnabledName);
    if (scratchpadEnabled) {
      let cmd = document.getElementById("Tools:Scratchpad");
      cmd.removeAttribute("disabled");
      cmd.removeAttribute("hidden");
    }

    
    let devtoolsRemoteEnabled = gPrefService.getBoolPref("devtools.debugger.remote-enabled");
    if (devtoolsRemoteEnabled) {
      let cmd = document.getElementById("Tools:DevToolsConnect");
      cmd.removeAttribute("disabled");
      cmd.removeAttribute("hidden");
    }

#ifdef MENUBAR_CAN_AUTOHIDE
    
    
    
    if ("true" != gPrefService.getComplexValue("browser.menu.showCharacterEncoding",
                                               Ci.nsIPrefLocalizedString).data)
      document.getElementById("appmenu_charsetMenu").hidden = true;
#endif

    
    let responsiveUIEnabled = gPrefService.getBoolPref("devtools.responsiveUI.enabled");
    if (responsiveUIEnabled) {
      let cmd = document.getElementById("Tools:ResponsiveUI");
      cmd.removeAttribute("disabled");
      cmd.removeAttribute("hidden");
    }

    
    gDevToolsBrowser.registerBrowserWindow(window);

    let appMenuButton = document.getElementById("appmenu-button");
    let appMenuPopup = document.getElementById("appmenu-popup");
    if (appMenuButton && appMenuPopup) {
      let appMenuOpening = null;
      appMenuButton.addEventListener("mousedown", function(event) {
        if (event.button == 0)
          appMenuOpening = new Date();
      }, false);
      appMenuPopup.addEventListener("popupshown", function(event) {
        if (event.target != appMenuPopup || !appMenuOpening)
          return;
        let duration = new Date() - appMenuOpening;
        appMenuOpening = null;
        Services.telemetry.getHistogramById("FX_APP_MENU_OPEN_MS").add(duration);
      }, false);
    }

    window.addEventListener("mousemove", MousePosTracker, false);
    window.addEventListener("dragover", MousePosTracker, false);

    
    
    try {
      const startupCrashEndDelay = 30 * 1000;
      setTimeout(Services.startup.trackStartupCrashEnd, startupCrashEndDelay);
    } catch (ex) {
      Cu.reportError("Could not end startup crash tracking: " + ex);
    }

    Services.obs.notifyObservers(window, "browser-delayed-startup-finished", "");
    TelemetryTimestamps.add("delayedStartupFinished");
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

    FullScreen.cleanup();

    Services.obs.removeObserver(gPluginHandler.pluginCrashed, "plugin-crashed");

    try {
      gBrowser.removeProgressListener(window.XULBrowserWindow);
      gBrowser.removeTabsProgressListener(window.TabsProgressListener);
    } catch (ex) {
    }

    PlacesStarButton.uninit();

    TabsOnTop.uninit();

    TabsInTitlebar.uninit();

    var enumerator = Services.wm.getEnumerator(null);
    enumerator.getNext();
    if (!enumerator.hasMoreElements()) {
      document.persist("sidebar-box", "sidebarcommand");
      document.persist("sidebar-box", "width");
      document.persist("sidebar-box", "src");
      document.persist("sidebar-title", "value");
    }

    
    
    if (this._boundDelayedStartup) {
      this._cancelDelayedStartup();
    } else {
      if (Win7Features)
        Win7Features.onCloseWindow();

      gPrefService.removeObserver(ctrlTab.prefName, ctrlTab);
      ctrlTab.uninit();
      TabView.uninit();
      gBrowserThumbnails.uninit();
      FullZoom.destroy();

      Services.obs.removeObserver(gSessionHistoryObserver, "browser:purge-session-history");
      Services.obs.removeObserver(gXPInstallObserver, "addon-install-disabled");
      Services.obs.removeObserver(gXPInstallObserver, "addon-install-started");
      Services.obs.removeObserver(gXPInstallObserver, "addon-install-blocked");
      Services.obs.removeObserver(gXPInstallObserver, "addon-install-failed");
      Services.obs.removeObserver(gXPInstallObserver, "addon-install-complete");
      Services.obs.removeObserver(gFormSubmitObserver, "invalidformsubmit");

      try {
        gPrefService.removeObserver(gHomeButton.prefDomain, gHomeButton);
      } catch (ex) {
        Cu.reportError(ex);
      }

      BrowserOffline.uninit();
      OfflineApps.uninit();
      IndexedDBPromptHelper.uninit();
      AddonManager.removeAddonListener(AddonsMgrListener);
      SocialUI.uninit();
    }

    
    window.XULBrowserWindow.destroy();
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
                         'View:PageInfo', 'Browser:ToggleTabView', 'Browser:ToggleAddonBar'];
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

    SocialUI.nonBrowserWindowInit();

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
    toggleSidebar('viewBookmarksSidebar');
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
    PrintUtils.print();
    break;
  case "Save":
    saveDocument(window.content.document);
    break;
  case "SendMail":
    MailIntegration.sendLinkForWindow(window.content);
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
      
      win = window.openDialog("chrome://browser/content/", "_blank",
                              "chrome,all,dialog=no", BROWSER_NEW_TAB_URL);
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

function loadURI(uri, referrer, postData, allowThirdPartyFixup) {
  if (postData === undefined)
    postData = null;

  var flags = nsIWebNavigation.LOAD_FLAGS_NONE;
  if (allowThirdPartyFixup)
    flags |= nsIWebNavigation.LOAD_FLAGS_ALLOW_THIRD_PARTY_FIXUP;

  try {
    gBrowser.loadURIWithFlags(uri, flags, referrer, null, postData);
  } catch (e) {}
}

function getShortcutOrURI(aURL, aPostDataRef, aMayInheritPrincipal) {
  
  if (aMayInheritPrincipal)
    aMayInheritPrincipal.value = false;

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
    if (charset && charset != "UTF-8")
      encodedParam = escape(convertFromUnicode(charset, param)).
                     replace(/[+@\/]+/g, encodeURIComponent);
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

  
  
  if (aMayInheritPrincipal)
    aMayInheritPrincipal.value = true;

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
  
  
  
  
  
  try{
    var PageLoader = webNav.QueryInterface(Components.interfaces.nsIWebPageDescriptor);

    pageCookie = PageLoader.currentDescriptor;
  } catch(err) {
    
  }

  top.gViewSourceUtils.viewSource(webNav.currentURI.spec, pageCookie, aDocument);
}




function BrowserPageInfo(doc, initialTab, imageElement) {
  var args = {doc: doc, initialTab: initialTab, imageElement: imageElement};
  var windows = Services.wm.getEnumerator("Browser:page-info");

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

function URLBarSetURI(aURI) {
  var value = gBrowser.userTypedValue;
  var valid = false;

  if (value == null) {
    let uri = aURI || gBrowser.currentURI;
    
    try {
      uri = Services.uriFixup.createExposableURI(uri);
    } catch (e) {}

    
    
    if (gInitialPages.indexOf(uri.spec) != -1)
      value = content.opener ? uri.spec : "";
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
  
  if (!/%25(?:3B|2F|3F|3A|40|26|3D|2B|24|2C|23)/i.test(value))
    try {
      value = decodeURI(value)
                
                
                
                
                
                
                
                .replace(/%(?!3B|2F|3F|3A|40|26|3D|2B|24|2C|23)|[\r\n\t]/ig,
                         encodeURIComponent);
    } catch (e) {}

  
  
  value = value.replace(/[\v\x0c\x1c\x1d\x1e\x1f\u2028\u2029\ufffc]/g,
                        encodeURIComponent);

  
  
  
  
  value = value.replace(/[\u00ad\u034f\u115f-\u1160\u17b4-\u17b5\u180b-\u180d\u200b\u200e-\u200f\u202a-\u202e\u2060-\u206f\u3164\ufe00-\ufe0f\ufeff\uffa0\ufff0-\ufff8]|\ud834[\udd73-\udd7a]|[\udb40-\udb43][\udc00-\udfff]/g,
                        encodeURIComponent);
  return value;
}

function UpdateUrlbarSearchSplitterState()
{
  var splitter = document.getElementById("urlbar-search-splitter");
  var urlbar = document.getElementById("urlbar-container");
  var searchbar = document.getElementById("search-container");
  var stop = document.getElementById("stop-button");

  var ibefore = null;
  if (urlbar && searchbar) {
    if (urlbar.nextSibling == searchbar ||
        urlbar.getAttribute("combined") &&
        stop && stop.nextSibling == searchbar)
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
      splitter.className = "chromeclass-toolbar-additional";
    }
    urlbar.parentNode.insertBefore(splitter, ibefore);
  } else if (splitter)
    splitter.parentNode.removeChild(splitter);
}

function setUrlAndSearchBarWidthForConditionalForwardButton() {
  
  
  var urlbarContainer = document.getElementById("urlbar-container");
  var searchbarContainer = document.getElementById("search-container");
  if (!urlbarContainer ||
      !searchbarContainer ||
      urlbarContainer.hasAttribute("width") ||
      searchbarContainer.hasAttribute("width") ||
      urlbarContainer.parentNode != searchbarContainer.parentNode)
    return;
  urlbarContainer.style.width = searchbarContainer.style.width = "";
  var urlbarWidth = urlbarContainer.clientWidth;
  var searchbarWidth = searchbarContainer.clientWidth;
  urlbarContainer.style.width = urlbarWidth + "px";
  searchbarContainer.style.width = searchbarWidth + "px";
}

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
  } else if (aState == "invalid") {
    gURLBar.removeEventListener("input", UpdatePageProxyState, false);
  }
}

function PageProxyClickHandler(aEvent)
{
  if (aEvent.button == 1 && gPrefService.getBoolPref("middlemouse.paste"))
    middleMousePaste(aEvent);
}





function BrowserOnAboutPageLoad(doc) {
  if (doc.documentURI.toLowerCase() == "about:home") {
    
    
    if (getBoolPref("browser.aboutHome.apps", false))
      doc.getElementById("apps").removeAttribute("hidden");

    let ss = Components.classes["@mozilla.org/browser/sessionstore;1"].
             getService(Components.interfaces.nsISessionStore);
    if (ss.canRestoreLastSession &&
        !PrivateBrowsingUtils.isWindowPrivate(window))
      doc.getElementById("launcher").setAttribute("session", "true");

    
    let docElt = doc.documentElement;
    docElt.setAttribute("snippetsURL", AboutHomeUtils.snippetsURL);
    docElt.setAttribute("snippetsVersion", AboutHomeUtils.snippetsVersion);
    docElt.setAttribute("searchEngineName",
                        AboutHomeUtils.defaultSearchEngine.name);
    docElt.setAttribute("searchEngineURL",
                        AboutHomeUtils.defaultSearchEngine.searchURL);

#ifdef MOZ_SERVICES_HEALTHREPORT
    doc.addEventListener("AboutHomeSearchEvent", function onSearch(e) {
      BrowserSearch.recordSearchInHealthReport(e.detail, "abouthome");
    }, true, true);
#endif
  }
}




let BrowserOnClick = {
  handleEvent: function BrowserOnClick_handleEvent(aEvent) {
    if (!aEvent.isTrusted || 
        aEvent.button == 2 || aEvent.target.localName != "button") {
      return;
    }

    let originalTarget = aEvent.originalTarget;
    let ownerDoc = originalTarget.ownerDocument;

    
    
    if (ownerDoc.documentURI.startsWith("about:certerror")) {
      this.onAboutCertError(originalTarget, ownerDoc);
    }
    else if (ownerDoc.documentURI.startsWith("about:blocked")) {
      this.onAboutBlocked(originalTarget, ownerDoc);
    }
    else if (ownerDoc.documentURI.startsWith("about:neterror")) {
      this.onAboutNetError(originalTarget, ownerDoc);
    }
    else if (ownerDoc.documentURI.toLowerCase() == "about:home") {
      this.onAboutHome(originalTarget, ownerDoc);
    }
  },

  onAboutCertError: function BrowserOnClick_onAboutCertError(aTargetElm, aOwnerDoc) {
    let elmId = aTargetElm.getAttribute("id");
    let secHistogram = Services.telemetry.getHistogramById("SECURITY_UI");

    switch (elmId) {
      case "exceptionDialogButton":
        secHistogram.add(Ci.nsISecurityUITelemetry.WARNING_BAD_CERT_CLICK_ADD_EXCEPTION);
        let params = { exceptionAdded : false };

        try {
          switch (Services.prefs.getIntPref("browser.ssl_override_behavior")) {
            case 2 : 
              params.prefetchCert = true;
            case 1 : 
              params.location = aOwnerDoc.location.href;
          }
        } catch (e) {
          Components.utils.reportError("Couldn't get ssl_override pref: " + e);
        }

        window.openDialog('chrome://pippki/content/exceptionDialog.xul',
                          '','chrome,centerscreen,modal', params);

        
        if (params.exceptionAdded) {
          aOwnerDoc.location.reload();
        }
        break;

      case "getMeOutOfHereButton":
        secHistogram.add(Ci.nsISecurityUITelemetry.WARNING_BAD_CERT_GET_ME_OUT_OF_HERE);
        getMeOutOfHere();
        break;

      case "technicalContent":
        secHistogram.add(Ci.nsISecurityUITelemetry.WARNING_BAD_CERT_TECHNICAL_DETAILS);
        break;

      case "expertContent":
        secHistogram.add(Ci.nsISecurityUITelemetry.WARNING_BAD_CERT_UNDERSTAND_RISKS);
        break;

    }
  },

  onAboutBlocked: function BrowserOnClick_onAboutBlocked(aTargetElm, aOwnerDoc) {
    let elmId = aTargetElm.getAttribute("id");
    let secHistogram = Services.telemetry.getHistogramById("SECURITY_UI");

    
    
    
    let isMalware = /e=malwareBlocked/.test(aOwnerDoc.documentURI);
    let bucketName = isMalware ? "WARNING_MALWARE_PAGE_":"WARNING_PHISHING_PAGE_";
    let nsISecTel = Ci.nsISecurityUITelemetry;

    switch (elmId) {
      case "getMeOutButton":
        secHistogram.add(nsISecTel[bucketName + "GET_ME_OUT_OF_HERE"]);
        getMeOutOfHere();
        break;

      case "reportButton":
        
        
        

        
        
        secHistogram.add(nsISecTel[bucketName + "WHY_BLOCKED"]);

        if (isMalware) {
          
          
          try {
            let reportURL = formatURL("browser.safebrowsing.malware.reportURL", true);
            reportURL += aOwnerDoc.location.href;
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
        break;

      case "ignoreWarningButton":
        secHistogram.add(nsISecTel[bucketName + "IGNORE_WARNING"]);
        this.ignoreWarningButton(isMalware);
        break;
    }
  },

  ignoreWarningButton: function BrowserOnClick_ignoreWarningButton(aIsMalware) {
    
    
    
    gBrowser.loadURIWithFlags(content.location.href,
                              nsIWebNavigation.LOAD_FLAGS_BYPASS_CLASSIFIER,
                              null, null, null);

    Services.perms.add(makeURI(content.location.href), "safe-browsing",
                       Ci.nsIPermissionManager.ALLOW_ACTION,
                       Ci.nsIPermissionManager.EXPIRE_SESSION);

    let buttons = [{
      label: gNavigatorBundle.getString("safebrowsing.getMeOutOfHereButton.label"),
      accessKey: gNavigatorBundle.getString("safebrowsing.getMeOutOfHereButton.accessKey"),
      callback: function() { getMeOutOfHere(); }
    }];

    let title;
    if (aIsMalware) {
      title = gNavigatorBundle.getString("safebrowsing.reportedAttackSite");
      buttons[1] = {
        label: gNavigatorBundle.getString("safebrowsing.notAnAttackButton.label"),
        accessKey: gNavigatorBundle.getString("safebrowsing.notAnAttackButton.accessKey"),
        callback: function() {
          openUILinkIn(gSafeBrowsing.getReportURL('MalwareError'), 'tab');
        }
      };
    } else {
      title = gNavigatorBundle.getString("safebrowsing.reportedWebForgery");
      buttons[1] = {
        label: gNavigatorBundle.getString("safebrowsing.notAForgeryButton.label"),
        accessKey: gNavigatorBundle.getString("safebrowsing.notAForgeryButton.accessKey"),
        callback: function() {
          openUILinkIn(gSafeBrowsing.getReportURL('Error'), 'tab');
        }
      };
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

  onAboutNetError: function BrowserOnClick_onAboutNetError(aTargetElm, aOwnerDoc) {
    let elmId = aTargetElm.getAttribute("id");
    if (elmId != "errorTryAgain" || !/e=netOffline/.test(aOwnerDoc.documentURI))
      return;
    Services.io.offline = false;
  },

  onAboutHome: function BrowserOnClick_onAboutHome(aTargetElm, aOwnerDoc) {
    let elmId = aTargetElm.getAttribute("id");

    switch (elmId) {
      case "restorePreviousSession":
        let ss = Cc["@mozilla.org/browser/sessionstore;1"].
                 getService(Ci.nsISessionStore);
        if (ss.canRestoreLastSession) {
          ss.restoreLastSession();
        }
        aOwnerDoc.getElementById("launcher").removeAttribute("session");
        break;

      case "downloads":
        BrowserDownloadsUI();
        break;

      case "bookmarks":
        PlacesCommandHook.showPlacesOrganizer("AllBookmarks");
        break;

      case "history":
        PlacesCommandHook.showPlacesOrganizer("History");
        break;

      case "apps":
        openUILinkIn("https://marketplace.mozilla.org/", "tab");
        break;

      case "addons":
        BrowserOpenAddonsMgr();
        break;

      case "sync":
        openPreferences("paneSync");
        break;

      case "settings":
        openPreferences();
        break;
    }
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
  content.location = url;
}

function BrowserFullScreen()
{
  window.fullScreen = !window.fullScreen;
}

function onFullScreen(event) {
  FullScreen.toggle(event);
}

function onMozEnteredDomFullscreen(event) {
  FullScreen.enterDomFullscreen(event);
}

function getWebNavigation()
{
  return gBrowser.webNavigation;
}

function BrowserReloadWithFlags(reloadFlags) {
  





  var webNav = gBrowser.webNavigation;
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

    if (this._chromeState.sidebarOpen)
      toggleSidebar(this._sidebarCommand);

#ifdef MENUBAR_CAN_AUTOHIDE
    updateAppButtonDisplay();
#endif
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
    var addonBar = document.getElementById("addon-bar");
    this._chromeState.addonBarOpen = !addonBar.collapsed;
    addonBar.collapsed = true;
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

    if (this._chromeState.addonBarOpen) {
      document.getElementById("addon-bar").collapsed = false;
      gBrowser.updateWindowResizers();
    }

    if (this._chromeState.findOpen)
      gFindBar.open();

    if (this._chromeState.globalNotificationsOpen)
      document.getElementById("global-notificationbox").notificationsHidden = false;

    if (this._chromeState.syncNotificationsOpen)
      document.getElementById("sync-notifications").notificationsHidden = false;
  }
}

function getMarkupDocumentViewer()
{
  return gBrowser.markupDocumentViewer;
}














function FillInHTMLTooltip(tipElement)
{
  var retVal = false;
  
  if (!tipElement.ownerDocument ||
      (tipElement.ownerDocument.compareDocumentPosition(tipElement) & document.DOCUMENT_POSITION_DISCONNECTED))
    return retVal;

  const XLinkNS = "http://www.w3.org/1999/xlink";
  const XULNS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";

  var titleText = null;
  var XLinkTitleText = null;
  var SVGTitleText = null;
  var lookingForSVGTitle = true;
  var direction = tipElement.ownerDocument.dir;

  
  
  if ((tipElement instanceof HTMLInputElement ||
       tipElement instanceof HTMLTextAreaElement ||
       tipElement instanceof HTMLSelectElement ||
       tipElement instanceof HTMLButtonElement) &&
      !tipElement.hasAttribute('title') &&
      (!tipElement.form || !tipElement.form.noValidate)) {
    
    
    titleText = tipElement.validationMessage;
  }

  while (!titleText && !XLinkTitleText && !SVGTitleText && tipElement) {
    if (tipElement.nodeType == Node.ELEMENT_NODE &&
        tipElement.namespaceURI != XULNS) {
      titleText = tipElement.getAttribute("title");
      if ((tipElement instanceof HTMLAnchorElement ||
           tipElement instanceof HTMLAreaElement ||
           tipElement instanceof HTMLLinkElement ||
           tipElement instanceof SVGAElement) && tipElement.href) {
        XLinkTitleText = tipElement.getAttributeNS(XLinkNS, "title");
      }
      if (lookingForSVGTitle &&
          (!(tipElement instanceof SVGElement) ||
           tipElement.parentNode.nodeType == Node.DOCUMENT_NODE)) {
        lookingForSVGTitle = false;
      }
      if (lookingForSVGTitle) {
        for (let childNode of tipElement.childNodes) {
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
      
      t = t.replace(/\r\n?/g, '\n');

      tipNode.setAttribute("label", t);
      retVal = true;
    }
  });
  return retVal;
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

var bookmarksButtonObserver = {
  onDrop: function (aEvent)
  {
    let name = { };
    let url = browserDragAndDrop.drop(aEvent, name);
    try {
      PlacesUIUtils.showBookmarkDialog({ action: "add"
                                       , type: "bookmark"
                                       , uri: makeURI(url)
                                       , title: name
                                       , hiddenRows: [ "description"
                                                     , "location"
                                                     , "loadInSidebar"
                                                     , "keyword" ]
                                       }, window);
    } catch(ex) { }
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
    browserDragAndDrop.dragOver(aEvent);
  },
  onDragExit: function (aEvent)
  {
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

const DOMLinkHandler = {
  handleEvent: function (event) {
    switch (event.type) {
      case "DOMLinkAdded":
        this.onLinkAdded(event);
        break;
    }
  },
  getLinkIconURI: function(aLink) {
    let targetDoc = aLink.ownerDocument;
    var uri = makeURI(aLink.href, targetDoc.characterSet);

    
    
    
    var isAllowedPage = [
      /^about:neterror\?/,
      /^about:blocked\?/,
      /^about:certerror\?/,
      /^about:home$/,
    ].some(function (re) re.test(targetDoc.documentURI));

    if (!isAllowedPage || !uri.schemeIs("chrome")) {
      var ssm = Services.scriptSecurityManager;
      try {
        ssm.checkLoadURIWithPrincipal(targetDoc.nodePrincipal, uri,
                                      Ci.nsIScriptSecurityManager.DISALLOW_SCRIPT);
      } catch(e) {
        return null;
      }
    }

    try {
      var contentPolicy = Cc["@mozilla.org/layout/content-policy;1"].
                          getService(Ci.nsIContentPolicy);
    } catch(e) {
      return null; 
    }

    
    if (contentPolicy.shouldLoad(Ci.nsIContentPolicy.TYPE_IMAGE,
                                 uri, targetDoc.documentURIObject,
                                 aLink, aLink.type, null)
                                 != Ci.nsIContentPolicy.ACCEPT)
      return null;
    
    try {
      uri.userPass = "";
    } catch(e) {
      
    }
    return uri;
  },
  onLinkAdded: function (event) {
    var link = event.originalTarget;
    var rel = link.rel && link.rel.toLowerCase();
    if (!link || !link.ownerDocument || !rel || !link.href)
      return;

    var feedAdded = false;
    var iconAdded = false;
    var searchAdded = false;
    var rels = {};
    for (let relString of rel.split(/\s+/))
      rels[relString] = true;

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

            var uri = this.getLinkIconURI(link);
            if (!uri)
              break;

            if (gBrowser.isFailedIcon(uri))
              break;

            var browserIndex = gBrowser.getBrowserIndexForDocument(link.ownerDocument);
            
            if (browserIndex == -1)
              break;

            let tab = gBrowser.tabs[browserIndex];
            gBrowser.setIcon(tab, uri.spec);
            iconAdded = true;
          }
          break;
        case "search":
          if (!searchAdded) {
            var type = link.type && link.type.toLowerCase();
            type = type.replace(/^\s+|\s*(?:;.*)?$/g, "");

            if (type == "application/opensearchdescription+xml" && link.title &&
                /^(?:https?|ftp):/i.test(link.href) &&
                !PrivateBrowsingUtils.isWindowPrivate(window)) {
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
    else
      browser.engines = engines;
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
    var searchBar = this.searchBar;
    if (searchBar && window.fullScreen)
      FullScreen.mouseoverToggle(true);
    if (searchBar)
      searchBar.select();
    if (!searchBar || document.activeElement != searchBar.textbox.inputField)
      openUILinkIn(Services.search.defaultEngine.searchForm, "current");
  },

  


















  loadSearch: function BrowserSearch_search(searchText, useNewTab, purpose) {
    var engine;

    
    
    if (isElementVisible(this.searchBar))
      engine = Services.search.currentEngine;
    else
      engine = Services.search.defaultEngine;

    var submission = engine.getSubmission(searchText, null, purpose); 

    
    
    
    
    if (!submission) {
      return null;
    }

    let inBackground = Services.prefs.getBoolPref("browser.search.context.loadInBackground");
    openLinkIn(submission.uri.spec,
               useNewTab ? "tab" : "current",
               { postData: submission.postData,
                 inBackground: inBackground,
                 relatedToCurrent: true });

    return engine.name;
  },

  





  loadSearchFromContext: function (terms) {
    let engine = BrowserSearch.loadSearch(terms, true, "contextmenu");
    if (engine) {
      BrowserSearch.recordSearchInHealthReport(engine, "contextmenu");
    }
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

  











  recordSearchInHealthReport: function (engine, source) {
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

    item.setAttribute("uri", uri);
    item.setAttribute("label", entry.title || uri);
    item.setAttribute("index", j);

    if (j != index) {
      PlacesUtils.favicons.getFaviconURLForPage(entry.URI, function (aURI) {
        if (aURI) {
          let iconURL = PlacesUtils.favicons.getFaviconLinkForIcon(aURI).spec;
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

function OpenBrowserWindow(options)
{
  var telemetryObj = {};
  TelemetryStopwatch.start("FX_NEW_WINDOW_MS", telemetryObj);

  function newDocumentShown(doc, topic, data) {
    if (topic == "document-shown" &&
        doc != document &&
        doc.defaultView == win) {
      Services.obs.removeObserver(newDocumentShown, "document-shown");
      TelemetryStopwatch.finish("FX_NEW_WINDOW_MS", telemetryObj);
    }
  };
  Services.obs.addObserver(newDocumentShown, "document-shown", false);

  var charsetArg = new String();
  var handler = Components.classes["@mozilla.org/browser/clh;1"]
                          .getService(Components.interfaces.nsIBrowserHandler);
  var defaultArgs = handler.defaultArgs;
  var wintype = document.documentElement.getAttribute('windowtype');

  var extraFeatures = "";
  if (options && options.private) {
    extraFeatures = ",private";
    
    defaultArgs = "about:privatebrowsing";
  } else {
    extraFeatures = ",non-private";
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

var gCustomizeSheet = false;
function BrowserCustomizeToolbar() {
  
  var menubar = document.getElementById("main-menubar");
  for (let childNode of menubar.childNodes)
    childNode.setAttribute("disabled", true);

  var cmd = document.getElementById("cmd_CustomizeToolbars");
  cmd.setAttribute("disabled", "true");

  var splitter = document.getElementById("urlbar-search-splitter");
  if (splitter)
    splitter.parentNode.removeChild(splitter);

  CombinedStopReload.uninit();

  PlacesToolbarHelper.customizeStart();
  BookmarksMenuButton.customizeStart();
  DownloadsButton.customizeStart();

  TabsInTitlebar.allowedBy("customizing-toolbars", false);

  var customizeURL = "chrome://global/content/customizeToolbar.xul";
  gCustomizeSheet = getBoolPref("toolbar.customization.usesheet", false);

  if (gCustomizeSheet) {
    let sheetFrame = document.createElement("iframe");
    let panel = document.getElementById("customizeToolbarSheetPopup");
    sheetFrame.id = "customizeToolbarSheetIFrame";
    sheetFrame.toolbox = gNavToolbox;
    sheetFrame.panel = panel;
    sheetFrame.setAttribute("style", panel.getAttribute("sheetstyle"));
    panel.appendChild(sheetFrame);

    
    
    panel.style.visibility = "hidden";
    gNavToolbox.addEventListener("beforecustomization", function onBeforeCustomization() {
      gNavToolbox.removeEventListener("beforecustomization", onBeforeCustomization, false);
      panel.style.removeProperty("visibility");
    }, false);

    sheetFrame.setAttribute("src", customizeURL);

    panel.openPopup(gNavToolbox, "after_start", 0, 0);
  } else {
    window.openDialog(customizeURL,
                      "CustomizeToolbar",
                      "chrome,titlebar,toolbar,location,resizable,dependent",
                      gNavToolbox);
  }
}

function BrowserToolboxCustomizeDone(aToolboxChanged) {
  if (gCustomizeSheet) {
    document.getElementById("customizeToolbarSheetPopup").hidePopup();
    let iframe = document.getElementById("customizeToolbarSheetIFrame");
    iframe.parentNode.removeChild(iframe);
  }

  
  if (aToolboxChanged) {
    gURLBar = document.getElementById("urlbar");

    gProxyFavIcon = document.getElementById("page-proxy-favicon");
    gHomeButton.updateTooltip();
    gIdentityHandler._cacheElements();
    window.XULBrowserWindow.init();

#ifndef XP_MACOSX
    updateEditUIVisibility();
#endif

    
    
    
    if (!__lookupGetter__("PopupNotifications"))
      PopupNotifications.iconBox = document.getElementById("notification-popup-box");
  }

  PlacesToolbarHelper.customizeDone();
  BookmarksMenuButton.customizeDone();
  DownloadsButton.customizeDone();

  
  
  CombinedStopReload.init();
  UpdateUrlbarSearchSplitterState();
  setUrlAndSearchBarWidthForConditionalForwardButton();

  
  if (gURLBar) {
    URLBarSetURI();
    XULBrowserWindow.asyncUpdateUI();
    PlacesStarButton.updateState();
    SocialShareButton.updateShareState();
  }

  TabsInTitlebar.allowedBy("customizing-toolbars", true);

  
  var menubar = document.getElementById("main-menubar");
  for (let childNode of menubar.childNodes)
    childNode.setAttribute("disabled", false);
  var cmd = document.getElementById("cmd_CustomizeToolbars");
  cmd.removeAttribute("disabled");

  
  if (!getBoolPref("ui.click_hold_context_menus", false))
    SetClickAndHoldHandlers();

  gBrowser.selectedBrowser.focus();
}

function BrowserToolboxCustomizeChange(aType) {
  switch (aType) {
    case "iconsize":
    case "mode":
      retrieveToolbarIconsizesFromTheme();
      break;
    default:
      gHomeButton.updatePersonalToolbarStyle();
      BookmarksMenuButton.customizeChange();
  }
}




function retrieveToolbarIconsizesFromTheme() {
  function retrieveToolbarIconsize(aToolbar) {
    if (aToolbar.localName != "toolbar")
      return;

    
    
    
    
    let counterReset = getComputedStyle(aToolbar).counterReset;
    if (counterReset == "smallicons 0")
      aToolbar.setAttribute("iconsize", "small");
    else if (counterReset == "largeicons 0")
      aToolbar.setAttribute("iconsize", "large");
  }

  Array.forEach(gNavToolbox.childNodes, retrieveToolbarIconsize);
  gNavToolbox.externalToolbars.forEach(retrieveToolbarIconsize);
}





















function updateEditUIVisibility()
{
#ifndef XP_MACOSX
  let editMenuPopupState = document.getElementById("menu_EditPopup").state;
  let contextMenuPopupState = document.getElementById("contentAreaContextMenu").state;
  let placesContextMenuPopupState = document.getElementById("placesContext").state;
#ifdef MENUBAR_CAN_AUTOHIDE
  let appMenuPopupState = document.getElementById("appmenu-popup").state;
#endif

  
  
  
  gEditUIVisible = editMenuPopupState == "showing" ||
                   editMenuPopupState == "open" ||
                   contextMenuPopupState == "showing" ||
                   contextMenuPopupState == "open" ||
                   placesContextMenuPopupState == "showing" ||
                   placesContextMenuPopupState == "open" ||
#ifdef MENUBAR_CAN_AUTOHIDE
                   appMenuPopupState == "showing" ||
                   appMenuPopupState == "open" ||
#endif
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





function updateCharacterEncodingMenuState()
{
  let charsetMenu = document.getElementById("charsetMenu");
  let appCharsetMenu = document.getElementById("appmenu_charsetMenu");
  let appDevCharsetMenu =
    document.getElementById("appmenu_developer_charsetMenu");
  
  
  
  if (gBrowser &&
      gBrowser.docShell &&
      gBrowser.docShell.mayEnableCharacterEncodingMenu) {
    if (charsetMenu) {
      charsetMenu.removeAttribute("disabled");
    }
    if (appCharsetMenu) {
      appCharsetMenu.removeAttribute("disabled");
    }
    if (appDevCharsetMenu) {
      appDevCharsetMenu.removeAttribute("disabled");
    }
  } else {
    if (charsetMenu) {
      charsetMenu.setAttribute("disabled", "true");
    }
    if (appCharsetMenu) {
      appCharsetMenu.setAttribute("disabled", "true");
    }
    if (appDevCharsetMenu) {
      appDevCharsetMenu.setAttribute("disabled", "true");
    }
  }
}










function mimeTypeIsTextBased(aMimeType)
{
  return aMimeType.startsWith("text/") ||
         aMimeType.endsWith("+xml") ||
         aMimeType == "application/x-javascript" ||
         aMimeType == "application/javascript" ||
         aMimeType == "application/xml" ||
         aMimeType == "mozilla.application/cached-xul";
}

var XULBrowserWindow = {
  
  status: "",
  defaultStatus: "",
  overLink: "",
  startTime: 0,
  statusText: "",
  isBusy: false,
  inContentWhitelist: ["about:addons", "about:downloads", "about:permissions",
                       "about:sync-progress", "about:preferences"],

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
    delete this.statusTextField;
    return this.statusTextField = document.getElementById("statusbar-display");
  },
  get isImage () {
    delete this.isImage;
    return this.isImage = document.getElementById("isImage");
  },

  init: function () {
    this.throbberElement = document.getElementById("navigator-throbber");

#ifdef MOZ_E10S_COMPAT
    
#else
    
    
    var securityUI = gBrowser.securityUI;
    this._hostChanged = true;
    this.onSecurityChange(null, null, securityUI.state);
#endif
  },

  destroy: function () {
    
    delete this.throbberElement;
    delete this.stopCommand;
    delete this.reloadCommand;
    delete this.statusTextField;
    delete this.statusText;
  },

  setJSStatus: function () {
    
  },

  setJSDefaultStatus: function () {
    
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
    let target = this._onBeforeLinkTraversal(originalTarget, linkURI, linkNode, isAppTab);
    SocialUI.closeSocialPanelForLinkTraversal(target, linkNode);
    return target;
  },

  _onBeforeLinkTraversal: function(originalTarget, linkURI, linkNode, isAppTab) {
    
    
    if (originalTarget != "" || !isAppTab)
      return originalTarget;

    
    
    let linkHost;
    let docHost;
    try {
      linkHost = linkURI.host;
      docHost = linkNode.ownerDocument.documentURIObject.host;
    } catch(e) {
      
      
      return originalTarget;
    }

    if (docHost == linkHost)
      return originalTarget;

    
    let [longHost, shortHost] =
      linkHost.length > docHost.length ? [linkHost, docHost] : [docHost, linkHost];
    if (longHost == "www." + shortHost)
      return originalTarget;

    return "_blank";
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

    if (aStateFlags & nsIWebProgressListener.STATE_START &&
        aStateFlags & nsIWebProgressListener.STATE_IS_NETWORK) {

      if (aRequest && aWebProgress.DOMWindow == content)
        this.startDocumentLoad(aRequest);

      this.isBusy = true;

      if (!(aStateFlags & nsIWebProgressListener.STATE_RESTORING)) {
        this._busyUI = true;

        
        if (this.throbberElement)
          this.throbberElement.setAttribute("busy", "true");

        
        this.stopCommand.removeAttribute("disabled");
        CombinedStopReload.switchToStop();
      }
    }
    else if (aStateFlags & nsIWebProgressListener.STATE_STOP) {
      if (aStateFlags & nsIWebProgressListener.STATE_IS_NETWORK &&
          aWebProgress.DOMWindow == content &&
          aRequest)
        this.endDocumentLoad(aRequest, aStatus);

      
      
      
      if (aRequest) {
        let msg = "";
        let location;
        
        if (aRequest instanceof nsIChannel || "URI" in aRequest) {
          location = aRequest.URI;

          
          if (location.scheme == "keyword" && aWebProgress.DOMWindow == content)
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

        
        if (content.document && mimeTypeIsTextBased(content.document.contentType))
          this.isImage.removeAttribute('disabled');
        else
          this.isImage.setAttribute('disabled', 'true');
      }

      this.isBusy = false;

      if (this._busyUI) {
        this._busyUI = false;

        
        if (this.throbberElement)
          this.throbberElement.removeAttribute("busy");

        this.stopCommand.setAttribute("disabled", "true");
        CombinedStopReload.switchToReload(aRequest instanceof Ci.nsIRequest);
      }
    }
  },

  onLocationChange: function (aWebProgress, aRequest, aLocationURI, aFlags) {
    var location = aLocationURI ? aLocationURI.spec : "";
    this._hostChanged = true;

    
    if (gFormSubmitObserver.panel) {
      gFormSubmitObserver.panel.hidePopup();
    }

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
        newSpec = newSpec.substr(0, newIndexOfHash);
      if (newSpec != oldSpec) {
        
        
        let nBox = gBrowser.getNotificationBox(selectedBrowser);
        nBox.removeTransientNotifications();
      }
    }

    
    if (content.document && mimeTypeIsTextBased(content.document.contentType))
      this.isImage.removeAttribute('disabled');
    else
      this.isImage.setAttribute('disabled', 'true');

    this.hideOverLinkImmediately = true;
    this.setOverLink("", null);
    this.hideOverLinkImmediately = false;

    
    
    
    

    var browser = gBrowser.selectedBrowser;
    if (aWebProgress.DOMWindow == content) {
      if ((location == "about:blank" && !content.opener) ||
          location == "") {  
                             
        this.reloadCommand.setAttribute("disabled", "true");
      } else {
        this.reloadCommand.removeAttribute("disabled");
      }

      if (gURLBar) {
        URLBarSetURI(aLocationURI);

        
        PlacesStarButton.updateState();
        SocialShareButton.updateShareState();
      }

      
      if (this.hideChromeForLocation(location)) {
        document.documentElement.setAttribute("disablechrome", "true");
      } else {
        let ss = Cc["@mozilla.org/browser/sessionstore;1"].getService(Ci.nsISessionStore);
        if (ss.getTabValue(gBrowser.selectedTab, "appOrigin"))
          document.documentElement.setAttribute("disablechrome", "true");
        else
          document.documentElement.removeAttribute("disablechrome");
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

      
      if (aLocationURI &&
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

      if (gFindBarInitialized) {
        if (gFindBar.findMode != gFindBar.FIND_NORMAL) {
          
          gFindBar.close();
        }

        
        gFindBar.getElement("highlight").checked = false;
      }
    }
    UpdateBackForwardCommands(gBrowser.webNavigation);

    gGestureSupport.restoreRotationState();

    
    
    if (aRequest)
      setTimeout(function () { XULBrowserWindow.asyncUpdateUI(); }, 0);
    else
      this.asyncUpdateUI();
  },

  asyncUpdateUI: function () {
    FeedHandler.updateFeeds();
  },

  hideChromeForLocation: function(aLocation) {
    aLocation = aLocation.toLowerCase();
    return this.inContentWhitelist.some(function(aSpec) {
      return aSpec == aLocation;
    });
  },

  onStatusChange: function (aWebProgress, aRequest, aStatus, aMessage) {
    this.status = aMessage;
    this.updateStatusField();
  },

  
  _state: null,
  _hostChanged: false, 

  onSecurityChange: function (aWebProgress, aRequest, aState) {
    
    
    if (this._state == aState &&
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

    
    
    
    var location = gBrowser.contentWindow.location;
    var locationObj = {};
    try {
      
      locationObj.protocol = location == "about:blank" ? "http:" : location.protocol;
      locationObj.host = location.host;
      locationObj.hostname = location.hostname;
      locationObj.port = location.port;
    } catch (ex) {
      
      
      
    }
    gIdentityHandler.checkIdentity(this._state, locationObj);
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
  },

  startDocumentLoad: function XWB_startDocumentLoad(aRequest) {
    
    gBrowser.selectedBrowser.feeds = null;

    
    gBrowser.selectedBrowser.engines = null;

    var uri = aRequest.QueryInterface(Ci.nsIChannel).URI;
    try {
      Services.obs.notifyObservers(content, "StartDocumentLoad", uri.spec);
    } catch (e) {
    }
  },

  endDocumentLoad: function XWB_endDocumentLoad(aRequest, aStatus) {
    var urlStr = aRequest.QueryInterface(Ci.nsIChannel).originalURI.spec;

    var notification = Components.isSuccessCode(aStatus) ? "EndDocumentLoad" : "FailDocumentLoad";
    try {
      Services.obs.notifyObservers(content, notification, urlStr);
    } catch (e) {
    }
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

    var urlbar = document.getElementById("urlbar-container");
    var reload = document.getElementById("reload-button");
    var stop = document.getElementById("stop-button");

    if (urlbar) {
      if (urlbar.parentNode.getAttribute("mode") != "icons" ||
          !reload || urlbar.nextSibling != reload ||
          !stop || reload.nextSibling != stop)
        urlbar.removeAttribute("combined");
      else {
        urlbar.setAttribute("combined", "true");
        reload = document.getElementById("urlbar-reload-button");
        stop = document.getElementById("urlbar-stop-button");
      }
    }
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
#ifdef MOZ_CRASHREPORTER
    if (aRequest instanceof Ci.nsIChannel &&
        aStateFlags & Ci.nsIWebProgressListener.STATE_START &&
        aStateFlags & Ci.nsIWebProgressListener.STATE_IS_DOCUMENT &&
        gCrashReporter.enabled) {
      gCrashReporter.annotateCrashReport("URL", aRequest.URI.spec);
    }
#endif

    
    if (aWebProgress.DOMWindow == aWebProgress.DOMWindow.top) {
      if (aStateFlags & Ci.nsIWebProgressListener.STATE_IS_WINDOW) {
        if (aStateFlags & Ci.nsIWebProgressListener.STATE_START)
          TelemetryStopwatch.start("FX_PAGE_LOAD_MS", aBrowser);
        else if (aStateFlags & Ci.nsIWebProgressListener.STATE_STOP)
          TelemetryStopwatch.finish("FX_PAGE_LOAD_MS", aBrowser);
      } else if (aStateFlags & Ci.nsIWebProgressListener.STATE_STOP &&
                 aStatus == Cr.NS_BINDING_ABORTED) {
        TelemetryStopwatch.cancel("FX_PAGE_LOAD_MS", aBrowser);
      }
    }

    
    
    
    
    
    

    let doc = aWebProgress.DOMWindow.document;
    if (aStateFlags & Ci.nsIWebProgressListener.STATE_STOP &&
        Components.isSuccessCode(aStatus) &&
        doc.documentURI.startsWith("about:") &&
        !doc.documentURI.toLowerCase().startsWith("about:blank") &&
        !doc.documentElement.hasAttribute("hasBrowserHandlers")) {
      
      
      doc.documentElement.setAttribute("hasBrowserHandlers", "true");
      aBrowser.addEventListener("click", BrowserOnClick, true);
      aBrowser.addEventListener("pagehide", function onPageHide(event) {
        if (event.target.defaultView.frameElement)
          return;
        aBrowser.removeEventListener("click", BrowserOnClick, true);
        aBrowser.removeEventListener("pagehide", onPageHide, true);
      }, true);

      
      BrowserOnAboutPageLoad(doc);
    }
  },

  onLocationChange: function (aBrowser, aWebProgress, aRequest, aLocationURI,
                              aFlags) {
    
    
    if (aBrowser.contentWindow == aWebProgress.DOMWindow &&
        !(aFlags & Ci.nsIWebProgressListener.LOCATION_CHANGE_SAME_DOCUMENT)) {
      
      aBrowser._clickToPlayPluginsActivated = new Map();
      aBrowser._clickToPlayAllPluginsActivated = false;
      aBrowser._pluginScriptedState = gPluginHandler.PLUGIN_SCRIPTED_STATE_NONE;

      
      
      
      if (!Object.getOwnPropertyDescriptor(window, "PopupNotifications").get)
        PopupNotifications.locationChange(aBrowser);

      
      
      if (aBrowser != gBrowser.selectedBrowser)
        gBrowser.getNotificationBox(aBrowser).removeTransientNotifications();

      FullZoom.onLocationChange(aLocationURI, false, aBrowser);
    }
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

  openURI: function (aURI, aOpener, aWhere, aContext) {
    var newWindow = null;
    var isExternal = (aContext == Ci.nsIBrowserDOMWindow.OPEN_EXTERNAL);

    if (isExternal && aURI && aURI.schemeIs("chrome")) {
      dump("use -chrome command-line option to load external chrome urls\n");
      return null;
    }

    if (aWhere == Ci.nsIBrowserDOMWindow.OPEN_DEFAULTWINDOW) {
      if (isExternal &&
          gPrefService.prefHasUserValue("browser.link.open_newwindow.override.external"))
        aWhere = gPrefService.getIntPref("browser.link.open_newwindow.override.external");
      else
        aWhere = gPrefService.getIntPref("browser.link.open_newwindow");
    }
    switch (aWhere) {
      case Ci.nsIBrowserDOMWindow.OPEN_NEWWINDOW :
        
        
        var url = aURI ? aURI.spec : "about:blank";
        
        
        newWindow = openDialog(getBrowserURL(), "_blank", "all,dialog=no", url, null, null, null);
        break;
      case Ci.nsIBrowserDOMWindow.OPEN_NEWTAB :
        let win, needToFocusWin;

        
        if (window.toolbar.visible)
          win = window;
        else {
          let isPrivate = PrivateBrowsingUtils.isWindowPrivate(aOpener || window);
          win = RecentWindow.getMostRecentBrowserWindow({private: isPrivate});
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
          window.focus();
    }
    return newWindow;
  },

  isTabContentWindow: function (aWindow) {
    return gBrowser.browsers.some(function (browser) browser.contentWindow == aWindow);
  }
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

  let toolbarNodes = Array.slice(gNavToolbox.childNodes);
  toolbarNodes.push(document.getElementById("addon-bar"));

  for (let toolbar of toolbarNodes) {
    let toolbarName = toolbar.getAttribute("toolbarname");
    if (toolbarName) {
      let menuItem = document.createElement("menuitem");
      let hidingAttribute = toolbar.getAttribute("type") == "menubar" ?
                            "autohide" : "collapsed";
      menuItem.setAttribute("id", "toggle_" + toolbar.id);
      menuItem.setAttribute("toolbarId", toolbar.id);
      menuItem.setAttribute("type", "checkbox");
      menuItem.setAttribute("label", toolbarName);
      menuItem.setAttribute("checked", toolbar.getAttribute(hidingAttribute) != "true");
      if (popup.id != "appmenu_customizeMenu")
        menuItem.setAttribute("accesskey", toolbar.getAttribute("accesskey"));
      if (popup.id != "toolbar-context-menu")
        menuItem.setAttribute("key", toolbar.getAttribute("key"));

      popup.insertBefore(menuItem, firstMenuItem);

      menuItem.addEventListener("command", onViewToolbarCommand, false);
    }
  }
}

function onViewToolbarCommand(aEvent) {
  var toolbarId = aEvent.originalTarget.getAttribute("toolbarId");
  var toolbar = document.getElementById(toolbarId);
  var isVisible = aEvent.originalTarget.getAttribute("checked") == "true";
  setToolbarVisibility(toolbar, isVisible);
}

function setToolbarVisibility(toolbar, isVisible) {
  var hidingAttribute = toolbar.getAttribute("type") == "menubar" ?
                        "autohide" : "collapsed";

  toolbar.setAttribute(hidingAttribute, !isVisible);
  document.persist(toolbar.id, hidingAttribute);

  PlacesToolbarHelper.init();
  BookmarksMenuButton.updatePosition();
  gBrowser.updateWindowResizers();

#ifdef MENUBAR_CAN_AUTOHIDE
  updateAppButtonDisplay();
#endif
}

var TabsOnTop = {
  init: function TabsOnTop_init() {
    Services.prefs.addObserver(this._prefName, this, false);

    
    if (Services.prefs.getBoolPref(this._prefName)) {
      for (let item of document.querySelectorAll("menuitem[command=cmd_ToggleTabsOnTop]"))
        item.parentNode.removeChild(item);
    }
  },

  uninit: function TabsOnTop_uninit() {
    Services.prefs.removeObserver(this._prefName, this);
  },

  toggle: function () {
    this.enabled = !Services.prefs.getBoolPref(this._prefName);
  },

  syncUI: function () {
    let userEnabled = Services.prefs.getBoolPref(this._prefName);
    let enabled = userEnabled && gBrowser.tabContainer.visible;

    document.getElementById("cmd_ToggleTabsOnTop")
            .setAttribute("checked", userEnabled);

    document.documentElement.setAttribute("tabsontop", enabled);
    document.getElementById("navigator-toolbox").setAttribute("tabsontop", enabled);
    document.getElementById("TabsToolbar").setAttribute("tabsontop", enabled);
    document.getElementById("nav-bar").setAttribute("tabsontop", enabled);
    gBrowser.tabContainer.setAttribute("tabsontop", enabled);
    TabsInTitlebar.allowedBy("tabs-on-top", enabled);
  },

  get enabled () {
    return gNavToolbox.getAttribute("tabsontop") == "true";
  },

  set enabled (val) {
    Services.prefs.setBoolPref(this._prefName, !!val);
    return val;
  },

  observe: function (subject, topic, data) {
    if (topic == "nsPref:changed")
      this.syncUI();
  },

  _prefName: "browser.tabs.onTop"
}

var TabsInTitlebar = {
  init: function () {
#ifdef CAN_DRAW_IN_TITLEBAR
    this._readPref();
    Services.prefs.addObserver(this._prefName, this, false);

    
    
    this.allowedBy("sizemode", false);

    this._initialized = true;
#endif
  },

  allowedBy: function (condition, allow) {
#ifdef CAN_DRAW_IN_TITLEBAR
    if (allow) {
      if (condition in this._disallowed) {
        delete this._disallowed[condition];
        this._update();
      }
    } else {
      if (!(condition in this._disallowed)) {
        this._disallowed[condition] = null;
        this._update();
      }
    }
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

  _initialized: false,
  _disallowed: {},
  _prefName: "browser.tabs.drawInTitlebar",

  _readPref: function () {
    this.allowedBy("pref",
                   Services.prefs.getBoolPref(this._prefName));
  },

  _update: function () {
    if (!this._initialized || window.fullScreen)
      return;

    let allowed = true;
    for (let something in this._disallowed) {
      allowed = false;
      break;
    }

    if (allowed == this.enabled)
      return;

    function $(id) document.getElementById(id);
    let titlebar = $("titlebar");

    if (allowed) {
      function rect(ele)   ele.getBoundingClientRect();

      let tabsToolbar       = $("TabsToolbar");

#ifdef MENUBAR_CAN_AUTOHIDE
      let appmenuButtonBox  = $("appmenu-button-container");
      this._sizePlaceholder("appmenu-button", rect(appmenuButtonBox).width);
#endif
      let captionButtonsBox = $("titlebar-buttonbox");
      this._sizePlaceholder("caption-buttons", rect(captionButtonsBox).width);

      let tabsToolbarRect = rect(tabsToolbar);
      let titlebarTop = rect($("titlebar-content")).top;
      titlebar.style.marginBottom = - Math.min(tabsToolbarRect.top - titlebarTop,
                                               tabsToolbarRect.height) + "px";

      document.documentElement.setAttribute("tabsintitlebar", "true");

      if (!this._draghandle) {
        let tmp = {};
        Components.utils.import("resource://gre/modules/WindowDraggingUtils.jsm", tmp);
        this._draghandle = new tmp.WindowDraggingElement(tabsToolbar);
        this._draghandle.mouseDownCheck = function () {
          return !this._dragBindingAlive && TabsInTitlebar.enabled;
        };
      }
    } else {
      document.documentElement.removeAttribute("tabsintitlebar");

      titlebar.style.marginBottom = "";
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
#endif
  }
};

#ifdef MENUBAR_CAN_AUTOHIDE
function updateAppButtonDisplay() {
  var displayAppButton =
    !gInPrintPreviewMode &&
    window.menubar.visible &&
    document.getElementById("toolbar-menubar").getAttribute("autohide") == "true";

#ifdef CAN_DRAW_IN_TITLEBAR
  document.getElementById("titlebar").hidden = !displayAppButton;

  if (displayAppButton)
    document.documentElement.setAttribute("chromemargin", "0,2,2,2");
  else
    document.documentElement.removeAttribute("chromemargin");

  TabsInTitlebar.allowedBy("drawing-in-titlebar", displayAppButton);
#else
  document.getElementById("appmenu-toolbar-button").hidden =
    !displayAppButton;
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
      
      
      
      
      
      sidebar.setAttribute("src", "about:blank");
      sidebar.docShell.createAboutBlankContentViewer(null);

      sidebarBroadcaster.removeAttribute("checked");
      sidebarBox.setAttribute("sidebarcommand", "");
      sidebarTitle.value = "";
      sidebarBox.hidden = true;
      sidebarSplitter.hidden = true;
      gBrowser.selectedBrowser.focus();
    } else {
      fireSidebarFocusedEvent();
    }
    return;
  }

  

  
  var broadcasters = document.getElementsByAttribute("group", "sidebar");
  for (let broadcaster of broadcasters) {
    
    
    if (broadcaster.localName != "broadcaster")
      continue;

    if (broadcaster != sidebarBroadcaster)
      broadcaster.removeAttribute("checked");
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
  }
};










function getBrowserSelection(aCharLen) {
  
  const kMaxSelectionLen = 150;
  const charLen = Math.min(aCharLen || kMaxSelectionLen, kMaxSelectionLen);
  let commandDispatcher = document.commandDispatcher;

  var focusedWindow = commandDispatcher.focusedWindow;
  var selection = focusedWindow.getSelection().toString();
  
  if (!selection) {
    let element = commandDispatcher.focusedElement;
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

  urlSecurityCheck(href, doc.nodePrincipal);
  openLinkIn(href, where, { referrerURI: doc.documentURIObject,
                            charset: doc.characterSet });
  event.preventDefault();
  return true;
}

function middleMousePaste(event) {
  let clipboard = readFromClipboard();
  if (!clipboard)
    return;

  
  
  clipboard = clipboard.replace(/\s*\n\s*/g, "");

  let mayInheritPrincipal = { value: false };
  let url = getShortcutOrURI(clipboard, mayInheritPrincipal);
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

  openUILink(url, event,
             { ignoreButton: true,
               disallowInheritPrincipal: !mayInheritPrincipal.value });

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
        BrowserCharsetReload();
        SelectDetector(event, false);
    } else if (name == 'charsetGroup') {
        var charset = node.getAttribute('id');
        charset = charset.substring('charset.'.length, charset.length)
        BrowserSetForcedCharacterSet(charset);
    } else if (name == 'charsetCustomize') {
        
    } else {
        BrowserSetForcedCharacterSet(node.getAttribute('id'));
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

function BrowserSetForcedCharacterSet(aCharset)
{
  gBrowser.docShell.gatherCharsetMenuTelemetry();
  gBrowser.docShell.charset = aCharset;
  
  if (!PrivateBrowsingUtils.isWindowPrivate(window))
    PlacesUtils.history.setCharsetForURI(getWebNavigation().currentURI, aCharset);
  BrowserCharsetReload();
}

function BrowserCharsetReload()
{
  BrowserReloadWithFlags(nsIWebNavigation.LOAD_FLAGS_CHARSET_CHANGE);
}

function charsetMenuGetElement(parent, id) {
  return parent.getElementsByAttribute("id", id)[0];
}

function UpdateCurrentCharset(target) {
    
    var wnd = document.commandDispatcher.focusedWindow;
    if ((window == wnd) || (wnd == null)) wnd = window.content;

    
    if (gPrevCharset) {
        var pref_item = charsetMenuGetElement(target, "charset." + gPrevCharset);
        if (pref_item)
          pref_item.setAttribute('checked', 'false');
    }

    var menuitem = charsetMenuGetElement(target, "charset." + wnd.document.characterSet);
    if (menuitem) {
        menuitem.setAttribute('checked', 'true');
    }
}

function UpdateCharsetDetector(target) {
  var prefvalue;

  try {
    prefvalue = gPrefService.getComplexValue("intl.charset.detector", Ci.nsIPrefLocalizedString).data;
  }
  catch (ex) {}

  if (!prefvalue)
    prefvalue = "off";

  var menuitem = charsetMenuGetElement(target, "chardet." + prefvalue);
  if (menuitem)
    menuitem.setAttribute("checked", "true");
}

function UpdateMenus(event) {
  UpdateCurrentCharset(event.target);
  UpdateCharsetDetector(event.target);
}

function CreateMenu(node) {
  Services.obs.notifyObservers(null, "charsetmenu-selected", node);
}

function charsetLoadListener() {
  var charset = window.content.document.characterSet;

  if (charset.length > 0 && (charset != gLastBrowserCharset)) {
    if (!gCharsetMenu)
      gCharsetMenu = Cc['@mozilla.org/rdf/datasource;1?name=charset-menu'].getService(Ci.nsICurrentCharsetListener);
    gCharsetMenu.SetCurrentCharset(charset);
    gPrevCharset = gLastBrowserCharset;
    gLastBrowserCharset = charset;
  }
}


var gPageStyleMenu = {

  _getAllStyleSheets: function (frameset) {
    var styleSheetsArray = Array.slice(frameset.document.styleSheets);
    for (let i = 0; i < frameset.frames.length; i++) {
      let frameSheets = this._getAllStyleSheets(frameset.frames[i]);
      styleSheetsArray = styleSheetsArray.concat(frameSheets);
    }
    return styleSheetsArray;
  },

  fillPopup: function (menuPopup) {
    var noStyle = menuPopup.firstChild;
    var persistentOnly = noStyle.nextSibling;
    var sep = persistentOnly.nextSibling;
    while (sep.nextSibling)
      menuPopup.removeChild(sep.nextSibling);

    var styleSheets = this._getAllStyleSheets(window.content);
    var currentStyleSheets = {};
    var styleDisabled = getMarkupDocumentViewer().authorStyleDisabled;
    var haveAltSheets = false;
    var altStyleSelected = false;

    for (let currentStyleSheet of styleSheets) {
      if (!currentStyleSheet.title)
        continue;

      
      if (currentStyleSheet.media.length > 0) {
        let mediaQueryList = currentStyleSheet.media.mediaText;
        if (!window.content.matchMedia(mediaQueryList).matches)
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
        menuItem.setAttribute("oncommand", "gPageStyleMenu.switchStyleSheet(this.getAttribute('data'));");
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
  },

  _stylesheetInFrame: function (frame, title) {
    return Array.some(frame.document.styleSheets,
                      function (stylesheet) stylesheet.title == title);
  },

  _stylesheetSwitchFrame: function (frame, title) {
    var docStyleSheets = frame.document.styleSheets;

    for (let i = 0; i < docStyleSheets.length; ++i) {
      let docStyleSheet = docStyleSheets[i];

      if (docStyleSheet.title)
        docStyleSheet.disabled = (docStyleSheet.title != title);
      else if (docStyleSheet.disabled)
        docStyleSheet.disabled = false;
    }
  },

  _stylesheetSwitchAll: function (frameset, title) {
    if (!title || this._stylesheetInFrame(frameset, title))
      this._stylesheetSwitchFrame(frameset, title);

    for (let i = 0; i < frameset.frames.length; i++)
      this._stylesheetSwitchAll(frameset.frames[i], title);
  },

  switchStyleSheet: function (title, contentWindow) {
    getMarkupDocumentViewer().authorStyleDisabled = false;
    this._stylesheetSwitchAll(contentWindow || content, title);
  },

  disableStyle: function () {
    getMarkupDocumentViewer().authorStyleDisabled = true;
  },
};


var getAllStyleSheets   = gPageStyleMenu._getAllStyleSheets.bind(gPageStyleMenu);
var stylesheetFillPopup = gPageStyleMenu.fillPopup.bind(gPageStyleMenu);
function stylesheetSwitchAll(contentWindow, title) {
  gPageStyleMenu.switchStyleSheet(title, contentWindow);
}
function setStyleDisabled(disabled) {
  if (disabled)
    gPageStyleMenu.disableStyle();
}


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
          for (let document of notification.documents) {
            OfflineApps.allowSite(document);
          }
        }
      },{
        label: gNavigatorBundle.getString("offlineApps.never"),
        accessKey: gNavigatorBundle.getString("offlineApps.neverAccessKey"),
        callback: function() {
          for (let document of notification.documents) {
            OfflineApps.disallowSite(document);
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

  _quotaPrompt: "indexedDB-quota-prompt",
  _quotaResponse: "indexedDB-quota-response",
  _quotaCancel: "indexedDB-quota-cancel",

  _notificationIcon: "indexedDB-notification-icon",

  init:
  function IndexedDBPromptHelper_init() {
    Services.obs.addObserver(this, this._permissionsPrompt, false);
    Services.obs.addObserver(this, this._quotaPrompt, false);
    Services.obs.addObserver(this, this._quotaCancel, false);
  },

  uninit:
  function IndexedDBPromptHelper_uninit() {
    Services.obs.removeObserver(this, this._permissionsPrompt, false);
    Services.obs.removeObserver(this, this._quotaPrompt, false);
    Services.obs.removeObserver(this, this._quotaCancel, false);
  },

  observe:
  function IndexedDBPromptHelper_observe(subject, topic, data) {
    if (topic != this._permissionsPrompt &&
        topic != this._quotaPrompt &&
        topic != this._quotaCancel) {
      throw new Error("Unexpected topic!");
    }

    var requestor = subject.QueryInterface(Ci.nsIInterfaceRequestor);

    var contentWindow = requestor.getInterface(Ci.nsIDOMWindow);
    var contentDocument = contentWindow.document;
    var browserWindow =
      OfflineApps._getBrowserWindowForContentWindow(contentWindow);

    if (browserWindow != window) {
      
      return;
    }

    var browser =
      OfflineApps._getBrowserForContentWindow(browserWindow, contentWindow);

    var host = contentDocument.documentURIObject.asciiHost;

    var message;
    var responseTopic;
    if (topic == this._permissionsPrompt) {
      message = gNavigatorBundle.getFormattedString("offlineApps.available",
                                                    [ host ]);
      responseTopic = this._permissionsResponse;
    }
    else if (topic == this._quotaPrompt) {
      message = gNavigatorBundle.getFormattedString("indexedDB.usage",
                                                    [ host, data ]);
      responseTopic = this._quotaResponse;
    }
    else if (topic == this._quotaCancel) {
      responseTopic = this._quotaResponse;
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

    if (topic == this._quotaCancel) {
      notification = PopupNotifications.getNotification(this._quotaPrompt,
                                                        browser);
      timeoutNotification();
      return;
    }

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

  for (let browser of gBrowser.browsers) {
    let ds = browser.docShell;
    if (ds.contentViewer && !ds.contentViewer.permitUnload())
      return false;
  }

  return true;
}






function warnAboutClosingWindow() {
  
  let isPBWindow = PrivateBrowsingUtils.isWindowPrivate(window);
  if (!isPBWindow && !toolbar.visible)
    return gBrowser.warnAboutClosingTabs(true);

  
  let e = Services.wm.getEnumerator("navigator:browser");
  let otherPBWindowExists = false;
  let nonPopupPresent = false;
  while (e.hasMoreElements()) {
    let win = e.getNext();
    if (win != window) {
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

  if (!isPBWindow && nonPopupPresent)
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
  
  
  
  return isPBWindow || gBrowser.warnAboutClosingTabs(true);
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
  else
    spec += "?" + formData.join("&");

  PlacesUIUtils.showBookmarkDialog({ action: "add"
                                   , type: "bookmark"
                                   , uri: makeURI(spec)
                                   , title: title
                                   , description: description
                                   , keyword: ""
                                   , postData: postData
                                   , charSet: charset
                                   , hiddenRows: [ "location"
                                                 , "description"
                                                 , "tags"
                                                 , "loadInSidebar" ]
                                   }, window);
}

function SwitchDocumentDirection(aWindow) {
  aWindow.document.dir = (aWindow.document.dir == "ltr" ? "rtl" : "ltr");
  for (var run = 0; run < aWindow.frames.length; run++)
    SwitchDocumentDirection(aWindow.frames[run]);
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
  if (gBrowser.tabs.length == 1 &&
      !gPrefService.getBoolPref("browser.tabs.autoHide") &&
      isTabEmpty(gBrowser.selectedTab))
    blankTabToRemove = gBrowser.selectedTab;

  var tab = null;
  var ss = Cc["@mozilla.org/browser/sessionstore;1"].
           getService(Ci.nsISessionStore);
  if (ss.getClosedTabCount(window) > (aIndex || 0)) {
    TabView.prepareUndoCloseTab(blankTabToRemove);
    tab = ss.undoCloseTab(window, aIndex || 0);
    TabView.afterUndoCloseTab();

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
  if (aTab.hasAttribute("busy"))
    return false;

  let browser = aTab.linkedBrowser;
  if (!isBlankPageURL(browser.currentURI.spec))
    return false;

  if (browser.contentWindow.opener)
    return false;

  if (browser.sessionHistory && browser.sessionHistory.count >= 2)
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
  
  IDENTITY_MODE_IDENTIFIED       : "verifiedIdentity", 
  IDENTITY_MODE_DOMAIN_VERIFIED  : "verifiedDomain",   
  IDENTITY_MODE_UNKNOWN          : "unknownIdentity",  
  IDENTITY_MODE_MIXED_CONTENT    : "unknownIdentity mixedContent",  
  IDENTITY_MODE_MIXED_ACTIVE_CONTENT    : "unknownIdentity mixedContent mixedActiveContent",  
  IDENTITY_MODE_CHROMEUI         : "chromeUI",         

  
  _lastStatus : null,
  _lastLocation : null,
  _mode : "unknownIdentity",

  
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
    this._encryptionLabel[this.IDENTITY_MODE_MIXED_ACTIVE_CONTENT] =
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
  get _identityIcon () {
    delete this._identityIcon;
    return this._identityIcon = document.getElementById("page-proxy-favicon");
  },

  



  _cacheElements : function() {
    delete this._identityBox;
    delete this._identityIconLabel;
    delete this._identityIconCountryLabel;
    delete this._identityIcon;
    this._identityBox = document.getElementById("identity-box");
    this._identityIconLabel = document.getElementById("identity-icon-label");
    this._identityIconCountryLabel = document.getElementById("identity-icon-country-label");
    this._identityIcon = document.getElementById("page-proxy-favicon");
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
    if (location.protocol == "chrome:" || location.protocol == "about:") {
      this.setMode(this.IDENTITY_MODE_CHROMEUI);
    } else if (state & nsIWebProgressListener.STATE_IDENTITY_EV_TOPLEVEL) {
      this.setMode(this.IDENTITY_MODE_IDENTIFIED);
    } else if (state & nsIWebProgressListener.STATE_IS_SECURE) {
      this.setMode(this.IDENTITY_MODE_DOMAIN_VERIFIED);
    } else if (state & nsIWebProgressListener.STATE_IS_BROKEN) {
      if ((state & nsIWebProgressListener.STATE_LOADED_MIXED_ACTIVE_CONTENT) &&
          gPrefService.getBoolPref("security.mixed_content.block_active_content")) {
        this.setMode(this.IDENTITY_MODE_MIXED_ACTIVE_CONTENT);
      } else {
        this.setMode(this.IDENTITY_MODE_MIXED_CONTENT);
      }
    } else {
      this.setMode(this.IDENTITY_MODE_UNKNOWN);
    }

    
    if (state & nsIWebProgressListener.STATE_BLOCKED_MIXED_ACTIVE_CONTENT)
      this.showMixedContentDoorhanger();
  },

  



  showMixedContentDoorhanger : function() {
    
    if (PopupNotifications.getNotification("mixed-content-blocked", gBrowser.selectedBrowser))
      return;

    let helplink = document.getElementById("mixed-content-blocked-helplink");
    helplink.href = Services.urlFormatter.formatURLPref("browser.mixedcontent.warning.infoURL");

    let brandBundle = document.getElementById("bundle_brand");
    let brandShortName = brandBundle.getString("brandShortName");
    let messageString = gNavigatorBundle.getFormattedString("mixedContentBlocked.message", [brandShortName]);
    let action = {
      label: gNavigatorBundle.getString("mixedContentBlocked.keepBlockingButton.label"),
      accessKey: gNavigatorBundle.getString("mixedContentBlocked.keepBlockingButton.accesskey"),
      callback: function() {  }
    };
    let secondaryActions = [
      {
        label: gNavigatorBundle.getString("mixedContentBlocked.unblock.label"),
        accessKey: gNavigatorBundle.getString("mixedContentBlocked.unblock.accesskey"),
        callback: function() {
          
          BrowserReloadWithFlags(nsIWebNavigation.LOAD_FLAGS_ALLOW_MIXED_CONTENT);
        }
      }
    ];
    let options = {
      dismissed: true,
    };
    PopupNotifications.show(gBrowser.selectedBrowser, "mixed-content-blocked",
                            messageString, "mixed-content-blocked-notification-icon",
                            action, secondaryActions, options);
  },

  


  getEffectiveHost : function() {
    if (!this._IDNService)
      this._IDNService = Cc["@mozilla.org/network/idn-service;1"]
                         .getService(Ci.nsIIDNService);
    try {
      let baseDomain =
        Services.eTLD.getBaseDomainFromHost(this._lastLocation.hostname);
      return this._IDNService.convertToDisplayIDN(baseDomain, {});
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

      
      
      
      
      
      
      
      
      if (this._lastLocation.hostname &&
          this._overrideService.hasMatchingOverride(this._lastLocation.hostname,
                                                    (this._lastLocation.port || 443),
                                                    iData.cert, {}, {}))
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
      owner = gNavigatorBundle.getString("identity.ownerUnknown2");
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
    TelemetryStopwatch.start("FX_IDENTITY_POPUP_OPEN_MS");
    event.stopPropagation();

    if ((event.type == "click" && event.button != 0) ||
        (event.type == "keypress" && event.charCode != KeyEvent.DOM_VK_SPACE &&
         event.keyCode != KeyEvent.DOM_VK_RETURN)) {
      TelemetryStopwatch.cancel("FX_IDENTITY_POPUP_OPEN_MS");
      return; 
    }

    
    
    if (this._mode == this.IDENTITY_MODE_CHROMEUI ||
        gURLBar.getAttribute("pageproxystate") != "valid") {
      TelemetryStopwatch.cancel("FX_IDENTITY_POPUP_OPEN_MS");
      return;
    }

    
    
    this._identityPopup.hidden = false;

    
    this.setPopupMessages(this._identityBox.className);

    
    this._identityBox.setAttribute("open", "true");
    var self = this;
    this._identityPopup.addEventListener("popuphidden", function onPopupHidden(e) {
      e.currentTarget.removeEventListener("popuphidden", onPopupHidden, false);
      self._identityBox.removeAttribute("open");
    }, false);

    
    this._identityPopup.openPopup(this._identityIcon, "bottomcenter topleft");
  },

  onPopupShown : function(event) {
    TelemetryStopwatch.finish("FX_IDENTITY_POPUP_OPEN_MS");
    document.getElementById('identity-popup-more-info-button').focus();
  },

  onDragStart: function (event) {
    if (gURLBar.getAttribute("pageproxystate") != "valid")
      return;

    var value = content.location.href;
    var urlString = value + "\n" + content.document.title;
    var htmlString = "<a href=\"" + value + "\">" + value + "</a>";

    var dt = event.dataTransfer;
    dt.setData("text/x-moz-url", urlString);
    dt.setData("text/uri-list", value);
    dt.setData("text/plain", value);
    dt.setData("text/html", htmlString);
    dt.setDragImage(gProxyFavIcon, 16, 16);
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
#ifdef XP_MACOSX
      if (!PrivateBrowsingUtils.permanentPrivateBrowsing) {
        document.documentElement.setAttribute("drawintitlebar", true);
      }
#endif

      
      let docElement = document.documentElement;
      docElement.setAttribute("title",
        docElement.getAttribute("title_privatebrowsing"));
      docElement.setAttribute("titlemodifier",
        docElement.getAttribute("titlemodifier_privatebrowsing"));
      docElement.setAttribute("privatebrowsingmode",
        PrivateBrowsingUtils.permanentPrivateBrowsing ? "permanent" : "temporary");
      gBrowser.updateTitlebar();

      if (PrivateBrowsingUtils.permanentPrivateBrowsing) {
        
        [
          { normal: "menu_newNavigator", private: "menu_newPrivateWindow" },
          { normal: "appmenu_newNavigator", private: "appmenu_newPrivateWindow" },
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

    if (gURLBar) {
      
      gURLBar.setAttribute("autocompletesearchparam", "");
    }
  }
};














function switchToTabHavingURI(aURI, aOpenNew) {
  
  function switchIfURIInWindow(aWindow) {
    
    
    if (PrivateBrowsingUtils.isWindowPrivate(window) ||
        PrivateBrowsingUtils.isWindowPrivate(aWindow)) {
      return false;
    }

    let browsers = aWindow.gBrowser.browsers;
    for (let i = 0; i < browsers.length; i++) {
      let browser = browsers[i];
      if (browser.currentURI.equals(aURI)) {
        
        aWindow.focus();
        aWindow.gBrowser.tabContainer.selectedIndex = i;
        return true;
      }
    }
    return false;
  }

  
  if (!(aURI instanceof Ci.nsIURI))
    aURI = Services.io.newURI(aURI, null, null);

  let isBrowserWindow = !!window.gBrowser;

  
  if (isBrowserWindow && switchIfURIInWindow(window))
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
    if (isBrowserWindow && isTabEmpty(gBrowser.selectedTab))
      gBrowser.selectedBrowser.loadURI(aURI.spec);
    else
      openUILinkIn(aURI.spec, "tab");
  }

  return false;
}

function restoreLastSession() {
  let ss = Cc["@mozilla.org/browser/sessionstore;1"].
           getService(Ci.nsISessionStore);
  ss.restoreLastSession();
}

var TabContextMenu = {
  contextTab: null,
  updateContextMenu: function updateContextMenu(aPopupMenu) {
    this.contextTab = aPopupMenu.triggerNode.localName == "tab" ?
                      aPopupMenu.triggerNode : gBrowser.selectedTab;
    let disabled = gBrowser.tabs.length == 1;

    
    document.getElementById("context_closeTab").disabled =
      disabled && gBrowser.tabContainer._closeWindowWithLastTab;

    var menuItems = aPopupMenu.getElementsByAttribute("tbattr", "tabbrowser-multiple");
    for (let menuItem of menuItems)
      menuItem.disabled = disabled;

    disabled = gBrowser.visibleTabs.length == 1;
    menuItems = aPopupMenu.getElementsByAttribute("tbattr", "tabbrowser-multiple-visible");
    for (let menuItem of menuItems)
      menuItem.disabled = disabled;

    
    document.getElementById("context_undoCloseTab").disabled =
      Cc["@mozilla.org/browser/sessionstore;1"].
      getService(Ci.nsISessionStore).
      getClosedTabCount(window) == 0;

    
    document.getElementById("context_pinTab").hidden = this.contextTab.pinned;
    document.getElementById("context_unpinTab").hidden = !this.contextTab.pinned;

    
    
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

XPCOMUtils.defineLazyGetter(this, "HUDConsoleUI", function () {
  return Cu.import("resource:///modules/HUDService.jsm", {}).HUDService.consoleUI;
});


function safeModeRestart()
{
  
  let promptTitle = gNavigatorBundle.getString("safeModeRestartPromptTitle");
  let promptMessage =
    gNavigatorBundle.getString("safeModeRestartPromptMessage");
  let restartText = gNavigatorBundle.getString("safeModeRestartButton");
  let buttonFlags = (Services.prompt.BUTTON_POS_0 *
                     Services.prompt.BUTTON_TITLE_IS_STRING) +
                    (Services.prompt.BUTTON_POS_1 *
                     Services.prompt.BUTTON_TITLE_CANCEL) +
                    Services.prompt.BUTTON_POS_0_DEFAULT;

  let rv = Services.prompt.confirmEx(window, promptTitle, promptMessage,
                                     buttonFlags, restartText, null, null,
                                     null, {});
  if (rv == 0) {
    Services.startup.restartInSafeMode(Ci.nsIAppStartup.eAttemptQuit);
  }
}











function duplicateTabIn(aTab, where, delta) {
  let newTab = Cc['@mozilla.org/browser/sessionstore;1']
                 .getService(Ci.nsISessionStore)
                 .duplicateTab(window, aTab, delta);

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

function toggleAddonBar() {
  let addonBar = document.getElementById("addon-bar");
  setToolbarVisibility(addonBar, addonBar.collapsed);
}

var Scratchpad = {
  prefEnabledName: "devtools.scratchpad.enabled",

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

XPCOMUtils.defineLazyGetter(window, "gShowPageResizers", function () {
#ifdef XP_WIN
  
  return parseFloat(Services.sysinfo.getProperty("version")) < 6;
#else
  return false;
#endif
});

var MousePosTracker = {
  _listeners: [],
  _x: 0,
  _y: 0,
  get _windowUtils() {
    delete this._windowUtils;
    return this._windowUtils = window.getInterface(Ci.nsIDOMWindowUtils);
  },

  addListener: function (listener) {
    if (this._listeners.indexOf(listener) >= 0)
      return;

    listener._hover = false;
    this._listeners.push(listener);

    this._callListener(listener);
  },

  removeListener: function (listener) {
    var index = this._listeners.indexOf(listener);
    if (index < 0)
      return;

    this._listeners.splice(index, 1);
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
  if (element.ownerDocument == document)
    focusAndSelectUrlBar();
}

