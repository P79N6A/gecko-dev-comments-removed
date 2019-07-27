



"use strict";
const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

this.EXPORTED_SYMBOLS = ["Pocket"];

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "CustomizableUI",
  "resource:///modules/CustomizableUI.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "ReaderMode",
  "resource://gre/modules/ReaderMode.jsm");

let Pocket = {
  get site() Services.prefs.getCharPref("browser.pocket.site"),
  get listURL() { return "https://" + Pocket.site; },

  


  onPanelViewShowing(event) {
    let document = event.target.ownerDocument;
    let window = document.defaultView;
    let iframe = document.getElementById('pocket-panel-iframe');

    
    
    window.setTimeout(function() {
      window.pktUI.pocketButtonOnCommand();

      if (iframe.contentDocument &&
          iframe.contentDocument.readyState == "complete")
      {
        window.pktUI.pocketPanelDidShow();
      } else {
        
        
        
        iframe.addEventListener("load", Pocket.onFrameLoaded, true);
      }
    }, 0);
  },

  onFrameLoaded(event) {
    let document = event.currentTarget.ownerDocument;
    let window = document.defaultView;
    let iframe = document.getElementById('pocket-panel-iframe');

    iframe.removeEventListener("load", Pocket.onFrameLoaded, true);
    window.pktUI.pocketPanelDidShow();
  },

  onPanelViewHiding(event) {
    let window = event.target.ownerDocument.defaultView;
    window.pktUI.pocketPanelDidHide(event);
  },

  
  
  onLocationChange(browser, locationURI) {
    if (!locationURI) {
      return;
    }
    let widget = CustomizableUI.getWidget("pocket-button");
    for (let instance of widget.instances) {
      let node = instance.node;
      if (!node ||
          node.ownerDocument != browser.ownerDocument) {
        continue;
      }
      if (node) {
        let win = browser.ownerDocument.defaultView;
        node.disabled = win.pktApi.isUserLoggedIn() &&
                        !locationURI.schemeIs("http") &&
                        !locationURI.schemeIs("https") &&
                        !(locationURI.schemeIs("about") &&
                          locationURI.spec.toLowerCase().startsWith("about:reader?url="));
      }
    }
  },
};
