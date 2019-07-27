




"use strict";

let { interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/Services.jsm");

let systemAppOrigin = (function() {
  let systemOrigin = "_";
  try {
    systemOrigin = Services.io.newURI(
      Services.prefs.getCharPref("b2g.system_manifest_url"), null, null)
      .prePath;
  } catch(e) {
    
  }
  return systemOrigin;
})();

let threshold = 25;
try {
  threshold = Services.prefs.getIntPref("ui.dragThresholdX");
} catch(e) {
  
}

let delay = 500;
try {
  delay = Services.prefs.getIntPref("ui.click_hold_context_menus.delay");
} catch(e) {
  
}





let simulator = {
  events: [
    "mousedown",
    "mousemove",
    "mouseup",
    "touchstart",
    "touchend",
  ],

  messages: [
    "TouchEventSimulator:Start",
    "TouchEventSimulator:Stop",
  ],

  contextMenuTimeout: null,

  init() {
    this.messages.forEach(msgName => {
      addMessageListener(msgName, this);
    });
  },

  receiveMessage(msg) {
    switch (msg.name) {
      case "TouchEventSimulator:Start":
        this.start();
        break;
      case "TouchEventSimulator:Stop":
        this.stop();
        break;
    }
  },

  start() {
    this.events.forEach(evt => {
      
      
      addEventListener(evt, this, true, false);
    });
    sendAsyncMessage("TouchEventSimulator:Started");
  },

  stop() {
    this.events.forEach(evt => {
      removeEventListener(evt, this, true);
    });
    sendAsyncMessage("TouchEventSimulator:Stopped");
  },

  handleEvent(evt) {
    
    
    
    let content = this.getContent(evt.target);
    if (!content) {
      return;
    }
    let isSystemWindow = content.location.toString()
                                .startsWith(systemAppOrigin);

    
    
    if (evt.type.startsWith("touch") && !isSystemWindow) {
      let sysFrame = content.realFrameElement;
      if (!sysFrame) {
        return;
      }
      let sysDocument = sysFrame.ownerDocument;
      let sysWindow = sysDocument.defaultView;

      let touchEvent = sysDocument.createEvent("touchevent");
      let touch = evt.touches[0] || evt.changedTouches[0];
      let point = sysDocument.createTouch(sysWindow, sysFrame, 0,
                                          touch.pageX, touch.pageY,
                                          touch.screenX, touch.screenY,
                                          touch.clientX, touch.clientY,
                                          1, 1, 0, 0);

      let touches = sysDocument.createTouchList(point);
      let targetTouches = touches;
      let changedTouches = touches;
      touchEvent.initTouchEvent(evt.type, true, true, sysWindow, 0,
                                false, false, false, false,
                                touches, targetTouches, changedTouches);
      sysFrame.dispatchEvent(touchEvent);
      return;
    }

    
    
    if (evt.button ||
        evt.mozInputSource != Ci.nsIDOMMouseEvent.MOZ_SOURCE_MOUSE ||
        evt.isSynthesized) {
      return;
    }

    let eventTarget = this.target;
    let type = "";
    switch (evt.type) {
      case "mousedown":
        this.target = evt.target;

        this.contextMenuTimeout =
          this.sendContextMenu(evt.target, evt.pageX, evt.pageY);

        this.cancelClick = false;
        this.startX = evt.pageX;
        this.startY = evt.pageY;

        
        
        evt.target.setCapture(false);

        type = "touchstart";
        break;

      case "mousemove":
        if (!eventTarget) {
          return;
        }

        if (!this.cancelClick) {
          if (Math.abs(this.startX - evt.pageX) > threshold ||
              Math.abs(this.startY - evt.pageY) > threshold) {
            this.cancelClick = true;
            content.clearTimeout(this.contextMenuTimeout);
          }
        }

        type = "touchmove";
        break;

      case "mouseup":
        if (!eventTarget) {
          return;
        }
        this.target = null;

        content.clearTimeout(this.contextMenuTimeout);
        type = "touchend";

        
        
        
        if (evt.detail == 1) {
          addEventListener("click", this, true, false);
        }
        break;

      case "click":
        
        
        evt.preventDefault();
        evt.stopImmediatePropagation();

        removeEventListener("click", this, true, false);

        if (this.cancelClick) {
          return;
        }

        content.setTimeout(function dispatchMouseEvents(self) {
          try {
            self.fireMouseEvent("mousedown", evt);
            self.fireMouseEvent("mousemove", evt);
            self.fireMouseEvent("mouseup", evt);
          } catch(e) {
            Cu.reportError("Exception in touch event helper: " + e);
          }
        }, 0, this);
        return;
    }

    let target = eventTarget || this.target;
    if (target && type) {
      this.sendTouchEvent(evt, target, type);
    }

    if (!isSystemWindow) {
      evt.preventDefault();
      evt.stopImmediatePropagation();
    }
  },

  fireMouseEvent(type, evt) {
    let content = this.getContent(evt.target);
    let utils = content.QueryInterface(Ci.nsIInterfaceRequestor)
                       .getInterface(Ci.nsIDOMWindowUtils);
    utils.sendMouseEvent(type, evt.clientX, evt.clientY, 0, 1, 0, true, 0,
                         Ci.nsIDOMMouseEvent.MOZ_SOURCE_TOUCH);
  },

  sendContextMenu(target, x, y) {
    let doc = target.ownerDocument;
    let evt = doc.createEvent("MouseEvent");
    evt.initMouseEvent("contextmenu", true, true, doc.defaultView,
                       0, x, y, x, y, false, false, false, false,
                       0, null);

    let content = this.getContent(target);
    let timeout = content.setTimeout((function contextMenu() {
      target.dispatchEvent(evt);
      this.cancelClick = true;
    }).bind(this), delay);

    return timeout;
  },

  sendTouchEvent(evt, target, name) {
    function clone(obj) {
      return Cu.cloneInto(obj, target);
    }
    
    
    if (target.localName == "iframe" && target.mozbrowser === true) {
      if (name == "touchstart") {
        this.touchstartTime = Date.now();
      } else if (name == "touchend") {
        
        
        if (Date.now() - this.touchstartTime < delay) {
          this.cancelClick = true;
        }
      }
      let unwrapped = XPCNativeWrapper.unwrap(target);
      unwrapped.sendTouchEvent(name, clone([0]),       
                               clone([evt.clientX]),   
                               clone([evt.clientY]),   
                               clone([1]), clone([1]), 
                               clone([0]), clone([0]), 
                               1);                     
      return;
    }
    let document = target.ownerDocument;
    let content = this.getContent(target);
    if (!content) {
      return;
    }

    let touchEvent = document.createEvent("touchevent");
    let point = document.createTouch(content, target, 0,
                                     evt.pageX, evt.pageY,
                                     evt.screenX, evt.screenY,
                                     evt.clientX, evt.clientY,
                                     1, 1, 0, 0);
    let touches = document.createTouchList(point);
    let targetTouches = touches;
    let changedTouches = touches;
    touchEvent.initTouchEvent(name, true, true, content, 0,
                              false, false, false, false,
                              touches, targetTouches, changedTouches);
    target.dispatchEvent(touchEvent);
  },

  getContent(target) {
    let win = (target && target.ownerDocument)
      ? target.ownerDocument.defaultView
      : null;
    return win;
  }
};

simulator.init();
