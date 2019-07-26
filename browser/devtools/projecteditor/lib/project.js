





const { Cu } = require("chrome");
const { Class } = require("sdk/core/heritage");
const { EventTarget } = require("sdk/event/target");
const { emit } = require("sdk/event/core");
const { scope, on, forget } = require("projecteditor/helpers/event");
const prefs = require("sdk/preferences/service");
const { LocalStore } = require("projecteditor/stores/local");
const { OS } = Cu.import("resource://gre/modules/osfile.jsm", {});
const { Task } = Cu.import("resource://gre/modules/Task.jsm", {});
const promise = require("projecteditor/helpers/promise");
const { TextEncoder, TextDecoder } = require('sdk/io/buffer');
const url = require('sdk/url');

const gDecoder = new TextDecoder();
const gEncoder = new TextEncoder();






var Project = Class({
  extends: EventTarget,

  





  initialize: function(options) {
    this.localStores = new Map();

    this.load(options);
  },

  destroy: function() {
    
    
    
    this.removeAllStores();
  },

  toString: function() {
    return "[Project] " + this.name;
  },

  








  load: function(options) {
    this.id = options.id;
    this.name = options.name || "Untitled";

    let paths = new Set(options.directories.map(name => OS.Path.normalize(name)));

    for (let [path, store] of this.localStores) {
      if (!paths.has(path)) {
        this.removePath(path);
      }
    }

    for (let path of paths) {
      this.addPath(path);
    }
  },

  





  refresh: function() {
    return Task.spawn(function*() {
      for (let [path, store] of this.localStores) {
        yield store.refresh();
      }
    }.bind(this));
  },


  









  resourceFor: function(path, options) {
    let store = this.storeContaining(path);
    return store.resourceFor(path, options);
  },

  





  allResources: function() {
    let resources = [];
    for (let store of this.allStores()) {
      resources = resources.concat(store.allResources());
    }
    return resources;
  },

  





  allStores: function*() {
    for (let [path, store] of this.localStores) {
      yield store;
    }
  },

  





  allPaths: function*() {
    for (let [path, store] of this.localStores) {
      yield path;
    }
  },

  






  storeContaining: function(path) {
    let containingStore = null;
    for (let store of this.allStores()) {
      if (store.contains(path)) {
        
        containingStore = store;
      }
    }
    return containingStore;
  },

  






  addPath: function(path) {
    if (!this.localStores.has(path)) {
      this.addLocalStore(new LocalStore(path));
    }
    return this.localStores.get(path);
  },

  




  removePath: function(path) {
    this.removeLocalStore(this.localStores.get(path));
  },


  





  addLocalStore: function(store) {
    store.canPair = true;
    this.localStores.set(store.path, store);

    
    on(this, store, "resource-added", (resource) => {
      emit(this, "resource-added", resource);
    });
    on(this, store, "resource-removed", (resource) => {
      emit(this, "resource-removed", resource);
    })

    emit(this, "store-added", store);
  },


  


  removeAllStores: function() {
    for (let store of this.allStores()) {
      this.removeLocalStore(store);
    }
  },

  





  removeLocalStore: function(store) {
    
    
    if (store) {
      this.localStores.delete(store.path);
      forget(this, store);
      emit(this, "store-removed", store);
      store.destroy();
    }
  }
});

exports.Project = Project;
