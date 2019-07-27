





"use strict";

const Services = require("Services");
const { Cc, Ci, Cu, components, ChromeWorker } = require("chrome");
const { ActorPool, getOffsetColumn } = require("devtools/server/actors/common");
const { DebuggerServer } = require("devtools/server/main");
const DevToolsUtils = require("devtools/toolkit/DevToolsUtils");
const { dbg_assert, dumpn, update, fetch } = DevToolsUtils;
const { SourceMapConsumer, SourceMapGenerator } = require("source-map");
const promise = require("promise");
const PromiseDebugging = require("PromiseDebugging");
const Debugger = require("Debugger");
const xpcInspector = require("xpcInspector");
const mapURIToAddonID = require("./utils/map-uri-to-addon-id");

const { defer, resolve, reject, all } = require("devtools/toolkit/deprecated-sync-thenables");
const { CssLogic } = require("devtools/styleinspector/css-logic");

let TYPED_ARRAY_CLASSES = ["Uint8Array", "Uint8ClampedArray", "Uint16Array",
      "Uint32Array", "Int8Array", "Int16Array", "Int32Array", "Float32Array",
      "Float64Array"];



let OBJECT_PREVIEW_MAX_ITEMS = 10;













Debugger.Object.prototype.getPromiseState = function () {
  if (this.class != "Promise") {
    throw new Error(
      "Can't call `getPromiseState` on `Debugger.Object`s that don't " +
      "refer to Promise objects.");
  }

  const state = PromiseDebugging.getState(this.unsafeDereference());
  return {
    state: state.state,
    value: this.makeDebuggeeValue(state.value),
    reason: this.makeDebuggeeValue(state.reason)
  };
};






function BreakpointStore() {
  this._size = 0;

  
  
  
  
  
  
  
  
  
  this._wholeLineBreakpoints = Object.create(null);

  
  
  
  
  
  
  
  
  
  this._breakpoints = Object.create(null);
}

BreakpointStore.prototype = {
  _size: null,
  get size() { return this._size; },

  














  addBreakpoint: function (aBreakpoint, aActor) {
    let { source: { actor }, line, column } = aBreakpoint;

    if (column != null) {
      if (!this._breakpoints[actor]) {
        this._breakpoints[actor] = [];
      }
      if (!this._breakpoints[actor][line]) {
        this._breakpoints[actor][line] = [];
      }

      if (!this._breakpoints[actor][line][column]) {
        this._breakpoints[actor][line][column] = aActor;
        this._size++;
      }
      return this._breakpoints[actor][line][column];
    } else {
      
      if (!this._wholeLineBreakpoints[actor]) {
        this._wholeLineBreakpoints[actor] = [];
      }

      if (!this._wholeLineBreakpoints[actor][line]) {
        this._wholeLineBreakpoints[actor][line] = aActor;
        this._size++;
      }
      return this._wholeLineBreakpoints[actor][line];
    }
  },

  









  removeBreakpoint: function ({ source: { actor }, line, column }) {
    if (column != null) {
      if (this._breakpoints[actor]) {
        if (this._breakpoints[actor][line]) {
          if (this._breakpoints[actor][line][column]) {
            delete this._breakpoints[actor][line][column];
            this._size--;

            
            
            
            
            
            
            
            if (Object.keys(this._breakpoints[actor][line]).length === 0) {
              delete this._breakpoints[actor][line];
            }
          }
        }
      }
    } else {
      if (this._wholeLineBreakpoints[actor]) {
        if (this._wholeLineBreakpoints[actor][line]) {
          delete this._wholeLineBreakpoints[actor][line];
          this._size--;
        }
      }
    }
  },

  










  getBreakpoint: function (aLocation) {
    let { source: { actor }, line, column } = aLocation;
    dbg_assert(actor != null);
    dbg_assert(line != null);
    for (let actor of this.findBreakpoints(aLocation)) {
      
      
      
      
      return actor;
    }

    return null;
  },

  










  findBreakpoints: function* (aSearchParams={}) {
    if (aSearchParams.column != null) {
      dbg_assert(aSearchParams.line != null);
    }
    if (aSearchParams.line != null) {
      dbg_assert(aSearchParams.source != null);
      dbg_assert(aSearchParams.source.actor != null);
    }

    let actor = aSearchParams.source ? aSearchParams.source.actor : null;
    for (let actor of this._iterActors(actor)) {
      for (let line of this._iterLines(actor, aSearchParams.line)) {
        
        
        if (aSearchParams.column == null
            && this._wholeLineBreakpoints[actor]
            && this._wholeLineBreakpoints[actor][line]) {
          yield this._wholeLineBreakpoints[actor][line];
        }
        for (let column of this._iterColumns(actor, line, aSearchParams.column)) {
          yield this._breakpoints[actor][line][column];
        }
      }
    }
  },

  _iterActors: function* (aActor) {
    if (aActor) {
      if (this._breakpoints[aActor] || this._wholeLineBreakpoints[aActor]) {
        yield aActor;
      }
    } else {
      for (let actor of Object.keys(this._wholeLineBreakpoints)) {
        yield actor;
      }
      for (let actor of Object.keys(this._breakpoints)) {
        if (actor in this._wholeLineBreakpoints) {
          continue;
        }
        yield actor;
      }
    }
  },

  _iterLines: function* (aActor, aLine) {
    if (aLine != null) {
      if ((this._wholeLineBreakpoints[aActor]
           && this._wholeLineBreakpoints[aActor][aLine])
          || (this._breakpoints[aActor] && this._breakpoints[aActor][aLine])) {
        yield aLine;
      }
    } else {
      const wholeLines = this._wholeLineBreakpoints[aActor]
        ? Object.keys(this._wholeLineBreakpoints[aActor])
        : [];
      const columnLines = this._breakpoints[aActor]
        ? Object.keys(this._breakpoints[aActor])
        : [];

      const lines = wholeLines.concat(columnLines).sort();

      let lastLine;
      for (let line of lines) {
        if (line === lastLine) {
          continue;
        }
        yield line;
        lastLine = line;
      }
    }
  },

  _iterColumns: function* (aActor, aLine, aColumn) {
    if (!this._breakpoints[aActor] || !this._breakpoints[aActor][aLine]) {
      return;
    }

    if (aColumn != null) {
      if (this._breakpoints[aActor][aLine][aColumn]) {
        yield aColumn;
      }
    } else {
      for (let column in this._breakpoints[aActor][aLine]) {
        yield column;
      }
    }
  },
};

exports.BreakpointStore = BreakpointStore;







function SourceActorStore() {
  
  this._sourceActorIds = Object.create(null);
}

SourceActorStore.prototype = {
  


  getReusableActorId: function(aSource, aOriginalUrl) {
    let url = this.getUniqueKey(aSource, aOriginalUrl);
    if (url && url in this._sourceActorIds) {
      return this._sourceActorIds[url];
    }
    return null;
  },

  


  setReusableActorId: function(aSource, aOriginalUrl, actorID) {
    let url = this.getUniqueKey(aSource, aOriginalUrl);
    if (url) {
      this._sourceActorIds[url] = actorID;
    }
  },

  


  getUniqueKey: function(aSource, aOriginalUrl) {
    if (aOriginalUrl) {
      
      return aOriginalUrl;
    }
    else {
      return getSourceURL(aSource);
    }
  }
};

exports.SourceActorStore = SourceActorStore;
















function EventLoopStack({ thread, connection, hooks }) {
  this._hooks = hooks;
  this._thread = thread;
  this._connection = connection;
}

EventLoopStack.prototype = {
  


  get size() {
    return xpcInspector.eventLoopNestLevel;
  },

  


  get lastPausedUrl() {
    let url = null;
    if (this.size > 0) {
      try {
        url = xpcInspector.lastNestRequestor.url
      } catch (e) {
        
        
        dumpn(e);
      }
    }
    return url;
  },

  



  get lastConnection() {
    return xpcInspector.lastNestRequestor._connection;
  },

  




  push: function () {
    return new EventLoop({
      thread: this._thread,
      connection: this._connection,
      hooks: this._hooks
    });
  }
};













function EventLoop({ thread, connection, hooks }) {
  this._thread = thread;
  this._hooks = hooks;
  this._connection = connection;

  this.enter = this.enter.bind(this);
  this.resolve = this.resolve.bind(this);
}

EventLoop.prototype = {
  entered: false,
  resolved: false,
  get url() { return this._hooks.url; },

  


  enter: function () {
    let nestData = this._hooks.preNest
      ? this._hooks.preNest()
      : null;

    this.entered = true;
    xpcInspector.enterNestedEventLoop(this);

    
    if (xpcInspector.eventLoopNestLevel > 0) {
      const { resolved } = xpcInspector.lastNestRequestor;
      if (resolved) {
        xpcInspector.exitNestedEventLoop();
      }
    }

    dbg_assert(this._thread.state === "running",
               "Should be in the running state");

    if (this._hooks.postNest) {
      this._hooks.postNest(nestData);
    }
  },

  







  resolve: function () {
    if (!this.entered) {
      throw new Error("Can't resolve an event loop before it has been entered!");
    }
    if (this.resolved) {
      throw new Error("Already resolved this nested event loop!");
    }
    this.resolved = true;
    if (this === xpcInspector.lastNestRequestor) {
      xpcInspector.exitNestedEventLoop();
      return true;
    }
    return false;
  },
};
























function ThreadActor(aParent, aGlobal)
{
  this._state = "detached";
  this._frameActors = [];
  this._parent = aParent;
  this._dbg = null;
  this._gripDepth = 0;
  this._threadLifetimePool = null;
  this._tabClosed = false;

  this._options = {
    useSourceMaps: false,
    autoBlackBox: false
  };

  this.breakpointStore = new BreakpointStore();
  this.sourceActorStore = new SourceActorStore();
  this.blackBoxedSources = new Set(["self-hosted"]);
  this.prettyPrintedSources = new Map();

  
  
  this._hiddenBreakpoints = new Map();

  this.global = aGlobal;

  this._allEventsListener = this._allEventsListener.bind(this);
  this.onNewGlobal = this.onNewGlobal.bind(this);
  this.onNewSource = this.onNewSource.bind(this);
  this.uncaughtExceptionHook = this.uncaughtExceptionHook.bind(this);
  this.onDebuggerStatement = this.onDebuggerStatement.bind(this);
  this.onNewScript = this.onNewScript.bind(this);
  
  
  this.wrappedJSObject = this;
}

ThreadActor.prototype = {
  
  _gripDepth: null,

  actorPrefix: "context",

  get dbg() {
    if (!this._dbg) {
      this._dbg = this._parent.makeDebugger();
      this._dbg.uncaughtExceptionHook = this.uncaughtExceptionHook;
      this._dbg.onDebuggerStatement = this.onDebuggerStatement;
      this._dbg.onNewScript = this.onNewScript;
      this._dbg.on("newGlobal", this.onNewGlobal);
      
      this._dbg.enabled = this._state != "detached";
    }
    return this._dbg;
  },

  get globalDebugObject() {
    return this.dbg.makeGlobalObjectReference(this._parent.window);
  },

  get state() { return this._state; },
  get attached() this.state == "attached" ||
                 this.state == "running" ||
                 this.state == "paused",

  get threadLifetimePool() {
    if (!this._threadLifetimePool) {
      this._threadLifetimePool = new ActorPool(this.conn);
      this.conn.addActorPool(this._threadLifetimePool);
      this._threadLifetimePool.objectActors = new WeakMap();
    }
    return this._threadLifetimePool;
  },

  get sources() {
    if (!this._sources) {
      this._sources = new ThreadSources(this, this._options,
                                        this._allowSource, this.onNewSource);
    }
    return this._sources;
  },

  get youngestFrame() {
    if (this.state != "paused") {
      return null;
    }
    return this.dbg.getNewestFrame();
  },

  _prettyPrintWorker: null,
  get prettyPrintWorker() {
    if (!this._prettyPrintWorker) {
      this._prettyPrintWorker = new ChromeWorker(
        "resource://gre/modules/devtools/server/actors/pretty-print-worker.js");

      this._prettyPrintWorker.addEventListener(
        "error", this._onPrettyPrintError, false);

      if (dumpn.wantLogging) {
        this._prettyPrintWorker.addEventListener("message", this._onPrettyPrintMsg, false);

        const postMsg = this._prettyPrintWorker.postMessage;
        this._prettyPrintWorker.postMessage = data => {
          dumpn("Sending message to prettyPrintWorker: "
                + JSON.stringify(data, null, 2) + "\n");
          return postMsg.call(this._prettyPrintWorker, data);
        };
      }
    }
    return this._prettyPrintWorker;
  },

  _onPrettyPrintError: function ({ message, filename, lineno }) {
    reportError(new Error(message + " @ " + filename + ":" + lineno));
  },

  _onPrettyPrintMsg: function ({ data }) {
    dumpn("Received message from prettyPrintWorker: "
          + JSON.stringify(data, null, 2) + "\n");
  },

  





  _threadPauseEventLoops: null,
  _pushThreadPause: function () {
    if (!this._threadPauseEventLoops) {
      this._threadPauseEventLoops = [];
    }
    const eventLoop = this._nestedEventLoops.push();
    this._threadPauseEventLoops.push(eventLoop);
    eventLoop.enter();
  },
  _popThreadPause: function () {
    const eventLoop = this._threadPauseEventLoops.pop();
    dbg_assert(eventLoop, "Should have an event loop.");
    eventLoop.resolve();
  },

  


  clearDebuggees: function () {
    if (this._dbg) {
      this.dbg.removeAllDebuggees();
    }
    this._sources = null;
  },

  


  onNewGlobal: function (aGlobal) {
    
    this.conn.send({
      from: this.actorID,
      type: "newGlobal",
      
      hostAnnotations: aGlobal.hostAnnotations
    });
  },

  disconnect: function () {
    dumpn("in ThreadActor.prototype.disconnect");
    if (this._state == "paused") {
      this.onResume();
    }

    
    
    
    this._sourceActorStore = null;

    this.clearDebuggees();
    this.conn.removeActorPool(this._threadLifetimePool);
    this._threadLifetimePool = null;

    if (this._prettyPrintWorker) {
      this._prettyPrintWorker.removeEventListener(
        "error", this._onPrettyPrintError, false);
      this._prettyPrintWorker.removeEventListener(
        "message", this._onPrettyPrintMsg, false);
      this._prettyPrintWorker.terminate();
      this._prettyPrintWorker = null;
    }

    if (!this._dbg) {
      return;
    }
    this._dbg.enabled = false;
    this._dbg = null;
  },

  


  exit: function () {
    this.disconnect();
    this._state = "exited";
  },

  
  onAttach: function (aRequest) {
    if (this.state === "exited") {
      return { type: "exited" };
    }

    if (this.state !== "detached") {
      return { error: "wrongState",
               message: "Current state is " + this.state };
    }

    this._state = "attached";

    update(this._options, aRequest.options || {});

    
    
    this._nestedEventLoops = new EventLoopStack({
      hooks: this._parent,
      connection: this.conn,
      thread: this
    });

    this.dbg.addDebuggees();
    this.dbg.enabled = true;
    try {
      
      let packet = this._paused();
      if (!packet) {
        return { error: "notAttached" };
      }
      packet.why = { type: "attached" };

      this._restoreBreakpoints();

      
      
      
      this.conn.send(packet);

      
      this._pushThreadPause();

      
      
      return null;
    } catch (e) {
      reportError(e);
      return { error: "notAttached", message: e.toString() };
    }
  },

  onDetach: function (aRequest) {
    this.disconnect();
    this._state = "detached";

    dumpn("ThreadActor.prototype.onDetach: returning 'detached' packet");
    return {
      type: "detached"
    };
  },

  onReconfigure: function (aRequest) {
    if (this.state == "exited") {
      return { error: "wrongState" };
    }

    update(this._options, aRequest.options || {});
    
    this._sources = null;

    return {};
  },

  











  _pauseAndRespond: function (aFrame, aReason, onPacket=function (k) { return k; }) {
    try {
      let packet = this._paused(aFrame);
      if (!packet) {
        return undefined;
      }
      packet.why = aReason;

      let loc = getFrameLocation(aFrame);
      this.sources.getOriginalLocation(loc).then(aOrigPosition => {
        if (!aOrigPosition.sourceActor) {
          
          
          
          
          DevToolsUtils.reportException(
            'ThreadActor',
            new Error('Attempted to pause in a script with a sourcemap but ' +
                      'could not find original location.')
          );

          return undefined;
        }

        packet.frame.where = {
          source: aOrigPosition.sourceActor.form(),
          line: aOrigPosition.line,
          column: aOrigPosition.column
        };
        resolve(onPacket(packet))
          .then(null, error => {
            reportError(error);
            return {
              error: "unknownError",
              message: error.message + "\n" + error.stack
            };
          })
          .then(packet => {
            this.conn.send(packet);
          });
      });

      this._pushThreadPause();
    } catch(e) {
      reportError(e, "Got an exception during TA__pauseAndRespond: ");
    }

    
    
    
    return this._tabClosed ? null : undefined;
  },

  






  _forceCompletion: function (aRequest) {
    
    
    return {
      error: "notImplemented",
      message: "forced completion is not yet implemented."
    };
  },

  _makeOnEnterFrame: function ({ pauseAndRespond }) {
    return aFrame => {
      const generatedLocation = getFrameLocation(aFrame);
      let { sourceActor } = this.synchronize(this.sources.getOriginalLocation(
        generatedLocation));
      let url = sourceActor.url;

      return this.sources.isBlackBoxed(url)
        ? undefined
        : pauseAndRespond(aFrame);
    };
  },

  _makeOnPop: function ({ thread, pauseAndRespond, createValueGrip }) {
    return function (aCompletion) {
      

      const generatedLocation = getFrameLocation(this);
      const { sourceActor } = thread.synchronize(thread.sources.getOriginalLocation(
        generatedLocation));
      const url = sourceActor.url;

      if (thread.sources.isBlackBoxed(url)) {
        return undefined;
      }

      
      
      this.reportedPop = true;

      return pauseAndRespond(this, aPacket => {
        aPacket.why.frameFinished = {};
        if (!aCompletion) {
          aPacket.why.frameFinished.terminated = true;
        } else if (aCompletion.hasOwnProperty("return")) {
          aPacket.why.frameFinished.return = createValueGrip(aCompletion.return);
        } else if (aCompletion.hasOwnProperty("yield")) {
          aPacket.why.frameFinished.return = createValueGrip(aCompletion.yield);
        } else {
          aPacket.why.frameFinished.throw = createValueGrip(aCompletion.throw);
        }
        return aPacket;
      });
    };
  },

  _makeOnStep: function ({ thread, pauseAndRespond, startFrame,
                           startLocation, steppingType }) {
    
    if (steppingType === "break") {
      return function () {
        return pauseAndRespond(this);
      };
    }

    
    return function () {
      

      const generatedLocation = getFrameLocation(this);
      const newLocation = thread.synchronize(thread.sources.getOriginalLocation(
        generatedLocation));

      
      
      
      
      
      
      
      
      
      
      
      
      
      

      
      if (newLocation.url == null
          || thread.sources.isBlackBoxed(newLocation.url)) {
        return undefined;
      }

      
      if (this !== startFrame
          || startLocation.url !== newLocation.url
          || startLocation.line !== newLocation.line) {
        return pauseAndRespond(this);
      }

      
      
      return undefined;
    };
  },

  


  _makeSteppingHooks: function (aStartLocation, steppingType) {
    
    
    
    
    const steppingHookState = {
      pauseAndRespond: (aFrame, onPacket=(k)=>k) => {
        return this._pauseAndRespond(aFrame, { type: "resumeLimit" }, onPacket);
      },
      createValueGrip: this.createValueGrip.bind(this),
      thread: this,
      startFrame: this.youngestFrame,
      startLocation: aStartLocation,
      steppingType: steppingType
    };

    return {
      onEnterFrame: this._makeOnEnterFrame(steppingHookState),
      onPop: this._makeOnPop(steppingHookState),
      onStep: this._makeOnStep(steppingHookState)
    };
  },

  








  _handleResumeLimit: function (aRequest) {
    let steppingType = aRequest.resumeLimit.type;
    if (["break", "step", "next", "finish"].indexOf(steppingType) == -1) {
      return reject({ error: "badParameterType",
                      message: "Unknown resumeLimit type" });
    }

    const generatedLocation = getFrameLocation(this.youngestFrame);
    return this.sources.getOriginalLocation(generatedLocation)
      .then(originalLocation => {
        const { onEnterFrame, onPop, onStep } = this._makeSteppingHooks(originalLocation,
                                                                        steppingType);

        
        
        let stepFrame = this._getNextStepFrame(this.youngestFrame);
        if (stepFrame) {
          switch (steppingType) {
            case "step":
              this.dbg.onEnterFrame = onEnterFrame;
              
            case "break":
            case "next":
              if (stepFrame.script) {
                  stepFrame.onStep = onStep;
              }
              stepFrame.onPop = onPop;
              break;
            case "finish":
              stepFrame.onPop = onPop;
          }
        }

        return true;
      });
  },

  






  _clearSteppingHooks: function (aFrame) {
    if (aFrame && aFrame.live) {
      while (aFrame) {
        aFrame.onStep = undefined;
        aFrame.onPop = undefined;
        aFrame = aFrame.older;
      }
    }
  },

  





  _maybeListenToEvents: function (aRequest) {
    
    let events = aRequest.pauseOnDOMEvents;
    if (this.global && events &&
        (events == "*" ||
        (Array.isArray(events) && events.length))) {
      this._pauseOnDOMEvents = events;
      let els = Cc["@mozilla.org/eventlistenerservice;1"]
                .getService(Ci.nsIEventListenerService);
      els.addListenerForAllEvents(this.global, this._allEventsListener, true);
    }
  },

  


  onResume: function (aRequest) {
    if (this._state !== "paused") {
      return {
        error: "wrongState",
        message: "Can't resume when debuggee isn't paused. Current state is '"
          + this._state + "'"
      };
    }

    
    
    
    if (this._nestedEventLoops.size && this._nestedEventLoops.lastPausedUrl
        && (this._nestedEventLoops.lastPausedUrl !== this._parent.url
            || this._nestedEventLoops.lastConnection !== this.conn)) {
      return {
        error: "wrongOrder",
        message: "trying to resume in the wrong order.",
        lastPausedUrl: this._nestedEventLoops.lastPausedUrl
      };
    }

    if (aRequest && aRequest.forceCompletion) {
      return this._forceCompletion(aRequest);
    }

    let resumeLimitHandled;
    if (aRequest && aRequest.resumeLimit) {
      resumeLimitHandled = this._handleResumeLimit(aRequest)
    } else {
      this._clearSteppingHooks(this.youngestFrame);
      resumeLimitHandled = resolve(true);
    }

    return resumeLimitHandled.then(() => {
      if (aRequest) {
        this._options.pauseOnExceptions = aRequest.pauseOnExceptions;
        this._options.ignoreCaughtExceptions = aRequest.ignoreCaughtExceptions;
        this.maybePauseOnExceptions();
        this._maybeListenToEvents(aRequest);
      }

      let packet = this._resumed();
      this._popThreadPause();
      
      
      if (Services.obs) {
        Services.obs.notifyObservers(this, "devtools-thread-resumed", null);
      }
      return packet;
    }, error => {
      return error instanceof Error
        ? { error: "unknownError",
            message: DevToolsUtils.safeErrorString(error) }
        
        
        : error;
    });
  },

  






  synchronize: function(aPromise) {
    let needNest = true;
    let eventLoop;
    let returnVal;

    aPromise
      .then((aResolvedVal) => {
        needNest = false;
        returnVal = aResolvedVal;
      })
      .then(null, (aError) => {
        reportError(aError, "Error inside synchronize:");
      })
      .then(() => {
        if (eventLoop) {
          eventLoop.resolve();
        }
      });

    if (needNest) {
      eventLoop = this._nestedEventLoops.push();
      eventLoop.enter();
    }

    return returnVal;
  },

  


  maybePauseOnExceptions: function() {
    if (this._options.pauseOnExceptions) {
      this.dbg.onExceptionUnwind = this.onExceptionUnwind.bind(this);
    }
  },

  








  _allEventsListener: function(event) {
    if (this._pauseOnDOMEvents == "*" ||
        this._pauseOnDOMEvents.indexOf(event.type) != -1) {
      for (let listener of this._getAllEventListeners(event.target)) {
        if (event.type == listener.type || this._pauseOnDOMEvents == "*") {
          this._breakOnEnter(listener.script);
        }
      }
    }
  },

  







  _getAllEventListeners: function(eventTarget) {
    let els = Cc["@mozilla.org/eventlistenerservice;1"]
                .getService(Ci.nsIEventListenerService);

    let targets = els.getEventTargetChainFor(eventTarget);
    let listeners = [];

    for (let target of targets) {
      let handlers = els.getListenerInfoFor(target);
      for (let handler of handlers) {
        
        
        
        if (!handler || !handler.listenerObject || !handler.type)
          continue;
        
        let l = Object.create(null);
        l.type = handler.type;
        let listener = handler.listenerObject;
        let listenerDO = this.globalDebugObject.makeDebuggeeValue(listener);
        
        if (listenerDO.class == "Object" || listenerDO.class == "XULElement") {
          
          
          if (!listenerDO.unwrap()) {
            continue;
          }
          let heDesc;
          while (!heDesc && listenerDO) {
            heDesc = listenerDO.getOwnPropertyDescriptor("handleEvent");
            listenerDO = listenerDO.proto;
          }
          if (heDesc && heDesc.value) {
            listenerDO = heDesc.value;
          }
        }
        
        
        while (listenerDO.isBoundFunction) {
          listenerDO = listenerDO.boundTargetFunction;
        }
        l.script = listenerDO.script;
        
        
        if (!l.script)
          continue;
        listeners.push(l);
      }
    }
    return listeners;
  },

  


  _breakOnEnter: function(script) {
    let offsets = script.getAllOffsets();
    let sourceActor = this.sources.source({ source: script.source });

    for (let line = 0, n = offsets.length; line < n; line++) {
      if (offsets[line]) {
        let location = { line: line };
        let resp = sourceActor._setBreakpoint(location);
        dbg_assert(!resp.actualLocation, "No actualLocation should be returned");
        if (resp.error) {
          reportError(new Error("Unable to set breakpoint on event listener"));
          return;
        }
        let bpActor = this.breakpointStore.getBreakpoint({
          source: sourceActor.form(),
          line: location.line
        });
        dbg_assert(bpActor, "Breakpoint actor must be created");
        this._hiddenBreakpoints.set(bpActor.actorID, bpActor);
        break;
      }
    }
  },

  


  _getNextStepFrame: function (aFrame) {
    let stepFrame = aFrame.reportedPop ? aFrame.older : aFrame;
    if (!stepFrame || !stepFrame.script) {
      stepFrame = null;
    }
    return stepFrame;
  },

  onClientEvaluate: function (aRequest) {
    if (this.state !== "paused") {
      return { error: "wrongState",
               message: "Debuggee must be paused to evaluate code." };
    }

    let frame = this._requestFrame(aRequest.frame);
    if (!frame) {
      return { error: "unknownFrame",
               message: "Evaluation frame not found" };
    }

    if (!frame.environment) {
      return { error: "notDebuggee",
               message: "cannot access the environment of this frame." };
    }

    let youngest = this.youngestFrame;

    
    let resumedPacket = this._resumed();
    this.conn.send(resumedPacket);

    
    
    let completion = frame.eval(aRequest.expression);

    
    let packet = this._paused(youngest);
    packet.why = { type: "clientEvaluated",
                   frameFinished: this.createProtocolCompletionValue(completion) };

    
    return packet;
  },

  onFrames: function (aRequest) {
    if (this.state !== "paused") {
      return { error: "wrongState",
               message: "Stack frames are only available while the debuggee is paused."};
    }

    let start = aRequest.start ? aRequest.start : 0;
    let count = aRequest.count;

    
    let frame = this.youngestFrame;
    let i = 0;
    while (frame && (i < start)) {
      frame = frame.older;
      i++;
    }

    
    
    let frames = [];
    let promises = [];
    for (; frame && (!count || i < (start + count)); i++, frame=frame.older) {
      let form = this._createFrameActor(frame).form();
      form.depth = i;
      frames.push(form);

      let promise = this.sources.getOriginalLocation({
        source: frame.script.source,
        line: form.where.line,
        column: form.where.column
      }).then((aOrigLocation) => {
        let sourceForm = aOrigLocation.sourceActor.form();
        form.where = {
          source: sourceForm,
          line: aOrigLocation.line,
          column: aOrigLocation.column
        };
        form.source = sourceForm;
      });
      promises.push(promise);
    }

    return all(promises).then(function () {
      return { frames: frames };
    });
  },

  onReleaseMany: function (aRequest) {
    if (!aRequest.actors) {
      return { error: "missingParameter",
               message: "no actors were specified" };
    }

    let res;
    for each (let actorID in aRequest.actors) {
      let actor = this.threadLifetimePool.get(actorID);
      if (!actor) {
        if (!res) {
          res = { error: "notReleasable",
                  message: "Only thread-lifetime actors can be released." };
        }
        continue;
      }
      actor.onRelease();
    }
    return res ? res : {};
  },

  


  _discoverSources: function () {
    
    const sourcesToScripts = new Map();
    for (let s of this.dbg.findScripts()) {
      if (s.source) {
        sourcesToScripts.set(s.source, s);
      }
    }

    return all([this.sources.sourcesForScript(script)
                for (script of sourcesToScripts.values())]);
  },

  onSources: function (aRequest) {
    return this._discoverSources().then(() => {
      return {
        sources: [s.form() for (s of this.sources.iter())]
      };
    });
  },

  






  disableAllBreakpoints: function () {
    for (let bpActor of this.breakpointStore.findBreakpoints()) {
      bpActor.removeScripts();
    }
  },


  


  onInterrupt: function (aRequest) {
    if (this.state == "exited") {
      return { type: "exited" };
    } else if (this.state == "paused") {
      
      return { type: "paused", why: { type: "alreadyPaused" } };
    } else if (this.state != "running") {
      return { error: "wrongState",
               message: "Received interrupt request in " + this.state +
                        " state." };
    }

    try {
      
      let packet = this._paused();
      if (!packet) {
        return { error: "notInterrupted" };
      }
      packet.why = { type: "interrupted" };

      
      
      
      this.conn.send(packet);

      
      this._pushThreadPause();

      
      
      return null;
    } catch (e) {
      reportError(e);
      return { error: "notInterrupted", message: e.toString() };
    }
  },

  


  onEventListeners: function (aRequest) {
    
    if (!this.global) {
      return {
        error: "notImplemented",
        message: "eventListeners request is only supported in content debugging"
      };
    }

    let els = Cc["@mozilla.org/eventlistenerservice;1"]
                .getService(Ci.nsIEventListenerService);

    let nodes = this.global.document.getElementsByTagName("*");
    nodes = [this.global].concat([].slice.call(nodes));
    let listeners = [];

    for (let node of nodes) {
      let handlers = els.getListenerInfoFor(node);

      for (let handler of handlers) {
        
        let listenerForm = Object.create(null);
        let listener = handler.listenerObject;
        
        
        if (!listener || !handler.type) {
          continue;
        }

        
        let selector = node.tagName ? CssLogic.findCssSelector(node) : "window";
        let nodeDO = this.globalDebugObject.makeDebuggeeValue(node);
        listenerForm.node = {
          selector: selector,
          object: this.createValueGrip(nodeDO)
        };
        listenerForm.type = handler.type;
        listenerForm.capturing = handler.capturing;
        listenerForm.allowsUntrusted = handler.allowsUntrusted;
        listenerForm.inSystemEventGroup = handler.inSystemEventGroup;
        let handlerName = "on" + listenerForm.type;
        listenerForm.isEventHandler = false;
        if (typeof node.hasAttribute !== "undefined") {
          listenerForm.isEventHandler = !!node.hasAttribute(handlerName);
        }
        if (!!node[handlerName]) {
          listenerForm.isEventHandler = !!node[handlerName];
        }
        
        let listenerDO = this.globalDebugObject.makeDebuggeeValue(listener);
        
        if (listenerDO.class == "Object" || listenerDO.class == "XULElement") {
          
          
          if (!listenerDO.unwrap()) {
            continue;
          }
          let heDesc;
          while (!heDesc && listenerDO) {
            heDesc = listenerDO.getOwnPropertyDescriptor("handleEvent");
            listenerDO = listenerDO.proto;
          }
          if (heDesc && heDesc.value) {
            listenerDO = heDesc.value;
          }
        }
        
        
        while (listenerDO.isBoundFunction) {
          listenerDO = listenerDO.boundTargetFunction;
        }
        listenerForm.function = this.createValueGrip(listenerDO);
        listeners.push(listenerForm);
      }
    }
    return { listeners: listeners };
  },

  


  _requestFrame: function (aFrameID) {
    if (!aFrameID) {
      return this.youngestFrame;
    }

    if (this._framePool.has(aFrameID)) {
      return this._framePool.get(aFrameID).frame;
    }

    return undefined;
  },

  _paused: function (aFrame) {
    
    
    
    
    
    

    if (this.state === "paused") {
      return undefined;
    }

    
    this.dbg.onEnterFrame = undefined;
    this.dbg.onExceptionUnwind = undefined;
    if (aFrame) {
      aFrame.onStep = undefined;
      aFrame.onPop = undefined;
    }

    
    
    
    if (this.global && !this.global.toString().contains("Sandbox")) {
      let els = Cc["@mozilla.org/eventlistenerservice;1"]
                .getService(Ci.nsIEventListenerService);
      els.removeListenerForAllEvents(this.global, this._allEventsListener, true);
      for (let [,bp] of this._hiddenBreakpoints) {
        bp.onDelete();
      }
      this._hiddenBreakpoints.clear();
    }

    this._state = "paused";

    
    
    dbg_assert(!this._pausePool, "No pause pool should exist yet");
    this._pausePool = new ActorPool(this.conn);
    this.conn.addActorPool(this._pausePool);

    
    
    this._pausePool.threadActor = this;

    
    dbg_assert(!this._pauseActor, "No pause actor should exist yet");
    this._pauseActor = new PauseActor(this._pausePool);
    this._pausePool.addActor(this._pauseActor);

    
    let poppedFrames = this._updateFrames();

    
    let packet = { from: this.actorID,
                   type: "paused",
                   actor: this._pauseActor.actorID };
    if (aFrame) {
      packet.frame = this._createFrameActor(aFrame).form();
    }

    if (poppedFrames) {
      packet.poppedFrames = poppedFrames;
    }

    return packet;
  },

  _resumed: function () {
    this._state = "running";

    
    this.conn.removeActorPool(this._pausePool);

    this._pausePool = null;
    this._pauseActor = null;

    return { from: this.actorID, type: "resumed" };
  },

  




  _updateFrames: function () {
    let popped = [];

    
    let framePool = new ActorPool(this.conn);
    let frameList = [];

    for each (let frameActor in this._frameActors) {
      if (frameActor.frame.live) {
        framePool.addActor(frameActor);
        frameList.push(frameActor);
      } else {
        popped.push(frameActor.actorID);
      }
    }

    
    
    if (this._framePool) {
      this.conn.removeActorPool(this._framePool);
    }

    this._frameActors = frameList;
    this._framePool = framePool;
    this.conn.addActorPool(framePool);

    return popped;
  },

  _createFrameActor: function (aFrame) {
    if (aFrame.actor) {
      return aFrame.actor;
    }

    let actor = new FrameActor(aFrame, this);
    this._frameActors.push(actor);
    this._framePool.addActor(actor);
    aFrame.actor = actor;

    return actor;
  },

  









  createEnvironmentActor: function (aEnvironment, aPool) {
    if (!aEnvironment) {
      return undefined;
    }

    if (aEnvironment.actor) {
      return aEnvironment.actor;
    }

    let actor = new EnvironmentActor(aEnvironment, this);
    aPool.addActor(actor);
    aEnvironment.actor = actor;

    return actor;
  },

  



  createValueGrip: function (aValue, aPool=false) {
    if (!aPool) {
      aPool = this._pausePool;
    }

    switch (typeof aValue) {
      case "boolean":
        return aValue;

      case "string":
        if (this._stringIsLong(aValue)) {
          return this.longStringGrip(aValue, aPool);
        }
        return aValue;

      case "number":
        if (aValue === Infinity) {
          return { type: "Infinity" };
        } else if (aValue === -Infinity) {
          return { type: "-Infinity" };
        } else if (Number.isNaN(aValue)) {
          return { type: "NaN" };
        } else if (!aValue && 1 / aValue === -Infinity) {
          return { type: "-0" };
        }
        return aValue;

      case "undefined":
        return { type: "undefined" };

      case "object":
        if (aValue === null) {
          return { type: "null" };
        }
        return this.objectGrip(aValue, aPool);

      case "symbol":
        let form = {
          type: "symbol"
        };
        let name = getSymbolName(aValue);
        if (name !== undefined) {
          form.name = this.createValueGrip(name);
        }
        return form;

      default:
        dbg_assert(false, "Failed to provide a grip for: " + aValue);
        return null;
    }
  },

  



  createProtocolCompletionValue: function (aCompletion) {
    let protoValue = {};
    if (aCompletion == null) {
      protoValue.terminated = true;
    } else if ("return" in aCompletion) {
      protoValue.return = this.createValueGrip(aCompletion.return);
    } else if ("throw" in aCompletion) {
      protoValue.throw = this.createValueGrip(aCompletion.throw);
    } else {
      protoValue.return = this.createValueGrip(aCompletion.yield);
    }
    return protoValue;
  },

  







  objectGrip: function (aValue, aPool) {
    if (!aPool.objectActors) {
      aPool.objectActors = new WeakMap();
    }

    if (aPool.objectActors.has(aValue)) {
      return aPool.objectActors.get(aValue).grip();
    } else if (this.threadLifetimePool.objectActors.has(aValue)) {
      return this.threadLifetimePool.objectActors.get(aValue).grip();
    }

    let actor = new PauseScopedObjectActor(aValue, this);
    aPool.addActor(actor);
    aPool.objectActors.set(aValue, actor);
    return actor.grip();
  },

  





  pauseObjectGrip: function (aValue) {
    if (!this._pausePool) {
      throw "Object grip requested while not paused.";
    }

    return this.objectGrip(aValue, this._pausePool);
  },

  





  threadObjectGrip: function (aActor) {
    
    
    aActor.registeredPool.objectActors.delete(aActor.obj);
    this.threadLifetimePool.addActor(aActor);
    this.threadLifetimePool.objectActors.set(aActor.obj, aActor);
  },

  






  onThreadGrips: function (aRequest) {
    if (this.state != "paused") {
      return { error: "wrongState" };
    }

    if (!aRequest.actors) {
      return { error: "missingParameter",
               message: "no actors were specified" };
    }

    for (let actorID of aRequest.actors) {
      let actor = this._pausePool.get(actorID);
      if (actor) {
        this.threadObjectGrip(actor);
      }
    }
    return {};
  },

  







  longStringGrip: function (aString, aPool) {
    if (!aPool.longStringActors) {
      aPool.longStringActors = {};
    }

    if (aPool.longStringActors.hasOwnProperty(aString)) {
      return aPool.longStringActors[aString].grip();
    }

    let actor = new LongStringActor(aString, this);
    aPool.addActor(actor);
    aPool.longStringActors[aString] = actor;
    return actor.grip();
  },

  





  pauseLongStringGrip: function (aString) {
    return this.longStringGrip(aString, this._pausePool);
  },

  





  threadLongStringGrip: function (aString) {
    return this.longStringGrip(aString, this._threadLifetimePool);
  },

  






  _stringIsLong: function (aString) {
    return aString.length >= DebuggerServer.LONG_STRING_LENGTH;
  },

  

  







  uncaughtExceptionHook: function (aException) {
    dumpn("Got an exception: " + aException.message + "\n" + aException.stack);
  },

  






  onDebuggerStatement: function (aFrame) {
    
    
    const generatedLocation = getFrameLocation(aFrame);
    const { sourceActor } = this.synchronize(this.sources.getOriginalLocation(
      generatedLocation));
    const url = sourceActor ? sourceActor.url : null;

    return this.sources.isBlackBoxed(url) || aFrame.onStep
      ? undefined
      : this._pauseAndRespond(aFrame, { type: "debuggerStatement" });
  },

  








  onExceptionUnwind: function (aFrame, aValue) {
    let willBeCaught = false;
    for (let frame = aFrame; frame != null; frame = frame.older) {
      if (frame.script.isInCatchScope(frame.offset)) {
        willBeCaught = true;
        break;
      }
    }

    if (willBeCaught && this._options.ignoreCaughtExceptions) {
      return undefined;
    }

    const generatedLocation = getFrameLocation(aFrame);
    const { sourceActor } = this.synchronize(this.sources.getOriginalLocation(
      generatedLocation));
    const url = sourceActor ? sourceActor.url : null;

    if (this.sources.isBlackBoxed(url)) {
      return undefined;
    }

    try {
      let packet = this._paused(aFrame);
      if (!packet) {
        return undefined;
      }

      packet.why = { type: "exception",
                     exception: this.createValueGrip(aValue) };
      this.conn.send(packet);

      this._pushThreadPause();
    } catch(e) {
      reportError(e, "Got an exception during TA_onExceptionUnwind: ");
    }

    return undefined;
  },

  








  onNewScript: function (aScript, aGlobal) {
    this.sources.sourcesForScript(aScript);
    this._addScript(aScript);

    
    
    
    for (let s of aScript.getChildScripts()) {
      this._addScript(s);
    }
  },

  onNewSource: function (aSource) {
    this.conn.send({
      from: this.actorID,
      type: "newSource",
      source: aSource.form()
    });
  },

  







  _allowSource: function (aSource) {
    return !isHiddenSource(aSource);
  },

  


  _restoreBreakpoints: function () {
    if (this.breakpointStore.size === 0) {
      return;
    }

    for (let s of this.dbg.findScripts()) {
      this._addScript(s);
    }
  },

  






  _addScript: function (aScript) {
    if (!this._allowSource(aScript.source)) {
      return false;
    }

    

    let endLine = aScript.startLine + aScript.lineCount - 1;
    let source = this.sources.source({ source: aScript.source });
    for (let bpActor of this.breakpointStore.findBreakpoints({ source: source.form() })) {
      
      if (bpActor.location.line >= aScript.startLine
          && bpActor.location.line <= endLine) {
        source._setBreakpoint(bpActor.location, aScript);
      }
    }

    return true;
  },


  


  onPrototypesAndProperties: function (aRequest) {
    let result = {};
    for (let actorID of aRequest.actors) {
      
      
      let actor = this.conn.getActor(actorID);
      if (!actor) {
        return { from: this.actorID,
                 error: "noSuchActor" };
      }
      let handler = actor.onPrototypeAndProperties;
      if (!handler) {
        return { from: this.actorID,
                 error: "unrecognizedPacketType",
                 message: ('Actor "' + actorID +
                           '" does not recognize the packet type ' +
                           '"prototypeAndProperties"') };
      }
      result[actorID] = handler.call(actor, {});
    }
    return { from: this.actorID,
             actors: result };
  }
};

ThreadActor.prototype.requestTypes = {
  "attach": ThreadActor.prototype.onAttach,
  "detach": ThreadActor.prototype.onDetach,
  "reconfigure": ThreadActor.prototype.onReconfigure,
  "resume": ThreadActor.prototype.onResume,
  "clientEvaluate": ThreadActor.prototype.onClientEvaluate,
  "frames": ThreadActor.prototype.onFrames,
  "interrupt": ThreadActor.prototype.onInterrupt,
  "eventListeners": ThreadActor.prototype.onEventListeners,
  "releaseMany": ThreadActor.prototype.onReleaseMany,
  "sources": ThreadActor.prototype.onSources,
  "threadGrips": ThreadActor.prototype.onThreadGrips,
  "prototypesAndProperties": ThreadActor.prototype.onPrototypesAndProperties
};

exports.ThreadActor = ThreadActor;










function PauseActor(aPool)
{
  this.pool = aPool;
}

PauseActor.prototype = {
  actorPrefix: "pause"
};









function PauseScopedActor()
{
}








PauseScopedActor.withPaused = function (aMethod) {
  return function () {
    if (this.isPaused()) {
      return aMethod.apply(this, arguments);
    } else {
      return this._wrongState();
    }
  };
};

PauseScopedActor.prototype = {

  


  isPaused: function () {
    
    
    
    return this.threadActor ? this.threadActor.state === "paused" : true;
  },

  


  _wrongState: function () {
    return {
      error: "wrongState",
      message: this.constructor.name +
        " actors can only be accessed while the thread is paused."
    };
  }
};











function resolveURIToLocalPath(aURI) {
  let resolved;
  switch (aURI.scheme) {
    case "jar":
    case "file":
      return aURI;

    case "chrome":
      resolved = Cc["@mozilla.org/chrome/chrome-registry;1"].
                 getService(Ci.nsIChromeRegistry).convertChromeURL(aURI);
      return resolveURIToLocalPath(resolved);

    case "resource":
      resolved = Cc["@mozilla.org/network/protocol;1?name=resource"].
                 getService(Ci.nsIResProtocolHandler).resolveURI(aURI);
      aURI = Services.io.newURI(resolved, null, null);
      return resolveURIToLocalPath(aURI);

    default:
      return null;
  }
}






































function SourceActor({ source, thread, originalUrl, generatedSource,
                       contentType }) {
  this._threadActor = thread;
  this._originalUrl = originalUrl;
  this._source = source;
  this._generatedSource = generatedSource;
  this._contentType = contentType;

  this.onSource = this.onSource.bind(this);
  this._invertSourceMap = this._invertSourceMap.bind(this);
  this._encodeAndSetSourceMapURL = this._encodeAndSetSourceMapURL.bind(this);
  this._getSourceText = this._getSourceText.bind(this);

  this._mapSourceToAddon();

  if (this.threadActor.sources.isPrettyPrinted(this.url)) {
    this._init = this.onPrettyPrint({
      indent: this.threadActor.sources.prettyPrintIndent(this.url)
    }).then(null, error => {
      DevToolsUtils.reportException("SourceActor", error);
    });
  } else {
    this._init = null;
  }
}

SourceActor.prototype = {
  constructor: SourceActor,
  actorPrefix: "source",

  _oldSourceMap: null,
  _init: null,
  _addonID: null,
  _addonPath: null,

  get threadActor() { return this._threadActor; },
  get dbg() { return this.threadActor.dbg; },
  get source() { return this._source; },
  get generatedSource() { return this._generatedSource; },
  get breakpointStore() { return this.threadActor.breakpointStore; },
  get url() {
    if (this.source) {
      return getSourceURL(this.source);
    }
    return this._originalUrl;
  },
  get addonID() { return this._addonID; },
  get addonPath() { return this._addonPath; },

  get prettyPrintWorker() {
    return this.threadActor.prettyPrintWorker;
  },

  form: function () {
    let source = this.source || this.generatedSource;
    
    
    
    let introductionUrl = null;
    if (source && source.introductionScript) {
      introductionUrl = source.introductionScript.source.url;
    }

    return {
      actor: this.actorID,
      url: this.url,
      addonID: this._addonID,
      addonPath: this._addonPath,
      isBlackBoxed: this.threadActor.sources.isBlackBoxed(this.url),
      isPrettyPrinted: this.threadActor.sources.isPrettyPrinted(this.url),
      introductionUrl: introductionUrl,
      introductionType: source ? source.introductionType : null
    };
  },

  disconnect: function () {
    if (this.registeredPool && this.registeredPool.sourceActors) {
      delete this.registeredPool.sourceActors[this.actorID];
    }
  },

  _mapSourceToAddon: function() {
    try {
      var nsuri = Services.io.newURI(this.url.split(" -> ").pop(), null, null);
    }
    catch (e) {
      
      return;
    }

    let localURI = resolveURIToLocalPath(nsuri);

    let id = {};
    if (localURI && mapURIToAddonID(localURI, id)) {
      this._addonID = id.value;

      if (localURI instanceof Ci.nsIJARURI) {
        
        this._addonPath = localURI.JAREntry;
      }
      else if (localURI instanceof Ci.nsIFileURL) {
        
        
        let target = localURI.file;
        let path = target.leafName;

        
        
        let root = target.parent;
        let file = root.parent;
        while (file && mapURIToAddonID(Services.io.newFileURI(file), {})) {
          path = root.leafName + "/" + path;
          root = file;
          file = file.parent;
        }

        if (!file) {
          const error = new Error("Could not find the root of the add-on for " + this.url);
          DevToolsUtils.reportException("SourceActor.prototype._mapSourceToAddon", error)
          return;
        }

        this._addonPath = path;
      }
    }
  },

  _getSourceText: function () {
    let toResolvedContent = t => ({
      content: t,
      contentType: this._contentType
    });

    let genSource = this.generatedSource || this.source;
    return this.threadActor.sources.fetchSourceMap(genSource).then(map => {
      let sc;
      if (map && (sc = map.sourceContentFor(this.url))) {
        return toResolvedContent(sc);
      }

      
      
      
      
      
      
      if (this.source &&
          this.source.text !== "[no source]" &&
          this._contentType &&
          this._contentType.indexOf('javascript') !== -1) {
        return toResolvedContent(this.source.text);
      }
      else {
        
        
        
        let sourceFetched = fetch(this.url, { loadFromCache: !this.source });

        
        return sourceFetched.then(result => {
          this._contentType = result.contentType;
          return result;
        });
      }
    });
  },

  



  getExecutableLines: function () {
    
    let packet = {
      from: this.actorID
    };

    function sortLines(lines) {
      
      lines = [line for (line of lines)];
      lines.sort((a, b) => {
        return a - b;
      });
      return lines;
    }

    if (this.generatedSource) {
      return this.threadActor.sources.getSourceMap(this.generatedSource).then(sm => {
        let lines = new Set();

        
        let offsets = this.getExecutableOffsets(this.generatedSource, false);
        for (let offset of offsets) {
          let {line, source: sourceUrl} = sm.originalPositionFor({
            line: offset.lineNumber,
            column: offset.columnNumber
          });

          if (sourceUrl === this.url) {
            lines.add(line);
          }
        }

        packet.lines = sortLines(lines);
        return packet;
      });
    }

    let lines = this.getExecutableOffsets(this.source, true);
    packet.lines = sortLines(lines);
    return packet;
  },

  





  getExecutableOffsets: function  (source, onlyLine) {
    let offsets = new Set();
    for (let s of this.threadActor.dbg.findScripts({ source: source })) {
      for (let offset of s.getAllColumnOffsets()) {
        offsets.add(onlyLine ? offset.lineNumber : offset);
      }
    }

    return offsets;
  },

  


  onSource: function () {
    return resolve(this._init)
      .then(this._getSourceText)
      .then(({ content, contentType }) => {
        return {
          from: this.actorID,
          source: this.threadActor.createValueGrip(
            content, this.threadActor.threadLifetimePool),
          contentType: contentType
        };
      })
      .then(null, aError => {
        reportError(aError, "Got an exception during SA_onSource: ");
        return {
          "from": this.actorID,
          "error": "loadSourceError",
          "message": "Could not load the source for " + this.url + ".\n"
            + DevToolsUtils.safeErrorString(aError)
        };
      });
  },

  


  onPrettyPrint: function ({ indent }) {
    this.threadActor.sources.prettyPrint(this.url, indent);
    return this._getSourceText()
      .then(this._sendToPrettyPrintWorker(indent))
      .then(this._invertSourceMap)
      .then(this._encodeAndSetSourceMapURL)
      .then(() => {
        
        
        
        this._init = null;
      })
      .then(this.onSource)
      .then(null, error => {
        this.onDisablePrettyPrint();
        return {
          from: this.actorID,
          error: "prettyPrintError",
          message: DevToolsUtils.safeErrorString(error)
        };
      });
  },

  











  _sendToPrettyPrintWorker: function (aIndent) {
    return ({ content }) => {
      const deferred = promise.defer();
      const id = Math.random();

      const onReply = ({ data }) => {
        if (data.id !== id) {
          return;
        }
        this.prettyPrintWorker.removeEventListener("message", onReply, false);

        if (data.error) {
          deferred.reject(new Error(data.error));
        } else {
          deferred.resolve(data);
        }
      };

      this.prettyPrintWorker.addEventListener("message", onReply, false);
      this.prettyPrintWorker.postMessage({
        id: id,
        url: this.url,
        indent: aIndent,
        source: content
      });

      return deferred.promise;
    };
  },

  







  _invertSourceMap: function ({ code, mappings }) {
    const generator = new SourceMapGenerator({ file: this.url });
    return DevToolsUtils.yieldingEach(mappings, m => {
      let mapping = {
        generated: {
          line: m.generatedLine,
          column: m.generatedColumn
        }
      };
      if (m.source) {
        mapping.source = m.source;
        mapping.original = {
          line: m.originalLine,
          column: m.originalColumn
        };
        mapping.name = m.name;
      }
      generator.addMapping(mapping);
    }).then(() => {
      generator.setSourceContent(this.url, code);
      const consumer = SourceMapConsumer.fromSourceMap(generator);

      
      
      

      const getOrigPos = consumer.originalPositionFor.bind(consumer);
      const getGenPos = consumer.generatedPositionFor.bind(consumer);

      consumer.originalPositionFor = ({ line, column }) => {
        const location = getGenPos({
          line: line,
          column: column,
          source: this.url
        });
        location.source = this.url;
        return location;
      };

      consumer.generatedPositionFor = ({ line, column }) => getOrigPos({
        line: line,
        column: column
      });

      return {
        code: code,
        map: consumer
      };
    });
  },

  





  _encodeAndSetSourceMapURL: function  ({ map: sm }) {
    let source = this.generatedSource || this.source;
    let sources = this.threadActor.sources;

    return sources.getSourceMap(source).then(prevMap => {
      if (prevMap) {
        
        this._oldSourceMapping = {
          url: source.sourceMapURL,
          map: prevMap
        };

        prevMap = SourceMapGenerator.fromSourceMap(prevMap);
        prevMap.applySourceMap(sm, this.url);
        sm = SourceMapConsumer.fromSourceMap(prevMap);
      }

      let sources = this.threadActor.sources;
      sources.clearSourceMapCache(source.sourceMapURL);
      sources.setSourceMapHard(source, null, sm);
    });
  },

  


  onDisablePrettyPrint: function () {
    let source = this.generatedSource || this.source;
    let sources = this.threadActor.sources;
    let sm = sources.getSourceMap(source);

    sources.clearSourceMapCache(source.sourceMapURL, { hard: true });

    if (this._oldSourceMapping) {
      sources.setSourceMapHard(source,
                               this._oldSourceMapping.url,
                               this._oldSourceMapping.map);
       this._oldSourceMapping = null;
    }

    this.threadActor.sources.disablePrettyPrint(this.url);
    return this.onSource();
  },

  


  onBlackBox: function (aRequest) {
    this.threadActor.sources.blackBox(this.url);
    let packet = {
      from: this.actorID
    };
    if (this.threadActor.state == "paused"
        && this.threadActor.youngestFrame
        && this.threadActor.youngestFrame.script.url == this.url) {
      packet.pausedInSource = true;
    }
    return packet;
  },

  


  onUnblackBox: function (aRequest) {
    this.threadActor.sources.unblackBox(this.url);
    return {
      from: this.actorID
    };
  },

  


  onSetBreakpoint: function(aRequest) {
    if (this.threadActor.state !== "paused") {
      return { error: "wrongState",
               message: "Breakpoints can only be set while the debuggee is paused."};
    }

    let loc = {
      url: this.url,
      line: aRequest.location.line,
      column: aRequest.location.column,
    };
    let originalLoc = loc;

    return this.threadActor.sources.getGeneratedLocation({
      sourceActor: this,
      line: loc.line,
      column: loc.column
    }).then(genLoc => {
      if (genLoc.sourceActor !== this) {
        return genLoc.sourceActor._createBreakpoint(genLoc, originalLoc, aRequest.condition);
      }
      else {
        return this._createBreakpoint(genLoc, originalLoc, aRequest.condition);
      }
    });
  },

  _createBreakpoint: function(loc, originalLoc, condition) {
    return resolve(null).then(() => {
      return this._setBreakpoint({
        line: loc.line,
        column: loc.column,
        condition: condition
      });
    }).then(response => {
      var actual = response.actualLocation;
      if (actual) {
        if (this.source) {
          return this.threadActor.sources.getOriginalLocation({
            source: this.source,
            line: actual.line,
            column: actual.column
          }).then(({ sourceActor, line, column }) => {
            if (sourceActor.url !== originalLoc.url ||
                line !== originalLoc.line ||
                column !== originalLoc.column) {
              response.actualLocation = {
                source: sourceActor.form(),
                line: line,
                column: column
              };
            }
            return response;
          });
        }
        else {
          response.actualLocation = {
            source: this.form(),
            line: actual.line,
            column: actual.column
          }
          return response;
        }
      }
      else {
        if (this.source) {
          
          
          return this.threadActor.sources.getOriginalLocation({
            source: this.source,
            line: loc.line,
            column: loc.column
          }).then(({ sourceActor, line }) => {
            if (originalLoc.url !== sourceActor.url ||
                originalLoc.line !== line) {
              response.actualLocation = {
                source: sourceActor.form(),
                line: line
              };
            }
            return response;
          });
        }
        else {
          return response;
        }
      }
    }).then(null, error => {
      DevToolsUtils.reportException("onSetBreakpoint", error);
    });
  },

  









  _getOrCreateBreakpointActor: function (location) {
    let actor = this.breakpointStore.getBreakpoint(location);
    if (!actor) {
      actor = new BreakpointActor(this.threadActor, {
        sourceActor: this,
        line: location.line,
        column: location.column,
        condition: location.condition
      });
      this.threadActor.threadLifetimePool.addActor(actor);
      this.breakpointStore.addBreakpoint(location, actor);
      return actor;
    }

    actor.condition = location.condition;
    return actor;
  },

  











  _setBreakpointAtColumn: function (scripts, location, actor) {
    
    const scriptsAndOffsetMappings = new Map();

    for (let script of scripts) {
      this._findClosestOffsetMappings(location, script, scriptsAndOffsetMappings);
    }

    for (let [script, mappings] of scriptsAndOffsetMappings) {
      for (let offsetMapping of mappings) {
        script.setBreakpoint(offsetMapping.offset, actor);
      }
      actor.addScript(script, this.threadActor);
    }

    return {
      actor: actor.actorID
    };
  },

  












  _findEntryPointsForLine: function (scripts, line) {
    const entryPoints = [];
    for (let script of scripts) {
      const offsets = script.getLineOffsets(line);
      if (offsets.length) {
        entryPoints.push({ script, offsets });
      }
    }
    return entryPoints;
  },

  


















  _findNextLineWithOffsets: function (scripts, startLine) {
    const maxLine = Math.max(...scripts.map(s => s.startLine + s.lineCount));

    for (let line = startLine; line < maxLine; line++) {
      const entryPoints = this._findEntryPointsForLine(scripts, line);
      if (entryPoints.length) {
        return { line, entryPoints };
      }
    }

    return null;
  },

  













  _setBreakpoint: function (aLocation, aOnlyThisScript=null) {
    const location = {
      source: this.form(),
      line: aLocation.line,
      column: aLocation.column,
      condition: aLocation.condition
    };
    const actor = location.actor = this._getOrCreateBreakpointActor(location);

    
    
    
    
    
    const scripts = this.dbg.findScripts({
      source: this.source || undefined,
      url: this._originalUrl || undefined,
      line: location.line,
    });

    if (scripts.length === 0) {
      
      
      
      
      
      
      return {
        actor: actor.actorID
      }
    }

    if (location.column) {
      return this._setBreakpointAtColumn(scripts, location, actor);
    }

    
    
    

    const result = this._findNextLineWithOffsets(scripts, location.line);
    if (!result) {
      return {
        error: "noCodeAtLineColumn",
        actor: actor.actorID
      };
    }

    const { line, entryPoints } = result;
    const actualLocation = line !== location.line
          ? { source: { actor: this.actorID }, line }
      : undefined;

    if (actualLocation) {
      
      
      
      

      let existingActor = this.breakpointStore.getBreakpoint(actualLocation);
      if (existingActor) {
        actor.onDelete();
        this.breakpointStore.removeBreakpoint(location);
        return {
          actor: existingActor.actorID,
          actualLocation
        };
      } else {
        actor.location = actualLocation;
        actor.location = {
          sourceActor: this,
          line: actualLocation.line
        };
        this.breakpointStore.removeBreakpoint(location);
        this.breakpointStore.addBreakpoint(actualLocation, actor);
      }
    }

    this._setBreakpointOnEntryPoints(
      actor,
      aOnlyThisScript
        ? entryPoints.filter(o => o.script === aOnlyThisScript)
        : entryPoints
    );

    return {
      actor: actor.actorID,
      actualLocation
    };
  },

  








  _setBreakpointOnEntryPoints: function (actor, entryPoints) {
    for (let { script, offsets } of entryPoints) {
      for (let offset of offsets) {
        script.setBreakpoint(offset, actor);
      }
      actor.addScript(script, this.threadActor);
    }
  },

  




































  _findClosestOffsetMappings: function (aTargetLocation,
                                aScript,
                                aScriptsAndOffsetMappings) {
    let offsetMappings = aScript.getAllColumnOffsets()
      .filter(({ lineNumber }) => lineNumber === aTargetLocation.line);

    
    
    
    
    let closestDistance = Infinity;
    if (aScriptsAndOffsetMappings.size) {
      for (let mappings of aScriptsAndOffsetMappings.values()) {
        closestDistance = Math.abs(aTargetLocation.column - mappings[0].columnNumber);
        break;
      }
    }

    for (let mapping of offsetMappings) {
      let currentDistance = Math.abs(aTargetLocation.column - mapping.columnNumber);

      if (currentDistance > closestDistance) {
        continue;
      } else if (currentDistance < closestDistance) {
        closestDistance = currentDistance;
        aScriptsAndOffsetMappings.clear();
        aScriptsAndOffsetMappings.set(aScript, [mapping]);
      } else {
        if (!aScriptsAndOffsetMappings.has(aScript)) {
          aScriptsAndOffsetMappings.set(aScript, []);
        }
        aScriptsAndOffsetMappings.get(aScript).push(mapping);
      }
    }
  }
};

SourceActor.prototype.requestTypes = {
  "source": SourceActor.prototype.onSource,
  "blackbox": SourceActor.prototype.onBlackBox,
  "unblackbox": SourceActor.prototype.onUnblackBox,
  "prettyPrint": SourceActor.prototype.onPrettyPrint,
  "disablePrettyPrint": SourceActor.prototype.onDisablePrettyPrint,
  "getExecutableLines": SourceActor.prototype.getExecutableLines,
  "setBreakpoint": SourceActor.prototype.onSetBreakpoint
};










function isObject(aValue) {
  const type = typeof aValue;
  return type == "object" ? aValue !== null : type == "function";
}










function createBuiltinStringifier(aCtor) {
  return aObj => aCtor.prototype.toString.call(aObj.unsafeDereference());
}









function errorStringify(aObj) {
  let name = DevToolsUtils.getProperty(aObj, "name");
  if (name === "" || name === undefined) {
    name = aObj.class;
  } else if (isObject(name)) {
    name = stringify(name);
  }

  let message = DevToolsUtils.getProperty(aObj, "message");
  if (isObject(message)) {
    message = stringify(message);
  }

  if (message === "" || message === undefined) {
    return name;
  }
  return name + ": " + message;
}









function stringify(aObj) {
  if (aObj.class == "DeadObject") {
    const error = new Error("Dead object encountered.");
    DevToolsUtils.reportException("stringify", error);
    return "<dead object>";
  }

  const stringifier = stringifiers[aObj.class] || stringifiers.Object;

  try {
    return stringifier(aObj);
  } catch (e) {
    DevToolsUtils.reportException("stringify", e);
    return "<failed to stringify object>";
  }
}


let seen = null;

let stringifiers = {
  Error: errorStringify,
  EvalError: errorStringify,
  RangeError: errorStringify,
  ReferenceError: errorStringify,
  SyntaxError: errorStringify,
  TypeError: errorStringify,
  URIError: errorStringify,
  Boolean: createBuiltinStringifier(Boolean),
  Function: createBuiltinStringifier(Function),
  Number: createBuiltinStringifier(Number),
  RegExp: createBuiltinStringifier(RegExp),
  String: createBuiltinStringifier(String),
  Object: obj => "[object " + obj.class + "]",
  Array: obj => {
    
    
    const topLevel = !seen;
    if (topLevel) {
      seen = new Set();
    } else if (seen.has(obj)) {
      return "";
    }

    seen.add(obj);

    const len = DevToolsUtils.getProperty(obj, "length");
    let string = "";

    
    
    
    if (typeof len == "number" && len > 0) {
      for (let i = 0; i < len; i++) {
        const desc = obj.getOwnPropertyDescriptor(i);
        if (desc) {
          const { value } = desc;
          if (value != null) {
            string += isObject(value) ? stringify(value) : value;
          }
        }

        if (i < len - 1) {
          string += ",";
        }
      }
    }

    if (topLevel) {
      seen = null;
    }

    return string;
  },
  DOMException: obj => {
    const message = DevToolsUtils.getProperty(obj, "message") || "<no message>";
    const result = (+DevToolsUtils.getProperty(obj, "result")).toString(16);
    const code = DevToolsUtils.getProperty(obj, "code");
    const name = DevToolsUtils.getProperty(obj, "name") || "<unknown>";

    return '[Exception... "' + message + '" ' +
           'code: "' + code +'" ' +
           'nsresult: "0x' + result + ' (' + name + ')"]';
  },
  Promise: obj => {
    const { state, value, reason } = obj.getPromiseState();
    let statePreview = state;
    if (state != "pending") {
      const settledValue = state === "fulfilled" ? value : reason;
      statePreview += ": " + (typeof settledValue === "object" && settledValue !== null
                                ? stringify(settledValue)
                                : settledValue);
    }
    return "Promise (" + statePreview + ")";
  },
};









function ObjectActor(aObj, aThreadActor)
{
  dbg_assert(!aObj.optimizedOut, "Should not create object actors for optimized out values!");
  this.obj = aObj;
  this.threadActor = aThreadActor;
}

ObjectActor.prototype = {
  actorPrefix: "obj",

  


  grip: function () {
    this.threadActor._gripDepth++;

    let g = {
      "type": "object",
      "class": this.obj.class,
      "actor": this.actorID,
      "extensible": this.obj.isExtensible(),
      "frozen": this.obj.isFrozen(),
      "sealed": this.obj.isSealed()
    };

    if (this.obj.class != "DeadObject") {
      
      if (this.obj.class == "Promise") {
        const { state, value, reason } = this.obj.getPromiseState();
        g.promiseState = { state };
        if (state == "fulfilled") {
          g.promiseState.value = this.threadActor.createValueGrip(value);
        } else if (state == "rejected") {
          g.promiseState.reason = this.threadActor.createValueGrip(reason);
        }
      }

      let raw = this.obj.unsafeDereference();

      
      
      if (Cu) {
        raw = Cu.unwaiveXrays(raw);
      }

      if (!DevToolsUtils.isSafeJSObject(raw)) {
        raw = null;
      }

      let previewers = DebuggerServer.ObjectActorPreviewers[this.obj.class] ||
                       DebuggerServer.ObjectActorPreviewers.Object;
      for (let fn of previewers) {
        try {
          if (fn(this, g, raw)) {
            break;
          }
        } catch (e) {
          DevToolsUtils.reportException("ObjectActor.prototype.grip previewer function", e);
        }
      }
    }

    this.threadActor._gripDepth--;
    return g;
  },

  


  release: function () {
    if (this.registeredPool.objectActors) {
      this.registeredPool.objectActors.delete(this.obj);
    }
    this.registeredPool.removeActor(this);
  },

  






  onDefinitionSite: function OA_onDefinitionSite(aRequest) {
    if (this.obj.class != "Function") {
      return {
        from: this.actorID,
        error: "objectNotFunction",
        message: this.actorID + " is not a function."
      };
    }

    if (!this.obj.script) {
      return {
        from: this.actorID,
        error: "noScript",
        message: this.actorID + " has no Debugger.Script"
      };
    }

    const generatedLocation = {
      source: this.obj.script.source,
      line: this.obj.script.startLine,
      
      column: 0
    };

    return this.threadActor.sources.getOriginalLocation(generatedLocation)
      .then(({ sourceActor, line, column }) => {

        return {
          from: this.actorID,
          source: sourceActor.form(),
          line: line,
          column: column
        };
      });
  },

  






  onOwnPropertyNames: function (aRequest) {
    return { from: this.actorID,
             ownPropertyNames: this.obj.getOwnPropertyNames() };
  },

  






  onPrototypeAndProperties: function (aRequest) {
    let ownProperties = Object.create(null);
    let names;
    try {
      names = this.obj.getOwnPropertyNames();
    } catch (ex) {
      
      
      return { from: this.actorID,
               prototype: this.threadActor.createValueGrip(null),
               ownProperties: ownProperties,
               safeGetterValues: Object.create(null) };
    }
    for (let name of names) {
      ownProperties[name] = this._propertyDescriptor(name);
    }
    return { from: this.actorID,
             prototype: this.threadActor.createValueGrip(this.obj.proto),
             ownProperties: ownProperties,
             safeGetterValues: this._findSafeGetterValues(ownProperties) };
  },

  












  _findSafeGetterValues: function (aOwnProperties, aLimit = 0)
  {
    let safeGetterValues = Object.create(null);
    let obj = this.obj;
    let level = 0, i = 0;

    while (obj) {
      let getters = this._findSafeGetters(obj);
      for (let name of getters) {
        
        
        
        if (name in safeGetterValues ||
            (obj != this.obj && name in aOwnProperties)) {
          continue;
        }

        let desc = null, getter = null;
        try {
          desc = obj.getOwnPropertyDescriptor(name);
          getter = desc.get;
        } catch (ex) {
          
        }
        if (!getter) {
          obj._safeGetters = null;
          continue;
        }

        let result = getter.call(this.obj);
        if (result && !("throw" in result)) {
          let getterValue = undefined;
          if ("return" in result) {
            getterValue = result.return;
          } else if ("yield" in result) {
            getterValue = result.yield;
          }
          
          
          if (getterValue !== undefined) {
            safeGetterValues[name] = {
              getterValue: this.threadActor.createValueGrip(getterValue),
              getterPrototypeLevel: level,
              enumerable: desc.enumerable,
              writable: level == 0 ? desc.writable : true,
            };
            if (aLimit && ++i == aLimit) {
              break;
            }
          }
        }
      }
      if (aLimit && i == aLimit) {
        break;
      }

      obj = obj.proto;
      level++;
    }

    return safeGetterValues;
  },

  










  _findSafeGetters: function (aObject)
  {
    if (aObject._safeGetters) {
      return aObject._safeGetters;
    }

    let getters = new Set();
    let names = [];
    try {
      names = aObject.getOwnPropertyNames()
    } catch (ex) {
      
      
    }

    for (let name of names) {
      let desc = null;
      try {
        desc = aObject.getOwnPropertyDescriptor(name);
      } catch (e) {
        
        
      }
      if (!desc || desc.value !== undefined || !("get" in desc)) {
        continue;
      }

      if (DevToolsUtils.hasSafeGetter(desc)) {
        getters.add(name);
      }
    }

    aObject._safeGetters = getters;
    return getters;
  },

  





  onPrototype: function (aRequest) {
    return { from: this.actorID,
             prototype: this.threadActor.createValueGrip(this.obj.proto) };
  },

  






  onProperty: function (aRequest) {
    if (!aRequest.name) {
      return { error: "missingParameter",
               message: "no property name was specified" };
    }

    return { from: this.actorID,
             descriptor: this._propertyDescriptor(aRequest.name) };
  },

  





  onDisplayString: function (aRequest) {
    const string = stringify(this.obj);
    return { from: this.actorID,
             displayString: this.threadActor.createValueGrip(string) };
  },

  













  _propertyDescriptor: function (aName, aOnlyEnumerable) {
    let desc;
    try {
      desc = this.obj.getOwnPropertyDescriptor(aName);
    } catch (e) {
      
      
      
      return {
        configurable: false,
        writable: false,
        enumerable: false,
        value: e.name
      };
    }

    if (!desc || aOnlyEnumerable && !desc.enumerable) {
      return undefined;
    }

    let retval = {
      configurable: desc.configurable,
      enumerable: desc.enumerable
    };

    if ("value" in desc) {
      retval.writable = desc.writable;
      retval.value = this.threadActor.createValueGrip(desc.value);
    } else {
      if ("get" in desc) {
        retval.get = this.threadActor.createValueGrip(desc.get);
      }
      if ("set" in desc) {
        retval.set = this.threadActor.createValueGrip(desc.set);
      }
    }
    return retval;
  },

  





  onDecompile: function (aRequest) {
    if (this.obj.class !== "Function") {
      return { error: "objectNotFunction",
               message: "decompile request is only valid for object grips " +
                        "with a 'Function' class." };
    }

    return { from: this.actorID,
             decompiledCode: this.obj.decompile(!!aRequest.pretty) };
  },

  





  onParameterNames: function (aRequest) {
    if (this.obj.class !== "Function") {
      return { error: "objectNotFunction",
               message: "'parameterNames' request is only valid for object " +
                        "grips with a 'Function' class." };
    }

    return { parameterNames: this.obj.parameterNames };
  },

  





  onRelease: function (aRequest) {
    this.release();
    return {};
  },

  





  onScope: function (aRequest) {
    if (this.obj.class !== "Function") {
      return { error: "objectNotFunction",
               message: "scope request is only valid for object grips with a" +
                        " 'Function' class." };
    }

    let envActor = this.threadActor.createEnvironmentActor(this.obj.environment,
                                                           this.registeredPool);
    if (!envActor) {
      return { error: "notDebuggee",
               message: "cannot access the environment of this function." };
    }

    return { from: this.actorID, scope: envActor.form() };
  }
};

ObjectActor.prototype.requestTypes = {
  "definitionSite": ObjectActor.prototype.onDefinitionSite,
  "parameterNames": ObjectActor.prototype.onParameterNames,
  "prototypeAndProperties": ObjectActor.prototype.onPrototypeAndProperties,
  "prototype": ObjectActor.prototype.onPrototype,
  "property": ObjectActor.prototype.onProperty,
  "displayString": ObjectActor.prototype.onDisplayString,
  "ownPropertyNames": ObjectActor.prototype.onOwnPropertyNames,
  "decompile": ObjectActor.prototype.onDecompile,
  "release": ObjectActor.prototype.onRelease,
  "scope": ObjectActor.prototype.onScope,
};

exports.ObjectActor = ObjectActor;

















DebuggerServer.ObjectActorPreviewers = {
  String: [function({obj, threadActor}, aGrip) {
    let result = genericObjectPreviewer("String", String, obj, threadActor);
    let length = DevToolsUtils.getProperty(obj, "length");

    if (!result || typeof length != "number") {
      return false;
    }

    aGrip.preview = {
      kind: "ArrayLike",
      length: length
    };

    if (threadActor._gripDepth > 1) {
      return true;
    }

    let items = aGrip.preview.items = [];

    const max = Math.min(result.value.length, OBJECT_PREVIEW_MAX_ITEMS);
    for (let i = 0; i < max; i++) {
      let value = threadActor.createValueGrip(result.value[i]);
      items.push(value);
    }

    return true;
  }],

  Boolean: [function({obj, threadActor}, aGrip) {
    let result = genericObjectPreviewer("Boolean", Boolean, obj, threadActor);
    if (result) {
      aGrip.preview = result;
      return true;
    }

    return false;
  }],

  Number: [function({obj, threadActor}, aGrip) {
    let result = genericObjectPreviewer("Number", Number, obj, threadActor);
    if (result) {
      aGrip.preview = result;
      return true;
    }

    return false;
  }],

  Function: [function({obj, threadActor}, aGrip) {
    if (obj.name) {
      aGrip.name = obj.name;
    }

    if (obj.displayName) {
      aGrip.displayName = obj.displayName.substr(0, 500);
    }

    if (obj.parameterNames) {
      aGrip.parameterNames = obj.parameterNames;
    }

    
    
    let userDisplayName;
    try {
      userDisplayName = obj.getOwnPropertyDescriptor("displayName");
    } catch (e) {
      
      
      dumpn(e);
    }

    if (userDisplayName && typeof userDisplayName.value == "string" &&
        userDisplayName.value) {
      aGrip.userDisplayName = threadActor.createValueGrip(userDisplayName.value);
    }

    return true;
  }],

  RegExp: [function({obj, threadActor}, aGrip) {
    
    if (!obj.proto || obj.proto.class != "RegExp") {
      return false;
    }

    let str = RegExp.prototype.toString.call(obj.unsafeDereference());
    aGrip.displayString = threadActor.createValueGrip(str);
    return true;
  }],

  Date: [function({obj, threadActor}, aGrip) {
    if (!obj.proto || obj.proto.class != "Date") {
      return false;
    }

    let time = Date.prototype.getTime.call(obj.unsafeDereference());

    aGrip.preview = {
      timestamp: threadActor.createValueGrip(time),
    };
    return true;
  }],

  Array: [function({obj, threadActor}, aGrip) {
    let length = DevToolsUtils.getProperty(obj, "length");
    if (typeof length != "number") {
      return false;
    }

    aGrip.preview = {
      kind: "ArrayLike",
      length: length,
    };

    if (threadActor._gripDepth > 1) {
      return true;
    }

    let raw = obj.unsafeDereference();
    let items = aGrip.preview.items = [];

    for (let i = 0; i < length; ++i) {
      
      
      
      
      
      
      let desc = Object.getOwnPropertyDescriptor(Cu.waiveXrays(raw), i);
      if (desc && !desc.get && !desc.set) {
        let value = Cu.unwaiveXrays(desc.value);
        value = makeDebuggeeValueIfNeeded(obj, value);
        items.push(threadActor.createValueGrip(value));
      } else {
        items.push(null);
      }

      if (items.length == OBJECT_PREVIEW_MAX_ITEMS) {
        break;
      }
    }

    return true;
  }], 

  Set: [function({obj, threadActor}, aGrip) {
    let size = DevToolsUtils.getProperty(obj, "size");
    if (typeof size != "number") {
      return false;
    }

    aGrip.preview = {
      kind: "ArrayLike",
      length: size,
    };

    
    if (threadActor._gripDepth > 1) {
      return true;
    }

    let raw = obj.unsafeDereference();
    let items = aGrip.preview.items = [];
    
    
    
    
    
    
    
    
    
    
    for (let item of Cu.waiveXrays(Set.prototype.values.call(raw))) {
      item = Cu.unwaiveXrays(item);
      item = makeDebuggeeValueIfNeeded(obj, item);
      items.push(threadActor.createValueGrip(item));
      if (items.length == OBJECT_PREVIEW_MAX_ITEMS) {
        break;
      }
    }

    return true;
  }], 

  Map: [function({obj, threadActor}, aGrip) {
    let size = DevToolsUtils.getProperty(obj, "size");
    if (typeof size != "number") {
      return false;
    }

    aGrip.preview = {
      kind: "MapLike",
      size: size,
    };

    if (threadActor._gripDepth > 1) {
      return true;
    }

    let raw = obj.unsafeDereference();
    let entries = aGrip.preview.entries = [];
    
    
    
    
    
    
    
    
    
    
    
    
    for (let keyValuePair of Cu.waiveXrays(Map.prototype.entries.call(raw))) {
      let key = Cu.unwaiveXrays(keyValuePair[0]);
      let value = Cu.unwaiveXrays(keyValuePair[1]);
      key = makeDebuggeeValueIfNeeded(obj, key);
      value = makeDebuggeeValueIfNeeded(obj, value);
      entries.push([threadActor.createValueGrip(key),
                    threadActor.createValueGrip(value)]);
      if (entries.length == OBJECT_PREVIEW_MAX_ITEMS) {
        break;
      }
    }

    return true;
  }], 

  DOMStringMap: [function({obj, threadActor}, aGrip, aRawObj) {
    if (!aRawObj) {
      return false;
    }

    let keys = obj.getOwnPropertyNames();
    aGrip.preview = {
      kind: "MapLike",
      size: keys.length,
    };

    if (threadActor._gripDepth > 1) {
      return true;
    }

    let entries = aGrip.preview.entries = [];
    for (let key of keys) {
      let value = makeDebuggeeValueIfNeeded(obj, aRawObj[key]);
      entries.push([key, threadActor.createValueGrip(value)]);
      if (entries.length == OBJECT_PREVIEW_MAX_ITEMS) {
        break;
      }
    }

    return true;
  }], 

}; 


















function genericObjectPreviewer(aClassName, aClass, aObj, aThreadActor) {
  if (!aObj.proto || aObj.proto.class != aClassName) {
    return null;
  }

  let raw = aObj.unsafeDereference();
  let v = null;
  try {
    v = aClass.prototype.valueOf.call(raw);
  } catch (ex) {
    
    return null;
  }

  if (v !== null) {
    v = aThreadActor.createValueGrip(makeDebuggeeValueIfNeeded(aObj, v));
    return { value: v };
  }

  return null;
}


DebuggerServer.ObjectActorPreviewers.Object = [
  function TypedArray({obj, threadActor}, aGrip) {
    if (TYPED_ARRAY_CLASSES.indexOf(obj.class) == -1) {
      return false;
    }

    let length = DevToolsUtils.getProperty(obj, "length");
    if (typeof length != "number") {
      return false;
    }

    aGrip.preview = {
      kind: "ArrayLike",
      length: length,
    };

    if (threadActor._gripDepth > 1) {
      return true;
    }

    let raw = obj.unsafeDereference();
    let global = Cu.getGlobalForObject(DebuggerServer);
    let classProto = global[obj.class].prototype;
    
    
    let safeView = Cu.cloneInto(classProto.subarray.call(raw, 0, OBJECT_PREVIEW_MAX_ITEMS), global);
    let items = aGrip.preview.items = [];
    for (let i = 0; i < safeView.length; i++) {
      items.push(safeView[i]);
    }

    return true;
  },

  function Error({obj, threadActor}, aGrip) {
    switch (obj.class) {
      case "Error":
      case "EvalError":
      case "RangeError":
      case "ReferenceError":
      case "SyntaxError":
      case "TypeError":
      case "URIError":
        let name = DevToolsUtils.getProperty(obj, "name");
        let msg = DevToolsUtils.getProperty(obj, "message");
        let stack = DevToolsUtils.getProperty(obj, "stack");
        let fileName = DevToolsUtils.getProperty(obj, "fileName");
        let lineNumber = DevToolsUtils.getProperty(obj, "lineNumber");
        let columnNumber = DevToolsUtils.getProperty(obj, "columnNumber");
        aGrip.preview = {
          kind: "Error",
          name: threadActor.createValueGrip(name),
          message: threadActor.createValueGrip(msg),
          stack: threadActor.createValueGrip(stack),
          fileName: threadActor.createValueGrip(fileName),
          lineNumber: threadActor.createValueGrip(lineNumber),
          columnNumber: threadActor.createValueGrip(columnNumber),
        };
        return true;
      default:
        return false;
    }
  },

  function CSSMediaRule({obj, threadActor}, aGrip, aRawObj) {
    if (isWorker || !aRawObj || !(aRawObj instanceof Ci.nsIDOMCSSMediaRule)) {
      return false;
    }
    aGrip.preview = {
      kind: "ObjectWithText",
      text: threadActor.createValueGrip(aRawObj.conditionText),
    };
    return true;
  },

  function CSSStyleRule({obj, threadActor}, aGrip, aRawObj) {
    if (isWorker || !aRawObj || !(aRawObj instanceof Ci.nsIDOMCSSStyleRule)) {
      return false;
    }
    aGrip.preview = {
      kind: "ObjectWithText",
      text: threadActor.createValueGrip(aRawObj.selectorText),
    };
    return true;
  },

  function ObjectWithURL({obj, threadActor}, aGrip, aRawObj) {
    if (isWorker || !aRawObj || !(aRawObj instanceof Ci.nsIDOMCSSImportRule ||
                                  aRawObj instanceof Ci.nsIDOMCSSStyleSheet ||
                                  aRawObj instanceof Ci.nsIDOMLocation ||
                                  aRawObj instanceof Ci.nsIDOMWindow)) {
      return false;
    }

    let url;
    if (aRawObj instanceof Ci.nsIDOMWindow && aRawObj.location) {
      url = aRawObj.location.href;
    } else if (aRawObj.href) {
      url = aRawObj.href;
    } else {
      return false;
    }

    aGrip.preview = {
      kind: "ObjectWithURL",
      url: threadActor.createValueGrip(url),
    };

    return true;
  },

  function ArrayLike({obj, threadActor}, aGrip, aRawObj) {
    if (isWorker || !aRawObj ||
        obj.class != "DOMStringList" &&
        obj.class != "DOMTokenList" &&
        !(aRawObj instanceof Ci.nsIDOMMozNamedAttrMap ||
          aRawObj instanceof Ci.nsIDOMCSSRuleList ||
          aRawObj instanceof Ci.nsIDOMCSSValueList ||
          aRawObj instanceof Ci.nsIDOMFileList ||
          aRawObj instanceof Ci.nsIDOMFontFaceList ||
          aRawObj instanceof Ci.nsIDOMMediaList ||
          aRawObj instanceof Ci.nsIDOMNodeList ||
          aRawObj instanceof Ci.nsIDOMStyleSheetList)) {
      return false;
    }

    if (typeof aRawObj.length != "number") {
      return false;
    }

    aGrip.preview = {
      kind: "ArrayLike",
      length: aRawObj.length,
    };

    if (threadActor._gripDepth > 1) {
      return true;
    }

    let items = aGrip.preview.items = [];

    for (let i = 0; i < aRawObj.length &&
                    items.length < OBJECT_PREVIEW_MAX_ITEMS; i++) {
      let value = makeDebuggeeValueIfNeeded(obj, aRawObj[i]);
      items.push(threadActor.createValueGrip(value));
    }

    return true;
  }, 

  function CSSStyleDeclaration({obj, threadActor}, aGrip, aRawObj) {
    if (isWorker || !aRawObj || !(aRawObj instanceof Ci.nsIDOMCSSStyleDeclaration)) {
      return false;
    }

    aGrip.preview = {
      kind: "MapLike",
      size: aRawObj.length,
    };

    let entries = aGrip.preview.entries = [];

    for (let i = 0; i < OBJECT_PREVIEW_MAX_ITEMS &&
                    i < aRawObj.length; i++) {
      let prop = aRawObj[i];
      let value = aRawObj.getPropertyValue(prop);
      entries.push([prop, threadActor.createValueGrip(value)]);
    }

    return true;
  },

  function DOMNode({obj, threadActor}, aGrip, aRawObj) {
    if (isWorker || obj.class == "Object" || !aRawObj ||
        !(aRawObj instanceof Ci.nsIDOMNode)) {
      return false;
    }

    let preview = aGrip.preview = {
      kind: "DOMNode",
      nodeType: aRawObj.nodeType,
      nodeName: aRawObj.nodeName,
    };

    if (aRawObj instanceof Ci.nsIDOMDocument && aRawObj.location) {
      preview.location = threadActor.createValueGrip(aRawObj.location.href);
    } else if (aRawObj instanceof Ci.nsIDOMDocumentFragment) {
      preview.childNodesLength = aRawObj.childNodes.length;

      if (threadActor._gripDepth < 2) {
        preview.childNodes = [];
        for (let node of aRawObj.childNodes) {
          let actor = threadActor.createValueGrip(obj.makeDebuggeeValue(node));
          preview.childNodes.push(actor);
          if (preview.childNodes.length == OBJECT_PREVIEW_MAX_ITEMS) {
            break;
          }
        }
      }
    } else if (aRawObj instanceof Ci.nsIDOMElement) {
      
      if (aRawObj instanceof Ci.nsIDOMHTMLElement) {
        preview.nodeName = preview.nodeName.toLowerCase();
      }

      let i = 0;
      preview.attributes = {};
      preview.attributesLength = aRawObj.attributes.length;
      for (let attr of aRawObj.attributes) {
        preview.attributes[attr.nodeName] = threadActor.createValueGrip(attr.value);
        if (++i == OBJECT_PREVIEW_MAX_ITEMS) {
          break;
        }
      }
    } else if (aRawObj instanceof Ci.nsIDOMAttr) {
      preview.value = threadActor.createValueGrip(aRawObj.value);
    } else if (aRawObj instanceof Ci.nsIDOMText ||
               aRawObj instanceof Ci.nsIDOMComment) {
      preview.textContent = threadActor.createValueGrip(aRawObj.textContent);
    }

    return true;
  }, 

  function DOMEvent({obj, threadActor}, aGrip, aRawObj) {
    if (isWorker || !aRawObj || !(aRawObj instanceof Ci.nsIDOMEvent)) {
      return false;
    }

    let preview = aGrip.preview = {
      kind: "DOMEvent",
      type: aRawObj.type,
      properties: Object.create(null),
    };

    if (threadActor._gripDepth < 2) {
      let target = obj.makeDebuggeeValue(aRawObj.target);
      preview.target = threadActor.createValueGrip(target);
    }

    let props = [];
    if (aRawObj instanceof Ci.nsIDOMMouseEvent) {
      props.push("buttons", "clientX", "clientY", "layerX", "layerY");
    } else if (aRawObj instanceof Ci.nsIDOMKeyEvent) {
      let modifiers = [];
      if (aRawObj.altKey) {
        modifiers.push("Alt");
      }
      if (aRawObj.ctrlKey) {
        modifiers.push("Control");
      }
      if (aRawObj.metaKey) {
        modifiers.push("Meta");
      }
      if (aRawObj.shiftKey) {
        modifiers.push("Shift");
      }
      preview.eventKind = "key";
      preview.modifiers = modifiers;

      props.push("key", "charCode", "keyCode");
    } else if (aRawObj instanceof Ci.nsIDOMTransitionEvent) {
      props.push("propertyName", "pseudoElement");
    } else if (aRawObj instanceof Ci.nsIDOMAnimationEvent) {
      props.push("animationName", "pseudoElement");
    } else if (aRawObj instanceof Ci.nsIDOMClipboardEvent) {
      props.push("clipboardData");
    }

    
    for (let prop of props) {
      let value = aRawObj[prop];
      if (value && (typeof value == "object" || typeof value == "function")) {
        
        if (threadActor._gripDepth > 1) {
          continue;
        }
        value = obj.makeDebuggeeValue(value);
      }
      preview.properties[prop] = threadActor.createValueGrip(value);
    }

    
    if (!props.length) {
      let i = 0;
      for (let prop in aRawObj) {
        let value = aRawObj[prop];
        if (prop == "target" || prop == "type" || value === null ||
            typeof value == "function") {
          continue;
        }
        if (value && typeof value == "object") {
          if (threadActor._gripDepth > 1) {
            continue;
          }
          value = obj.makeDebuggeeValue(value);
        }
        preview.properties[prop] = threadActor.createValueGrip(value);
        if (++i == OBJECT_PREVIEW_MAX_ITEMS) {
          break;
        }
      }
    }

    return true;
  }, 

  function DOMException({obj, threadActor}, aGrip, aRawObj) {
    if (isWorker || !aRawObj || !(aRawObj instanceof Ci.nsIDOMDOMException)) {
      return false;
    }

    aGrip.preview = {
      kind: "DOMException",
      name: threadActor.createValueGrip(aRawObj.name),
      message: threadActor.createValueGrip(aRawObj.message),
      code: threadActor.createValueGrip(aRawObj.code),
      result: threadActor.createValueGrip(aRawObj.result),
      filename: threadActor.createValueGrip(aRawObj.filename),
      lineNumber: threadActor.createValueGrip(aRawObj.lineNumber),
      columnNumber: threadActor.createValueGrip(aRawObj.columnNumber),
    };

    return true;
  },

  function GenericObject(aObjectActor, aGrip) {
    let {obj, threadActor} = aObjectActor;
    if (aGrip.preview || aGrip.displayString || threadActor._gripDepth > 1) {
      return false;
    }

    let i = 0, names = [];
    let preview = aGrip.preview = {
      kind: "Object",
      ownProperties: Object.create(null),
    };

    try {
      names = obj.getOwnPropertyNames();
    } catch (ex) {
      
      
    }

    preview.ownPropertiesLength = names.length;

    for (let name of names) {
      let desc = aObjectActor._propertyDescriptor(name, true);
      if (!desc) {
        continue;
      }

      preview.ownProperties[name] = desc;
      if (++i == OBJECT_PREVIEW_MAX_ITEMS) {
        break;
      }
    }

    if (i < OBJECT_PREVIEW_MAX_ITEMS) {
      preview.safeGetterValues = aObjectActor.
                                 _findSafeGetterValues(preview.ownProperties,
                                                       OBJECT_PREVIEW_MAX_ITEMS - i);
    }

    return true;
  }, 
]; 





function PauseScopedObjectActor()
{
  ObjectActor.apply(this, arguments);
}

PauseScopedObjectActor.prototype = Object.create(PauseScopedActor.prototype);

update(PauseScopedObjectActor.prototype, ObjectActor.prototype);

update(PauseScopedObjectActor.prototype, {
  constructor: PauseScopedObjectActor,
  actorPrefix: "pausedobj",

  onOwnPropertyNames:
    PauseScopedActor.withPaused(ObjectActor.prototype.onOwnPropertyNames),

  onPrototypeAndProperties:
    PauseScopedActor.withPaused(ObjectActor.prototype.onPrototypeAndProperties),

  onPrototype: PauseScopedActor.withPaused(ObjectActor.prototype.onPrototype),
  onProperty: PauseScopedActor.withPaused(ObjectActor.prototype.onProperty),
  onDecompile: PauseScopedActor.withPaused(ObjectActor.prototype.onDecompile),

  onDisplayString:
    PauseScopedActor.withPaused(ObjectActor.prototype.onDisplayString),

  onParameterNames:
    PauseScopedActor.withPaused(ObjectActor.prototype.onParameterNames),

  






  onThreadGrip: PauseScopedActor.withPaused(function (aRequest) {
    this.threadActor.threadObjectGrip(this);
    return {};
  }),

  





  onRelease: PauseScopedActor.withPaused(function (aRequest) {
    if (this.registeredPool !== this.threadActor.threadLifetimePool) {
      return { error: "notReleasable",
               message: "Only thread-lifetime actors can be released." };
    }

    this.release();
    return {};
  }),
});

update(PauseScopedObjectActor.prototype.requestTypes, {
  "threadGrip": PauseScopedObjectActor.prototype.onThreadGrip,
});









function LongStringActor(aString)
{
  this.string = aString;
  this.stringLength = aString.length;
}

LongStringActor.prototype = {

  actorPrefix: "longString",

  disconnect: function () {
    
    
    
    if (this.registeredPool && this.registeredPool.longStringActors) {
      delete this.registeredPool.longStringActors[this.actorID];
    }
  },

  


  grip: function () {
    return {
      "type": "longString",
      "initial": this.string.substring(
        0, DebuggerServer.LONG_STRING_INITIAL_LENGTH),
      "length": this.stringLength,
      "actor": this.actorID
    };
  },

  





  onSubstring: function (aRequest) {
    return {
      "from": this.actorID,
      "substring": this.string.substring(aRequest.start, aRequest.end)
    };
  },

  


  onRelease: function () {
    
    
    
    if (this.registeredPool.longStringActors) {
      delete this.registeredPool.longStringActors[this.actorID];
    }
    this.registeredPool.removeActor(this);
    return {};
  },
};

LongStringActor.prototype.requestTypes = {
  "substring": LongStringActor.prototype.onSubstring,
  "release": LongStringActor.prototype.onRelease
};

exports.LongStringActor = LongStringActor;









function FrameActor(aFrame, aThreadActor)
{
  this.frame = aFrame;
  this.threadActor = aThreadActor;
}

FrameActor.prototype = {
  actorPrefix: "frame",

  


  _frameLifetimePool: null,
  get frameLifetimePool() {
    if (!this._frameLifetimePool) {
      this._frameLifetimePool = new ActorPool(this.conn);
      this.conn.addActorPool(this._frameLifetimePool);
    }
    return this._frameLifetimePool;
  },

  



  disconnect: function () {
    this.conn.removeActorPool(this._frameLifetimePool);
    this._frameLifetimePool = null;
  },

  


  form: function () {
    let threadActor = this.threadActor;
    let form = { actor: this.actorID,
                 type: this.frame.type };
    if (this.frame.type === "call") {
      form.callee = threadActor.createValueGrip(this.frame.callee);
    }

    if (this.frame.environment) {
      let envActor = threadActor.createEnvironmentActor(
        this.frame.environment,
        this.frameLifetimePool
      );
      form.environment = envActor.form();
    }
    form.this = threadActor.createValueGrip(this.frame.this);
    form.arguments = this._args();
    if (this.frame.script) {
      form.where = getFrameLocation(this.frame);
    }

    if (!this.frame.older) {
      form.oldest = true;
    }

    return form;
  },

  _args: function () {
    if (!this.frame.arguments) {
      return [];
    }

    return [this.threadActor.createValueGrip(arg)
            for each (arg in this.frame.arguments)];
  },

  





  onPop: function (aRequest) {
    
    if (typeof this.frame.pop != "function") {
      return { error: "notImplemented",
               message: "Popping frames is not yet implemented." };
    }

    while (this.frame != this.threadActor.dbg.getNewestFrame()) {
      this.threadActor.dbg.getNewestFrame().pop();
    }
    this.frame.pop(aRequest.completionValue);

    
    
    return { from: this.actorID };
  }
};

FrameActor.prototype.requestTypes = {
  "pop": FrameActor.prototype.onPop,
};
















function BreakpointActor(aThreadActor, { sourceActor, line, column, condition })
{
  
  
  this.scripts = new Set();

  this.threadActor = aThreadActor;
  this.location = { sourceActor, line, column };
  this.condition = condition;
}

BreakpointActor.prototype = {
  actorPrefix: "breakpoint",
  condition: null,

  








  addScript: function (aScript, aThreadActor) {
    this.threadActor = aThreadActor;
    this.scripts.add(aScript);
  },

  


  removeScripts: function () {
    for (let script of this.scripts) {
      script.clearBreakpoint(this);
    }
    this.scripts.clear();
  },

  






  isValidCondition: function(aFrame) {
    if (!this.condition) {
      return true;
    }
    var res = aFrame.eval(this.condition);
    return res.return;
  },

  





  hit: function (aFrame) {
    
    
    let loc = getFrameLocation(aFrame);
    let { sourceActor } = this.threadActor.synchronize(
      this.threadActor.sources.getOriginalLocation(loc));
    let url = sourceActor.url;

    if (this.threadActor.sources.isBlackBoxed(url)
        || aFrame.onStep
        || !this.isValidCondition(aFrame)) {
      return undefined;
    }

    let reason = {};
    if (this.threadActor._hiddenBreakpoints.has(this.actorID)) {
      reason.type = "pauseOnDOMEvents";
    } else {
      reason.type = "breakpoint";
      
      reason.actors = [ this.actorID ];
    }
    return this.threadActor._pauseAndRespond(aFrame, reason);
  },

  





  onDelete: function (aRequest) {
    
    this.threadActor.breakpointStore.removeBreakpoint(
      update({}, this.location, { source: this.location.sourceActor.form() })
    );
    this.threadActor.threadLifetimePool.removeActor(this);
    
    this.removeScripts();
    return { from: this.actorID };
  }
};

BreakpointActor.prototype.requestTypes = {
  "delete": BreakpointActor.prototype.onDelete
};











function EnvironmentActor(aEnvironment, aThreadActor)
{
  this.obj = aEnvironment;
  this.threadActor = aThreadActor;
}

EnvironmentActor.prototype = {
  actorPrefix: "environment",

  


  form: function () {
    let form = { actor: this.actorID };

    
    if (this.obj.type == "declarative") {
      form.type = this.obj.callee ? "function" : "block";
    } else {
      form.type = this.obj.type;
    }

    
    if (this.obj.parent) {
      form.parent = (this.threadActor
                     .createEnvironmentActor(this.obj.parent,
                                             this.registeredPool)
                     .form());
    }

    
    if (this.obj.type == "object" || this.obj.type == "with") {
      form.object = this.threadActor.createValueGrip(this.obj.object);
    }

    
    if (this.obj.callee) {
      form.function = this.threadActor.createValueGrip(this.obj.callee);
    }

    
    if (this.obj.type == "declarative") {
      form.bindings = this._bindings();
    }

    return form;
  },

  



  _bindings: function () {
    let bindings = { arguments: [], variables: {} };

    
    
    if (typeof this.obj.getVariable != "function") {
    
      return bindings;
    }

    let parameterNames;
    if (this.obj.callee) {
      parameterNames = this.obj.callee.parameterNames;
    }
    for each (let name in parameterNames) {
      let arg = {};

      let value = this.obj.getVariable(name);
      
      
      if (value && value.optimizedOut) {
        continue;
      }

      
      
      let desc = {
        value: value,
        configurable: false,
        writable: true,
        enumerable: true
      };

      
      let descForm = {
        enumerable: true,
        configurable: desc.configurable
      };
      if ("value" in desc) {
        descForm.value = this.threadActor.createValueGrip(desc.value);
        descForm.writable = desc.writable;
      } else {
        descForm.get = this.threadActor.createValueGrip(desc.get);
        descForm.set = this.threadActor.createValueGrip(desc.set);
      }
      arg[name] = descForm;
      bindings.arguments.push(arg);
    }

    for each (let name in this.obj.names()) {
      if (bindings.arguments.some(function exists(element) {
                                    return !!element[name];
                                  })) {
        continue;
      }

      let value = this.obj.getVariable(name);
      
      
      
      if (value && (value.optimizedOut || value.missingArguments || value.uninitialized)) {
        continue;
      }

      
      
      let desc = {
        value: value,
        configurable: false,
        writable: true,
        enumerable: true
      };

      
      let descForm = {
        enumerable: true,
        configurable: desc.configurable
      };
      if ("value" in desc) {
        descForm.value = this.threadActor.createValueGrip(desc.value);
        descForm.writable = desc.writable;
      } else {
        descForm.get = this.threadActor.createValueGrip(desc.get || undefined);
        descForm.set = this.threadActor.createValueGrip(desc.set || undefined);
      }
      bindings.variables[name] = descForm;
    }

    return bindings;
  },

  






  onAssign: function (aRequest) {
    
    
    







    try {
      this.obj.setVariable(aRequest.name, aRequest.value);
    } catch (e if e instanceof Debugger.DebuggeeWouldRun) {
        return { error: "threadWouldRun",
                 cause: e.cause ? e.cause : "setter",
                 message: "Assigning a value would cause the debuggee to run" };
    }
    return { from: this.actorID };
  },

  






  onBindings: function (aRequest) {
    return { from: this.actorID,
             bindings: this._bindings() };
  }
};

EnvironmentActor.prototype.requestTypes = {
  "assign": EnvironmentActor.prototype.onAssign,
  "bindings": EnvironmentActor.prototype.onBindings
};

exports.EnvironmentActor = EnvironmentActor;





Debugger.Script.prototype.toString = function() {
  let output = "";
  if (this.url) {
    output += this.url;
  }
  if (typeof this.startLine != "undefined") {
    output += ":" + this.startLine;
    if (this.lineCount && this.lineCount > 1) {
      output += "-" + (this.startLine + this.lineCount - 1);
    }
  }
  if (this.strictMode) {
    output += ":strict";
  }
  return output;
};





Object.defineProperty(Debugger.Frame.prototype, "line", {
  configurable: true,
  get: function() {
    if (this.script) {
      return this.script.getOffsetLine(this.offset);
    } else {
      return null;
    }
  }
});















function ChromeDebuggerActor(aConnection, aParent)
{
  ThreadActor.call(this, aParent);
}

ChromeDebuggerActor.prototype = Object.create(ThreadActor.prototype);

update(ChromeDebuggerActor.prototype, {
  constructor: ChromeDebuggerActor,

  
  actorPrefix: "chromeDebugger"
});

exports.ChromeDebuggerActor = ChromeDebuggerActor;














function AddonThreadActor(aConnect, aParent) {
  ThreadActor.call(this, aParent);
}

AddonThreadActor.prototype = Object.create(ThreadActor.prototype);

update(AddonThreadActor.prototype, {
  constructor: AddonThreadActor,

  
  actorPrefix: "addonThread",

  




  _allowSource: function(aSource) {
    let url = aSource.url;

    if (isHiddenSource(aSource)) {
      return false;
    }

    
    if (url === "resource://gre/modules/addons/XPIProvider.jsm") {
      return false;
    }

    return true;
  },

});

exports.AddonThreadActor = AddonThreadActor;





function ThreadSources(aThreadActor, aOptions, aAllowPredicate,
                       aOnNewSource) {
  this._thread = aThreadActor;
  this._useSourceMaps = aOptions.useSourceMaps;
  this._autoBlackBox = aOptions.autoBlackBox;
  this._allow = aAllowPredicate;
  this._onNewSource = aOnNewSource;
  this._anonSourceMapId = 1;

  
  this._sourceMaps = new Map();
  
  this._sourceMapCache = Object.create(null);
  
  this._sourceActors = new Map();
  
  this._sourceMappedSourceActors = Object.create(null);
}






const MINIFIED_SOURCE_REGEXP = /\bmin\.js$/;

ThreadSources.prototype = {
  















  source: function  ({ source, originalUrl, generatedSource,
              isInlineSource, contentType }) {
    dbg_assert(source || (originalUrl && generatedSource),
               "ThreadSources.prototype.source needs an originalUrl or a source");

    if (source) {
      
      

      if (!this._allow(source)) {
        return null;
      }

      
      
      
      
      
      if (source.url in this._sourceMappedSourceActors) {
        return this._sourceMappedSourceActors[source.url];
      }

      if (isInlineSource) {
        
        
        
        
        originalUrl = source.url;
        source = null;
      }
      else if (this._sourceActors.has(source)) {
        return this._sourceActors.get(source);
      }
    }
    else if (originalUrl) {
      
      
      
      
      for (let [source, actor] of this._sourceActors) {
        if (source.url === originalUrl) {
          return actor;
        }
      }

      if (originalUrl in this._sourceMappedSourceActors) {
        return this._sourceMappedSourceActors[originalUrl];
      }
    }

    let actor = new SourceActor({
      thread: this._thread,
      source: source,
      originalUrl: originalUrl,
      generatedSource: generatedSource,
      contentType: contentType
    });

    let sourceActorStore = this._thread.sourceActorStore;
    var id = sourceActorStore.getReusableActorId(source, originalUrl);
    if (id) {
      actor.actorID = id;
    }

    this._thread.threadLifetimePool.addActor(actor);
    sourceActorStore.setReusableActorId(source, originalUrl, actor.actorID);

    if (this._autoBlackBox && this._isMinifiedURL(actor.url)) {
      this.blackBox(actor.url);
    }

    if (source) {
      this._sourceActors.set(source, actor);
    }
    else {
      this._sourceMappedSourceActors[originalUrl] = actor;
    }

    
    
    
    if (!source || !this._sourceMaps.has(source)) {
      try {
        this._onNewSource(actor);
      } catch (e) {
        reportError(e);
      }
    }

    return actor;
  },

  getSource: function(source) {
    if (source.url in this._sourceMappedSourceActors) {
      return this._sourceMappedSourceActors[source.url];
    }

    if (this._sourceActors.has(source)) {
      return this._sourceActors.get(source);
    }

    throw new Error('getSource: could not find source actor for ' +
                    (source.url || 'source'));
  },

  getSourceByURL: function(url) {
    if (url) {
      for (let [source, actor] of this._sourceActors) {
        if (source.url === url) {
          return actor;
        }
      }

      if (url in this._sourceMappedSourceActors) {
        return this._sourceMappedSourceActors[url];
      }
    }

    throw new Error('getSourceByURL: could not find source for ' + url);
  },

  







  _isMinifiedURL: function (aURL) {
    try {
      let url = Services.io.newURI(aURL, null, null)
                           .QueryInterface(Ci.nsIURL);
      return MINIFIED_SOURCE_REGEXP.test(url.fileName);
    } catch (e) {
      
      
      return MINIFIED_SOURCE_REGEXP.test(aURL);
    }
  },

  


  _sourceForScript: function (aScript) {
    let url = getSourceURL(aScript.source);
    let spec = {
      source: aScript.source
    };

    
    
    
    
    
    
    if (url) {
      try {
        let urlInfo = Services.io.newURI(url, null, null).QueryInterface(Ci.nsIURL);
        if (urlInfo.fileExtension === "html") {
          spec.isInlineSource = true;
        }
        else if (urlInfo.fileExtension === "js") {
          spec.contentType = "text/javascript";
        }
      } catch(ex) {
        
      }
    }
    else {
      
      spec.contentType = "text/javascript";
    }

    return this.source(spec);
  },

  







  sourcesForScript: function (aScript) {
    if (!this._useSourceMaps || !aScript.source.sourceMapURL) {
      return resolve([this._sourceForScript(aScript)].filter(isNotNull));
    }

    return this.fetchSourceMap(aScript.source)
      .then(map => {
        if (map) {
          return [
            this.source({ originalUrl: s,
                          generatedSource: aScript.source })
            for (s of map.sources)
          ];
        }

        return [this._sourceForScript(aScript)];
      })
      .then(ss => ss.filter(isNotNull));
  },

  





  fetchSourceMap: function (aSource) {
    if (this._sourceMaps.has(aSource)) {
      return this._sourceMaps.get(aSource);
    }
    else if (!aSource || !aSource.sourceMapURL) {
      return resolve(null);
    }

    let sourceMapURL = aSource.sourceMapURL;
    if (aSource.url) {
      sourceMapURL = this._normalize(sourceMapURL, aSource.url);
    }

    let map = this._fetchSourceMap(sourceMapURL, aSource.url);
    if (map) {
      this._sourceMaps.set(aSource, map);
      return map;
    }
    return resolve(null);
  },

  




  getSourceMap: function(aSource) {
    return resolve(this._sourceMaps.get(aSource));
  },

  



  setSourceMap: function(aSource, aMap) {
    this._sourceMaps.set(aSource, resolve(aMap));
  },

  












  _fetchSourceMap: function (aAbsSourceMapURL, aSourceURL) {
    if (this._sourceMapCache[aAbsSourceMapURL]) {
      return this._sourceMapCache[aAbsSourceMapURL];
    }
    else if (!this._useSourceMaps) {
      return null;
    }

    let fetching = fetch(aAbsSourceMapURL, { loadFromCache: false })
      .then(({ content }) => {
        let map = new SourceMapConsumer(content);
        this._setSourceMapRoot(map, aAbsSourceMapURL, aSourceURL);
        return map;
      })
      .then(null, error => {
        if (!DevToolsUtils.reportingDisabled) {
          DevToolsUtils.reportException("ThreadSources.prototype.getOriginalLocation", error);
        }
        return null;
      });
    this._sourceMapCache[aAbsSourceMapURL] = fetching;
    return fetching;
  },

  


  _setSourceMapRoot: function (aSourceMap, aAbsSourceMapURL, aScriptURL) {
    const base = this._dirname(
      aAbsSourceMapURL.indexOf("data:") === 0
        ? aScriptURL
        : aAbsSourceMapURL);
    aSourceMap.sourceRoot = aSourceMap.sourceRoot
      ? this._normalize(aSourceMap.sourceRoot, base)
      : base;
  },

  _dirname: function (aPath) {
    return Services.io.newURI(
      ".", null, Services.io.newURI(aPath, null, null)).spec;
  },

  














  clearSourceMapCache: function(aSourceMapURL, opts = { hard: false }) {
    let oldSm = this._sourceMapCache[aSourceMapURL];

    if (opts.hard) {
      delete this._sourceMapCache[aSourceMapURL];
    }

    if (oldSm) {
      
      for (let [source, sm] of this._sourceMaps.entries()) {
        if (sm === oldSm) {
          this._sourceMaps.delete(source);
        }
      }
    }
  },

  













  setSourceMapHard: function(aSource, aUrl, aMap) {
    let url = aUrl;
    if (!url) {
      
      
      
      
      
      
      
      url = "internal://sourcemap" + (this._anonSourceMapId++) + '/';
    }
    aSource.sourceMapURL = url;

    
    
    this._sourceMapCache[url] = resolve(aMap);
  },

  



  getOriginalLocation: function ({ source, line, column }) {
    
    
    
    
    
    return this.fetchSourceMap(source).then(sm => {
      if (sm) {
        let {
          source: sourceUrl,
          line: sourceLine,
          column: sourceCol,
          name: sourceName
        } = sm.originalPositionFor({
          line: line,
          column: column == null ? Infinity : column
        });

        return {
          sourceActor: sourceUrl && this.source({ originalUrl: sourceUrl }),
          url: sourceUrl,
          line: sourceLine,
          column: sourceCol,
          name: sourceName
        };
      }

      
      return resolve({
        
        
        sourceActor: this.source({ source }),
        url: source.url,
        line: line,
        column: column
      });
    });
  },

  








  getGeneratedLocation: function ({ sourceActor, line, column }) {
    
    
    
    
    let source = sourceActor.generatedSource || sourceActor.source;

    
    return this.fetchSourceMap(source).then(sm => {
      if (sm) {
        let { line: genLine, column: genColumn } = sm.generatedPositionFor({
          source: sourceActor.url,
          line: line,
          column: column == null ? Infinity : column
        });

        return {
          
          
          sourceActor: this.source({ source: source }),
          line: genLine,
          column: genColumn
        };
      }

      return resolve({
        sourceActor: sourceActor,
        line: line,
        column: column
      });
    });
  },

  






  isBlackBoxed: function (aURL) {
    return this._thread.blackBoxedSources.has(aURL);
  },

  





  blackBox: function (aURL) {
    this._thread.blackBoxedSources.add(aURL);
  },

  





  unblackBox: function (aURL) {
    this._thread.blackBoxedSources.delete(aURL);
  },

  





  isPrettyPrinted: function (aURL) {
    return this._thread.prettyPrintedSources.has(aURL);
  },

  





  prettyPrint: function (aURL, aIndent) {
    this._thread.prettyPrintedSources.set(aURL, aIndent);
  },

  


  prettyPrintIndent: function (aURL) {
    return this._thread.prettyPrintedSources.get(aURL);
  },

  





  disablePrettyPrint: function (aURL) {
    this._thread.prettyPrintedSources.delete(aURL);
  },

  


  _normalize: function (...aURLs) {
    dbg_assert(aURLs.length > 1, "Should have more than 1 URL");
    let base = Services.io.newURI(aURLs.pop(), null, null);
    let url;
    while ((url = aURLs.pop())) {
      base = Services.io.newURI(url, null, base);
    }
    return base.spec;
  },

  iter: function () {
    let actors = Object.keys(this._sourceMappedSourceActors).map(k => {
      return this._sourceMappedSourceActors[k];
    });
    for (let actor of this._sourceActors.values()) {
      if (!this._sourceMaps.has(actor.source)) {
        actors.push(actor);
      }
    }
    return actors;
  }
};

exports.ThreadSources = ThreadSources;







function isHiddenSource(aSource) {
  
  return aSource.text === '() {\n}';
}










function getFrameLocation(aFrame) {
  if (!aFrame || !aFrame.script) {
    return { source: null, line: null, column: null };
  }
  return {
    source: aFrame.script.source,
    line: aFrame.script.getOffsetLine(aFrame.offset),
    column: getOffsetColumn(aFrame.offset, aFrame.script)
  }
}




function isNotNull(aThing) {
  return aThing !== null;
}









let oldReportError = reportError;
reportError = function(aError, aPrefix="") {
  dbg_assert(aError instanceof Error, "Must pass Error objects to reportError");
  let msg = aPrefix + aError.message + ":\n" + aError.stack;
  oldReportError(msg);
  dumpn(msg);
}














function makeDebuggeeValueIfNeeded(obj, value) {
  if (value && (typeof value == "object" || typeof value == "function")) {
    return obj.makeDebuggeeValue(value);
  }
  return value;
}

function getInnerId(window) {
  return window.QueryInterface(Ci.nsIInterfaceRequestor).
                getInterface(Ci.nsIDOMWindowUtils).currentInnerWindowID;
};

const symbolProtoToString = typeof Symbol === "function" ? Symbol.prototype.toString : null;

function getSymbolName(symbol) {
  const name = symbolProtoToString.call(symbol).slice("Symbol(".length, -1);
  return name || undefined;
}

function getSourceURL(source) {
  let introType = source.introductionType;
  
  
  
  
  if (introType === 'eval' ||
      introType === 'Function' ||
      introType === 'eventHandler' ||
      introType === 'setTimeout' ||
      introType === 'setInterval') {
    return source.displayURL;
  }
  return source.url;
}
