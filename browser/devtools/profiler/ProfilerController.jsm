



"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/devtools/dbg-client.jsm");

let EXPORTED_SYMBOLS = ["ProfilerController"];

XPCOMUtils.defineLazyGetter(this, "DebuggerServer", function () {
  Cu.import("resource://gre/modules/devtools/dbg-server.jsm");
  return DebuggerServer;
});





function ProfilerConnection(client) {
  this.client = client;
}

ProfilerConnection.prototype = {
  actor: null,

  





  connect: function PCn_connect(aCallback) {
    this.client.listTabs(function (aResponse) {
      this.actor = aResponse.profilerActor;
      aCallback();
    }.bind(this));
  },

  







  isActive: function PCn_isActive(aCallback) {
    var message = { to: this.actor, type: "isActive" };
    this.client.request(message, aCallback);
  },

  







  startProfiler: function PCn_startProfiler(aCallback) {
    var message = {
      to: this.actor,
      type: "startProfiler",
      entries: 1000000,
      interval: 1,
      features: ["js"],
    };
    this.client.request(message, aCallback);
  },

  







  stopProfiler: function PCn_stopProfiler(aCallback) {
    var message = { to: this.actor, type: "stopProfiler" };
    this.client.request(message, aCallback);
  },

  







  getProfileData: function PCn_getProfileData(aCallback) {
    var message = { to: this.actor, type: "getProfile" };
    this.client.request(message, aCallback);
  },

  


  destroy: function PCn_destroy() {
    this.client = null;
  }
};




function ProfilerController(target) {
  this.profiler = new ProfilerConnection(target.client);
  
  
  this._connected = !!target.chrome;
  if (target.chrome) {
    this.profiler.actor = target.form.profilerActor;
  }
}

ProfilerController.prototype = {
  







  connect: function (aCallback) {
    if (this._connected) {
      return void aCallback();
    }

    this.profiler.connect(function onConnect() {
      this._connected = true;
      aCallback();
    }.bind(this));
  },

  








  isActive: function PC_isActive(aCallback) {
    this.profiler.isActive(function onActive(aResponse) {
      aCallback(aResponse.error, aResponse.isActive);
    });
  },

  







  start: function PC_start(aCallback) {
    this.profiler.startProfiler(function onStart(aResponse) {
      aCallback(aResponse.error);
    });
  },

  







  stop: function PC_stop(aCallback) {
    this.profiler.getProfileData(function onData(aResponse) {
      let data = aResponse.profile;
      if (aResponse.error) {
        Cu.reportError("Failed to fetch profile data before stopping the profiler.");
      }

      this.profiler.stopProfiler(function onStop(aResponse) {
        aCallback(aResponse.error, data);
      });
    }.bind(this));
  },

  


  destroy: function PC_destroy(aCallback) {
    this.profiler.destroy();
    this.profiler = null;
  }
};
