





"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

const EXPORTED_SYMBOLS = ["WorkerAPI"];

function WorkerAPI(port) {
  if (!port)
    throw new Error("Can't initialize WorkerAPI with a null port");

  this._port = port;
  this._port.onmessage = this._handleMessage.bind(this);

  this.initialized = false;

  
  
  
  this._port.postMessage({topic: "social.initialize"});
}

WorkerAPI.prototype = {
  _handleMessage: function _handleMessage(event) {
    let {topic, data} = event.data;
    let handler = this.handlers[topic];
    if (!handler) {
      Cu.reportError("WorkerAPI: topic doesn't have a handler: '" + topic + "'");
      return;
    }
    try {
      handler.call(this, data);
    } catch (ex) {
      Cu.reportError("WorkerAPI: failed to handle message '" + topic + "': " + ex);
    }
  },

  handlers: {
    "social.initialize-response": function (data) {
      this.initialized = true;
    }
  }
}
