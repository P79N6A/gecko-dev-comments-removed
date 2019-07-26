



"use strict";

module.metadata = {
  "stability": "experimental"
};


let { Class } = require("./heritage");
let { on, off } = require('../system/events');
let unloadSubject = require('@loader/unload');

let disposables = WeakMap();

function initialize(instance) {
  
  function handler(event) {
    if (event.subject.wrappedJSObject === unloadSubject) {
      instance.destroy();
    }
  }

  
  
  
  
  
  
  disposables.set(instance, handler);
  on("sdk:loader:destroy", handler);
}
exports.initialize = initialize;

function dispose(instance) {
  
  
  

  let handler = disposables.get(instance);
  if (handler) off("sdk:loader:destroy", handler);
  disposables.delete(instance);
}
exports.dispose = dispose;



let Disposable = Class({
  initialize: function setupDisposable() {
    
    
    
    this.setup.apply(this, arguments);
    initialize(this);
  },
  setup: function setup() {
    
  },
  dispose: function dispose() {
    
  },

  destroy: function destroy() {
    
    
    if (disposables.has(this)) {
      dispose(this);
      this.dispose();
    }
  }
});
exports.Disposable = Disposable;
