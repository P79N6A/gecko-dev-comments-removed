



"use strict";

let { classes: Cc, interfaces: Ci, results: Cr, utils: Cu }  = Components;
Cu.import("resource://gre/modules/Services.jsm");

function debug(msg) {
  
}



docShell.isActive = true;

function parentDocShell(docshell) {
  if (!docshell) {
    return null;
  }
  let treeitem = docshell.QueryInterface(Ci.nsIDocShellTreeItem);
  return treeitem.parent ? treeitem.parent.QueryInterface(Ci.nsIDocShell) : null;
}

function isTopBrowserElement(docShell) {
  while (docShell) {
    docShell = parentDocShell(docShell);
    if (docShell && docShell.isBrowserOrApp) {
      return false;
    }
  }
  return true;
}

if (!('BrowserElementIsPreloaded' in this)) {
  if (isTopBrowserElement(docShell) &&
      Services.prefs.getBoolPref("dom.mozInputMethod.enabled")) {
    try {
      Services.scriptloader.loadSubScript("chrome://global/content/forms.js");
    } catch (e) {
    }
  }

  Services.scriptloader.loadSubScript("chrome://global/content/BrowserElementPanning.js");
  ContentPanning.init();

  Services.scriptloader.loadSubScript("chrome://global/content/BrowserElementChildPreload.js");
} else {
  ContentPanning.init();
}

var BrowserElementIsReady = true;

let infos = sendSyncMessage('browser-element-api:call',
                            { 'msg_name': 'hello' })[0];
docShell.QueryInterface(Ci.nsIDocShellTreeItem).name = infos.name;
docShell.setFullscreenAllowed(infos.fullscreenAllowed);
if (infos.isPrivate) {
  if (!docShell.hasLoadedNonBlankURI) {
    Cu.reportError("We should not switch to Private Browsing after loading a document.");
  } else {
    docShell.QueryInterface(Ci.nsILoadContext).usePrivateBrowsing = true;
  }
}
