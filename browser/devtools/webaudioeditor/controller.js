







let gAudioNodes = new AudioNodesCollection();




function startupWebAudioEditor() {
  return all([
    WebAudioEditorController.initialize(),
    ContextView.initialize(),
    InspectorView.initialize()
  ]);
}




function shutdownWebAudioEditor() {
  return all([
    WebAudioEditorController.destroy(),
    ContextView.destroy(),
    InspectorView.destroy(),
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
    gDevTools.off("pref-changed", this._onThemeChange);
  },

  



  reset: function () {
    $("#content").hidden = true;
    ContextView.resetUI();
    InspectorView.resetUI();
  },

  
  
  
  
  
  _waitForNodeCreation: function (sourceActor, destActor) {
    let deferred = defer();
    let source = gAudioNodes.get(sourceActor.actorID);
    let dest = gAudioNodes.get(destActor.actorID);

    if (!source || !dest) {
      gAudioNodes.on("add", function createNodeListener (createdNode) {
        if (sourceActor.actorID === createdNode.id)
          source = createdNode;
        if (destActor.actorID === createdNode.id)
          dest = createdNode;
        if (source && dest) {
          gAudioNodes.off("add", createNodeListener);
          deferred.resolve([source, dest]);
        }
      });
    }
    else {
      deferred.resolve([source, dest]);
    }
    return deferred.promise;
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
    let [source, dest] = yield WebAudioEditorController._waitForNodeCreation(sourceActor, destActor);
    source.connect(dest);
  }),

  


  _onConnectParam: Task.async(function* ({ source: sourceActor, dest: destActor, param }) {
    let [source, dest] = yield WebAudioEditorController._waitForNodeCreation(sourceActor, destActor);
    source.connect(dest, param);
  }),

  


  _onDisconnectNode: function(nodeActor) {
    let node = gAudioNodes.get(nodeActor.actorID);
    node.disconnect();
  },

  


  _onChangeParam: function({ actor, param, value }) {
    window.emit(EVENTS.CHANGE_PARAM, gAudioNodes.get(actor.actorID), param, value);
  }
};
