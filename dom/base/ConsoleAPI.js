





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





function ConsoleAPI() {}
ConsoleAPI.prototype = {

  classID: Components.ID("{b49c18f8-3379-4fc0-8c90-d7772c1a9ff3}"),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIDOMGlobalPropertyInitializer,
                                         Ci.nsISupportsWeakReference,
                                         Ci.nsIObserver]),

  _timerInitialized: false,
  _queuedCalls: null,
  _window: null,
  _innerID: null,
  _outerID: null,
  _windowDestroyed: false,
  _timer: null,

  
  init: function CA_init(aWindow) {
    Services.obs.addObserver(this, "inner-window-destroyed", true);

    try {
      let windowUtils = aWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                          .getInterface(Ci.nsIDOMWindowUtils);

      this._outerID = windowUtils.outerWindowID;
      this._innerID = windowUtils.currentInnerWindowID;
    }
    catch (ex) {
      Cu.reportError(ex);
    }

    let self = this;
    let chromeObject = {
      
      log: function CA_log() {
        self.queueCall("log", arguments);
      },
      info: function CA_info() {
        self.queueCall("info", arguments);
      },
      warn: function CA_warn() {
        self.queueCall("warn", arguments);
      },
      error: function CA_error() {
        self.queueCall("error", arguments);
      },
      debug: function CA_debug() {
        self.queueCall("debug", arguments);
      },
      trace: function CA_trace() {
        self.queueCall("trace", arguments);
      },
      
      dir: function CA_dir() {
        self.queueCall("dir", arguments);
      },
      group: function CA_group() {
        self.queueCall("group", arguments);
      },
      groupCollapsed: function CA_groupCollapsed() {
        self.queueCall("groupCollapsed", arguments);
      },
      groupEnd: function CA_groupEnd() {
        self.queueCall("groupEnd", arguments);
      },
      time: function CA_time() {
        self.queueCall("time", arguments);
      },
      timeEnd: function CA_timeEnd() {
        self.queueCall("timeEnd", arguments);
      },
      profile: function CA_profile() {
        
        
        let consoleEvent = {
          action: "profile",
          arguments: arguments
        };
        consoleEvent.wrappedJSObject = consoleEvent;
        Services.obs.notifyObservers(consoleEvent, "console-api-profiler",
                                     null);  
      },
      profileEnd: function CA_profileEnd() {
        
        
        let consoleEvent = {
          action: "profileEnd",
          arguments: arguments
        };
        consoleEvent.wrappedJSObject = consoleEvent;
        Services.obs.notifyObservers(consoleEvent, "console-api-profiler",
                                     null);  
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
        timeEnd: "r",
        profile: "r",
        profileEnd: "r"
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
      profile: genPropDesc('profile'),
      profileEnd: genPropDesc('profileEnd'),
      __noSuchMethod__: { enumerable: true, configurable: true, writable: true,
                          value: function() {} },
      __mozillaConsole__: { value: true }
    };

    Object.defineProperties(contentObj, properties);
    Cu.makeObjectPropsNormal(contentObj);

    this._queuedCalls = [];
    this._timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    this._window = Cu.getWeakReference(aWindow);
    this.timerRegistry = {};

    return contentObj;
  },

  observe: function CA_observe(aSubject, aTopic, aData)
  {
    if (aTopic == "inner-window-destroyed") {
      let innerWindowID = aSubject.QueryInterface(Ci.nsISupportsPRUint64).data;
      if (innerWindowID == this._innerID) {
        Services.obs.removeObserver(this, "inner-window-destroyed");
        this._windowDestroyed = true;
        if (!this._timerInitialized) {
          this.timerRegistry = {};
        }
      }
    }
  },

  







  queueCall: function CA_queueCall(aMethod, aArguments)
  {
    let metaForCall = {
      isPrivate: PrivateBrowsingUtils.isWindowPrivate(this._window.get()),
      timeStamp: Date.now(),
      stack: this.getStackTrace(aMethod != "trace" ? 1 : null),
    };

    this._queuedCalls.push([aMethod, aArguments, metaForCall]);

    if (!this._timerInitialized) {
      this._timer.initWithCallback(this._timerCallback.bind(this), CALL_DELAY,
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
      this._timer.cancel();

      if (this._windowDestroyed) {
        ConsoleAPIStorage.clearEvents(this._innerID);
        this.timerRegistry = {};
      }
    }
  },

  






  _processQueuedCall: function CA__processQueuedItem(aCall)
  {
    let [method, args, meta] = aCall;

    let frame = meta.stack[0];
    let consoleEvent = {
      ID: this._outerID,
      innerID: this._innerID,
      level: method,
      filename: frame.filename,
      lineNumber: frame.lineNumber,
      functionName: frame.functionName,
      timeStamp: meta.timeStamp,
      arguments: args,
    };

    switch (method) {
      case "log":
      case "info":
      case "warn":
      case "error":
      case "debug":
        consoleEvent.arguments = this.processArguments(args);
        break;
      case "trace":
        consoleEvent.stacktrace = meta.stack;
        break;
      case "group":
      case "groupCollapsed":
      case "groupEnd":
        try {
          consoleEvent.groupName = Array.prototype.join.call(args, " ");
        }
        catch (ex) {
          Cu.reportError(ex);
          Cu.reportError(ex.stack);
          return;
        }
        break;
      case "dir":
        break;
      case "time":
        consoleEvent.timer = this.startTimer(args[0], meta.timeStamp);
        break;
      case "timeEnd":
        consoleEvent.timer = this.stopTimer(args[0], meta.timeStamp);
        break;
      default:
        
        return;
    }

    this.notifyObservers(method, consoleEvent, meta.isPrivate);
  },

  










  notifyObservers: function CA_notifyObservers(aLevel, aConsoleEvent, aPrivate)
  {
    aConsoleEvent.wrappedJSObject = aConsoleEvent;

    
    if (!aPrivate) {
      ConsoleAPIStorage.recordEvent(this._innerID, aConsoleEvent);
    }

    Services.obs.notifyObservers(aConsoleEvent, "console-api-log-event",
                                 this._outerID);
  },

  









  processArguments: function CA_processArguments(aArguments) {
    if (aArguments.length < 2 || typeof aArguments[0] != "string") {
      return aArguments;
    }

    let args = Array.prototype.slice.call(aArguments);
    let format = args.shift();
    let splitter = "%" + format.length + Date.now() + "%";
    let objects = [];

    
    let processed = format.replace(ARGUMENT_PATTERN, function CA_PA_substitute(match, submatch) {
      switch (submatch) {
        case "o":
          objects.push(args.shift());
          return splitter;
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

    let result = [];
    let processedArray = processed.split(splitter);
    processedArray.forEach(function(aValue, aIndex) {
      if (aValue !== "") {
        result.push(aValue);
      }
      if (objects[aIndex]) {
        result.push(objects[aIndex]);
      }
    });

    return result.concat(args);
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

  





  timerRegistry: null,

  












  startTimer: function CA_startTimer(aName, aTimestamp) {
    if (!aName) {
        return;
    }
    if (Object.keys(this.timerRegistry).length > MAX_PAGE_TIMERS - 1) {
        return { error: "maxTimersExceeded" };
    }
    let key = this._innerID + "-" + aName.toString();
    if (!(key in this.timerRegistry)) {
        this.timerRegistry[key] = aTimestamp || Date.now();
    }
    return { name: aName, started: this.timerRegistry[key] };
  },

  










  stopTimer: function CA_stopTimer(aName, aTimestamp) {
    if (!aName) {
        return;
    }
    let key = this._innerID + "-" + aName.toString();
    if (!(key in this.timerRegistry)) {
        return;
    }
    let duration = (aTimestamp || Date.now()) - this.timerRegistry[key];
    delete this.timerRegistry[key];
    return { name: aName, duration: duration };
  }
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([ConsoleAPI]);
