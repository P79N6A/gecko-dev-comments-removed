



"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/devtools/dbg-client.jsm");
Cu.import("resource://gre/modules/devtools/Console.jsm");
Cu.import("resource://gre/modules/AddonManager.jsm");

let EXPORTED_SYMBOLS = ["ProfilerController"];

XPCOMUtils.defineLazyModuleGetter(this, "gDevTools",
  "resource:///modules/devtools/gDevTools.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "DebuggerServer",
  "resource://gre/modules/devtools/dbg-server.jsm");






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
    timeEnded: def.timeEnded
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

  addTarget(target);

  
  
  if (target.chrome) {
    this.isConnected = true;
    this.actor = target.form.profilerActor;
  }

  sharedData.controllers.set(target, this);
};

ProfilerController.prototype = {
  




  get profiles() {
    return getProfiles(this.target);
  },

  






  isProfileRecording: function PC_isProfileRecording(profile) {
    return profile.timeStarted !== null && profile.timeEnded === null;
  },

  










  onConsoleEvent: function (type, data, panel) {
    let name = data.extra.name;

    let profileStart = () => {
      if (name && this.profiles.has(name))
        return;

      
      
      let profile = panel.createProfile(name);
      profile.start((name, cb) => cb());

      
      this.profiles.set(profile.name, makeProfile(profile.name, {
        timeStarted: data.extra.currentTime
      }));
      this.consoleProfiles.push(profile.name);
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
      profile.timeEnded = data.extra.currentTime;

      profileData.threads = profileData.threads.map((thread) => {
        let samples = thread.samples.filter((sample) => {
          return sample.time >= profile.timeStarted;
        });

        return { samples: samples };
      });

      let ui = panel.getProfileByName(name);
      ui.data = profileData;
      ui.parse(profileData, () => panel.emit("parsed"));
      ui.stop((name, cb) => cb());
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

            let panel = toolbox.getPanel("jsprofiler");
            if (panel)
              return void this.onConsoleEvent(resp.subject.action, resp.data, panel);

            
            
            
            
            
            
            
            
            
            
            

            toolbox.once("jsprofiler-ready", (_, panel) => {
              this.onConsoleEvent(resp.subject.action, resp.data, panel);
            });

            toolbox.loadTool("jsprofiler");
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
