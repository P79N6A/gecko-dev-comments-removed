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






var goPrefWindow = 0;
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



function focusElement(aElement) {
  
  var ww = Components.classes["@mozilla.org/embedcomp/window-watcher;1"]
                     .getService(Components.interfaces.nsIWindowWatcher);
  if (window == ww.activeWindow)
    aElement.focus();
  else {
    
    
    var cmdDispatcher = document.commandDispatcher;
    if (aElement instanceof Window) {
      cmdDispatcher.focusedWindow = aElement;
      cmdDispatcher.focusedElement = null;
    }
    else if (aElement instanceof Element) {
      cmdDispatcher.focusedWindow = aElement.ownerDocument.defaultView;
      cmdDispatcher.focusedElement = aElement;
    }
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
    e = { shiftKey:false, ctrlKey:false, metaKey:false, altKey:false, button:0 };

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

  var w = getTopWin();

  if (!w || where == "window") {
    openDialog(getBrowserURL(), "_blank", "chrome,all,dialog=no", url,
               null, referrerUrl, postData, allowThirdPartyFixup);
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
    var browser = w.getBrowser();
    browser.loadOneTab(url, referrerUrl, null, postData, loadInBackground,
                       allowThirdPartyFixup || false);
    break;
  }

  
  
  
  focusElement(w.content);
}



function checkForMiddleClick(node, event)
{
  
  
  
  if (node.getAttribute("disabled") == "true")
    return; 

  if (event.button == 1) {
    



    var fn = new Function("event", node.getAttribute("oncommand"));
    fn.call(node, event);

    
    
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
    }
  } catch (e) {}

  
  if (!rv)
    rv = getBoolPref("bidi.browser.ui");

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
    
    
    window.open("chrome://browser/content/aboutDialog.xul", "About",
                "chrome, resizable=no, minimizable=no");
  }
#else
  window.openDialog("chrome://browser/content/aboutDialog.xul", "About", "modal,centerscreen,chrome,resizable=no");
#endif
}

function openPreferences(paneID)
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
  }
  else
    openDialog("chrome://browser/content/preferences/preferences.xul",
               "Preferences", features, paneID);
}








function openReleaseNotes(event)
{
  var formatter = Components.classes["@mozilla.org/toolkit/URLFormatterService;1"]
                            .getService(Components.interfaces.nsIURLFormatter);
  var relnotesURL = formatter.formatURLPref("app.releaseNotesURL");
  
  openUILink(relnotesURL, event, false, true);
}
  



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

function buildHelpMenu()
{
  
  
  if (typeof safebrowsing != "undefined")
    safebrowsing.setReportPhishingMenu();

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
  if (um.activeUpdate && updates.isDownloading)
    checkForUpdates.setAttribute("loading", "true");
  else
    checkForUpdates.removeAttribute("loading");
}

function isElementVisible(aElement)
{
  
  
  
  var bo = aElement.boxObject;
  return (bo.height != 0 && bo.width != 0 &&
          document.defaultView
                  .getComputedStyle(aElement, null).visibility == "visible");
}

function getBrowserFromContentWindow(aContentWindow)
{
  var browsers = gBrowser.browsers;
  for (var i = 0; i < browsers.length; i++) {
    if (browsers[i].contentWindow == aContentWindow)
      return browsers[i];
  }
  return null;
}





















 
function openNewTabWith(aURL, aDocument, aPostData, aEvent,
                        aAllowThirdPartyFixup)
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

  
  var referrerURI = aDocument ? aDocument.documentURIObject : null;
  var browser = top.document.getElementById("content");
  browser.loadOneTab(aURL, referrerURI, originCharset, aPostData,
                     loadInBackground, aAllowThirdPartyFixup || false);
}

function openNewWindowWith(aURL, aDocument, aPostData, aAllowThirdPartyFixup)
{
  if (aDocument)
    urlSecurityCheck(aURL, aDocument.nodePrincipal);

  
  
  
  
  var charsetArg = null;
  var wintype = document.documentElement.getAttribute("windowtype");
  if (wintype == "navigator:browser")
    charsetArg = "charset=" + window.content.document.characterSet;

  var referrerURI = aDocument ? aDocument.documentURIObject : null;
  window.openDialog(getBrowserURL(), "_blank", "chrome,all,dialog=no",
                    aURL, charsetArg, referrerURI, aPostData,
                    aAllowThirdPartyFixup);
}











 
function recognizeFeedFromLink(aLink, aPrincipal)
{
  if (!aLink || !aPrincipal)
    return null;

  var erel = aLink.rel;
  var etype = aLink.type;
  var etitle = aLink.title;
  const rssTitleRegex = /(^|\s)rss($|\s)/i;
  var rels = {};

  if (erel) {
    for each (var relValue in erel.split(/\s+/))
      rels[relValue] = true;
  }
  var isFeed = rels.feed;

  if (!isFeed && (!rels.alternate || rels.stylesheet || !etype))
    return null;

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
    try { 
      urlSecurityCheck(aLink.href,
                       aPrincipal,
                       Components.interfaces.nsIScriptSecurityManager.DISALLOW_INHERIT_PRINCIPAL);
    }
    catch (ex) {
      dump(ex.message);
      return null; 
    }

    
    return {
        href: aLink.href,
        type: etype,
        title: aLink.title
      };
  }

  return null;
}
