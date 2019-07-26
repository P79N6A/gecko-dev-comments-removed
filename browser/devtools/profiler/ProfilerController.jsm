



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




function makeProfile(name) {
  return {
    name: name,
    timeStarted: null,
    timeEnded: null
  };
}





function ProfilerConnection(client) {
  this.client = client;
  this.startTime = 0;
}

ProfilerConnection.prototype = {
  actor: null,
  startTime: null,

  





  get currentTime() {
    return (new Date()).getTime() - this.startTime;
  },

  





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

    this.client.request(message, function () {
      
      
      this.startTime = (new Date()).getTime();
      aCallback.apply(null, Array.slice(arguments));
    }.bind(this));
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
  this.pool = {};

  
  
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

  






  isProfileRecording: function PC_isProfileRecording(profile) {
    return profile.timeStarted !== null && profile.timeEnded === null;
  },

  









  start: function PC_start(name, cb) {
    if (this.pool[name]) {
      return;
    }

    let profile = this.pool[name] = makeProfile(name);
    let profiler = this.profiler;

    
    if (this.isProfileRecording(profile)) {
      return void cb();
    }

    this.isActive(function (err, isActive) {
      if (isActive) {
        profile.timeStarted = profiler.currentTime;
        return void cb();
      }

      profiler.startProfiler(function onStart(aResponse) {
        if (aResponse.error) {
          return void cb(aResponse.error);
        }

        profile.timeStarted = profiler.currentTime;
        cb();
      });
    });
  },

  









  stop: function PC_stop(name, cb) {
    let profiler = this.profiler;
    let profile = this.pool[name];

    if (!profile || !this.isProfileRecording(profile)) {
      return;
    }

    let isRecording = function () {
      for (let name in this.pool) {
        if (this.isProfileRecording(this.pool[name])) {
          return true;
        }
      }

      return false;
    }.bind(this);

    let onStop = function (data) {
      if (isRecording()) {
        return void cb(null, data);
      }

      profiler.stopProfiler(function onStopProfiler(response) {
        cb(response.error, data);
      });
    }.bind(this);

    profiler.getProfileData(function onData(aResponse) {
      if (aResponse.error) {
        Cu.reportError("Failed to fetch profile data before stopping the profiler.");
        return void cb(aResponse.error, null);
      }

      let data = aResponse.profile;
      profile.timeEnded = profiler.currentTime;

      data.threads = data.threads.map(function (thread) {
        let samples = thread.samples.filter(function (sample) {
          return sample.time >= profile.timeStarted;
        });
        return { samples: samples };
      });

      onStop(data);
    });
  },

  


  destroy: function PC_destroy() {
    this.profiler.destroy();
    this.profiler = null;
  }
};
