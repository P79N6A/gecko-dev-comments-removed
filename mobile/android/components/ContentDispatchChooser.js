



const Ci = Components.interfaces;
const Cu = Components.utils;
const Cc = Components.classes;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

function ContentDispatchChooser() {}

ContentDispatchChooser.prototype =
{
  classID: Components.ID("5a072a22-1e66-4100-afc1-07aed8b62fc5"),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIContentDispatchChooser]),

  get protoSvc() {
    delete this.protoSvc;
    return this.protoSvc = Cc["@mozilla.org/uriloader/external-protocol-service;1"].getService(Ci.nsIExternalProtocolService);
  },

  _getChromeWin: function getChromeWin() {
    try {
      return Services.wm.getMostRecentWindow("navigator:browser");
    } catch (e) {
      throw Cr.NS_ERROR_FAILURE;
    }
  },

  ask: function ask(aHandler, aWindowContext, aURI, aReason) {
    let window = null;
    try {
      if (aWindowContext)
        window = aWindowContext.getInterface(Ci.nsIDOMWindow);
    } catch (e) {  }

    
    
    aHandler = this.protoSvc.getProtocolHandlerInfoFromOS(aURI.spec, {});

    
    
    if(aHandler.possibleApplicationHandlers.length > 1) {
      aHandler.launchWithURI(aURI, aWindowContext);
    } else {
      let win = this._getChromeWin();
      if (win && win.NativeWindow) {
        let bundle = Services.strings.createBundle("chrome://browser/locale/handling.properties");
        let text = bundle.GetStringFromName("protocol.failed");
        win.NativeWindow.toast.show(text, "long");
      }
    }
  }

};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([ContentDispatchChooser]);

