



this.EXPORTED_SYMBOLS = [
  "FrameManager"
];

let FRAME_SCRIPT = "chrome://marionette/content/marionette-listener.js";
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

Cu.import("resource://gre/modules/Log.jsm");
let logger = Log.repository.getLogger("Marionette");


let remoteFrames = [];





function MarionetteRemoteFrame(windowId, frameId) {
  this.windowId = windowId; 
  this.frameId = frameId ? frameId : null; 
  this.targetFrameId = this.frameId; 
};








this.FrameManager = function FrameManager(server) {
  
  this.currentRemoteFrame = null; 
  this.previousRemoteFrame = null; 
  this.handledModal = false; 
  this.server = server; 
};

FrameManager.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIMessageListener,
                                         Ci.nsISupportsWeakReference]),

  


  receiveMessage: function FM_receiveMessage(message) {
    switch (message.name) {
      case "MarionetteFrame:getInterruptedState":
        
        if (this.previousRemoteFrame) {
          let interruptedFrame = Services.wm.getOuterWindowWithId(this.previousRemoteFrame.windowId);
          if (this.previousRemoteFrame.frameId != null) {
            interruptedFrame = interruptedFrame.document.getElementsByTagName("iframe")[this.previousRemoteFrame.frameId]; 
          }
          
          if (interruptedFrame.src == message.target.src) {
            return {value: this.handledModal};
          }
        }
        else if (this.currentRemoteFrame == null) {
          
          return {value: this.handledModal};
        }
        return {value: false};
      case "MarionetteFrame:handleModal":
        


        
        
        
        let isLocal = true;
        if (this.currentRemoteFrame != null) {
          isLocal = false;
          this.removeMessageManagerListeners(this.currentRemoteFrame.messageManager.get());
          
          this.previousRemoteFrame = this.currentRemoteFrame;
          
          this.currentRemoteFrame = null;
          this.server.messageManager = Cc["@mozilla.org/globalmessagemanager;1"]
                                       .getService(Ci.nsIMessageBroadcaster);
        }
        this.handledModal = true;
        this.server.sendOk(this.server.command_id);
        return {value: isLocal};
    }
  },

  
  switchToFrame: function FM_switchToFrame(message) {
    
    let frameWindow = Services.wm.getOuterWindowWithId(message.json.win); 
    let oopFrame = frameWindow.document.getElementsByTagName("iframe")[message.json.frame]; 
    let mm = oopFrame.QueryInterface(Ci.nsIFrameLoaderOwner).frameLoader.messageManager; 

    
    
    for (let i = 0; i < remoteFrames.length; i++) {
      let frame = remoteFrames[i];
      let frameMessageManager = frame.messageManager.get();
      logger.info("trying remote frame " + i);
      try {
        frameMessageManager.sendAsyncMessage("aliveCheck", {});
      }
      catch(e) {
        if (e.result ==  Components.results.NS_ERROR_NOT_INITIALIZED) {
          logger.info("deleting frame");
          remoteFrames.splice(i, 1);
          continue;
        }
      }
      if (frameMessageManager == mm) {
        this.currentRemoteFrame = frame;
        this.addMessageManagerListeners(mm);
        mm.sendAsyncMessage("Marionette:restart", {});
        return;
      }
    }

    
    
    this.addMessageManagerListeners(mm);
    logger.info("frame-manager load script: " + mm.toString());
    mm.loadFrameScript(FRAME_SCRIPT, true, true);
    let aFrame = new MarionetteRemoteFrame(message.json.win, message.json.frame);
    aFrame.messageManager = Cu.getWeakReference(mm);
    remoteFrames.push(aFrame);
    this.currentRemoteFrame = aFrame;
  },

  



  switchToModalOrigin: function FM_switchToModalOrigin() {
    
    if (this.previousRemoteFrame != null) {
      this.currentRemoteFrame = this.previousRemoteFrame;
      this.addMessageManagerListeners(this.currentRemoteFrame.messageManager.get());
    }
    this.handledModal = false;
  },

  








  addMessageManagerListeners: function MDA_addMessageManagerListeners(messageManager) {
    messageManager.addWeakMessageListener("Marionette:ok", this.server);
    messageManager.addWeakMessageListener("Marionette:done", this.server);
    messageManager.addWeakMessageListener("Marionette:error", this.server);
    messageManager.addWeakMessageListener("Marionette:log", this.server);
    messageManager.addWeakMessageListener("Marionette:shareData", this.server);
    messageManager.addWeakMessageListener("Marionette:register", this.server);
    messageManager.addWeakMessageListener("Marionette:runEmulatorCmd", this.server);
    messageManager.addWeakMessageListener("Marionette:switchToModalOrigin", this.server);
    messageManager.addWeakMessageListener("Marionette:switchToFrame", this.server);
    messageManager.addWeakMessageListener("Marionette:switchedToFrame", this.server);
    messageManager.addWeakMessageListener("MarionetteFrame:handleModal", this);
    messageManager.addWeakMessageListener("MarionetteFrame:getInterruptedState", this);
  },

  











  removeMessageManagerListeners: function MDA_removeMessageManagerListeners(messageManager) {
    messageManager.removeWeakMessageListener("Marionette:ok", this.server);
    messageManager.removeWeakMessageListener("Marionette:done", this.server);
    messageManager.removeWeakMessageListener("Marionette:error", this.server);
    messageManager.removeWeakMessageListener("Marionette:log", this.server);
    messageManager.removeWeakMessageListener("Marionette:shareData", this.server);
    messageManager.removeWeakMessageListener("Marionette:register", this.server);
    messageManager.removeWeakMessageListener("Marionette:runEmulatorCmd", this.server);
    messageManager.removeWeakMessageListener("Marionette:switchToFrame", this.server);
    messageManager.removeWeakMessageListener("Marionette:switchedToFrame", this.server);
    messageManager.removeWeakMessageListener("MarionetteFrame:handleModal", this);
  },

};
