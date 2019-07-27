


"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource:///modules/devtools/ViewHelpers.jsm");
Cu.import("resource:///modules/devtools/gDevTools.jsm");


const { defer, all } = Cu.import("resource://gre/modules/Promise.jsm", {}).Promise;

const { Task } = Cu.import("resource://gre/modules/Task.jsm", {});
const require = Cu.import("resource://gre/modules/devtools/Loader.jsm", {}).devtools.require;
const EventEmitter = require("devtools/toolkit/event-emitter");
const STRINGS_URI = "chrome://browser/locale/devtools/webaudioeditor.properties"
const L10N = new ViewHelpers.L10N(STRINGS_URI);
const Telemetry = require("devtools/shared/telemetry");
const telemetry = new Telemetry();

let { console } = Cu.import("resource://gre/modules/devtools/Console.jsm", {});


const EVENTS = {
  
  
  START_CONTEXT: "WebAudioEditor:StartContext",

  
  CREATE_NODE: "WebAudioEditor:CreateNode",
  CONNECT_NODE: "WebAudioEditor:ConnectNode",
  DISCONNECT_NODE: "WebAudioEditor:DisconnectNode",

  
  DESTROY_NODE: "WebAudioEditor:DestroyNode",

  
  CHANGE_PARAM: "WebAudioEditor:ChangeParam",

  
  THEME_CHANGE: "WebAudioEditor:ThemeChange",

  
  UI_RESET: "WebAudioEditor:UIReset",

  
  
  UI_SET_PARAM: "WebAudioEditor:UISetParam",

  
  UI_SELECT_NODE: "WebAudioEditor:UISelectNode",

  
  UI_INSPECTOR_NODE_SET: "WebAudioEditor:UIInspectorNodeSet",

  
  UI_INSPECTOR_TOGGLED: "WebAudioEditor:UIInspectorToggled",

  
  UI_PROPERTIES_TAB_RENDERED: "WebAudioEditor:UIPropertiesTabRendered",

  
  
  
  
  UI_GRAPH_RENDERED: "WebAudioEditor:UIGraphRendered"
};




let gToolbox, gTarget, gFront;




let AudioNodes = [];
let AudioNodeConnections = new WeakMap(); 
let AudioParamConnections = new WeakMap(); 


function AudioNodeView (actor) {
  this.actor = actor;
  this.id = actor.actorID;
}



AudioNodeView.prototype.getType = Task.async(function* () {
  this.type = yield this.actor.getType();
  return this.type;
});





AudioNodeView.prototype.connect = function (destination) {
  let connections = AudioNodeConnections.get(this) || new Set();
  AudioNodeConnections.set(this, connections);

  
  if (!connections.has(destination)) {
    connections.add(destination);
    return true;
  }
  return false;
};





AudioNodeView.prototype.connectParam = function (destination, param) {
  let connections = AudioParamConnections.get(this) || {};
  AudioParamConnections.set(this, connections);

  let params = connections[destination.id] = connections[destination.id] || [];

  if (!~params.indexOf(param)) {
    params.push(param);
    return true;
  }
  return false;
};


AudioNodeView.prototype.disconnect = function () {
  AudioNodeConnections.set(this, new Set());
  AudioParamConnections.set(this, {});
};



AudioNodeView.prototype.getParams = function () {
  return this.actor.getParams();
};





function startupWebAudioEditor() {
  return all([
    WebAudioEditorController.initialize(),
    WebAudioGraphView.initialize(),
    WebAudioInspectorView.initialize(),
  ]);
}




function shutdownWebAudioEditor() {
  return all([
    WebAudioEditorController.destroy(),
    WebAudioGraphView.destroy(),
    WebAudioInspectorView.destroy(),
  ]);
}




let WebAudioEditorController = {
  


  initialize: function() {
    telemetry.toolOpened("webaudioeditor");
    this._onTabNavigated = this._onTabNavigated.bind(this);
    this._onThemeChange = this._onThemeChange.bind(this);
    gTarget.on("will-navigate", this._onTabNavigated);
    gTarget.on("navigate", this._onTabNavigated);
    gFront.on("start-context", this._onStartContext);
    gFront.on("create-node", this._onCreateNode);
    gFront.on("connect-node", this._onConnectNode);
    gFront.on("connect-param", this._onConnectParam);
    gFront.on("disconnect-node", this._onDisconnectNode);
    gFront.on("change-param", this._onChangeParam);
    gFront.on("destroy-node", this._onDestroyNode);

    
    
    
    gDevTools.on("pref-changed", this._onThemeChange);

    
    window.on(EVENTS.CREATE_NODE, this._onUpdatedContext);
    window.on(EVENTS.CONNECT_NODE, this._onUpdatedContext);
    window.on(EVENTS.DISCONNECT_NODE, this._onUpdatedContext);
    window.on(EVENTS.DESTROY_NODE, this._onUpdatedContext);
    window.on(EVENTS.CONNECT_PARAM, this._onUpdatedContext);
  },

  


  destroy: function() {
    telemetry.toolClosed("webaudioeditor");
    gTarget.off("will-navigate", this._onTabNavigated);
    gTarget.off("navigate", this._onTabNavigated);
    gFront.off("start-context", this._onStartContext);
    gFront.off("create-node", this._onCreateNode);
    gFront.off("connect-node", this._onConnectNode);
    gFront.off("connect-param", this._onConnectParam);
    gFront.off("disconnect-node", this._onDisconnectNode);
    gFront.off("change-param", this._onChangeParam);
    gFront.off("destroy-node", this._onDestroyNode);
    window.off(EVENTS.CREATE_NODE, this._onUpdatedContext);
    window.off(EVENTS.CONNECT_NODE, this._onUpdatedContext);
    window.off(EVENTS.DISCONNECT_NODE, this._onUpdatedContext);
    window.off(EVENTS.DESTROY_NODE, this._onUpdatedContext);
    window.off(EVENTS.CONNECT_PARAM, this._onUpdatedContext);
    gDevTools.off("pref-changed", this._onThemeChange);
  },

  



  reset: function () {
    $("#content").hidden = true;
    WebAudioGraphView.resetUI();
    WebAudioInspectorView.resetUI();
  },

  



  _onUpdatedContext: function () {
    WebAudioGraphView.draw();
  },

  




  _onThemeChange: function (event, data) {
    window.emit(EVENTS.THEME_CHANGE, data.newValue);
  },

  


  _onTabNavigated: Task.async(function* (event, {isFrameSwitching}) {
    switch (event) {
      case "will-navigate": {
        
        if (!isFrameSwitching) {
          yield gFront.setup({ reload: false });
        }

        
        this.reset();

        
        
        if (isFrameSwitching) {
          $("#reload-notice").hidden = false;
          $("#waiting-notice").hidden = true;
        } else {
          
          
          
          $("#reload-notice").hidden = true;
          $("#waiting-notice").hidden = false;
        }

        
        AudioNodes.length = 0;
        AudioNodeConnections.clear();
        window.emit(EVENTS.UI_RESET);
        break;
      }
      case "navigate": {
        
        
        break;
      }
    }
  }),

  



  _onStartContext: function() {
    $("#reload-notice").hidden = true;
    $("#waiting-notice").hidden = true;
    $("#content").hidden = false;
    window.emit(EVENTS.START_CONTEXT);
  },

  



  _onCreateNode: Task.async(function* (nodeActor) {
    let node = new AudioNodeView(nodeActor);
    yield node.getType();
    AudioNodes.push(node);
    window.emit(EVENTS.CREATE_NODE, node.id);
  }),

  



  _onDestroyNode: function (nodeActor) {
    for (let i = 0; i < AudioNodes.length; i++) {
      if (equalActors(AudioNodes[i].actor, nodeActor)) {
        AudioNodes.splice(i, 1);
        window.emit(EVENTS.DESTROY_NODE, nodeActor.actorID);
        break;
      }
    }
  },

  


  _onConnectNode: Task.async(function* ({ source: sourceActor, dest: destActor }) {
    let [source, dest] = yield waitForNodeCreation(sourceActor, destActor);

    
    if (source.connect(dest)) {
      window.emit(EVENTS.CONNECT_NODE, source.id, dest.id);
    }
  }),

  


  _onConnectParam: Task.async(function* ({ source: sourceActor, dest: destActor, param }) {
    let [source, dest] = yield waitForNodeCreation(sourceActor, destActor);

    if (source.connectParam(dest, param)) {
      window.emit(EVENTS.CONNECT_PARAM, source.id, dest.id, param);
    }
  }),

  


  _onDisconnectNode: function(nodeActor) {
    let node = getViewNodeByActor(nodeActor);
    node.disconnect();
    window.emit(EVENTS.DISCONNECT_NODE, node.id);
  },

  


  _onChangeParam: function({ actor, param, value }) {
    window.emit(EVENTS.CHANGE_PARAM, getViewNodeByActor(actor), param, value);
  }
};




EventEmitter.decorate(this);




function $(selector, target = document) { return target.querySelector(selector); }
function $$(selector, target = document) { return target.querySelectorAll(selector); }





function equalActors (actor1, actor2) {
  return actor1.actorID === actor2.actorID;
}




function getViewNodeByActor (actor) {
  for (let i = 0; i < AudioNodes.length; i++) {
    if (equalActors(AudioNodes[i].actor, actor))
      return AudioNodes[i];
  }
  return null;
}




function getViewNodeById (id) {
  return getViewNodeByActor({ actorID: id });
}






function waitForNodeCreation (sourceActor, destActor) {
  let deferred = defer();
  let eventName = EVENTS.CREATE_NODE;
  let source = getViewNodeByActor(sourceActor);
  let dest = getViewNodeByActor(destActor);

  if (!source || !dest)
    window.on(eventName, function createNodeListener (_, id) {
      let createdNode = getViewNodeById(id);
      if (equalActors(sourceActor, createdNode.actor))
        source = createdNode;
      if (equalActors(destActor, createdNode.actor))
        dest = createdNode;
      if (source && dest) {
        window.off(eventName, createNodeListener);
        deferred.resolve([source, dest]);
      }
    });
  else
    deferred.resolve([source, dest]);
  return deferred.promise;
}
