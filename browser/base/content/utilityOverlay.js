# -*- Mode: Java; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
#   Alec Flett <alecf@netscape.com>
#   Ehsan Akhgari <ehsan.akhgari@gmail.com>
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






var TAB_DROP_TYPE = "application/x-moz-tabbrowser-tab";

var gBidiUI = false;

function getBrowserURL()
{
  return "chrome://browser/content/browser.xul";
}

function goToggleToolbar( id, elementID )
{
  var toolbar = document.getElementById(id);
  var element = document.getElementById(elementID);
  if (toolbar)
  {
    var isHidden = toolbar.hidden;
    toolbar.hidden = !isHidden;
    document.persist(id, 'hidden');
    if (element) {
      element.setAttribute("checked", isHidden ? "true" : "false");
      document.persist(elementID, 'checked');
    }
  }
}

function getTopWin()
{
  var windowManager = Components.classes['@mozilla.org/appshell/window-mediator;1']
                                .getService(Components.interfaces.nsIWindowMediator);
  return windowManager.getMostRecentWindow("navigator:browser");
}

function openTopWin( url )
{
  openUILink(url, {})
}

function getBoolPref ( prefname, def )
{
  try { 
    var pref = Components.classes["@mozilla.org/preferences-service;1"]
                       .getService(Components.interfaces.nsIPrefBranch);
    return pref.getBoolPref(prefname);
  }
  catch(er) {
    return def;
  }
}


function openUILink( url, e, ignoreButton, ignoreAlt, allowKeywordFixup, postData, referrerUrl )
{
  var where = whereToOpenLink(e, ignoreButton, ignoreAlt);
  openUILinkIn(url, where, allowKeywordFixup, postData, referrerUrl);
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
  if (meta || (middle && middleUsesTabs)) {
#else
  if (ctrl || (middle && middleUsesTabs)) {
#endif
    if (shift)
      return "tabshifted";
    else
      return "tab";
  }
  else if (alt) {
    return "save";
  }
  else if (shift || (middle && !middleUsesTabs)) {
    return "window";
  }
  else {
    return "current";
  }
}














function openUILinkIn( url, where, allowThirdPartyFixup, postData, referrerUrl )
{
  if (!where || !url)
    return;

  if (where == "save") {
    saveURL(url, null, null, true, null, referrerUrl);
    return;
  }
  const Cc = Components.classes;
  const Ci = Components.interfaces;

  var w = getTopWin();

  if (!w || where == "window") {
    var sa = Cc["@mozilla.org/supports-array;1"].
             createInstance(Ci.nsISupportsArray);

    var wuri = Cc["@mozilla.org/supports-string;1"].
               createInstance(Ci.nsISupportsString);
    wuri.data = url;

    sa.AppendElement(wuri);
    sa.AppendElement(null);
    sa.AppendElement(referrerUrl);
    sa.AppendElement(postData);
    sa.AppendElement(allowThirdPartyFixup);

    var ww = Cc["@mozilla.org/embedcomp/window-watcher;1"].
             getService(Ci.nsIWindowWatcher);

    ww.openWindow(w || window,
                  getBrowserURL(),
                  null,
                  "chrome,dialog=no,all",
                  sa);

    return;
  }

  var loadInBackground = getBoolPref("browser.tabs.loadBookmarksInBackground", false);

  switch (where) {
  case "current":
    w.loadURI(url, referrerUrl, postData, allowThirdPartyFixup);
    break;
  case "tabshifted":
    loadInBackground = !loadInBackground;
    
  case "tab":
    let browser = w.getBrowser();
    browser.loadOneTab(url, {
                       referrerURI: referrerUrl,
                       postData: postData,
                       inBackground: loadInBackground,
                       allowThirdPartyFixup: allowThirdPartyFixup});
    break;
  }

  
  
  
  var fm = Components.classes["@mozilla.org/focus-manager;1"].
             getService(Components.interfaces.nsIFocusManager);
  if (window == fm.activeWindow)
    w.content.focus();
  else
    w.gBrowser.selectedBrowser.focus();
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
      
      if ( node.nextSibling ) {
        node = node.nextSibling;
      } else {
        
        node = node.parentNode.nextSibling;
        depth--;
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
  } catch (e) {dump("*** e = " + e + "\n");}
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
        var pref = Components.classes["@mozilla.org/preferences-service;1"]
                             .getService(Components.interfaces.nsIPrefBranch);
        pref.setBoolPref("bidi.browser.ui", true);
    }
  } catch (e) {}

  return rv;
}

function openAboutDialog()
{
#ifdef XP_MACOSX
  var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                     .getService(Components.interfaces.nsIWindowMediator);
  var win = wm.getMostRecentWindow("Browser:About");
  if (win)
    win.focus();
  else {
    window.openDialog("chrome://browser/content/aboutDialog.xul", "About",
                      "chrome, resizable=no, minimizable=no");
  }
#else
  window.openDialog("chrome://browser/content/aboutDialog.xul", "About", "centerscreen,chrome,resizable=no");
#endif
}

function openPreferences(paneID, extraArgs)
{
  var instantApply = getBoolPref("browser.preferences.instantApply", false);
  var features = "chrome,titlebar,toolbar,centerscreen" + (instantApply ? ",dialog=no" : ",modal");

  var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                     .getService(Components.interfaces.nsIWindowMediator);
  var win = wm.getMostRecentWindow("Browser:Preferences");
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

    return win;
  }

  return openDialog("chrome://browser/content/preferences/preferences.xul",
                    "Preferences", features, paneID, extraArgs);
}

function openAdvancedPreferences(tabID)
{
  return openPreferences("paneAdvanced", { "advancedTab" : tabID });
}




function openReleaseNotes()
{
  var formatter = Components.classes["@mozilla.org/toolkit/URLFormatterService;1"]
                            .getService(Components.interfaces.nsIURLFormatter);
  var relnotesURL = formatter.formatURLPref("app.releaseNotesURL");
  
  openUILinkIn(relnotesURL, "tab");
}





function openTroubleshootingPage()
{
  openUILinkIn("about:support", "tab");
}

#ifdef MOZ_UPDATER



function checkForUpdates()
{
  var um = 
      Components.classes["@mozilla.org/updates/update-manager;1"].
      getService(Components.interfaces.nsIUpdateManager);
  var prompter = 
      Components.classes["@mozilla.org/updates/update-prompt;1"].
      createInstance(Components.interfaces.nsIUpdatePrompt);

  
  
  
  if (um.activeUpdate && um.activeUpdate.state == "pending")
    prompter.showUpdateDownloaded(um.activeUpdate);
  else
    prompter.checkForUpdates();
}
#endif

function buildHelpMenu()
{
  
  
  if (typeof safebrowsing != "undefined")
    safebrowsing.setReportPhishingMenu();

#ifdef MOZ_UPDATER
  var updates = 
      Components.classes["@mozilla.org/updates/update-service;1"].
      getService(Components.interfaces.nsIApplicationUpdateService);
  var um = 
      Components.classes["@mozilla.org/updates/update-manager;1"].
      getService(Components.interfaces.nsIUpdateManager);

  
  
  var checkForUpdates = document.getElementById("checkForUpdates");
  var canUpdate = updates.canUpdate;
  checkForUpdates.setAttribute("disabled", !canUpdate);
  if (!canUpdate)
    return; 

  var strings = document.getElementById("bundle_browser");
  var activeUpdate = um.activeUpdate;
  
  
  
  function getStringWithUpdateName(key) {
    if (activeUpdate && activeUpdate.name)
      return strings.getFormattedString(key, [activeUpdate.name]);
    return strings.getString(key + "Fallback");
  }
  
  
  var key = "default";
  if (activeUpdate) {
    switch (activeUpdate.state) {
    case "downloading":
      
      
      
      key = updates.isDownloading ? "downloading" : "resume";
      break;
    case "pending":
      
      
      key = "pending";
      break;
    }
  }
  checkForUpdates.label = getStringWithUpdateName("updatesItem_" + key);
  checkForUpdates.accessKey = strings.getString("updatesItem_" + key + ".accesskey");
  if (um.activeUpdate && updates.isDownloading)
    checkForUpdates.setAttribute("loading", "true");
  else
    checkForUpdates.removeAttribute("loading");
#else
  
  document.getElementById("updateSeparator").hidden = true;
#endif
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
                        aAllowThirdPartyFixup, aReferrer)
{
  if (aDocument)
    urlSecurityCheck(aURL, aDocument.nodePrincipal);

  var prefSvc = Components.classes["@mozilla.org/preferences-service;1"]
                          .getService(Components.interfaces.nsIPrefService);
  prefSvc = prefSvc.getBranch(null);

  
  var loadInBackground = true;
  try {
    loadInBackground = prefSvc.getBoolPref("browser.tabs.loadInBackground");
  }
  catch(ex) {
  }

  if (aEvent && aEvent.shiftKey)
    loadInBackground = !loadInBackground;

  
  
  var wintype = document.documentElement.getAttribute("windowtype");
  var originCharset;
  if (wintype == "navigator:browser")
    originCharset = window.content.document.characterSet;

  
  var referrerURI = aDocument ? aDocument.documentURIObject : aReferrer;
  var browser = top.document.getElementById("content");
  return browser.loadOneTab(aURL, {
                            referrerURI: referrerURI,
                            charset: originCharset,
                            postData: aPostData,
                            inBackground: loadInBackground,
                            allowThirdPartyFixup: aAllowThirdPartyFixup});
}

function openNewWindowWith(aURL, aDocument, aPostData, aAllowThirdPartyFixup,
                           aReferrer)
{
  if (aDocument)
    urlSecurityCheck(aURL, aDocument.nodePrincipal);

  
  
  
  
  var charsetArg = null;
  var wintype = document.documentElement.getAttribute("windowtype");
  if (wintype == "navigator:browser")
    charsetArg = "charset=" + window.content.document.characterSet;

  var referrerURI = aDocument ? aDocument.documentURIObject : aReferrer;
  return window.openDialog(getBrowserURL(), "_blank", "chrome,all,dialog=no",
                           aURL, charsetArg, referrerURI, aPostData,
                           aAllowThirdPartyFixup);
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
  var prefs = Components.classes["@mozilla.org/preferences-service;1"]
                        .getService(Components.interfaces.nsIPrefBranch2);

  
  
  var instantApply = prefs.getBoolPref("browser.preferences.instantApply");

  var helpTopic = document.getElementsByTagName("prefwindow")[0].currentPane.helpTopic;
  openHelpLink(helpTopic, !instantApply);
}
