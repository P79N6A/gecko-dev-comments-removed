



"use strict";

module.metadata = {
  "stability": "experimental"
};


let { Class } = require("./heritage");
let { on, off } = require('../system/events');
let unloadSubject = require('@loader/unload');

function DisposeHandler(disposable) {
  return function onDisposal({subject}) {
    if (subject.wrappedJSObject === unloadSubject) {
      off("sdk:loader:destroy", onDisposal);
      disposable.destroy();
    }
  }
}



let Disposable = Class({
  initialize: function dispose() {
    this.setupDisposal();
    this.setup.apply(this, arguments);
  },
  setupDisposal: function setupDisposal() {
    
    
    Object.defineProperty(this, "onDisposal", { value: DisposeHandler(this) });
    on("sdk:loader:destroy", this.onDisposal);
  },
  teardownDisposable: function tearDisposal() {
    
    off("sdk:loader:destroy", this.onDisposal);
  },

  setup: function setup() {
    
  },
  dispose: function dispose() {
    
  },

  destroy: function destroy() {
    
    
    this.teardownDisposable();
    this.dispose();
  }
});

exports.Disposable = Disposable;
