



"use strict";

const { Cu } = require("chrome");
const { DebuggerServer } = require("devtools/server/main");
const { DevToolsUtils } = Cu.import("resource://gre/modules/devtools/DevToolsUtils.jsm", {});
const Debugger = require("Debugger");
const { getOffsetColumn } = require("devtools/server/actors/common");
const promise = require("promise");

Cu.import("resource://gre/modules/Task.jsm");



function getFrameDepth(frame) {
  if (typeof(frame.depth) != "number") {
    if (!frame.older) {
      frame.depth = 0;
    } else {
      
      const increment = frame.script && frame.script.url == "self-hosted"
        ? 0
        : 1;
      frame.depth = increment + getFrameDepth(frame.older);
    }
  }

  return frame.depth;
}

const { setTimeout } = require("sdk/timers");





const BUFFER_SEND_DELAY = 50;




const MAX_ARGUMENTS = 3;




const MAX_PROPERTIES = 3;




const TRACE_TYPES = new Set([
  "time",
  "return",
  "throw",
  "yield",
  "name",
  "location",
  "hitCount",
  "callsite",
  "parameterNames",
  "arguments",
  "depth"
]);





function TracerActor(aConn, aParent)
{
  this._dbg = null;
  this._parent = aParent;
  this._attached = false;
  this._activeTraces = new MapStack();
  this._totalTraces = 0;
  this._startTime = 0;
  this._sequence = 0;
  this._bufferSendTimer = null;
  this._buffer = [];
  this._hitCounts = new WeakMap();
  this._packetScheduler = new JobScheduler();

  
  
  
  this._requestsForTraceType = Object.create(null);
  for (let type of TRACE_TYPES) {
    this._requestsForTraceType[type] = 0;
  }

  this.onEnterFrame = this.onEnterFrame.bind(this);
  this.onExitFrame = this.onExitFrame.bind(this);
}

TracerActor.prototype = {
  actorPrefix: "trace",

  get attached() { return this._attached; },
  get idle()     { return this._attached && this._activeTraces.size === 0; },
  get tracing()  { return this._attached && this._activeTraces.size > 0; },

  get dbg() {
    if (!this._dbg) {
      this._dbg = this._parent.makeDebugger();
    }
    return this._dbg;
  },

  


  _send: function(aPacket) {
    this._buffer.push(aPacket);
    if (this._bufferSendTimer === null) {
      this._bufferSendTimer = setTimeout(() => {
        this.conn.send({
          from: this.actorID,
          type: "traces",
          traces: this._buffer.splice(0, this._buffer.length)
        });
        this._bufferSendTimer = null;
      }, BUFFER_SEND_DELAY);
    }
  },

  





  onAttach: function(aRequest) {
    if (this.attached) {
      return {
        error: "wrongState",
        message: "Already attached to a client"
      };
    }

    this.dbg.addDebuggees();
    this._attached = true;

    return {
      type: "attached",
      traceTypes: Object.keys(this._requestsForTraceType)
        .filter(k => !!this._requestsForTraceType[k])
    };
  },

  





  onDetach: function() {
    while (this.tracing) {
      this.onStopTrace();
    }

    this._dbg = null;
    this._attached = false;

    return {
      type: "detached"
    };
  },

  





  onStartTrace: function(aRequest) {
    for (let traceType of aRequest.trace) {
      if (!TRACE_TYPES.has(traceType)) {
        return {
          error: "badParameterType",
          message: "No such trace type: " + traceType
        };
      }
    }

    if (this.idle) {
      this.dbg.onEnterFrame = this.onEnterFrame;
      this.dbg.enabled = true;
      this._sequence = 0;
      this._startTime = Date.now();
    }

    
    for (let traceType of aRequest.trace) {
      this._requestsForTraceType[traceType]++;
    }

    this._totalTraces++;
    let name = aRequest.name || "Trace " + this._totalTraces;
    this._activeTraces.push(name, aRequest.trace);

    return { type: "startedTrace", why: "requested", name: name };
  },

  





  onStopTrace: function(aRequest) {
    if (!this.tracing) {
      return {
        error: "wrongState",
        message: "No active traces"
      };
    }

    let stoppedTraceTypes, name;
    if (aRequest && aRequest.name) {
      name = aRequest.name;
      if (!this._activeTraces.has(name)) {
        return {
          error: "noSuchTrace",
          message: "No active trace with name: " + name
        };
      }
      stoppedTraceTypes = this._activeTraces.delete(name);
    } else {
      name = this._activeTraces.peekKey();
      stoppedTraceTypes = this._activeTraces.pop();
    }

    for (let traceType of stoppedTraceTypes) {
      this._requestsForTraceType[traceType]--;
    }

    
    if (!this._requestsForTraceType.hitCount) {
      this._hitCounts.clear();
    }

    if (this.idle) {
      this._dbg.onEnterFrame = undefined;
      this.dbg.enabled = false;
    }

    return {
      type: "stoppedTrace",
      why: "requested",
      name
    };
  },

  

  






  onEnterFrame: function(aFrame) {
    if (aFrame.script && aFrame.script.url == "self-hosted") {
      return;
    }

    Task.spawn(function*() {
      
      
      
      let runInOrder = this._packetScheduler.schedule();

      let packet = {
        type: "enteredFrame",
        sequence: this._sequence++
      };

      let sourceMappedLocation;
      if (this._requestsForTraceType.name || this._requestsForTraceType.location) {
        if (aFrame.script) {
          sourceMappedLocation = yield this._parent.threadActor.sources.getOriginalLocation({
            url: aFrame.script.url,
            line: aFrame.script.startLine,
            
            
            
            
            column: getOffsetColumn(aFrame.offset, aFrame.script)
          });
        }
      }

      if (this._requestsForTraceType.name) {
        if (sourceMappedLocation && sourceMappedLocation.name) {
          packet.name = sourceMappedLocation.name;
        } else {
          packet.name = aFrame.callee
            ? aFrame.callee.displayName || "(anonymous function)"
            : "(" + aFrame.type + ")";
        }
      }

      if (this._requestsForTraceType.location) {
        if (sourceMappedLocation && sourceMappedLocation.url) {
          packet.location = sourceMappedLocation;
        }
      }

      if (this._requestsForTraceType.hitCount) {
        if (aFrame.script) {
          
          let previousHitCount = this._hitCounts.get(aFrame.script) || 0;
          this._hitCounts.set(aFrame.script, previousHitCount + 1);

          packet.hitCount = this._hitCounts.get(aFrame.script);
        }
      }

      if (this._parent.threadActor && aFrame.script) {
        packet.blackBoxed = this._parent.threadActor.sources.isBlackBoxed(aFrame.script.url);
      } else {
        packet.blackBoxed = false;
      }

      if (this._requestsForTraceType.callsite) {
        if (aFrame.older && aFrame.older.script) {
          let older = aFrame.older;
          packet.callsite = {
            url: older.script.url,
            line: older.script.getOffsetLine(older.offset),
            column: getOffsetColumn(older.offset, older.script)
          };
        }
      }

      if (this._requestsForTraceType.time) {
        packet.time = Date.now() - this._startTime;
      }

      if (this._requestsForTraceType.parameterNames && aFrame.callee) {
        packet.parameterNames = aFrame.callee.parameterNames;
      }

      if (this._requestsForTraceType.arguments && aFrame.arguments) {
        packet.arguments = [];
        let i = 0;
        for (let arg of aFrame.arguments) {
          if (i++ > MAX_ARGUMENTS) {
            break;
          }
          packet.arguments.push(createValueSnapshot(arg, true));
        }
      }

      if (this._requestsForTraceType.depth) {
        packet.depth = getFrameDepth(aFrame);
      }

      const onExitFrame = this.onExitFrame;
      aFrame.onPop = function (aCompletion) {
        onExitFrame(this, aCompletion);
      };

      runInOrder(() => this._send(packet));
    }.bind(this));
  },

  








  onExitFrame: function(aFrame, aCompletion) {
    let runInOrder = this._packetScheduler.schedule();

    let packet = {
      type: "exitedFrame",
      sequence: this._sequence++,
    };

    if (!aCompletion) {
      packet.why = "terminated";
    } else if (aCompletion.hasOwnProperty("return")) {
      packet.why = "return";
    } else if (aCompletion.hasOwnProperty("yield")) {
      packet.why = "yield";
    } else {
      packet.why = "throw";
    }

    if (this._requestsForTraceType.time) {
      packet.time = Date.now() - this._startTime;
    }

    if (this._requestsForTraceType.depth) {
      packet.depth = getFrameDepth(aFrame);
    }

    if (aCompletion) {
      if (this._requestsForTraceType.return && "return" in aCompletion) {
        packet.return = createValueSnapshot(aCompletion.return, true);
      }

      else if (this._requestsForTraceType.throw && "throw" in aCompletion) {
        packet.throw = createValueSnapshot(aCompletion.throw, true);
      }

      else if (this._requestsForTraceType.yield && "yield" in aCompletion) {
        packet.yield = createValueSnapshot(aCompletion.yield, true);
      }
    }

    runInOrder(() => this._send(packet));
  }
};




TracerActor.prototype.requestTypes = {
  "attach": TracerActor.prototype.onAttach,
  "detach": TracerActor.prototype.onDetach,
  "startTrace": TracerActor.prototype.onStartTrace,
  "stopTrace": TracerActor.prototype.onStopTrace
};

exports.TracerActor = TracerActor;








function MapStack()
{
  
  
  this._stack = [];
  this._map = Object.create(null);
}

MapStack.prototype = {
  get size() { return this._stack.length; },

  



  peekKey: function() {
    return this._stack[this.size - 1];
  },

  





  has: function(aKey) {
    return Object.prototype.hasOwnProperty.call(this._map, aKey);
  },

  






  get: function(aKey) {
    return this._map[aKey] || undefined;
  },

  










  push: function(aKey, aValue) {
    this.delete(aKey);
    this._stack.push(aKey);
    this._map[aKey] = aValue;
  },

  



  pop: function() {
    let key = this.peekKey();
    let value = this.get(key);
    this._stack.pop();
    delete this._map[key];
    return value;
  },

  







  delete: function(aKey) {
    let value = this.get(aKey);
    if (this.has(aKey)) {
      let keyIndex = this._stack.lastIndexOf(aKey);
      this._stack.splice(keyIndex, 1);
      delete this._map[aKey];
    }
    return value;
  }
};

















function createValueSnapshot(aValue, aDetailed=false) {
  switch (typeof aValue) {
    case "boolean":
      return aValue;
    case "string":
      if (aValue.length >= DebuggerServer.LONG_STRING_LENGTH) {
        return {
          type: "longString",
          initial: aValue.substring(0, DebuggerServer.LONG_STRING_INITIAL_LENGTH),
          length: aValue.length
        };
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
      return aDetailed
        ? detailedObjectSnapshot(aValue)
        : objectSnapshot(aValue);
    default:
      DevToolsUtils.reportException("TracerActor",
                      new Error("Failed to provide a grip for: " + aValue));
      return null;
  }
}







function objectSnapshot(aObject) {
  return {
    "type": "object",
    "class": aObject.class,
  };
}







function detailedObjectSnapshot(aObject) {
  let desc = objectSnapshot(aObject);
  let ownProperties = desc.ownProperties = Object.create(null);

  if (aObject.class == "DeadObject") {
    return desc;
  }

  let i = 0;
  for (let name of aObject.getOwnPropertyNames()) {
    if (i++ > MAX_PROPERTIES) {
      break;
    }
    let desc = propertySnapshot(name, aObject);
    if (desc) {
      ownProperties[name] = desc;
    }
  }

  return desc;
}













function propertySnapshot(aName, aObject) {
  let desc;
  try {
    desc = aObject.getOwnPropertyDescriptor(aName);
  } catch (e) {
    
    
    
    return {
      configurable: false,
      writable: false,
      enumerable: false,
      value: e.name
    };
  }

  
  
  if (!desc
      || typeof desc.value == "object" && desc.value !== null
      || !("value" in desc)) {
    return undefined;
  }

  return {
    configurable: desc.configurable,
    enumerable: desc.enumerable,
    writable: desc.writable,
    value: createValueSnapshot(desc.value)
  };
}





function JobScheduler()
{
  this._lastScheduledJob = promise.resolve();
}

JobScheduler.prototype = {
  





  schedule: function() {
    let deferred = promise.defer();
    let previousJob = this._lastScheduledJob;
    this._lastScheduledJob = deferred.promise;
    return function runInOrder(aJob) {
      previousJob.then(() => {
        aJob();
        deferred.resolve();
      });
    };
  }
};

exports.JobScheduler = JobScheduler;
