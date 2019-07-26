



"use strict";

var isJSM = typeof require !== "function";




if (isJSM) {
  var Cu = this["Components"].utils;
  let XPCOMUtils = Cu.import("resource://gre/modules/XPCOMUtils.jsm", {}).XPCOMUtils;
  this["loader"] = { lazyGetter: XPCOMUtils.defineLazyGetter.bind(XPCOMUtils) };
  this["require"] = Cu.import("resource://gre/modules/devtools/Loader.jsm", {}).devtools.require;
} else {
  var { Cu } = require("chrome");
}

const { L10N_BUNDLE } = require("devtools/profiler/consts");

var EventEmitter = require("devtools/shared/event-emitter");

Cu.import("resource://gre/modules/devtools/dbg-client.jsm");
Cu.import("resource://gre/modules/devtools/Console.jsm");
Cu.import("resource://gre/modules/AddonManager.jsm");
Cu.import("resource:///modules/devtools/ViewHelpers.jsm");

loader.lazyGetter(this, "L10N", () => new ViewHelpers.L10N(L10N_BUNDLE));

loader.lazyGetter(this, "gDevTools",
  () => Cu.import("resource:///modules/devtools/gDevTools.jsm", {}).gDevTools);

loader.lazyGetter(this, "DebuggerServer",
  () => Cu.import("resource:///modules/devtools/dbg-server.jsm", {}).DebuggerServer);






const sharedData = {
  data: new WeakMap(),
  controllers: new WeakMap(),
};




function makeProfile(name, def={}) {
  if (def.timeStarted == null)
    def.timeStarted = null;

  if (def.timeEnded == null)
    def.timeEnded = null;

  return {
    name: name,
    timeStarted: def.timeStarted,
    timeEnded: def.timeEnded,
    fromConsole: def.fromConsole || false
  };
}




function addTarget(target) {
  sharedData.data.set(target, new Map());
}

function getProfiles(target) {
  return sharedData.data.get(target);
}








function ProfilerController(target) {
  if (sharedData.controllers.has(target)) {
    return sharedData.controllers.get(target);
  }

  this.target = target;
  this.client = target.client;
  this.isConnected = false;
  this.consoleProfiles = [];
  this.reservedNames = {};

  addTarget(target);

  
  
  if (target.chrome) {
    this.isConnected = true;
    this.actor = target.form.profilerActor;
  }

  sharedData.controllers.set(target, this);
  EventEmitter.decorate(this);
};

ProfilerController.prototype = {
  target:          null,
  client:          null,
  isConnected:     null,
  consoleProfiles: null,
  reservedNames:   null,

  




  get profiles() {
    return getProfiles(this.target);
  },

  






  isProfileRecording: function PC_isProfileRecording(profile) {
    return profile.timeStarted !== null && profile.timeEnded === null;
  },

  getProfileName: function PC_getProfileName() {
    let num = 1;
    let name = L10N.getFormatStr("profiler.profileName", [num]);

    while (this.reservedNames[name]) {
      num += 1;
      name = L10N.getFormatStr("profiler.profileName", [num]);
    }

    this.reservedNames[name] = true;
    return name;
  },

  








  onConsoleEvent: function (type, data) {
    let name = data.extra.name;

    let profileStart = () => {
      if (name && this.profiles.has(name))
        return;

      
      let profile = makeProfile(name || this.getProfileName(), {
        timeStarted: data.extra.currentTime,
        fromConsole: true
      });

      this.profiles.set(profile.name, profile);
      this.consoleProfiles.push(profile.name);
      this.emit("profileStart", profile);
    };

    let profileEnd = () => {
      if (!name && !this.consoleProfiles.length)
        return;

      if (!name)
        name = this.consoleProfiles.pop();
      else
        this.consoleProfiles.filter((n) => n !== name);

      if (!this.profiles.has(name))
        return;

      let profile = this.profiles.get(name);
      if (!this.isProfileRecording(profile))
        return;

      let profileData = data.extra.profile;
      profileData.threads = profileData.threads.map((thread) => {
        let samples = thread.samples.filter((sample) => {
          return sample.time >= profile.timeStarted;
        });

        return { samples: samples };
      });

      profile.timeEnded = data.extra.currentTime;
      profile.data = profileData;

      this.emit("profileEnd", profile);
    };

    if (type === "profile")
      profileStart();

    if (type === "profileEnd")
      profileEnd();
  },

  







  connect: function (cb=function(){}) {
    if (this.isConnected) {
      return void cb();
    }

    
    
    
    

    let register = () => {
      let data = { events: ["console-api-profiler"] };

      
      
      
      
      
      

      AddonManager.getAddonByID("jid0-edalmuivkozlouyij0lpdx548bc@jetpack", (addon) => {
        if (addon && !addon.userDisabled && !addon.softDisabled)
          return void cb();

        this.request("registerEventNotifications", data, (resp) => {
          this.client.addListener("eventNotification", (type, resp) => {
            let toolbox = gDevTools.getToolbox(this.target);
            if (toolbox == null)
              return;

            this.onConsoleEvent(resp.subject.action, resp.data);
          });
        });

        cb();
      });
    };

    if (this.target.root) {
      this.actor = this.target.root.profilerActor;
      this.isConnected = true;
      return void register();
    }

    this.client.listTabs((resp) => {
      this.actor = resp.profilerActor;
      this.isConnected = true;
      register();
    });
  },

  










  request: function (type, data, cb) {
    data.to = this.actor;
    data.type = type;
    this.client.request(data, cb);
  },

  








  isActive: function (cb) {
    this.request("isActive", {}, (resp) => {
      cb(resp.error, resp.isActive, resp.currentTime);
    });
  },

  









  start: function PC_start(name, cb) {
    if (this.profiles.has(name)) {
      return;
    }

    let profile = makeProfile(name);
    this.consoleProfiles.push(name);
    this.profiles.set(name, profile);

    
    if (this.isProfileRecording(profile)) {
      return void cb();
    }

    this.isActive((err, isActive, currentTime) => {
      if (isActive) {
        profile.timeStarted = currentTime;
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

        profile.timeStarted = 0;
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
      profile.timeEnded = resp.currentTime;

      
      

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

if (isJSM) {
  var EXPORTED_SYMBOLS = ["ProfilerController"];
} else {
  module.exports = ProfilerController;
}