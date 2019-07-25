








































let Cu = Components.utils;
let Ci = Components.interfaces;
let Cc = Components.classes;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/ConsoleAPIStorage.jsm");

function ConsoleAPI() {}
ConsoleAPI.prototype = {

  classID: Components.ID("{b49c18f8-3379-4fc0-8c90-d7772c1a9ff3}"),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIDOMGlobalPropertyInitializer]),

  
  init: function CA_init(aWindow) {
    let outerID;
    let innerID;
    try {
      let windowUtils = aWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                          .getInterface(Ci.nsIDOMWindowUtils);

      outerID = windowUtils.outerWindowID;
      innerID = windowUtils.currentInnerWindowID;
    }
    catch (ex) {
      Cu.reportError(ex);
    }

    let self = this;
    let chromeObject = {
      
      log: function CA_log() {
        self.notifyObservers(outerID, innerID, "log", self.processArguments(arguments));
      },
      info: function CA_info() {
        self.notifyObservers(outerID, innerID, "info", self.processArguments(arguments));
      },
      warn: function CA_warn() {
        self.notifyObservers(outerID, innerID, "warn", self.processArguments(arguments));
      },
      error: function CA_error() {
        self.notifyObservers(outerID, innerID, "error", self.processArguments(arguments));
      },
      debug: function CA_debug() {
        self.notifyObservers(outerID, innerID, "log", self.processArguments(arguments));
      },
      trace: function CA_trace() {
        self.notifyObservers(outerID, innerID, "trace", self.getStackTrace());
      },
      
      dir: function CA_dir() {
        self.notifyObservers(outerID, innerID, "dir", arguments);
      },
      group: function CA_group() {
        self.notifyObservers(outerID, innerID, "group", self.beginGroup(arguments));
      },
      groupCollapsed: function CA_groupCollapsed() {
        self.notifyObservers(outerID, innerID, "groupCollapsed", self.beginGroup(arguments));
      },
      groupEnd: function CA_groupEnd() {
        self.notifyObservers(outerID, innerID, "groupEnd", arguments);
      },
      __exposedProps__: {
        log: "r",
        info: "r",
        warn: "r",
        error: "r",
        debug: "r",
        trace: "r",
        dir: "r",
        group: "r",
        groupCollapsed: "r",
        groupEnd: "r"
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
      group: genPropDesc('group'),
      groupCollapsed: genPropDesc('groupCollapsed'),
      groupEnd: genPropDesc('groupEnd'),
      __noSuchMethod__: { enumerable: true, configurable: true, writable: true,
                          value: function() {} },
      __mozillaConsole__: { value: true }
    };

    Object.defineProperties(contentObj, properties);
    Cu.makeObjectPropsNormal(contentObj);

    return contentObj;
  },

  











  notifyObservers:
  function CA_notifyObservers(aOuterWindowID, aInnerWindowID, aLevel, aArguments) {
    if (!aOuterWindowID) {
      return;
    }

    let stack = this.getStackTrace();
    
    let frame = stack[1];
    let consoleEvent = {
      ID: aOuterWindowID,
      innerID: aInnerWindowID,
      level: aLevel,
      filename: frame.filename,
      lineNumber: frame.lineNumber,
      functionName: frame.functionName,
      arguments: aArguments
    };

    consoleEvent.wrappedJSObject = consoleEvent;

    ConsoleAPIStorage.recordEvent(aInnerWindowID, consoleEvent);

    Services.obs.notifyObservers(consoleEvent,
                                 "console-api-log-event", aOuterWindowID);
  },

  









  processArguments: function CA_processArguments(aArguments) {
    if (aArguments.length < 2) {
      return aArguments;
    }
    let args = Array.prototype.slice.call(aArguments);
    let format = args.shift();
    
    let pattern = /%(\d*).?(\d*)[a-zA-Z]/g;
    let processed = format.replace(pattern, function CA_PA_substitute(spec) {
      switch (spec[spec.length-1]) {
        case "o":
        case "s":
          return args.shift().toString();
        case "d":
        case "i":
          return parseInt(args.shift());
        case "f":
          return parseFloat(args.shift());
        default:
          return spec;
      };
    });
    args.unshift(processed);
    return args;
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
  },

  


  beginGroup: function CA_beginGroup() {
    return Array.prototype.join.call(arguments[0], " ");
  }
};

let NSGetFactory = XPCOMUtils.generateNSGetFactory([ConsoleAPI]);
