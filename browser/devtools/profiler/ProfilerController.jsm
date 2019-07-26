



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






const sharedData = {
  startTime: 0,
  data: new WeakMap(),
};




function makeProfile(name) {
  return {
    name: name,
    timeStarted: null,
    timeEnded: null
  };
}




function addTarget(target) {
  sharedData.data.set(target, new Map());
}

function getProfiles(target) {
  return sharedData.data.get(target);
}

function getCurrentTime() {
  return (new Date()).getTime() - sharedData.startTime;
}








function ProfilerController(target) {
  this.target = target;
  this.client = target.client;
  this.isConnected = false;

  addTarget(target);

  
  
  if (target.chrome) {
    this.isConnected = true;
    this.actor = target.form.profilerActor;
  }
};

ProfilerController.prototype = {
  




  get profiles() {
    return getProfiles(this.target);
  },

  






  isProfileRecording: function PC_isProfileRecording(profile) {
    return profile.timeStarted !== null && profile.timeEnded === null;
  },

  







  connect: function (cb) {
    if (this.isConnected) {
      return void cb();
    }

    this.client.listTabs((resp) => {
      this.actor = resp.profilerActor;
      this.isConnected = true;
      cb();
    })
  },

  










  request: function (type, data, cb) {
    data.to = this.actor;
    data.type = type;
    this.client.request(data, cb);
  },

  








  isActive: function (cb) {
    this.request("isActive", {}, (resp) => cb(resp.error, resp.isActive));
  },

  









  start: function PC_start(name, cb) {
    if (this.profiles.has(name)) {
      return;
    }

    let profile = makeProfile(name);
    this.profiles.set(name, profile);

    
    if (this.isProfileRecording(profile)) {
      return void cb();
    }

    this.isActive((err, isActive) => {
      if (isActive) {
        profile.timeStarted = getCurrentTime();
        return void cb();
      }

      let params = {
        entries: 1000000,
        interval: 1,
        features: ["js"],
      };

      this.request("startProfiler", params, (resp) => {
        if (resp.error) {
          return void cb(resp.error);
        }

        sharedData.startTime = (new Date()).getTime();
        profile.timeStarted = getCurrentTime();
        cb();
      });
    });
  },

  











  stop: function PC_stop(name, cb) {
    if (!this.profiles.has(name)) {
      return;
    }

    let profile = this.profiles.get(name);
    if (!this.isProfileRecording(profile)) {
      return;
    }

    this.request("getProfile", {}, (resp) => {
      if (resp.error) {
        Cu.reportError("Failed to fetch profile data.");
        return void cb(resp.error, null);
      }

      let data = resp.profile;
      profile.timeEnded = getCurrentTime();

      
      

      data.threads = data.threads.map((thread) => {
        let samples = thread.samples.filter((sample) => {
          return sample.time >= profile.timeStarted;
        });

        return { samples: samples };
      });

      cb(null, data);
    });
  },

  


  destroy: function PC_destroy() {
    this.client = null;
    this.target = null;
    this.actor = null;
  }
};
