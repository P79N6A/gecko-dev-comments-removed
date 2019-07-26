



"use strict";

this.EXPORTED_SYMBOLS = [ "SharedFrame" ];

const Ci = Components.interfaces;
const Cu = Components.utils;










let Frames = new Map();


























function UNLOADED_URL(aStr) "data:text/html;charset=utf-8,<!-- Unloaded frame " + aStr + " -->";


this.SharedFrame = {
  















  createFrame: function (aGroupName, aParent, aFrameAttributes, aPreload = true) {
    let frame = aParent.ownerDocument.createElement("iframe");

    for (let [key, val] of Iterator(aFrameAttributes)) {
      frame.setAttribute(key, val);
    }

    let src = aFrameAttributes.src;
    if (!src) {
      aPreload = false;
    }

    let group = Frames.get(aGroupName);

    if (group) {
      

      if (aPreload && !group.isAlive) {
        
        
        
        
        group.url = src;
        this.preload(aGroupName, frame);
      } else {
        
        
        
        frame.setAttribute("src", UNLOADED_URL(aGroupName));
      }

    } else {
      
      
      group = new _SharedFrameGroup(src);
      Frames.set(aGroupName, group);

      if (aPreload) {
        this.preload(aGroupName, frame);
      } else {
        frame.setAttribute("src", UNLOADED_URL(aGroupName));
      }
    }

    aParent.appendChild(frame);
    return frame;

  },

  








  setOwner: function (aGroupName, aTargetFrame) {
    let group = Frames.get(aGroupName);
    let frame = group.activeFrame;

    if (frame == aTargetFrame) {
      
      return;
    }

    if (group.isAlive) {
      
      frame.QueryInterface(Ci.nsIFrameLoaderOwner).swapFrameLoaders(aTargetFrame);
      group.activeFrame = aTargetFrame;
    } else {
      
      aTargetFrame.setAttribute("src", group.url);
      group.activeFrame = aTargetFrame;
    }
  },

  





  updateURL: function (aGroupName, aURL) {
    let group = Frames.get(aGroupName);
    group.url = aURL;

    if (group.isAlive) {
      group.activeFrame.setAttribute("src", aURL);
    }
  },

  







  preload: function (aGroupName, aTargetFrame) {
    let group = Frames.get(aGroupName);
    if (!group.isAlive) {
      aTargetFrame.setAttribute("src", group.url);
      group.activeFrame = aTargetFrame;
    }
  },

  




  isGroupAlive: function (aGroupName) {
    return Frames.get(aGroupName).isAlive;
  },

  





  forgetGroup: function (aGroupName) {
    Frames.delete(aGroupName);
  }
}


function _SharedFrameGroup(aURL) {
  this.url = aURL;
  this._activeFrame = null;
}

_SharedFrameGroup.prototype = {
  get isAlive() {
    let frame = this.activeFrame;
    return !!(frame &&
              frame.contentDocument &&
              frame.contentDocument.location);
  },

  get activeFrame() {
    return this._activeFrame &&
           this._activeFrame.get();
  },

  set activeFrame(aActiveFrame) {
    this._activeFrame = aActiveFrame
                        ? Cu.getWeakReference(aActiveFrame)
                        : null;
  }
}
