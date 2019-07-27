



"use strict";
const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

this.EXPORTED_SYMBOLS = ["Pocket"];

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "CustomizableUI",
  "resource:///modules/CustomizableUI.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "ReaderMode",
  "resource://gre/modules/ReaderMode.jsm");

let Pocket = {
  get site() Services.prefs.getCharPref("browser.pocket.site"),
  get listURL() { return "https://" + Pocket.site; },

  


  onPanelViewShowing(event) {
    let window = event.target.ownerDocument.defaultView;
    window.addEventListener("popupshowing", Pocket.onPocketPanelShowing, true);
    window.addEventListener("popupshown", Pocket.onPocketPanelShown, true);
  },

  onPocketPanelShowing(event) {
    let window = event.target.ownerDocument.defaultView;
    window.removeEventListener("popupshowing", Pocket.onPocketPanelShowing, true);
    window.pktUI.pocketButtonOnCommand(event);
  },

  onPocketPanelShown(event) {
    let window = event.target.ownerDocument.defaultView;
    window.removeEventListener("popupshown", Pocket.onPocketPanelShown, true);
    window.pktUI.pocketPanelDidShow(event);
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
