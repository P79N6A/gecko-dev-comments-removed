



this.EXPORTED_SYMBOLS = [
  "FrameManager"
];

let FRAME_SCRIPT = "chrome://marionette/content/marionette-listener.js";
Cu.import("resource://gre/modules/Services.jsm");





function MarionetteRemoteFrame(windowId, frameId) {
  this.windowId = windowId; 
  this.frameId = frameId ? frameId : null; 
  this.targetFrameId = this.frameId; 
  this.messageManager = null;
};








this.FrameManager = function FrameManager(server) {
  
  this.messageManager = Cc["@mozilla.org/globalmessagemanager;1"]
                             .getService(Ci.nsIMessageBroadcaster);
  this.currentRemoteFrame = null; 
  this.previousRemoteFrame = null; 
  this.handledModal = false; 
  this.remoteFrames = []; 
  this.server = server; 
};

FrameManager.prototype = {
  


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
          this.removeMessageManagerListeners(this.currentRemoteFrame.messageManager);
          
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

  
  switchToRemoteFrame: function FM_switchToRemoteFrame(message) {
    
    let frameWindow = Services.wm.getOuterWindowWithId(message.json.win); 
    let oopFrame = frameWindow.document.getElementsByTagName("iframe")[message.json.frame]; 
    let mm = oopFrame.QueryInterface(Ci.nsIFrameLoaderOwner).frameLoader.messageManager; 

    
    
    for (let i = 0; i < this.remoteFrames.length; i++) {
      let frame = this.remoteFrames[i];
      if (frame.messageManager == mm) {
        this.currentRemoteFrame = frame;
        mm = frame.messageManager;
        this.addMessageManagerListeners(mm);
        mm.sendAsyncMessage("Marionette:restart", {});
        return;
      }
    }

    
    
    this.addMessageManagerListeners(mm);
    mm.loadFrameScript(FRAME_SCRIPT, true);
    let aFrame = new MarionetteRemoteFrame(message.json.win, message.json.frame);
    aFrame.messageManager = mm;
    this.remoteFrames.push(aFrame);
    this.currentRemoteFrame = aFrame;
  },

  



  switchToModalOrigin: function FM_switchToModalOrigin() {
    
    if (this.previousRemoteFrame != null) {
      this.currentRemoteFrame = this.previousRemoteFrame;
      this.addMessageManagerListeners(this.currentRemoteFrame.messageManager);
    }
    this.handledModal = false;
  },

  








  addMessageManagerListeners: function MDA_addMessageManagerListeners(messageManager) {
    messageManager.addMessageListener("Marionette:ok", this.server);
    messageManager.addMessageListener("Marionette:done", this.server);
    messageManager.addMessageListener("Marionette:error", this.server);
    messageManager.addMessageListener("Marionette:log", this.server);
    messageManager.addMessageListener("Marionette:shareData", this.server);
    messageManager.addMessageListener("Marionette:register", this.server);
    messageManager.addMessageListener("Marionette:runEmulatorCmd", this.server);
    messageManager.addMessageListener("Marionette:switchToModalOrigin", this.server);
    messageManager.addMessageListener("Marionette:switchToFrame", this.server);
    messageManager.addMessageListener("Marionette:switchedToFrame", this.server);
    messageManager.addMessageListener("MarionetteFrame:handleModal", this);
    messageManager.addMessageListener("MarionetteFrame:getInterruptedState", this);
  },

  











  removeMessageManagerListeners: function MDA_removeMessageManagerListeners(messageManager) {
    messageManager.removeMessageListener("Marionette:ok", this.server);
    messageManager.removeMessageListener("Marionette:done", this.server);
    messageManager.removeMessageListener("Marionette:error", this.server);
    messageManager.removeMessageListener("Marionette:log", this.server);
    messageManager.removeMessageListener("Marionette:shareData", this.server);
    messageManager.removeMessageListener("Marionette:register", this.server);
    messageManager.removeMessageListener("Marionette:runEmulatorCmd", this.server);
    messageManager.removeMessageListener("Marionette:switchToFrame", this.server);
    messageManager.removeMessageListener("Marionette:switchedToFrame", this.server);
    messageManager.removeMessageListener("MarionetteFrame:handleModal", this);
  },

};
