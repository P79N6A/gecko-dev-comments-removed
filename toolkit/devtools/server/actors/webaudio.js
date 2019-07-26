


"use strict";

const {Cc, Ci, Cu, Cr} = require("chrome");

const Services = require("Services");

const { Promise: promise } = Cu.import("resource://gre/modules/Promise.jsm", {});
const events = require("sdk/event/core");
const protocol = require("devtools/server/protocol");
const { CallWatcherActor, CallWatcherFront } = require("devtools/server/actors/call-watcher");
const { ThreadActor } = require("devtools/server/actors/script");

const { on, once, off, emit } = events;
const { method, Arg, Option, RetVal } = protocol;

exports.register = function(handle) {
  handle.addTabActor(WebAudioActor, "webaudioActor");
};

exports.unregister = function(handle) {
  handle.removeTabActor(WebAudioActor);
};

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
  "ChannelMergerNode": {}
};








let AudioNodeActor = exports.AudioNodeActor = protocol.ActorClass({
  typeName: "audionode",

  







  initialize: function (conn, node) {
    protocol.Actor.prototype.initialize.call(this, conn);
    this.node = unwrap(node);
    try {
      this.type = getConstructorName(this.node);
    } catch (e) {
      this.type = "";
    }
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
    try {
      if (isAudioParam(this.node, param))
        this.node[param].value = value;
      else
        this.node[param] = value;
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
    
    
    let value = isAudioParam(this.node, param) ? this.node[param].value : this.node[param];

    
    
    
    
    
    let grip;
    try {
      grip = ThreadActor.prototype.createValueGrip(value);
    }
    catch (e) {
      grip = createObjectGrip(value);
    }
    return grip;
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

  



  getParams: method(function (param) {
    let props = Object.keys(NODE_PROPERTIES[this.type]);
    return props.map(prop =>
      ({ param: prop, value: this.getParam(prop), flags: this.getParamFlags(prop) }));
  }, {
    response: { params: RetVal("json") }
  })
});




let AudioNodeFront = protocol.FrontClass(AudioNodeActor, {
  initialize: function (client, form) {
    protocol.Front.prototype.initialize.call(this, client, form);
    client.addActorPool(this);
    this.manage(this);
  }
});






let WebAudioActor = exports.WebAudioActor = protocol.ActorClass({
  typeName: "webaudio",
  initialize: function(conn, tabActor) {
    protocol.Actor.prototype.initialize.call(this, conn);
    this.tabActor = tabActor;
    this._onContentFunctionCall = this._onContentFunctionCall.bind(this);
  },

  destroy: function(conn) {
    protocol.Actor.prototype.destroy.call(this, conn);
    this.finalize();
  },

  






  setup: method(function({ reload }) {
    if (this._initialized) {
      return;
    }
    this._initialized = true;

    
    this._nodeActors = new Map();

    this._callWatcher = new CallWatcherActor(this.conn, this.tabActor);
    this._callWatcher.onCall = this._onContentFunctionCall;
    this._callWatcher.setup({
      tracedGlobals: AUDIO_GLOBALS,
      startRecording: true,
      performReload: reload
    });

    
    
    this._firstNodeCreated = false;
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
    let source = unwrap(caller);
    let dest = unwrap(args[0]);
    let isAudioParam = dest instanceof unwrap(window.AudioParam);

    
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
      this._onCreateNode(unwrap(caller.destination));
      this._firstNodeCreated = true;
    }
    this._onCreateNode(result);
  },

  




  finalize: method(function() {
    if (!this._initialized) {
      return;
    }
    this._initialized = false;
    this._callWatcher.eraseRecording();

    this._callWatcher.finalize();
    this._callWatcher = null;
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
      source: Arg(0, "audionode"),
      param: Arg(1, "string")
    },
    "change-param": {
      type: "changeParam",
      source: Option(0, "audionode"),
      param: Option(0, "string"),
      value: Option(0, "string")
    },
    "create-node": {
      type: "createNode",
      source: Arg(0, "audionode")
    }
  },

  




  _constructAudioNode: function (node) {
    let actor = new AudioNodeActor(this.conn, node);
    this.manage(actor);
    this._nodeActors.set(node, actor);
    return actor;
  },

  





  _actorFor: function (node) {
    let actor = this._nodeActors.get(node);
    if (!actor) {
      actor = this._constructAudioNode(node);
    }
    return actor;
  },

  


  _onStartContext: function () {
    events.emit(this, "start-context");
  },

  


  _onConnectNode: function (source, dest) {
    let sourceActor = this._actorFor(source);
    let destActor = this._actorFor(dest);
    events.emit(this, "connect-node", {
      source: sourceActor,
      dest: destActor
    });
  },

  



  _onConnectParam: function (source, dest) {
    
  },

  


  _onDisconnectNode: function (node) {
    let actor = this._actorFor(node);
    events.emit(this, "disconnect-node", actor);
  },

  


  _onParamChange: function (node, param, value) {
    let actor = this._actorFor(node);
    events.emit(this, "param-change", {
      source: actor,
      param: param,
      value: value
    });
  },

  


  _onCreateNode: function (node) {
    let actor = this._constructAudioNode(node);
    events.emit(this, "create-node", actor);
  }
});




let WebAudioFront = exports.WebAudioFront = protocol.FrontClass(WebAudioActor, {
  initialize: function(client, { webaudioActor }) {
    protocol.Front.prototype.initialize.call(this, client, { actor: webaudioActor });
    client.addActorPool(this);
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







function getConstructorName (obj) {
  return obj.toString().match(/\[object (.*)\]$/)[1];
}






function createObjectGrip (value) {
  return {
    type: "object",
    preview: {
      kind: "ObjectWithText",
      text: ""
    },
    class: getConstructorName(value)
  };
}
function unwrap (obj) {
  return XPCNativeWrapper.unwrap(obj);
}
