



function debug(msg) {
  Services.console.logStringMessage("SessionStoreContent: " + msg);
}

Cu.import("resource://gre/modules/XPCOMUtils.jsm", this);





let EventListener = {

  DOM_EVENTS: [
    "pageshow", "change", "input", "MozStorageChanged"
  ],

  init: function () {
    this.DOM_EVENTS.forEach(e => addEventListener(e, this, true));
  },

  handleEvent: function (event) {
    switch (event.type) {
      case "pageshow":
        if (event.persisted)
          sendAsyncMessage("SessionStore:pageshow");
        break;
      case "input":
      case "change":
        sendAsyncMessage("SessionStore:input");
        break;
      case "MozStorageChanged": {
        let isSessionStorage = true;
        
        try {
          if (event.storageArea != content.sessionStorage) {
            isSessionStorage = false;
          }
        } catch (ex) {
          
          
          isSessionStorage = false;
        }
        if (isSessionStorage) {
          sendAsyncMessage("SessionStore:MozStorageChanged");
        }
        break;
      }
      default:
        debug("received unknown event '" + event.type + "'");
        break;
    }
  }
};
EventListener.init();

let ProgressListener = {
  init: function() {
    let webProgress = docShell.QueryInterface(Ci.nsIInterfaceRequestor)
                              .getInterface(Ci.nsIWebProgress);
    webProgress.addProgressListener(this, Ci.nsIWebProgress.NOTIFY_LOCATION);
  },
  onLocationChange: function(aWebProgress, aRequest, aLocation, aFlags) {
    
    sendAsyncMessage("SessionStore:loadStart");
  },
  onStateChange: function(aWebProgress, aRequest, aStateFlags, aStatus) {},
  onProgressChange: function() {},
  onStatusChange: function() {},
  onSecurityChange: function() {},
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIWebProgressListener,
                                         Ci.nsISupportsWeakReference])
};
ProgressListener.init();
