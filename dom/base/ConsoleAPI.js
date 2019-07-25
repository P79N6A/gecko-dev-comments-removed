






































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
      trace: function CA_trace() {
        self.notifyObservers(id, "trace", self.getStackTrace());
      },
      
      dir: function CA_dir() {
        self.notifyObservers(id, "dir", arguments);
      },
      __exposedProps__: {
        log: "r",
        info: "r",
        warn: "r",
        error: "r",
        debug: "r",
        trace: "r",
        dir: "r"
      }
    };

    
    
    let contentObj = Cu.createObjectIn(aWindow);
    function genPropDesc(fun) {
      return { enumerable: true, configurable: true, writable: true,
               value: chromeObject[fun].bind(chromeObject) };
    }
    const properties = {
      log: genPropDesc('log'),
      info: genPropDesc('info'),
      warn: genPropDesc('warn'),
      error: genPropDesc('error'),
      debug: genPropDesc('debug'),
      trace: genPropDesc('trace'),
      dir: genPropDesc('dir'),
      __noSuchMethod__: { enumerable: true, configurable: true, writable: true,
                          value: function() {} },
      __mozillaConsole__: { value: true }
    };

    Object.defineProperties(contentObj, properties);
    Cu.makeObjectPropsNormal(contentObj);

    return contentObj;
  },

  


  notifyObservers: function CA_notifyObservers(aID, aLevel, aArguments) {
    if (!aID)
      return;

    let stack = this.getStackTrace();
    
    let frame = stack[1];
    let consoleEvent = {
      ID: aID,
      level: aLevel,
      filename: frame.filename,
      lineNumber: frame.lineNumber,
      functionName: frame.functionName,
      arguments: aArguments
    };

    consoleEvent.wrappedJSObject = consoleEvent;

    Services.obs.notifyObservers(consoleEvent,
                                 "console-api-log-event", aID);
  },

  






  getStackTrace: function CA_getStackTrace() {
    let stack = [];
    let frame = Components.stack.caller;
    while (frame = frame.caller) {
      if (frame.language == Ci.nsIProgrammingLanguage.JAVASCRIPT ||
          frame.language == Ci.nsIProgrammingLanguage.JAVASCRIPT2) {
        stack.push({
          filename: frame.filename,
          lineNumber: frame.lineNumber,
          functionName: frame.name,
          language: frame.language,
        });
      }
    }

    return stack;
  }
};

let NSGetFactory = XPCOMUtils.generateNSGetFactory([ConsoleAPI]);
