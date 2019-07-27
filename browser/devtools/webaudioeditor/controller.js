







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
    
    
    this.queue = new Queue();

    telemetry.toolOpened("webaudioeditor");
    this._onTabNavigated = this._onTabNavigated.bind(this);
    this._onThemeChange = this._onThemeChange.bind(this);
    this._onStartContext = this._onStartContext.bind(this);

    this._onCreateNode = this.queue.addHandler(this._onCreateNode.bind(this));
    this._onConnectNode = this.queue.addHandler(this._onConnectNode.bind(this));
    this._onConnectParam = this.queue.addHandler(this._onConnectParam.bind(this));
    this._onDisconnectNode = this.queue.addHandler(this._onDisconnectNode.bind(this));
    this._onChangeParam = this.queue.addHandler(this._onChangeParam.bind(this));
    this._onDestroyNode = this.queue.addHandler(this._onDestroyNode.bind(this));

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

  


  destroy: Task.async(function*() {
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
    gDevTools.off("pref-changed", this._onThemeChange);
    yield this.queue.clear();
    this.queue = null;
  }),

  



  reset: function () {
    $("#content").hidden = true;
    ContextView.resetUI();
    InspectorView.resetUI();
    PropertiesView.resetUI();
  },

  


  getNode: function (nodeActor) {
    let id = nodeActor.actorID;
    let node = gAudioNodes.get(id);
    return node;
  },

  




  _onThemeChange: function (event, data) {
    window.emit(EVENTS.THEME_CHANGE, data.newValue);
  },

  


  _onTabNavigated: Task.async(function* (event, {isFrameSwitching}) {
    switch (event) {
      case "will-navigate": {
        yield this.queue.clear();
        gAudioNodes.reset();

        
        if (!isFrameSwitching) {
          yield gFront.setup({ reload: false });
        }

        
        yield this.reset();

        
        
        if (isFrameSwitching) {
          $("#reload-notice").hidden = false;
          $("#waiting-notice").hidden = true;
        } else {
          
          
          
          $("#reload-notice").hidden = true;
          $("#waiting-notice").hidden = false;
        }

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

  


  _onConnectNode: function ({ source: sourceActor, dest: destActor }) {
    let source = this.getNode(sourceActor);
    let dest = this.getNode(destActor);
    source.connect(dest);
  },

  


  _onConnectParam: function ({ source: sourceActor, dest: destActor, param }) {
    let source = this.getNode(sourceActor);
    let dest = this.getNode(destActor);
    source.connect(dest, param);
  },

  


  _onDisconnectNode: function (nodeActor) {
    let node = this.getNode(nodeActor);
    node.disconnect();
  },

  


  _onChangeParam: function ({ actor, param, value }) {
    let node = this.getNode(actor);
    window.emit(EVENTS.CHANGE_PARAM, node, param, value);
  }
};
