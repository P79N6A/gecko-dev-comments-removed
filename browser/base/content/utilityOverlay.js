# -*- indent-tabs-mode: nil; js-indent-level: 4 -*-
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http:


Components.utils.import("resource://gre/modules/Services.jsm");
Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/PrivateBrowsingUtils.jsm");
Components.utils.import("resource:///modules/RecentWindow.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "NewTabURL",
  "resource:///modules/NewTabURL.jsm");

this.__defineGetter__("BROWSER_NEW_TAB_URL", () => {
  if (PrivateBrowsingUtils.isWindowPrivate(window) &&
      !PrivateBrowsingUtils.permanentPrivateBrowsing &&
      !NewTabURL.overridden) {
    return "about:privatebrowsing";
  }
  return NewTabURL.get();
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

  let isPrivate = PrivateBrowsingUtils.isWindowPrivate(window);
  return RecentWindow.getMostRecentBrowserWindow({private: isPrivate,
                                                  allowPopups: !skipPopups});
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
      referrerPolicy: Components.interfaces.nsIHttpChannel.REFERRER_POLICY_DEFAULT,
      initiatingDoc: event ? event.target.ownerDocument : null,
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
      referrerURI: aReferrerURI,
      referrerPolicy: Components.interfaces.nsIHttpChannel.REFERRER_POLICY_DEFAULT,
    };
  }

  params.fromChrome = true;

  openLinkIn(url, where, params);
}

function openLinkIn(url, where, params) {
  if (!where || !url)
    return;
  const Cc = Components.classes;
  const Ci = Components.interfaces;

  var aFromChrome           = params.fromChrome;
  var aAllowThirdPartyFixup = params.allowThirdPartyFixup;
  var aPostData             = params.postData;
  var aCharset              = params.charset;
  var aReferrerURI          = params.referrerURI;
  var aReferrerPolicy       = ('referrerPolicy' in params ?
      params.referrerPolicy : Ci.nsIHttpChannel.REFERRER_POLICY_DEFAULT);
  var aRelatedToCurrent     = params.relatedToCurrent;
  var aAllowMixedContent    = params.allowMixedContent;
  var aInBackground         = params.inBackground;
  var aDisallowInheritPrincipal = params.disallowInheritPrincipal;
  var aInitiatingDoc        = params.initiatingDoc;
  var aIsPrivate            = params.private;
  var aSkipTabAnimation     = params.skipTabAnimation;
  var aAllowPinnedTabHostChange = !!params.allowPinnedTabHostChange;
  var aNoReferrer           = params.noReferrer;
  var aAllowPopups          = !!params.allowPopups;

  if (where == "save") {
    if (!aInitiatingDoc) {
      Components.utils.reportError("openUILink/openLinkIn was called with " +
        "where == 'save' but without initiatingDoc.  See bug 814264.");
      return;
    }
    
    saveURL(url, null, null, true, null, aNoReferrer ? null : aReferrerURI, aInitiatingDoc);
    return;
  }

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

    var referrerURISupports = null;
    if (aReferrerURI && !aNoReferrer) {
      referrerURISupports = Cc["@mozilla.org/supports-string;1"].
                            createInstance(Ci.nsISupportsString);
      referrerURISupports.data = aReferrerURI.spec;
    }

    var referrerPolicySupports = Cc["@mozilla.org/supports-PRUint32;1"].
                                 createInstance(Ci.nsISupportsPRUint32);
    referrerPolicySupports.data = aReferrerPolicy;

    sa.AppendElement(wuri);
    sa.AppendElement(charset);
    sa.AppendElement(referrerURISupports);
    sa.AppendElement(aPostData);
    sa.AppendElement(allowThirdPartyFixupSupports);
    sa.AppendElement(referrerPolicySupports);

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

  let uriObj;
  if (where == "current") {
    try {
      uriObj = Services.io.newURI(url, null, null);
    } catch (e) {}
  }

  if (where == "current" && w.gBrowser.selectedTab.pinned &&
      !aAllowPinnedTabHostChange) {
    try {
      
      if (!uriObj || (!uriObj.schemeIs("javascript") &&
                      w.gBrowser.currentURI.host != uriObj.host)) {
        where = "tab";
        loadInBackground = false;
      }
    } catch (err) {
      where = "tab";
      loadInBackground = false;
    }
  }

  
  
  w.focus();

  switch (where) {
  case "current":
    let flags = Ci.nsIWebNavigation.LOAD_FLAGS_NONE;

    if (aAllowThirdPartyFixup) {
      flags |= Ci.nsIWebNavigation.LOAD_FLAGS_ALLOW_THIRD_PARTY_FIXUP;
      flags |= Ci.nsIWebNavigation.LOAD_FLAGS_FIXUP_SCHEME_TYPOS;
    }

    
    
    
    
    if (aDisallowInheritPrincipal && !(uriObj && uriObj.schemeIs("javascript"))) {
      flags |= Ci.nsIWebNavigation.LOAD_FLAGS_DISALLOW_INHERIT_OWNER;
    }

    if (aAllowPopups) {
      flags |= Ci.nsIWebNavigation.LOAD_FLAGS_ALLOW_POPUPS;
    }

    w.gBrowser.loadURIWithFlags(url, {
      flags: flags,
      referrerURI: aNoReferrer ? null : aReferrerURI,
      referrerPolicy: aReferrerPolicy,
      postData: aPostData,
    });
    break;
  case "tabshifted":
    loadInBackground = !loadInBackground;
    
  case "tab":
    w.gBrowser.loadOneTab(url, {
      referrerURI: aReferrerURI,
      referrerPolicy: aReferrerPolicy,
      charset: aCharset,
      postData: aPostData,
      inBackground: loadInBackground,
      allowThirdPartyFixup: aAllowThirdPartyFixup,
      relatedToCurrent: aRelatedToCurrent,
      skipAnimation: aSkipTabAnimation,
      allowMixedContent: aAllowMixedContent,
      noReferrer: aNoReferrer
    });
    break;
  }

  w.gBrowser.selectedBrowser.focus();

  if (!loadInBackground && w.isBlankPageURL(url)) {
    w.focusAndSelectUrlBar();
  }
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
        text += " " + altText;
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
  
  text = text.trim();
  
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

  
  var chromeReg = Components.classes["@mozilla.org/chrome/chrome-registry;1"].
                  getService(Components.interfaces.nsIXULChromeRegistry);
  if (chromeReg.isLocaleRTL("global"))
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
      case "ug-":
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
    if (win.closed) {
      continue;
    }
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
  function switchToAdvancedSubPane(doc) {
    if (extraArgs && extraArgs["advancedTab"]) {
      let advancedPaneTabs = doc.getElementById("advancedPrefs");
      advancedPaneTabs.selectedTab = doc.getElementById(extraArgs["advancedTab"]);
    }
  }

  
  function internalPrefCategoryNameToFriendlyName(aName) {
    return (aName || "").replace(/^pane./, function(toReplace) { return toReplace[4].toLowerCase(); });
  }

  if (getBoolPref("browser.preferences.inContent")) {
    let win = Services.wm.getMostRecentWindow("navigator:browser");
    let friendlyCategoryName = internalPrefCategoryNameToFriendlyName(paneID);
    let preferencesURL = "about:preferences" +
                         (friendlyCategoryName ? "#" + friendlyCategoryName : "");
    let newLoad = true;
    let browser = null;
    if (!win) {
      const Cc = Components.classes;
      const Ci = Components.interfaces;
      let windowArguments = Cc["@mozilla.org/supports-array;1"]
                              .createInstance(Ci.nsISupportsArray);
      let supportsStringPrefURL = Cc["@mozilla.org/supports-string;1"]
                                    .createInstance(Ci.nsISupportsString);
      supportsStringPrefURL.data = preferencesURL;
      windowArguments.AppendElement(supportsStringPrefURL);

      win = Services.ww.openWindow(null, Services.prefs.getCharPref("browser.chromeURL"),
                                   "_blank", "chrome,dialog=no,all", windowArguments);
    } else {
      newLoad = !win.switchToTabHavingURI(preferencesURL, true, {ignoreFragment: true});
      browser = win.gBrowser.selectedBrowser;
    }

    if (newLoad) {
      Services.obs.addObserver(function advancedPaneLoadedObs(prefWin, topic, data) {
        if (!browser) {
          browser = win.gBrowser.selectedBrowser;
        }
        if (prefWin != browser.contentWindow) {
          return;
        }
        Services.obs.removeObserver(advancedPaneLoadedObs, "advanced-pane-loaded");
        switchToAdvancedSubPane(browser.contentDocument);
      }, "advanced-pane-loaded", false);
    } else {
      if (paneID) {
        browser.contentWindow.gotoPref(paneID);
      }
      switchToAdvancedSubPane(browser.contentDocument);
    }
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

      switchToAdvancedSubPane(win.document);
    } else {
      openDialog("chrome://browser/content/preferences/preferences.xul",
                 "Preferences", features, paneID, extraArgs);
    }
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
  var url = Components.classes["@mozilla.org/toolkit/URLFormatterService;1"]
                      .getService(Components.interfaces.nsIURLFormatter)
                      .formatURLPref("app.feedback.baseURL");
  openUILinkIn(url, "tab");
}

function openTourPage()
{
  let scope = {}
  Components.utils.import("resource:///modules/UITour.jsm", scope);
  openUILinkIn(scope.UITour.url, "tab");
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
                        aAllowThirdPartyFixup, aReferrer, aReferrerPolicy) {

  
  
  let originCharset = null;
  if (document.documentElement.getAttribute("windowtype") == "navigator:browser")
    originCharset = gBrowser.selectedBrowser.characterSet;

  openLinkIn(aURL, aEvent && aEvent.shiftKey ? "tabshifted" : "tab",
             { charset: originCharset,
               postData: aPostData,
               allowThirdPartyFixup: aAllowThirdPartyFixup,
               referrerURI: aReferrer,
               referrerPolicy: aReferrerPolicy,
             });
}





function openNewWindowWith(aURL, aDocument, aPostData, aAllowThirdPartyFixup,
                           aReferrer, aReferrerPolicy) {
  
  
  let originCharset = null;
  if (document.documentElement.getAttribute("windowtype") == "navigator:browser")
    originCharset = gBrowser.selectedBrowser.characterSet;

  openLinkIn(aURL, "window",
             { charset: originCharset,
               postData: aPostData,
               allowThirdPartyFixup: aAllowThirdPartyFixup,
               referrerURI: aReferrer,
               referrerPolicy: aReferrerPolicy,
             });
}


function openHelpLink(aHelpTopic, aCalledFromModal, aWhere) {
  var url = Components.classes["@mozilla.org/toolkit/URLFormatterService;1"]
                      .getService(Components.interfaces.nsIURLFormatter)
                      .formatURLPref("app.support.baseURL");
  url += aHelpTopic;

  var where = aWhere;
  if (!aWhere)
    where = aCalledFromModal ? "window" : "tab";

  openUILinkIn(url, where);
}

function openPrefsHelp() {
  
  
  var instantApply = getBoolPref("browser.preferences.instantApply");

  var helpTopic = document.getElementsByTagName("prefwindow")[0].currentPane.helpTopic;
  openHelpLink(helpTopic, !instantApply);
}

function trimURL(aURL) {
  
  

  
  let url = aURL.replace(/^((?:http|https|ftp):\/\/[^/]+)\/$/, "$1");

  
  if (!url.startsWith("http://")) {
    return url;
  }
  let urlWithoutProtocol = url.substring(7);

  let flags = Services.uriFixup.FIXUP_FLAG_ALLOW_KEYWORD_LOOKUP |
              Services.uriFixup.FIXUP_FLAG_FIX_SCHEME_TYPOS;
  let fixedUpURL = Services.uriFixup.createFixupURI(urlWithoutProtocol, flags);
  let expectedURLSpec;
  try {
    expectedURLSpec = makeURI(aURL).spec;
  } catch (ex) {
    return url;
  }
  if (fixedUpURL.spec == expectedURLSpec) {
    return urlWithoutProtocol;
  }
  return url;
}
