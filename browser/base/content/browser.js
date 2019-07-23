# -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
#   Simon BÃ¼nzli <zeniko@gmail.com>
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

const kXULNS =
    "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";

#ifndef MOZ_PLACES
# For Places-enabled builds, this is in
# chrome:
var Ci = Components.interfaces;
var Cc = Components.classes;
var Cr = Components.results;
#endif

const nsIWebNavigation = Components.interfaces.nsIWebNavigation;

const MAX_HISTORY_MENU_ITEMS = 15;


#ifdef XP_MACOSX
const BROWSER_ADD_BM_FEATURES = "centerscreen,chrome,dialog,resizable,modal";
#else
const BROWSER_ADD_BM_FEATURES = "centerscreen,chrome,dialog,resizable,dependent";
#endif

const TYPE_MAYBE_FEED = "application/vnd.mozilla.maybe.feed";
const TYPE_XUL = "application/vnd.mozilla.xul+xml";

var gBrowserGlue = Components.classes["@mozilla.org/browser/browserglue;1"]
                             .getService(Components.interfaces.nsIBrowserGlue);
var gRDF = null;
var gGlobalHistory = null;
var gURIFixup = null;
var gPageStyleButton = null;
var gCharsetMenu = null;
var gLastBrowserCharset = null;
var gPrevCharset = null;
var gURLBar = null;
var gFindBar = null;
var gProxyButton = null;
var gProxyFavIcon = null;
var gProxyDeck = null;
var gNavigatorBundle = null;
var gIsLoadingBlank = false;
var gLastValidURLStr = "";
var gLastValidURL = null;
var gClickSelectsAll = false;
var gMustLoadSidebar = false;
var gProgressMeterPanel = null;
var gProgressCollapseTimer = null;
var gPrefService = null;
var appCore = null;
var gBrowser = null;
var gSidebarCommand = "";


var gContextMenu = null;

var gChromeState = null; 

var gSanitizeListener = null;

var gURLBarAutoFillPrefListener = null;
var gAutoHideTabbarPrefListener = null;
var gBookmarkAllTabsHandler = null;

#ifdef XP_MACOSX
var gClickAndHoldTimer = null;
#endif






function pageShowEventHandlers(event)
{
  
  if (event.originalTarget == content.document) {
    checkForDirectoryListing();
    charsetLoadListener(event);
    
    XULBrowserWindow.asyncUpdateUI();
  }

  
  var targetBrowser = null;
  if (gBrowser.mTabbedMode) {
    var targetBrowserIndex = gBrowser.getBrowserIndexForDocument(event.originalTarget);
    if (targetBrowserIndex == -1)
      return;
    targetBrowser = gBrowser.getBrowserAtIndex(targetBrowserIndex);
  } else {
    targetBrowser = gBrowser.mCurrentBrowser;
  }

#ifndef MOZ_PLACES_BOOKMARKS
  
  if (targetBrowser.currentURI.spec)
    BMSVC.updateLastVisitedDate(targetBrowser.currentURI.spec,
                                targetBrowser.contentDocument.characterSet);
#endif
}





function getContentAreaFrameCount()
{
  var saveFrameItem = document.getElementById("menu_saveFrame");
  if (!content || !content.frames.length || !isContentFrame(document.commandDispatcher.focusedWindow))
    saveFrameItem.setAttribute("hidden", "true");
  else
    saveFrameItem.removeAttribute("hidden");
}

function UpdateBackForwardCommands(aWebNavigation)
{
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




function ClickAndHoldMouseDownCallback(aButton)
{
  aButton.open = true;
  gClickAndHoldTimer = null;
}

function ClickAndHoldMouseDown(aEvent)
{
  







  if (aEvent.button != 0 ||
      aEvent.originalTarget == aEvent.currentTarget ||
      aEvent.currentTarget.disabled)
    return;

  gClickAndHoldTimer =
    setTimeout(ClickAndHoldMouseDownCallback, 500, aEvent.currentTarget);
}

function MayStopClickAndHoldTimer(aEvent)
{
  
  clearTimeout(gClickAndHoldTimer);
}

function ClickAndHoldStopEvent(aEvent)
{
  if (aEvent.originalTarget.localName != "menuitem" &&
      aEvent.currentTarget.open)
    aEvent.stopPropagation();
}

function SetClickAndHoldHandlers()
{
  function _addClickAndHoldListenersOnElement(aElm)
  {
    aElm.addEventListener("mousedown",
                          ClickAndHoldMouseDown,
                          false);
    aElm.addEventListener("mouseup",
                          MayStopClickAndHoldTimer,
                          false);
    aElm.addEventListener("mouseout",
                          MayStopClickAndHoldTimer,
                          false);  
    
    
    
    aElm.addEventListener("command",
                          ClickAndHoldStopEvent,
                          true);  
    aElm.addEventListener("click",
                          ClickAndHoldStopEvent,
                          true);  
  }

  var backButton = document.getElementById("back-button");
  if (backButton)
    _addClickAndHoldListenersOnElement(backButton);
  var forwardButton = document.getElementById("forward-button");
  if (forwardButton)
    _addClickAndHoldListenersOnElement(forwardButton);
}
#endif

function addBookmarkMenuitems()
{
  var tabbrowser = getBrowser();
  var tabMenu = document.getAnonymousElementByAttribute(tabbrowser,"anonid","tabContextMenu");
  var bookmarkAllTabsItem = document.createElement("menuitem");
  bookmarkAllTabsItem.setAttribute("label", gNavigatorBundle.getString("bookmarkAllTabs_label"));
  bookmarkAllTabsItem.setAttribute("accesskey", gNavigatorBundle.getString("bookmarkAllTabs_accesskey"));
  bookmarkAllTabsItem.setAttribute("command", "Browser:BookmarkAllTabs");
  var bookmarkCurTabItem = document.createElement("menuitem");
  bookmarkCurTabItem.setAttribute("label", gNavigatorBundle.getString("bookmarkCurTab_label"));
  bookmarkCurTabItem.setAttribute("accesskey", gNavigatorBundle.getString("bookmarkCurTab_accesskey"));
  bookmarkCurTabItem.setAttribute("oncommand", "BookmarkThisTab();");
  var menuseparator = document.createElement("menuseparator");
  var insertPos = tabMenu.lastChild.previousSibling;
  tabMenu.insertBefore(bookmarkAllTabsItem, insertPos);
  tabMenu.insertBefore(bookmarkCurTabItem, bookmarkAllTabsItem);
  tabMenu.insertBefore(menuseparator, bookmarkCurTabItem);
}

function BookmarkThisTab()
{
  var tab = getBrowser().mContextTab;
  if (tab.localName != "tab")
    tab = getBrowser().mCurrentTab;

#ifdef MOZ_PLACES_BOOKMARKS
  PlacesCommandHook.bookmarkPage(tab.linkedBrowser);
#else
  addBookmarkAs(tab.linkedBrowser, false);
#endif
}

#ifdef MOZ_PLACES_BOOKMARKS




var gBookmarksObserver = {
  QueryInterface: function G_BO_QueryInterface(aIID) {
    if (aIID.equals(Ci.nsINavBookmarkObserver) ||
        aIID.equals(Ci.nsISupports))
      return this;

    throw Cr.NS_NOINTERFACE;
  },

  _toolbar: null,
  get toolbar() {
    if (!this._toolbar)
      this._toolbar = document.getElementById("bookmarksBarContent");

    return this._toolbar;
  },

  onBeginUpdateBatch: function() { },
  onEndUpdateBatch: function() { },
  onItemAdded: function() { },
  onItemRemoved: function() { },

  onItemChanged:
  function G_BO_onItemChanged(aID, aProperty, aIsAnnotationProperty, aValue) {
    if (aProperty == "became_toolbar_folder" && this.toolbar)
      this.toolbar.place = PlacesUtils.getQueryStringForFolder(aID);
  },
  
  onItemVisited: function() { },
  onFolderAdded: function() { },
  onFolderRemoved: function() { },
  onFolderMoved: function() { },
  onFolderChanged: function() { },
  onSeparatorAdded: function() { },
  onSeparatorRemoved: function() { }
};




function initBookmarksToolbar() {
  var bt = document.getElementById("bookmarksBarContent");
  if (!bt)
    return;

  bt.place =
    PlacesUtils.getQueryStringForFolder(PlacesUtils.bookmarks.toolbarFolder);
}
#endif

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
  _kIPM: Components.interfaces.nsIPermissionManager,

  onUpdatePageReport: function ()
  {
    if (!this._reportButton)
      this._reportButton = document.getElementById("page-report-button");

    if (!gBrowser.pageReport) {
      
      this._reportButton.removeAttribute("blocked");

      return;
    }

    this._reportButton.setAttribute("blocked", true);

    
    
    
    if (!gBrowser.pageReport.reported) {
      if (!gPrefService)
        gPrefService = Components.classes["@mozilla.org/preferences-service;1"]
                                 .getService(Components.interfaces.nsIPrefBranch2);
      if (gPrefService.getBoolPref("privacy.popups.showBrowserMessage")) {
        var bundle_browser = document.getElementById("bundle_browser");
        var brandBundle = document.getElementById("bundle_brand");
        var brandShortName = brandBundle.getString("brandShortName");
        var message;
        var popupCount = gBrowser.pageReport.length;
#ifdef XP_WIN
        var popupButtonText = bundle_browser.getString("popupWarningButton");
        var popupButtonAccesskey = bundle_browser.getString("popupWarningButton.accesskey");
#else
        var popupButtonText = bundle_browser.getString("popupWarningButtonUnix");
        var popupButtonAccesskey = bundle_browser.getString("popupWarningButtonUnix.accesskey");
#endif
        if (popupCount > 1)
          message = bundle_browser.getFormattedString("popupWarningMultiple", [brandShortName, popupCount]);
        else
          message = bundle_browser.getFormattedString("popupWarning", [brandShortName]);

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
    var currentURI = gBrowser.selectedBrowser.webNavigation.currentURI;
    var pm = Components.classes["@mozilla.org/permissionmanager;1"]
                       .getService(this._kIPM);
    var shouldBlock = aEvent.target.getAttribute("block") == "true";
    var perm = shouldBlock ? this._kIPM.DENY_ACTION : this._kIPM.ALLOW_ACTION;
    pm.add(currentURI, "popup", perm);

    gBrowser.getNotificationBox().removeCurrentNotification();
  },

  fillPopupList: function (aEvent)
  {
    var bundle_browser = document.getElementById("bundle_browser");
    
    
    
    
    
    
    
    
    
    var uri = gBrowser.selectedBrowser.webNavigation.currentURI;
    var blockedPopupAllowSite = document.getElementById("blockedPopupAllowSite");
    try {
      blockedPopupAllowSite.removeAttribute("hidden");

      var pm = Components.classes["@mozilla.org/permissionmanager;1"]
                        .getService(this._kIPM);
      if (pm.testPermission(uri, "popup") == this._kIPM.ALLOW_ACTION) {
        
        
        var blockString = bundle_browser.getFormattedString("popupBlock", [uri.host]);
        blockedPopupAllowSite.setAttribute("label", blockString);
        blockedPopupAllowSite.setAttribute("block", "true");
      }
      else {
        
        var allowString = bundle_browser.getFormattedString("popupAllow", [uri.host]);
        blockedPopupAllowSite.setAttribute("label", allowString);
        blockedPopupAllowSite.removeAttribute("block");
      }
    }
    catch (e) {
      blockedPopupAllowSite.setAttribute("hidden", "true");
    }

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
        var label = bundle_browser.getFormattedString("popupShowPopupPrefix",
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
      blockedPopupDontShowMessage.setAttribute("label", bundle_browser.getString("popupWarningDontShowFromMessage"));
    else
      blockedPopupDontShowMessage.setAttribute("label", bundle_browser.getString("popupWarningDontShowFromStatusbar"));
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
      var uri = gBrowser.selectedBrowser.webNavigation.currentURI;
      host = uri.host;
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
    var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                        .getService(Components.interfaces.nsIWindowMediator);
    var existingWindow = wm.getMostRecentWindow("Browser:Permissions");
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
    var tabbrowser = getBrowser();
    for (var i = 0; i < tabbrowser.browsers.length; ++i) {
      var browser = tabbrowser.getBrowserAtIndex(i);
      if (this._findChildShell(browser.docShell, aDocShell))
        return browser;
    }
    return null;
  },

  observe: function (aSubject, aTopic, aData)
  {
    var brandBundle = document.getElementById("bundle_brand");
    var browserBundle = document.getElementById("bundle_browser");
    var browser, webNav, wm;
    switch (aTopic) {
    case "xpinstall-install-blocked":
      var shell = aSubject.QueryInterface(Components.interfaces.nsIDocShell);
      browser = this._getBrowser(shell);
      if (browser) {
        var host = browser.docShell.QueryInterface(Components.interfaces.nsIWebNavigation).currentURI.host;
        var brandShortName = brandBundle.getString("brandShortName");
        var notificationName, messageString, buttons;
        if (!gPrefService.getBoolPref("xpinstall.enabled")) {
          notificationName = "xpinstall-disabled"
          if (gPrefService.prefIsLocked("xpinstall.enabled")) {
            messageString = browserBundle.getString("xpinstallDisabledMessageLocked");
            buttons = [];
          }
          else {
            messageString = browserBundle.getFormattedString("xpinstallDisabledMessage",
                                                             [brandShortName, host]);

            buttons = [{
              label: browserBundle.getString("xpinstallDisabledButton"),
              accessKey: browserBundle.getString("xpinstallDisabledButton.accesskey"),
              popup: null,
              callback: function editPrefs() {
                gPrefService.setBoolPref("xpinstall.enabled", true);
                return false;
              }
            }];
          }
        }
        else {
          
          
          
          notificationName = "xpinstall"
          messageString = browserBundle.getFormattedString("xpinstallPromptWarning",
                                                           [brandShortName, host]);

          buttons = [{
            label: browserBundle.getString("xpinstallPromptWarningButton"),
            accessKey: browserBundle.getString("xpinstallPromptWarningButton.accesskey"),
            popup: null,
            callback: function() { return xpinstallEditPermissions(shell); }
          }];
        }

        var notificationBox = gBrowser.getNotificationBox(browser);
        if (!notificationBox.getNotificationWithValue(notificationName)) {
          const priority = notificationBox.PRIORITY_WARNING_MEDIUM;
          const iconURL = "chrome://mozapps/skin/xpinstall/xpinstallItemGeneric.png";
          notificationBox.appendNotification(messageString, notificationName,
                                             iconURL, priority, buttons);
        }
      }
      break;
    }
  }
};

function xpinstallEditPermissions(aDocShell)
{
  var browser = gXPInstallObserver._getBrowser(aDocShell);
  if (browser) {
    var bundlePreferences = document.getElementById("bundle_preferences");
    var webNav = aDocShell.QueryInterface(Components.interfaces.nsIWebNavigation);
    var params = { blockVisible   : false,
                   sessionVisible : false,
                   allowVisible   : true,
                   prefilledHost  : webNav.currentURI.host,
                   permissionType : "install",
                   windowTitle    : bundlePreferences.getString("addons_permissions_title"),
                   introText      : bundlePreferences.getString("addonspermissionstext") };
    var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                   .getService(Components.interfaces.nsIWindowMediator);
    var existingWindow = wm.getMostRecentWindow("Browser:Permissions");
    if (existingWindow) {
      existingWindow.initWithParams(params);
      existingWindow.focus();
    }
    else
      window.openDialog("chrome://browser/content/preferences/permissions.xul",
                        "_blank", "resizable,dialog=no,centerscreen", params);
    return false;
  }

  return true;
}

function BrowserStartup()
{
  gBrowser = document.getElementById("content");

  window.tryToClose = WindowIsClosing;

  var uriToLoad = null;
  
  if ("arguments" in window && window.arguments[0])
    uriToLoad = window.arguments[0];

  gIsLoadingBlank = uriToLoad == "about:blank";

  prepareForStartup();

#ifdef ENABLE_PAGE_CYCLER
  appCore.startPageCycler();
#else
# only load url passed in when we're not page cycling
  if (uriToLoad && !gIsLoadingBlank) {
    if (window.arguments.length >= 3)
      loadURI(uriToLoad, window.arguments[2], window.arguments[3] || null,
              window.arguments[4] || false);
    else
      loadOneOrMoreURIs(uriToLoad);
  }
#endif

  var sidebarSplitter;
  if (window.opener && !window.opener.closed) {
    var openerFindBar = window.opener.gFindBar;
    if (openerFindBar && !openerFindBar.hidden &&
        openerFindBar.findMode == gFindBar.FIND_NORMAL)
      gFindBar.open();

    var openerSidebarBox = window.opener.document.getElementById("sidebar-box");
    
    
    if (openerSidebarBox && !openerSidebarBox.hidden) {
      var sidebarBox = document.getElementById("sidebar-box");
      var sidebarTitle = document.getElementById("sidebar-title");
      sidebarTitle.setAttribute("value", window.opener.document.getElementById("sidebar-title").getAttribute("value"));
      sidebarBox.setAttribute("width", openerSidebarBox.boxObject.width);
      var sidebarCmd = openerSidebarBox.getAttribute("sidebarcommand");
      sidebarBox.setAttribute("sidebarcommand", sidebarCmd);
      sidebarBox.setAttribute("src", window.opener.document.getElementById("sidebar").getAttribute("src"));
      gMustLoadSidebar = true;
      sidebarBox.hidden = false;
      sidebarSplitter = document.getElementById("sidebar-splitter");
      sidebarSplitter.hidden = false;
      document.getElementById(sidebarCmd).setAttribute("checked", "true");
    }
  }
  else {
    var box = document.getElementById("sidebar-box");
    if (box.hasAttribute("sidebarcommand")) {
      var commandID = box.getAttribute("sidebarcommand");
      if (commandID) {
        var command = document.getElementById(commandID);
        if (command) {
          gMustLoadSidebar = true;
          box.hidden = false;
          sidebarSplitter = document.getElementById("sidebar-splitter");
          sidebarSplitter.hidden = false;
          command.setAttribute("checked", "true");
        }
        else {
          
          
          
          box.removeAttribute("sidebarcommand");
        }
      }
    }
  }

  
  
  var obs = Components.classes["@mozilla.org/observer-service;1"].getService(Components.interfaces.nsIObserverService);
  obs.notifyObservers(null, "browser-window-before-show", "");

  
  if (!document.documentElement.hasAttribute("width")) {
    var defaultWidth = 994, defaultHeight;
    if (screen.availHeight <= 600) {
      document.documentElement.setAttribute("sizemode", "maximized");
      defaultWidth = 610;
      defaultHeight = 450;
    }
    else {
      
      
      if ((screen.availWidth / 2) >= 800)
        defaultWidth = (screen.availWidth / 2) - 20;
      defaultHeight = screen.availHeight - 10;
#ifdef MOZ_WIDGET_GTK2
#define USE_HEIGHT_ADJUST
#endif
#ifdef USE_HEIGHT_ADJUST
      
      
      defaultHeight -= 28;
#endif
#ifdef XP_MACOSX
      
      defaultHeight -= 22;
#endif
    }
    document.documentElement.setAttribute("width", defaultWidth);
    document.documentElement.setAttribute("height", defaultHeight);
  }

  setTimeout(delayedStartup, 0);
}

function HandleAppCommandEvent(evt)
{
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

function prepareForStartup()
{
  gURLBar = document.getElementById("urlbar");
  gNavigatorBundle = document.getElementById("bundle_browser");
  gProgressMeterPanel = document.getElementById("statusbar-progresspanel");
  gFindBar = document.getElementById("FindToolbar");
  gBrowser.addEventListener("DOMUpdatePageReport", gPopupBlockerObserver.onUpdatePageReport, false);
  
  
  gBrowser.addEventListener("PluginNotFound", gMissingPluginInstaller.newMissingPlugin, true, true);
  gBrowser.addEventListener("NewTab", BrowserOpenTab, false);
  window.addEventListener("AppCommand", HandleAppCommandEvent, true);

  var webNavigation;
  try {
    
    appCore = Components.classes["@mozilla.org/appshell/component/browser/instance;1"]
                        .createInstance(Components.interfaces.nsIBrowserInstance);
    if (!appCore)
      throw "couldn't create a browser instance";

    webNavigation = getWebNavigation();
    if (!webNavigation)
      throw "no XBL binding for browser";
  } catch (e) {
    alert("Error launching browser window:" + e);
    window.close(); 
    return;
  }

  
  
  window.XULBrowserWindow = new nsBrowserStatusHandler();
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

  
  appCore.setWebShellWindow(window);

  
  
  
  
  
  webNavigation.sessionHistory = Components.classes["@mozilla.org/browser/shistory;1"]
                                           .createInstance(Components.interfaces.nsISHistory);
  var os = Components.classes["@mozilla.org/observer-service;1"].getService(Components.interfaces.nsIObserverService);
  os.addObserver(gBrowser.browsers[0], "browser:purge-session-history", false);

  
  
  gBrowser.browsers[0].removeAttribute("disablehistory");

  
  gBrowser.docShell.QueryInterface(Components.interfaces.nsIDocShellHistory).useGlobalHistory = true;

  
  gBrowser.addProgressListener(window.XULBrowserWindow, Components.interfaces.nsIWebProgress.NOTIFY_ALL);

  
  FeedHandler.init();

  
  BrowserSearch.init();
}

function delayedStartup()
{
  var os = Components.classes["@mozilla.org/observer-service;1"].getService(Components.interfaces.nsIObserverService);
  os.addObserver(gSessionHistoryObserver, "browser:purge-session-history", false);
  os.addObserver(gXPInstallObserver, "xpinstall-install-blocked", false);

  if (!gPrefService)
    gPrefService = Components.classes["@mozilla.org/preferences-service;1"]
                             .getService(Components.interfaces.nsIPrefBranch2);
  BrowserOffline.init();
  
  if (gURLBar && document.documentElement.getAttribute("chromehidden").indexOf("toolbar") != -1) {
    gURLBar.setAttribute("readonly", "true");
    gURLBar.setAttribute("enablehistory", "false");
    var goButtonStack = document.getElementById("go-button-stack");
    if (goButtonStack)
      goButtonStack.setAttribute("hidden", "true");
  }

  if (gURLBar)
    gURLBar.addEventListener("dragdrop", URLBarOnDrop, true);

  gBrowser.addEventListener("pageshow", function(evt) { setTimeout(pageShowEventHandlers, 0, evt); }, true);

  window.addEventListener("keypress", ctrlNumberTabSelection, false);

  if (gMustLoadSidebar) {
    var sidebar = document.getElementById("sidebar");
    var sidebarBox = document.getElementById("sidebar-box");
    sidebar.setAttribute("src", sidebarBox.getAttribute("src"));
  }

  
  addBookmarkMenuitems();

#ifndef MOZ_PLACES_BOOKMARKS
  initServices();
  initBMService();
  
  BMSVC.readBookmarks();
  var bt = document.getElementById("bookmarks-ptf");
  if (bt) {
    var btf = BMSVC.getBookmarksToolbarFolder().Value;
    bt.ref = btf;
    document.getElementById("bookmarks-chevron").ref = btf;
    bt.database.AddObserver(BookmarksToolbarRDFObserver);
  }
  window.addEventListener("resize", BookmarksToolbar.resizeFunc, false);
  document.getElementById("PersonalToolbar")
          .controllers.appendController(BookmarksMenuController);
#else
  PlacesMenuDNDController.init();

  initBookmarksToolbar();
  PlacesUtils.bookmarks.addObserver(gBookmarksObserver, false);
#endif

  
  
  window.addEventListener("fullscreen", onFullScreen, true);

  if (gIsLoadingBlank && gURLBar && isElementVisible(gURLBar))
    focusElement(gURLBar);
  else
    focusElement(content);

  SetPageProxyState("invalid");

  var toolbox = document.getElementById("navigator-toolbox");
  toolbox.customizeDone = BrowserToolboxCustomizeDone;

  
  gSanitizeListener = new SanitizeListener();

  
  gURLBarAutoFillPrefListener = new URLBarAutoFillPrefListener();
  gPrefService.addObserver(gURLBarAutoFillPrefListener.domain,
                           gURLBarAutoFillPrefListener, false);

  
  gAutoHideTabbarPrefListener = new AutoHideTabbarPrefListener();
  gPrefService.addObserver(gAutoHideTabbarPrefListener.domain,
                           gAutoHideTabbarPrefListener, false);

  gPrefService.addObserver(gHomeButton.prefDomain, gHomeButton, false);
  gHomeButton.updateTooltip();

  gClickSelectsAll = gPrefService.getBoolPref("browser.urlbar.clickSelectsAll");
  if (gURLBar)
    gURLBar.clickSelectsAll = gClickSelectsAll;

#ifdef HAVE_SHELL_SERVICE
  
  var shell = getShellService();
  if (shell) {
    var shouldCheck = shell.shouldCheckDefaultBrowser;
    var willRestoreSession = false;
    try {
      var ss = Cc["@mozilla.org/browser/sessionstartup;1"].
               getService(Ci.nsISessionStartup);
      willRestoreSession = ss.doRestore();
    }
    catch (ex) {  }
    if (shouldCheck && !shell.isDefaultBrowser(true) && !willRestoreSession) {
      var brandBundle = document.getElementById("bundle_brand");
      var shellBundle = document.getElementById("bundle_shell");

      var brandShortName = brandBundle.getString("brandShortName");
      var promptTitle = shellBundle.getString("setDefaultBrowserTitle");
      var promptMessage = shellBundle.getFormattedString("setDefaultBrowserMessage",
                                                         [brandShortName]);
      var checkboxLabel = shellBundle.getFormattedString("setDefaultBrowserDontAsk",
                                                         [brandShortName]);
      const IPS = Components.interfaces.nsIPromptService;
      var ps = Components.classes["@mozilla.org/embedcomp/prompt-service;1"]
                                                .getService(IPS);
      var checkEveryTime = { value: shouldCheck };
      var rv = ps.confirmEx(window, promptTitle, promptMessage,
                            IPS.STD_YES_NO_BUTTONS,
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

  
  
  Cc["@mozilla.org/microsummary/service;1"].getService(Ci.nsIMicrosummaryService);

  
  if (document.documentElement.getAttribute("windowtype") == "navigator:browser") {
    try {
      var ss = Cc["@mozilla.org/browser/sessionstore;1"].
               getService(Ci.nsISessionStore);
      ss.init(window);
    } catch(ex) {
      dump("nsSessionStore could not be initialized: " + ex + "\n");
    }
  }

  
  AugmentTabs.init();

  
  gBookmarkAllTabsHandler = new BookmarkAllTabsHandler();
}

function BrowserShutdown()
{
  var os = Components.classes["@mozilla.org/observer-service;1"]
    .getService(Components.interfaces.nsIObserverService);
  os.removeObserver(gSessionHistoryObserver, "browser:purge-session-history");
  os.removeObserver(gXPInstallObserver, "xpinstall-install-blocked");

  try {
    gBrowser.removeProgressListener(window.XULBrowserWindow);
  } catch (ex) {
  }

#ifdef MOZ_PLACES_BOOKMARKS
  PlacesUtils.bookmarks.removeObserver(gBookmarksObserver);
#else
  try {
    document.getElementById("PersonalToolbar")
            .controllers.removeController(BookmarksMenuController);
  } catch (ex) {
  }

  var bt = document.getElementById("bookmarks-ptf");
  if (bt) {
    try {
      bt.database.RemoveObserver(BookmarksToolbarRDFObserver);
    } catch (ex) {
    }
  }
#endif

  try {
    gPrefService.removeObserver(gURLBarAutoFillPrefListener.domain,
                                gURLBarAutoFillPrefListener);
    gPrefService.removeObserver(gAutoHideTabbarPrefListener.domain,
                                gAutoHideTabbarPrefListener);
    gPrefService.removeObserver(gHomeButton.prefDomain, gHomeButton);
  } catch (ex) {
    Components.utils.reportError(ex);
  }

  if (gSanitizeListener)
    gSanitizeListener.shutdown();

  BrowserOffline.uninit();

  var windowManager = Components.classes['@mozilla.org/appshell/window-mediator;1'].getService();
  var windowManagerInterface = windowManager.QueryInterface(Components.interfaces.nsIWindowMediator);
  var enumerator = windowManagerInterface.getEnumerator(null);
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

  
  if (appCore)
    appCore.close();
}

#ifdef XP_MACOSX


function nonBrowserWindowStartup()
{
  
  var disabledItems = ['cmd_newNavigatorTab', 'Browser:SavePage', 'Browser:SendLink',
                       'cmd_pageSetup', 'cmd_print', 'cmd_find', 'cmd_findAgain', 'viewToolbarsMenu',
                       'cmd_toggleTaskbar', 'viewSidebarMenuMenu', 'Browser:Reload', 'Browser:ReloadSkipCache',
                       'viewTextZoomMenu', 'pageStyleMenu', 'charsetMenu', 'View:PageSource', 'View:FullScreen',
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
  }

  gNavigatorBundle = document.getElementById("bundle_browser");

  setTimeout(nonBrowserWindowDelayedStartup, 0);
}

function nonBrowserWindowDelayedStartup()
{
  
#ifndef MOZ_PLACES_BOOKMARKS
  initServices();
  initBMService();
#endif

  
  gPrefService = Components.classes["@mozilla.org/preferences-service;1"]
                           .getService(Components.interfaces.nsIPrefBranch2);

  
  gSanitizeListener = new SanitizeListener();
}
#endif

function URLBarAutoFillPrefListener()
{
  this.toggleAutoFillInURLBar();
}

URLBarAutoFillPrefListener.prototype =
{
  domain: "browser.urlbar.autoFill",
  observe: function (aSubject, aTopic, aPrefName)
  {
    if (aTopic != "nsPref:changed" || aPrefName != this.domain)
      return;

    this.toggleAutoFillInURLBar();
  },

  toggleAutoFillInURLBar: function ()
  {
    if (!gURLBar)
      return;

    var prefValue = false;
    try {
      prefValue = gPrefService.getBoolPref(this.domain);
    }
    catch (e) {
    }

    if (prefValue)
      gURLBar.setAttribute("completedefaultindex", "true");
    else
      gURLBar.removeAttribute("completedefaultindex");
  }
}

function AutoHideTabbarPrefListener()
{
  this.toggleAutoHideTabbar();
}

AutoHideTabbarPrefListener.prototype =
{
  domain: "browser.tabs.autoHide",
  observe: function (aSubject, aTopic, aPrefName)
  {
    if (aTopic != "nsPref:changed" || aPrefName != this.domain)
      return;

    this.toggleAutoHideTabbar();
  },

  toggleAutoHideTabbar: function ()
  {
    if (gBrowser.tabContainer.childNodes.length == 1 &&
        window.toolbar.visible) {
      var aVisible = false;
      try {
        aVisible = !gPrefService.getBoolPref(this.domain);
      }
      catch (e) {
      }
      gBrowser.setStripVisibilityTo(aVisible);
      gPrefService.setBoolPref("browser.tabs.forceHide", false);
    }
  }
}

function SanitizeListener()
{
  gPrefService.addObserver(this.promptDomain, this, false);

  this._defaultLabel = document.getElementById("sanitizeItem")
                               .getAttribute("label");
  this._updateSanitizeItem();

  if (gPrefService.prefHasUserValue(this.didSanitizeDomain)) {
    gPrefService.clearUserPref(this.didSanitizeDomain);
    
    
    gPrefService.savePrefFile(null);
  }
}

SanitizeListener.prototype =
{
  promptDomain      : "privacy.sanitize.promptOnSanitize",
  didSanitizeDomain : "privacy.sanitize.didShutdownSanitize",

  observe: function (aSubject, aTopic, aPrefName)
  {
    this._updateSanitizeItem();
  },

  shutdown: function ()
  {
    gPrefService.removeObserver(this.promptDomain, this);
  },

  _updateSanitizeItem: function ()
  {
    var label = gPrefService.getBoolPref(this.promptDomain) ?
        gNavigatorBundle.getString("sanitizeWithPromptLabel") : 
        this._defaultLabel;
    document.getElementById("sanitizeItem").setAttribute("label", label);
  }
}

function ctrlNumberTabSelection(event)
{
  if (event.altKey && event.keyCode == KeyEvent.DOM_VK_RETURN) {
    
    
    
    if (!(document.commandDispatcher.focusedElement instanceof HTMLAnchorElement)) {
      
      event.preventDefault();
      return;
    }
  }

#ifdef XP_MACOSX
  if (!event.metaKey)
#else
#ifdef XP_UNIX
  
  if (!event.altKey || event.shiftKey)
#else
  if (!event.ctrlKey)
#endif
#endif
    return;

  
  
  var regExp = /\d/;
  if (!regExp.test(String.fromCharCode(event.charCode)))
    return;

  
  
  
  var digit1 = (event.charCode & 0xFFF0) | 1;
  if (!regExp.test(String.fromCharCode(digit1)))
    digit1 += 6;

  var index = event.charCode - digit1;
  if (index < 0)
    return;

  
  if (index == 8)
    index = gBrowser.tabContainer.childNodes.length - 1;
  else if (index >= gBrowser.tabContainer.childNodes.length)
    return;

  var oldTab = gBrowser.selectedTab;
  var newTab = gBrowser.tabContainer.childNodes[index];
  if (newTab != oldTab)
    gBrowser.selectedTab = newTab;

  event.preventDefault();
  event.stopPropagation();
}

function gotoHistoryIndex(aEvent)
{
  var index = aEvent.target.getAttribute("index");
  if (!index)
    return false;

  var where = whereToOpenLink(aEvent);

  if (where == "current") {
    

    try {
      getWebNavigation().gotoIndex(index);
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
    openUILinkIn(url, where);
    return true;
  }
}

function BrowserForward(aEvent, aIgnoreAlt)
{
  var where = whereToOpenLink(aEvent, false, aIgnoreAlt);

  if (where == "current") {
    try {
      getWebNavigation().goForward();
    }
    catch(ex) {
    }
  }
  else {
    var sessionHistory = getWebNavigation().sessionHistory;
    var currentIndex = sessionHistory.index;
    var entry = sessionHistory.getEntryAtIndex(currentIndex + 1, false);
    var url = entry.URI.spec;
    openUILinkIn(url, where);
  }
}

function BrowserBack(aEvent, aIgnoreAlt)
{
  var where = whereToOpenLink(aEvent, false, aIgnoreAlt);

  if (where == "current") {
    try {
      getWebNavigation().goBack();
    }
    catch(ex) {
    }
  }
  else {
    var sessionHistory = getWebNavigation().sessionHistory;
    var currentIndex = sessionHistory.index;
    var entry = sessionHistory.getEntryAtIndex(currentIndex - 1, false);
    var url = entry.URI.spec;
    openUILinkIn(url, where);
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

function BrowserBackMenu(event)
{
  return FillHistoryMenu(event.target, "back");
}

function BrowserForwardMenu(event)
{
  return FillHistoryMenu(event.target, "forward");
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

function BrowserReload()
{
  const reloadFlags = nsIWebNavigation.LOAD_FLAGS_NONE;
  return BrowserReloadWithFlags(reloadFlags);
}

function BrowserReloadSkipCache()
{
  
  const reloadFlags = nsIWebNavigation.LOAD_FLAGS_BYPASS_PROXY | nsIWebNavigation.LOAD_FLAGS_BYPASS_CACHE;
  return BrowserReloadWithFlags(reloadFlags);
}

function BrowserHome()
{
  var homePage = gHomeButton.getHomePage();
  loadOneOrMoreURIs(homePage);
}

function BrowserHomeClick(aEvent)
{
  if (aEvent.button == 2) 
    return;

  var homePage = gHomeButton.getHomePage();
  var where = whereToOpenLink(aEvent);
  var urls;

  
  switch (where) {
  case "save":
    urls = homePage.split("|");
    saveURL(urls[0], null, null, true);  
    break;
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
    newWindow = openDialog(getBrowserURL(), "_blank", "all,dialog=no", aURIString);
    return;
  }
#endif
  
  
  try {
    gBrowser.loadTabs(aURIString.split("|"), false, true);
  } 
  catch (e) {
  }
}

#ifndef MOZ_PLACES
function constructGoMenuItem(goMenu, beforeItem, url, title)
{
  var menuitem = document.createElementNS(kXULNS, "menuitem");
  menuitem.setAttribute("statustext", url);
  menuitem.setAttribute("label", title);
  goMenu.insertBefore(menuitem, beforeItem);
  return menuitem;
}

function onGoMenuHidden(aEvent)
{
  if (aEvent.target == aEvent.currentTarget)
    setTimeout(destroyGoMenuItems, 0, document.getElementById('goPopup'));
}

function destroyGoMenuItems(goMenu) {
  var startSeparator = document.getElementById("startHistorySeparator");
  var endSeparator = document.getElementById("endHistorySeparator");
  endSeparator.hidden = true;

  
  var destroy = false;
  for (var i = 0; i < goMenu.childNodes.length; i++) {
    var item = goMenu.childNodes[i];
    if (item == endSeparator)
      break;

    if (destroy) {
      i--;
      goMenu.removeChild(item);
    }

    if (item == startSeparator)
      destroy = true;
  }
}

function updateGoMenu(aEvent, goMenu)
{
  if (aEvent.target != aEvent.currentTarget)
    return;

  
  destroyGoMenuItems(goMenu);

  
  
  HistoryMenu.toggleRecentlyClosedTabs();

  var history = document.getElementById("hiddenHistoryTree");

  if (history.hidden) {
    history.hidden = false;
    var globalHistory = Components.classes["@mozilla.org/browser/global-history;2"]
                                  .getService(Components.interfaces.nsIRDFDataSource);
    history.database.AddDataSource(globalHistory);
  }

  if (!history.ref)
    history.ref = "NC:HistoryRoot";

  var count = history.view.rowCount;
  if (count > 10)
    count = 10;

  if (count == 0)
    return;

  const NC_NS     = "http://home.netscape.com/NC-rdf#";

  if (!gRDF)
     gRDF = Components.classes["@mozilla.org/rdf/rdf-service;1"]
                      .getService(Components.interfaces.nsIRDFService);

  var builder = history.builder.QueryInterface(Components.interfaces.nsIXULTreeBuilder);

  var beforeItem = document.getElementById("endHistorySeparator");

  var nameResource = gRDF.GetResource(NC_NS + "Name");

  var endSep = beforeItem;
  var showSep = false;

  for (var i = count-1; i >= 0; i--) {
    var res = builder.getResourceAtIndex(i);
    var url = res.Value;
    var titleRes = history.database.GetTarget(res, nameResource, true);
    if (!titleRes)
      continue;

    showSep = true;
    var titleLiteral = titleRes.QueryInterface(Components.interfaces.nsIRDFLiteral);
    beforeItem = constructGoMenuItem(goMenu, beforeItem, url, titleLiteral.Value);
  }

  if (showSep)
    endSep.hidden = false;
}
#endif
 
#ifndef MOZ_PLACES_BOOKMARKS
function addBookmarkAs(aBrowser, aBookmarkAllTabs, aIsWebPanel)
{
  const browsers = aBrowser.browsers;

  
  
  
  if ((!browsers || browsers.length == 1) && aBookmarkAllTabs)
    return;

  if (browsers && browsers.length > 1)
    addBookmarkForTabBrowser(aBrowser, aBookmarkAllTabs);
  else
    addBookmarkForBrowser(aBrowser.webNavigation, aIsWebPanel);
}

function addBookmarkForTabBrowser(aTabBrowser, aBookmarkAllTabs, aSelect)
{
  var tabsInfo = [];
  var currentTabInfo = { name: "", url: "", charset: null };

  const activeBrowser = aTabBrowser.selectedBrowser;
  const browsers = aTabBrowser.browsers;
  for (var i = 0; i < browsers.length; ++i) {
    var webNav = browsers[i].webNavigation;
    var url = webNav.currentURI.spec;
    var name = "";
    var charSet, description;
    try {
      var doc = webNav.document;
      name = doc.title || url;
      charSet = doc.characterSet;
      description = BookmarksUtils.getDescriptionFromDocument(doc);
    } catch (e) {
      name = url;
    }
    tabsInfo[i] = { name: name, url: url, charset: charSet, description: description };
    if (browsers[i] == activeBrowser)
      currentTabInfo = tabsInfo[i];
  }
  var dialogArgs = currentTabInfo;
  if (aBookmarkAllTabs) {
    dialogArgs = { name: gNavigatorBundle.getString("bookmarkAllTabsDefault") };
    dialogArgs.bBookmarkAllTabs = true;
  }

  dialogArgs.objGroup = tabsInfo;
  openDialog("chrome:
             BROWSER_ADD_BM_FEATURES, dialogArgs);
}

function addBookmarkForBrowser(aDocShell, aIsWebPanel)
{
  
  
  
  
  
  var url = aDocShell.currentURI.spec;
  var title, charSet = null;
  var description;
  try {
    title = aDocShell.document.title || url;
    charSet = aDocShell.document.characterSet;
    description = BookmarksUtils.getDescriptionFromDocument(aDocShell.document);
  }
  catch (e) {
    title = url;
  }
  BookmarksUtils.addBookmark(url, title, charSet, aIsWebPanel, description);
}
#endif

function openLocation()
{
  if (gURLBar && isElementVisible(gURLBar)) {
    gURLBar.focus();
    gURLBar.select();
    return;
  }
#ifdef XP_MACOSX
  if (window.location.href != getBrowserURL()) {
    var win = getTopWin();
    if (win) {
      
      win.focus()
      win.openLocation();
    }
    else {
      
      win = window.openDialog("chrome:
                              "chrome,all,dialog=no", "about:blank");
      win.addEventListener("load", openLocationCallback, false);
    }
    return;
  }
#endif
  openDialog("chrome:
             "chrome,modal,titlebar", window);
}

function openLocationCallback()
{
  
  setTimeout(function() { this.openLocation(); }, 0);
}

function BrowserOpenTab()
{
  gBrowser.loadOneTab("about:blank", null, null, null, false, false);
  if (gURLBar)
    setTimeout(function() { gURLBar.focus(); }, 0);
}





function delayedOpenWindow(chrome, flags, href, postData)
{
  
  
  
  
  
  setTimeout(function() { openDialog(chrome, "_blank", flags, href, null, null, postData); }, 10);
}



function delayedOpenTab(aUrl, aReferrer, aCharset, aPostData, aAllowThirdPartyFixup)
{
  gBrowser.loadOneTab(aUrl, aReferrer, aCharset, aPostData, false, aAllowThirdPartyFixup);
}

function BrowserOpenFileWindow()
{
  
  try {
    const nsIFilePicker = Components.interfaces.nsIFilePicker;
    var fp = Components.classes["@mozilla.org/filepicker;1"].createInstance(nsIFilePicker);
    fp.init(window, gNavigatorBundle.getString("openFile"), nsIFilePicker.modeOpen);
    fp.appendFilters(nsIFilePicker.filterAll | nsIFilePicker.filterText | nsIFilePicker.filterImages |
                     nsIFilePicker.filterXML | nsIFilePicker.filterHTML);

    if (fp.show() == nsIFilePicker.returnOK)
      openTopWin(fp.fileURL.spec);
  } catch (ex) {
  }
}

function BrowserCloseTabOrWindow()
{
#ifdef XP_MACOSX
  
  if (window.location.href != getBrowserURL()) {
    closeWindow(true);
    return;
  }
#endif

  if (gBrowser.tabContainer.childNodes.length > 1) {
    
    gBrowser.removeCurrentTab();
    return;
  }
#ifndef XP_MACOSX
  if (window.toolbar.visible &&
      !gPrefService.getBoolPref("browser.tabs.autoHide")) {
    
    gBrowser.removeCurrentTab();
    if (gURLBar)
      setTimeout(function() { gURLBar.focus(); }, 0);
    return;
  }
#endif

  BrowserCloseWindow();
}

function BrowserTryToCloseWindow()
{
  
  if (typeof(window.tryToClose) != "function" || window.tryToClose())
    BrowserCloseWindow();
}

function BrowserCloseWindow()
{
  
  
  
  
  

  
  var x = window.screenX;
  var y = window.screenY;
  var h = window.outerHeight;
  var w = window.outerWidth;

  
  var win = document.getElementById( "main-window" );
  win.setAttribute( "x", x );
  win.setAttribute( "y", y );
  win.setAttribute( "height", h );
  win.setAttribute( "width", w );

  closeWindow(true);
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
    getWebNavigation().loadURI(uri, flags, referrer, postData, null);
  } catch (e) {
  }
}

function BrowserLoadURL(aTriggeringEvent, aPostData) {
  var url = gURLBar.value;

  if (aTriggeringEvent instanceof MouseEvent) {
    if (aTriggeringEvent.button == 2)
      return; 

    
    
    openUILink(url, aTriggeringEvent, false, false,
               true , aPostData);
    return;
  }

  if (aTriggeringEvent && aTriggeringEvent.altKey) {
    handleURLBarRevert();
    content.focus();
    gBrowser.loadOneTab(url, null, null, aPostData, false,
                        true );
    aTriggeringEvent.preventDefault();
    aTriggeringEvent.stopPropagation();
  }
  else
    loadURI(url, null, aPostData, true );

  focusElement(content);
}

function getShortcutOrURI(aURL, aPostDataRef)
{
  
  try {
    var shortcutURL = null;
#ifdef MOZ_PLACES_BOOKMARKS
    var shortcutURI = PlacesUtils.bookmarks.getURIForKeyword(aURL);
    if (shortcutURI)
      shortcutURL = shortcutURI.spec;
#else
    shortcutURL = BMSVC.resolveKeyword(aURL, aPostDataRef);
#endif
    if (!shortcutURL) {
      
      
      var aOffset = aURL.indexOf(" ");
      if (aOffset > 0) {
        var cmd = aURL.substr(0, aOffset);
        var text = aURL.substr(aOffset+1);
#ifdef MOZ_PLACES_BOOKMARKS
        shortcutURI = PlacesUtils.bookmarks.getURIForKeyword(cmd);
        if (shortcutURI)
          shortcutURL = shortcutURI.spec;
#else
        shortcutURL = BMSVC.resolveKeyword(cmd, aPostDataRef);
#endif
        if (shortcutURL && text) {
          var encodedText = null; 
          var charset = "";
          const re = /^(.*)\&mozcharset=([a-zA-Z][_\-a-zA-Z0-9]+)\s*$/; 
          var matches = shortcutURL.match(re);
          if (matches) {
             shortcutURL = matches[1];
             charset = matches[2];
          }
#ifndef MOZ_PLACES_BOOKMARKS
          
          else if (/%s/.test(shortcutURL) || 
                   (aPostDataRef && /%s/.test(aPostDataRef.value))) {
            try {
              charset = BMSVC.getLastCharset(shortcutURL);
            } catch (ex) {
            }
          }
#endif

          if (charset)
            encodedText = escape(convertFromUnicode(charset, text)); 
          else  
            encodedText = encodeURIComponent(text);

          if (aPostDataRef && aPostDataRef.value) {
            
            
            aPostDataRef.value = unescape(aPostDataRef.value);
            if (aPostDataRef.value.match(/%[sS]/)) {
              aPostDataRef.value = getPostDataStream(aPostDataRef.value,
                                                     text, encodedText,
                                                     "application/x-www-form-urlencoded");
            }
            else {
              shortcutURL = null;
              aPostDataRef.value = null;
            }
          }
          else {
            if (/%[sS]/.test(shortcutURL))
              shortcutURL = shortcutURL.replace(/%s/g, encodedText)
                                       .replace(/%S/g, text);
            else 
              shortcutURL = null;
          }
        }
      }
    }

    if (shortcutURL)
      aURL = shortcutURL;

  } catch (ex) {
  }
  return aURL;
}

#if 0


function normalizePostData(aStringData)
{
  var parts = aStringData.split("&");
  var result = "";
  for (var i = 0; i < parts.length; ++i) {
    var part = unescape(parts[i]);
    if (part)
      result += part + "\r\n";
  }
  return result;
}
#endif
function getPostDataStream(aStringData, aKeyword, aEncKeyword, aType)
{
  var dataStream = Components.classes["@mozilla.org/io/string-input-stream;1"]
                            .createInstance(Components.interfaces.nsIStringInputStream);
  aStringData = aStringData.replace(/%s/g, aEncKeyword).replace(/%S/g, aKeyword);
#ifdef MOZILLA_1_8_BRANCH
# bug 318193
  dataStream.setData(aStringData, aStringData.length);
#else
  dataStream.data = aStringData;
#endif

  var mimeStream = Components.classes["@mozilla.org/network/mime-input-stream;1"]
                              .createInstance(Components.interfaces.nsIMIMEInputStream);
  mimeStream.addHeader("Content-Type", aType);
  mimeStream.addContentLength = true;
  mimeStream.setData(dataStream);
  return mimeStream.QueryInterface(Components.interfaces.nsIInputStream);
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

  ViewSourceOfURL(webNav.currentURI.spec, pageCookie, aDocument);
}

function ViewSourceOfURL(aURL, aPageDescriptor, aDocument)
{
  if (getBoolPref("view_source.editor.external", false)) {
    gViewSourceUtils.openInExternalEditor(aURL, aPageDescriptor, aDocument);
  }
  else {
    gViewSourceUtils.openInInternalViewer(aURL, aPageDescriptor, aDocument);
  }
}



function BrowserPageInfo(doc, initialTab)
{
  var args = {doc: doc, initialTab: initialTab};
  toOpenDialogByTypeAndUrl("Browser:page-info",
                           doc ? doc.location : window.content.document.location,
                           "chrome:
                           "chrome,toolbar,dialog=no,resizable",
                           args);
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

function checkForDirectoryListing()
{
  if ( "HTTPIndex" in content &&
       content.HTTPIndex instanceof Components.interfaces.nsIHTTPIndex ) {
    content.wrappedJSObject.defaultCharacterset =
      getMarkupDocumentViewer().defaultCharacterSet;
  }
}



function handleURLBarRevert()
{
  var url = getWebNavigation().currentURI.spec;
  var throbberElement = document.getElementById("navigator-throbber");

  var isScrolling = gURLBar.popupOpen;

  
  
  if ((!throbberElement || !throbberElement.hasAttribute("busy")) && !isScrolling) {
    if (url != "about:blank" || content.opener) {
      gURLBar.value = url;
      gURLBar.select();
      SetPageProxyState("valid");
    } else { 
      gURLBar.value = "";
    }
  }

  gBrowser.userTypedValue = null;

  
  
  return !isScrolling;
}

function handleURLBarCommand(aTriggeringEvent) {
  if (!gURLBar.value)
    return;

  var postData = { };
  canonizeUrl(aTriggeringEvent, postData);

  try {
    addToUrlbarHistory(gURLBar.value);
  } catch (ex) {
    
    
  }

  BrowserLoadURL(aTriggeringEvent, postData.value);
}

function canonizeUrl(aTriggeringEvent, aPostDataRef) {
  if (!gURLBar || !gURLBar.value)
    return;

  var url = gURLBar.value;

  
  
  
  
  if (!/^(www|http)|\/\s*$/i.test(url) &&
      (aTriggeringEvent instanceof KeyEvent)) {
#ifdef XP_MACOSX
    var accel = aTriggeringEvent.metaKey;
#else
    var accel = aTriggeringEvent.ctrlKey;
#endif
    var shift = aTriggeringEvent.shiftKey;

    var suffix = "";

    switch (true) {
      case (accel && shift):
        suffix = ".org/";
        break;
      case (shift):
        suffix = ".net/";
        break;
      case (accel):
        try {
          suffix = gPrefService.getCharPref("browser.fixup.alternate.suffix");
          if (suffix.charAt(suffix.length - 1) != "/")
            suffix += "/";
        } catch(e) {
          suffix = ".com/";
        }
        break;
    }

    if (suffix) {
      
      url = url.replace(/^\s+/, "").replace(/\s+$/, "");

      
      
      
      
      var firstSlash = url.indexOf("/");
      var existingSuffix = url.indexOf(suffix.substring(0, suffix.length - 1));

      
      
      
      
      
      
      
      if (firstSlash >= 0) {
        if (existingSuffix == -1 || existingSuffix > firstSlash)
          url = url.substring(0, firstSlash) + suffix +
                url.substring(firstSlash + 1);
      } else
        url = url + (existingSuffix == -1 ? suffix : "/");

      url = "http:
    }
  }

  gURLBar.value = getShortcutOrURI(url, aPostDataRef);

  
  gBrowser.userTypedValue = gURLBar.value;
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

  if (!gProxyButton)
    gProxyButton = document.getElementById("page-proxy-button");
  if (!gProxyFavIcon)
    gProxyFavIcon = document.getElementById("page-proxy-favicon");
  if (!gProxyDeck)
    gProxyDeck = document.getElementById("page-proxy-deck");

  gProxyButton.setAttribute("pageproxystate", aState);

  
  
  if (aState == "valid") {
    gLastValidURLStr = gURLBar.value;
    gURLBar.addEventListener("input", UpdatePageProxyState, false);

    PageProxySetIcon(gBrowser.mCurrentBrowser.mIconURL);
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
  else if (gProxyDeck.selectedIndex != 1)
    gProxyDeck.selectedIndex = 1;
}

function PageProxyClearIcon ()
{
  if (gProxyDeck.selectedIndex != 0)
    gProxyDeck.selectedIndex = 0;
  if (gProxyFavIcon.hasAttribute("src"))
    gProxyFavIcon.removeAttribute("src");
}

function PageProxyDragGesture(aEvent)
{
  if (gProxyButton.getAttribute("pageproxystate") == "valid") {
    nsDragAndDrop.startDrag(aEvent, proxyIconDNDObserver);
    return true;
  }
  return false;
}

function PageProxyClickHandler(aEvent)
{
  switch (aEvent.button) {
    case 0:
      gURLBar.select();
      break;
    case 1:
      if (gPrefService.getBoolPref("middlemouse.paste"))
        middleMousePaste(aEvent);
      break;
  }
  return true;
}

function URLBarOnDrop(evt)
{
  nsDragAndDrop.drop(evt, urlbarObserver);
}

var urlbarObserver = {
  onDrop: function (aEvent, aXferData, aDragSession)
    {
      var url = transferUtils.retrieveURLFromData(aXferData.data, aXferData.flavour.contentType);

      
      
      if (url) {
        getBrowser().dragDropSecurityCheck(aEvent, aDragSession, url);

        try {
          gURLBar.value = url;
          const nsIScriptSecMan = Components.interfaces.nsIScriptSecurityManager;
          urlSecurityCheck(gURLBar.value,
                           gBrowser.contentPrincipal,
                           nsIScriptSecMan.DISALLOW_INHERIT_PRINCIPAL);
          handleURLBarCommand();
        } catch (ex) {}
      }
    },
  getSupportedFlavours: function ()
    {
      var flavourSet = new FlavourSet();

      
      flavourSet.appendFlavour("text/unicode");
      flavourSet.appendFlavour("text/x-moz-url");
      flavourSet.appendFlavour("application/x-moz-file", "nsIFile");
      return flavourSet;
    }
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
    window.openDialog("chrome:
                      "migration", "centerscreen,chrome,resizable=no");
  }
#else
  window.openDialog("chrome:
                    "migration", "modal,centerscreen,chrome,resizable=no");
#endif
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

function BrowserReloadWithFlags(reloadFlags)
{
  





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

function toggleAffectedChrome(aHide)
{
  
  
  
  
  
  
  
  

  var navToolbox = document.getElementById("navigator-toolbox");
  navToolbox.hidden = aHide;
  if (aHide)
  {
    gChromeState = {};
    var sidebar = document.getElementById("sidebar-box");
    gChromeState.sidebarOpen = !sidebar.hidden;
    gSidebarCommand = sidebar.getAttribute("sidebarcommand");

    var notificationBox = gBrowser.getNotificationBox();
    gChromeState.notificationsOpen = !notificationBox.notificationsHidden;
    notificationBox.notificationsHidden = aHide;

    var statusbar = document.getElementById("status-bar");
    gChromeState.statusbarOpen = !statusbar.hidden;
    statusbar.hidden = aHide;

    gChromeState.findOpen = !gFindBar.hidden;
    gFindBar.close();
  }
  else {
    if (gChromeState.notificationsOpen) {
      gBrowser.getNotificationBox().notificationsHidden = aHide;
    }

    if (gChromeState.statusbarOpen) {
      var statusbar = document.getElementById("status-bar");
      statusbar.hidden = aHide;
    }

    if (gChromeState.findOpen)
      gFindBar.open();
  }

  if (gChromeState.sidebarOpen)
    toggleSidebar(gSidebarCommand);
}

function onEnterPrintPreview()
{
  toggleAffectedChrome(true);
}

function onExitPrintPreview()
{
  
  toggleAffectedChrome(false);
}

function getMarkupDocumentViewer()
{
  return gBrowser.markupDocumentViewer;
}














function FillInHTMLTooltip(tipElement)
{
  var retVal = false;
  if (tipElement.namespaceURI == "http:
    return retVal;

  const XLinkNS = "http:


  var titleText = null;
  var XLinkTitleText = null;
  var direction = tipElement.ownerDocument.dir;

  while (!titleText && !XLinkTitleText && tipElement) {
    if (tipElement.nodeType == Node.ELEMENT_NODE) {
      titleText = tipElement.getAttribute("title");
      XLinkTitleText = tipElement.getAttributeNS(XLinkNS, "title");
      var defView = tipElement.ownerDocument.defaultView;
      
      
      if (!defView)
        return retVal;
      direction = defView.getComputedStyle(tipElement, "")
        .getPropertyValue("direction");
    }
    tipElement = tipElement.parentNode;
  }

  var tipNode = document.getElementById("aHTMLTooltip").firstChild;
  var label = tipNode.firstChild;

  for each (var t in [titleText, XLinkTitleText]) {
    if (t && /\S/.test(t)) {
      tipNode.style.direction = direction;

      
      
      
      
      
      t = t.replace(/[\r\t]/g, ' ');
      t = t.replace(/\n/g, '');
      
      label.textContent = t;
      
      
      tipNode.width = "";
      tipNode.height = "";
      tipNode.width = label.boxObject.width;
      tipNode.height = label.boxObject.height;

      retVal = true;
    }
  }

  return retVal;
}

var proxyIconDNDObserver = {
  onDragStart: function (aEvent, aXferData, aDragAction)
    {
      var value = gURLBar.value;
      
      
      
      if (!value) return;

      var urlString = value + "\n" + window.content.document.title;
      var htmlString = "<a href=\"" + value + "\">" + value + "</a>";

      aXferData.data = new TransferData();
      aXferData.data.addDataForFlavour("text/x-moz-url", urlString);
      aXferData.data.addDataForFlavour("text/unicode", value);
      aXferData.data.addDataForFlavour("text/html", htmlString);

      
      
      
      aDragAction.action =
        Components.interfaces.nsIDragService.DRAGDROP_ACTION_COPY |
        Components.interfaces.nsIDragService.DRAGDROP_ACTION_MOVE |
        Components.interfaces.nsIDragService.DRAGDROP_ACTION_LINK;
    }
}

var homeButtonObserver = {
  onDrop: function (aEvent, aXferData, aDragSession)
    {
      var url = transferUtils.retrieveURLFromData(aXferData.data, aXferData.flavour.contentType);
      setTimeout(openHomeDialog, 0, url);
    },

  onDragOver: function (aEvent, aFlavour, aDragSession)
    {
      var statusTextFld = document.getElementById("statusbar-display");
      statusTextFld.label = gNavigatorBundle.getString("droponhomebutton");
      aDragSession.dragAction = Components.interfaces.nsIDragService.DRAGDROP_ACTION_LINK;
    },

  onDragExit: function (aEvent, aDragSession)
    {
      var statusTextFld = document.getElementById("statusbar-display");
      statusTextFld.label = "";
    },

  getSupportedFlavours: function ()
    {
      var flavourSet = new FlavourSet();
      flavourSet.appendFlavour("application/x-moz-file", "nsIFile");
      flavourSet.appendFlavour("text/x-moz-url");
      flavourSet.appendFlavour("text/unicode");
      return flavourSet;
    }
}

function openHomeDialog(aURL)
{
  var promptService = Components.classes["@mozilla.org/embedcomp/prompt-service;1"].getService(Components.interfaces.nsIPromptService);
  var promptTitle = gNavigatorBundle.getString("droponhometitle");
  var promptMsg   = gNavigatorBundle.getString("droponhomemsg");
  var pressedVal  = promptService.confirmEx(window, promptTitle, promptMsg,
                          promptService.STD_YES_NO_BUTTONS,
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

#ifndef MOZ_PLACES_BOOKMARKS
var bookmarksButtonObserver = {
  onDrop: function (aEvent, aXferData, aDragSession)
  {
    var split = aXferData.data.split("\n");
    var url = split[0];
    if (url != aXferData.data) {  
      var dialogArgs = {
        name: split[1],
        url: url
      }
      openDialog("chrome://browser/content/bookmarks/addBookmark2.xul", "",
                 BROWSER_ADD_BM_FEATURES, dialogArgs);
    }
  },

  onDragOver: function (aEvent, aFlavour, aDragSession)
  {
    var statusTextFld = document.getElementById("statusbar-display");
    statusTextFld.label = gNavigatorBundle.getString("droponbookmarksbutton");
    aDragSession.dragAction = Components.interfaces.nsIDragService.DRAGDROP_ACTION_LINK;
  },

  onDragExit: function (aEvent, aDragSession)
  {
    var statusTextFld = document.getElementById("statusbar-display");
    statusTextFld.label = "";
  },

  getSupportedFlavours: function ()
  {
    var flavourSet = new FlavourSet();
    flavourSet.appendFlavour("application/x-moz-file", "nsIFile");
    flavourSet.appendFlavour("text/x-moz-url");
    flavourSet.appendFlavour("text/unicode");
    return flavourSet;
  }
}
#endif

var newTabButtonObserver = {
  onDragOver: function(aEvent, aFlavour, aDragSession)
    {
      var statusTextFld = document.getElementById("statusbar-display");
      statusTextFld.label = gNavigatorBundle.getString("droponnewtabbutton");
      aEvent.target.setAttribute("dragover", "true");
      return true;
    },
  onDragExit: function (aEvent, aDragSession)
    {
      var statusTextFld = document.getElementById("statusbar-display");
      statusTextFld.label = "";
      aEvent.target.removeAttribute("dragover");
    },
  onDrop: function (aEvent, aXferData, aDragSession)
    {
      var xferData = aXferData.data.split("\n");
      var draggedText = xferData[0] || xferData[1];
      var postData = {};
      var url = getShortcutOrURI(draggedText, postData);
      if (url) {
        getBrowser().dragDropSecurityCheck(aEvent, aDragSession, url);
        
        openNewTabWith(url, null, postData.value, aEvent, true);
      }
    },
  getSupportedFlavours: function ()
    {
      var flavourSet = new FlavourSet();
      flavourSet.appendFlavour("text/unicode");
      flavourSet.appendFlavour("text/x-moz-url");
      flavourSet.appendFlavour("application/x-moz-file", "nsIFile");
      return flavourSet;
    }
}

var newWindowButtonObserver = {
  onDragOver: function(aEvent, aFlavour, aDragSession)
    {
      var statusTextFld = document.getElementById("statusbar-display");
      statusTextFld.label = gNavigatorBundle.getString("droponnewwindowbutton");
      aEvent.target.setAttribute("dragover", "true");
      return true;
    },
  onDragExit: function (aEvent, aDragSession)
    {
      var statusTextFld = document.getElementById("statusbar-display");
      statusTextFld.label = "";
      aEvent.target.removeAttribute("dragover");
    },
  onDrop: function (aEvent, aXferData, aDragSession)
    {
      var xferData = aXferData.data.split("\n");
      var draggedText = xferData[0] || xferData[1];
      var postData = {};
      var url = getShortcutOrURI(draggedText, postData);
      if (url) {
        getBrowser().dragDropSecurityCheck(aEvent, aDragSession, url);
        
        openNewWindowWith(url, null, postData.value, true);
      }
    },
  getSupportedFlavours: function ()
    {
      var flavourSet = new FlavourSet();
      flavourSet.appendFlavour("text/unicode");
      flavourSet.appendFlavour("text/x-moz-url");
      flavourSet.appendFlavour("application/x-moz-file", "nsIFile");
      return flavourSet;
    }
}

var goButtonObserver = {
  onDragOver: function(aEvent, aFlavour, aDragSession)
    {
      var statusTextFld = document.getElementById("statusbar-display");
      statusTextFld.label = gNavigatorBundle.getString("dropongobutton");
      aEvent.target.setAttribute("dragover", "true");
      return true;
    },
  onDragExit: function (aEvent, aDragSession)
    {
      var statusTextFld = document.getElementById("statusbar-display");
      statusTextFld.label = "";
      aEvent.target.removeAttribute("dragover");
    },
  onDrop: function (aEvent, aXferData, aDragSession)
    {
      var xferData = aXferData.data.split("\n");
      var draggedText = xferData[0] || xferData[1];
      var postData = {};
      var url = getShortcutOrURI(draggedText, postData);
      try {
        getBrowser().dragDropSecurityCheck(aEvent, aDragSession, url);
        urlSecurityCheck(url,
                         gBrowser.contentPrincipal,
                         Ci.nsIScriptSecurityManager.DISALLOW_INHERIT_PRINCIPAL);
        loadURI(url, null, postData.value, true);
      } catch (ex) {}
    },
  getSupportedFlavours: function ()
    {
      var flavourSet = new FlavourSet();
      flavourSet.appendFlavour("text/unicode");
      flavourSet.appendFlavour("text/x-moz-url");
      flavourSet.appendFlavour("application/x-moz-file", "nsIFile");
      return flavourSet;
    }
}

var DownloadsButtonDNDObserver = {
  
  
  onDragOver: function (aEvent, aFlavour, aDragSession)
  {
    var statusTextFld = document.getElementById("statusbar-display");
    statusTextFld.label = gNavigatorBundle.getString("dropondownloadsbutton");
    aDragSession.canDrop = (aFlavour.contentType == "text/x-moz-url" ||
                            aFlavour.contentType == "text/unicode");
  },

  onDragExit: function (aEvent, aDragSession)
  {
    var statusTextFld = document.getElementById("statusbar-display");
    statusTextFld.label = "";
  },

  onDrop: function (aEvent, aXferData, aDragSession)
  {
    var split = aXferData.data.split("\n");
    var url = split[0];
    if (url != aXferData.data) {  
      getBrowser().dragDropSecurityCheck(aEvent, aDragSession, url);

      var name = split[1];
      saveURL(url, name, null, true, true);
    }
  },
  getSupportedFlavours: function ()
  {
    var flavourSet = new FlavourSet();
    flavourSet.appendFlavour("text/x-moz-url");
    flavourSet.appendFlavour("text/unicode");
    return flavourSet;
  }
}

const BrowserSearch = {

  


  init: function() {
    gBrowser.addEventListener("DOMLinkAdded", 
                              function (event) { BrowserSearch.onLinkAdded(event); }, 
                              false);
  },

  



  onLinkAdded: function(event) {
    
    
    const target = event.target;
    var etype = target.type;
    const searchRelRegex = /(^|\s)search($|\s)/i;
    const searchHrefRegex = /^(https?|ftp):\/\//i;

    if (!etype)
      return;
      
    
    
    if (!target.title)
      return;

    if (etype == "application/opensearchdescription+xml" &&
        searchRelRegex.test(target.rel) && searchHrefRegex.test(target.href))
    {
      const targetDoc = target.ownerDocument;
      
      var searchButton = document.getAnonymousElementByAttribute(this.getSearchBar(),
                                  "anonid", "searchbar-engine-button");
      if (searchButton) {
        var browser = gBrowser.getBrowserForDocument(targetDoc);
         
        var iconURL = null;
        if (gBrowser.shouldLoadFavIcon(browser.currentURI))
          iconURL = browser.currentURI.prePath + "/favicon.ico";

        var hidden = false;
        
        
        
        
         var searchService =
            Components.classes["@mozilla.org/browser/search-service;1"]
                      .getService(Components.interfaces.nsIBrowserSearchService);
        if (searchService.getEngineByName(target.title))
          hidden = true;

        var engines = [];
        if (hidden) {
          if (browser.hiddenEngines)
            engines = browser.hiddenEngines;
        }
        else {
          if (browser.engines)
            engines = browser.engines;
        }

        engines.push({ uri: target.href,
                       title: target.title,
                       icon: iconURL });

         if (hidden) {
           browser.hiddenEngines = engines;
         }
         else {
           browser.engines = engines;
           if (browser == gBrowser || browser == gBrowser.mCurrentBrowser)
             this.updateSearchButton();
         }
      }
    }
  },

  




  updateSearchButton: function() {
    var searchButton = document.getAnonymousElementByAttribute(this.getSearchBar(),
                                "anonid", "searchbar-engine-button");
    if (!searchButton)
      return;
    var engines = gBrowser.mCurrentBrowser.engines;
    if (!engines || engines.length == 0) {
      if (searchButton.hasAttribute("addengines"))
        searchButton.removeAttribute("addengines");
    }
    else {
      searchButton.setAttribute("addengines", "true");
    }
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
    var searchBar = this.getSearchBar();
    if (searchBar) {
      searchBar.select();
      searchBar.focus();
    } else {
      var ss = Cc["@mozilla.org/browser/search-service;1"].
               getService(Ci.nsIBrowserSearchService);
      var searchForm = ss.defaultEngine.searchForm;
      loadURI(searchForm, null, null, false);
    }
  },

  










  loadSearch: function BrowserSearch_search(searchText, useNewTab) {
    var ss = Cc["@mozilla.org/browser/search-service;1"].
             getService(Ci.nsIBrowserSearchService);
    var engine;
  
    
    
    if (this.getSearchBar())
      engine = ss.currentEngine;
    else
      engine = ss.defaultEngine;
  
    var submission = engine.getSubmission(searchText, null); 

    
    
    
    
    if (!submission)
      return;
  
    if (useNewTab) {
      getBrowser().loadOneTab(submission.uri.spec, null, null,
                              submission.postData, null, false);
    } else
      loadURI(submission.uri.spec, null, submission.postData, false);
  },

  



  getSearchBar: function BrowserSearch_getSearchBar() {
    var searchBar = document.getElementById("searchbar");
    if (searchBar && isElementVisible(searchBar))
      return searchBar;

    return null;
  },

  loadAddEngines: function BrowserSearch_loadAddEngines() {
    var newWindowPref = gPrefService.getIntPref("browser.link.open_newwindow");
    var where = newWindowPref == 3 ? "tab" : "window";
    var regionBundle = document.getElementById("bundle_browser_region");
    var searchEnginesURL = formatURL("browser.search.searchEnginesURL", true);
    openUILinkIn(searchEnginesURL, where);
  }
}

function FillHistoryMenu(aParent, aMenu)
  {
    
    deleteHistoryItems(aParent);

    var webNav = getWebNavigation();
    var sessionHistory = webNav.sessionHistory;

    var count = sessionHistory.count;
    var index = sessionHistory.index;
    var end;
    var j;
    var entry;

    switch (aMenu)
      {
        case "back":
          end = (index > MAX_HISTORY_MENU_ITEMS) ? index - MAX_HISTORY_MENU_ITEMS : 0;
          if ((index - 1) < end) return false;
          for (j = index - 1; j >= end; j--)
            {
              entry = sessionHistory.getEntryAtIndex(j, false);
              if (entry)
                createMenuItem(aParent, j, entry.title);
            }
          break;
        case "forward":
          end  = ((count-index) > MAX_HISTORY_MENU_ITEMS) ? index + MAX_HISTORY_MENU_ITEMS : count - 1;
          if ((index + 1) > end) return false;
          for (j = index + 1; j <= end; j++)
            {
              entry = sessionHistory.getEntryAtIndex(j, false);
              if (entry)
                createMenuItem(aParent, j, entry.title);
            }
          break;
      }

    return true;
  }

function addToUrlbarHistory(aUrlToAdd)
{
  if (!aUrlToAdd)
     return;
  if (aUrlToAdd.search(/[\x00-\x1F]/) != -1) 
     return;

  if (!gGlobalHistory)
    gGlobalHistory = Components.classes["@mozilla.org/browser/global-history;2"]
                               .getService(Components.interfaces.nsIBrowserHistory);

  if (!gURIFixup)
    gURIFixup = Components.classes["@mozilla.org/docshell/urifixup;1"]
                          .getService(Components.interfaces.nsIURIFixup);
   try {
     if (aUrlToAdd.indexOf(" ") == -1) {
       var fixedUpURI = gURIFixup.createFixupURI(aUrlToAdd, 0);
       gGlobalHistory.markPageAsTyped(fixedUpURI);
     }
   }
   catch(ex) {
   }
}

function createMenuItem( aParent, aIndex, aLabel)
  {
    var menuitem = document.createElement( "menuitem" );
    menuitem.setAttribute( "label", aLabel );
    menuitem.setAttribute( "index", aIndex );
    aParent.appendChild( menuitem );
  }

function deleteHistoryItems(aParent)
{
  var children = aParent.childNodes;
  for (var i = children.length - 1; i >= 0; --i)
    {
      var index = children[i].getAttribute("index");
      if (index)
        aParent.removeChild(children[i]);
    }
}

function toJavaScriptConsole()
{
  toOpenWindowByType("global:console", "chrome://global/content/console.xul");
}

function toOpenWindowByType(inType, uri, features)
{
  var windowManager = Components.classes['@mozilla.org/appshell/window-mediator;1'].getService();
  var windowManagerInterface = windowManager.QueryInterface(Components.interfaces.nsIWindowMediator);
  var topWindow = windowManagerInterface.getMostRecentWindow(inType);

  if (topWindow)
    topWindow.focus();
  else if (features)
    window.open(uri, "_blank", features);
  else
    window.open(uri, "_blank", "chrome,extrachrome,menubar,resizable,scrollbars,status,toolbar");
}

function toOpenDialogByTypeAndUrl(inType, relatedUrl, windowUri, features, extraArgument)
{
  var windowManager = Components.classes['@mozilla.org/appshell/window-mediator;1'].getService();
  var windowManagerInterface = windowManager.QueryInterface(Components.interfaces.nsIWindowMediator);
  var windows = windowManagerInterface.getEnumerator(inType);

  
  while (windows.hasMoreElements()) {
    var currentWindow = windows.getNext();
    if (currentWindow.document.documentElement.getAttribute("relatedUrl") == relatedUrl) {
    	currentWindow.focus();
    	return;
    }
  }

  
  if (features)
    window.openDialog(windowUri, "_blank", features, extraArgument);
  else
    window.openDialog(windowUri, "_blank", "chrome,extrachrome,menubar,resizable,scrollbars,status,toolbar", extraArgument);
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

function BrowserCustomizeToolbar()
{
  
  var menubar = document.getElementById("main-menubar");
  for (var i = 0; i < menubar.childNodes.length; ++i)
    menubar.childNodes[i].setAttribute("disabled", true);

  var cmd = document.getElementById("cmd_CustomizeToolbars");
  cmd.setAttribute("disabled", "true");

#ifdef TOOLBAR_CUSTOMIZATION_SHEET
  document.getElementById("customizeToolbarSheetBox").hidden = false;

  




  function TabOpenSheetHandler(aEvent) {
    getBrowser().removeEventListener("TabOpen", TabOpenSheetHandler, false);

    document.getElementById("customizeToolbarSheetIFrame")
            .contentWindow.gCustomizeToolbarSheet.done();
    document.getElementById("customizeToolbarSheetBox").hidden = true;
    BrowserCustomizeToolbar();
    
  }
  getBrowser().addEventListener("TabOpen", TabOpenSheetHandler, false);
#else
  window.openDialog("chrome://global/content/customizeToolbar.xul",
                    "CustomizeToolbar",
                    "chrome,all,dependent",
                    document.getElementById("navigator-toolbox"));
#endif
}

function BrowserToolboxCustomizeDone(aToolboxChanged)
{
#ifdef TOOLBAR_CUSTOMIZATION_SHEET
  document.getElementById("customizeToolbarSheetBox").hidden = true;
#endif

  
  if (aToolboxChanged) {
    gURLBar = document.getElementById("urlbar");
    if (gURLBar)
      gURLBar.clickSelectsAll = gClickSelectsAll;
    gProxyButton = document.getElementById("page-proxy-button");
    gProxyFavIcon = document.getElementById("page-proxy-favicon");
    gProxyDeck = document.getElementById("page-proxy-deck");
    gHomeButton.updateTooltip();
    window.XULBrowserWindow.init();
  }

  
  var url = getWebNavigation().currentURI.spec;
  if (gURLBar) {
    gURLBar.value = url == "about:blank" ? "" : url;
    SetPageProxyState("valid");
    XULBrowserWindow.asyncUpdateUI();    
  }

  
  var menubar = document.getElementById("main-menubar");
  for (var i = 0; i < menubar.childNodes.length; ++i)
    menubar.childNodes[i].setAttribute("disabled", false);
  var cmd = document.getElementById("cmd_CustomizeToolbars");
  cmd.removeAttribute("disabled");

  
  
  var reloadButton = document.getElementById("reload-button");
  if (reloadButton) {
    reloadButton.disabled =
      document.getElementById("Browser:Reload").getAttribute("disabled") == "true";
  }

#ifdef XP_MACOSX
  
  if (!getBoolPref("ui.click_hold_context_menus", false))
    SetClickAndHoldHandlers();
#endif

#ifndef MOZ_PLACES_BOOKMARKS
  
  var bt = document.getElementById("bookmarks-ptf");
  if (bt) {
    var btf = BMSVC.getBookmarksToolbarFolder().Value;
    var btchevron = document.getElementById("bookmarks-chevron");
    bt.ref = btf;
    btchevron.ref = btf;
    
    try {
      bt.database.RemoveObserver(BookmarksToolbarRDFObserver);
    } catch (ex) {
      
    }
    bt.database.AddObserver(BookmarksToolbarRDFObserver);
    bt.builder.rebuild();
    btchevron.builder.rebuild();

    
    
    BookmarksToolbar.resizeFunc(null);
  }
#endif

#ifndef TOOLBAR_CUSTOMIZATION_SHEET
  
  window.focus();
#endif
}

var FullScreen =
{
  toggle: function()
  {
    
    this.showXULChrome("toolbar", window.fullScreen);
    this.showXULChrome("statusbar", window.fullScreen);
    document.getElementById("fullScreenItem").setAttribute("checked", !window.fullScreen);
  },

  showXULChrome: function(aTag, aShow)
  {
    var XULNS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
    var els = document.getElementsByTagNameNS(XULNS, aTag);

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
          els[i].removeAttribute("context");

          
          
          els[i].setAttribute("inFullscreen", true);
        }
        else {
          function restoreAttr(attrName) {
            var savedAttr = "saved-" + attrName;
            if (els[i].hasAttribute(savedAttr)) {
              var savedValue = els[i].getAttribute(savedAttr);
              els[i].setAttribute(attrName, savedValue);
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

    var toolbox = document.getElementById("navigator-toolbox");
    if (aShow)
      toolbox.removeAttribute("inFullscreen");
    else
      toolbox.setAttribute("inFullscreen", true);

#ifndef XP_MACOSX
    var controls = document.getElementsByAttribute("fullscreencontrol", "true");
    for (var i = 0; i < controls.length; ++i)
      controls[i].hidden = aShow;
#endif
  }
};

function nsBrowserStatusHandler()
{
  this.init();
}

nsBrowserStatusHandler.prototype =
{
  
  status : "",
  defaultStatus : "",
  jsStatus : "",
  jsDefaultStatus : "",
  overLink : "",
  startTime : 0,
  statusText: "",
  lastURI: null,

  statusTimeoutInEffect : false,

  QueryInterface : function(aIID)
  {
    if (aIID.equals(Ci.nsIWebProgressListener) ||
        aIID.equals(Ci.nsIWebProgressListener2) ||
        aIID.equals(Ci.nsISupportsWeakReference) ||
        aIID.equals(Ci.nsIXULBrowserWindow) ||
        aIID.equals(Ci.nsISupports))
      return this;
    throw Cr.NS_NOINTERFACE;
  },

  init : function()
  {
    this.throbberElement        = document.getElementById("navigator-throbber");
    this.statusMeter            = document.getElementById("statusbar-icon");
    this.stopCommand            = document.getElementById("Browser:Stop");
    this.reloadCommand          = document.getElementById("Browser:Reload");
    this.reloadSkipCacheCommand = document.getElementById("Browser:ReloadSkipCache");
    this.statusTextField        = document.getElementById("statusbar-display");
    this.securityButton         = document.getElementById("security-button");
    this.urlBar                 = document.getElementById("urlbar");
    this.isImage                = document.getElementById("isImage");

    
    var securityUI = getBrowser().securityUI;
    this.onSecurityChange(null, null, securityUI.state);
  },

  destroy : function()
  {
    
    this.throbberElement        = null;
    this.statusMeter            = null;
    this.stopCommand            = null;
    this.reloadCommand          = null;
    this.reloadSkipCacheCommand = null;
    this.statusTextField        = null;
    this.securityButton         = null;
    this.urlBar                 = null;
    this.statusText             = null;
    this.lastURI                = null;
  },

  setJSStatus : function(status)
  {
    this.jsStatus = status;
    this.updateStatusField();
  },

  setJSDefaultStatus : function(status)
  {
    this.jsDefaultStatus = status;
    this.updateStatusField();
  },

  setDefaultStatus : function(status)
  {
    this.defaultStatus = status;
    this.updateStatusField();
  },

  setOverLink : function(link, b)
  {
    this.overLink = link;
    this.updateStatusField();
  },

  updateStatusField : function()
  {
    var text = this.overLink || this.status || this.jsStatus || this.jsDefaultStatus || this.defaultStatus;

    
    
    if (this.statusText != text) {
      this.statusTextField.label = text;
      this.statusText = text;
    }
  },
  
  mimeTypeIsTextBased : function(contentType)
  {
    return /^text\/|\+xml$/.test(contentType) ||
           contentType == "application/x-javascript" ||
           contentType == "application/xml" ||
           contentType == "mozilla.application/cached-xul";
  },

  onLinkIconAvailable : function(aBrowser)
  {
    if (gProxyFavIcon &&
        gBrowser.mCurrentBrowser == aBrowser &&
        gBrowser.userTypedValue === null)
    {
      
      PageProxySetIcon(aBrowser.mIconURL);
    }

#ifdef MOZ_PLACES_BOOKMARKS
    
    if (aBrowser.mIconURL) {
      var faviconService = Components.classes["@mozilla.org/browser/favicon-service;1"]
        .getService(Components.interfaces.nsIFaviconService);
      var uri = Components.classes["@mozilla.org/network/io-service;1"]
        .getService(Components.interfaces.nsIIOService).newURI(aBrowser.mIconURL, null, null);
      faviconService.setAndLoadFaviconForPage(aBrowser.currentURI, uri, false);
    }
#endif
  },

  onProgressChange : function (aWebProgress, aRequest,
                               aCurSelfProgress, aMaxSelfProgress,
                               aCurTotalProgress, aMaxTotalProgress)
  {
    if (aMaxTotalProgress > 0) {
      
      
      
      var percentage = (aCurTotalProgress * 100) / aMaxTotalProgress;
      this.statusMeter.value = percentage;
    }
  },

  onProgressChange64 : function (aWebProgress, aRequest,
                                 aCurSelfProgress, aMaxSelfProgress,
                                 aCurTotalProgress, aMaxTotalProgress)
  {
    return this.onProgressChange(aWebProgress, aRequest,
      aCurSelfProgress, aMaxSelfProgress, aCurTotalProgress,
      aMaxTotalProgress);
  },

  onStateChange : function(aWebProgress, aRequest, aStateFlags, aStatus)
  {
    const nsIWebProgressListener = Components.interfaces.nsIWebProgressListener;
    const nsIChannel = Components.interfaces.nsIChannel;
    if (aStateFlags & nsIWebProgressListener.STATE_START) {
        
        
        

        
        if (aStateFlags & nsIWebProgressListener.STATE_IS_NETWORK &&
            aRequest && aWebProgress.DOMWindow == content)
          this.startDocumentLoad(aRequest);

        if (this.throbberElement) {
          
          this.throbberElement.setAttribute("busy", "true");
        }

        
        this.statusMeter.value = 0;  
        if (gProgressCollapseTimer) {
          window.clearTimeout(gProgressCollapseTimer);
          gProgressCollapseTimer = null;
        }
        else
          this.statusMeter.parentNode.collapsed = false;

        
        this.stopCommand.removeAttribute("disabled");
    }
    else if (aStateFlags & nsIWebProgressListener.STATE_STOP) {
      if (aStateFlags & nsIWebProgressListener.STATE_IS_NETWORK) {
        if (aWebProgress.DOMWindow == content) {
          if (aRequest)
            this.endDocumentLoad(aRequest, aStatus);
          var browser = gBrowser.mCurrentBrowser;
          if (!gBrowser.mTabbedMode && !browser.mIconURL)
            gBrowser.useDefaultIcon(gBrowser.mCurrentTab);
#ifndef MOZ_PLACES_BOOKMARKS
          if (browser.mIconURL)
            BookmarksUtils.loadFavIcon(browser.currentURI.spec, browser.mIconURL);
#endif
        }
      }

      
      
      
      if (aRequest) {
        var msg = "";
          
          if (aRequest instanceof nsIChannel || "URI" in aRequest) {
            var location = aRequest.URI;

            
            if (location.scheme == "keyword" && aWebProgress.DOMWindow == content)
              getBrowser().userTypedValue = null;

            if (location.spec != "about:blank") {
              const kErrorBindingAborted = 0x804B0002;
              const kErrorNetTimeout = 0x804B000E;
              switch (aStatus) {
                case kErrorBindingAborted:
                  msg = gNavigatorBundle.getString("nv_stopped");
                  break;
                case kErrorNetTimeout:
                  msg = gNavigatorBundle.getString("nv_timeout");
                  break;
              }
            }
          }
          
          
          if (!msg && (!location || location.spec != "about:blank")) {
            msg = gNavigatorBundle.getString("nv_done");
          }
          this.status = "";
          this.setDefaultStatus(msg);

          
          if (content.document && this.mimeTypeIsTextBased(content.document.contentType))
            this.isImage.removeAttribute('disabled');
          else
            this.isImage.setAttribute('disabled', 'true');
        }

        
        gProgressCollapseTimer = window.setTimeout(
          function() {
            gProgressMeterPanel.collapsed = true;
            gProgressCollapseTimer = null;
          }, 100);

        if (this.throbberElement)
          this.throbberElement.removeAttribute("busy");

        this.stopCommand.setAttribute("disabled", "true");
    }
  },

  onLocationChange : function(aWebProgress, aRequest, aLocationURI)
  {
    var location = aLocationURI ? aLocationURI.spec : "";
 
    if (document.tooltipNode) {
      
      if (aWebProgress.DOMWindow == content) {
        document.getElementById("aHTMLTooltip").hidePopup();
        document.tooltipNode = null;
      }
      else {
        for (var tooltipWindow =
               document.tooltipNode.target.ownerDocument.defaultView;
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

    
    
    
    
    
    
    
    
    var selectedBrowser = getBrowser().selectedBrowser;
    if (selectedBrowser.lastURI) {
      var oldSpec = selectedBrowser.lastURI.spec;
      var oldIndexOfHash = oldSpec.indexOf("#");
      if (oldIndexOfHash != -1)
        oldSpec = oldSpec.substr(0, oldIndexOfHash);
      var newSpec = location;
      var newIndexOfHash = newSpec.indexOf("#");
      if (newIndexOfHash != -1)
        newSpec = newSpec.substr(0, newSpec.indexOf("#"));
      if (newSpec != oldSpec) {
        gBrowser.getNotificationBox(selectedBrowser).removeAllNotifications(true);
      }
    }
    selectedBrowser.lastURI = aLocationURI;

    
    if (content.document && this.mimeTypeIsTextBased(content.document.contentType))
      this.isImage.removeAttribute('disabled');
    else
      this.isImage.setAttribute('disabled', 'true');

    this.setOverLink("", null);

    
    
    
    

    var browser = getBrowser().selectedBrowser;
    if (aWebProgress.DOMWindow == content) {

      if ((location == "about:blank" && !content.opener) ||
           location == "") {                        
        location = "";                              
        this.reloadCommand.setAttribute("disabled", "true");
        this.reloadSkipCacheCommand.setAttribute("disabled", "true");
      } else {
        this.reloadCommand.removeAttribute("disabled");
        this.reloadSkipCacheCommand.removeAttribute("disabled");
      }

      
      if (browser.userTypedClear > 0 && aRequest)
        browser.userTypedValue = null;

      if (!gBrowser.mTabbedMode && aWebProgress.isLoadingDocument)
        gBrowser.setIcon(gBrowser.mCurrentTab, null);

      
      
      if (gURLBar) {
        var userTypedValue = browser.userTypedValue;
        if (!userTypedValue) {
          
          
          
          if (!gURIFixup)
            gURIFixup = Components.classes["@mozilla.org/docshell/urifixup;1"]
                                  .getService(Components.interfaces.nsIURIFixup);
          if (location && gURIFixup) {
            try {
              location = gURIFixup.createExposableURI(aLocationURI).spec;
            } catch (ex) {}
          }

          gURLBar.value = location;
          SetPageProxyState("valid");

          
          
          browser.userTypedValue = userTypedValue;
        } else {
          gURLBar.value = userTypedValue;
          SetPageProxyState("invalid");
        }
      }
    }
    UpdateBackForwardCommands(gBrowser.webNavigation);

    if (gFindBar.findMode != gFindBar.FIND_NORMAL) {
      
      gFindBar.close();
    }

    
    

    
    gFindBar.getElement("highlight").checked = false;

    
    
    if (aRequest) {
      var self = this;
      setTimeout(function() { self.asyncUpdateUI(); }, 0);
    } 
    else
      this.asyncUpdateUI();
  },
  
  asyncUpdateUI : function () {
    FeedHandler.updateFeeds();
    BrowserSearch.updateSearchButton();
  },

  onStatusChange : function(aWebProgress, aRequest, aStatus, aMessage)
  {
    this.status = aMessage;
    this.updateStatusField();
  },

  onRefreshAttempted : function(aWebProgress, aURI, aDelay, aSameURI)
  {
    if (gPrefService.getBoolPref("accessibility.blockautorefresh")) {
      var brandBundle = document.getElementById("bundle_brand");
      var brandShortName = brandBundle.getString("brandShortName");
      var refreshButtonText = 
        gNavigatorBundle.getString("refreshBlocked.goButton");
      var refreshButtonAccesskey = 
        gNavigatorBundle.getString("refreshBlocked.goButton.accesskey");
      var message;
      if (aSameURI)
        message = gNavigatorBundle.getFormattedString(
          "refreshBlocked.refreshLabel", [brandShortName]);
      else
        message = gNavigatorBundle.getFormattedString(
          "refreshBlocked.redirectLabel", [brandShortName]);
      var topBrowser = getBrowserFromContentWindow(aWebProgress.DOMWindow.top);
      var docShell = aWebProgress.DOMWindow
                                 .QueryInterface(Ci.nsIInterfaceRequestor)
                                 .getInterface(Ci.nsIWebNavigation)
                                 .QueryInterface(Ci.nsIDocShell);
      var notificationBox = gBrowser.getNotificationBox(topBrowser);
      var notification = notificationBox.getNotificationWithValue(
        "refresh-blocked");
      if (notification) {
        notification.label = message;
        notification.refreshURI = aURI;
        notification.delay = aDelay;
        notification.docShell = docShell;
      }
      else {
        var buttons = [{
          label: refreshButtonText,
          accessKey: refreshButtonAccesskey,
          callback: function(aNotification, aButton) {
            var refreshURI = aNotification.docShell
                                          .QueryInterface(Ci.nsIRefreshURI);
            refreshURI.forceRefreshURI(aNotification.refreshURI,
                                       aNotification.delay, true);
          }
        }];
        const priority = notificationBox.PRIORITY_INFO_MEDIUM;
        notification = notificationBox.appendNotification(
          message,
          "refresh-blocked",
          "chrome://browser/skin/Info.png",
          priority,
          buttons);
        notification.refreshURI = aURI;
        notification.delay = aDelay;
        notification.docShell = docShell;
      }
      return false;
    }
    return true;
  },

  onSecurityChange : function(aWebProgress, aRequest, aState)
  {
    const wpl = Components.interfaces.nsIWebProgressListener;
    this.securityButton.removeAttribute("label");

    switch (aState) {
      case wpl.STATE_IS_SECURE | wpl.STATE_SECURE_HIGH:
        this.securityButton.setAttribute("level", "high");
        if (this.urlBar)
          this.urlBar.setAttribute("level", "high");
        try {
          this.securityButton.setAttribute("label",
            gBrowser.contentWindow.location.host);
        } catch(exception) {}
        break;
      case wpl.STATE_IS_SECURE | wpl.STATE_SECURE_LOW:
        this.securityButton.setAttribute("level", "low");
        if (this.urlBar)
          this.urlBar.setAttribute("level", "low");
        try {
          this.securityButton.setAttribute("label",
            gBrowser.contentWindow.location.host);
        } catch(exception) {}
        break;
      case wpl.STATE_IS_BROKEN:
        this.securityButton.setAttribute("level", "broken");
        if (this.urlBar)
          this.urlBar.setAttribute("level", "broken");
        break;
      case wpl.STATE_IS_INSECURE:
      default:
        this.securityButton.removeAttribute("level");
        if (this.urlBar)
          this.urlBar.removeAttribute("level");
        break;
    }

    var securityUI = gBrowser.securityUI;
    this.securityButton.setAttribute("tooltiptext", securityUI.tooltipText);
    var lockIcon = document.getElementById("lock-icon");
    if (lockIcon)
      lockIcon.setAttribute("tooltiptext", securityUI.tooltipText);
  },

  
  onUpdateCurrentBrowser : function(aStateFlags, aStatus, aMessage, aTotalProgress)
  {
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

  startDocumentLoad : function(aRequest)
  {
    
    
    
    
    
    getBrowser().userTypedClear++;

    
    gBrowser.mCurrentBrowser.feeds = null;

    
    gBrowser.mCurrentBrowser.engines = null;    

    const nsIChannel = Components.interfaces.nsIChannel;
    var urlStr = aRequest.QueryInterface(nsIChannel).URI.spec;
    var observerService = Components.classes["@mozilla.org/observer-service;1"]
                                    .getService(Components.interfaces.nsIObserverService);
    try {
      observerService.notifyObservers(content, "StartDocumentLoad", urlStr);
    } catch (e) {
    }
  },

  endDocumentLoad : function(aRequest, aStatus)
  {
    
    
    if (getBrowser().userTypedClear > 0)
      getBrowser().userTypedClear--;

    const nsIChannel = Components.interfaces.nsIChannel;
    var urlStr = aRequest.QueryInterface(nsIChannel).originalURI.spec;

    var observerService = Components.classes["@mozilla.org/observer-service;1"]
                                    .getService(Components.interfaces.nsIObserverService);

    var notification = Components.isSuccessCode(aStatus) ? "EndDocumentLoad" : "FailDocumentLoad";
    try {
      observerService.notifyObservers(content, notification, urlStr);
    } catch (e) {
    }
  }
}

function nsBrowserAccess()
{
}

nsBrowserAccess.prototype =
{
  QueryInterface : function(aIID)
  {
    if (aIID.equals(Ci.nsIBrowserDOMWindow) ||
        aIID.equals(Ci.nsISupports))
      return this;
    throw Components.results.NS_NOINTERFACE;
  },

  openURI : function(aURI, aOpener, aWhere, aContext)
  {
    var newWindow = null;
    var referrer = null;
    var isExternal = (aContext == Ci.nsIBrowserDOMWindow.OPEN_EXTERNAL);

    if (isExternal && aURI && aURI.schemeIs("chrome")) {
      dump("use -chrome command-line option to load external chrome urls\n");
      return null;
    }

    var loadflags = isExternal ?
                       Ci.nsIWebNavigation.LOAD_FLAGS_FROM_EXTERNAL :
                       Ci.nsIWebNavigation.LOAD_FLAGS_NONE;
    var location;
    if (aWhere == Ci.nsIBrowserDOMWindow.OPEN_DEFAULTWINDOW) {
      switch (aContext) {
        case Ci.nsIBrowserDOMWindow.OPEN_EXTERNAL :
          aWhere = gPrefService.getIntPref("browser.link.open_external");
          break;
        default : 
          aWhere = gPrefService.getIntPref("browser.link.open_newwindow");
      }
    }
    var url = aURI ? aURI.spec : "about:blank";
    switch(aWhere) {
      case Ci.nsIBrowserDOMWindow.OPEN_NEWWINDOW :
        newWindow = openDialog(getBrowserURL(), "_blank", "all,dialog=no", url);
        break;
      case Ci.nsIBrowserDOMWindow.OPEN_NEWTAB :
        var loadInBackground = gPrefService.getBoolPref("browser.tabs.loadDivertedInBackground");
        var newTab = gBrowser.loadOneTab("about:blank", null, null, null, loadInBackground, false);
        newWindow = gBrowser.getBrowserForTab(newTab).docShell
                            .QueryInterface(Ci.nsIInterfaceRequestor)
                            .getInterface(Ci.nsIDOMWindow);
        try {
          if (aOpener) {
            location = aOpener.location;
            referrer =
                    Components.classes["@mozilla.org/network/io-service;1"]
                              .getService(Components.interfaces.nsIIOService)
                              .newURI(location, null, null);
          }
          newWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                   .getInterface(Ci.nsIWebNavigation)
                   .loadURI(url, loadflags, referrer, null, null);
        } catch(e) {
        }
        break;
      default : 
        try {
          if (aOpener) {
            newWindow = aOpener.top;
            location = aOpener.location;
            referrer =
                    Components.classes["@mozilla.org/network/io-service;1"]
                              .getService(Components.interfaces.nsIIOService)
                              .newURI(location, null, null);

            newWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                     .getInterface(nsIWebNavigation)
                     .loadURI(url, loadflags, referrer, null, null);
          } else {
            newWindow = gBrowser.selectedBrowser.docShell
                                .QueryInterface(Ci.nsIInterfaceRequestor)
                                .getInterface(Ci.nsIDOMWindow);
            getWebNavigation().loadURI(url, loadflags, null, null, null);
          }
          if(!gPrefService.getBoolPref("browser.tabs.loadDivertedInBackground"))
            content.focus();
        } catch(e) {
        }
    }
    return newWindow;
  },

  isTabContentWindow : function(aWindow)
  {
    var browsers = gBrowser.browsers;
    for (var ctr = 0; ctr < browsers.length; ctr++)
      if (browsers.item(ctr).contentWindow == aWindow)
        return true;
    return false;
  }
}

function onViewToolbarsPopupShowing(aEvent)
{
  var popup = aEvent.target;
  var i;

  
  for (i = popup.childNodes.length-1; i >= 0; --i) {
    var deadItem = popup.childNodes[i];
    if (deadItem.hasAttribute("toolbarindex"))
      popup.removeChild(deadItem);
  }

  var firstMenuItem = popup.firstChild;

  var toolbox = document.getElementById("navigator-toolbox");
  for (i = 0; i < toolbox.childNodes.length; ++i) {
    var toolbar = toolbox.childNodes[i];
    var toolbarName = toolbar.getAttribute("toolbarname");
    var type = toolbar.getAttribute("type");
    if (toolbarName && type != "menubar") {
      var menuItem = document.createElement("menuitem");
      menuItem.setAttribute("toolbarindex", i);
      menuItem.setAttribute("type", "checkbox");
      menuItem.setAttribute("label", toolbarName);
      menuItem.setAttribute("accesskey", toolbar.getAttribute("accesskey"));
      menuItem.setAttribute("checked", toolbar.getAttribute("collapsed") != "true");
      popup.insertBefore(menuItem, firstMenuItem);

      menuItem.addEventListener("command", onViewToolbarCommand, false);
    }
    toolbar = toolbar.nextSibling;
  }
}

function onViewToolbarCommand(aEvent)
{
  var toolbox = document.getElementById("navigator-toolbox");
  var index = aEvent.originalTarget.getAttribute("toolbarindex");
  var toolbar = toolbox.childNodes[index];

  toolbar.collapsed = aEvent.originalTarget.getAttribute("checked") != "true";
  document.persist(toolbar.id, "collapsed");
}

function displaySecurityInfo()
{
  BrowserPageInfo(null, "security");
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
      sidebarBox.hidden = true;
      sidebarSplitter.hidden = true;
      content.focus();
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
}

var gHomeButton = {
  prefDomain: "browser.startup.homepage",
  observe: function (aSubject, aTopic, aPrefName)
  {
    if (aTopic != "nsPref:changed" || aPrefName != this.prefDomain)
      return;

    this.updateTooltip();
  },

  updateTooltip: function ()
  {
    var homeButton = document.getElementById("home-button");
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
      var configBundle = SBS.createBundle("resource:/browserconfig.properties");
      url = configBundle.GetStringFromName(this.prefDomain);
    }

    return url;
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
         
         
         
         var dialogArgs = {
           name: wrapper.getAttribute("title"),
           url: wrapper.href,
           bWebPanel: true
         }
#ifndef MOZ_PLACES_BOOKMARKS
         openDialog("chrome://browser/content/bookmarks/addBookmark2.xul", "",
                    BROWSER_ADD_BM_FEATURES, dialogArgs);
         event.preventDefault();
#else
         PlacesUtils.showAddBookmarkUI(makeURI(wrapper.href),
                                       wrapper.getAttribute("title"),
                                       null, true, true);
         event.preventDefault();
#endif
         return false;
       }
       else if (target == "_search") {
         
         

         
         
         try {
           const nsIScriptSecurityMan = Ci.nsIScriptSecurityManager;
           urlSecurityCheck(wrapper.href,
                            wrapper.ownerDocument.nodePrincipal,
                            nsIScriptSecurityMan.DISALLOW_INHERIT_PRINCIPAL);
         }
         catch(ex) {
           return false;
         } 

         openWebPanel(gNavigatorBundle.getString("webPanels"), wrapper.href);
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
      var tab;
      try {
        tab = gPrefService.getBoolPref("browser.tabs.opentabfor.middleclick")
      }
      catch(ex) {
        tab = true;
      }
      if (tab)
        openNewTabWith(href, doc, null, event, false);
      else
        openNewWindowWith(href, doc, null, false);
      event.stopPropagation();
      return true;
  }
  return false;
}

function middleMousePaste(event)
{
  var url = readFromClipboard();
  if (!url)
    return;
  var postData = { };
  url = getShortcutOrURI(url, postData);
  if (!url)
    return;

  try {
    addToUrlbarHistory(url);
  } catch (ex) {
    
    
  }

  openUILink(url,
             event,
             true );

  event.stopPropagation();
}

function makeURLAbsolute( base, url )
{
  
  var ioService = Components.classes["@mozilla.org/network/io-service;1"]
                .getService(Components.interfaces.nsIIOService);
  var baseURI  = ioService.newURI(base, null, null);

  return ioService.newURI(baseURI.resolve(url), null, null).spec;
}










var contentAreaDNDObserver = {
  onDrop: function (aEvent, aXferData, aDragSession)
    {
      var url = transferUtils.retrieveURLFromData(aXferData.data, aXferData.flavour.contentType);

      
      
      
      if (!url || !url.length || url.indexOf(" ", 0) != -1 ||
          /^\s*(javascript|data):/.test(url))
        return;

      getBrowser().dragDropSecurityCheck(aEvent, aDragSession, url);

      switch (document.documentElement.getAttribute('windowtype')) {
        case "navigator:browser":
          var postData = { };
          var uri = getShortcutOrURI(url, postData);
          loadURI(uri, null, postData.value, false);
          break;
        case "navigator:view-source":
          viewSource(url);
          break;
      }

      
      
      aEvent.preventDefault();
    },

  getSupportedFlavours: function ()
    {
      var flavourSet = new FlavourSet();
      flavourSet.appendFlavour("text/x-moz-url");
      flavourSet.appendFlavour("text/unicode");
      flavourSet.appendFlavour("application/x-moz-file", "nsIFile");
      return flavourSet;
    }

};


function getBrowser()
{
  if (!gBrowser)
    gBrowser = document.getElementById("content");
  return gBrowser;
}

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
        var pref = Components.classes["@mozilla.org/preferences-service;1"]
                             .getService(Components.interfaces.nsIPrefBranch);
        var str =  Components.classes["@mozilla.org/supports-string;1"]
                             .createInstance(Components.interfaces.nsISupportsString);

        str.data = prefvalue;
        pref.setComplexValue("intl.charset.detector",
                             Components.interfaces.nsISupportsString, str);
        if (doReload) window.content.location.reload();
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
  var docCharset = getBrowser().docShell.QueryInterface(
                            Components.interfaces.nsIDocCharset);
  docCharset.charset = aCharset;
  BrowserReloadWithFlags(nsIWebNavigation.LOAD_FLAGS_CHARSET_CHANGE);
}

function BrowserSetForcedDetector(doReload)
{
  getBrowser().documentCharsetInfo.forcedDetector = true;
  if (doReload)
    BrowserReloadWithFlags(nsIWebNavigation.LOAD_FLAGS_CHARSET_CHANGE);
}

function UpdateCurrentCharset()
{
    var menuitem = null;

    
    var wnd = document.commandDispatcher.focusedWindow;
    if ((window == wnd) || (wnd == null)) wnd = window.content;
    menuitem = document.getElementById('charset.' + wnd.document.characterSet);

    if (menuitem) {
        
        
        if (gPrevCharset) {
            var pref_item = document.getElementById('charset.' + gPrevCharset);
            if (pref_item)
              pref_item.setAttribute('checked', 'false');
        }
        menuitem.setAttribute('checked', 'true');
    }
}

function UpdateCharsetDetector()
{
    var prefvalue;

    try {
        var pref = Components.classes["@mozilla.org/preferences-service;1"]
                             .getService(Components.interfaces.nsIPrefBranch);
        prefvalue = pref.getComplexValue("intl.charset.detector",
                                         Components.interfaces.nsIPrefLocalizedString).data;
    }
    catch (ex) {
        prefvalue = "";
    }

    if (prefvalue == "") prefvalue = "off";
    dump("intl.charset.detector = "+ prefvalue + "\n");

    prefvalue = 'chardet.' + prefvalue;
    var menuitem = document.getElementById(prefvalue);

    if (menuitem) {
        menuitem.setAttribute('checked', 'true');
    }
}

function UpdateMenus(event)
{
    
    
    
    UpdateCurrentCharset();
    setTimeout(UpdateCurrentCharset, 0);
    UpdateCharsetDetector();
    setTimeout(UpdateCharsetDetector, 0);
}

function CreateMenu(node)
{
  var observerService = Components.classes["@mozilla.org/observer-service;1"].getService(Components.interfaces.nsIObserverService);
  observerService.notifyObservers(null, "charsetmenu-selected", node);
}

function charsetLoadListener (event)
{
    var charset = window.content.document.characterSet;

    if (charset.length > 0 && (charset != gLastBrowserCharset)) {
        if (!gCharsetMenu)
          gCharsetMenu = Components.classes['@mozilla.org/rdf/datasource;1?name=charset-menu'].getService().QueryInterface(Components.interfaces.nsICurrentCharsetListener);
        gCharsetMenu.SetCurrentCharset(charset);
        gPrevCharset = gLastBrowserCharset;
        gLastBrowserCharset = charset;
    }
}


function getStyleSheetArray(frame)
{
  var styleSheets = frame.document.styleSheets;
  var styleSheetsArray = new Array(styleSheets.length);
  for (var i = 0; i < styleSheets.length; i++) {
    styleSheetsArray[i] = styleSheets[i];
  }
  return styleSheetsArray;
}

function getAllStyleSheets(frameset)
{
  var styleSheetsArray = getStyleSheetArray(frameset);
  for (var i = 0; i < frameset.frames.length; i++) {
    var frameSheets = getAllStyleSheets(frameset.frames[i]);
    styleSheetsArray = styleSheetsArray.concat(frameSheets);
  }
  return styleSheetsArray;
}

function stylesheetFillPopup(menuPopup)
{
  var noStyle = menuPopup.firstChild;
  var persistentOnly = noStyle.nextSibling;
  var sep = persistentOnly.nextSibling;
  while (sep.nextSibling)
    menuPopup.removeChild(sep.nextSibling);

  var styleSheets = getAllStyleSheets(window.content);
  var currentStyleSheets = [];
  var styleDisabled = getMarkupDocumentViewer().authorStyleDisabled;
  var haveAltSheets = false;
  var altStyleSelected = false;

  for (var i = 0; i < styleSheets.length; ++i) {
    var currentStyleSheet = styleSheets[i];

    
    var media = currentStyleSheet.media.mediaText.toLowerCase();
    if (media && (media.indexOf("screen") == -1) && (media.indexOf("all") == -1))
        continue;

    if (currentStyleSheet.title) {
      if (!currentStyleSheet.disabled)
        altStyleSelected = true;

      haveAltSheets = true;

      var lastWithSameTitle = null;
      if (currentStyleSheet.title in currentStyleSheets)
        lastWithSameTitle = currentStyleSheets[currentStyleSheet.title];

      if (!lastWithSameTitle) {
        var menuItem = document.createElement("menuitem");
        menuItem.setAttribute("type", "radio");
        menuItem.setAttribute("label", currentStyleSheet.title);
        menuItem.setAttribute("data", currentStyleSheet.title);
        menuItem.setAttribute("checked", !currentStyleSheet.disabled && !styleDisabled);
        menuPopup.appendChild(menuItem);
        currentStyleSheets[currentStyleSheet.title] = menuItem;
      } else {
        if (currentStyleSheet.disabled)
          lastWithSameTitle.removeAttribute("checked");
      }
    }
  }

  noStyle.setAttribute("checked", styleDisabled);
  persistentOnly.setAttribute("checked", !altStyleSelected && !styleDisabled);
  persistentOnly.hidden = (window.content.document.preferredStyleSheetSet) ? haveAltSheets : false;
  sep.hidden = (noStyle.hidden && persistentOnly.hidden) || !haveAltSheets;
  return true;
}

function stylesheetInFrame(frame, title) {
  var docStyleSheets = frame.document.styleSheets;

  for (var i = 0; i < docStyleSheets.length; ++i) {
    if (docStyleSheets[i].title == title)
      return true;
  }
  return false;
}

function stylesheetSwitchFrame(frame, title) {
  var docStyleSheets = frame.document.styleSheets;

  for (var i = 0; i < docStyleSheets.length; ++i) {
    var docStyleSheet = docStyleSheets[i];

    if (title == "_nostyle")
      docStyleSheet.disabled = true;
    else if (docStyleSheet.title)
      docStyleSheet.disabled = (docStyleSheet.title != title);
    else if (docStyleSheet.disabled)
      docStyleSheet.disabled = false;
  }
}

function stylesheetSwitchAll(frameset, title) {
  if (!title || title == "_nostyle" || stylesheetInFrame(frameset, title)) {
    stylesheetSwitchFrame(frameset, title);
  }
  for (var i = 0; i < frameset.frames.length; i++) {
    stylesheetSwitchAll(frameset.frames[i], title);
  }
}

function setStyleDisabled(disabled) {
  getMarkupDocumentViewer().authorStyleDisabled = disabled;
}



var BrowserOffline = {
  
  
  init: function ()
  {
    if (!this._uiElement)
      this._uiElement = document.getElementById("goOfflineMenuitem");

    var os = Components.classes["@mozilla.org/observer-service;1"].getService(Components.interfaces.nsIObserverService);
    os.addObserver(this, "network:offline-status-changed", false);

    var ioService = Components.classes["@mozilla.org/network/io-service;1"].
      getService(Components.interfaces.nsIIOService2);

    
    
    
    
    if (!ioService.manageOfflineStatus) {
      
      var isOffline = false;
      try {
        isOffline = gPrefService.getBoolPref("browser.offline");
      }
      catch (e) { }
      ioService.offline = isOffline;
    }
    
    this._updateOfflineUI(ioService.offline);
  },

  uninit: function ()
  {
    try {
      var os = Components.classes["@mozilla.org/observer-service;1"].getService(Components.interfaces.nsIObserverService);
      os.removeObserver(this, "network:offline-status-changed");
    } catch (ex) {
    }
  },

  toggleOfflineStatus: function ()
  {
    var ioService = Components.classes["@mozilla.org/network/io-service;1"].
      getService(Components.interfaces.nsIIOService2);

    
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
    var os = Components.classes["@mozilla.org/observer-service;1"].getService(Components.interfaces.nsIObserverService);
    if (os) {
      try {
        var cancelGoOffline = Components.classes["@mozilla.org/supports-PRBool;1"].createInstance(Components.interfaces.nsISupportsPRBool);
        os.notifyObservers(cancelGoOffline, "offline-requested", null);

        
        if (cancelGoOffline.data)
          return false;
      }
      catch (ex) {
      }
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

function WindowIsClosing()
{
  var browser = getBrowser();
  var cn = browser.tabContainer.childNodes;
  var numtabs = cn.length;
  var reallyClose = browser.warnAboutClosingTabs(true);

  for (var i = 0; reallyClose && i < numtabs; ++i) {
    var ds = browser.getBrowserForTab(cn[i]).docShell;

    if (ds.contentViewer && !ds.contentViewer.permitUnload())
      reallyClose = false;
  }

  if (reallyClose)
    return closeWindow(false);

  return reallyClose;
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

    var ioService = Components.classes["@mozilla.org/network/io-service;1"]
                              .getService(Components.interfaces.nsIIOService);
    var uri = ioService.newURI(mailtoUrl, null, null);

    
    this._launchExternalUrl(uri);
  },

  
  
  
  _launchExternalUrl: function (aURL) {
    var extProtocolSvc =
       Components.classes["@mozilla.org/uriloader/external-protocol-service;1"]
                 .getService(Components.interfaces.nsIExternalProtocolService);
    if (extProtocolSvc)
      extProtocolSvc.loadUrl(aURL);
  }
};

function BrowserOpenAddonsMgr()
{
  const EMTYPE = "Extension:Manager";
  var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                     .getService(Components.interfaces.nsIWindowMediator);
  var theEM = wm.getMostRecentWindow(EMTYPE);
  if (theEM) {
    theEM.focus();
    return;
  }

  const EMURL = "chrome://mozapps/content/extensions/extensions.xul";
  const EMFEATURES = "chrome,menubar,extra-chrome,toolbar,dialog=no,resizable";
  window.openDialog(EMURL, "", EMFEATURES);
}

function escapeNameValuePair(aName, aValue, aIsFormUrlEncoded)
{
  if (aIsFormUrlEncoded)
    return escape(aName + "=" + aValue);
  else
    return escape(aName) + "=" + escape(aValue);
}

function AddKeywordForSearchField()
{
  var node = document.popupNode;

  var docURI = makeURI(node.ownerDocument.URL,
                       node.ownerDocument.characterSet);

  var formURI = makeURI(node.form.getAttribute("action"),
                        node.ownerDocument.characterSet,
                        docURI);

  var spec = formURI.spec;

  var isURLEncoded = 
               (node.form.method.toUpperCase() == "POST"
                && (node.form.enctype == "application/x-www-form-urlencoded" ||
                    node.form.enctype == ""));

  var el, type;
  var formData = [];

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
    
    if ((type == "text" || type == "hidden" || type == "textarea") ||
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

#ifndef MOZ_PLACES_BOOKMARKS
  var dialogArgs = {
    name: "",
    url: spec,
    charset: node.ownerDocument.characterSet,
    bWebPanel: false,
    keyword: "",
    bNeedKeyword: true,
    postData: postData,
    description: BookmarksUtils.getDescriptionFromDocument(node.ownerDocument)
  }
  openDialog("chrome://browser/content/bookmarks/addBookmark2.xul", "",
             BROWSER_ADD_BM_FEATURES, dialogArgs);
#else
  var description = PlacesUtils.getDescriptionFromDocument(node.ownerDocument);
  PlacesUtils.showAddBookmarkUI(makeURI(spec), "", description, null, null, null, "");
#endif
}

function SwitchDocumentDirection(aWindow) {
  aWindow.document.dir = (aWindow.document.dir == "ltr" ? "rtl" : "ltr");
  for (var run = 0; run < aWindow.frames.length; run++)
    SwitchDocumentDirection(aWindow.frames[run]);
}

function missingPluginInstaller(){
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
      var docShell = findChildShell(doc, gBrowser.selectedBrowser.docShell, null);
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

missingPluginInstaller.prototype.installSinglePlugin = function(aEvent){
  var tabbrowser = getBrowser();
  var missingPluginsArray = new Object;

  var pluginInfo = getPluginInfo(aEvent.target);
  missingPluginsArray[pluginInfo.mimetype] = pluginInfo;

  if (missingPluginsArray) {
    window.openDialog("chrome://mozapps/content/plugins/pluginInstallerWizard.xul",
                      "PFSWindow", "modal,chrome,resizable=yes",
                      {plugins: missingPluginsArray, tab: tabbrowser.mCurrentTab});
  }

  aEvent.preventDefault();
}

missingPluginInstaller.prototype.newMissingPlugin = function(aEvent){
  
  
  if (!(aEvent.target instanceof Components.interfaces.nsIObjectLoadingContent))
    return;

  
  
  
  

  if (!(aEvent.target instanceof HTMLObjectElement)) {
    aEvent.target.addEventListener("click",
                                   gMissingPluginInstaller.installSinglePlugin,
                                   false);
  }

  var tabbrowser = getBrowser();
  const browsers = tabbrowser.mPanelContainer.childNodes;

  var window = aEvent.target.ownerDocument.defaultView;
  
  while (window.parent != window)
    window = window.parent;

  var i = 0;
  for (; i < browsers.length; i++) {
    if (tabbrowser.getBrowserAtIndex(i).contentWindow == window)
      break;
  }

  var tab = tabbrowser.mTabContainer.childNodes[i];
  if (!tab.missingPlugins)
    tab.missingPlugins = new Object();

  var pluginInfo = getPluginInfo(aEvent.target);

  tab.missingPlugins[pluginInfo.mimetype] = pluginInfo;

  var browser = tabbrowser.getBrowserAtIndex(i);
  var notificationBox = gBrowser.getNotificationBox(browser);
  if (!notificationBox.getNotificationWithValue("missing-plugins")) {
    var bundle_browser = document.getElementById("bundle_browser");
    var messageString = bundle_browser.getString("missingpluginsMessage.title");
    var buttons = [{
      label: bundle_browser.getString("missingpluginsMessage.button.label"),
      accessKey: bundle_browser.getString("missingpluginsMessage.button.accesskey"),
      popup: null,
      callback: pluginsMissing
    }];

    const priority = notificationBox.PRIORITY_WARNING_MEDIUM;
    const iconURL = "chrome://mozapps/skin/xpinstall/xpinstallItemGeneric.png";
    notificationBox.appendNotification(messageString, "missing-plugins",
                                       iconURL, priority, buttons);
  }
}

missingPluginInstaller.prototype.closeNotification = function() {
  var notificationBox = gBrowser.getNotificationBox();
  var notification = notificationBox.getNotificationWithValue("missing-plugins");

  if (notification) {
    notificationBox.removeNotification(notification);
  }
}

function pluginsMissing()
{
  
  var tabbrowser = getBrowser();
  var missingPluginsArray = tabbrowser.mCurrentTab.missingPlugins;
  if (missingPluginsArray) {
    window.openDialog("chrome://mozapps/content/plugins/pluginInstallerWizard.xul",
      "PFSWindow", "modal,chrome,resizable=yes", {plugins: missingPluginsArray, tab: tabbrowser.mCurrentTab});
  }
}

var gMissingPluginInstaller = new missingPluginInstaller();

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
  


  init: function() {
    gBrowser.addEventListener("DOMLinkAdded", 
                              function (event) { FeedHandler.onLinkAdded(event); }, 
                              true);
    gBrowser.addEventListener("pagehide", FeedHandler.onPageHide, true);
  },
  
  onPageHide: function(event) {
    var theBrowser = gBrowser.getBrowserForDocument(event.target);
    if (theBrowser)
      theBrowser.feeds = null;
  },
  
  





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
      menuItem.setAttribute("label", labelStr);
      menuItem.setAttribute("feed", feedInfo.href);
      menuItem.setAttribute("tooltiptext", feedInfo.href);
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
    
  





  _getContentShell: function(doc) {
    var browsers = getBrowser().browsers;
    for (var i = 0; i < browsers.length; i++) {
      var shell = findChildShell(doc, browsers[i].docShell, null);
      if (shell)
        return { shell: shell, browser: browsers[i] };
    }
    return null;
  },
  
#ifndef MOZ_PLACES_BOOKMARKS
  








  addLiveBookmark: function(url, feedTitle, feedSubtitle) {
    var doc = gBrowser.selectedBrowser.contentDocument;
    var title = (arguments.length > 1) ? feedTitle : doc.title;
    var description;
    if (arguments.length > 2)
      description = feedSubtitle;
    else
      description = BookmarksUtils.getDescriptionFromDocument(doc);
    BookmarksUtils.addLivemark(doc.baseURI, url, title, description);
  },
#endif

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

    var feeds = gBrowser.mCurrentBrowser.feeds;
    if (!feeds || feeds.length == 0) {
      if (feedButton) {
        feedButton.removeAttribute("feeds");
        feedButton.removeAttribute("feed");
        feedButton.setAttribute("tooltiptext", 
                                gNavigatorBundle.getString("feedNoFeeds"));
      }
      this._feedMenuitem.setAttribute("disabled", "true");
      this._feedMenupopup.setAttribute("hidden", "true");
      this._feedMenuitem.removeAttribute("hidden");
    } else {
      if (feedButton) {
        feedButton.setAttribute("feeds", "true");
        feedButton.setAttribute("tooltiptext", 
                                gNavigatorBundle.getString("feedHasFeedsNew"));
      }
      
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
  
  



  onLinkAdded: function(event) {
    
    
    
    
    
    

    var erel = event.target.rel;
    var etype = event.target.type;
    var etitle = event.target.title;
    const rssTitleRegex = /(^|\s)rss($|\s)/i;
    var rels = {};

    if (erel) {
      for each (var relValue in erel.split(/\s+/))
        rels[relValue] = true;
    }
    var isFeed = rels.feed;

    if (!isFeed &&
        (!rels.alternate || rels.stylesheet || !etype))
      return;

    if (!isFeed) {
      
      etype = etype.replace(/^\s+/, "");
      etype = etype.replace(/\s+$/, "");
      etype = etype.replace(/\s*;.*/, "");
      etype = etype.toLowerCase();
      isFeed = (etype == "application/rss+xml" ||
                etype == "application/atom+xml");
      if (!isFeed) {
        
        isFeed = ((etype == "text/xml" || etype == "application/xml" ||
                   etype == "application/rdf+xml") && rssTitleRegex.test(etitle));
      }
    }

    if (isFeed) {
      const targetDoc = event.target.ownerDocument;

      
      var browserForLink = gBrowser.getBrowserForDocument(targetDoc);
      if (!browserForLink) {
        
        return;
      }

      var feeds = [];
      if (browserForLink.feeds != null)
        feeds = browserForLink.feeds;
      var wrapper = event.target;

      try { 
        urlSecurityCheck(wrapper.href,
                         gBrowser.contentPrincipal,
                         Ci.nsIScriptSecurityManager.DISALLOW_INHERIT_PRINCIPAL);
      }
      catch (ex) {
        dump(ex.message);
        return; 
      }

      feeds.push({ href: wrapper.href,
                   type: etype,
                   title: wrapper.title});
      browserForLink.feeds = feeds;
      if (browserForLink == gBrowser || browserForLink == gBrowser.mCurrentBrowser) {
        var feedButton = document.getElementById("feed-button");
        if (feedButton) {
          feedButton.setAttribute("feeds", "true");
          feedButton.setAttribute("tooltiptext", 
                                  gNavigatorBundle.getString("feedHasFeedsNew"));
        }
      }
    }
  }
};

#ifdef MOZ_PLACES
#include browser-places.js
#endif




var AugmentTabs = {

  tabContextMenu: null,
  undoCloseTabMenu: null,

  


  init: function at_init() {
    
    var tabbrowser = getBrowser();
    this.tabContextMenu = document.getAnonymousElementByAttribute(tabbrowser, "anonid", "tabContextMenu");

    
    this.tabContextMenu.addEventListener("popupshowing", this.onTabContextMenuLoad, false);

    if (gPrefService.getBoolPref("browser.sessionstore.enabled"))
      this._addUndoCloseTabContextMenu();
  },

  


  _addUndoCloseTabContextMenu: function at_addUndoCloseTabContextMenu() {
    
    var menuLabel = gNavigatorBundle.getString("tabContext.undoCloseTab");
    var menuAccessKey = gNavigatorBundle.getString("tabContext.undoCloseTabAccessKey");

    
    var undoCloseTabItem = document.createElement("menuitem");
    undoCloseTabItem.setAttribute("id", "tabContextUndoCloseTab");
    undoCloseTabItem.setAttribute("label", menuLabel);
    undoCloseTabItem.setAttribute("accesskey", menuAccessKey);
    undoCloseTabItem.setAttribute("command", "History:UndoCloseTab");

    
    var insertPos = this.tabContextMenu.lastChild.previousSibling;
    this.undoCloseTabMenu = this.tabContextMenu.insertBefore(undoCloseTabItem, insertPos);
  },

  onTabContextMenuLoad: function at_onTabContextMenuLoad() {
    if (AugmentTabs.undoCloseTabMenu) {
      
      var ss = Cc["@mozilla.org/browser/sessionstore;1"].
               getService(Ci.nsISessionStore);
      AugmentTabs.undoCloseTabMenu.hidden = !(ss.getClosedTabCount(window) > 0);
    }
  }
};




#ifndef MOZ_PLACES
var HistoryMenu = {};
#endif

HistoryMenu.toggleRecentlyClosedTabs = function PHM_toggleRecentlyClosedTabs() {
  
  var undoPopup = document.getElementById("historyUndoPopup");

  
  var ss = Cc["@mozilla.org/browser/sessionstore;1"].
           getService(Ci.nsISessionStore);
  
  if (ss.getClosedTabCount(window) == 0)
    undoPopup.parentNode.setAttribute("disabled", true);
  else
    undoPopup.parentNode.removeAttribute("disabled");
}




HistoryMenu.populateUndoSubmenu = function PHM_populateUndoSubmenu() {
  var undoPopup = document.getElementById("historyUndoPopup");

  
  while (undoPopup.hasChildNodes())
    undoPopup.removeChild(undoPopup.firstChild);

  
  var ss = Cc["@mozilla.org/browser/sessionstore;1"].
           getService(Ci.nsISessionStore);
  
  if (ss.getClosedTabCount(window) == 0) {
    undoPopup.parentNode.setAttribute("disabled", true);
    return;
  }

  
  undoPopup.parentNode.removeAttribute("disabled");

  
  var undoItems = eval("(" + ss.getClosedTabData(window) + ")");
  for (var i = 0; i < undoItems.length; i++) {
    var m = undoPopup.appendChild(document.createElement("menuitem"));
    m.setAttribute("label", undoItems[i].title);
    m.setAttribute("value", i);
    m.setAttribute("oncommand", "undoCloseTab(" + i + ");");
    m.addEventListener("click", undoCloseMiddleClick, false);
  }

  
  var strings = gNavigatorBundle;
  undoPopup.appendChild(document.createElement("menuseparator"));
  m = undoPopup.appendChild(document.createElement("menuitem"));
  m.setAttribute("label", strings.getString("menuOpenAllInTabs.label"));
  m.setAttribute("accesskey", strings.getString("menuOpenAllInTabs.accesskey"));
  m.addEventListener("command", function() {
    for (var i = 0; i < undoItems.length; i++)
      undoCloseTab();
  }, false);
}







function undoCloseMiddleClick(aEvent) {
  if (aEvent.button != 1)
    return;

  undoCloseTab(aEvent.originalTarget.value);
  getBrowser().moveTabToEnd();
}






function undoCloseTab(aIndex) {
  
  var tabbrowser = getBrowser();
  var blankTabToRemove = null;
  if (tabbrowser.tabContainer.childNodes.length == 1 &&
      !gPrefService.getBoolPref("browser.tabs.autoHide") &&
      tabbrowser.selectedBrowser.sessionHistory.count < 2 &&
      tabbrowser.selectedBrowser.currentURI.spec == "about:blank" &&
      !tabbrowser.selectedBrowser.contentDocument.body.hasChildNodes() &&
      !tabbrowser.selectedTab.hasAttribute("busy"))
    blankTabToRemove = tabbrowser.selectedTab;

  var ss = Cc["@mozilla.org/browser/sessionstore;1"].
           getService(Ci.nsISessionStore);
  if (ss.getClosedTabCount(window) == 0)
    return;
  ss.undoCloseTab(window, aIndex || 0);

  if (blankTabToRemove)
    tabbrowser.removeTab(blankTabToRemove);
}









function formatURL(aFormat, aIsPref) {
  var formatter = Cc["@mozilla.org/toolkit/URLFormatterService;1"].getService(Ci.nsIURLFormatter);
  return aIsPref ? formatter.formatURLPref(aFormat) : formatter.formatURL(aFormat);
}






function BookmarkAllTabsHandler() {
  this._command = document.getElementById("Browser:BookmarkAllTabs");
  gBrowser.addEventListener("TabOpen", this, true);
  gBrowser.addEventListener("TabClose", this, true);
  this._updateCommandState();
}

BookmarkAllTabsHandler.prototype = {
  QueryInterface: function BATH_QueryInterface(aIID) {
    if (aIID.equals(Ci.nsIDOMEventListener) ||
        aIID.equals(Ci.nsISupports))
      return this;

    throw Cr.NS_NOINTERFACE;
  },

  _updateCommandState: function BATH__updateCommandState(aTabClose) {
    var numTabs = gBrowser.tabContainer.childNodes.length;

    
    if (aTabClose)
      numTabs--;

    if (numTabs > 1)
      this._command.removeAttribute("disabled");
    else
      this._command.setAttribute("disabled", "true");
  },

  doCommand: function BATH_doCommand() {
#ifdef MOZ_PLACES_BOOKMARKS
    PlacesCommandHook.bookmarkCurrentPages();
#else
    addBookmarkAs(gBrowser, true);
#endif
  },

  
  handleEvent: function(aEvent) {
    this._updateCommandState(aEvent.type == "TabClose");
  }
};
