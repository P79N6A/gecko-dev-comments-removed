


"use strict";

const {Cc, Ci, Cu, Cr} = require("chrome");
const Services = require("Services");
const { Promise: promise } = Cu.import("resource://gre/modules/Promise.jsm", {});
const events = require("sdk/event/core");
const { on: systemOn, off: systemOff } = require("sdk/system/events");
const { setTimeout, clearTimeout } = require("sdk/timers");
const protocol = require("devtools/server/protocol");
const { CallWatcherActor, CallWatcherFront } = require("devtools/server/actors/call-watcher");
const { ThreadActor } = require("devtools/server/actors/script");
const { on, once, off, emit } = events;
const { method, Arg, Option, RetVal } = protocol;

exports.register = function(handle) {
  handle.addTabActor(WebAudioActor, "webaudioActor");
  handle.addGlobalActor(WebAudioActor, "webaudioActor");
};

exports.unregister = function(handle) {
  handle.removeTabActor(WebAudioActor);
  handle.removeGlobalActor(WebAudioActor);
};



const PARAM_POLLING_FREQUENCY = 1000;

const AUDIO_GLOBALS = [
  "AudioContext", "AudioNode"
];

const NODE_CREATION_METHODS = [
  "createBufferSource", "createMediaElementSource", "createMediaStreamSource",
  "createMediaStreamDestination", "createScriptProcessor", "createAnalyser",
  "createGain", "createDelay", "createBiquadFilter", "createWaveShaper",
  "createPanner", "createConvolver", "createChannelSplitter", "createChannelMerger",
  "createDynamicsCompressor", "createOscillator"
];

const NODE_ROUTING_METHODS = [
  "connect", "disconnect"
];

const NODE_PROPERTIES = {
  "OscillatorNode": {
    "type": {},
    "frequency": {},
    "detune": {}
  },
  "GainNode": {
    "gain": {}
  },
  "DelayNode": {
    "delayTime": {}
  },
  "AudioBufferSourceNode": {
    "buffer": { "Buffer": true },
    "playbackRate": {},
    "loop": {},
    "loopStart": {},
    "loopEnd": {}
  },
  "ScriptProcessorNode": {
    "bufferSize": { "readonly": true }
  },
  "PannerNode": {
    "panningModel": {},
    "distanceModel": {},
    "refDistance": {},
    "maxDistance": {},
    "rolloffFactor": {},
    "coneInnerAngle": {},
    "coneOuterAngle": {},
    "coneOuterGain": {}
  },
  "ConvolverNode": {
    "buffer": { "Buffer": true },
    "normalize": {},
  },
  "DynamicsCompressorNode": {
    "threshold": {},
    "knee": {},
    "ratio": {},
    "reduction": {},
    "attack": {},
    "release": {}
  },
  "BiquadFilterNode": {
    "type": {},
    "frequency": {},
    "Q": {},
    "detune": {},
    "gain": {}
  },
  "WaveShaperNode": {
    "curve": { "Float32Array": true },
    "oversample": {}
  },
  "AnalyserNode": {
    "fftSize": {},
    "minDecibels": {},
    "maxDecibels": {},
    "smoothingTimeConstant": {},
    "frequencyBinCount": { "readonly": true },
  },
  "AudioDestinationNode": {},
  "ChannelSplitterNode": {},
  "ChannelMergerNode": {},
  "MediaElementAudioSourceNode": {},
  "MediaStreamAudioSourceNode": {},
  "MediaStreamAudioDestinationNode": {
    "stream": { "MediaStream": true }
  }
};





let AudioNodeActor = exports.AudioNodeActor = protocol.ActorClass({
  typeName: "audionode",

  







  initialize: function (conn, node) {
    protocol.Actor.prototype.initialize.call(this, conn);

    
    
    
    this.nativeID = node.id;
    this.node = Cu.getWeakReference(node);

    try {
      this.type = getConstructorName(node);
    } catch (e) {
      this.type = "";
    }
  },

  destroy: function(conn) {
    protocol.Actor.prototype.destroy.call(this, conn);
  },

  



  getType: method(function () {
    return this.type;
  }, {
    response: { type: RetVal("string") }
  }),

  



  isSource: method(function () {
    return !!~this.type.indexOf("Source") || this.type === "OscillatorNode";
  }, {
    response: { source: RetVal("boolean") }
  }),

  








  setParam: method(function (param, value) {
    let node = this.node.get();

    if (node === null) {
      return CollectedAudioNodeError();
    }

    try {
      if (isAudioParam(node, param))
        node[param].value = value;
      else
        node[param] = value;

      return undefined;
    } catch (e) {
      return constructError(e);
    }
  }, {
    request: {
      param: Arg(0, "string"),
      value: Arg(1, "nullable:primitive")
    },
    response: { error: RetVal("nullable:json") }
  }),

  





  getParam: method(function (param) {
    let node = this.node.get();

    if (node === null) {
      return CollectedAudioNodeError();
    }

    
    
    let value = isAudioParam(node, param) ? node[param].value : node[param];

    
    
    
    
    
    return createGrip(value);
  }, {
    request: {
      param: Arg(0, "string")
    },
    response: { text: RetVal("nullable:primitive") }
  }),

  







  getParamFlags: method(function (param) {
    return (NODE_PROPERTIES[this.type] || {})[param];
  }, {
    request: { param: Arg(0, "string") },
    response: { flags: RetVal("nullable:primitive") }
  }),

  




  getParams: method(function () {
    let props = Object.keys(NODE_PROPERTIES[this.type]);
    return props.map(prop =>
      ({ param: prop, value: this.getParam(prop), flags: this.getParamFlags(prop) }));
  }, {
    response: { params: RetVal("json") }
  }),

  





  isAlive: function () {
    return !!this.node.get();
  }
});




let AudioNodeFront = protocol.FrontClass(AudioNodeActor, {
  initialize: function (client, form) {
    protocol.Front.prototype.initialize.call(this, client, form);
    this.manage(this);
  }
});






let WebAudioActor = exports.WebAudioActor = protocol.ActorClass({
  typeName: "webaudio",
  initialize: function(conn, tabActor) {
    protocol.Actor.prototype.initialize.call(this, conn);
    this.tabActor = tabActor;

    this._onContentFunctionCall = this._onContentFunctionCall.bind(this);

    
    
    
    this._nativeToActorID = new Map();

    this._onDestroyNode = this._onDestroyNode.bind(this);
    this._onGlobalDestroyed = this._onGlobalDestroyed.bind(this);
  },

  destroy: function(conn) {
    protocol.Actor.prototype.destroy.call(this, conn);
    this.finalize();
  },

  






  setup: method(function({ reload }) {
    
    
    this._firstNodeCreated = false;

    
    
    this._nativeToActorID.clear();

    if (this._initialized) {
      return;
    }

    this._initialized = true;

    this._callWatcher = new CallWatcherActor(this.conn, this.tabActor);
    this._callWatcher.onCall = this._onContentFunctionCall;
    this._callWatcher.setup({
      tracedGlobals: AUDIO_GLOBALS,
      startRecording: true,
      performReload: reload,
      holdWeak: true,
      storeCalls: false
    });
    
    
    
    
    on(this._callWatcher._contentObserver, "global-destroyed", this._onGlobalDestroyed);
  }, {
    request: { reload: Option(0, "boolean") },
    oneway: true
  }),

  



  _onContentFunctionCall: function(functionCall) {
    let { name } = functionCall.details;

    
    
    if (WebAudioFront.NODE_ROUTING_METHODS.has(name)) {
      this._handleRoutingCall(functionCall);
    }
    else if (WebAudioFront.NODE_CREATION_METHODS.has(name)) {
      this._handleCreationCall(functionCall);
    }
  },

  _handleRoutingCall: function(functionCall) {
    let { caller, args, window, name } = functionCall.details;
    let source = caller;
    let dest = args[0];
    let isAudioParam = dest ? getConstructorName(dest) === "AudioParam" : false;

    
    if (name === "connect" && isAudioParam) {
      this._onConnectParam(source, dest);
    }
    
    else if (name === "connect") {
      this._onConnectNode(source, dest);
    }
    
    else if (name === "disconnect") {
      this._onDisconnectNode(source);
    }
  },

  _handleCreationCall: function (functionCall) {
    let { caller, result } = functionCall.details;
    
    
    
    
    if (!this._firstNodeCreated) {
      
      
      this._onStartContext();
      this._onCreateNode(caller.destination);
      this._firstNodeCreated = true;
    }
    this._onCreateNode(result);
  },

  




  finalize: method(function() {
    if (!this._initialized) {
      return;
    }
    this.tabActor = null;
    this._initialized = false;
    off(this._callWatcher._contentObserver, "global-destroyed", this._onGlobalDestroyed);
    this.disableChangeParamEvents();
    this._nativeToActorID = null;
    this._callWatcher.eraseRecording();
    this._callWatcher.finalize();
    this._callWatcher = null;
  }, {
   oneway: true
  }),

  








  enableChangeParamEvents: method(function (nodeActor, wait) {
    
    this.disableChangeParamEvents();

    
    if (!nodeActor.isAlive()) {
      return;
    }

    let previous = mapAudioParams(nodeActor);

    
    this._pollingID = nodeActor.actorID;

    this.poller = new Poller(() => {
      
      if (!nodeActor.isAlive()) {
        this.disableChangeParamEvents();
        return;
      }

      let current = mapAudioParams(nodeActor);
      diffAudioParams(previous, current).forEach(changed => {
        this._onChangeParam(nodeActor, changed);
      });
      previous = current;
    }).on(wait || PARAM_POLLING_FREQUENCY);
  }, {
    request: {
      node: Arg(0, "audionode"),
      wait: Arg(1, "nullable:number"),
    },
    oneway: true
  }),

  disableChangeParamEvents: method(function () {
    if (this.poller) {
      this.poller.off();
    }
    this._pollingID = null;
  }, {
    oneway: true
  }),

  


  events: {
    "start-context": {
      type: "startContext"
    },
    "connect-node": {
      type: "connectNode",
      source: Option(0, "audionode"),
      dest: Option(0, "audionode")
    },
    "disconnect-node": {
      type: "disconnectNode",
      source: Arg(0, "audionode")
    },
    "connect-param": {
      type: "connectParam",
      source: Option(0, "audionode"),
      dest: Option(0, "audionode"),
      param: Option(0, "string")
    },
    "create-node": {
      type: "createNode",
      source: Arg(0, "audionode")
    },
    "destroy-node": {
      type: "destroyNode",
      source: Arg(0, "audionode")
    },
    "change-param": {
      type: "changeParam",
      param: Option(0, "string"),
      newValue: Option(0, "json"),
      oldValue: Option(0, "json"),
      actorID: Option(0, "string")
    }
  },

  




  _constructAudioNode: function (node) {
    
    node = new XPCNativeWrapper(node);

    this._instrumentParams(node);

    let actor = new AudioNodeActor(this.conn, node);
    this.manage(actor);
    this._nativeToActorID.set(node.id, actor.actorID);
    return actor;
  },

  




  _instrumentParams: function (node) {
    let type = getConstructorName(node);
    Object.keys(NODE_PROPERTIES[type])
      .filter(isAudioParam.bind(null, node))
      .forEach(paramName => {
        let param = node[paramName];
        param._parentID = node.id;
        param._paramName = paramName;
      });
  },

  





  _getActorByNativeID: function (nativeID) {
    
    
    
    
    if (!this._nativeToActorID) {
      return null;
    }

    
    
    nativeID = ~~nativeID;

    let actorID = this._nativeToActorID.get(nativeID);
    let actor = actorID != null ? this.conn.getActor(actorID) : null;
    return actor;
  },

  


  _onStartContext: function () {
    systemOn("webaudio-node-demise", this._onDestroyNode);
    emit(this, "start-context");
  },

  


  _onConnectNode: function (source, dest) {
    let sourceActor = this._getActorByNativeID(source.id);
    let destActor = this._getActorByNativeID(dest.id);
    emit(this, "connect-node", {
      source: sourceActor,
      dest: destActor
    });
  },

  


  _onConnectParam: function (source, param) {
    let sourceActor = this._getActorByNativeID(source.id);
    let destActor = this._getActorByNativeID(param._parentID);
    emit(this, "connect-param", {
      source: sourceActor,
      dest: destActor,
      param: param._paramName
    });
  },

  


  _onDisconnectNode: function (node) {
    let actor = this._getActorByNativeID(node.id);
    emit(this, "disconnect-node", actor);
  },

  



  _onChangeParam: function (actor, changed) {
    changed.actorID = actor.actorID;
    emit(this, "change-param", changed);
  },

  


  _onCreateNode: function (node) {
    let actor = this._constructAudioNode(node);
    emit(this, "create-node", actor);
  },

  



  _onDestroyNode: function ({data}) {
    
    let nativeID = ~~data;

    let actor = this._getActorByNativeID(nativeID);

    
    
    
    if (actor) {
      
      if (this._pollingID === actor.actorID) {
        this.disableChangeParamEvents();
      }
      this._nativeToActorID.delete(nativeID);
      emit(this, "destroy-node", actor);
    }
  },

  




  _onGlobalDestroyed: function (id) {
    if (this._callWatcher._tracedWindowId !== id) {
      return;
    }

    if (this._nativeToActorID) {
      this._nativeToActorID.clear();
    }
    systemOff("webaudio-node-demise", this._onDestroyNode);
  }
});




let WebAudioFront = exports.WebAudioFront = protocol.FrontClass(WebAudioActor, {
  initialize: function(client, { webaudioActor }) {
    protocol.Front.prototype.initialize.call(this, client, { actor: webaudioActor });
    this.manage(this);
  }
});

WebAudioFront.NODE_CREATION_METHODS = new Set(NODE_CREATION_METHODS);
WebAudioFront.NODE_ROUTING_METHODS = new Set(NODE_ROUTING_METHODS);










function isAudioParam (node, prop) {
  return !!(node[prop] && /AudioParam/.test(node[prop].toString()));
}








function constructError (err) {
  return {
    message: err.message,
    type: err.constructor.name
  };
}







function CollectedAudioNodeError () {
  return {
    message: "AudioNode has been garbage collected and can no longer be reached.",
    type: "UnreachableAudioNode"
  };
}








function getConstructorName (obj) {
  return obj.toString().match(/\[object ([^\[\]]*)\]\]?$/)[1];
}






function createGrip (value) {
  try {
    return ThreadActor.prototype.createValueGrip(value);
  }
  catch (e) {
    return {
      type: "object",
      preview: {
        kind: "ObjectWithText",
        text: ""
      },
      class: getConstructorName(value)
    };
  }
}






function mapAudioParams (node) {
  return node.getParams().reduce(function (obj, p) {
    obj[p.param] = p.value;
    return obj;
  }, {});
}










function diffAudioParams (prev, current) {
  return Object.keys(current).reduce((changed, param) => {
    if (!equalGrips(current[param], prev[param])) {
      changed.push({
        param: param,
        oldValue: prev[param],
        newValue: current[param]
      });
    }
    return changed;
  }, []);
}








function equalGrips (a, b) {
  let aType = typeof a;
  let bType = typeof b;
  if (aType !== bType) {
    return false;
  } else if (aType === "object") {
    
    
    
    
    if (a.type === b.type) {
      return true;
    }
    
    
    return false;
  } else {
    return a === b;
  }
}





function Poller (fn) {
  this.fn = fn;
}

Poller.prototype.on = function (wait) {
  let poller = this;
  poller.timer = setTimeout(poll, wait);
  function poll () {
    poller.fn();
    poller.timer = setTimeout(poll, wait);
  }
  return this;
};

Poller.prototype.off = function () {
  if (this.timer) {
    clearTimeout(this.timer);
  }
  return this;
};
