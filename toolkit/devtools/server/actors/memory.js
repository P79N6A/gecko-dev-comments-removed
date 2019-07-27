



"use strict";

const {Cc, Ci, Cu} = require("chrome");
let protocol = require("devtools/server/protocol");
let {method, RetVal} = protocol;







let MemoryActor = protocol.ActorClass({
  typeName: "memory",

  initialize: function(conn, tabActor) {
    protocol.Actor.prototype.initialize.call(this, conn);
    this.tabActor = tabActor;
    this._mgr = Cc["@mozilla.org/memory-reporter-manager;1"]
                  .getService(Ci.nsIMemoryReporterManager);
  },

  destroy: function() {
    this._mgr = null;
    protocol.Actor.prototype.destroy.call(this);
  },

  





  measure: method(function() {
    let result = {};

    let jsObjectsSize = {};
    let jsStringsSize = {};
    let jsOtherSize = {};
    let domSize = {};
    let styleSize = {};
    let otherSize = {};
    let totalSize = {};
    let jsMilliseconds = {};
    let nonJSMilliseconds = {};

    try {
      this._mgr.sizeOfTab(this.tabActor.window, jsObjectsSize, jsStringsSize, jsOtherSize,
                          domSize, styleSize, otherSize, totalSize, jsMilliseconds, nonJSMilliseconds);
      result.total = totalSize.value;
      result.domSize = domSize.value;
      result.styleSize = styleSize.value;
      result.jsObjectsSize = jsObjectsSize.value;
      result.jsStringsSize = jsStringsSize.value;
      result.jsOtherSize = jsOtherSize.value;
      result.otherSize = otherSize.value;
      result.jsMilliseconds = jsMilliseconds.value.toFixed(1);
      result.nonJSMilliseconds = nonJSMilliseconds.value.toFixed(1);
    } catch (e) {
      console.error(e);
      let url = this.tabActor.url;
      console.error("Error getting size of "+url);
    }

    return result;
  }, {
    request: {},
    response: RetVal("json"),
  }),

  residentUnique: method(function() {
    return this._mgr.residentUnique;
  }, {
    request: {},
    response: { value: RetVal("number") }
  })
});

exports.MemoryActor = MemoryActor;

exports.MemoryFront = protocol.FrontClass(MemoryActor, {
  initialize: function(client, form) {
    protocol.Front.prototype.initialize.call(this, client, form);
    this.actorID = form.memoryActor;
    this.manage(this);
  }
});

exports.register = function(handle) {
  handle.addGlobalActor(MemoryActor, "memoryActor");
  handle.addTabActor(MemoryActor, "memoryActor");
};

exports.unregister = function(handle) {
  handle.removeGlobalActor(MemoryActor, "memoryActor");
  handle.removeTabActor(MemoryActor, "memoryActor");
};
