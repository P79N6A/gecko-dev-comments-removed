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
#   Gavin Sharp <gavin@gavinsharp.com>
#   DÃ£o Gottwald <dao@design-noir.de>
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


Components.utils.import("resource://gre/modules/Services.jsm");

var TAB_DROP_TYPE = "application/x-moz-tabbrowser-tab";

var gBidiUI = false;

function getBrowserURL()
{
  return "chrome://browser/content/browser.xul";
}

function getTopWin(skipPopups) {
  
  
  
  if (top.document.documentElement.getAttribute("windowtype") == "navigator:browser" &&
      (!skipPopups || !top.document.documentElement.getAttribute("chromehidden")))
    return top;

  if (skipPopups) {
    return Components.classes["@mozilla.org/browser/browserglue;1"]
                     .getService(Components.interfaces.nsIBrowserGlue)
                     .getMostRecentBrowserWindow();
  }
  return Services.wm.getMostRecentWindow("navigator:browser");
}

function openTopWin( url )
{
  openUILink(url, {})
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
  if (meta || (middle && middleUsesTabs))
#else
  if (ctrl || (middle && middleUsesTabs))
#endif
    return shift ? "tabshifted" : "tab";

  if (alt)
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

  if (where == "save") {
    saveURL(url, null, null, true, null, aReferrerURI);
    return;
  }
  const Cc = Components.classes;
  const Ci = Components.interfaces;

  var w = getTopWin();
  if ((where == "tab" || where == "tabshifted") &&
      w && w.document.documentElement.getAttribute("chromehidden")) {
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

    Services.ww.openWindow(w || window, getBrowserURL(),
                           null, "chrome,dialog=no,all", sa);
    return;
  }

  var loadInBackground = aFromChrome ?
                         getBoolPref("browser.tabs.loadBookmarksInBackground") :
                         getBoolPref("browser.tabs.loadInBackground");

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
    w.loadURI(url, aReferrerURI, aPostData, aAllowThirdPartyFixup);
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
                       relatedToCurrent: aRelatedToCurrent});
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
    if (node.namespaceURI == "http:
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

    return win;
  }

  return openDialog("chrome://browser/content/preferences/preferences.xul",
                    "Preferences", features, paneID, extraArgs);
}

function openAdvancedPreferences(tabID)
{
  return openPreferences("paneAdvanced", { "advancedTab" : tabID });
}





function openTroubleshootingPage()
{
  openUILinkIn("about:support", "tab");
}




function openFeedbackPage()
{
  openUILinkIn("http://input.mozilla.com/feedback", "tab");
}

function buildHelpMenu()
{
  
  
  if (typeof safebrowsing != "undefined")
    safebrowsing.setReportPhishingMenu();
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
              
             .replace(/^http:\/\/((?!ftp\.)[^\/@]+(?:\/|$))/, "$1");
}
