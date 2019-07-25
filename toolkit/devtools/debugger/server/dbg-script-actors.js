







































"use strict";













function ThreadActor(aHooks)
{
  this._state = "detached";
  this._frameActors = [];
  this._environmentActors = [];
  this._hooks = aHooks ? aHooks : {};
}

ThreadActor.prototype = {
  actorPrefix: "context",

  get state() { return this._state; },

  get dbg() { return this._dbg; },

  get threadLifetimePool() {
    if (!this._threadLifetimePool) {
      this._threadLifetimePool = new ActorPool(this.conn);
      this.conn.addActorPool(this._threadLifetimePool);
    }
    return this._threadLifetimePool;
  },

  _breakpointPool: null,
  get breakpointActorPool() {
    if (!this._breakpointPool) {
      this._breakpointPool = new ActorPool(this.conn);
      this.conn.addActorPool(this._breakpointPool);
    }
    return this._breakpointPool;
  },

  _scripts: {},

  


  addDebuggee: function TA_addDebuggee(aGlobal) {
    
    
    
    

    if (!this._dbg) {
      this._dbg = new Debugger();
    }

    this.dbg.addDebuggee(aGlobal);
    this.dbg.uncaughtExceptionHook = this.uncaughtExceptionHook.bind(this);
    this.dbg.onDebuggerStatement = this.onDebuggerStatement.bind(this);
    this.dbg.onNewScript = this.onNewScript.bind(this);
    
    this.dbg.enabled = false;
  },

  


  removeDebugee: function TA_removeDebuggee(aGlobal) {
    try {
      this.dbg.removeDebuggee(aGlobal);
    } catch(ex) {
      
      
    }
  },

  disconnect: function TA_disconnect() {
    this._state = "exited";
    if (this.dbg) {
      this.dbg.enabled = false;
      this._dbg = null;
    }
    this.conn.removeActorPool(this._threadLifetimePool || undefined);
    this._threadLifetimePool = null;
    this.conn.removeActorPool(this._breakpointPool);
    this._breakpointPool = null;
    for (let url in this._scripts) {
      delete this._scripts[url];
    }
    this._scripts = {};
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

  onResume: function TA_onResume(aRequest) {
    let packet = this._resumed();
    DebuggerServer.xpcInspector.exitNestedEventLoop();
    return packet;
  },

  onClientEvaluate: function TA_onClientEvaluate(aRequest) {
    if (this.state !== "paused") {
      return { type: "wrongState",
               message: "Debuggee must be paused to evaluate code." };
    };

    let frame = this._requestFrame(aRequest.frame);
    if (!frame) {
      
      return { type: "unknownFrame",
               message: "Evaluation frame not found" };
    }


    
    
    
    
    let youngest = this._youngestFrame;

    
    let resumedPacket = this._resumed();
    this.conn.send(resumedPacket);

    
    
    let completion = frame.eval(aRequest.expression);

    
    let packet = this._paused(youngest);
    packet.why = { type: "clientEvaluated" };
    if ("return" in completion) {
      packet.why.value = this.createValueGrip(completion["return"]);
    } else if ("throw" in completion) {
      packet.why.exception = this.createValueGrip(completion["throw"]);
    } else {
      
      packet.why.terminated = true;
    }

    

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
      let grip = this._createFrameActor(frame).grip();
      grip.depth = i;
      frames.push(grip);
      frame = frame.older;
    }

    return { frames: frames };
  },

  onReleaseMany: function TA_onReleaseMany(aRequest) {
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
    
    if (!this._scripts[location.url] || location.line < 0) {
      return { from: this.actorID,
               error: "noScript" };
    }
    
    let scripts = this._scripts[location.url];
    
    let script = null;
    for (let i = location.line; i >= 0; i--) {
      
      if (scripts[i]) {
        
        
        if (i + scripts[i].lineCount < location.line) {
          break;
        }
        script = scripts[i];
        break;
      }
    }
    if (!script) {
      return { from: this.actorID,
               error: "noScript" };
    }
    let bpActor = new BreakpointActor(script, this);
    this.breakpointActorPool.addActor(bpActor);
    var offsets = script.getLineOffsets(location.line);
    for (var i = 0; i < offsets.length; i++) {
      script.setBreakpoint(offsets[i], bpActor);
    }
    let packet = { from: this.actorID,
                   actor: bpActor.actorID };
    return packet;
  },

  


  onScripts: function TA_onScripts(aRequest) {
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

  


  onInterrupt: function TA_onScripts(aRequest) {
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
      packet.frame = this._createFrameActor(aFrame).grip();
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

  








  createEnvironmentActor: function TA_createEnvironmentActor(aObject, aPool) {
    let environment = aObject.environment;
    
    
    
    if (!environment) {
      return undefined;
    }

    if (environment.actor) {
      return environment.actor;
    }

    let actor = new EnvironmentActor(aObject, this);
    this._environmentActors.push(actor);
    aPool.addActor(actor);
    environment.actor = actor;

    return actor;
  },

  



  createValueGrip: function TA_createValueGrip(aValue) {
    let type = typeof(aValue);
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

  

  







  uncaughtExceptionHook: function TA_uncaughtExceptionHook(aException) {
    dumpn("Got an exception:" + aException);
  },

  






  onDebuggerStatement: function TA_onDebuggerStatement(aFrame) {
    try {
      let packet = this._paused(aFrame);
      if (!packet) {
        return undefined;
      }
      packet.why = { type: "debuggerStatement" };
      this.conn.send(packet);
      return this._nest();
    } catch(e) {
      Cu.reportError("Got an exception during onDebuggerStatement: " + e +
                     ": " + e.stack);
      return undefined;
    }
  },

  












  onNewScript: function TA_onNewScript(aScript, aFunction) {
    dumpn("Got a new script:" + aScript + ", url: " + aScript.url +
          ", startLine: " + aScript.startLine + ", lineCount: " +
          aScript.lineCount + ", strictMode: " + aScript.strictMode +
          ", function: " + aFunction);
    
    
    
    
    if (!this._scripts[aScript.url]) {
      this._scripts[aScript.url] = [];
    }
    this._scripts[aScript.url][aScript.startLine] = aScript;
    
    this.conn.send({ from: this.actorID, type: "newScript",
                     url: aScript.url, startLine: aScript.startLine });
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










function ObjectActor(aObj, aThreadActor)
{
  this.obj = aObj;
  this.threadActor = aThreadActor;
}

ObjectActor.prototype = {
  actorPrefix: "obj",

  WRONG_STATE_RESPONSE: {
    error: "wrongState",
    message: "Object actors can only be accessed while the thread is paused."
  },

  


  grip: function OA_grip() {
    return { "type": "object",
             "class": this.obj["class"],
             "actor": this.actorID };
  },

  


  release: function OA_release() {
    this.registeredPool.objectActors.delete(this.obj);
    this.registeredPool.removeActor(this.actorID);
  },

  






  onOwnPropertyNames: function OA_onOwnPropertyNames(aRequest) {
    if (this.threadActor.state !== "paused") {
      return this.WRONG_STATE_RESPONSE;
    }

    return { from: this.actorID,
             ownPropertyNames: this.obj.getOwnPropertyNames() };
  },

  






  onPrototypeAndProperties: function OA_onPrototypeAndProperties(aRequest) {
    if (this.threadActor.state !== "paused") {
      return this.WRONG_STATE_RESPONSE;
    }

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
  },

  





  onPrototype: function OA_onPrototype(aRequest) {
    if (this.threadActor.state !== "paused") {
      return this.WRONG_STATE_RESPONSE;
    }

    return { from: this.actorID,
             prototype: this.threadActor.createValueGrip(this.obj.proto) };
  },

  






  onProperty: function OA_onProperty(aRequest) {
    if (this.threadActor.state !== "paused") {
      return this.WRONG_STATE_RESPONSE;
    }
    
    if (!aRequest.name) {
      return { error: "noPropertyName",
               message: "no property name was specified" };
    }

    let desc = this.obj.getOwnPropertyDescriptor(aRequest.name);
    return { from: this.actorID,
             descriptor: this._propertyDescriptor(desc) };
  },

  






  _propertyDescriptor: function OA_propertyDescriptor(aObject) {
    let descriptor = {};
    descriptor.configurable = aObject.configurable;
    descriptor.enumerable = aObject.enumerable;
    if (aObject.value) {
      descriptor.writable = aObject.writable;
      descriptor.value = this.threadActor.createValueGrip(aObject.value);
    } else {
      descriptor.get = this.threadActor.createValueGrip(aObject.get);
      descriptor.set = this.threadActor.createValueGrip(aObject.set);
    }
    return descriptor;
  },

  





  onDecompile: function OA_onDecompile(aRequest) {
    if (this.threadActor.state !== "paused") {
      return this.WRONG_STATE_RESPONSE;
    }

    if (this.obj["class"] !== "Function") {
      
      return { error: "unrecognizedPacketType",
               message: "decompile request is only valid for object grips " +
                        "with a 'Function' class." };
    }

    return { from: this.actorID,
             decompiledCode: this.obj.decompile(!!aRequest.pretty) };
  },

  





  onScope: function OA_onScope(aRequest) {
    if (this.threadActor.state !== "paused") {
      return this.WRONG_STATE_RESPONSE;
    }

    if (this.obj["class"] !== "Function") {
      
      return { error: "unrecognizedPacketType",
               message: "scope request is only valid for object grips with a" +
                        " 'Function' class." };
    }

    let packet = { name: this.obj.name || null };
    let envActor = this.threadActor.createEnvironmentActor(this.obj, this.registeredPool);
    packet.scope = envActor ? envActor.grip() : envActor;

    return packet;
  },

  





  onNameAndParameters: function OA_onNameAndParameters(aRequest) {
    if (this.threadActor.state !== "paused") {
      return this.WRONG_STATE_RESPONSE;
    }

    if (this.obj["class"] !== "Function") {
      
      return { error: "unrecognizedPacketType",
               message: "nameAndParameters request is only valid for object grips with a 'Function' class." };
    }

    return { name: this.obj.name || null,
             parameters: this.obj.parameterNames };
  },

  






  onThreadGrip: function OA_onThreadGrip(aRequest) {
    if (this.threadActor.state !== "paused") {
      return this.WRONG_STATE_RESPONSE;
    }

    return { threadGrip: this.threadActor.threadObjectGrip(this.obj) };
  },

  





  onRelease: function OA_onRelease(aRequest) {
    if (this.threadActor.state !== "paused") {
      return this.WRONG_STATE_RESPONSE;
    }
    if (this.registeredPool !== this.threadActor.threadLifetimePool) {
      
      return { error: "unrecognizedPacketType",
               message: "release is only recognized on thread-lifetime actors." };
    }

    this.release();
    return {};
  },
};

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

  


  grip: function FA_grip() {
    let grip = { actor: this.actorID,
                 type: this.frame.type };
    if (this.frame.type === "call") {
      grip.callee = this.threadActor.createValueGrip(this.frame.callee);
      grip.calleeName = this.frame.callee.name;
    }

    let envActor = this.threadActor
                       .createEnvironmentActor(this.frame,
                                               this.frameLifetimePool);
    grip.environment = envActor ? envActor.grip() : envActor;
    grip["this"] = this.threadActor.createValueGrip(this.frame["this"]);
    grip.arguments = this._args();

    if (!this.frame.older) {
      grip.oldest = true;
    }

    return grip;
  },

  _args: function FA__args() {
    if (!this.frame["arguments"]) {
      return [];
    }

    return [this.threadActor.createValueGrip(arg)
            for each (arg in this.frame["arguments"])];
  },

  





  onPop: function FA_onPop(aRequest) {
    return { error: "notImplemented",
             message: "Popping frames is not yet implemented." };
  }
};

FrameActor.prototype.requestTypes = {
  "pop": FrameActor.prototype.onPop,
};












function BreakpointActor(aScript, aThreadActor)
{
  this.script = aScript;
  this.threadActor = aThreadActor;
}

BreakpointActor.prototype = {
  actorPrefix: "breakpoint",

  





  hit: function BA_hit(aFrame) {
    try {
      let packet = this.threadActor._paused(aFrame);
      if (!packet) {
        return undefined;
      }
      
      packet.why = { type: "breakpoint", actors: [ this.actorID ] };
      this.conn.send(packet);
      return this.threadActor._nest();
    } catch(e) {
      Cu.reportError("Got an exception during hit: " + e + ': ' + e.stack);
      return undefined;
    }
  },

  





  onDelete: function BA_onDelete(aRequest) {
    this.threadActor.breakpointActorPool.removeActor(this.actorID);
    this.script.clearBreakpoint(this);
    this.script = null;

    return { from: this.actorID };
  }
};

BreakpointActor.prototype.requestTypes = {
  "delete": BreakpointActor.prototype.onDelete
};












function EnvironmentActor(aObject, aThreadActor)
{
  this.obj = aObject;
  this.threadActor = aThreadActor;
}

EnvironmentActor.prototype = {
  actorPrefix: "environment",

  


  grip: function EA_grip() {
    
    
    if (!this.obj.live) {
      return undefined;
    }

    let parent;
    if (this.obj.environment.parent) {
      parent = this.threadActor
                   .createEnvironmentActor(this.obj.environment.parent,
                                           this.registeredPool);
    }
    let grip = { actor: this.actorID,
                 parent: parent ? parent.grip() : parent };

    if (this.obj.environment.type == "object") {
      grip.type = "object"; 
      grip.object = this.threadActor.createValueGrip(this.obj.environment.object);
    } else {
      if (this.obj["class"] == "Function") {
        grip.type = "function";
        grip["function"] = this.threadActor.createValueGrip(this.obj);
        grip.functionName = this.obj.name;
      } else {
        grip.type = "block";
      }

      grip.bindings = this._bindings();
    }

    return grip;
  },

  



  _bindings: function EA_bindings() {
    let bindings = { mutable: {}, immutable: {} };

    
    if (typeof this.obj.environment.getVariableDescriptor != "function") {
      return bindings;
    }

    for (let name in this.obj.environment.names()) {
      let desc = this.obj.environment.getVariableDescriptor(name);
      
      if (desc.writable) {
        grip.bindings.mutable[name] = desc.value;
      } else {
        grip.bindings.immutable[name] = desc.value;
      }
    }

    return bindings;
  },

  






  onAssign: function EA_onAssign(aRequest) {
    let desc = this.obj.environment.getVariableDescriptor(aRequest.name);

    if (!desc.writable) {
      return { error: "immutableBinding",
               message: "Changing the value of an immutable binding is not " +
                        "allowed" };
    }

    try {
      this.obj.environment.setVariable(aRequest.name, aRequest.value);
    } catch (e) {
      if (e instanceof Debugger.DebuggeeWouldRun) {
        
        return { error: "debuggeeWouldRun",
                 message: "Assigning this value would cause the debuggee to run." };
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

