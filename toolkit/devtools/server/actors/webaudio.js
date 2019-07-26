


"use strict";

const {Cc, Ci, Cu, Cr} = require("chrome");

const Services = require("Services");

const events = require("sdk/event/core");
const protocol = require("devtools/server/protocol");

const { on, once, off, emit } = events;
const { method, Arg, Option, RetVal } = protocol;
const { AudioNodeActor } = require("devtools/server/actors/audionode");
const console = Cu.import("resource://gre/modules/devtools/Console.jsm").console;

exports.register = function(handle) {
  handle.addTabActor(WebAudioActor, "webaudioActor");
};

exports.unregister = function(handle) {
  handle.removeTabActor(WebAudioActor);
};









let WebAudioActor = exports.WebAudioActor = protocol.ActorClass({
  typeName: "webaudio",
  initialize: function(conn, tabActor) {
    protocol.Actor.prototype.initialize.call(this, conn);
    this.tabActor = tabActor;
    this._onGlobalCreated = this._onGlobalCreated.bind(this);
    this._onGlobalDestroyed = this._onGlobalDestroyed.bind(this);

    this._onStartContext = this._onStartContext.bind(this);
    this._onConnectNode = this._onConnectNode.bind(this);
    this._onConnectParam = this._onConnectParam.bind(this);
    this._onDisconnectNode = this._onDisconnectNode.bind(this);
    this._onParamChange = this._onParamChange.bind(this);
    this._onCreateNode = this._onCreateNode.bind(this);
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

    this._contentObserver = new ContentObserver(this.tabActor);
    this._webaudioObserver = new WebAudioObserver();

    on(this._contentObserver, "global-created", this._onGlobalCreated);
    on(this._contentObserver, "global-destroyed", this._onGlobalDestroyed);

    on(this._webaudioObserver, "start-context", this._onStartContext);
    on(this._webaudioObserver, "connect-node", this._onConnectNode);
    on(this._webaudioObserver, "connect-param", this._onConnectParam);
    on(this._webaudioObserver, "disconnect-node", this._onDisconnectNode);
    on(this._webaudioObserver, "param-change", this._onParamChange);
    on(this._webaudioObserver, "create-node", this._onCreateNode);

    if (reload) {
      this.tabActor.window.location.reload();
    }
  }, {
    request: { reload: Option(0, "boolean") },
    oneway: true
  }),

  




  finalize: method(function() {
    if (!this._initialized) {
      return;
    }
    this._initialized = false;

    this._contentObserver.stopListening();
    off(this._contentObserver, "global-created", this._onGlobalCreated);
    off(this._contentObserver, "global-destroyed", this._onGlobalDestroyed);

    off(this._webaudioObserver, "start-context", this._onStartContext);
    off(this._webaudioObserver, "connect-node", this._onConnectNode);
    off(this._webaudioObserver, "connect-param", this._onConnectParam);
    off(this._webaudioObserver, "disconnect-node", this._onDisconnectNode);
    off(this._webaudioObserver, "param-change", this._onParamChange);
    off(this._webaudioObserver, "create-node", this._onCreateNode);

    this._contentObserver = null;
    this._webaudioObserver = null;
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

  


  _onGlobalCreated: function(window) {
    WebAudioInstrumenter.handle(window, this._webaudioObserver);
  },

  


  _onGlobalDestroyed: function(id) {
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








function ContentObserver(tabActor) {
  this._contentWindow = tabActor.window;
  this._onContentGlobalCreated = this._onContentGlobalCreated.bind(this);
  this._onInnerWindowDestroyed = this._onInnerWindowDestroyed.bind(this);
  this.startListening();
}

ContentObserver.prototype = {
  


  startListening: function() {
    Services.obs.addObserver(
      this._onContentGlobalCreated, "content-document-global-created", false);
    Services.obs.addObserver(
      this._onInnerWindowDestroyed, "inner-window-destroyed", false);
  },

  


  stopListening: function() {
    Services.obs.removeObserver(
      this._onContentGlobalCreated, "content-document-global-created", false);
    Services.obs.removeObserver(
      this._onInnerWindowDestroyed, "inner-window-destroyed", false);
  },

  


  _onContentGlobalCreated: function(subject, topic, data) {
    if (subject == this._contentWindow) {
      emit(this, "global-created", subject);
    }
  },

  


  _onInnerWindowDestroyed: function(subject, topic, data) {
    let id = subject.QueryInterface(Ci.nsISupportsPRUint64).data;
    emit(this, "global-destroyed", id);
  }
};





let WebAudioInstrumenter = {
  







  handle: function(window, observer) {
    let self = this;

    let AudioContext = unwrap(window.AudioContext);
    let AudioNode = unwrap(window.AudioNode);
    let ctxProto = AudioContext.prototype;
    let nodeProto = AudioNode.prototype;

    
    

    
    let originalConnect = nodeProto.connect;
    nodeProto.connect = function (...args) {
      let source = unwrap(this);
      let nodeOrParam = unwrap(args[0]);
      originalConnect.apply(source, args);

      
      if (nodeOrParam instanceof AudioNode)
        observer.connectNode(source, nodeOrParam);
      else
        observer.connectParam(source, nodeOrParam);
    };

    
    let originalDisconnect = nodeProto.disconnect;
    nodeProto.disconnect = function (...args) {
      let source = unwrap(this);
      originalDisconnect.apply(source, args);
      observer.disconnectNode(source);
    };


    
    
    
    
    let firstNodeCreated = false;

    
    
    NODE_CREATION_METHODS.forEach(method => {
      let originalMethod = ctxProto[method];
      ctxProto[method] = function (...args) {
        let node = originalMethod.apply(this, args);
        
        
        if (!firstNodeCreated) {
          firstNodeCreated = true;
          observer.startContext();
          observer.createNode(node.context.destination);
        }
        observer.createNode(node);
        return node;
      };
    });
  }
};





function WebAudioObserver () {}

WebAudioObserver.prototype = {
  startContext: function () {
    emit(this, "start-context");
  },

  connectNode: function (source, dest) {
    emit(this, "connect-node", source, dest);
  },

  connectParam: function (source, param) {
    emit(this, "connect-param", source, param);
  },

  disconnectNode: function (source) {
    emit(this, "disconnect-node", source);
  },

  createNode: function (source) {
    emit(this, "create-node", source);
  },

  paramChange: function (node, param, val) {
    emit(this, "param-change", node, param, val);
  }
};

function unwrap (obj) {
  return XPCNativeWrapper.unwrap(obj);
}

let NODE_CREATION_METHODS = [
  "createBufferSource", "createMediaElementSource", "createMediaStreamSource",
  "createMediaStreamDestination", "createScriptProcessor", "createAnalyser",
  "createGain", "createDelay", "createBiquadFilter", "createWaveShaper",
  "createPanner", "createConvolver", "createChannelSplitter", "createChannelMerger",
  "createDynamicsCompressor", "createOscillator"
];
