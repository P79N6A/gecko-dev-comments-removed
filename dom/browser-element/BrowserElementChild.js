



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

  if (docShell.asyncPanZoomEnabled === false) {
    Services.scriptloader.loadSubScript("chrome://global/content/BrowserElementPanningAPZDisabled.js");
    ContentPanningAPZDisabled.init();
  }

  Services.scriptloader.loadSubScript("chrome://global/content/BrowserElementPanning.js");
  ContentPanning.init();

  Services.scriptloader.loadSubScript("chrome://global/content/BrowserElementChildPreload.js");
} else {
  if (docShell.asyncPanZoomEnabled === false) {
    ContentPanningAPZDisabled.init();
  }
  ContentPanning.init();
}

var BrowserElementIsReady = true;


sendAsyncMessage('browser-element-api:call', { 'msg_name': 'hello' });
