





"use strict";















function ThreadActor(aHooks)
{
  this._state = "detached";
  this._frameActors = [];
  this._environmentActors = [];
  this._hooks = aHooks ? aHooks : {};
  this._scripts = {};
}





ThreadActor._breakpointStore = {};

ThreadActor.prototype = {
  actorPrefix: "context",

  get state() { return this._state; },

  get dbg() { return this._dbg; },

  get _breakpointStore() { return ThreadActor._breakpointStore; },

  get threadLifetimePool() {
    if (!this._threadLifetimePool) {
      this._threadLifetimePool = new ActorPool(this.conn);
      this.conn.addActorPool(this._threadLifetimePool);
    }
    return this._threadLifetimePool;
  },

  clearDebuggees: function TA_clearDebuggees() {
    if (this._dbg) {
      let debuggees = this._dbg.getDebuggees();
      for (let debuggee of debuggees) {
        this._dbg.removeDebuggee(debuggee);
      }
    }
    this.conn.removeActorPool(this._threadLifetimePool || undefined);
    this._threadLifetimePool = null;
    
    
    
    for (let url in this._scripts) {
      delete this._scripts[url];
    }
    this._scripts = {};
  },

  


  addDebuggee: function TA_addDebuggee(aGlobal) {
    
    
    
    

    if (!this._dbg) {
      this._dbg = new Debugger();
      this._dbg.uncaughtExceptionHook = this.uncaughtExceptionHook.bind(this);
      this._dbg.onDebuggerStatement = this.onDebuggerStatement.bind(this);
      this._dbg.onNewScript = this.onNewScript.bind(this);
      
      this.dbg.enabled = this._state != "detached";
    }

    this.dbg.addDebuggee(aGlobal);
    for (let s of this.dbg.findScripts()) {
      this._addScript(s);
    }
  },

  


  removeDebugee: function TA_removeDebuggee(aGlobal) {
    try {
      this.dbg.removeDebuggee(aGlobal);
    } catch(ex) {
      
      
    }
  },

  disconnect: function TA_disconnect() {
    if (this._state == "paused") {
      this.onResume();
    }

    this._state = "exited";

    this.clearDebuggees();

    if (!this._dbg) {
      return;
    }
    this._dbg.enabled = false;
    this._dbg = null;
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
    } catch(e) {
      Cu.reportError(e);
      return { error: "notAttached", message: e.toString() };
    }
  },

  onDetach: function TA_onDetach(aRequest) {
    this.disconnect();
    return { type: "detached" };
  },

  








  _pauseAndRespond: function TA__pauseAndRespond(aFrame, aReason) {
    try {
      let packet = this._paused(aFrame);
      if (!packet) {
        return undefined;
      }
      packet.why = aReason;
      this.conn.send(packet);
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
      
      
      let pauseAndRespond = this._pauseAndRespond.bind(this);
      let createValueGrip = this.createValueGrip.bind(this);

      let startFrame = this._youngestFrame;
      let startLine;
      if (this._youngestFrame.script) {
        let offset = this._youngestFrame.offset;
        startLine = this._youngestFrame.script.getOffsetLine(offset);
      }

      

      let onEnterFrame = function TA_onEnterFrame(aFrame) {
        return pauseAndRespond(aFrame, { type: "resumeLimit" });
      };

      let onPop = function TA_onPop(aCompletion) {
        

        
        
        this.reportedPop = true;

        return pauseAndRespond(this, { type: "resumeLimit" });
      }

      let onStep = function TA_onStep() {
        

        
        if (this !== startFrame ||
            (this.script &&
             this.script.getOffsetLine(this.offset) != startLine)) {
          return pauseAndRespond(this, { type: "resumeLimit" });
        }

        
        return undefined;
      }

      switch (aRequest.resumeLimit.type) {
        case "step":
          this.dbg.onEnterFrame = onEnterFrame;
          
        case "next":
          let stepFrame = this._getNextStepFrame(startFrame);
          if (stepFrame) {
            stepFrame.onStep = onStep;
            stepFrame.onPop = onPop;
          }
          break;
        case "finish":
          stepFrame = this._getNextStepFrame(startFrame);
          if (stepFrame) {
            stepFrame.onPop = onPop;
          }
          break;
        default:
          return { error: "badParameterType",
                   message: "Unknown resumeLimit type" };
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

    
    
    
    
    let youngest = this._youngestFrame;

    
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

    
    let frame = this._youngestFrame;
    let i = 0;
    while (frame && (i < start)) {
      frame = frame.older;
      i++;
    }

    
    
    let frames = [];
    for (; frame && (!count || i < (start + count)); i++) {
      let form = this._createFrameActor(frame).form();
      form.depth = i;
      frames.push(form);
      frame = frame.older;
    }

    return { frames: frames };
  },

  onReleaseMany: function TA_onReleaseMany(aRequest) {
    if (!aRequest.actors) {
      return { error: "missingParameter",
               message: "no actors were specified" };
    }

    for each (let actorID in aRequest.actors) {
      let actor = this.threadLifetimePool.get(actorID);
      this.threadLifetimePool.objectActors.delete(actor.obj);
      this.threadLifetimePool.removeActor(actorID);
    }
    return {};
  },

  


  onSetBreakpoint: function TA_onSetBreakpoint(aRequest) {
    if (this.state !== "paused") {
      return { error: "wrongState",
               message: "Breakpoints can only be set while the debuggee is paused."};
    }

    let location = aRequest.location;
    let line = location.line;
    if (!this._scripts[location.url] || line < 0) {
      return { error: "noScript" };
    }

    
    
    if (!this._breakpointStore[location.url]) {
      this._breakpointStore[location.url] = [];
    }
    let scriptBreakpoints = this._breakpointStore[location.url];
    scriptBreakpoints[line] = {
      url: location.url,
      line: line,
      column: location.column
    };

    return this._setBreakpoint(location);
  },

  









  _setBreakpoint: function TA__setBreakpoint(aLocation) {
    
    let scripts = this._scripts[aLocation.url];
    
    let script = null;
    for (let i = aLocation.line; i >= 0; i--) {
      
      if (scripts[i]) {
        
        
        if (i + scripts[i].lineCount < aLocation.line) {
          continue;
        }
        script = scripts[i];
        break;
      }
    }

    let location = { url: aLocation.url, line: aLocation.line };
    
    let scriptBreakpoints = this._breakpointStore[location.url];
    let bpActor;
    if (scriptBreakpoints &&
        scriptBreakpoints[location.line] &&
        scriptBreakpoints[location.line].actor) {
      bpActor = scriptBreakpoints[location.line].actor;
    }
    if (!bpActor) {
      bpActor = new BreakpointActor(this, location);
      this._hooks.addToBreakpointPool(bpActor);
      if (scriptBreakpoints[location.line]) {
        scriptBreakpoints[location.line].actor = bpActor;
      }
    }

    if (!script) {
      return { error: "noScript", actor: bpActor.actorID };
    }

    script = this._getInnermostContainer(script, aLocation.line);
    bpActor.addScript(script, this);

    let offsets = script.getLineOffsets(aLocation.line);
    let codeFound = false;
    for (let i = 0; i < offsets.length; i++) {
      script.setBreakpoint(offsets[i], bpActor);
      codeFound = true;
    }

    let actualLocation;
    if (offsets.length == 0) {
      
      let lines = script.getAllOffsets();
      let oldLine = aLocation.line;
      for (let line = oldLine; line < lines.length; ++line) {
        if (lines[line]) {
          for (let i = 0; i < lines[line].length; i++) {
            script.setBreakpoint(lines[line][i], bpActor);
            codeFound = true;
          }
          actualLocation = {
            url: aLocation.url,
            line: line,
            column: aLocation.column
          };
          bpActor.location = actualLocation;
          
          scriptBreakpoints[line] = scriptBreakpoints[oldLine];
          scriptBreakpoints[line].line = line;
          delete scriptBreakpoints[oldLine];
          break;
        }
      }
    }
    if (!codeFound) {
      return  { error: "noCodeAtLineColumn", actor: bpActor.actorID };
    }

    return { actor: bpActor.actorID, actualLocation: actualLocation };
  },

  








  _getInnermostContainer: function TA__getInnermostContainer(aScript, aLine) {
    let children = aScript.getChildScripts();
    if (children.length > 0) {
      for (let i = 0; i < children.length; i++) {
        let child = children[i];
        
        if (child.startLine <= aLine &&
            child.startLine + child.lineCount > aLine) {
          return this._getInnermostContainer(child, aLine);
        }
      }
    }
    
    return aScript;
  },

  


  onScripts: function TA_onScripts(aRequest) {
    
    for (let s of this.dbg.findScripts()) {
      this._addScript(s);
    }
    
    let scripts = [];
    for (let url in this._scripts) {
      for (let i = 0; i < this._scripts[url].length; i++) {
        if (!this._scripts[url][i]) {
          continue;
        }
        let script = {
          url: url,
          startLine: i,
          lineCount: this._scripts[url][i].lineCount
        };
        scripts.push(script);
      }
    }

    let packet = { from: this.actorID,
                   scripts: scripts };
    return packet;
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
    } catch(e) {
      Cu.reportError(e);
      return { error: "notInterrupted", message: e.toString() };
    }
  },

  


  _requestFrame: function TA_requestFrame(aFrameID) {
    if (!aFrameID) {
      return this._youngestFrame;
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

    
    
    this._youngestFrame = aFrame;

    
    
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

    DebuggerServer.xpcInspector.enterNestedEventLoop();

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
    this._youngestFrame = null;

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

  



  createValueGrip: function TA_createValueGrip(aValue) {
    let type = typeof(aValue);

    if (type === "string" && this._stringIsLong(aValue)) {
      return this.longStringGrip(aValue);
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
      return this.pauseObjectGrip(aValue);
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
    }

    let actor = new ObjectActor(aValue, this);
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

  





  threadObjectGrip: function TA_threadObjectGrip(aValue) {
    return this.objectGrip(aValue, this.threadLifetimePool);
  },

  





  longStringGrip: function TA_longStringGrip(aString) {
    if (!this._pausePool) {
      throw new Error("LongString grip requested while not paused.");
    }

    if (!this._pausePool.longStringActors) {
      this._pausePool.longStringActors = {};
    }

    if (this._pausePool.longStringActors.hasOwnProperty(aString)) {
      return this._pausePool.longStringActors[aString].grip();
    }

    let actor = new LongStringActor(aString, this);
    this._pausePool.addActor(actor);
    this._pausePool.longStringActors[aString] = actor;
    return actor.grip();
  },

  






  _stringIsLong: function TA__stringIsLong(aString) {
    return aString.length >= DebuggerServer.LONG_STRING_LENGTH;
  },

  

  







  uncaughtExceptionHook: function TA_uncaughtExceptionHook(aException) {
    dumpn("Got an exception:" + aException);
  },

  






  onDebuggerStatement: function TA_onDebuggerStatement(aFrame) {
    return this._pauseAndRespond(aFrame, { type: "debuggerStatement" });
  },

  








  onExceptionUnwind: function TA_onExceptionUnwind(aFrame, aValue) {
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
    if (this._addScript(aScript)) {
      
      this.conn.send({
        from: this.actorID,
        type: "newScript",
        url: aScript.url,
        startLine: aScript.startLine,
        lineCount: aScript.lineCount
      });
    }
  },

  






  _addScript: function TA__addScript(aScript) {
    
    if (aScript.url.indexOf("chrome://") == 0) {
      return false;
    }
    
    if (aScript.url.indexOf("about:") == 0) {
      return false;
    }
    
    
    if (!this._scripts[aScript.url]) {
      this._scripts[aScript.url] = [];
    }
    this._scripts[aScript.url][aScript.startLine] = aScript;

    
    let existing = this._breakpointStore[aScript.url];
    if (existing) {
      let endLine = aScript.startLine + aScript.lineCount - 1;
      
      
      for (let line = existing.length - 1; line >= 0; line--) {
        let bp = existing[line];
        
        if (bp && line >= aScript.startLine && line <= endLine) {
          this._setBreakpoint(bp);
        }
      }
    }
    return true;
  }

};

ThreadActor.prototype.requestTypes = {
  "attach": ThreadActor.prototype.onAttach,
  "detach": ThreadActor.prototype.onDetach,
  "resume": ThreadActor.prototype.onResume,
  "clientEvaluate": ThreadActor.prototype.onClientEvaluate,
  "frames": ThreadActor.prototype.onFrames,
  "interrupt": ThreadActor.prototype.onInterrupt,
  "releaseMany": ThreadActor.prototype.onReleaseMany,
  "setBreakpoint": ThreadActor.prototype.onSetBreakpoint,
  "scripts": ThreadActor.prototype.onScripts
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
    }
  }
};











function update(aTarget, aNewAttrs) {
  for (let key in aNewAttrs) {
    aTarget[key] = aNewAttrs[key];
  }
}










function ObjectActor(aObj, aThreadActor)
{
  this.obj = aObj;
  this.threadActor = aThreadActor;
}

ObjectActor.prototype = Object.create(PauseScopedActor.prototype);

update(ObjectActor.prototype, {
  constructor: ObjectActor,
  actorPrefix: "obj",

  


  grip: function OA_grip() {
    return { "type": "object",
             "class": this.obj.class,
             "actor": this.actorID };
  },

  


  release: function OA_release() {
    this.registeredPool.objectActors.delete(this.obj);
    this.registeredPool.removeActor(this.actorID);
  },

  






  onOwnPropertyNames:
  PauseScopedActor.withPaused(function OA_onOwnPropertyNames(aRequest) {
    return { from: this.actorID,
             ownPropertyNames: this.obj.getOwnPropertyNames() };
  }),

  






  onPrototypeAndProperties:
  PauseScopedActor.withPaused(function OA_onPrototypeAndProperties(aRequest) {
    let ownProperties = {};
    for each (let name in this.obj.getOwnPropertyNames()) {
      try {
        let desc = this.obj.getOwnPropertyDescriptor(name);
        ownProperties[name] = this._propertyDescriptor(desc);
      } catch (e if e.name == "NS_ERROR_XPC_BAD_OP_ON_WN_PROTO") {
        
        
        dumpn("Error while getting the property descriptor for " + name +
              ": " + e.name);
      }
    }
    return { from: this.actorID,
             prototype: this.threadActor.createValueGrip(this.obj.proto),
             ownProperties: ownProperties };
  }),

  





  onPrototype: PauseScopedActor.withPaused(function OA_onPrototype(aRequest) {
    return { from: this.actorID,
             prototype: this.threadActor.createValueGrip(this.obj.proto) };
  }),

  






  onProperty: PauseScopedActor.withPaused(function OA_onProperty(aRequest) {
    if (!aRequest.name) {
      return { error: "missingParameter",
               message: "no property name was specified" };
    }

    let desc = this.obj.getOwnPropertyDescriptor(aRequest.name);
    return { from: this.actorID,
             descriptor: this._propertyDescriptor(desc) };
  }),

  






  _propertyDescriptor: function OA_propertyDescriptor(aObject) {
    let descriptor = {};
    descriptor.configurable = aObject.configurable;
    descriptor.enumerable = aObject.enumerable;
    if (aObject.value !== undefined) {
      descriptor.writable = aObject.writable;
      descriptor.value = this.threadActor.createValueGrip(aObject.value);
    } else {
      descriptor.get = this.threadActor.createValueGrip(aObject.get);
      descriptor.set = this.threadActor.createValueGrip(aObject.set);
    }
    return descriptor;
  },

  





  onDecompile: PauseScopedActor.withPaused(function OA_onDecompile(aRequest) {
    if (this.obj.class !== "Function") {
      return { error: "objectNotFunction",
               message: "decompile request is only valid for object grips " +
                        "with a 'Function' class." };
    }

    return { from: this.actorID,
             decompiledCode: this.obj.decompile(!!aRequest.pretty) };
  }),

  





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

    
    
    
    
    return { name: this.obj.name || null,
             scope: envActor.form(this.obj) };
  }),

  





  onNameAndParameters: PauseScopedActor.withPaused(function OA_onNameAndParameters(aRequest) {
    if (this.obj.class !== "Function") {
      return { error: "objectNotFunction",
               message: "nameAndParameters request is only valid for object " +
                        "grips with a 'Function' class." };
    }

    return { name: this.obj.name || null,
             parameters: this.obj.parameterNames };
  }),

  






  onThreadGrip: PauseScopedActor.withPaused(function OA_onThreadGrip(aRequest) {
    return { threadGrip: this.threadActor.threadObjectGrip(this.obj) };
  }),

  





  onRelease: PauseScopedActor.withPaused(function OA_onRelease(aRequest) {
    if (this.registeredPool !== this.threadActor.threadLifetimePool) {
      return { error: "notReleasable",
               message: "only thread-lifetime actors can be released." };
    }

    this.release();
    return {};
  }),
});

ObjectActor.prototype.requestTypes = {
  "nameAndParameters": ObjectActor.prototype.onNameAndParameters,
  "prototypeAndProperties": ObjectActor.prototype.onPrototypeAndProperties,
  "prototype": ObjectActor.prototype.onPrototype,
  "property": ObjectActor.prototype.onProperty,
  "ownPropertyNames": ObjectActor.prototype.onOwnPropertyNames,
  "scope": ObjectActor.prototype.onScope,
  "decompile": ObjectActor.prototype.onDecompile,
  "threadGrip": ObjectActor.prototype.onThreadGrip,
  "release": ObjectActor.prototype.onRelease,
};









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
  }

};

LongStringActor.prototype.requestTypes = {
  "substring": LongStringActor.prototype.onSubstring
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
      form.calleeName = getFunctionName(this.frame.callee);
    }

    let envActor = this.threadActor
                       .createEnvironmentActor(this.frame.environment,
                                               this.frameLifetimePool);
    form.environment = envActor ? envActor.form(this.frame) : envActor;
    form.this = this.threadActor.createValueGrip(this.frame.this);
    form.arguments = this._args();
    if (this.frame.script) {
      form.where = { url: this.frame.script.url,
                     line: this.frame.script.getOffsetLine(this.frame.offset) };
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

  





  hit: function BA_hit(aFrame) {
    
    let reason = { type: "breakpoint", actors: [ this.actorID ] };
    return this.threadActor._pauseAndRespond(aFrame, reason);
  },

  





  onDelete: function BA_onDelete(aRequest) {
    
    let scriptBreakpoints = this.threadActor._breakpointStore[this.location.url];
    delete scriptBreakpoints[this.location.line];
    
    this.threadActor._hooks.removeFromBreakpointPool(this.actorID);
    for (let script of this.scripts) {
      script.clearBreakpoint(this);
    }
    this.scripts = null;

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

  









  form: function EA_form(aObject) {
    
    
    if (!aObject.live) {
      return undefined;
    }

    let parent;
    if (this.obj.parent) {
      let thread = this.threadActor;
      parent = thread.createEnvironmentActor(this.obj.parent,
                                             this.registeredPool);
    }
    
    
    let parentFrame = aObject;
    if (this.obj.type == "declarative" && aObject.older) {
      parentFrame = aObject.older;
    }
    let form = { actor: this.actorID,
                 parent: parent ? parent.form(parentFrame) : parent };

    if (this.obj.type == "with") {
      form.type = "with";
      form.object = this.threadActor.createValueGrip(this.obj.object);
    } else if (this.obj.type == "object") {
      form.type = "object";
      form.object = this.threadActor.createValueGrip(this.obj.object);
    } else { 
      if (aObject.callee) {
        form.type = "function";
        form.function = this.threadActor.createValueGrip(aObject.callee);
        form.functionName = getFunctionName(aObject.callee);
      } else {
        form.type = "block";
      }
      form.bindings = this._bindings(aObject);
    }

    return form;
  },

  










  _bindings: function EA_bindings(aObject) {
    let bindings = { arguments: [], variables: {} };

    
    
    if (typeof this.obj.getVariable != "function") {
    
      return bindings;
    }

    let parameterNames;
    if (aObject && aObject.callee) {
      parameterNames = aObject.callee.parameterNames;
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







function getFunctionName(aFunction) {
  let name;
  if (aFunction.name) {
    name = aFunction.name;
  } else {
    
    
    let desc = aFunction.getOwnPropertyDescriptor("displayName");
    if (desc && desc.value && typeof desc.value == "string") {
      name = desc.value;
    } else if ("displayName" in aFunction) {
      
      name = aFunction.displayName;
    }
  }
  return name;
}
