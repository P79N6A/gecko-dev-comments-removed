





let Cu = Components.utils;
let Ci = Components.interfaces;
let Cc = Components.classes;


const MAX_PAGE_TIMERS = 10000;



const ARGUMENT_PATTERN = /%\d*\.?\d*([osdif])\b/g;



const DEFAULT_MAX_STACKTRACE_DEPTH = 200;



const CALL_DELAY = 15; 


const MESSAGES_IN_INTERVAL = 1500;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/ConsoleAPIStorage.jsm");
Cu.import("resource://gre/modules/PrivateBrowsingUtils.jsm");

let gTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);

function ConsoleAPI() {}
ConsoleAPI.prototype = {

  classID: Components.ID("{b49c18f8-3379-4fc0-8c90-d7772c1a9ff3}"),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIDOMGlobalPropertyInitializer]),

  _timerInitialized: false,
  _queuedCalls: null,
  _timerCallback: null,
  _destroyedWindows: null,

  
  init: function CA_init(aWindow) {
    Services.obs.addObserver(this, "xpcom-shutdown", false);
    Services.obs.addObserver(this, "inner-window-destroyed", false);

    let self = this;
    let chromeObject = {
      
      log: function CA_log() {
        self.queueCall("log", arguments, aWindow);
      },
      info: function CA_info() {
        self.queueCall("info", arguments, aWindow);
      },
      warn: function CA_warn() {
        self.queueCall("warn", arguments, aWindow);
      },
      error: function CA_error() {
        self.queueCall("error", arguments, aWindow);
      },
      debug: function CA_debug() {
        self.queueCall("debug", arguments, aWindow);
      },
      trace: function CA_trace() {
        self.queueCall("trace", arguments, aWindow);
      },
      
      dir: function CA_dir() {
        self.queueCall("dir", arguments, aWindow);
      },
      group: function CA_group() {
        self.queueCall("group", arguments, aWindow);
      },
      groupCollapsed: function CA_groupCollapsed() {
        self.queueCall("groupCollapsed", arguments, aWindow);
      },
      groupEnd: function CA_groupEnd() {
        self.queueCall("groupEnd", arguments, aWindow);
      },
      time: function CA_time() {
        self.queueCall("time", arguments, aWindow);
      },
      timeEnd: function CA_timeEnd() {
        self.queueCall("timeEnd", arguments, aWindow);
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
        groupEnd: "r",
        time: "r",
        timeEnd: "r"
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
      time: genPropDesc('time'),
      timeEnd: genPropDesc('timeEnd'),
      __noSuchMethod__: { enumerable: true, configurable: true, writable: true,
                          value: function() {} },
      __mozillaConsole__: { value: true }
    };

    Object.defineProperties(contentObj, properties);
    Cu.makeObjectPropsNormal(contentObj);

    this._queuedCalls = [];
    this._destroyedWindows = [];

    return contentObj;
  },

  observe: function CA_observe(aSubject, aTopic, aData)
  {
    if (aTopic == "xpcom-shutdown") {
      Services.obs.removeObserver(this, "xpcom-shutdown");
      Services.obs.removeObserver(this, "inner-window-destroyed");
      this._destroyedWindows = [];
      this._queuedCalls = [];
      gTimer = null;
    }
    else if (aTopic == "inner-window-destroyed") {
      let innerWindowID = aSubject.QueryInterface(Ci.nsISupportsPRUint64).data;
      delete this.timerRegistry[innerWindowID + ""];
      this._destroyedWindows.push(innerWindowID);
    }
  },

  









  queueCall: function CA_queueCall(aMethod, aArguments, aWindow)
  {
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
      return;
    }

    let metaForCall = {
      outerID: outerID,
      innerID: innerID,
      isPrivate: PrivateBrowsingUtils.isWindowPrivate(aWindow),
      timeStamp: Date.now(),
      stack: this.getStackTrace(aMethod != "trace" ? 1 : null),
    };

    this._queuedCalls.push([aMethod, aArguments, metaForCall]);

    if (!this._timerInitialized) {
      gTimer.initWithCallback(this._timerCallback.bind(this), CALL_DELAY,
                              Ci.nsITimer.TYPE_REPEATING_SLACK);
      this._timerInitialized = true;
    }
  },

  



  _timerCallback: function CA__timerCallback()
  {
    this._queuedCalls.splice(0, MESSAGES_IN_INTERVAL)
      .forEach(this._processQueuedCall, this);

    if (!this._queuedCalls.length) {
      this._timerInitialized = false;
      this._destroyedWindows = [];
      gTimer.cancel();
    }
  },

  






  _processQueuedCall: function CA__processQueuedItem(aCall)
  {
    let [method, args, meta] = aCall;

    let notifyMeta = {
      outerID: meta.outerID,
      innerID: meta.innerID,
      isPrivate: meta.isPrivate,
      timeStamp: meta.timeStamp,
      frame: meta.stack[0],
    };

    let notifyArguments = null;

    switch (method) {
      case "log":
      case "info":
      case "warn":
      case "error":
      case "debug":
        notifyArguments = this.processArguments(args);
        break;
      case "trace":
        notifyArguments = meta.stack;
        break;
      case "group":
      case "groupCollapsed":
        notifyArguments = this.beginGroup(args);
        break;
      case "groupEnd":
      case "dir":
        notifyArguments = args;
        break;
      case "time":
        notifyArguments = this.startTimer(meta.innerID, args[0], meta.timeStamp);
        break;
      case "timeEnd":
        notifyArguments = this.stopTimer(meta.innerID, args[0], meta.timeStamp);
        break;
      default:
        
        return;
    }

    this.notifyObservers(method, notifyArguments, notifyMeta);
  },

  














  notifyObservers: function CA_notifyObservers(aLevel, aArguments, aMeta) {
    let consoleEvent = {
      ID: aMeta.outerID,
      innerID: aMeta.innerID,
      level: aLevel,
      filename: aMeta.frame.filename,
      lineNumber: aMeta.frame.lineNumber,
      functionName: aMeta.frame.functionName,
      arguments: aArguments,
      timeStamp: aMeta.timeStamp,
    };

    consoleEvent.wrappedJSObject = consoleEvent;

    
    if (!aMeta.isPrivate && this._destroyedWindows.indexOf(aMeta.innerID) == -1) {
      ConsoleAPIStorage.recordEvent(aMeta.innerID, consoleEvent);
    }

    Services.obs.notifyObservers(consoleEvent, "console-api-log-event",
                                 aMeta.outerID);
  },

  









  processArguments: function CA_processArguments(aArguments) {
    if (aArguments.length < 2 || typeof aArguments[0] != "string") {
      return aArguments;
    }
    let args = Array.prototype.slice.call(aArguments);
    let format = args.shift();
    
    let processed = format.replace(ARGUMENT_PATTERN, function CA_PA_substitute(match, submatch) {
      switch (submatch) {
        case "o":
        case "s":
          return String(args.shift());
        case "d":
        case "i":
          return parseInt(args.shift());
        case "f":
          return parseFloat(args.shift());
        default:
          return submatch;
      };
    });
    args.unshift(processed);
    return args;
  },

  








  getStackTrace: function CA_getStackTrace(aMaxDepth) {
    if (!aMaxDepth) {
      aMaxDepth = DEFAULT_MAX_STACKTRACE_DEPTH;
    }

    let stack = [];
    let frame = Components.stack.caller.caller;
    while (frame = frame.caller) {
      if (frame.language == Ci.nsIProgrammingLanguage.JAVASCRIPT ||
          frame.language == Ci.nsIProgrammingLanguage.JAVASCRIPT2) {
        stack.push({
          filename: frame.filename,
          lineNumber: frame.lineNumber,
          functionName: frame.name,
          language: frame.language,
        });
        if (stack.length == aMaxDepth) {
          break;
        }
      }
    }

    return stack;
  },

  


  beginGroup: function CA_beginGroup() {
    return Array.prototype.join.call(arguments[0], " ");
  },

  






  timerRegistry: {},

  














  startTimer: function CA_startTimer(aWindowId, aName, aTimestamp) {
    if (!aName) {
        return;
    }
    let innerID = aWindowId + "";
    if (!this.timerRegistry[innerID]) {
        this.timerRegistry[innerID] = {};
    }
    let pageTimers = this.timerRegistry[innerID];
    if (Object.keys(pageTimers).length > MAX_PAGE_TIMERS - 1) {
        return { error: "maxTimersExceeded" };
    }
    let key = aWindowId + "-" + aName.toString();
    if (!pageTimers[key]) {
        pageTimers[key] = aTimestamp || Date.now();
    }
    return { name: aName, started: pageTimers[key] };
  },

  












  stopTimer: function CA_stopTimer(aWindowId, aName, aTimestamp) {
    if (!aName) {
        return;
    }
    let innerID = aWindowId + "";
    let pageTimers = this.timerRegistry[innerID];
    if (!pageTimers) {
        return;
    }
    let key = aWindowId + "-" + aName.toString();
    if (!pageTimers[key]) {
        return;
    }
    let duration = (aTimestamp || Date.now()) - pageTimers[key];
    delete pageTimers[key];
    return { name: aName, duration: duration };
  }
};

let NSGetFactory = XPCOMUtils.generateNSGetFactory([ConsoleAPI]);
