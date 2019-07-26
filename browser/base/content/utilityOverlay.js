# -*- Mode: javascript; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http:


Components.utils.import("resource://gre/modules/Services.jsm");
Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/PrivateBrowsingUtils.jsm");

XPCOMUtils.defineLazyGetter(this, "BROWSER_NEW_TAB_URL", function () {
  const PREF = "browser.newtab.url";

  function getNewTabPageURL() {
    if (!Services.prefs.prefHasUserValue(PREF)) {
      if (PrivateBrowsingUtils.isWindowPrivate(window) &&
          !PrivateBrowsingUtils.permanentPrivateBrowsing)
        return "about:privatebrowsing";
    }
    return Services.prefs.getCharPref(PREF) || "about:blank";
  }

  function update() {
    BROWSER_NEW_TAB_URL = getNewTabPageURL();
  }

  Services.prefs.addObserver(PREF, update, false);

  addEventListener("unload", function onUnload() {
    removeEventListener("unload", onUnload);
    Services.prefs.removeObserver(PREF, update);
  });

  return getNewTabPageURL();
});

var TAB_DROP_TYPE = "application/x-moz-tabbrowser-tab";

var gBidiUI = false;




function isBlankPageURL(aURL) {
  return aURL == "about:blank" || aURL == BROWSER_NEW_TAB_URL;
}

function getBrowserURL()
{
  return "chrome://browser/content/browser.xul";
}

function getTopWin(skipPopups) {
  
  
  
  if (top.document.documentElement.getAttribute("windowtype") == "navigator:browser" &&
      (!skipPopups || top.toolbar.visible))
    return top;

  if (skipPopups) {
    return Components.classes["@mozilla.org/browser/browserglue;1"]
                     .getService(Components.interfaces.nsIBrowserGlue)
                     .getMostRecentBrowserWindow();
  }
  return Services.wm.getMostRecentWindow("navigator:browser");
}

function openTopWin(url) {
  
  openUILinkIn(url, "current");
}

function getBoolPref(prefname, def)
{
  try {
    return Services.prefs.getBoolPref(prefname);
  }
  catch(er) {
    return def;
  }
}






function openUILink(url, event, aIgnoreButton, aIgnoreAlt, aAllowThirdPartyFixup,
                    aPostData, aReferrerURI) {
  let params;

  if (aIgnoreButton && typeof aIgnoreButton == "object") {
    params = aIgnoreButton;

    
    aIgnoreButton = params.ignoreButton;
    aIgnoreAlt = params.ignoreAlt;
    delete params.ignoreButton;
    delete params.ignoreAlt;
  } else {
    params = {
      allowThirdPartyFixup: aAllowThirdPartyFixup,
      postData: aPostData,
      referrerURI: aReferrerURI,
      initiatingDoc: event ? event.target.ownerDocument : null
    };
  }

  let where = whereToOpenLink(event, aIgnoreButton, aIgnoreAlt);
  openUILinkIn(url, where, params);
}





















function whereToOpenLink( e, ignoreButton, ignoreAlt )
{
  
  
  
  if (!e)
    return "current";

  var shift = e.shiftKey;
  var ctrl =  e.ctrlKey;
  var meta =  e.metaKey;
  var alt  =  e.altKey && !ignoreAlt;

  
  var middle = !ignoreButton && e.button == 1;
  var middleUsesTabs = getBoolPref("browser.tabs.opentabfor.middleclick", true);

  

#ifdef XP_MACOSX
  if (meta || (middle && middleUsesTabs))
#else
  if (ctrl || (middle && middleUsesTabs))
#endif
    return shift ? "tabshifted" : "tab";

  if (alt && getBoolPref("browser.altClickSave", false))
    return "save";

  if (shift || (middle && !middleUsesTabs))
    return "window";

  return "current";
}





















function openUILinkIn(url, where, aAllowThirdPartyFixup, aPostData, aReferrerURI) {
  var params;

  if (arguments.length == 3 && typeof arguments[2] == "object") {
    params = aAllowThirdPartyFixup;
  } else {
    params = {
      allowThirdPartyFixup: aAllowThirdPartyFixup,
      postData: aPostData,
      referrerURI: aReferrerURI
    };
  }

  params.fromChrome = true;

  openLinkIn(url, where, params);
}

function openLinkIn(url, where, params) {
  if (!where || !url)
    return;

  var aFromChrome           = params.fromChrome;
  var aAllowThirdPartyFixup = params.allowThirdPartyFixup;
  var aPostData             = params.postData;
  var aCharset              = params.charset;
  var aReferrerURI          = params.referrerURI;
  var aRelatedToCurrent     = params.relatedToCurrent;
  var aInBackground         = params.inBackground;
  var aDisallowInheritPrincipal = params.disallowInheritPrincipal;
  
  var aIsUTF8               = params.isUTF8;
  var aInitiatingDoc        = params.initiatingDoc;
  var aIsPrivate            = params.private;

  if (where == "save") {
    if (!aInitiatingDoc) {
      Components.utils.reportError("openUILink/openLinkIn was called with " +
        "where == 'save' but without initiatingDoc.  See bug 814264.");
      return;
    }
    saveURL(url, null, null, true, null, aReferrerURI, aInitiatingDoc);
    return;
  }
  const Cc = Components.classes;
  const Ci = Components.interfaces;

  var w = getTopWin();
  if ((where == "tab" || where == "tabshifted") &&
      w && !w.toolbar.visible) {
    w = getTopWin(true);
    aRelatedToCurrent = false;
  }

  if (!w || where == "window") {
    var sa = Cc["@mozilla.org/supports-array;1"].
             createInstance(Ci.nsISupportsArray);

    var wuri = Cc["@mozilla.org/supports-string;1"].
               createInstance(Ci.nsISupportsString);
    wuri.data = url;

    let charset = null;
    if (aCharset) {
      charset = Cc["@mozilla.org/supports-string;1"]
                  .createInstance(Ci.nsISupportsString);
      charset.data = "charset=" + aCharset;
    }

    var allowThirdPartyFixupSupports = Cc["@mozilla.org/supports-PRBool;1"].
                                       createInstance(Ci.nsISupportsPRBool);
    allowThirdPartyFixupSupports.data = aAllowThirdPartyFixup;

    sa.AppendElement(wuri);
    sa.AppendElement(charset);
    sa.AppendElement(aReferrerURI);
    sa.AppendElement(aPostData);
    sa.AppendElement(allowThirdPartyFixupSupports);

    let features = "chrome,dialog=no,all";
    if (aIsPrivate) {
      features += ",private";
    }

    Services.ww.openWindow(w || window, getBrowserURL(), null, features, sa);
    return;
  }

  let loadInBackground = where == "current" ? false : aInBackground;
  if (loadInBackground == null) {
    loadInBackground = aFromChrome ?
                         false :
                         getBoolPref("browser.tabs.loadInBackground");
  }

  if (where == "current" && w.gBrowser.selectedTab.pinned) {
    try {
      let uriObj = Services.io.newURI(url, null, null);
      if (!uriObj.schemeIs("javascript") &&
          w.gBrowser.currentURI.host != uriObj.host) {
        where = "tab";
        loadInBackground = false;
      }
    } catch (err) {
      where = "tab";
      loadInBackground = false;
    }
  }

  switch (where) {
  case "current":
    let flags = Ci.nsIWebNavigation.LOAD_FLAGS_NONE;
    if (aAllowThirdPartyFixup)
      flags |= Ci.nsIWebNavigation.LOAD_FLAGS_ALLOW_THIRD_PARTY_FIXUP;
    if (aDisallowInheritPrincipal)
      flags |= Ci.nsIWebNavigation.LOAD_FLAGS_DISALLOW_INHERIT_OWNER;
    if (aIsUTF8)
      flags |= Ci.nsIWebNavigation.LOAD_FLAGS_URI_IS_UTF8;
    w.gBrowser.loadURIWithFlags(url, flags, aReferrerURI, null, aPostData);
    break;
  case "tabshifted":
    loadInBackground = !loadInBackground;
    
  case "tab":
    let browser = w.gBrowser;
    browser.loadOneTab(url, {
                       referrerURI: aReferrerURI,
                       charset: aCharset,
                       postData: aPostData,
                       inBackground: loadInBackground,
                       allowThirdPartyFixup: aAllowThirdPartyFixup,
                       relatedToCurrent: aRelatedToCurrent,
                       isUTF8: aIsUTF8});
    break;
  }

  
  
  
  
  var fm = Components.classes["@mozilla.org/focus-manager;1"].
             getService(Components.interfaces.nsIFocusManager);
  if (window == fm.activeWindow || w.isBlankPageURL(url))
    w.focus();
  w.gBrowser.selectedBrowser.focus();

  if (!loadInBackground && w.isBlankPageURL(url))
    w.focusAndSelectUrlBar();
}



function checkForMiddleClick(node, event) {
  
  
  
  if (node.getAttribute("disabled") == "true")
    return; 

  if (event.button == 1) {
    



    var target = node.hasAttribute("oncommand") ? node :
                 node.ownerDocument.getElementById(node.getAttribute("command"));
    var fn = new Function("event", target.getAttribute("oncommand"));
    fn.call(target, event);

    
    
    closeMenus(event.target);
  }
}


function closeMenus(node)
{
  if ("tagName" in node) {
    if (node.namespaceURI == "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul"
    && (node.tagName == "menupopup" || node.tagName == "popup"))
      node.hidePopup();

    closeMenus(node.parentNode);
  }
}


function gatherTextUnder ( root ) 
{
  var text = "";
  var node = root.firstChild;
  var depth = 1;
  while ( node && depth > 0 ) {
    
    if ( node.nodeType == Node.TEXT_NODE ) {
      
      text += " " + node.data;
    } else if ( node instanceof HTMLImageElement) {
      
      var altText = node.getAttribute( "alt" );
      if ( altText && altText != "" ) {
        text = altText;
        break;
      }
    }
    
    
    if ( node.hasChildNodes() ) {
      
      node = node.firstChild;
      depth++;
    } else {
      
      while ( depth > 0 && !node.nextSibling ) {
        node = node.parentNode;
        depth--;
      }
      if ( node.nextSibling ) {
        node = node.nextSibling;
      }
    }
  }
  
  text = text.replace( /^\s+/, "" );
  
  text = text.replace( /\s+$/, "" );
  
  text = text.replace( /\s+/g, " " );
  return text;
}

function getShellService()
{
  var shell = null;
  try {
    shell = Components.classes["@mozilla.org/browser/shell-service;1"]
      .getService(Components.interfaces.nsIShellService);
  } catch (e) {
  }
  return shell;
}

function isBidiEnabled() {
  
  if (getBoolPref("bidi.browser.ui", false))
    return true;

  
  
  var rv = false;

  try {
    var localeService = Components.classes["@mozilla.org/intl/nslocaleservice;1"]
                                  .getService(Components.interfaces.nsILocaleService);
    var systemLocale = localeService.getSystemLocale().getCategory("NSILOCALE_CTYPE").substr(0,3);

    switch (systemLocale) {
      case "ar-":
      case "he-":
      case "fa-":
      case "ur-":
      case "syr":
        rv = true;
        Services.prefs.setBoolPref("bidi.browser.ui", true);
    }
  } catch (e) {}

  return rv;
}

function openAboutDialog() {
  var enumerator = Services.wm.getEnumerator("Browser:About");
  while (enumerator.hasMoreElements()) {
    
    let win = enumerator.getNext();
    win.focus();
    return;
  }

#ifdef XP_WIN
  var features = "chrome,centerscreen,dependent";
#elifdef XP_MACOSX
  var features = "chrome,resizable=no,minimizable=no";
#else
  var features = "chrome,centerscreen,dependent,dialog=no";
#endif
  window.openDialog("chrome://browser/content/aboutDialog.xul", "", features);
}

function openPreferences(paneID, extraArgs)
{
  if (Services.prefs.getBoolPref("browser.preferences.inContent")) {
    openUILinkIn("about:preferences", "tab");
  } else {
    var instantApply = getBoolPref("browser.preferences.instantApply", false);
    var features = "chrome,titlebar,toolbar,centerscreen" + (instantApply ? ",dialog=no" : ",modal");

    var win = Services.wm.getMostRecentWindow("Browser:Preferences");
    if (win) {
      win.focus();
      if (paneID) {
        var pane = win.document.getElementById(paneID);
        win.document.documentElement.showPane(pane);
      }

      if (extraArgs && extraArgs["advancedTab"]) {
        var advancedPaneTabs = win.document.getElementById("advancedPrefs");
        advancedPaneTabs.selectedTab = win.document.getElementById(extraArgs["advancedTab"]);
      }

     return;
    }

    openDialog("chrome://browser/content/preferences/preferences.xul",
               "Preferences", features, paneID, extraArgs);
  }
}

function openAdvancedPreferences(tabID)
{
  openPreferences("paneAdvanced", { "advancedTab" : tabID });
}





function openTroubleshootingPage()
{
  openUILinkIn("about:support", "tab");
}

#ifdef MOZ_SERVICES_HEALTHREPORT




function openHealthReport()
{
  openUILinkIn("about:healthreport", "tab");
}
#endif




function openFeedbackPage()
{
  openUILinkIn("http://input.mozilla.com/feedback", "tab");
}

function buildHelpMenu()
{
  
  if (typeof gSafeBrowsing != "undefined")
    gSafeBrowsing.setReportPhishingMenu();
}

function isElementVisible(aElement)
{
  if (!aElement)
    return false;

  
  
  var bo = aElement.boxObject;
  return (bo.height > 0 && bo.width > 0);
}

function makeURLAbsolute(aBase, aUrl)
{
  
  return makeURI(aUrl, null, makeURI(aBase)).spec;
}
























 
function openNewTabWith(aURL, aDocument, aPostData, aEvent,
                        aAllowThirdPartyFixup, aReferrer) {
  if (aDocument)
    urlSecurityCheck(aURL, aDocument.nodePrincipal);

  
  
  var originCharset = aDocument && aDocument.characterSet;
  if (!originCharset &&
      document.documentElement.getAttribute("windowtype") == "navigator:browser")
    originCharset = window.content.document.characterSet;

  openLinkIn(aURL, aEvent && aEvent.shiftKey ? "tabshifted" : "tab",
             { charset: originCharset,
               postData: aPostData,
               allowThirdPartyFixup: aAllowThirdPartyFixup,
               referrerURI: aDocument ? aDocument.documentURIObject : aReferrer });
}

function openNewWindowWith(aURL, aDocument, aPostData, aAllowThirdPartyFixup, aReferrer) {
  if (aDocument)
    urlSecurityCheck(aURL, aDocument.nodePrincipal);

  
  
  
  
  var originCharset = aDocument && aDocument.characterSet;
  if (!originCharset &&
      document.documentElement.getAttribute("windowtype") == "navigator:browser")
    originCharset = window.content.document.characterSet;

  openLinkIn(aURL, "window",
             { charset: originCharset,
               postData: aPostData,
               allowThirdPartyFixup: aAllowThirdPartyFixup,
               referrerURI: aDocument ? aDocument.documentURIObject : aReferrer });
}











 
function isValidFeed(aLink, aPrincipal, aIsFeed)
{
  if (!aLink || !aPrincipal)
    return false;

  var type = aLink.type.toLowerCase().replace(/^\s+|\s*(?:;.*)?$/g, "");
  if (!aIsFeed) {
    aIsFeed = (type == "application/rss+xml" ||
               type == "application/atom+xml");
  }

  if (aIsFeed) {
    try {
      urlSecurityCheck(aLink.href, aPrincipal,
                       Components.interfaces.nsIScriptSecurityManager.DISALLOW_INHERIT_PRINCIPAL);
      return type || "application/rss+xml";
    }
    catch(ex) {
    }
  }

  return null;
}


function openHelpLink(aHelpTopic, aCalledFromModal) {
  var url = Components.classes["@mozilla.org/toolkit/URLFormatterService;1"]
                      .getService(Components.interfaces.nsIURLFormatter)
                      .formatURLPref("app.support.baseURL");
  url += aHelpTopic;

  var where = aCalledFromModal ? "window" : "tab";
  openUILinkIn(url, where);
}

function openPrefsHelp() {
  
  
  var instantApply = getBoolPref("browser.preferences.instantApply");

  var helpTopic = document.getElementsByTagName("prefwindow")[0].currentPane.helpTopic;
  openHelpLink(helpTopic, !instantApply);
}

function trimURL(aURL) {
  
  
  return aURL 
             .replace(/^((?:http|https|ftp):\/\/[^/]+)\/$/, "$1")
              
             .replace(/^http:\/\/((?!ftp\d*\.)[^\/@]+(?:\/|$))/, "$1");
}
