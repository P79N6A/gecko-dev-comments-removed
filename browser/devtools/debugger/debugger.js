








































"use strict";

var gInitialized = false;
var gClient = null;
var gTabClient = null;


function initDebugger()
{
  window.removeEventListener("DOMContentLoaded", initDebugger, false);
  if (gInitialized) {
    return;
  }
  gInitialized = true;

  DebuggerView.Stackframes.initialize();
  DebuggerView.Properties.initialize();
  DebuggerView.Scripts.initialize();
}









function startDebuggingTab(aClient, aTabGrip)
{
  gClient = aClient;

  gClient.attachTab(aTabGrip.actor, function(aResponse, aTabClient) {
    if (aTabClient) {
      gTabClient = aTabClient;
      gClient.attachThread(aResponse.threadActor, function(aResponse, aThreadClient) {
        if (!aThreadClient) {
          dump("Couldn't attach to thread: "+aResponse.error+"\n");
          return;
        }
        ThreadState.connect(aThreadClient, function() {
          StackFrames.connect(aThreadClient, function() {
            SourceScripts.connect(aThreadClient, function() {
              aThreadClient.resume();
            });
          });
        });
      });
    }
  });
}

function shutdownDebugger()
{
  window.removeEventListener("unload", shutdownDebugger, false);

  SourceScripts.disconnect();
  StackFrames.disconnect();
  ThreadState.disconnect();
  ThreadState.activeThread = false;

  DebuggerView.Stackframes.destroy();
  DebuggerView.Properties.destroy();
  DebuggerView.Scripts.destroy();
}






var ThreadState = {
  activeThread: null,

  






  connect: function TS_connect(aThreadClient, aCallback) {
    this.activeThread = aThreadClient;
    aThreadClient.addListener("paused", ThreadState.update);
    aThreadClient.addListener("resumed", ThreadState.update);
    aThreadClient.addListener("detached", ThreadState.update);
    this.update();
    aCallback && aCallback();
  },

  


  update: function TS_update(aEvent) {
    DebuggerView.Stackframes.updateState(this.activeThread.state);
    if (aEvent == "detached") {
      ThreadState.activeThread = false;
    }
  },

  


  disconnect: function TS_disconnect() {
    this.activeThread.removeListener("paused", ThreadState.update);
    this.activeThread.removeListener("resumed", ThreadState.update);
    this.activeThread.removeListener("detached", ThreadState.update);
  }
};

ThreadState.update = ThreadState.update.bind(ThreadState);





var StackFrames = {
  pageSize: 25,
  activeThread: null,
  selectedFrame: null,

  






  connect: function SF_connect(aThreadClient, aCallback) {
    DebuggerView.Stackframes.addClickListener(this.onClick);

    this.activeThread = aThreadClient;
    aThreadClient.addListener("paused", this.onPaused);
    aThreadClient.addListener("framesadded", this.onFrames);
    aThreadClient.addListener("framescleared", this.onFramesCleared);
    this.onFramesCleared();
    aCallback && aCallback();
  },

  


  disconnect: function TS_disconnect() {
    this.activeThread.removeListener("paused", this.onPaused);
    this.activeThread.removeListener("framesadded", this.onFrames);
    this.activeThread.removeListener("framescleared", this.onFramesCleared);
  },

  


  onPaused: function SF_onPaused() {
    this.activeThread.fillFrames(this.pageSize);
  },

  


  onFrames: function SF_onFrames() {
    DebuggerView.Stackframes.empty();

    for each (let frame in this.activeThread.cachedFrames) {
      this._addFramePanel(frame);
    }

    if (this.activeThread.moreFrames) {
      DebuggerView.Stackframes.dirty = true;
    }

    if (!this.selectedFrame) {
      this.selectFrame(0);
    }
  },

  


  onFramesCleared: function SF_onFramesCleared() {
    DebuggerView.Stackframes.emptyText();
    this.selectedFrame = null;
    
    DebuggerView.Properties.localScope.empty();
    DebuggerView.Properties.globalScope.empty();
  },

  


  onClick: function SF_onClick(aEvent) {
    let target = aEvent.target;
    while (target) {
      if (target.stackFrame) {
        this.selectFrame(target.stackFrame.depth);
        return;
      }
      target = target.parentNode;
    }
  },

  






  selectFrame: function SF_selectFrame(aDepth) {
    if (this.selectedFrame !== null) {
      DebuggerView.Stackframes.highlightFrame(this.selectedFrame, false);
    }

    this.selectedFrame = aDepth;
    if (aDepth !== null) {
      DebuggerView.Stackframes.highlightFrame(this.selectedFrame, true);
    }

    
    let frame = this.activeThread.cachedFrames[aDepth];
    if (!frame) {
      return;
    }
    let localScope = DebuggerView.Properties.localScope;
    localScope.empty();
    
    if (frame["this"]) {
      let thisVar = localScope.addVar("this");
      thisVar.setGrip({ "type": frame["this"].type,
                        "class": frame["this"].class });
      this._addExpander(thisVar, frame["this"]);
    }

    if (frame.arguments && frame.arguments.length > 0) {
      
      let argsVar = localScope.addVar("arguments");
      argsVar.setGrip({ "type": "object", "class": "Arguments" });
      this._addExpander(argsVar, frame.arguments);

      
      let objClient = this.activeThread.pauseGrip(frame.callee);
      objClient.getSignature(function SF_getSignature(aResponse) {
        for (let i = 0; i < aResponse.parameters.length; i++) {
          let param = aResponse.parameters[i];
          let paramVar = localScope.addVar(param);
          let paramVal = frame.arguments[i];
          paramVar.setGrip(paramVal);
          this._addExpander(paramVar, paramVal);
        }
      }.bind(this));
    }
  },

  _addExpander: function SF_addExpander(aVar, aObject) {
    
    
    if (!aObject || typeof aObject != "object" ||
        (aObject.type != "object" && !Array.isArray(aObject))) {
      return;
    }
    
    aVar.addProperties({ " ": { value: " " }});
    aVar.onexpand = this._addVarProperties.bind(this, aVar, aObject);
  },

  _addVarProperties: function SF_addVarProperties(aVar, aObject) {
    
    if (aVar.fetched) {
      return;
    }
    
    aVar.empty();

    
    
    if (Array.isArray(aObject)) {
      let properties = { length: { writable: true, value: aObject.length } };
      for (let i = 0; i < aObject.length; i++) {
        properties[i + ""] = { value: aObject[i] };
      }
      aVar.addProperties(properties);
      
      for (let i = 0; i < aObject.length; i++) {
        this._addExpander(aVar[i + ""], aObject[i]);
      }
      aVar.fetched = true;
      return;
    }

    let objClient = this.activeThread.pauseGrip(aObject);
    objClient.getPrototypeAndProperties(function SF_onProtoAndProps(aResponse) {
      
      if (aResponse.prototype.type != "null") {
        let properties = {};
        properties["__proto__ "] = { value: aResponse.prototype };
        aVar.addProperties(properties);
        
        this._addExpander(aVar["__proto__ "], aResponse.prototype);
      }

      
      let properties = {};
      for each (let prop in Object.keys(aResponse.ownProperties).sort()) {
        properties[prop] = aResponse.ownProperties[prop];
      }
      aVar.addProperties(properties);
      
      for (let prop in aResponse.ownProperties) {
        this._addExpander(aVar[prop], aResponse.ownProperties[prop].value);
      }

      aVar.fetched = true;
    }.bind(this));
  },

  





  _addFramePanel: function SF_addFramePanel(aFrame) {
    let depth = aFrame.depth;
    let idText = "#" + aFrame.depth + " ";
    let nameText = this._frameTitle(aFrame);

    let panel = DebuggerView.Stackframes.addFrame(depth, idText, nameText);

    if (panel) {
      panel.stackFrame = aFrame;
    }
  },

  


  _addMoreFrames: function SF_addMoreFrames() {
    this.activeThread.fillFrames(
      this.activeThread.cachedFrames.length + this.pageSize);
  },

  






  _frameTitle: function SF_frameTitle(aFrame) {
    if (aFrame.type == "call") {
      return aFrame["calleeName"] ? aFrame["calleeName"] + "()" : "(anonymous)";
    }

    return "(" + aFrame.type + ")";
  }
};

StackFrames.onPaused = StackFrames.onPaused.bind(StackFrames);
StackFrames.onFrames = StackFrames.onFrames.bind(StackFrames);
StackFrames.onFramesCleared = StackFrames.onFramesCleared.bind(StackFrames);
StackFrames.onClick = StackFrames.onClick.bind(StackFrames);





var SourceScripts = {
  pageSize: 25,
  activeThread: null,

  






  connect: function SS_connect(aThreadClient, aCallback) {
    DebuggerView.Scripts.addChangeListener(this.onChange);

    this.activeThread = aThreadClient;
    aThreadClient.addListener("paused", this.onPaused);
    aThreadClient.addListener("scriptsadded", this.onScripts);
    aThreadClient.addListener("scriptscleared", this.onScriptsCleared);
    this.onScriptsCleared();
    aCallback && aCallback();
  },

  


  disconnect: function TS_disconnect() {
    this.activeThread.removeListener("paused", this.onPaused);
    this.activeThread.removeListener("scriptsadded", this.onScripts);
    this.activeThread.removeListener("scriptscleared", this.onScriptsCleared);
  },

  




  onPaused: function SS_onPaused() {
    this.activeThread.removeListener("paused", this.onPaused);
    this.activeThread.fillScripts();
  },

  


  onNewScript: function SS_onNewScript(aNotification, aPacket) {
    this._addScript({ url: aPacket.url, startLine: aPacket.startLine });
  },

  


  onScripts: function SS_onScripts() {
    this.onScriptsCleared();
    for each (let script in this.activeThread.cachedScripts) {
      this._addScript(script);
    }
  },

  


  onScriptsCleared: function SS_onScriptsCleared() {
    DebuggerView.Scripts.empty();
  },

  


  onChange: function SS_onClick(aEvent) {
    let scripts = aEvent.target;
    let script = scripts.selectedItem.getUserData("sourceScript");
    this._showScript(script);
  },

  



  _addScript: function SS_addScript(aScript) {
    DebuggerView.Scripts.addScript(aScript.url, aScript);

    if (window.editor.getCharCount() == 0) {
      this._showScript(aScript);
    }
  },

  



  _showScript: function SS_showScript(aScript) {
    if (!aScript.loaded) {
      
      var evt = document.createEvent("CustomEvent");
      evt.initCustomEvent("Debugger:LoadSource", true, false, aScript.url);
      document.documentElement.dispatchEvent(evt);
      window.editor.setText(DebuggerView.getStr("loadingText"));
    } else {
      window.editor.setText(aScript.text);
    }
  }
};

SourceScripts.onPaused = SourceScripts.onPaused.bind(SourceScripts);
SourceScripts.onScripts = SourceScripts.onScripts.bind(SourceScripts);
SourceScripts.onNewScript = SourceScripts.onNewScript.bind(SourceScripts);
SourceScripts.onScriptsCleared = SourceScripts.onScriptsCleared.bind(SourceScripts);
SourceScripts.onChange = SourceScripts.onChange.bind(SourceScripts);

window.addEventListener("DOMContentLoaded", initDebugger, false);
window.addEventListener("unload", shutdownDebugger, false);
