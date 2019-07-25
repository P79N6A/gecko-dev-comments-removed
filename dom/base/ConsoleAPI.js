





































let Cu = Components.utils;
let Ci = Components.interfaces;
let Cc = Components.classes;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

function ConsoleAPI() {}
ConsoleAPI.prototype = {

  classID: Components.ID("{b49c18f8-3379-4fc0-8c90-d7772c1a9ff3}"),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIDOMGlobalPropertyInitializer]),

  
  init: function CA_init(aWindow) {
    let id;
    try {
      id = aWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                  .getInterface(Ci.nsIDOMWindowUtils)
                  .outerWindowID;
    } catch (ex) {
      Cu.reportError(ex);
    }

    let self = this;
    let chromeObject = {
      
      log: function CA_log() {
        self.notifyObservers(id, "log", arguments);
      },
      info: function CA_info() {
        self.notifyObservers(id, "info", arguments);
      },
      warn: function CA_warn() {
        self.notifyObservers(id, "warn", arguments);
      },
      error: function CA_error() {
        self.notifyObservers(id, "error", arguments);
      },
      debug: function CA_debug() {
        self.notifyObservers(id, "log", arguments);
      },
      __exposedProps__: {
        log: "r",
        info: "r",
        warn: "r",
        error: "r",
        debug: "r",
      }
    };

    
    
    let sandbox = Cu.Sandbox(aWindow);
    let contentObject = Cu.evalInSandbox(
        "(function(x) {\
          var bind = Function.bind;\
          var obj = {\
            log: bind.call(x.log, x),\
            info: bind.call(x.info, x),\
            warn: bind.call(x.warn, x),\
            error: bind.call(x.error, x),\
            debug: bind.call(x.debug, x),\
            __noSuchMethod__: function() {}\
          };\
          Object.defineProperty(obj, '__mozillaConsole__', { value: true });\
          return obj;\
        })", sandbox)(chromeObject);

      return contentObject;
  },

  


  notifyObservers: function CA_notifyObservers(aID, aLevel, aArguments) {
    if (!aID)
      return;

    let consoleEvent = {
      ID: aID,
      level: aLevel,
      arguments: aArguments
    };

    consoleEvent.wrappedJSObject = consoleEvent;

    Services.obs.notifyObservers(consoleEvent,
                                 "console-api-log-event", aID);
  }
};

let NSGetFactory = XPCOMUtils.generateNSGetFactory([ConsoleAPI]);
