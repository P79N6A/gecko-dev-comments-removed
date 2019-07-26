





"use strict";






function BreakpointStore() {
  
  
  
  
  
  
  
  
  
  this._wholeLineBreakpoints = Object.create(null);

  
  
  
  
  
  
  
  
  
  this._breakpoints = Object.create(null);
}

BreakpointStore.prototype = {

  











  addBreakpoint: function BS_addBreakpoint(aBreakpoint) {
    let { url, line, column } = aBreakpoint;

    if (column != null) {
      if (!this._breakpoints[url]) {
        this._breakpoints[url] = [];
      }
      if (!this._breakpoints[url][line]) {
        this._breakpoints[url][line] = [];
      }
      this._breakpoints[url][line][column] = aBreakpoint;
    } else {
      
      if (!this._wholeLineBreakpoints[url]) {
        this._wholeLineBreakpoints[url] = [];
      }
      this._wholeLineBreakpoints[url][line] = aBreakpoint;
    }
  },

  









  removeBreakpoint: function BS_removeBreakpoint({ url, line, column }) {
    if (column != null) {
      if (this._breakpoints[url]) {
        if (this._breakpoints[url][line]) {
          delete this._breakpoints[url][line][column];

          
          
          
          
          
          
          
          if (Object.keys(this._breakpoints[url][line]).length === 0) {
            delete this._breakpoints[url][line];
          }
        }
      }
    } else {
      if (this._wholeLineBreakpoints[url]) {
        delete this._wholeLineBreakpoints[url][line];
      }
    }
  },

  










  getBreakpoint: function BS_getBreakpoint(aLocation) {
    let { url, line, column } = aLocation;
    dbg_assert(url != null);
    dbg_assert(line != null);

    var foundBreakpoint = this.hasBreakpoint(aLocation);
    if (foundBreakpoint == null) {
      throw new Error("No breakpoint at url = " + url
          + ", line = " + line
          + ", column = " + column);
    }

    return foundBreakpoint;
  },

  










  hasBreakpoint: function BS_hasBreakpoint(aLocation) {
    let { url, line, column } = aLocation;
    dbg_assert(url != null);
    dbg_assert(line != null);
    for (let bp of this.findBreakpoints(aLocation)) {
      
      
      
      
      return bp;
    }

    return null;
  },

  










  findBreakpoints: function BS_findBreakpoints(aSearchParams={}) {
    if (aSearchParams.column != null) {
      dbg_assert(aSearchParams.line != null);
    }
    if (aSearchParams.line != null) {
      dbg_assert(aSearchParams.url != null);
    }

    for (let url of this._iterUrls(aSearchParams.url)) {
      for (let line of this._iterLines(url, aSearchParams.line)) {
        
        
        if (aSearchParams.column == null
            && this._wholeLineBreakpoints[url]
            && this._wholeLineBreakpoints[url][line]) {
          yield this._wholeLineBreakpoints[url][line];
        }
        for (let column of this._iterColumns(url, line, aSearchParams.column)) {
          yield this._breakpoints[url][line][column];
        }
      }
    }
  },

  _iterUrls: function BS__iterUrls(aUrl) {
    if (aUrl) {
      if (this._breakpoints[aUrl] || this._wholeLineBreakpoints[aUrl]) {
        yield aUrl;
      }
    } else {
      for (let url of Object.keys(this._wholeLineBreakpoints)) {
        yield url;
      }
      for (let url of Object.keys(this._breakpoints)) {
        if (url in this._wholeLineBreakpoints) {
          continue;
        }
        yield url;
      }
    }
  },

  _iterLines: function BS__iterLines(aUrl, aLine) {
    if (aLine != null) {
      if ((this._wholeLineBreakpoints[aUrl]
           && this._wholeLineBreakpoints[aUrl][aLine])
          || (this._breakpoints[aUrl] && this._breakpoints[aUrl][aLine])) {
        yield aLine;
      }
    } else {
      const wholeLines = this._wholeLineBreakpoints[aUrl]
        ? Object.keys(this._wholeLineBreakpoints[aUrl])
        : [];
      const columnLines = this._breakpoints[aUrl]
        ? Object.keys(this._breakpoints[aUrl])
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

  _iterColumns: function BS__iterColumns(aUrl, aLine, aColumn) {
    if (!this._breakpoints[aUrl] || !this._breakpoints[aUrl][aLine]) {
      return;
    }

    if (aColumn != null) {
      if (this._breakpoints[aUrl][aLine][aColumn]) {
        yield aColumn;
      }
    } else {
      for (let column in this._breakpoints[aUrl][aLine]) {
        yield column;
      }
    }
  },
};

















function EventLoopStack({ inspector, thread, hooks }) {
  this._inspector = inspector;
  this._hooks = hooks;
  this._thread = thread;
}

EventLoopStack.prototype = {
  


  get size() {
    return this._inspector.eventLoopNestLevel;
  },

  


  get lastPausedUrl() {
    let url = null;
    if (this.size > 0) {
      try {
        url = this._inspector.lastNestRequestor.url
      } catch (e) {
        
        
        dumpn(e);
      }
    }
    return url;
  },

  




  push: function () {
    return new EventLoop({
      inspector: this._inspector,
      thread: this._thread,
      hooks: this._hooks
    });
  }
};













function EventLoop({ inspector, thread, hooks }) {
  this._inspector = inspector;
  this._thread = thread;
  this._hooks = hooks;

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
    this._inspector.enterNestedEventLoop(this);

    
    if (this._inspector.eventLoopNestLevel > 0) {
      const { resolved } = this._inspector.lastNestRequestor;
      if (resolved) {
        this._inspector.exitNestedEventLoop();
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
    if (this === this._inspector.lastNestRequestor) {
      this._inspector.exitNestedEventLoop();
      return true;
    }
    return false;
  },
};



















function ThreadActor(aHooks, aGlobal)
{
  this._state = "detached";
  this._frameActors = [];
  this._environmentActors = [];
  this._hooks = aHooks;
  this.global = aGlobal;
  this._nestedEventLoops = new EventLoopStack({
    inspector: DebuggerServer.xpcInspector,
    hooks: aHooks,
    thread: this
  });
  
  this._hiddenBreakpoints = new Map();

  this.findGlobals = this.globalManager.findGlobals.bind(this);
  this.onNewGlobal = this.globalManager.onNewGlobal.bind(this);
  this.onNewSource = this.onNewSource.bind(this);
  this._allEventsListener = this._allEventsListener.bind(this);

  this._options = {
    useSourceMaps: false
  };
}





ThreadActor.breakpointStore = new BreakpointStore();

ThreadActor.prototype = {
  actorPrefix: "context",

  get state() { return this._state; },
  get attached() this.state == "attached" ||
                 this.state == "running" ||
                 this.state == "paused",

  get breakpointStore() { return ThreadActor.breakpointStore; },

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
      this._sources = new ThreadSources(this, this._options.useSourceMaps,
                                        this._allowSource, this.onNewSource);
    }
    return this._sources;
  },

  _prettyPrintWorker: null,
  get prettyPrintWorker() {
    if (!this._prettyPrintWorker) {
      this._prettyPrintWorker = new ChromeWorker(
        "resource://gre/modules/devtools/server/actors/pretty-print-worker.js");

      this._prettyPrintWorker.addEventListener(
        "error", this._onPrettyPrintError, false);

      if (wantLogging) {
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

  _onPrettyPrintError: function (error) {
    reportError(new Error(error));
  },

  _onPrettyPrintMsg: function ({ data }) {
    dumpn("Received message from prettyPrintWorker: "
          + JSON.stringify(data, null, 2) + "\n");
  },

  





  _threadPauseEventLoops: null,
  _pushThreadPause: function TA__pushThreadPause() {
    if (!this._threadPauseEventLoops) {
      this._threadPauseEventLoops = [];
    }
    const eventLoop = this._nestedEventLoops.push();
    this._threadPauseEventLoops.push(eventLoop);
    eventLoop.enter();
  },
  _popThreadPause: function TA__popThreadPause() {
    const eventLoop = this._threadPauseEventLoops.pop();
    dbg_assert(eventLoop, "Should have an event loop.");
    eventLoop.resolve();
  },

  clearDebuggees: function TA_clearDebuggees() {
    if (this.dbg) {
      this.dbg.removeAllDebuggees();
    }
    this.conn.removeActorPool(this._threadLifetimePool || undefined);
    this._threadLifetimePool = null;
    this._sources = null;
  },

  




  addDebuggee: function TA_addDebuggee(aGlobal) {
    let globalDebugObject;
    try {
      globalDebugObject = this.dbg.addDebuggee(aGlobal);
    } catch (e) {
      
      dumpn("Ignoring request to add the debugger's compartment as a debuggee");
    }
    return globalDebugObject;
  },

  


  _initDebugger: function TA__initDebugger() {
    this.dbg = new Debugger();
    this.dbg.uncaughtExceptionHook = this.uncaughtExceptionHook.bind(this);
    this.dbg.onDebuggerStatement = this.onDebuggerStatement.bind(this);
    this.dbg.onNewScript = this.onNewScript.bind(this);
    this.dbg.onNewGlobalObject = this.globalManager.onNewGlobal.bind(this);
    
    this.dbg.enabled = this._state != "detached";
  },

  


  removeDebugee: function TA_removeDebuggee(aGlobal) {
    try {
      this.dbg.removeDebuggee(aGlobal);
    } catch(ex) {
      
      
    }
  },

  




  _addDebuggees: function TA__addDebuggees(aWindow) {
    let globalDebugObject = this.addDebuggee(aWindow);
    let frames = aWindow.frames;
    if (frames) {
      for (let i = 0; i < frames.length; i++) {
        this._addDebuggees(frames[i]);
      }
    }
    return globalDebugObject;
  },

  



  globalManager: {
    findGlobals: function TA_findGlobals() {
      this.globalDebugObject = this._addDebuggees(this.global);
    },

    






    onNewGlobal: function TA_onNewGlobal(aGlobal) {
      
      
      if (aGlobal.hostAnnotations &&
          aGlobal.hostAnnotations.type == "document" &&
          aGlobal.hostAnnotations.element === this.global) {
        this.addDebuggee(aGlobal);
        
        this.conn.send({
          from: this.actorID,
          type: "newGlobal",
          
          hostAnnotations: aGlobal.hostAnnotations
        });
      }
    }
  },

  disconnect: function TA_disconnect() {
    dumpn("in ThreadActor.prototype.disconnect");
    if (this._state == "paused") {
      this.onResume();
    }

    this._state = "exited";

    this.clearDebuggees();

    if (this._prettyPrintWorker) {
      this._prettyPrintWorker.removeEventListener(
        "error", this._onPrettyPrintError, false);
      this._prettyPrintWorker.removeEventListener(
        "message", this._onPrettyPrintMsg, false);
      this._prettyPrintWorker.terminate();
      this._prettyPrintWorker = null;
    }

    if (!this.dbg) {
      return;
    }
    this.dbg.enabled = false;
    this.dbg = null;
  },

  


  exit: function TA_exit() {
    this.disconnect();
  },

  
  onAttach: function TA_onAttach(aRequest) {
    if (this.state === "exited") {
      return { type: "exited" };
    }

    if (this.state !== "detached") {
      return { error: "wrongState" };
    }

    this._state = "attached";

    update(this._options, aRequest.options || {});

    if (!this.dbg) {
      this._initDebugger();
    }
    this.findGlobals();
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

  onDetach: function TA_onDetach(aRequest) {
    this.disconnect();
    dumpn("ThreadActor.prototype.onDetach: returning 'detached' packet");
    return {
      type: "detached"
    };
  },

  onReconfigure: function TA_onReconfigure(aRequest) {
    if (this.state == "exited") {
      return { error: "wrongState" };
    }

    update(this._options, aRequest.options || {});
    
    this._sources = null;

    return {};
  },

  











  _pauseAndRespond: function TA__pauseAndRespond(aFrame, aReason,
                                                 onPacket=function (k) { return k; }) {
    try {
      let packet = this._paused(aFrame);
      if (!packet) {
        return undefined;
      }
      packet.why = aReason;

      this.sources.getOriginalLocation(packet.frame.where).then(aOrigPosition => {
        packet.frame.where = aOrigPosition;
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

    return undefined;
  },

  






  _forceCompletion: function TA__forceCompletion(aRequest) {
    
    
    return {
      error: "notImplemented",
      message: "forced completion is not yet implemented."
    };
  },

  _makeOnEnterFrame: function TA__makeOnEnterFrame({ pauseAndRespond }) {
    return aFrame => {
      const generatedLocation = getFrameLocation(aFrame);
      let { url } = this.synchronize(this.sources.getOriginalLocation(
        generatedLocation));

      return this.sources.isBlackBoxed(url)
        ? undefined
        : pauseAndRespond(aFrame);
    };
  },

  _makeOnPop: function TA__makeOnPop({ thread, pauseAndRespond, createValueGrip }) {
    return function (aCompletion) {
      

      const generatedLocation = getFrameLocation(this);
      const { url } = thread.synchronize(thread.sources.getOriginalLocation(
        generatedLocation));

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

  _makeOnStep: function TA__makeOnStep({ thread, pauseAndRespond, startFrame,
                                         startLocation }) {
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

  


  _makeSteppingHooks: function TA__makeSteppingHooks(aStartLocation) {
    
    
    
    
    const steppingHookState = {
      pauseAndRespond: (aFrame, onPacket=(k)=>k) => {
        this._pauseAndRespond(aFrame, { type: "resumeLimit" }, onPacket);
      },
      createValueGrip: this.createValueGrip.bind(this),
      thread: this,
      startFrame: this.youngestFrame,
      startLocation: aStartLocation
    };

    return {
      onEnterFrame: this._makeOnEnterFrame(steppingHookState),
      onPop: this._makeOnPop(steppingHookState),
      onStep: this._makeOnStep(steppingHookState)
    };
  },

  








  _handleResumeLimit: function TA__handleResumeLimit(aRequest) {
    let steppingType = aRequest.resumeLimit.type;
    if (["step", "next", "finish"].indexOf(steppingType) == -1) {
      return reject({ error: "badParameterType",
                      message: "Unknown resumeLimit type" });
    }

    const generatedLocation = getFrameLocation(this.youngestFrame);
    return this.sources.getOriginalLocation(generatedLocation)
      .then(originalLocation => {
        const { onEnterFrame, onPop, onStep } = this._makeSteppingHooks(originalLocation);

        
        
        let stepFrame = this._getNextStepFrame(this.youngestFrame);
        if (stepFrame) {
          switch (steppingType) {
            case "step":
              this.dbg.onEnterFrame = onEnterFrame;
              
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

  






  _clearSteppingHooks: function TA__clearSteppingHooks(aFrame) {
    while (aFrame) {
      aFrame.onStep = undefined;
      aFrame.onPop = undefined;
      aFrame = aFrame.older;
    }
  },

  





  _maybeListenToEvents: function TA__maybeListenToEvents(aRequest) {
    
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

  


  onResume: function TA_onResume(aRequest) {
    if (this._state !== "paused") {
      return {
        error: "wrongState",
        message: "Can't resume when debuggee isn't paused. Current state is '"
          + this._state + "'"
      };
    }

    
    
    
    if (this._nestedEventLoops.size && this._nestedEventLoops.lastPausedUrl
        && this._nestedEventLoops.lastPausedUrl !== this._hooks.url) {
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
      return packet;
    }, error => {
      return error instanceof Error
        ? { error: "unknownError",
            message: safeErrorString(error) }
        
        
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
        l.script = this.globalDebugObject.makeDebuggeeValue(listener).script;
        
        
        if (!l.script)
          continue;
        listeners.push(l);
      }
    }
    return listeners;
  },

  


  _breakOnEnter: function(script) {
    let offsets = script.getAllOffsets();
    for (let line = 0, n = offsets.length; line < n; line++) {
      if (offsets[line]) {
        let location = { url: script.url, line: line };
        let resp = this._createAndStoreBreakpoint(location);
        dbg_assert(!resp.actualLocation, "No actualLocation should be returned");
        if (resp.error) {
          reportError(new Error("Unable to set breakpoint on event listener"));
          return;
        }
        let bp = this.breakpointStore.getBreakpoint(location);
        let bpActor = bp.actor;
        dbg_assert(bp, "Breakpoint must exist");
        dbg_assert(bpActor, "Breakpoint actor must be created");
        this._hiddenBreakpoints.set(bpActor.actorID, bpActor);
        break;
      }
    }
  },

  


  _getNextStepFrame: function TA__getNextStepFrame(aFrame) {
    let stepFrame = aFrame.reportedPop ? aFrame.older : aFrame;
    if (!stepFrame || !stepFrame.script) {
      stepFrame = null;
    }
    return stepFrame;
  },

  onClientEvaluate: function TA_onClientEvaluate(aRequest) {
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

  onFrames: function TA_onFrames(aRequest) {
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

      let promise = this.sources.getOriginalLocation(form.where)
        .then((aOrigLocation) => {
          form.where = aOrigLocation;
          let source = this.sources.source({ url: form.where.url });
          if (source) {
            form.source = source.form();
          }
        });
      promises.push(promise);
    }

    return all(promises).then(function () {
      return { frames: frames };
    });
  },

  onReleaseMany: function TA_onReleaseMany(aRequest) {
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

  


  onSetBreakpoint: function TA_onSetBreakpoint(aRequest) {
    if (this.state !== "paused") {
      return { error: "wrongState",
               message: "Breakpoints can only be set while the debuggee is paused."};
    }

    let { url: originalSource,
          line: originalLine,
          column: originalColumn } = aRequest.location;

    let locationPromise = this.sources.getGeneratedLocation(aRequest.location);
    return locationPromise.then(({url, line, column}) => {
      if (line == null ||
          line < 0 ||
          this.dbg.findScripts({ url: url }).length == 0) {
        return { error: "noScript" };
      }

      let response = this._createAndStoreBreakpoint({
        url: url,
        line: line,
        column: column
      });
      
      
      
      let originalLocation = this.sources.getOriginalLocation({
        url: url,
        line: line,
        column: column
      });

      return all([response, originalLocation])
        .then(([aResponse, {url, line}]) => {
          if (aResponse.actualLocation) {
            let actualOrigLocation = this.sources.getOriginalLocation(aResponse.actualLocation);
            return actualOrigLocation.then(({ url, line, column }) => {
              if (url !== originalSource
                  || line !== originalLine
                  || column !== originalColumn) {
                aResponse.actualLocation = {
                  url: url,
                  line: line,
                  column: column
                };
              }
              return aResponse;
            });
          }

          if (url !== originalSource || line !== originalLine) {
            aResponse.actualLocation = { url: url, line: line };
          }

          return aResponse;
        });
    });
  },

  






  _createAndStoreBreakpoint: function (aLocation) {
    
    
    this.breakpointStore.addBreakpoint(aLocation);
    return this._setBreakpoint(aLocation);
  },

  










  _setBreakpoint: function TA__setBreakpoint(aLocation) {
    let actor;
    let storedBp = this.breakpointStore.getBreakpoint(aLocation);
    if (storedBp.actor) {
      actor = storedBp.actor;
    } else {
      storedBp.actor = actor = new BreakpointActor(this, {
        url: aLocation.url,
        line: aLocation.line,
        column: aLocation.column
      });
      this._hooks.addToParentPool(actor);
    }

    
    let scripts = this.dbg.findScripts(aLocation);
    if (scripts.length == 0) {
      return {
        error: "noScript",
        actor: actor.actorID
      };
    }

   




    
    let scriptsAndOffsetMappings = new Map();

    for (let script of scripts) {
      this._findClosestOffsetMappings(aLocation,
                                      script,
                                      scriptsAndOffsetMappings);
    }

    if (scriptsAndOffsetMappings.size > 0) {
      for (let [script, mappings] of scriptsAndOffsetMappings) {
        for (let offsetMapping of mappings) {
          script.setBreakpoint(offsetMapping.offset, actor);
        }
        actor.addScript(script, this);
      }

      return {
        actor: actor.actorID
      };
    }

   






    
    let scripts = this.dbg.findScripts({
      url: aLocation.url,
      line: aLocation.line,
      innermost: true
    });

    




    let actualLocation;
    let found = false;
    for (let script of scripts) {
      let offsets = script.getAllOffsets();
      for (let line = aLocation.line; line < offsets.length; ++line) {
        if (offsets[line]) {
          for (let offset of offsets[line]) {
            script.setBreakpoint(offset, actor);
          }
          actor.addScript(script, this);
          if (!actualLocation) {
            actualLocation = {
              url: aLocation.url,
              line: line,
              column: 0
            };
          }
          found = true;
          break;
        }
      }
    }
    if (found) {
      let existingBp = this.breakpointStore.hasBreakpoint(actualLocation);

      if (existingBp && existingBp.actor) {
        




        actor.onDelete();
        this.breakpointStore.removeBreakpoint(aLocation);
        return {
          actor: existingBp.actor.actorID,
          actualLocation: actualLocation
        };
      } else {
        




        actor.location = actualLocation;
        this.breakpointStore.addBreakpoint({
          actor: actor,
          url: actualLocation.url,
          line: actualLocation.line,
          column: actualLocation.column
        });
        this.breakpointStore.removeBreakpoint(aLocation);
        return {
          actor: actor.actorID,
          actualLocation: actualLocation
        };
      }
    }

    



    return {
      error: "noCodeAtLineColumn",
      actor: actor.actorID
    };
  },

  















  _findClosestOffsetMappings: function TA__findClosestOffsetMappings(aTargetLocation,
                                                                     aScript,
                                                                     aScriptsAndOffsetMappings) {
    
    

    if (aTargetLocation.column == null) {
      let offsetMappings = aScript.getLineOffsets(aTargetLocation.line)
        .map(o => ({
          line: aTargetLocation.line,
          offset: o
        }));
      if (offsetMappings.length) {
        aScriptsAndOffsetMappings.set(aScript, offsetMappings);
      }
      return;
    }

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
  },

  





  _discoverSources: function TA__discoverSources() {
    
    let scriptsByUrl = {};
    for (let s of this.dbg.findScripts()) {
      scriptsByUrl[s.url] = s;
    }

    return all([this.sources.sourcesForScript(scriptsByUrl[s])
                for (s of Object.keys(scriptsByUrl))]);
  },

  onSources: function TA_onSources(aRequest) {
    return this._discoverSources().then(() => {
      return {
        sources: [s.form() for (s of this.sources.iter())]
      };
    });
  },

  






  disableAllBreakpoints: function () {
    for (let bp of this.breakpointStore.findBreakpoints()) {
      if (bp.actor) {
        bp.actor.removeScripts();
      }
    }
  },

  


  onInterrupt: function TA_onInterrupt(aRequest) {
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

  


  onEventListeners: function TA_onEventListeners(aRequest) {
    
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
        
        
        if (!listener) {
          continue;
        }

        
        let selector = node.tagName ? findCssSelector(node) : "window";
        let nodeDO = this.globalDebugObject.makeDebuggeeValue(node);
        listenerForm.node = {
          selector: selector,
          object: this.createValueGrip(nodeDO)
        };
        listenerForm.type = handler.type;
        listenerForm.capturing = handler.capturing;
        listenerForm.allowsUntrusted = handler.allowsUntrusted;
        listenerForm.inSystemEventGroup = handler.inSystemEventGroup;
        listenerForm.isEventHandler = !!node["on" + listenerForm.type];
        
        let listenerDO = this.globalDebugObject.makeDebuggeeValue(listener);
        listenerForm.function = this.createValueGrip(listenerDO);
        listeners.push(listenerForm);
      }
    }
    return { listeners: listeners };
  },

  


  _requestFrame: function TA_requestFrame(aFrameID) {
    if (!aFrameID) {
      return this.youngestFrame;
    }

    if (this._framePool.has(aFrameID)) {
      return this._framePool.get(aFrameID).frame;
    }

    return undefined;
  },

  _paused: function TA__paused(aFrame) {
    
    
    
    
    
    

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

    
    
    this.youngestFrame = aFrame;

    
    
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

  _resumed: function TA_resumed() {
    this._state = "running";

    
    this.conn.removeActorPool(this._pausePool);

    this._pausePool = null;
    this._pauseActor = null;
    this.youngestFrame = null;

    return { from: this.actorID, type: "resumed" };
  },

  




  _updateFrames: function TA_updateFrames() {
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

  _createFrameActor: function TA_createFrameActor(aFrame) {
    if (aFrame.actor) {
      return aFrame.actor;
    }

    let actor = new FrameActor(aFrame, this);
    this._frameActors.push(actor);
    this._framePool.addActor(actor);
    aFrame.actor = actor;

    return actor;
  },

  









  createEnvironmentActor:
  function TA_createEnvironmentActor(aEnvironment, aPool) {
    if (!aEnvironment) {
      return undefined;
    }

    if (aEnvironment.actor) {
      return aEnvironment.actor;
    }

    let actor = new EnvironmentActor(aEnvironment, this);
    this._environmentActors.push(actor);
    aPool.addActor(actor);
    aEnvironment.actor = actor;

    return actor;
  },

  



  createValueGrip: function TA_createValueGrip(aValue, aPool=false) {
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
      default:
        dbg_assert(false, "Failed to provide a grip for: " + aValue);
        return null;
    }
  },

  



  createProtocolCompletionValue:
  function TA_createProtocolCompletionValue(aCompletion) {
    let protoValue = {};
    if ("return" in aCompletion) {
      protoValue.return = this.createValueGrip(aCompletion.return);
    } else if ("yield" in aCompletion) {
      protoValue.return = this.createValueGrip(aCompletion.yield);
    } else if ("throw" in aCompletion) {
      protoValue.throw = this.createValueGrip(aCompletion.throw);
    } else {
      protoValue.terminated = true;
    }
    return protoValue;
  },

  







  objectGrip: function TA_objectGrip(aValue, aPool) {
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

  





  pauseObjectGrip: function TA_pauseObjectGrip(aValue) {
    if (!this._pausePool) {
      throw "Object grip requested while not paused.";
    }

    return this.objectGrip(aValue, this._pausePool);
  },

  





  threadObjectGrip: function TA_threadObjectGrip(aActor) {
    
    
    aActor.registeredPool.objectActors.delete(aActor.obj);
    this.threadLifetimePool.addActor(aActor);
    this.threadLifetimePool.objectActors.set(aActor.obj, aActor);
  },

  






  onThreadGrips: function OA_onThreadGrips(aRequest) {
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

  







  longStringGrip: function TA_longStringGrip(aString, aPool) {
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

  





  pauseLongStringGrip: function TA_pauseLongStringGrip (aString) {
    return this.longStringGrip(aString, this._pausePool);
  },

  





  threadLongStringGrip: function TA_pauseLongStringGrip (aString) {
    return this.longStringGrip(aString, this._threadLifetimePool);
  },

  






  _stringIsLong: function TA__stringIsLong(aString) {
    return aString.length >= DebuggerServer.LONG_STRING_LENGTH;
  },

  

  







  uncaughtExceptionHook: function TA_uncaughtExceptionHook(aException) {
    dumpn("Got an exception: " + aException.message + "\n" + aException.stack);
  },

  






  onDebuggerStatement: function TA_onDebuggerStatement(aFrame) {
    
    
    const generatedLocation = getFrameLocation(aFrame);
    const { url } = this.synchronize(this.sources.getOriginalLocation(
      generatedLocation));

    return this.sources.isBlackBoxed(url) || aFrame.onStep
      ? undefined
      : this._pauseAndRespond(aFrame, { type: "debuggerStatement" });
  },

  








  onExceptionUnwind: function TA_onExceptionUnwind(aFrame, aValue) {
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
    const { url } = this.synchronize(this.sources.getOriginalLocation(
      generatedLocation));

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

  








  onNewScript: function TA_onNewScript(aScript, aGlobal) {
    this._addScript(aScript);
    this.sources.sourcesForScript(aScript);
  },

  onNewSource: function TA_onNewSource(aSource) {
    this.conn.send({
      from: this.actorID,
      type: "newSource",
      source: aSource.form()
    });
  },

  







  _allowSource: function TA__allowSource(aSourceUrl) {
    
    if (!aSourceUrl)
      return false;
    
    if (aSourceUrl.indexOf("chrome://") == 0) {
      return false;
    }
    
    if (aSourceUrl.indexOf("about:") == 0) {
      return false;
    }
    return true;
  },

  


  _restoreBreakpoints: function TA__restoreBreakpoints() {
    for (let s of this.dbg.findScripts()) {
      this._addScript(s);
    }
  },

  






  _addScript: function TA__addScript(aScript) {
    if (!this._allowSource(aScript.url)) {
      return false;
    }

    

    let endLine = aScript.startLine + aScript.lineCount - 1;
    for (let bp of this.breakpointStore.findBreakpoints({ url: aScript.url })) {
      
      
      
      if (!bp.actor.scripts.length
          && bp.line >= aScript.startLine
          && bp.line <= endLine) {
        this._setBreakpoint(bp);
      }
    }

    return true;
  },


  


  onPrototypesAndProperties: function TA_onPrototypesAndProperties(aRequest) {
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
  "setBreakpoint": ThreadActor.prototype.onSetBreakpoint,
  "sources": ThreadActor.prototype.onSources,
  "threadGrips": ThreadActor.prototype.onThreadGrips,
  "prototypesAndProperties": ThreadActor.prototype.onPrototypesAndProperties
};











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








PauseScopedActor.withPaused = function PSA_withPaused(aMethod) {
  return function () {
    if (this.isPaused()) {
      return aMethod.apply(this, arguments);
    } else {
      return this._wrongState();
    }
  };
};

PauseScopedActor.prototype = {

  


  isPaused: function PSA_isPaused() {
    
    
    
    return this.threadActor ? this.threadActor.state === "paused" : true;
  },

  


  _wrongState: function PSA_wrongState() {
    return {
      error: "wrongState",
      message: this.constructor.name +
        " actors can only be accessed while the thread is paused."
    };
  }
};



















function SourceActor({ url, thread, sourceMap, generatedSource, text,
                       contentType }) {
  this._threadActor = thread;
  this._url = url;
  this._sourceMap = sourceMap;
  this._generatedSource = generatedSource;
  this._text = text;
  this._contentType = contentType;

  this.onSource = this.onSource.bind(this);
  this._invertSourceMap = this._invertSourceMap.bind(this);
  this._saveMap = this._saveMap.bind(this);
}

SourceActor.prototype = {
  constructor: SourceActor,
  actorPrefix: "source",

  get threadActor() this._threadActor,
  get url() this._url,

  get prettyPrintWorker() {
    return this.threadActor.prettyPrintWorker;
  },

  form: function SA_form() {
    return {
      actor: this.actorID,
      url: this._url,
      isBlackBoxed: this.threadActor.sources.isBlackBoxed(this.url)
      
    };
  },

  disconnect: function SA_disconnect() {
    if (this.registeredPool && this.registeredPool.sourceActors) {
      delete this.registeredPool.sourceActors[this.actorID];
    }
  },

  _getSourceText: function SA__getSourceText() {
    const toResolvedContent = t => resolve({
      content: t,
      contentType: this._contentType
    });

    let sc;
    if (this._sourceMap && (sc = this._sourceMap.sourceContentFor(this._url))) {
      return toResolvedContent(sc);
    }

    if (this._text) {
      return toResolvedContent(this._text);
    }

    
    
    
    return fetch(this._url, { loadFromCache: !this._sourceMap });
  },

  


  onSource: function SA_onSource(aRequest) {
    return this._getSourceText()
      .then(({ content, contentType }) => {
        return {
          from: this.actorID,
          source: this.threadActor.createValueGrip(
            content, this.threadActor.threadLifetimePool),
          contentType: contentType
        };
      })
      .then(null, (aError) => {
        reportError(aError, "Got an exception during SA_onSource: ");
        return {
          "from": this.actorID,
          "error": "loadSourceError",
          "message": "Could not load the source for " + this._url + ".\n"
            + safeErrorString(aError)
        };
      });
  },

  


  onPrettyPrint: function ({ indent }) {
    return this._getSourceText()
      .then(this._parseAST)
      .then(this._sendToPrettyPrintWorker(indent))
      .then(this._invertSourceMap)
      .then(this._saveMap)
      .then(this.onSource)
      .then(null, error => ({
        from: this.actorID,
        error: "prettyPrintError",
        message: DevToolsUtils.safeErrorString(error)
      }));
  },

  


  _parseAST: function SA__parseAST({ content}) {
    return Reflect.parse(content);
  },

  











  _sendToPrettyPrintWorker: function SA__sendToPrettyPrintWorker(aIndent) {
    return aAST => {
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
        url: this._url,
        indent: aIndent,
        ast: aAST
      });

      return deferred.promise;
    };
  },

  







  _invertSourceMap: function SA__invertSourceMap({ code, mappings }) {
    const generator = new SourceMapGenerator({ file: this._url });
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
      generator.setSourceContent(this._url, code);
      const consumer = SourceMapConsumer.fromSourceMap(generator);

      
      
      

      const getOrigPos = consumer.originalPositionFor.bind(consumer);
      const getGenPos = consumer.generatedPositionFor.bind(consumer);

      consumer.originalPositionFor = ({ line, column }) => {
        const location = getGenPos({
          line: line,
          column: column,
          source: this._url
        });
        location.source = this._url;
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

  





  _saveMap: function SA__saveMap({ map }) {
    if (this._sourceMap) {
      
      this._sourceMap = SourceMapGenerator.fromSourceMap(this._sourceMap);
      this._sourceMap.applySourceMap(map, this._url);
      this._sourceMap = SourceMapConsumer.fromSourceMap(this._sourceMap);
      this._threadActor.sources.saveSourceMap(this._sourceMap,
                                              this._generatedSource);
    } else {
      this._sourceMap = map;
      this._threadActor.sources.saveSourceMap(this._sourceMap, this._url);
    }
  },

  


  onBlackBox: function SA_onBlackBox(aRequest) {
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

  


  onUnblackBox: function SA_onUnblackBox(aRequest) {
    this.threadActor.sources.unblackBox(this.url);
    return {
      from: this.actorID
    };
  }
};

SourceActor.prototype.requestTypes = {
  "source": SourceActor.prototype.onSource,
  "blackbox": SourceActor.prototype.onBlackBox,
  "unblackbox": SourceActor.prototype.onUnblackBox,
  "prettyPrint": SourceActor.prototype.onPrettyPrint
};










function ObjectActor(aObj, aThreadActor)
{
  this.obj = aObj;
  this.threadActor = aThreadActor;
}

ObjectActor.prototype = {
  actorPrefix: "obj",

  


  grip: function OA_grip() {
    let g = {
      "type": "object",
      "class": this.obj.class,
      "actor": this.actorID,
      "extensible": this.obj.isExtensible(),
      "frozen": this.obj.isFrozen(),
      "sealed": this.obj.isSealed()
    };

    
    if (this.obj.class === "Function") {
      if (this.obj.name) {
        g.name = this.obj.name;
      }
      if (this.obj.displayName) {
        g.displayName = this.obj.displayName;
      }

      
      
      let desc = this.obj.getOwnPropertyDescriptor("displayName");
      if (desc && desc.value && typeof desc.value == "string") {
        g.userDisplayName = this.threadActor.createValueGrip(desc.value);
      }

      
      if (this.obj.script) {
        g.url = this.obj.script.url;
        g.line = this.obj.script.startLine;
      }
    }

    return g;
  },

  


  release: function OA_release() {
    if (this.registeredPool.objectActors) {
      this.registeredPool.objectActors.delete(this.obj);
    }
    this.registeredPool.removeActor(this);
  },

  






  onOwnPropertyNames: function OA_onOwnPropertyNames(aRequest) {
    return { from: this.actorID,
             ownPropertyNames: this.obj.getOwnPropertyNames() };
  },

  






  onPrototypeAndProperties: function OA_onPrototypeAndProperties(aRequest) {
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

  










  _findSafeGetterValues: function OA__findSafeGetterValues(aOwnProperties)
  {
    let safeGetterValues = Object.create(null);
    let obj = this.obj;
    let level = 0;

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
          }
        }
      }

      obj = obj.proto;
      level++;
    }

    return safeGetterValues;
  },

  










  _findSafeGetters: function OA__findSafeGetters(aObject)
  {
    if (aObject._safeGetters) {
      return aObject._safeGetters;
    }

    let getters = new Set();
    for (let name of aObject.getOwnPropertyNames()) {
      let desc = null;
      try {
        desc = aObject.getOwnPropertyDescriptor(name);
      } catch (e) {
        
        
      }
      if (!desc || desc.value !== undefined || !("get" in desc)) {
        continue;
      }

      let fn = desc.get;
      if (fn && fn.callable && fn.class == "Function" &&
          fn.script === undefined) {
        getters.add(name);
      }
    }

    aObject._safeGetters = getters;
    return getters;
  },

  





  onPrototype: function OA_onPrototype(aRequest) {
    return { from: this.actorID,
             prototype: this.threadActor.createValueGrip(this.obj.proto) };
  },

  






  onProperty: function OA_onProperty(aRequest) {
    if (!aRequest.name) {
      return { error: "missingParameter",
               message: "no property name was specified" };
    }

    return { from: this.actorID,
             descriptor: this._propertyDescriptor(aRequest.name) };
  },

  





  onDisplayString: function OA_onDisplayString(aRequest) {
    let toString;
    try {
      
      let obj = this.obj;
      do {
        let desc = obj.getOwnPropertyDescriptor("toString");
        if (desc) {
          toString = desc.value;
          break;
        }
        obj = obj.proto;
      } while ((obj));
    } catch (e) {
      dumpn(e);
    }

    let result = null;
    if (toString && toString.callable) {
      
      let ret = toString.call(this.obj).return;
      if (typeof ret == "string") {
        
        result = ret;
      }
    }

    return { from: this.actorID,
             displayString: this.threadActor.createValueGrip(result) };
  },

  






  _propertyDescriptor: function OA_propertyDescriptor(aName) {
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

    if (!desc) {
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

  





  onDecompile: function OA_onDecompile(aRequest) {
    if (this.obj.class !== "Function") {
      return { error: "objectNotFunction",
               message: "decompile request is only valid for object grips " +
                        "with a 'Function' class." };
    }

    return { from: this.actorID,
             decompiledCode: this.obj.decompile(!!aRequest.pretty) };
  },

  





  onParameterNames: function OA_onParameterNames(aRequest) {
    if (this.obj.class !== "Function") {
      return { error: "objectNotFunction",
               message: "'parameterNames' request is only valid for object " +
                        "grips with a 'Function' class." };
    }

    return { parameterNames: this.obj.parameterNames };
  },

  





  onRelease: function OA_onRelease(aRequest) {
    this.release();
    return {};
  },
};

ObjectActor.prototype.requestTypes = {
  "parameterNames": ObjectActor.prototype.onParameterNames,
  "prototypeAndProperties": ObjectActor.prototype.onPrototypeAndProperties,
  "prototype": ObjectActor.prototype.onPrototype,
  "property": ObjectActor.prototype.onProperty,
  "displayString": ObjectActor.prototype.onDisplayString,
  "ownPropertyNames": ObjectActor.prototype.onOwnPropertyNames,
  "decompile": ObjectActor.prototype.onDecompile,
  "release": ObjectActor.prototype.onRelease,
};






function PauseScopedObjectActor()
{
  ObjectActor.apply(this, arguments);
}

PauseScopedObjectActor.prototype = Object.create(PauseScopedActor.prototype);

update(PauseScopedObjectActor.prototype, ObjectActor.prototype);

update(PauseScopedObjectActor.prototype, {
  constructor: PauseScopedObjectActor,

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

  





  onScope: PauseScopedActor.withPaused(function OA_onScope(aRequest) {
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
  }),

  






  onThreadGrip: PauseScopedActor.withPaused(function OA_onThreadGrip(aRequest) {
    this.threadActor.threadObjectGrip(this);
    return {};
  }),

  





  onRelease: PauseScopedActor.withPaused(function OA_onRelease(aRequest) {
    if (this.registeredPool !== this.threadActor.threadLifetimePool) {
      return { error: "notReleasable",
               message: "Only thread-lifetime actors can be released." };
    }

    this.release();
    return {};
  }),
});

update(PauseScopedObjectActor.prototype.requestTypes, {
  "scope": PauseScopedObjectActor.prototype.onScope,
  "threadGrip": PauseScopedObjectActor.prototype.onThreadGrip,
});









function LongStringActor(aString)
{
  this.string = aString;
  this.stringLength = aString.length;
}

LongStringActor.prototype = {

  actorPrefix: "longString",

  disconnect: function LSA_disconnect() {
    
    
    
    if (this.registeredPool && this.registeredPool.longStringActors) {
      delete this.registeredPool.longStringActors[this.actorID];
    }
  },

  


  grip: function LSA_grip() {
    return {
      "type": "longString",
      "initial": this.string.substring(
        0, DebuggerServer.LONG_STRING_INITIAL_LENGTH),
      "length": this.stringLength,
      "actor": this.actorID
    };
  },

  





  onSubstring: function LSA_onSubString(aRequest) {
    return {
      "from": this.actorID,
      "substring": this.string.substring(aRequest.start, aRequest.end)
    };
  },

  


  onRelease: function LSA_onRelease() {
    
    
    
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

  



  disconnect: function FA_disconnect() {
    this.conn.removeActorPool(this._frameLifetimePool);
    this._frameLifetimePool = null;
  },

  


  form: function FA_form() {
    let form = { actor: this.actorID,
                 type: this.frame.type };
    if (this.frame.type === "call") {
      form.callee = this.threadActor.createValueGrip(this.frame.callee);
    }

    if (this.frame.environment) {
      let envActor = this.threadActor
        .createEnvironmentActor(this.frame.environment,
                                this.frameLifetimePool);
      form.environment = envActor.form();
    }
    form.this = this.threadActor.createValueGrip(this.frame.this);
    form.arguments = this._args();
    if (this.frame.script) {
      form.where = getFrameLocation(this.frame);
    }

    if (!this.frame.older) {
      form.oldest = true;
    }

    return form;
  },

  _args: function FA__args() {
    if (!this.frame.arguments) {
      return [];
    }

    return [this.threadActor.createValueGrip(arg)
            for each (arg in this.frame.arguments)];
  },

  





  onPop: function FA_onPop(aRequest) {
    
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












function BreakpointActor(aThreadActor, aLocation)
{
  this.scripts = [];
  this.threadActor = aThreadActor;
  this.location = aLocation;
}

BreakpointActor.prototype = {
  actorPrefix: "breakpoint",

  








  addScript: function BA_addScript(aScript, aThreadActor) {
    this.threadActor = aThreadActor;
    this.scripts.push(aScript);
  },

  


  removeScripts: function () {
    for (let script of this.scripts) {
      script.clearBreakpoint(this);
    }
    this.scripts = [];
  },

  





  hit: function BA_hit(aFrame) {
    
    
    let { url } = this.threadActor.synchronize(
      this.threadActor.sources.getOriginalLocation({
        url: this.location.url,
        line: this.location.line,
        column: this.location.column
      }));

    if (this.threadActor.sources.isBlackBoxed(url) || aFrame.onStep) {
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

  





  onDelete: function BA_onDelete(aRequest) {
    
    this.threadActor.breakpointStore.removeBreakpoint(this.location);
    this.threadActor._hooks.removeFromParentPool(this);
    
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

  


  form: function EA_form() {
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

  



  _bindings: function EA_bindings() {
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
      
      
      let desc = {
        value: this.obj.getVariable(name),
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

      
      
      let desc = {
        configurable: false,
        writable: true,
        enumerable: true
      };
      try {
        desc.value = this.obj.getVariable(name);
      } catch (e) {
        
        
        if (name != "arguments") {
          throw e;
        }
      }
      
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
      bindings.variables[name] = descForm;
    }

    return bindings;
  },

  






  onAssign: function EA_onAssign(aRequest) {
    
    
    







    try {
      this.obj.setVariable(aRequest.name, aRequest.value);
    } catch (e) {
      if (e instanceof Debugger.DebuggeeWouldRun) {
        return { error: "threadWouldRun",
                 cause: e.cause ? e.cause : "setter",
                 message: "Assigning a value would cause the debuggee to run" };
      }
      
      throw e;
    }
    return { from: this.actorID };
  },

  






  onBindings: function EA_onBindings(aRequest) {
    return { from: this.actorID,
             bindings: this._bindings() };
  }
};

EnvironmentActor.prototype.requestTypes = {
  "assign": EnvironmentActor.prototype.onAssign,
  "bindings": EnvironmentActor.prototype.onBindings
};





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

















function ChromeDebuggerActor(aConnection, aHooks)
{
  ThreadActor.call(this, aHooks);
}

ChromeDebuggerActor.prototype = Object.create(ThreadActor.prototype);

update(ChromeDebuggerActor.prototype, {
  constructor: ChromeDebuggerActor,

  
  actorPrefix: "chromeDebugger",

  



  _allowSource: function(aSourceURL) !!aSourceURL,

   





  globalManager: {
    findGlobals: function CDA_findGlobals() {
      
      this.dbg.addAllGlobalsAsDebuggees();
    },

    






    onNewGlobal: function CDA_onNewGlobal(aGlobal) {
      this.addDebuggee(aGlobal);
      
      this.conn.send({
        from: this.actorID,
        type: "newGlobal",
        
        hostAnnotations: aGlobal.hostAnnotations
      });
    }
  }
});






function ThreadSources(aThreadActor, aUseSourceMaps, aAllowPredicate,
                       aOnNewSource) {
  this._thread = aThreadActor;
  this._useSourceMaps = aUseSourceMaps;
  this._allow = aAllowPredicate;
  this._onNewSource = aOnNewSource;

  
  this._sourceMapsByGeneratedSource = Object.create(null);
  
  this._sourceMapsByOriginalSource = Object.create(null);
  
  this._sourceActors = Object.create(null);
  
  this._generatedUrlsByOriginalUrl = Object.create(null);
}





ThreadSources._blackBoxedSources = new Set();

ThreadSources.prototype = {
  




















  source: function TS_source({ url, sourceMap, generatedSource, text,
                               contentType }) {
    if (!this._allow(url)) {
      return null;
    }

    if (url in this._sourceActors) {
      return this._sourceActors[url];
    }

    let actor = new SourceActor({
      url: url,
      thread: this._thread,
      sourceMap: sourceMap,
      generatedSource: generatedSource,
      text: text,
      contentType: contentType
    });
    this._thread.threadLifetimePool.addActor(actor);
    this._sourceActors[url] = actor;
    try {
      this._onNewSource(actor);
    } catch (e) {
      reportError(e);
    }
    return actor;
  },

  


  _sourceForScript: function TS__sourceForScript(aScript) {
    const spec = {
      url: aScript.url
    };

    
    
    
    
    
    if (aScript.url) {
      try {
        const url = Services.io.newURI(aScript.url, null, null)
          .QueryInterface(Ci.nsIURL);
        if (url.fileExtension === "js") {
          spec.contentType = "text/javascript";
          spec.text = aScript.source.text;
        }
      } catch(ex) {
        
      }
    }

    return this.source(spec);
  },

  







  sourcesForScript: function TS_sourcesForScript(aScript) {
    if (!this._useSourceMaps || !aScript.sourceMapURL) {
      return resolve([this._sourceForScript(aScript)].filter(isNotNull));
    }

    return this.sourceMap(aScript)
      .then((aSourceMap) => {
        return [
          this.source({ url: s,
                        sourceMap: aSourceMap,
                        generatedSource: aScript.url })
          for (s of aSourceMap.sources)
        ];
      })
      .then(null, (e) => {
        reportError(e);
        delete this._sourceMapsByGeneratedSource[aScript.url];
        return [this._sourceForScript(aScript)];
      })
      .then(ss => ss.filter(isNotNull));
  },

  




  sourceMap: function TS_sourceMap(aScript) {
    dbg_assert(aScript.sourceMapURL, "Script should have a sourceMapURL");
    let sourceMapURL = this._normalize(aScript.sourceMapURL, aScript.url);
    let map = this._fetchSourceMap(sourceMapURL, aScript.url)
      .then(aSourceMap => this.saveSourceMap(aSourceMap, aScript.url));
    this._sourceMapsByGeneratedSource[aScript.url] = map;
    return map;
  },

  



  saveSourceMap: function TS_saveSourceMap(aSourceMap, aGeneratedSource) {
    this._sourceMapsByGeneratedSource[aGeneratedSource] = resolve(aSourceMap);
    for (let s of aSourceMap.sources) {
      this._generatedUrlsByOriginalUrl[s] = aGeneratedSource;
      this._sourceMapsByOriginalSource[s] = resolve(aSourceMap);
    }
    return aSourceMap;
  },

  











  _fetchSourceMap: function TS__fetchSourceMap(aAbsSourceMapURL, aScriptURL) {
    return fetch(aAbsSourceMapURL, { loadFromCache: false })
      .then(({ content }) => {
        let map = new SourceMapConsumer(content);
        this._setSourceMapRoot(map, aAbsSourceMapURL, aScriptURL);
        return map;
      });
  },

  


  _setSourceMapRoot: function TS__setSourceMapRoot(aSourceMap, aAbsSourceMapURL,
                                                   aScriptURL) {
    const base = this._dirname(
      aAbsSourceMapURL.indexOf("data:") === 0
        ? aScriptURL
        : aAbsSourceMapURL);
    aSourceMap.sourceRoot = aSourceMap.sourceRoot
      ? this._normalize(aSourceMap.sourceRoot, base)
      : base;
  },

  _dirname: function TS__dirname(aPath) {
    return Services.io.newURI(
      ".", null, Services.io.newURI(aPath, null, null)).spec;
  },

  



  getOriginalLocation: function TS_getOriginalLocation({ url, line, column }) {
    if (url in this._sourceMapsByGeneratedSource) {
      return this._sourceMapsByGeneratedSource[url]
        .then((aSourceMap) => {
          let { source: aSourceURL, line: aLine, column: aColumn } = aSourceMap.originalPositionFor({
            line: line,
            column: column
          });
          return {
            url: aSourceURL,
            line: aLine,
            column: aColumn
          };
        });
    }

    
    return resolve({
      url: url,
      line: line,
      column: column
    });
  },

  








  getGeneratedLocation: function TS_getGeneratedLocation({ url, line, column }) {
    if (url in this._sourceMapsByOriginalSource) {
      return this._sourceMapsByOriginalSource[url]
        .then((aSourceMap) => {
          let { line: aLine, column: aColumn } = aSourceMap.generatedPositionFor({
            source: url,
            line: line,
            column: column == null ? Infinity : column
          });
          return {
            url: this._generatedUrlsByOriginalUrl[url],
            line: aLine,
            column: aColumn
          };
        });
    }

    
    return resolve({
      url: url,
      line: line,
      column: column
    });
  },

  






  isBlackBoxed: function TS_isBlackBoxed(aURL) {
    return ThreadSources._blackBoxedSources.has(aURL);
  },

  







  blackBox: function TS_blackBox(aURL) {
    ThreadSources._blackBoxedSources.add(aURL);
  },

  





  unblackBox: function TS_unblackBox(aURL) {
    ThreadSources._blackBoxedSources.delete(aURL);
  },

  


  _normalize: function TS__normalize(...aURLs) {
    dbg_assert(aURLs.length > 1, "Should have more than 1 URL");
    let base = Services.io.newURI(aURLs.pop(), null, null);
    let url;
    while ((url = aURLs.pop())) {
      base = Services.io.newURI(url, null, base);
    }
    return base.spec;
  },

  iter: function TS_iter() {
    for (let url in this._sourceActors) {
      yield this._sourceActors[url];
    }
  }
};





function getOffsetColumn(aOffset, aScript) {
  let bestOffsetMapping = null;
  for (let offsetMapping of aScript.getAllColumnOffsets()) {
    if (!bestOffsetMapping ||
        (offsetMapping.offset <= aOffset &&
         offsetMapping.offset > bestOffsetMapping.offset)) {
      bestOffsetMapping = offsetMapping;
    }
  }

  if (!bestOffsetMapping) {
    
    
    
    
    reportError(new Error("Could not find a column for offset " + aOffset
                          + " in the script " + aScript));
    return 0;
  }

  return bestOffsetMapping.columnNumber;
}










function getFrameLocation(aFrame) {
  if (!aFrame.script) {
    return { url: null, line: null, column: null };
  }
  return {
    url: aFrame.script.url,
    line: aFrame.script.getOffsetLine(aFrame.offset),
    column: getOffsetColumn(aFrame.offset, aFrame.script)
  }
}










function update(aTarget, aNewAttrs) {
  for (let key in aNewAttrs) {
    let desc = Object.getOwnPropertyDescriptor(aNewAttrs, key);

    if (desc) {
      Object.defineProperty(aTarget, key, desc);
    }
  }
}




function isNotNull(aThing) {
  return aThing !== null;
}













function fetch(aURL, aOptions={ loadFromCache: true }) {
  let deferred = defer();
  let scheme;
  let url = aURL.split(" -> ").pop();
  let charset;
  let contentType;

  try {
    scheme = Services.io.extractScheme(url);
  } catch (e) {
    
    
    
    url = "file://" + url;
    scheme = Services.io.extractScheme(url);
  }

  switch (scheme) {
    case "file":
    case "chrome":
    case "resource":
      try {
        NetUtil.asyncFetch(url, function onFetch(aStream, aStatus, aRequest) {
          if (!Components.isSuccessCode(aStatus)) {
            deferred.reject(new Error("Request failed with status code = "
                                      + aStatus
                                      + " after NetUtil.asyncFetch for url = "
                                      + url));
            return;
          }

          let source = NetUtil.readInputStreamToString(aStream, aStream.available());
          contentType = aRequest.contentType;
          deferred.resolve(source);
          aStream.close();
        });
      } catch (ex) {
        deferred.reject(ex);
      }
      break;

    default:
      let channel;
      try {
        channel = Services.io.newChannel(url, null, null);
      } catch (e if e.name == "NS_ERROR_UNKNOWN_PROTOCOL") {
        
        
        url = "file:///" + url;
        channel = Services.io.newChannel(url, null, null);
      }
      let chunks = [];
      let streamListener = {
        onStartRequest: function(aRequest, aContext, aStatusCode) {
          if (!Components.isSuccessCode(aStatusCode)) {
            deferred.reject(new Error("Request failed with status code = "
                                      + aStatusCode
                                      + " in onStartRequest handler for url = "
                                      + url));
          }
        },
        onDataAvailable: function(aRequest, aContext, aStream, aOffset, aCount) {
          chunks.push(NetUtil.readInputStreamToString(aStream, aCount));
        },
        onStopRequest: function(aRequest, aContext, aStatusCode) {
          if (!Components.isSuccessCode(aStatusCode)) {
            deferred.reject(new Error("Request failed with status code = "
                                      + aStatusCode
                                      + " in onStopRequest handler for url = "
                                      + url));
            return;
          }

          charset = channel.contentCharset;
          contentType = channel.contentType;
          deferred.resolve(chunks.join(""));
        }
      };

      channel.loadFlags = aOptions.loadFromCache
        ? channel.LOAD_FROM_CACHE
        : channel.LOAD_BYPASS_CACHE;
      channel.asyncOpen(streamListener, null);
      break;
  }

  return deferred.promise.then(source => {
    return {
      content: convertToUnicode(source, charset),
      contentType: contentType
    };
  });
}









function convertToUnicode(aString, aCharset=null) {
  
  let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"]
    .createInstance(Ci.nsIScriptableUnicodeConverter);
  try {
    converter.charset = aCharset || "UTF-8";
    return converter.ConvertToUnicode(aString);
  } catch(e) {
    return aString;
  }
}









function reportError(aError, aPrefix="") {
  dbg_assert(aError instanceof Error, "Must pass Error objects to reportError");
  let msg = aPrefix + aError.message + ":\n" + aError.stack;
  Cu.reportError(msg);
  dumpn(msg);
}









function findCssSelector(ele) {
  var document = ele.ownerDocument;
  if (ele.id && document.getElementById(ele.id) === ele) {
    return '#' + ele.id;
  }

  
  var tagName = ele.tagName.toLowerCase();
  if (tagName === 'html') {
    return 'html';
  }
  if (tagName === 'head') {
    return 'head';
  }
  if (tagName === 'body') {
    return 'body';
  }

  if (ele.parentNode == null) {
    console.log('danger: ' + tagName);
  }

  
  var selector, index, matches;
  if (ele.classList.length > 0) {
    for (var i = 0; i < ele.classList.length; i++) {
      
      selector = '.' + ele.classList.item(i);
      matches = document.querySelectorAll(selector);
      if (matches.length === 1) {
        return selector;
      }
      
      selector = tagName + selector;
      matches = document.querySelectorAll(selector);
      if (matches.length === 1) {
        return selector;
      }
      
      index = positionInNodeList(ele, ele.parentNode.children) + 1;
      selector = selector + ':nth-child(' + index + ')';
      matches = document.querySelectorAll(selector);
      if (matches.length === 1) {
        return selector;
      }
    }
  }

  
  index = positionInNodeList(ele, ele.parentNode.children) + 1;
  selector = findCssSelector(ele.parentNode) + ' > ' +
          tagName + ':nth-child(' + index + ')';

  return selector;
};





function positionInNodeList(element, nodeList) {
  for (var i = 0; i < nodeList.length; i++) {
    if (element === nodeList[i]) {
      return i;
    }
  }
  return -1;
}
