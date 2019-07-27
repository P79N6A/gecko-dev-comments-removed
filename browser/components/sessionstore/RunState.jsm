



"use strict";

this.EXPORTED_SYMBOLS = ["RunState"];

const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm", this);

const STATE_STOPPED = 0;
const STATE_RUNNING = 1;
const STATE_QUITTING = 2;
const STATE_CLOSING = 3;
const STATE_CLOSED = 4;


let state = STATE_STOPPED;

function observer(subj, topic) {
  Services.obs.removeObserver(observer, topic);
  state = STATE_QUITTING;
}


Services.obs.addObserver(observer, "quit-application-granted", false);







this.RunState = Object.freeze({
  
  
  
  get isStopped() {
    return state == STATE_STOPPED;
  },

  
  
  
  get isRunning() {
    return state == STATE_RUNNING;
  },

  
  
  
  
  
  get isQuitting() {
    return state >= STATE_QUITTING;
  },

  
  
  
  get isClosing() {
    return state == STATE_CLOSING;
  },

  
  
  get isClosed() {
    return state == STATE_CLOSED;
  },

  
  
  
  setRunning() {
    if (this.isStopped) {
      state = STATE_RUNNING;
    }
  },

  
  
  
  setClosing() {
    if (this.isQuitting) {
      state = STATE_CLOSING;
    }
  },

  
  
  
  setClosed() {
    if (this.isClosing) {
      state = STATE_CLOSED;
    }
  }
});
