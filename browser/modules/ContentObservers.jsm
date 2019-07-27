












"use strict";

this.EXPORTED_SYMBOLS = [];

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/Services.jsm");

let gEMEUIObserver = function(subject, topic, data) {
  let win = subject.top;
  let mm = getMessageManagerForWindow(win);
  if (mm) {
    mm.sendAsyncMessage("EMEVideo:ContentMediaKeysRequest", data);
  }
};

function getMessageManagerForWindow(aContentWindow) {
  let ir = aContentWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                         .getInterface(Ci.nsIDocShell)
                         .sameTypeRootTreeItem
                         .QueryInterface(Ci.nsIInterfaceRequestor);
  try {
    
    return ir.getInterface(Ci.nsIContentFrameMessageManager);
  } catch(e if e.result == Cr.NS_NOINTERFACE) {
    return null;
  }
}

Services.obs.addObserver(gEMEUIObserver, "mediakeys-request", false);
