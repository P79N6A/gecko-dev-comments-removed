



"use strict";

module.metadata = {
  "stability": "experimental"
};


const { Class } = require("./heritage");
const { Observer, subscribe, unsubscribe, observe } = require("./observer");
const { isWeak, WeakReference } = require("./reference");
const method = require("../../method/core");

const unloadSubject = require('@loader/unload');
const addonUnloadTopic = "sdk:loader:destroy";



const uninstall = method("disposable/uninstall");
exports.uninstall = uninstall;


const shutdown = method("disposable/shutdown");
exports.shutdown = shutdown;

const disable = method("disposable/disable");
exports.disable = disable;

const upgrade = method("disposable/upgrade");
exports.upgrade = upgrade;

const downgrade = method("disposable/downgrade");
exports.downgrade = downgrade;

const unload = method("disposable/unload");
exports.unload = unload;

const dispose = method("disposable/dispose");
exports.dispose = dispose;
dispose.define(Object, object => object.dispose());


const setup = method("disposable/setup");
exports.setup = setup;
setup.define(Object, (object, ...args) => object.setup(...args));



const setupDisposable = disposable => {
  subscribe(disposable, addonUnloadTopic, isWeak(disposable));
};


const disposeDisposable = disposable => {
  unsubscribe(disposable, addonUnloadTopic);
};



const Disposable = Class({
  implements: [Observer],
  initialize: function(...args) {
    
    
    
    setup(this, ...args);
    setupDisposable(this);
  },
  destroy: function(reason) {
    
    
    disposeDisposable(this);
    unload(this, reason);
  },
  setup: function() {
    
  },
  dispose: function() {
    
  }
});
exports.Disposable = Disposable;



observe.define(Disposable, (disposable, subject, topic, data) => {
  const isUnloadTopic = topic === addonUnloadTopic;
  const isUnloadSubject = subject.wrappedJSObject === unloadSubject;
  if (isUnloadTopic && isUnloadSubject) {
    unsubscribe(disposable, topic);
    unload(disposable);
  }
});

const unloaders = {
  destroy: dispose,
  uninstall: uninstall,
  shutdown: shutdown,
  disable: disable,
  upgrade: upgrade,
  downgrade: downgrade
}
const unloaded = new WeakMap();
unload.define(Disposable, (disposable, reason) => {
  if (!unloaded.get(disposable)) {
    unloaded.set(disposable, true);
    
    
    
    const unload = unloaders[reason] || unloaders.destroy;
    unload(disposable);
  }
});





disable.define(Disposable, dispose);
downgrade.define(Disposable, dispose);
upgrade.define(Disposable, dispose);
uninstall.define(Disposable, dispose);





shutdown.define(Disposable, disposable => {});

