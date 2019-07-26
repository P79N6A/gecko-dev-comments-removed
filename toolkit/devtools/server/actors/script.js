





"use strict";



















function ThreadActor(aHooks, aGlobal)
{
  this._state = "detached";
  this._frameActors = [];
  this._environmentActors = [];
  this._hooks = aHooks;
  this.global = aGlobal;

  this.findGlobals = this.globalManager.findGlobals.bind(this);
  this.onNewGlobal = this.globalManager.onNewGlobal.bind(this);
  this.onNewSource = this.onNewSource.bind(this);

  this._options = {
    useSourceMaps: false
  };
}





ThreadActor._breakpointStore = {};

ThreadActor.prototype = {
  actorPrefix: "context",

  get state() { return this._state; },
  get attached() this.state == "attached" ||
                 this.state == "running" ||
                 this.state == "paused",

  get _breakpointStore() { return ThreadActor._breakpointStore; },

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

  clearDebuggees: function TA_clearDebuggees() {
    if (this.dbg) {
      this.dbg.removeAllDebuggees();
    }
    this.conn.removeActorPool(this._threadLifetimePool || undefined);
    this._threadLifetimePool = null;
    this._sources = null;
  },

  


  addDebuggee: function TA_addDebuggee(aGlobal) {
    try {
      this.dbg.addDebuggee(aGlobal);
    } catch (e) {
      
      dumpn("Ignoring request to add the debugger's compartment as a debuggee");
    }
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
    this.addDebuggee(aWindow);
    let frames = aWindow.frames;
    if (frames) {
      for (let i = 0; i < frames.length; i++) {
        this._addDebuggees(frames[i]);
      }
    }
  },

  



  globalManager: {
    findGlobals: function TA_findGlobals() {
      this._addDebuggees(this.global);
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

      
      
      
      this.conn.send(packet);

      
      this._nest();

      
      
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
                                                 onPacket=function (k) k) {
    try {
      let packet = this._paused(aFrame);
      if (!packet) {
        return undefined;
      }
      packet.why = aReason;
      resolve(onPacket(packet)).then(this.conn.send.bind(this.conn));
      return this._nest();
    } catch(e) {
      let msg = "Got an exception during TA__pauseAndRespond: " + e +
                ": " + e.stack;
      Cu.reportError(msg);
      dumpn(msg);
      return undefined;
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

    
    
    
    if (DebuggerServer.xpcInspector.eventLoopNestLevel > 1) {
      let lastNestRequestor = DebuggerServer.xpcInspector.lastNestRequestor;
      if (lastNestRequestor.connection != this.conn) {
        return { error: "wrongOrder",
                 message: "trying to resume in the wrong order.",
                 lastPausedUrl: lastNestRequestor.url };
      }
    }

    if (aRequest && aRequest.forceCompletion) {
      
      
      if (typeof this.frame.pop != "function") {
        return { error: "notImplemented",
                 message: "forced completion is not yet implemented." };
      }

      this.dbg.getNewestFrame().pop(aRequest.completionValue);
      let packet = this._resumed();
      DebuggerServer.xpcInspector.exitNestedEventLoop();
      return { type: "resumeLimit", frameFinished: aRequest.forceCompletion };
    }

    if (aRequest && aRequest.resumeLimit) {
      
      
      let pauseAndRespond = aFrame => {
        this._pauseAndRespond(aFrame, { type: "resumeLimit" });
      };
      let createValueGrip = this.createValueGrip.bind(this);

      let startFrame = this.youngestFrame;
      let startLine;
      if (this.youngestFrame.script) {
        let offset = this.youngestFrame.offset;
        startLine = this.youngestFrame.script.getOffsetLine(offset);
      }

      

      let onEnterFrame = aFrame => {
        if (this.sources.isBlackBoxed(aFrame.script.url)) {
          return undefined;
        }
        return pauseAndRespond(aFrame);
      };

      let thread = this;

      let onPop = function TA_onPop(aCompletion) {
        

        if (thread.sources.isBlackBoxed(this.script.url)) {
          return undefined;
        }

        
        
        this.reportedPop = true;

        return pauseAndRespond(this);
      };

      let onStep = function TA_onStep() {
        

        if (thread.sources.isBlackBoxed(this.script.url)) {
          return undefined;
        }

        
        if (this !== startFrame ||
            (this.script &&
             this.script.getOffsetLine(this.offset) != startLine)) {
          return pauseAndRespond(this);
        }

        
        return undefined;
      };

      let steppingType = aRequest.resumeLimit.type;
      if (["step", "next", "finish"].indexOf(steppingType) == -1) {
            return { error: "badParameterType",
                     message: "Unknown resumeLimit type" };
      }
      
      
      let stepFrame = this._getNextStepFrame(startFrame);
      if (stepFrame) {
        switch (steppingType) {
          case "step":
            this.dbg.onEnterFrame = onEnterFrame;
            
          case "next":
            stepFrame.onStep = onStep;
            stepFrame.onPop = onPop;
            break;
          case "finish":
            stepFrame.onPop = onPop;
        }
      }
    }

    if (aRequest && aRequest.pauseOnExceptions) {
      this.dbg.onExceptionUnwind = this.onExceptionUnwind.bind(this);
    }
    let packet = this._resumed();
    DebuggerServer.xpcInspector.exitNestedEventLoop();
    return packet;
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
    };

    let frame = this._requestFrame(aRequest.frame);
    if (!frame) {
      return { error: "unknownFrame",
               message: "Evaluation frame not found" };
    }

    if (!frame.environment) {
      return { error: "notDebuggee",
               message: "cannot access the environment of this frame." };
    };

    
    
    
    
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

      let promise = this.sources.getOriginalLocation(form.where.url,
                                                     form.where.line)
        .then(function (aOrigLocation) {
          form.where = aOrigLocation;
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

    let locationPromise = this.sources.getGeneratedLocation(originalSource,
                                                            originalLine)
    return locationPromise.then((aLocation) => {
      let line = aLocation.line;
      if (this.dbg.findScripts({ url: aLocation.url }).length == 0 ||
          line < 0 ||
          line == null) {
        return { error: "noScript" };
      }

      
      
      if (!this._breakpointStore[aLocation.url]) {
        this._breakpointStore[aLocation.url] = [];
      }
      let scriptBreakpoints = this._breakpointStore[aLocation.url];
      scriptBreakpoints[line] = {
        url: aLocation.url,
        line: line,
        column: aLocation.column
      };

      let response = this._setBreakpoint(aLocation);
      
      
      
      let originalLocation = this.sources.getOriginalLocation(aLocation.url,
                                                              aLocation.line);

      return all([response, originalLocation])
        .then(([aResponse, {url, line}]) => {
          if (aResponse.actualLocation) {
            let actualOrigLocation = this.sources.getOriginalLocation(
              aResponse.actualLocation.url, aResponse.actualLocation.line);
            return actualOrigLocation.then(function ({ url, line }) {
              if (url !== originalSource || line !== originalLine) {
                aResponse.actualLocation = { url: url, line: line };
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

  









  _setBreakpoint: function TA__setBreakpoint(aLocation) {
    let breakpoints = this._breakpointStore[aLocation.url];

    
    let actor;
    if (breakpoints[aLocation.line].actor) {
      actor = breakpoints[aLocation.line].actor;
    } else {
      actor = breakpoints[aLocation.line].actor = new BreakpointActor(this, {
        url: aLocation.url,
        line: aLocation.line
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

   



    let found = false;
    for (let script of scripts) {
      let offsets = script.getLineOffsets(aLocation.line);
      if (offsets.length > 0) {
        for (let offset of offsets) {
          script.setBreakpoint(offset, actor);
        }
        actor.addScript(script, this);
        found = true;
      }
    }
    if (found) {
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
      if (breakpoints[actualLocation.line] &&
          breakpoints[actualLocation.line].actor) {
        




        actor.onDelete();
        delete breakpoints[aLocation.line];
        return {
          actor: breakpoints[actualLocation.line].actor.actorID,
          actualLocation: actualLocation
        };
      } else {
        




        actor.location = actualLocation;
        breakpoints[actualLocation.line] = breakpoints[aLocation.line];
        delete breakpoints[aLocation.line];
        
        breakpoints[actualLocation.line].line = actualLocation.line;
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

  


  _discoverScriptsAndSources: function TA__discoverScriptsAndSources() {
    let promises = [];
    let foundSourceMaps = false;
    let scripts = this.dbg.findScripts();
    for (let s of scripts) {
      if (s.sourceMapURL && !foundSourceMaps) {
        foundSourceMaps = true;
        break;
      }
    }
    if (this._options.useSourceMaps && foundSourceMaps) {
      for (let s of scripts) {
        promises.push(this._addScript(s));
      }
      return all(promises);
    }
    
    
    
    
    
    for (let s of scripts) {
      this._addScriptSync(s);
    }
    return resolve(null);
  },

  onSources: function TA_onSources(aRequest) {
    return this._discoverScriptsAndSources().then(() => {
      return {
        sources: [s.form() for (s of this.sources.iter())]
      };
    });
  },

  






  disableAllBreakpoints: function () {
    for (let url in this._breakpointStore) {
      for (let line in this._breakpointStore[url]) {
        let bp = this._breakpointStore[url][line];
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

      
      this._nest();

      
      
      return null;
    } catch (e) {
      reportError(e);
      return { error: "notInterrupted", message: e.toString() };
    }
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

  _paused: function TA_paused(aFrame) {
    
    
    
    
    
    

    if (this.state === "paused") {
      return undefined;
    }

    
    this.dbg.onEnterFrame = undefined;
    this.dbg.onExceptionUnwind = undefined;
    if (aFrame) {
      aFrame.onStep = undefined;
      aFrame.onPop = undefined;
    }

    this._state = "paused";

    
    
    this.youngestFrame = aFrame;

    
    
    dbg_assert(!this._pausePool);
    this._pausePool = new ActorPool(this.conn);
    this.conn.addActorPool(this._pausePool);

    
    
    this._pausePool.threadActor = this;

    
    dbg_assert(!this._pauseActor);
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

  _nest: function TA_nest() {
    if (this._hooks.preNest) {
      var nestData = this._hooks.preNest();
    }

    let requestor = Object.create(null);
    requestor.url = this._hooks.url;
    requestor.connection = this.conn;
    DebuggerServer.xpcInspector.enterNestedEventLoop(requestor);

    dbg_assert(this.state === "running");

    if (this._hooks.postNest) {
      this._hooks.postNest(nestData)
    }

    
    return undefined;
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
    let type = typeof(aValue);

    if (type === "string" && this._stringIsLong(aValue)) {
      return this.longStringGrip(aValue, aPool);
    }

    if (type === "boolean" || type === "string" || type === "number") {
      return aValue;
    }

    if (aValue === null) {
      return { type: "null" };
    }

    if (aValue === undefined) {
      return { type: "undefined" }
    }

    if (typeof(aValue) === "object") {
      return this.objectGrip(aValue, aPool);
    }

    dbg_assert(false, "Failed to provide a grip for: " + aValue);
    return null;
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
    if (this.sources.isBlackBoxed(aFrame.script.url)) {
      return undefined;
    }
    return this._pauseAndRespond(aFrame, { type: "debuggerStatement" });
  },

  








  onExceptionUnwind: function TA_onExceptionUnwind(aFrame, aValue) {
    if (this.sources.isBlackBoxed(aFrame.script.url)) {
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
      return this._nest();
    } catch(e) {
      Cu.reportError("Got an exception during TA_onExceptionUnwind: " + e +
                     ": " + e.stack);
      return undefined;
    }
  },

  








  onNewScript: function TA_onNewScript(aScript, aGlobal) {
    this._addScript(aScript);
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

  







  _addScriptSync: function TA__addScriptSync(aScript) {
    if (!this._allowSource(aScript.url)) {
      return false;
    }

    this.sources.source(aScript.url);
    
    let existing = this._breakpointStore[aScript.url];
    if (existing) {
      let endLine = aScript.startLine + aScript.lineCount - 1;
      
      
      for (let line = existing.length - 1; line >= 0; line--) {
        let bp = existing[line];
        
        
        
        if (bp && !bp.actor.scripts.length &&
            line >= aScript.startLine && line <= endLine) {
          this._setBreakpoint(bp);
        }
      }
    }
    return true;
  },

  






  _addScript: function TA__addScript(aScript) {
    if (!this._allowSource(aScript.url)) {
      return resolve(false);
    }

    
    
    return this.sources.sourcesForScript(aScript).then(() => {

      
      let existing = this._breakpointStore[aScript.url];
      if (existing) {
        let endLine = aScript.startLine + aScript.lineCount - 1;
        
        
        for (let line = existing.length - 1; line >= 0; line--) {
          let bp = existing[line];
          
          
          
          if (bp && !bp.actor.scripts.length &&
              line >= aScript.startLine && line <= endLine) {
            this._setBreakpoint(bp);
          }
        }
      }

      return true;
    });
  },
};

ThreadActor.prototype.requestTypes = {
  "attach": ThreadActor.prototype.onAttach,
  "detach": ThreadActor.prototype.onDetach,
  "reconfigure": ThreadActor.prototype.onReconfigure,
  "resume": ThreadActor.prototype.onResume,
  "clientEvaluate": ThreadActor.prototype.onClientEvaluate,
  "frames": ThreadActor.prototype.onFrames,
  "interrupt": ThreadActor.prototype.onInterrupt,
  "releaseMany": ThreadActor.prototype.onReleaseMany,
  "setBreakpoint": ThreadActor.prototype.onSetBreakpoint,
  "sources": ThreadActor.prototype.onSources,
  "threadGrips": ThreadActor.prototype.onThreadGrips
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












function SourceActor(aUrl, aThreadActor, aSourceMap=null) {
  this._threadActor = aThreadActor;
  this._url = aUrl;
  this._sourceMap = aSourceMap;
}

SourceActor.prototype = {
  constructor: SourceActor,
  actorPrefix: "source",

  get threadActor() this._threadActor,
  get url() this._url,

  form: function SA_form() {
    return {
      actor: this.actorID,
      url: this._url,
      isBlackBoxed: this.threadActor.sources.isBlackBoxed(this.url)
      
    };
  },

  disconnect: function LSA_disconnect() {
    if (this.registeredPool && this.registeredPool.sourceActors) {
      delete this.registeredPool.sourceActors[this.actorID];
    }
  },

  


  onSource: function SA_onSource(aRequest) {
    let sourceContent = null;
    if (this._sourceMap) {
      sourceContent = this._sourceMap.sourceContentFor(this._url);
    }

    if (sourceContent) {
      return {
        from: this.actorID,
        source: this.threadActor.createValueGrip(
          sourceContent, this.threadActor.threadLifetimePool)
      };
    }

    
    
    
    return fetch(this._url, { loadFromCache: !this._sourceMap })
      .then((aSource) => {
        return this.threadActor.createValueGrip(
          aSource, this.threadActor.threadLifetimePool);
      })
      .then((aSourceGrip) => {
        return {
          from: this.actorID,
          source: aSourceGrip
        };
      }, (aError) => {
        let msg = "Got an exception during SA_onSource: " + aError +
          "\n" + aError.stack;
        Cu.reportError(msg);
        dumpn(msg);
        return {
          "from": this.actorID,
          "error": "loadSourceError",
          "message": "Could not load the source for " + this._url + "."
        };
      });
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
  "unblackbox": SourceActor.prototype.onUnblackBox
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
      } else if (this.obj.displayName) {
        g.displayName = this.obj.displayName;
      }

      
      
      let desc = this.obj.getOwnPropertyDescriptor("displayName");
      if (desc && desc.value && typeof desc.value == "string") {
        g.userDisplayName = this.threadActor.createValueGrip(desc.value);
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
    for (let name of this.obj.getOwnPropertyNames()) {
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

    let retval = {
      configurable: desc.configurable,
      enumerable: desc.enumerable
    };

    if (desc.value !== undefined) {
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
      form.where = { url: this.frame.script.url,
                     line: this.frame.script.getOffsetLine(this.frame.offset) };
      form.isBlackBoxed = this.threadActor.sources.isBlackBoxed(this.frame.script.url)
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
    if (this.threadActor.sources.isBlackBoxed(this.location.url)) {
      return undefined;
    }

    
    let reason = { type: "breakpoint", actors: [ this.actorID ] };
    return this.threadActor._pauseAndRespond(aFrame, reason, (aPacket) => {
      let { url, line } = aPacket.frame.where;
      return this.threadActor.sources.getOriginalLocation(url, line)
        .then(function (aOrigPosition) {
          aPacket.frame.where = aOrigPosition;
          return aPacket;
        });
    });
  },

  





  onDelete: function BA_onDelete(aRequest) {
    
    let scriptBreakpoints = this.threadActor._breakpointStore[this.location.url];
    delete scriptBreakpoints[this.location.line];
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

  
  this._sourceMaps = Object.create(null);
  
  this._sourceMapsByGeneratedSource = Object.create(null);
  
  this._sourceMapsByOriginalSource = Object.create(null);
  
  this._sourceActors = Object.create(null);
  
  this._generatedUrlsByOriginalUrl = Object.create(null);
}





ThreadSources._blackBoxedSources = new Set();

ThreadSources.prototype = {
  













  source: function TS_source(aURL, aSourceMap=null) {
    if (!this._allow(aURL)) {
      return null;
    }

    if (aURL in this._sourceActors) {
      return this._sourceActors[aURL];
    }

    let actor = new SourceActor(aURL, this._thread, aSourceMap);
    this._thread.threadLifetimePool.addActor(actor);
    this._sourceActors[aURL] = actor;
    try {
      this._onNewSource(actor);
    } catch (e) {
      reportError(e);
    }
    return actor;
  },

  







  sourcesForScript: function TS_sourcesForScript(aScript) {
    if (!this._useSourceMaps || !aScript.sourceMapURL) {
      return resolve([this.source(aScript.url)].filter(isNotNull));
    }

    return this.sourceMap(aScript)
      .then((aSourceMap) => {
        return [
          this.source(s, aSourceMap) for (s of aSourceMap.sources)
        ];
      }, (e) => {
        reportError(e);
        delete this._sourceMaps[this._normalize(aScript.sourceMapURL, aScript.url)];
        delete this._sourceMapsByGeneratedSource[aScript.url];
        return [this.source(aScript.url)];
      })
      .then(function (aSources) {
        return aSources.filter(isNotNull);
      });
  },

  




  sourceMap: function TS_sourceMap(aScript) {
    if (aScript.url in this._sourceMapsByGeneratedSource) {
      return this._sourceMapsByGeneratedSource[aScript.url];
    }
    dbg_assert(aScript.sourceMapURL);
    let sourceMapURL = this._normalize(aScript.sourceMapURL, aScript.url);
    let map = this._fetchSourceMap(sourceMapURL)
      .then((aSourceMap) => {
        for (let s of aSourceMap.sources) {
          this._generatedUrlsByOriginalUrl[s] = aScript.url;
          this._sourceMapsByOriginalSource[s] = resolve(aSourceMap);
        }
        return aSourceMap;
      });
    this._sourceMapsByGeneratedSource[aScript.url] = map;
    return map;
  },

  




  _fetchSourceMap: function TS__fetchSourceMap(aAbsSourceMapURL) {
    if (aAbsSourceMapURL in this._sourceMaps) {
      return this._sourceMaps[aAbsSourceMapURL];
    } else {
      let promise = fetch(aAbsSourceMapURL).then((rawSourceMap) => {
        let map =  new SourceMapConsumer(rawSourceMap);
        let base = aAbsSourceMapURL.replace(/\/[^\/]+$/, '/');
        if (base.indexOf("data:") !== 0) {
          map.sourceRoot = map.sourceRoot
            ? this._normalize(map.sourceRoot, base)
            : base;
        }
        return map;
      });
      this._sourceMaps[aAbsSourceMapURL] = promise;
      return promise;
    }
  },

  





  getOriginalLocation: function TS_getOriginalLocation(aSourceUrl, aLine) {
    if (aSourceUrl in this._sourceMapsByGeneratedSource) {
      return this._sourceMapsByGeneratedSource[aSourceUrl]
        .then(function (aSourceMap) {
          let { source, line } = aSourceMap.originalPositionFor({
            source: aSourceUrl,
            line: aLine,
            column: Infinity
          });
          return {
            url: source,
            line: line
          };
        });
    }

    
    return resolve({
      url: aSourceUrl,
      line: aLine
    });
  },

  










  getGeneratedLocation: function TS_getGeneratedLocation(aSourceUrl, aLine) {
    if (aSourceUrl in this._sourceMapsByOriginalSource) {
      return this._sourceMapsByOriginalSource[aSourceUrl]
        .then((aSourceMap) => {
          let { line } = aSourceMap.generatedPositionFor({
            source: aSourceUrl,
            line: aLine,
            column: Infinity
          });
          return {
            url: this._generatedUrlsByOriginalUrl[aSourceUrl],
            line: line
          };
        });
    }

    
    return resolve({
      url: aSourceUrl,
      line: aLine
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
    dbg_assert(aURLs.length > 1);
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
        NetUtil.asyncFetch(url, function onFetch(aStream, aStatus) {
          if (!Components.isSuccessCode(aStatus)) {
            deferred.reject("Request failed: " + url);
            return;
          }

          let source = NetUtil.readInputStreamToString(aStream, aStream.available());
          deferred.resolve(source);
          aStream.close();
        });
      } catch (ex) {
        deferred.reject("Request failed: " + url);
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
            deferred.reject("Request failed: " + url);
          }
        },
        onDataAvailable: function(aRequest, aContext, aStream, aOffset, aCount) {
          chunks.push(NetUtil.readInputStreamToString(aStream, aCount));
        },
        onStopRequest: function(aRequest, aContext, aStatusCode) {
          if (!Components.isSuccessCode(aStatusCode)) {
            deferred.reject("Request failed: " + url);
            return;
          }

          charset = channel.contentCharset;
          deferred.resolve(chunks.join(""));
        }
      };

      channel.loadFlags = aOptions.loadFromCache
        ? channel.LOAD_FROM_CACHE
        : channel.LOAD_BYPASS_CACHE;
      channel.asyncOpen(streamListener, null);
      break;
  }

  return deferred.promise.then(function (source) {
    return convertToUnicode(source, charset);
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




function reportError(aError) {
  Cu.reportError(aError);
  dumpn(aError.message + ":\n" + aError.stack);
}
