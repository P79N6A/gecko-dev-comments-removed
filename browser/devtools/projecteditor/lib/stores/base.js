





const { Cc, Ci, Cu } = require("chrome");
const { Class } = require("sdk/core/heritage");
const { EventTarget } = require("sdk/event/target");
const { emit } = require("sdk/event/core");
const promise = require("projecteditor/helpers/promise");










var Store = Class({
  extends: EventTarget,

  


  initStore: function() {
    this.resources = new Map();
  },

  refresh: function() {
    return promise.resolve();
  },

  


  allResources: function() {
    var resources = [];
    function addResource(resource) {
      resources.push(resource);
      resource.childrenSorted.forEach(addResource);
    }
    addResource(this.root);
    return resources;
  },

  notifyAdd: function(resource) {
    emit(this, "resource-added", resource);
  },

  notifyRemove: function(resource) {
    emit(this, "resource-removed", resource);
  }
});

exports.Store = Store;
