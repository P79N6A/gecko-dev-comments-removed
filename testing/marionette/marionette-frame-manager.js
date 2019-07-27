



let {classes: Cc, interfaces: Ci, utils: Cu} = Components;

this.EXPORTED_SYMBOLS = ["FrameManager"];

let FRAME_SCRIPT = "chrome://marionette/content/marionette-listener.js";

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

let loader = Cc["@mozilla.org/moz/jssubscript-loader;1"]
               .getService(Ci.mozIJSSubScriptLoader);
let specialpowers = {};


let remoteFrames = [];





function MarionetteRemoteFrame(windowId, frameId) {
  this.windowId = windowId; 
  this.frameId = frameId; 
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
      case "MarionetteFrame:getCurrentFrameId":
        if (this.currentRemoteFrame != null) {
          return this.currentRemoteFrame.frameId;
        }
    }
  },

  getOopFrame: function FM_getOopFrame(winId, frameId) {
    
    let outerWin = Services.wm.getOuterWindowWithId(winId);
    
    let f = outerWin.document.getElementsByTagName("iframe")[frameId];
    return f;
  },

  getFrameMM: function FM_getFrameMM(winId, frameId) {
    let oopFrame = this.getOopFrame(winId, frameId);
    let mm = oopFrame.QueryInterface(Ci.nsIFrameLoaderOwner)
        .frameLoader.messageManager;
    return mm;
  },

  



  switchToFrame: function FM_switchToFrame(winId, frameId) {
    let oopFrame = this.getOopFrame(winId, frameId);
    let mm = this.getFrameMM(winId, frameId);

    if (!specialpowers.hasOwnProperty("specialPowersObserver")) {
      loader.loadSubScript("chrome://specialpowers/content/SpecialPowersObserver.js",
          specialpowers);
    }

    
    
    for (let i = 0; i < remoteFrames.length; i++) {
      let frame = remoteFrames[i];
      let frameMessageManager = frame.messageManager.get();
      try {
        frameMessageManager.sendAsyncMessage("aliveCheck", {});
      } catch (e) {
        if (e.result ==  Components.results.NS_ERROR_NOT_INITIALIZED) {
          remoteFrames.splice(i, 1);
          continue;
        }
      }
      if (frameMessageManager == mm) {
        this.currentRemoteFrame = frame;
        this.addMessageManagerListeners(mm);

        if (!frame.specialPowersObserver) {
          frame.specialPowersObserver = new specialpowers.SpecialPowersObserver();
          frame.specialPowersObserver.init(mm);
        }

        mm.sendAsyncMessage("Marionette:restart");
        return oopFrame.id;
      }
    }

    
    
    
    this.addMessageManagerListeners(mm);
    let aFrame = new MarionetteRemoteFrame(winId, frameId);
    aFrame.messageManager = Cu.getWeakReference(mm);
    remoteFrames.push(aFrame);
    this.currentRemoteFrame = aFrame;

    mm.loadFrameScript(FRAME_SCRIPT, true, true);

    aFrame.specialPowersObserver = new specialpowers.SpecialPowersObserver();
    aFrame.specialPowersObserver.init(mm);

    return oopFrame.id;
  },

  



  switchToModalOrigin: function FM_switchToModalOrigin() {
    
    if (this.previousRemoteFrame != null) {
      this.currentRemoteFrame = this.previousRemoteFrame;
      this.addMessageManagerListeners(this.currentRemoteFrame.messageManager.get());
    }
    this.handledModal = false;
  },

  


  removeSpecialPowers: function FM_removeSpecialPowers() {
    for (let i = 0; i < remoteFrames.length; i++) {
      let frame = remoteFrames[i];

      if (frame.specialPowersObserver) {
        frame.specialPowersObserver.uninit();
        frame.specialPowersObserver = null;
      }
    }
  },

  










  addMessageManagerListeners: function FM_addMessageManagerListeners(mm) {
    mm.addWeakMessageListener("Marionette:emitTouchEvent", this.server);
    mm.addWeakMessageListener("Marionette:log", this.server);
    mm.addWeakMessageListener("Marionette:runEmulatorCmd", this.server);
    mm.addWeakMessageListener("Marionette:runEmulatorShell", this.server);
    mm.addWeakMessageListener("Marionette:shareData", this.server);
    mm.addWeakMessageListener("Marionette:switchToModalOrigin", this.server);
    mm.addWeakMessageListener("Marionette:switchedToFrame", this.server);
    mm.addWeakMessageListener("Marionette:addCookie", this.server);
    mm.addWeakMessageListener("Marionette:getVisibleCookies", this.server);
    mm.addWeakMessageListener("Marionette:deleteCookie", this.server);
    mm.addWeakMessageListener("Marionette:register", this.server);
    mm.addWeakMessageListener("Marionette:listenersAttached", this.server);
    mm.addWeakMessageListener("MarionetteFrame:handleModal", this);
    mm.addWeakMessageListener("MarionetteFrame:getCurrentFrameId", this);
    mm.addWeakMessageListener("MarionetteFrame:getInterruptedState", this);
  },

  











  removeMessageManagerListeners: function FM_removeMessageManagerListeners(mm) {
    mm.removeWeakMessageListener("Marionette:log", this.server);
    mm.removeWeakMessageListener("Marionette:shareData", this.server);
    mm.removeWeakMessageListener("Marionette:runEmulatorCmd", this.server);
    mm.removeWeakMessageListener("Marionette:runEmulatorShell", this.server);
    mm.removeWeakMessageListener("Marionette:switchedToFrame", this.server);
    mm.removeWeakMessageListener("Marionette:addCookie", this.server);
    mm.removeWeakMessageListener("Marionette:getVisibleCookies", this.server);
    mm.removeWeakMessageListener("Marionette:deleteCookie", this.server);
    mm.removeWeakMessageListener("Marionette:listenersAttached", this.server);
    mm.removeWeakMessageListener("Marionette:register", this.server);
    mm.removeWeakMessageListener("MarionetteFrame:handleModal", this);
    mm.removeWeakMessageListener("MarionetteFrame:getCurrentFrameId", this);
  }
};
