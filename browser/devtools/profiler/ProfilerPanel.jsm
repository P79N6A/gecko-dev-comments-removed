



"use strict";

const Cu = Components.utils;

Cu.import("resource:///modules/devtools/ProfilerController.jsm");
Cu.import("resource:///modules/devtools/ProfilerHelpers.jsm");
Cu.import("resource://gre/modules/commonjs/sdk/core/promise.js");
Cu.import("resource:///modules/devtools/EventEmitter.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

this.EXPORTED_SYMBOLS = ["ProfilerPanel"];

XPCOMUtils.defineLazyGetter(this, "DebuggerServer", function () {
  Cu.import("resource://gre/modules/devtools/dbg-server.jsm");
  return DebuggerServer;
});



















function ProfileUI(uid, panel) {
  let doc = panel.document;
  let win = panel.window;

  EventEmitter.decorate(this);

  this.isReady = false;
  this.panel = panel;
  this.uid = uid;

  this.iframe = doc.createElement("iframe");
  this.iframe.setAttribute("flex", "1");
  this.iframe.setAttribute("id", "profiler-cleo-" + uid);
  this.iframe.setAttribute("src", "devtools/cleopatra.html?" + uid);
  this.iframe.setAttribute("hidden", "true");

  
  
  

  doc.getElementById("profiler-report").appendChild(this.iframe);
  win.addEventListener("message", function (event) {
    if (parseInt(event.data.uid, 10) !== parseInt(this.uid, 10)) {
      return;
    }

    switch (event.data.status) {
      case "loaded":
        this.isReady = true;
        this.emit("ready");
        break;
      case "start":
        
        
        this.panel.startProfiling(function onStart() {
          var data = JSON.stringify({task: "onStarted"});
          this.iframe.contentWindow.postMessage(data, "*");
        }.bind(this));

        break;
      case "stop":
        
        
        this.panel.stopProfiling(function onStop() {
          var data = JSON.stringify({task: "onStopped"});
          this.iframe.contentWindow.postMessage(data, "*");
        }.bind(this));
    }
  }.bind(this));
}

ProfileUI.prototype = {
  show: function PUI_show() {
    this.iframe.removeAttribute("hidden");
  },

  hide: function PUI_hide() {
    this.iframe.setAttribute("hidden", true);
  },

  









  parse: function PUI_parse(data, onParsed) {
    if (!this.isReady) {
      return;
    }

    let win = this.iframe.contentWindow;

    win.postMessage(JSON.stringify({
      task: "receiveProfileData",
      rawProfile: data
    }), "*");

    let poll = function pollBreadcrumbs() {
      let wait = this.panel.window.setTimeout.bind(null, poll, 100);
      let trail = win.gBreadcrumbTrail;

      if (!trail) {
        return wait();
      }

      if (!trail._breadcrumbs || !trail._breadcrumbs.length) {
        return wait();
      }

      onParsed();
    }.bind(this);

    poll();
  },

  


  destroy: function PUI_destroy() {
    this.isReady = null
    this.panel = null;
    this.uid = null;
    this.iframe = null;
  }
};



























function ProfilerPanel(frame, toolbox) {
  this.isReady = false;
  this.window = frame.window;
  this.document = frame.document;
  this.target = toolbox.target;
  this.controller = new ProfilerController();

  this.profiles = new Map();
  this._uid = 0;

  EventEmitter.decorate(this);
}

ProfilerPanel.prototype = {
  isReady:    null,
  window:     null,
  document:   null,
  target:     null,
  controller: null,
  profiles:   null,

  _uid:       null,
  _activeUid: null,

  get activeProfile() {
    return this.profiles.get(this._activeUid);
  },

  set activeProfile(profile) {
    this._activeUid = profile.uid;
  },

  





  open: function PP_open() {
    let deferred = Promise.defer();

    this.controller.connect(function onConnect() {
      let create = this.document.getElementById("profiler-create");
      create.addEventListener("click", this.createProfile.bind(this), false);
      create.removeAttribute("disabled");

      let profile = this.createProfile();
      this.switchToProfile(profile, function () {
        this.isReady = true;
        this.emit("ready");

        deferred.resolve(this);
      }.bind(this))
    }.bind(this));

    return deferred.promise;
  },

  








  createProfile: function PP_addProfile() {
    let uid  = ++this._uid;
    let list = this.document.getElementById("profiles-list");
    let item = this.document.createElement("li");
    let wrap = this.document.createElement("h1");

    item.setAttribute("id", "profile-" + uid);
    item.setAttribute("data-uid", uid);
    item.addEventListener("click", function (ev) {
      this.switchToProfile(this.profiles.get(uid));
    }.bind(this), false);

    wrap.className = "profile-name";
    wrap.textContent = L10N.getFormatStr("profiler.profileName", [uid]);

    item.appendChild(wrap);
    list.appendChild(item);

    let profile = new ProfileUI(uid, this);
    this.profiles.set(uid, profile);

    this.emit("profileCreated", uid);
    return profile;
  },

  










  switchToProfile: function PP_switchToProfile(profile, onLoad=function() {}) {
    let doc = this.document;

    if (this.activeProfile) {
      this.activeProfile.hide();
    }

    let active = doc.querySelector("#profiles-list > li.splitview-active");
    if (active) {
      active.className = "";
    }

    doc.getElementById("profile-" + profile.uid).className = "splitview-active";
    profile.show();
    this.activeProfile = profile;

    if (profile.isReady) {
      this.emit("profileSwitched", profile.uid);
      onLoad();
      return;
    }

    profile.once("ready", function () {
      this.emit("profileSwitched", profile.uid);
      onLoad();
    }.bind(this));
  },

  






  startProfiling: function PP_startProfiling(onStart) {
    this.controller.start(function (err) {
      if (err) {
        Cu.reportError("ProfilerController.start: " + err.message);
        return;
      }

      onStart();
      this.emit("started");
    }.bind(this));
  },

  







  stopProfiling: function PP_stopProfiling(onStop) {
    this.controller.isActive(function (err, isActive) {
      if (err) {
        Cu.reportError("ProfilerController.isActive: " + err.message);
        return;
      }

      if (!isActive) {
        return;
      }

      this.controller.stop(function (err, data) {
        if (err) {
          Cu.reportError("ProfilerController.stop: " + err.message);
          return;
        }

        this.activeProfile.parse(data, function onParsed() {
          this.emit("parsed");
        }.bind(this));

        onStop();
        this.emit("stopped");
      }.bind(this));
    }.bind(this));
  },

  


  destroy: function PP_destroy() {
    if (this.profiles) {
      let uid = this._uid;

      while (uid >= 0) {
        if (this.profiles.has(uid)) {
          this.profiles.get(uid).destroy();
          this.profiles.delete(uid);
        }
        uid -= 1;
      }
    }

    if (this.controller) {
      this.controller.destroy();
    }

    this.isReady = null;
    this.window = null;
    this.document = null;
    this.target = null;
    this.controller = null;
    this.profiles = null;
    this._uid = null;
    this._activeUid = null;

    this.emit("destroyed");
  }
};
