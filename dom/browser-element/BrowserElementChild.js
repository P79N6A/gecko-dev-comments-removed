



"use strict";

let { classes: Cc, interfaces: Ci, results: Cr, utils: Cu }  = Components;
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Geometry.jsm");

function debug(msg) {
  
}



docShell.isActive = true;

let infos = sendSyncMessage('browser-element-api:call',
                            { 'msg_name': 'hello' })[0];
docShell.QueryInterface(Ci.nsIDocShellTreeItem).name = infos.name;
docShell.setFullscreenAllowed(infos.fullscreenAllowed);


if (!('BrowserElementIsPreloaded' in this)) {
  
  try {
    Services.scriptloader.loadSubScript("chrome://browser/content/forms.js");
  } catch (e) {
  }
  Services.scriptloader.loadSubScript("chrome://global/content/BrowserElementPanning.js");
  Services.scriptloader.loadSubScript("chrome://global/content/BrowserElementChildPreload.js");
}

var BrowserElementIsReady = true;
