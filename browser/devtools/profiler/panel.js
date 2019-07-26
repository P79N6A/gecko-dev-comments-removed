



"use strict";

const { Cu } = require("chrome");
const {
  PROFILE_IDLE,
  PROFILE_RUNNING,
  PROFILE_COMPLETED,
  SHOW_PLATFORM_DATA
} = require("devtools/profiler/consts");

var EventEmitter = require("devtools/shared/event-emitter");
var promise      = require("sdk/core/promise");
var Cleopatra    = require("devtools/profiler/cleopatra");
var Sidebar      = require("devtools/profiler/sidebar");
var ProfilerController = require("devtools/profiler/controller");

Cu.import("resource:///modules/devtools/gDevTools.jsm");
Cu.import("resource://gre/modules/devtools/Console.jsm");
Cu.import("resource://gre/modules/Services.jsm");



























function ProfilerPanel(frame, toolbox) {
  this.isReady = false;
  this.window = frame.window;
  this.document = frame.document;
  this.target = toolbox.target;

  this.profiles = new Map();
  this._uid = 0;
  this._msgQueue = {};

  EventEmitter.decorate(this);
}

ProfilerPanel.prototype = {
  isReady:     null,
  window:      null,
  document:    null,
  target:      null,
  controller:  null,
  profiles:    null,
  sidebar:     null,

  _uid:        null,
  _activeUid:  null,
  _runningUid: null,
  _browserWin: null,
  _msgQueue:   null,

  get controls() {
    let doc = this.document;

    return {
      get record() doc.querySelector("#profiler-start")
    };
  },

  get activeProfile() {
    return this.profiles.get(this._activeUid);
  },

  set activeProfile(profile) {
    if (this._activeUid === profile.uid)
      return;

    if (this.activeProfile)
      this.activeProfile.hide();

    this._activeUid = profile.uid;
    profile.show();
  },

  set recordingProfile(profile) {
    let btn = this.controls.record;
    this._runningUid = profile ? profile.uid : null;

    if (this._runningUid)
      btn.setAttribute("checked", true)
    else
      btn.removeAttribute("checked");
  },

  get recordingProfile() {
    return this.profiles.get(this._runningUid);
  },

  get browserWindow() {
    if (this._browserWin) {
      return this._browserWin;
    }

    let win = this.window.top;
    let type = win.document.documentElement.getAttribute("windowtype");

    if (type !== "navigator:browser") {
      win = Services.wm.getMostRecentWindow("navigator:browser");
    }

    return this._browserWin = win;
  },

  get showPlatformData() {
    return Services.prefs.getBoolPref(SHOW_PLATFORM_DATA);
  },

  set showPlatformData(enabled) {
    Services.prefs.setBoolPref(SHOW_PLATFORM_DATA, enabled);
  },

  





  open: function PP_open() {
    
    let target = this.target;
    let targetPromise = !target.isRemote ? target.makeRemote() : promise.resolve(target);

    return targetPromise
      .then((target) => {
        let deferred = promise.defer();

        this.controller = new ProfilerController(this.target);

        this.sidebar = new Sidebar(this.document.querySelector("#profiles-list"));
        this.sidebar.widget.addEventListener("select", (ev) => {
          if (!ev.detail)
            return;

          let profile = this.profiles.get(parseInt(ev.detail.value, 10));
          this.activeProfile = profile;

          if (profile.isReady) {
            return void this.emit("profileSwitched", profile.uid);
          }

          profile.once("ready", () => {
            this.emit("profileSwitched", profile.uid);
          });
        });

        this.controller.connect(() => {
          let btn = this.controls.record;
          btn.addEventListener("click", () => this.toggleRecording(), false);
          btn.removeAttribute("disabled");

          
          for (let [name, data] of this.controller.profiles) {
            let profile = this.createProfile(name);
            profile.isStarted = false;
            profile.isFinished = true;
            profile.data = data.data;
            profile.parse(data.data, () => this.emit("parsed"));

            this.sidebar.setProfileState(profile, PROFILE_COMPLETED);
            if (!this.sidebar.selectedItem) {
              this.sidebar.selectedItem = this.sidebar.getItemByProfile(profile);
            }
          }

          this.isReady = true;
          this.emit("ready");
          deferred.resolve(this);
        });

        this.controller.on("profileEnd", (_, data) => {
          let profile = this.createProfile(data.name);
          profile.isStarted = false;
          profile.isFinished = true;
          profile.data = data.data;
          profile.parse(data.data, () => this.emit("parsed"));

          this.sidebar.setProfileState(profile, PROFILE_COMPLETED);
          if (!this.sidebar.selectedItem)
            this.sidebar.selectedItem = this.sidebar.getItemByProfile(profile);

          if (this.recordingProfile && !data.fromConsole)
            this.recordingProfile = null;

          this.emit("stopped");
        });

        return deferred.promise;
      })
      .then(null, (reason) =>
        Cu.reportError("ProfilePanel open failed: " + reason.message));
  },

  











  createProfile: function (name) {
    if (name && this.getProfileByName(name)) {
      return this.getProfileByName(name);
    }

    let uid = ++this._uid;
    let name = name || this.controller.getProfileName();
    let profile = new Cleopatra(this, {
      uid: uid,
      name: name,
      showPlatformData: this.showPlatformData
    });

    this.profiles.set(uid, profile);
    this.sidebar.addProfile(profile);
    this.emit("profileCreated", uid);

    return profile;
  },

  


  toggleRecording: function () {
    let profile = this.recordingProfile;

    if (!profile) {
      profile = this.createProfile();

      this.startProfiling(profile.name, () => {
        profile.isStarted = true;

        this.sidebar.setProfileState(profile, PROFILE_RUNNING);
        this.recordingProfile = profile;
        this.emit("started");
      });

      return;
    }

    this.stopProfiling(profile.name, (data) => {
      profile.isStarted = false;
      profile.isFinished = true;
      profile.data = data;
      profile.parse(data, () => this.emit("parsed"));

      this.sidebar.setProfileState(profile, PROFILE_COMPLETED);
      this.activeProfile = profile;
      this.sidebar.selectedItem = this.sidebar.getItemByProfile(profile);
      this.recordingProfile = null;
      this.emit("stopped");
    });
  },

  






  startProfiling: function (name, onStart) {
    this.controller.start(name, (err) => {
      if (err) {
        return void Cu.reportError("ProfilerController.start: " + err.message);
      }

      onStart();
      this.emit("started");
    });
  },

  






  stopProfiling: function (name, onStop) {
    this.controller.isActive((err, isActive) => {
      if (err) {
        Cu.reportError("ProfilerController.isActive: " + err.message);
        return;
      }

      if (!isActive) {
        return;
      }

      this.controller.stop(name, (err, data) => {
        if (err) {
          Cu.reportError("ProfilerController.stop: " + err.message);
          return;
        }

        onStop(data);
        this.emit("stopped", data);
      });
    });
  },

  





  getProfileByName: function PP_getProfileByName(name) {
    if (!this.profiles) {
      return null;
    }

    for (let [ uid, profile ] of this.profiles) {
      if (profile.name === name) {
        return profile;
      }
    }

    return null;
  },

  





  getProfileByUID: function PP_getProfileByUID(uid) {
    if (!this.profiles) {
      return null;
    }

    return this.profiles.get(uid) || null;
  },

  





  eachProfile: function PP_eachProfile(cb) {
    let uid = this._uid;

    if (!this.profiles) {
      return;
    }

    while (uid >= 0) {
      if (this.profiles.has(uid)) {
        cb(this.profiles.get(uid));
      }

      uid -= 1;
    }
  },

  









  broadcast: function PP_broadcast(target, data) {
    if (!this.profiles) {
      return;
    }

    this.eachProfile((profile) => {
      profile.message({
        uid: target,
        isCurrent: target === profile.uid,
        task: data.task
      });
    });
  },

  








  displaySource: function PP_displaySource(data, onOpen=function() {}) {
    let win = this.window;
    let panelWin, timeout;

    function onSourceShown(event) {
      if (event.detail.url !== data.uri) {
        return;
      }

      panelWin.removeEventListener("Debugger:SourceShown", onSourceShown, false);
      panelWin.editor.setCaretPosition(data.line - 1);
      onOpen();
    }

    if (data.isChrome) {
      return void this.browserWindow.gViewSourceUtils.
        viewSource(data.uri, null, this.document, data.line);
    }

    gDevTools.showToolbox(this.target, "jsdebugger").then(function (toolbox) {
      let dbg = toolbox.getCurrentPanel();
      panelWin = dbg.panelWin;

      let view = dbg.panelWin.DebuggerView;
      if (view.Sources.selectedValue === data.uri) {
        view.editor.setCaretPosition(data.line - 1);
        onOpen();
        return;
      }

      panelWin.addEventListener("Debugger:SourceShown", onSourceShown, false);
      panelWin.DebuggerView.Sources.preferredSource = data.uri;
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

module.exports = ProfilerPanel;
