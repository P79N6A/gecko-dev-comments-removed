



"use strict";

const { Cu, Cc, Ci, components } = require("chrome");

const {
  PROFILE_IDLE,
  PROFILE_RUNNING,
  PROFILE_COMPLETED,
  SHOW_PLATFORM_DATA,
  L10N_BUNDLE
} = require("devtools/profiler/consts");

const { TextEncoder } = Cu.import("resource://gre/modules/commonjs/toolkit/loader.js", {});

var EventEmitter = require("devtools/shared/event-emitter");
var promise      = require("sdk/core/promise");
var Cleopatra    = require("devtools/profiler/cleopatra");
var Sidebar      = require("devtools/profiler/sidebar");
var ProfilerController = require("devtools/profiler/controller");

Cu.import("resource:///modules/devtools/gDevTools.jsm");
Cu.import("resource://gre/modules/devtools/Console.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource:///modules/devtools/ViewHelpers.jsm");
Cu.import("resource://gre/modules/osfile.jsm");
Cu.import("resource://gre/modules/NetUtil.jsm");

loader.lazyGetter(this, "L10N", () => new ViewHelpers.L10N(L10N_BUNDLE));



























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
      get record() doc.querySelector("#profiler-start"),
      get import() doc.querySelector("#profiler-import"),
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
      btn.setAttribute("checked", true);
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

        this.sidebar.on("save", (_, uid) => {
          let profile = this.profiles.get(uid);

          if (!profile.data)
            return void Cu.reportError("Can't save profile because there's no data.");

          this.openFileDialog({ mode: "save", name: profile.name }).then((file) => {
            if (file)
              this.saveProfile(file, profile.data);
          });
        });

        this.sidebar.on("select", (_, uid) => {
          let profile = this.profiles.get(uid);
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

          let imp = this.controls.import;
          imp.addEventListener("click", () => {
            this.openFileDialog({ mode: "open" }).then((file) => {
              if (file)
                this.loadProfile(file);
            });
          }, false);
          imp.removeAttribute("disabled");

          
          for (let [name, data] of this.controller.profiles) {
            this.importProfile(name, data.data);
          }

          this.isReady = true;
          this.emit("ready");
          deferred.resolve(this);
        });

        this.controller.on("profileEnd", (_, data) => {
          this.importProfile(data.name, data.data);

          if (this.recordingProfile && !data.fromConsole)
            this.recordingProfile = null;

          this.emit("stopped");
        });

        return deferred.promise;
      })
      .then(null, (reason) =>
        Cu.reportError("ProfilePanel open failed: " + reason.message));
  },

  











  createProfile: function (name, opts={}) {
    if (name && this.getProfileByName(name)) {
      return this.getProfileByName(name);
    }

    let uid = ++this._uid;
    let name = name || this.controller.getProfileName();
    let profile = new Cleopatra(this, {
      uid: uid,
      name: name,
      showPlatformData: this.showPlatformData,
      external: opts.external
    });

    this.profiles.set(uid, profile);
    this.sidebar.addProfile(profile);
    this.emit("profileCreated", uid);

    return profile;
  },

  









  importProfile: function (name, data, opts={}) {
    let profile = this.createProfile(name, { external: opts.external });
    profile.isStarted = false;
    profile.isFinished = true;
    profile.data = data;
    profile.parse(data, () => this.emit("parsed"));

    this.sidebar.setProfileState(profile, PROFILE_COMPLETED);
    if (!this.sidebar.selectedItem)
      this.sidebar.selectedItem = this.sidebar.getItemByProfile(profile);

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

  








  displaySource: function PP_displaySource(data) {
    let { browserWindow: win, document: doc } = this;
    let { uri, line, isChrome } = data;
    let deferred = promise.defer();

    if (isChrome) {
      return void win.gViewSourceUtils.viewSource(uri, null, doc, line);
    }

    let showSource = ({ DebuggerView }) => {
      if (DebuggerView.Sources.containsValue(uri)) {
        DebuggerView.setEditorLocation(uri, line).then(deferred.resolve);
      }
      
      
    }

    
    
    
    let toolbox = gDevTools.getToolbox(this.target);
    let debuggerAlreadyOpen = toolbox.getPanel("jsdebugger");
    toolbox.selectTool("jsdebugger").then(({ panelWin: dbg }) => {
      if (debuggerAlreadyOpen) {
        showSource(dbg);
      } else {
        dbg.once(dbg.EVENTS.SOURCES_ADDED, () => showSource(dbg));
      }
    });

    return deferred.promise;
  },

  







  openFileDialog: function (opts={}) {
    let deferred = promise.defer();

    let picker = Ci.nsIFilePicker;
    let fp = Cc["@mozilla.org/filepicker;1"].createInstance(picker);
    let { name, mode } = opts;
    let save = mode === "save";
    let title = L10N.getStr(save ? "profiler.saveFileAs" : "profiler.openFile");

    fp.init(this.window, title, save ? picker.modeSave : picker.modeOpen);
    fp.appendFilter("JSON", "*.json");
    fp.appendFilters(picker.filterText | picker.filterAll);

    if (save)
      fp.defaultString = (name || "profile") + ".json";

    fp.open((result) => {
      deferred.resolve(result === picker.returnCancel ? null : fp.file);
    });

    return deferred.promise;
  },

  







  saveProfile: function (file, data) {
    let encoder = new TextEncoder();
    let buffer = encoder.encode(JSON.stringify({ profile: data }, null, "  "));
    let opts = { tmpPath: file.path + ".tmp" };

    return OS.File.writeAtomic(file.path, buffer, opts);
  },

  





  loadProfile: function (file) {
    let deferred = promise.defer();
    let ch = NetUtil.newChannel(file);
    ch.contentType = "application/json";

    NetUtil.asyncFetch(ch, (input, status) => {
      if (!components.isSuccessCode(status)) throw new Error(status);

      let conv = Cc["@mozilla.org/intl/scriptableunicodeconverter"]
        .createInstance(Ci.nsIScriptableUnicodeConverter);
      conv.charset = "UTF-8";

      let data = NetUtil.readInputStreamToString(input, input.available());
      data = conv.ConvertToUnicode(data);
      this.importProfile(file.leafName, JSON.parse(data).profile, { external: true });

      deferred.resolve();
    });

    return deferred.promise;
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
