



"use strict";

this.EXPORTED_SYMBOLS = ["GlobalState"];

const EXPORTED_METHODS = ["getState", "clear", "get", "set", "delete", "setFromState"];



function GlobalState() {
  let internal = new GlobalStateInternal();
  let external = {};
  for (let method of EXPORTED_METHODS) {
    external[method] = internal[method].bind(internal);
  }
  return Object.freeze(external);
}

function GlobalStateInternal() {
  
  this.state = {};
}

GlobalStateInternal.prototype = {
  


  getState: function() {
    return this.state;
  },

  


  clear: function() {
    this.state = {};
  },

  






  get: function(aKey) {
    return this.state[aKey] || "";
  },

  





  set: function(aKey, aStringValue) {
    this.state[aKey] = aStringValue;
  },

  





  delete: function(aKey) {
    delete this.state[aKey];
  },

  







  setFromState: function (aState) {
    this.state = (aState && aState.global) || {};
  }
};
