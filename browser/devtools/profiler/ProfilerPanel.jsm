



"use strict";

const Cu = Components.utils;

Cu.import("resource:///modules/devtools/gDevTools.jsm");
Cu.import("resource:///modules/devtools/ProfilerController.jsm");
Cu.import("resource:///modules/devtools/ProfilerHelpers.jsm");
Cu.import("resource://gre/modules/commonjs/sdk/core/promise.js");
Cu.import("resource:///modules/devtools/EventEmitter.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

this.EXPORTED_SYMBOLS = ["ProfilerPanel"];

XPCOMUtils.defineLazyModuleGetter(this, "DebuggerServer",
  "resource://gre/modules/devtools/dbg-server.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Services",
  "resource://gre/modules/Services.jsm");






















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

    let label = doc.querySelector("li#profile-" + this.uid + " > h1");
    switch (event.data.status) {
      case "loaded":
        if (this.panel._runningUid !== null) {
          this.iframe.contentWindow.postMessage(JSON.stringify({
            uid: this._runningUid,
            isCurrent: this._runningUid === uid,
            task: "onStarted"
          }), "*");
        }

        this.isReady = true;
        this.emit("ready");
        break;
      case "start":
        
        
        
        
        this.panel.startProfiling(function onStart() {
          this.panel.broadcast(this.uid, {task: "onStarted"});
          label.textContent = label.textContent + " *";
        }.bind(this));

        break;
      case "stop":
        
        
        
        this.panel.stopProfiling(function onStop() {
          this.panel.broadcast(this.uid, {task: "onStopped"});
          label.textContent = label.textContent.replace(/\s\*$/, "");
        }.bind(this));
        break;
      case "disabled":
        this.emit("disabled");
        break;
      case "enabled":
        this.emit("enabled");
        break;
      case "displaysource":
        this.panel.displaySource(event.data.data);
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
  this.controller = new ProfilerController(this.target);

  this.profiles = new Map();
  this._uid = 0;

  EventEmitter.decorate(this);
}

ProfilerPanel.prototype = {
  isReady:     null,
  window:      null,
  document:    null,
  target:      null,
  controller:  null,
  profiles:    null,

  _uid:        null,
  _activeUid:  null,
  _runningUid: null,
  _browserWin: null,

  get activeProfile() {
    return this.profiles.get(this._activeUid);
  },

  set activeProfile(profile) {
    this._activeUid = profile.uid;
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

  









  broadcast: function PP_broadcast(target, data) {
    if (!this.profiles) {
      return;
    }

    if (data.task === "onStarted") {
      this._runningUid = target;
    } else {
      this._runningUid = null;
    }

    let uid = this._uid;
    while (uid >= 0) {
      if (this.profiles.has(uid)) {
        let iframe = this.profiles.get(uid).iframe;
        iframe.contentWindow.postMessage(JSON.stringify({
          uid: target,
          isCurrent: target === uid,
          task: data.task
        }), "*");
      }
      uid -= 1;
    }
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
