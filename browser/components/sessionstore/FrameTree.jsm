



"use strict";

this.EXPORTED_SYMBOLS = ["FrameTree"];

const Cu = Components.utils;
const Ci = Components.interfaces;

Cu.import("resource://gre/modules/XPCOMUtils.jsm", this);

const EXPORTED_METHODS = ["addObserver", "contains", "map", "forEach"];









function FrameTree(chromeGlobal) {
  let internal = new FrameTreeInternal(chromeGlobal);
  let external = {};

  for (let method of EXPORTED_METHODS) {
    external[method] = internal[method].bind(internal);
  }

  return Object.freeze(external);
}






function FrameTreeInternal(chromeGlobal) {
  
  
  
  
  
  
  
  
  
  
  
  
  
  this._frames = new WeakMap();

  
  this._observers = new Set();

  
  this._chromeGlobal = chromeGlobal;

  
  let docShell = chromeGlobal.docShell;
  let ifreq = docShell.QueryInterface(Ci.nsIInterfaceRequestor);
  let webProgress = ifreq.getInterface(Ci.nsIWebProgress);
  webProgress.addProgressListener(this, Ci.nsIWebProgress.NOTIFY_STATE_DOCUMENT);
}

FrameTreeInternal.prototype = {

  
  get content() {
    return this._chromeGlobal.content;
  },

  






  addObserver: function (obs) {
    this._observers.add(obs);
  },

  




  notifyObservers: function (method) {
    for (let obs of this._observers) {
      if (obs.hasOwnProperty(method)) {
        obs[method]();
      }
    }
  },

  






  contains: function (frame) {
    return this._frames.has(frame);
  },

  















  map: function (cb) {
    let frames = this._frames;

    function walk(frame) {
      let obj = cb(frame) || {};

      if (frames.has(frame)) {
        let children = [];

        Array.forEach(frame.frames, subframe => {
          
          
          if (!frames.has(subframe)) {
            return;
          }

          
          let index = frames.get(subframe);

          
          let result = walk(subframe, cb);
          if (result && Object.keys(result).length) {
            children[index] = result;
          }
        });

        if (children.length) {
          obj.children = children;
        }
      }

      return Object.keys(obj).length ? obj : null;
    }

    return walk(this.content);
  },

  







  forEach: function (cb) {
    let frames = this._frames;

    function walk(frame) {
      cb(frame);

      if (!frames.has(frame)) {
        return;
      }

      Array.forEach(frame.frames, subframe => {
        if (frames.has(subframe)) {
          cb(subframe);
        }
      });
    }

    walk(this.content);
  },

  






  collect: function (frame, index = 0) {
    
    this._frames.set(frame, index);

    
    Array.forEach(frame.frames, this.collect, this);
  },

  






  onStateChange: function (webProgress, request, stateFlags, status) {
    
    
    
    if (!webProgress.isTopLevel || webProgress.DOMWindow != this.content) {
      return;
    }

    if (stateFlags & Ci.nsIWebProgressListener.STATE_START) {
      
      this._frames.clear();

      
      this.notifyObservers("onFrameTreeReset");
    } else if (stateFlags & Ci.nsIWebProgressListener.STATE_STOP) {
      
      this.collect(webProgress.DOMWindow);

      
      this.notifyObservers("onFrameTreeCollected");
    }
  },

  
  onLocationChange: function () {},
  onProgressChange: function () {},
  onSecurityChange: function () {},
  onStatusChange: function () {},

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIWebProgressListener,
                                         Ci.nsISupportsWeakReference])
};
