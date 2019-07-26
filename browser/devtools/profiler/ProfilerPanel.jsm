



"use strict";

const Cu = Components.utils;

Cu.import("resource:///modules/devtools/gDevTools.jsm");
Cu.import("resource:///modules/devtools/ProfilerController.jsm");
Cu.import("resource:///modules/devtools/ProfilerHelpers.jsm");
Cu.import("resource:///modules/devtools/shared/event-emitter.js");
Cu.import("resource:///modules/devtools/SideMenuWidget.jsm");
Cu.import("resource:///modules/devtools/ViewHelpers.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/devtools/Console.jsm");

this.EXPORTED_SYMBOLS = ["ProfilerPanel"];

XPCOMUtils.defineLazyModuleGetter(this, "Promise",
    "resource://gre/modules/commonjs/sdk/core/promise.js");

XPCOMUtils.defineLazyModuleGetter(this, "DebuggerServer",
  "resource://gre/modules/devtools/dbg-server.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Services",
  "resource://gre/modules/Services.jsm");

const PROFILE_IDLE = 0;
const PROFILE_RUNNING = 1;
const PROFILE_COMPLETED = 2;






















function ProfileUI(uid, name, panel) {
  let doc = panel.document;
  let win = panel.window;

  EventEmitter.decorate(this);

  this.isReady = false;
  this.isStarted = false;
  this.isFinished = false;

  this.messages = [];
  this.panel = panel;
  this.uid = uid;
  this.name = name;

  this.iframe = doc.createElement("iframe");
  this.iframe.setAttribute("flex", "1");
  this.iframe.setAttribute("id", "profiler-cleo-" + uid);
  this.iframe.setAttribute("src", "cleopatra.html?" + uid);
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
        this.start();
        break;
      case "stop":
        this.stop();
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
  



  get contentWindow() {
    if (!this.iframe) {
      return null;
    }

    try {
      return this.iframe.contentWindow;
    } catch (err) {
      return null;
    }
  },

  show: function PUI_show() {
    this.iframe.removeAttribute("hidden");
  },

  hide: function PUI_hide() {
    this.iframe.setAttribute("hidden", true);
  },

  









  parse: function PUI_parse(data, onParsed) {
    if (!this.isReady) {
      return void this.on("ready", this.parse.bind(this, data, onParsed));
    }

    this.message({ task: "receiveProfileData", rawProfile: data }).then(() => {
      let poll = () => {
        let wait = this.panel.window.setTimeout.bind(null, poll, 100);
        let trail = this.contentWindow.gBreadcrumbTrail;

        if (!trail) {
          return wait();
        }

        if (!trail._breadcrumbs || !trail._breadcrumbs.length) {
          return wait();
        }

        onParsed();
      };

      poll();
    });
  },

  












  start: function PUI_start(startFn) {
    if (this.isStarted || this.isFinished) {
      return;
    }

    startFn = startFn || this.panel.startProfiling.bind(this.panel);
    startFn(this.name, () => {
      this.isStarted = true;
      this.panel.sidebar.setProfileState(this, PROFILE_RUNNING);
      this.panel.broadcast(this.uid, {task: "onStarted"}); 
      this.emit("started");
    });
  },

  











  stop: function PUI_stop(stopFn) {
    if (!this.isStarted || this.isFinished) {
      return;
    }

    stopFn = stopFn || this.panel.stopProfiling.bind(this.panel);
    stopFn(this.name, () => {
      this.isStarted = false;
      this.isFinished = true;
      this.panel.sidebar.setProfileState(this, PROFILE_COMPLETED);
      this.panel.broadcast(this.uid, {task: "onStopped"});
      this.emit("stopped");
    });
  },

  






  message: function PIU_message(data) {
    let deferred = Promise.defer();
    let win = this.contentWindow;
    data = JSON.stringify(data);

    if (win) {
      win.postMessage(data, "*");
      deferred.resolve();
    } else {
      this.messages.push({ data: data, onSuccess: () => deferred.resolve() });
    }

    return deferred.promise;
  },

  


  flushMessages: function PIU_flushMessages() {
    if (!this.contentWindow) {
      return;
    }

    let msg;
    while (msg = this.messages.shift()) {
      this.contentWindow.postMessage(msg.data, "*");
      msg.onSuccess();
    }
  },

  


  destroy: function PUI_destroy() {
    this.isReady = null
    this.panel = null;
    this.uid = null;
    this.iframe = null;
    this.messages = null;
  }
};

function SidebarView(el) {
  EventEmitter.decorate(this);
  this.widget = new SideMenuWidget(el);
}

SidebarView.prototype = Heritage.extend(WidgetMethods, {
  getItemByProfile: function (profile) {
    return this.getItemForPredicate(item => item.attachment.uid === profile.uid);
  },

  setProfileState: function (profile, state) {
    let item = this.getItemByProfile(profile);
    let label = item.target.querySelector(".profiler-sidebar-item > span");

    switch (state) {
      case PROFILE_IDLE:
        label.textContent = L10N.getStr("profiler.stateIdle");
        break;
      case PROFILE_RUNNING:
        label.textContent = L10N.getStr("profiler.stateRunning");
        break;
      case PROFILE_COMPLETED:
        label.textContent = L10N.getStr("profiler.stateCompleted");
        break;
      default: 
        return;
    }

    item.attachment.state = state;
    this.emit("stateChanged", item);
  }
});



























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
    
    let target = this.target;
    let promise = !target.isRemote ? target.makeRemote() : Promise.resolve(target);

    return promise
      .then((target) => {
        let deferred = Promise.defer();

        this.controller = new ProfilerController(this.target);
        this.sidebar = new SidebarView(this.document.querySelector("#profiles-list"));
        this.sidebar.widget.addEventListener("select", (ev) => {
          if (!ev.detail)
            return;

          let profile = this.profiles.get(ev.detail.attachment.uid);
          this.activeProfile = profile;

          if (profile.isReady) {
            profile.flushMessages();
            return void this.emit("profileSwitched", profile.uid);
          }

          profile.once("ready", () => {
            profile.flushMessages();
            this.emit("profileSwitched", profile.uid);
          });
        });

        this.controller.connect(() => {
          let create = this.document.getElementById("profiler-create");
          create.addEventListener("click", () => this.createProfile(), false);
          create.removeAttribute("disabled");

          let profile = this.createProfile();
          let onSwitch = (_, uid) => {
            if (profile.uid !== uid)
              return;

            this.off("profileSwitched", onSwitch);
            this.isReady = true;
            this.emit("ready");

            deferred.resolve(this);
          };

          this.on("profileSwitched", onSwitch);
          this.sidebar.selectedItem = this.sidebar.getItemByProfile(profile);
        });

        return deferred.promise;
      })
      .then(null, (reason) =>
        Cu.reportError("ProfilePanel open failed: " + reason.message));
  },

  











  createProfile: function PP_createProfile(name) {
    if (name && this.getProfileByName(name)) {
      return this.getProfileByName(name);
    }

    let uid = ++this._uid;

    
    
    
    

    if (!name) {
      name = L10N.getFormatStr("profiler.profileName", [uid]);
      while (this.getProfileByName(name)) {
        uid = ++this._uid;
        name = L10N.getFormatStr("profiler.profileName", [uid]);
      }
    }

    let box = this.document.createElement("vbox");
    box.className = "profiler-sidebar-item";
    box.id = "profile-" + uid;
    let h3 = this.document.createElement("h3");
    h3.textContent = name;
    let span = this.document.createElement("span");
    span.textContent = L10N.getStr("profiler.stateIdle");
    box.appendChild(h3);
    box.appendChild(span);

    this.sidebar.push(box, { attachment: { uid: uid, name: name, state: PROFILE_IDLE } });

    let profile = new ProfileUI(uid, name, this);
    this.profiles.set(uid, profile);

    this.emit("profileCreated", uid);
    return profile;
  },

  






  startProfiling: function PP_startProfiling(name, onStart) {
    this.controller.start(name, (err) => {
      if (err) {
        return void Cu.reportError("ProfilerController.start: " + err.message);
      }

      onStart();
      this.emit("started");
    });
  },

  







  stopProfiling: function PP_stopProfiling(name, onStop) {
    this.controller.isActive(function (err, isActive) {
      if (err) {
        Cu.reportError("ProfilerController.isActive: " + err.message);
        return;
      }

      if (!isActive) {
        return;
      }

      this.controller.stop(name, function (err, data) {
        if (err) {
          Cu.reportError("ProfilerController.stop: " + err.message);
          return;
        }

        this.activeProfile.data = data;
        this.activeProfile.parse(data, function onParsed() {
          this.emit("parsed");
        }.bind(this));

        onStop();
        this.emit("stopped", data);
      }.bind(this));
    }.bind(this));
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

    if (data.task === "onStarted") {
      this._runningUid = target;
    } else {
      this._runningUid = null;
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
