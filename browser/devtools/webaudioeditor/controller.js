







let gAudioNodes = new AudioNodesCollection();




function startupWebAudioEditor() {
  return all([
    WebAudioEditorController.initialize(),
    ContextView.initialize(),
    InspectorView.initialize(),
    PropertiesView.initialize(),
    AutomationView.initialize()
  ]);
}




function shutdownWebAudioEditor() {
  return all([
    WebAudioEditorController.destroy(),
    ContextView.destroy(),
    InspectorView.destroy(),
    PropertiesView.destroy(),
    AutomationView.destroy()
  ]);
}




let WebAudioEditorController = {
  


  initialize: Task.async(function* () {
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

    
    
    
    let actorHasDefinition = yield gTarget.actorHasMethod("webaudio", "getDefinition");
    if (actorHasDefinition) {
      AUDIO_NODE_DEFINITION = yield gFront.getDefinition();
    } else {
      AUDIO_NODE_DEFINITION = require("devtools/server/actors/utils/audionodes.json");
    }
  }),

  


  destroy: function() {
    gTarget.off("will-navigate", this._onTabNavigated);
    gTarget.off("navigate", this._onTabNavigated);
    gFront.off("start-context", this._onStartContext);
    gFront.off("create-node", this._onCreateNode);
    gFront.off("connect-node", this._onConnectNode);
    gFront.off("connect-param", this._onConnectParam);
    gFront.off("disconnect-node", this._onDisconnectNode);
    gFront.off("change-param", this._onChangeParam);
    gFront.off("destroy-node", this._onDestroyNode);
    gDevTools.off("pref-changed", this._onThemeChange);
  },

  



  reset: function () {
    $("#content").hidden = true;
    ContextView.resetUI();
    InspectorView.resetUI();
    PropertiesView.resetUI();
  },

  
  
  
  getNode: function* (nodeActor) {
    let id = nodeActor.actorID;
    let node = gAudioNodes.get(id);

    if (!node) {
      let { resolve, promise } = defer();
      gAudioNodes.on("add", function createNodeListener (createdNode) {
        if (createdNode.id === id) {
          gAudioNodes.off("add", createNodeListener);
          resolve(createdNode);
        }
      });
      node = yield promise;
    }
    return node;
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

        
        gAudioNodes.reset();

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
    yield gAudioNodes.add(nodeActor);
  }),

  



  _onDestroyNode: function (nodeActor) {
    gAudioNodes.remove(gAudioNodes.get(nodeActor.actorID));
  },

  


  _onConnectNode: Task.async(function* ({ source: sourceActor, dest: destActor }) {
    let source = yield WebAudioEditorController.getNode(sourceActor);
    let dest = yield WebAudioEditorController.getNode(destActor);
    source.connect(dest);
  }),

  


  _onConnectParam: Task.async(function* ({ source: sourceActor, dest: destActor, param }) {
    let source = yield WebAudioEditorController.getNode(sourceActor);
    let dest = yield WebAudioEditorController.getNode(destActor);
    source.connect(dest, param);
  }),

  


  _onDisconnectNode: Task.async(function* (nodeActor) {
    let node = yield WebAudioEditorController.getNode(nodeActor);
    node.disconnect();
  }),

  


  _onChangeParam: Task.async(function* ({ actor, param, value }) {
    let node = yield WebAudioEditorController.getNode(actor);
    window.emit(EVENTS.CHANGE_PARAM, node, param, value);
  })
};
