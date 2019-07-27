












"use strict";

this.EXPORTED_SYMBOLS = [];

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/Services.jsm");

let gEMEUIObserver = function(subject, topic, data) {
  let win = subject.ownerDocument.defaultView.top;
  let mm = getMessageManagerForWindow(win);
  if (mm) {
    mm.sendAsyncMessage("EMEVideo:MetadataLoaded", {
      
      
      drmProvider: "Adobe"
    });
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

Services.obs.addObserver(gEMEUIObserver, "media-eme-metadataloaded", false);
