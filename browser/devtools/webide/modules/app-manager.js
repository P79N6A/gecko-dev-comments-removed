



const {Cu} = require("chrome");

let { Promise: promise } = Cu.import("resource://gre/modules/Promise.jsm", {});

const {devtools} = Cu.import("resource://gre/modules/devtools/Loader.jsm", {});
const {Services} = Cu.import("resource://gre/modules/Services.jsm");
const {FileUtils} = Cu.import("resource://gre/modules/FileUtils.jsm");
const EventEmitter = require("devtools/toolkit/event-emitter");
const {TextEncoder, OS}  = Cu.import("resource://gre/modules/osfile.jsm", {});
const {AppProjects} = require("devtools/app-manager/app-projects");
const TabStore = require("devtools/webide/tab-store");
const {AppValidator} = require("devtools/app-manager/app-validator");
const {ConnectionManager, Connection} = require("devtools/client/connection-manager");
const {AppActorFront} = require("devtools/app-actor-front");
const {getDeviceFront} = require("devtools/server/actors/device");
const {getPreferenceFront} = require("devtools/server/actors/preference");
const {getSettingsFront} = require("devtools/server/actors/settings");
const {setTimeout} = require("sdk/timers");
const {Task} = Cu.import("resource://gre/modules/Task.jsm", {});
const {RuntimeScanners, RuntimeTypes} = require("devtools/webide/runtimes");
const {NetUtil} = Cu.import("resource://gre/modules/NetUtil.jsm", {});
const Telemetry = require("devtools/shared/telemetry");
const {ProjectBuilding} = require("./build");

const Strings = Services.strings.createBundle("chrome://browser/locale/devtools/webide.properties");

let AppManager = exports.AppManager = {

  
  DEFAULT_PROJECT_ICON: "chrome://browser/skin/devtools/app-manager/default-app-icon.png",
  DEFAULT_PROJECT_NAME: "--",

  _initialized: false,

  init: function() {
    if (this._initialized) {
      return;
    }
    this._initialized = true;

    let port = Services.prefs.getIntPref("devtools.debugger.remote-port");
    this.connection = ConnectionManager.createConnection("localhost", port);
    this.onConnectionChanged = this.onConnectionChanged.bind(this);
    this.connection.on(Connection.Events.STATUS_CHANGED, this.onConnectionChanged);

    this.tabStore = new TabStore(this.connection);
    this.onTabList = this.onTabList.bind(this);
    this.onTabNavigate = this.onTabNavigate.bind(this);
    this.onTabClosed = this.onTabClosed.bind(this);
    this.tabStore.on("tab-list", this.onTabList);
    this.tabStore.on("navigate", this.onTabNavigate);
    this.tabStore.on("closed", this.onTabClosed);

    this._clearRuntimeList();
    this._rebuildRuntimeList = this._rebuildRuntimeList.bind(this);
    RuntimeScanners.on("runtime-list-updated", this._rebuildRuntimeList);
    RuntimeScanners.enable();
    this._rebuildRuntimeList();

    this.onInstallProgress = this.onInstallProgress.bind(this);

    this._telemetry = new Telemetry();
  },

  destroy: function() {
    if (!this._initialized) {
      return;
    }
    this._initialized = false;

    this.selectedProject = null;
    this.selectedRuntime = null;
    RuntimeScanners.off("runtime-list-updated", this._rebuildRuntimeList);
    RuntimeScanners.disable();
    this.runtimeList = null;
    this.tabStore.off("tab-list", this.onTabList);
    this.tabStore.off("navigate", this.onTabNavigate);
    this.tabStore.off("closed", this.onTabClosed);
    this.tabStore.destroy();
    this.tabStore = null;
    this.connection.off(Connection.Events.STATUS_CHANGED, this.onConnectionChanged);
    this._listTabsResponse = null;
    this.connection.disconnect();
    this.connection = null;
  },

  















































  update: function(what, details) {
    
    this.emit("app-manager-update", what, details);
  },

  reportError: function(l10nProperty, ...l10nArgs) {
    let win = Services.wm.getMostRecentWindow("devtools:webide");
    if (win) {
      win.UI.reportError(l10nProperty, ...l10nArgs);
    } else {
      let text;
      if (l10nArgs.length > 0) {
        text = Strings.formatStringFromName(l10nProperty, l10nArgs, l10nArgs.length);
      } else {
        text = Strings.GetStringFromName(l10nProperty);
      }
      console.error(text);
    }
  },

  onConnectionChanged: function() {
    console.log("Connection status changed: " + this.connection.status);

    if (this.connection.status == Connection.Status.DISCONNECTED) {
      this.selectedRuntime = null;
    }

    if (!this.connected) {
      if (this._appsFront) {
        this._appsFront.off("install-progress", this.onInstallProgress);
        this._appsFront.unwatchApps();
        this._appsFront = null;
      }
      this._listTabsResponse = null;
    } else {
      this.connection.client.listTabs((response) => {
        if (response.webappsActor) {
          let front = new AppActorFront(this.connection.client,
                                        response);
          front.on("install-progress", this.onInstallProgress);
          front.watchApps(() => this.checkIfProjectIsRunning())
          .then(() => {
            
            
            
            this._appsFront = front;
            this._listTabsResponse = response;
            this.update("runtime-global-actors");
          })
          .then(() => {
            this.checkIfProjectIsRunning();
            this.update("runtime-targets", { type: "apps" });
            front.fetchIcons().then(() => this.update("runtime-apps-icons"));
          });
        } else {
          this._listTabsResponse = response;
          this.update("runtime-global-actors");
        }
      });
    }

    this.update("connection");
  },

  get connected() {
    return this.connection &&
           this.connection.status == Connection.Status.CONNECTED;
  },

  get apps() {
    if (this._appsFront) {
      return this._appsFront.apps;
    } else {
      return new Map();
    }
  },

  onInstallProgress: function(event, details) {
    this.update("install-progress", details);
  },

  isProjectRunning: function() {
    if (this.selectedProject.type == "mainProcess" ||
        this.selectedProject.type == "tab") {
      return true;
    }

    let app = this._getProjectFront(this.selectedProject);
    return app && app.running;
  },

  checkIfProjectIsRunning: function() {
    if (this.selectedProject) {
      if (this.isProjectRunning()) {
        this.update("project-started");
      } else {
        this.update("project-stopped");
      }
    }
  },

  listTabs: function() {
    return this.tabStore.listTabs();
  },

  onTabList: function() {
    this.update("runtime-targets", { type: "tabs" });
  },

  
  onTabNavigate: function() {
    this.update("runtime-targets", { type: "tabs" });
    if (this.selectedProject.type !== "tab") {
      return;
    }
    let tab = this.selectedProject.app = this.tabStore.selectedTab;
    let uri = NetUtil.newURI(tab.url);
    
    
    
    
    tab.favicon = uri.prePath + "/favicon.ico";
    tab.name = tab.title || Strings.GetStringFromName("project_tab_loading");
    if (uri.scheme.startsWith("http")) {
      tab.name = uri.host + ": " + tab.name;
    }
    this.selectedProject.location = tab.url;
    this.selectedProject.name = tab.name;
    this.selectedProject.icon = tab.favicon;
    this.update("project-validated");
  },

  onTabClosed: function() {
    if (this.selectedProject.type !== "tab") {
      return;
    }
    this.selectedProject = null;
  },

  reloadTab: function() {
    if (this.selectedProject && this.selectedProject.type != "tab") {
      return promise.reject("tried to reload non-tab project");
    }
    return this.getTarget().then(target => {
      target.activeTab.reload();
    }, console.error.bind(console));
  },

  getTarget: function() {
    if (this.selectedProject.type == "mainProcess") {
      
      if (this.connection.client.mainRoot.traits.allowChromeProcess) {
        return this.connection.client.getProcess()
                   .then(aResponse => {
                     return devtools.TargetFactory.forRemoteTab({
                       form: aResponse.form,
                       client: this.connection.client,
                       chrome: true
                     });
                   });
      } else {
        
        return devtools.TargetFactory.forRemoteTab({
          form: this._listTabsResponse,
          client: this.connection.client,
          chrome: true,
          isTabActor: false
        });
      }
    }

    if (this.selectedProject.type == "tab") {
      return this.tabStore.getTargetForTab();
    }

    let app = this._getProjectFront(this.selectedProject);
    if (!app) {
      return promise.reject("Can't find app front for selected project");
    }

    return Task.spawn(function* () {
      
      
      

      for (let i = 0; i < 10; i++) {
        try {
          return yield app.getTarget();
        } catch(e) {}
        let deferred = promise.defer();
        setTimeout(deferred.resolve, 500);
        yield deferred.promise;
      }

      AppManager.reportError("error_cantConnectToApp", app.manifest.manifestURL);
      throw new Error("can't connect to app");
    });
  },

  getProjectManifestURL: function(project) {
    let manifest = null;
    if (project.type == "runtimeApp") {
      manifest = project.app.manifestURL;
    }

    if (project.type == "hosted") {
      manifest = project.location;
    }

    if (project.type == "packaged" && project.packagedAppOrigin) {
      manifest = "app://" + project.packagedAppOrigin + "/manifest.webapp";
    }

    return manifest;
  },

  _getProjectFront: function(project) {
    let manifest = this.getProjectManifestURL(project);
    if (manifest && this._appsFront) {
      return this._appsFront.apps.get(manifest);
    }
    return null;
  },

  _selectedProject: null,
  set selectedProject(project) {
    
    if (JSON.stringify(this._selectedProject) ===
        JSON.stringify(project)) {
      return;
    }

    let cancelled = false;
    this.update("before-project", { cancel: () => { cancelled = true; } });
    if (cancelled)  {
      return;
    }

    this._selectedProject = project;

    
    this.tabStore.selectedTab = null;

    if (project) {
      if (project.type == "packaged" ||
          project.type == "hosted") {
        this.validateAndUpdateProject(project);
      }
      if (project.type == "tab") {
        this.tabStore.selectedTab = project.app;
      }
    }

    this.update("project");
    this.checkIfProjectIsRunning();
  },
  get selectedProject() {
    return this._selectedProject;
  },

  removeSelectedProject: Task.async(function*() {
    let location = this.selectedProject.location;
    AppManager.selectedProject = null;
    
    if (AppManager.selectedProject != null) {
      return;
    }

    yield AppProjects.remove(location);
    AppManager.update("project-removed");
  }),

  packageProject: Task.async(function*(project) {
    if (!project) {
      return;
    }
    if (project.type == "packaged" ||
        project.type == "hosted") {
      yield ProjectBuilding.build({
        project: project,
        logger: this.update.bind(this, "pre-package")
      });
    }
  }),

  _selectedRuntime: null,
  set selectedRuntime(value) {
    this._selectedRuntime = value;
    if (!value && this.selectedProject &&
        (this.selectedProject.type == "mainProcess" ||
         this.selectedProject.type == "runtimeApp" ||
         this.selectedProject.type == "tab")) {
      this.selectedProject = null;
    }
    this.update("runtime");
  },

  get selectedRuntime() {
    return this._selectedRuntime;
  },

  connectToRuntime: function(runtime) {

    if (this.connected && this.selectedRuntime === runtime) {
      
      return promise.resolve();
    }

    let deferred = promise.defer();

    this.disconnectRuntime().then(() => {
      this.selectedRuntime = runtime;

      let onConnectedOrDisconnected = () => {
        this.connection.off(Connection.Events.CONNECTED, onConnectedOrDisconnected);
        this.connection.off(Connection.Events.DISCONNECTED, onConnectedOrDisconnected);
        if (this.connected) {
          deferred.resolve();
        } else {
          deferred.reject();
        }
      };
      this.connection.on(Connection.Events.CONNECTED, onConnectedOrDisconnected);
      this.connection.on(Connection.Events.DISCONNECTED, onConnectedOrDisconnected);
      try {
        
        this.connection.resetOptions();
        
        
        this.selectedRuntime.connect(this.connection)
                            .then(null, e => deferred.reject(e));
      } catch(e) {
        deferred.reject(e);
      }
    }, deferred.reject);

    
    let logResult = result => {
      this._telemetry.log("DEVTOOLS_WEBIDE_CONNECTION_RESULT", result);
      if (runtime.type) {
        this._telemetry.log("DEVTOOLS_WEBIDE_" + runtime.type +
                            "_CONNECTION_RESULT", result);
      }
    };
    deferred.promise.then(() => logResult(true), () => logResult(false));

    
    deferred.promise.then(() => {
      const timerId = "DEVTOOLS_WEBIDE_CONNECTION_TIME_SECONDS";
      this._telemetry.startTimer(timerId);
      this.connection.once(Connection.Events.STATUS_CHANGED, () => {
        this._telemetry.stopTimer(timerId);
      });
    }).catch(() => {
      
      
      
    });

    return deferred.promise;
  },

  isMainProcessDebuggable: function() {
    
    
    return this.connection.client &&
           this.connection.client.mainRoot &&
           this.connection.client.mainRoot.traits.allowChromeProcess ||
           (this._listTabsResponse &&
            this._listTabsResponse.consoleActor);
  },

  get deviceFront() {
    if (!this._listTabsResponse) {
      return null;
    }
    return getDeviceFront(this.connection.client, this._listTabsResponse);
  },

  get preferenceFront() {
    if (!this._listTabsResponse) {
      return null;
    }
    return getPreferenceFront(this.connection.client, this._listTabsResponse);
  },

  get settingsFront() {
     if (!this._listTabsResponse) {
      return null;
    }
    return getSettingsFront(this.connection.client, this._listTabsResponse);
  },

  disconnectRuntime: function() {
    if (!this.connected) {
      return promise.resolve();
    }
    let deferred = promise.defer();
    this.connection.once(Connection.Events.DISCONNECTED, () => deferred.resolve());
    this.connection.disconnect();
    return deferred.promise;
  },

  launchRuntimeApp: function() {
    if (this.selectedProject && this.selectedProject.type != "runtimeApp") {
      return promise.reject("attempting to launch a non-runtime app");
    }
    let app = this._getProjectFront(this.selectedProject);
    return app.launch();
  },

  launchOrReloadRuntimeApp: function() {
    if (this.selectedProject && this.selectedProject.type != "runtimeApp") {
      return promise.reject("attempting to launch / reload a non-runtime app");
    }
    let app = this._getProjectFront(this.selectedProject);
    if (!app.running) {
      return app.launch();
    } else {
      return app.reload();
    }
  },

  runtimeCanHandleApps: function() {
    return !!this._appsFront;
  },

  installAndRunProject: function() {
    let project = this.selectedProject;

    if (!project || (project.type != "packaged" && project.type != "hosted")) {
      console.error("Can't install project. Unknown type of project.");
      return promise.reject("Can't install");
    }

    if (!this._listTabsResponse) {
      this.reportError("error_cantInstallNotFullyConnected");
      return promise.reject("Can't install");
    }

    if (!this._appsFront) {
      console.error("Runtime doesn't have a webappsActor");
      return promise.reject("Can't install");
    }

    return Task.spawn(function* () {
      let self = AppManager;

      
      yield self.packageProject(project);
      yield self.validateAndUpdateProject(project);

      if (project.errorsCount > 0) {
        self.reportError("error_cantInstallValidationErrors");
        return;
      }

      let installPromise;

      if (project.type != "packaged" && project.type != "hosted") {
        return promise.reject("Don't know how to install project");
      }

      let response;
      if (project.type == "packaged") {
        let packageDir = yield ProjectBuilding.getPackageDir(project);
        console.log("Installing app from " + packageDir);

        response = yield self._appsFront.installPackaged(packageDir,
                                                         project.packagedAppOrigin);

        
        
        project.packagedAppOrigin = response.appId;
        
        AppProjects.update(project);
      }

      if (project.type == "hosted") {
        let manifestURLObject = Services.io.newURI(project.location, null, null);
        let origin = Services.io.newURI(manifestURLObject.prePath, null, null);
        let appId = origin.host;
        let metadata = {
          origin: origin.spec,
          manifestURL: project.location
        };
        response = yield self._appsFront.installHosted(appId,
                                            metadata,
                                            project.manifest);
      }

      
      
      if (project.manifest.role && project.manifest.role === "addon") {
        return;
      }

      let {app} = response;
      if (!app.running) {
        let deferred = promise.defer();
        self.on("app-manager-update", function onUpdate(event, what) {
          if (what == "project-started") {
            self.off("app-manager-update", onUpdate);
            deferred.resolve();
          }
        });
        yield app.launch();
        yield deferred.promise;
      } else {
        yield app.reload();
      }
    });
  },

  stopRunningApp: function() {
    let app = this._getProjectFront(this.selectedProject);
    return app.close();
  },

  

  validateAndUpdateProject: function(project) {
    if (!project) {
      return promise.reject();
    }

    return Task.spawn(function* () {

      let packageDir = yield ProjectBuilding.getPackageDir(project);
      let validation = new AppValidator({
        type: project.type,
        
        location: packageDir
      });

      yield validation.validate();

      if (validation.manifest) {
        let manifest = validation.manifest;
        let iconPath;
        if (manifest.icons) {
          let size = Object.keys(manifest.icons).sort(function(a, b) b - a)[0];
          if (size) {
            iconPath = manifest.icons[size];
          }
        }
        if (!iconPath) {
          project.icon = AppManager.DEFAULT_PROJECT_ICON;
        } else {
          if (project.type == "hosted") {
            let manifestURL = Services.io.newURI(project.location, null, null);
            let origin = Services.io.newURI(manifestURL.prePath, null, null);
            project.icon = Services.io.newURI(iconPath, null, origin).spec;
          } else if (project.type == "packaged") {
            let projectFolder = FileUtils.File(packageDir);
            let folderURI = Services.io.newFileURI(projectFolder).spec;
            project.icon = folderURI + iconPath.replace(/^\/|\\/, "");
          }
        }
        project.manifest = validation.manifest;

        if ("name" in project.manifest) {
          project.name = project.manifest.name;
        } else {
          project.name = AppManager.DEFAULT_PROJECT_NAME;
        }
      } else {
        project.manifest = null;
        project.icon = AppManager.DEFAULT_PROJECT_ICON;
        project.name = AppManager.DEFAULT_PROJECT_NAME;
      }

      project.validationStatus = "valid";

      if (validation.warnings.length > 0) {
        project.warningsCount = validation.warnings.length;
        project.warnings = validation.warnings;
        project.validationStatus = "warning";
      } else {
        project.warnings = "";
        project.warningsCount = 0;
      }

      if (validation.errors.length > 0) {
        project.errorsCount = validation.errors.length;
        project.errors = validation.errors;
        project.validationStatus = "error";
      } else {
        project.errors = "";
        project.errorsCount = 0;
      }

      if (project.warningsCount && project.errorsCount) {
        project.validationStatus = "error warning";
      }

      if (project.type === "hosted" && project.location !== validation.manifestURL) {
        yield AppProjects.updateLocation(project, validation.manifestURL);
      } else if (AppProjects.get(project.location)) {
        yield AppProjects.update(project);
      }

      if (AppManager.selectedProject === project) {
        AppManager.update("project-validated");
      }
    });
  },

  

  _clearRuntimeList: function() {
    this.runtimeList = {
      usb: [],
      wifi: [],
      simulator: [],
      other: []
    };
  },

  _rebuildRuntimeList: function() {
    let runtimes = RuntimeScanners.listRuntimes();
    this._clearRuntimeList();

    
    for (let runtime of runtimes) {
      switch (runtime.type) {
        case RuntimeTypes.USB:
          this.runtimeList.usb.push(runtime);
          break;
        case RuntimeTypes.WIFI:
          this.runtimeList.wifi.push(runtime);
          break;
        case RuntimeTypes.SIMULATOR:
          this.runtimeList.simulator.push(runtime);
          break;
        default:
          this.runtimeList.other.push(runtime);
      }
    }

    this.update("runtime-details");
    this.update("runtime-list");
  },

  

  writeManifest: function(project) {
    if (project.type != "packaged") {
      return promise.reject("Not a packaged app");
    }

    if (!project.manifest) {
      project.manifest = {};
    }

    let folder = project.location;
    let manifestPath = OS.Path.join(folder, "manifest.webapp");
    let text = JSON.stringify(project.manifest, null, 2);
    let encoder = new TextEncoder();
    let array = encoder.encode(text);
    return OS.File.writeAtomic(manifestPath, array, {tmpPath: manifestPath + ".tmp"});
  },
};

EventEmitter.decorate(AppManager);
