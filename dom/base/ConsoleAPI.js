



































let Cu = Components.utils;
let Ci = Components.interfaces;
let Cc = Components.classes;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

function ConsoleAPI() {}
ConsoleAPI.prototype = {

  classID: Components.ID("{b49c18f8-3379-4fc0-8c90-d7772c1a9ff3}"),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIDOMGlobalPropertyInitializer]),

  
  id: null,

  
  
  init: function CA_init(aWindow) {
    try {
      this.id = aWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                       .getInterface(Ci.nsIDOMWindowUtils)
                       .outerWindowID;
    } catch (ex) {
      Cu.reportError(ex);
    }

    return this;
  },

  
  log: function CA_log() {
    this.notifyObservers("log", arguments);
  },
  info: function CA_info() {
    this.notifyObservers("info", arguments);
  },
  warn: function CA_warn() {
    this.notifyObservers("warn", arguments);
  },
  error: function CA_error() {
    this.notifyObservers("error", arguments);
  },

  __exposedProps__: {
    log: "r",
    info: "r",
    warn: "r",
    error: "r"
  },

  


  notifyObservers: function CA_notifyObservers(aLevel, aArguments) {
    let consoleEvent = {
      ID: this.id,
      level: aLevel,
      arguments: aArguments
    };

    consoleEvent.wrappedJSObject = consoleEvent;

    Services.obs.notifyObservers(consoleEvent,
                                 "console-api-log-event", this.id);
  }
};

let NSGetFactory = XPCOMUtils.generateNSGetFactory([ConsoleAPI]);
