



const Cc = Components.classes;
const Cu = Components.utils;
const Ci = Components.interfaces;

Cu.import("resource:///modules/devtools/gDevTools.jsm");
Cu.import("resource://gre/modules/Task.jsm");

const {devtools} = Cu.import("resource://gre/modules/devtools/Loader.jsm", {});
const {require} = devtools;
const {Services} = Cu.import("resource://gre/modules/Services.jsm");
const {AppProjects} = require("devtools/app-manager/app-projects");
const {Connection} = require("devtools/client/connection-manager");
const {AppManager} = require("devtools/webide/app-manager");
const {Promise: promise} = Cu.import("resource://gre/modules/Promise.jsm", {});
const ProjectEditor = require("projecteditor/projecteditor");
const {Devices} = Cu.import("resource://gre/modules/devtools/Devices.jsm");
const {GetAvailableAddons} = require("devtools/webide/addons");
const {getJSON} = require("devtools/shared/getjson");
const utils = require("devtools/webide/utils");
const Telemetry = require("devtools/shared/telemetry");
const {RuntimeScanners, WiFiScanner} = require("devtools/webide/runtimes");
const {showDoorhanger} = require("devtools/shared/doorhanger");
const ProjectList = require("devtools/webide/project-list");

const Strings = Services.strings.createBundle("chrome://browser/locale/devtools/webide.properties");

const HTML = "http://www.w3.org/1999/xhtml";
const HELP_URL = "https://developer.mozilla.org/docs/Tools/WebIDE/Troubleshooting";

const MAX_ZOOM = 1.4;
const MIN_ZOOM = 0.6;


getJSON("devtools.webide.addonsURL", true);
getJSON("devtools.webide.templatesURL", true);


console.log = console.log.bind(console);
console.warn = console.warn.bind(console);
console.error = console.error.bind(console);

window.addEventListener("load", function onLoad() {
  window.removeEventListener("load", onLoad);
  UI.init();
});

window.addEventListener("unload", function onUnload() {
  window.removeEventListener("unload", onUnload);
  UI.destroy();
});

let projectList;

let UI = {
  init: function() {
    this._telemetry = new Telemetry();
    this._telemetry.toolOpened("webide");

    AppManager.init();

    this.onMessage = this.onMessage.bind(this);
    window.addEventListener("message", this.onMessage);

    this.appManagerUpdate = this.appManagerUpdate.bind(this);
    AppManager.on("app-manager-update", this.appManagerUpdate);

    projectList = new ProjectList(window, window);
    ProjectPanel.toggle(projectList.sidebarsEnabled);

    this.updateCommands();
    this.updateRuntimeList();

    this.onfocus = this.onfocus.bind(this);
    window.addEventListener("focus", this.onfocus, true);

    AppProjects.load().then(() => {
      this.autoSelectProject();
      projectList.update();
    }, e => {
      console.error(e);
      this.reportError("error_appProjectsLoadFailed");
    });

    
    
    let autoinstallADBHelper = Services.prefs.getBoolPref("devtools.webide.autoinstallADBHelper");
    let autoinstallFxdtAdapters = Services.prefs.getBoolPref("devtools.webide.autoinstallFxdtAdapters");
    if (autoinstallADBHelper) {
      GetAvailableAddons().then(addons => {
        addons.adb.install();
      }, console.error);
    }
    if (autoinstallFxdtAdapters) {
      GetAvailableAddons().then(addons => {
        addons.adapters.install();
      }, console.error);
    }
    Services.prefs.setBoolPref("devtools.webide.autoinstallADBHelper", false);
    Services.prefs.setBoolPref("devtools.webide.autoinstallFxdtAdapters", false);

    if (Services.prefs.getBoolPref("devtools.webide.widget.autoinstall") &&
        !Services.prefs.getBoolPref("devtools.webide.widget.enabled")) {
      Services.prefs.setBoolPref("devtools.webide.widget.enabled", true);
      gDevToolsBrowser.moveWebIDEWidgetInNavbar();
    }

    this.setupDeck();

    this.contentViewer = window.QueryInterface(Ci.nsIInterfaceRequestor)
                               .getInterface(Ci.nsIWebNavigation)
                               .QueryInterface(Ci.nsIDocShell)
                               .contentViewer;
    this.contentViewer.fullZoom = Services.prefs.getCharPref("devtools.webide.zoom");

    gDevToolsBrowser.isWebIDEInitialized.resolve();
  },

  destroy: function() {
    window.removeEventListener("focus", this.onfocus, true);
    AppManager.off("app-manager-update", this.appManagerUpdate);
    AppManager.destroy();
    projectList = null;
    window.removeEventListener("message", this.onMessage);
    this.updateConnectionTelemetry();
    this._telemetry.toolClosed("webide");
  },

  canCloseProject: function() {
    if (this.projecteditor) {
      return this.projecteditor.confirmUnsaved();
    }
    return true;
  },

  onfocus: function() {
    
    
    
    
    if (AppManager.selectedProject &&
        AppManager.selectedProject.type != "mainProcess" &&
        AppManager.selectedProject.type != "runtimeApp" &&
        AppManager.selectedProject.type != "tab") {
      AppManager.validateAndUpdateProject(AppManager.selectedProject);
    }

    
    
    
    showDoorhanger({ window, type: "deveditionpromo", anchor: document.querySelector("#deck") });
  },

  appManagerUpdate: function(event, what, details) {
    
    
    switch (what) {
      case "runtime-list":
        this.updateRuntimeList();
        this.autoConnectRuntime();
        break;
      case "connection":
        this.updateRuntimeButton();
        this.updateCommands();
        this.updateConnectionTelemetry();
        break;
      case "before-project":
        if (!this.canCloseProject())  {
          details.cancel();
        }
        break;
      case "project":
        this._updatePromise = Task.spawn(function() {
          UI.updateTitle();
          yield UI.destroyToolbox();
          UI.updateCommands();
          UI.updateProjectButton();
          UI.openProject();
          yield UI.autoStartProject();
          UI.autoOpenToolbox();
          UI.saveLastSelectedProject();
          projectList.update();
        });
        return;
      case "project-started":
        this.updateCommands();
        projectList.update();
        UI.autoOpenToolbox();
        break;
      case "project-stopped":
        UI.destroyToolbox();
        this.updateCommands();
        projectList.update();
        break;
      case "runtime-global-actors":
        this.updateCommands();
        projectList.update();
        break;
      case "runtime-details":
        this.updateRuntimeButton();
        break;
      case "runtime":
        this.updateRuntimeButton();
        this.saveLastConnectedRuntime();
        break;
      case "project-validated":
        this.updateTitle();
        this.updateCommands();
        this.updateProjectButton();
        this.updateProjectEditorHeader();
        projectList.update();
        break;
      case "project-removed":
        projectList.update();
        break;
      case "install-progress":
        this.updateProgress(Math.round(100 * details.bytesSent / details.totalBytes));
        break;
      case "runtime-targets":
        this.autoSelectProject();
        projectList.update(details);
        break;
      case "pre-package":
        this.prePackageLog(details);
        break;
    };
    this._updatePromise = promise.resolve();
  },

  openInBrowser: function(url) {
    
    let browserWin = Services.wm.getMostRecentWindow("navigator:browser");
    if (browserWin) {
      let gBrowser = browserWin.gBrowser;
      gBrowser.selectedTab = gBrowser.addTab(url);
      browserWin.focus();
    } else {
      window.open(url);
    }
  },

  updateTitle: function() {
    let project = AppManager.selectedProject;
    if (project) {
      window.document.title = Strings.formatStringFromName("title_app", [project.name], 1);
    } else {
      window.document.title = Strings.GetStringFromName("title_noApp");
    }
  },

  
  hidePanels: function() {
    let panels = document.querySelectorAll("panel");
    for (let p of panels) {
      p.hidePopup();
    }
  },

  

  _busyTimeout: null,
  _busyOperationDescription: null,
  _busyPromise: null,

  updateProgress: function(percent) {
    let progress = document.querySelector("#action-busy-determined");
    progress.mode = "determined";
    progress.value = percent;
    this.setupBusyTimeout();
  },

  busy: function() {
    this.hidePanels();
    let win = document.querySelector("window");
    win.classList.add("busy")
    win.classList.add("busy-undetermined");
    this.updateCommands();
  },

  unbusy: function() {
    let win = document.querySelector("window");
    win.classList.remove("busy")
    win.classList.remove("busy-determined");
    win.classList.remove("busy-undetermined");
    this.updateCommands();
    this._busyPromise = null;
  },

  setupBusyTimeout: function() {
    this.cancelBusyTimeout();
    this._busyTimeout = setTimeout(() => {
      this.unbusy();
      UI.reportError("error_operationTimeout", this._busyOperationDescription);
    }, Services.prefs.getIntPref("devtools.webide.busyTimeout"));
  },

  cancelBusyTimeout: function() {
    clearTimeout(this._busyTimeout);
  },

  busyWithProgressUntil: function(promise, operationDescription) {
    let busy = this.busyUntil(promise, operationDescription);
    let win = document.querySelector("window");
    let progress = document.querySelector("#action-busy-determined");
    progress.mode = "undetermined";
    win.classList.add("busy-determined");
    win.classList.remove("busy-undetermined");
    return busy;
  },

  busyUntil: function(promise, operationDescription) {
    
    
    this._busyPromise = promise;
    this._busyOperationDescription = operationDescription;
    this.setupBusyTimeout();
    this.busy();
    promise.then(() => {
      this.cancelBusyTimeout();
      this.unbusy();
    }, (e) => {
      let message;
      if (e && e.error && e.message) {
        
        
        message = operationDescription + " (" + e.error + "): " + e.message;
      } else {
        message = operationDescription + (e ? (": " + e) : "");
      }
      this.cancelBusyTimeout();
      let operationCanceled = e && e.canceled;
      if (!operationCanceled) {
        UI.reportError("error_operationFail", message);
        if (e) {
          console.error(e);
        }
      }
      this.unbusy();
    });
    return promise;
  },

  reportError: function(l10nProperty, ...l10nArgs) {
    let text;

    if (l10nArgs.length > 0) {
      text = Strings.formatStringFromName(l10nProperty, l10nArgs, l10nArgs.length);
    } else {
      text = Strings.GetStringFromName(l10nProperty);
    }

    console.error(text);

    let buttons = [{
      label: Strings.GetStringFromName("notification_showTroubleShooting_label"),
      accessKey: Strings.GetStringFromName("notification_showTroubleShooting_accesskey"),
      callback: function () {
        Cmds.showTroubleShooting();
      }
    }];

    let nbox = document.querySelector("#notificationbox");
    nbox.removeAllNotifications(true);
    nbox.appendNotification(text, "webide:errornotification", null,
                            nbox.PRIORITY_WARNING_LOW, buttons);
  },

  dismissErrorNotification: function() {
    let nbox = document.querySelector("#notificationbox");
    nbox.removeAllNotifications(true);
  },

  

  updateRuntimeList: function() {
    let wifiHeaderNode = document.querySelector("#runtime-header-wifi");
    if (WiFiScanner.allowed) {
      wifiHeaderNode.removeAttribute("hidden");
    } else {
      wifiHeaderNode.setAttribute("hidden", "true");
    }

    let usbListNode = document.querySelector("#runtime-panel-usb");
    let wifiListNode = document.querySelector("#runtime-panel-wifi");
    let simulatorListNode = document.querySelector("#runtime-panel-simulator");
    let otherListNode = document.querySelector("#runtime-panel-other");

    let noHelperNode = document.querySelector("#runtime-panel-noadbhelper");
    let noUSBNode = document.querySelector("#runtime-panel-nousbdevice");

    if (Devices.helperAddonInstalled) {
      noHelperNode.setAttribute("hidden", "true");
    } else {
      noHelperNode.removeAttribute("hidden");
    }

    let runtimeList = AppManager.runtimeList;

    if (runtimeList.usb.length === 0 && Devices.helperAddonInstalled) {
      noUSBNode.removeAttribute("hidden");
    } else {
      noUSBNode.setAttribute("hidden", "true");
    }

    for (let [type, parent] of [
      ["usb", usbListNode],
      ["wifi", wifiListNode],
      ["simulator", simulatorListNode],
      ["other", otherListNode],
    ]) {
      while (parent.hasChildNodes()) {
        parent.firstChild.remove();
      }
      for (let runtime of runtimeList[type]) {
        let r = runtime;
        let panelItemNode = document.createElement("hbox");
        panelItemNode.className = "panel-item-complex";

        let connectButton = document.createElement("toolbarbutton");
        connectButton.className = "panel-item runtime-panel-item-" + type;
        connectButton.setAttribute("label", r.name);
        connectButton.setAttribute("flex", "1");
        connectButton.addEventListener("click", () => {
          this.hidePanels();
          this.dismissErrorNotification();
          this.connectToRuntime(r);
        }, true);
        panelItemNode.appendChild(connectButton);

        if (r.configure && UI.isRuntimeConfigurationEnabled()) {
          let configButton = document.createElement("toolbarbutton");
          configButton.className = "configure-button";
          configButton.addEventListener("click", r.configure.bind(r), true);
          panelItemNode.appendChild(configButton);
        }

        parent.appendChild(panelItemNode);
      }
    }
  },

  get lastConnectedRuntime() {
    return Services.prefs.getCharPref("devtools.webide.lastConnectedRuntime");
  },

  set lastConnectedRuntime(runtime) {
    Services.prefs.setCharPref("devtools.webide.lastConnectedRuntime", runtime);
  },

  autoConnectRuntime: function () {
    
    
    if (AppManager.selectedRuntime ||
        !Services.prefs.getBoolPref("devtools.webide.autoConnectRuntime") ||
        !this.lastConnectedRuntime) {
      return;
    }
    let [_, type, id] = this.lastConnectedRuntime.match(/^(\w+):(.+)$/);

    type = type.toLowerCase();

    
    if (type == "local") {
      type = "other";
    }

    
    
    if (type == "usb" || type == "wifi" || type == "other") {
      for (let runtime of AppManager.runtimeList[type]) {
        
        
        if (runtime.id == id) {
          
          this.lastConnectedRuntime = "";
          this.connectToRuntime(runtime);
        }
      }
    }
  },

  connectToRuntime: function(runtime) {
    let name = runtime.name;
    let promise = AppManager.connectToRuntime(runtime);
    promise.then(() => this.initConnectionTelemetry())
           .catch(() => {
             
             
             
           });
    promise = this.busyUntil(promise, "Connecting to " + name);
    
    
    if (runtime.prolongedConnection) {
      this.cancelBusyTimeout();
    }
    return promise;
  },

  updateRuntimeButton: function() {
    let labelNode = document.querySelector("#runtime-panel-button > .panel-button-label");
    if (!AppManager.selectedRuntime) {
      labelNode.setAttribute("value", Strings.GetStringFromName("runtimeButton_label"));
    } else {
      let name = AppManager.selectedRuntime.name;
      labelNode.setAttribute("value", name);
    }
  },

  saveLastConnectedRuntime: function () {
    if (AppManager.selectedRuntime &&
        AppManager.selectedRuntime.id !== undefined) {
      this.lastConnectedRuntime = AppManager.selectedRuntime.type + ":" +
                                  AppManager.selectedRuntime.id;
    } else {
      this.lastConnectedRuntime = "";
    }
  },

  _actionsToLog: new Set(),

  




  initConnectionTelemetry: function() {
    this._actionsToLog.add("play");
    this._actionsToLog.add("debug");
  },

  



  onAction: function(action) {
    if (!this._actionsToLog.has(action)) {
      return;
    }
    this.logActionState(action, true);
    this._actionsToLog.delete(action);
  },

  



  updateConnectionTelemetry: function() {
    for (let action of this._actionsToLog.values()) {
      this.logActionState(action, false);
    }
    this._actionsToLog.clear();
  },

  logActionState: function(action, state) {
    let histogramId = "DEVTOOLS_WEBIDE_CONNECTION_" +
                      action.toUpperCase() + "_USED";
    this._telemetry.log(histogramId, state);
  },

  

  

  updateProjectButton: function() {
    let buttonNode = document.querySelector("#project-panel-button");
    let labelNode = buttonNode.querySelector(".panel-button-label");
    let imageNode = buttonNode.querySelector(".panel-button-image");

    let project = AppManager.selectedProject;

    if (!projectList.sidebarsEnabled) {
      if (!project) {
        buttonNode.classList.add("no-project");
        labelNode.setAttribute("value", Strings.GetStringFromName("projectButton_label"));
        imageNode.removeAttribute("src");
      } else {
        buttonNode.classList.remove("no-project");
        labelNode.setAttribute("value", project.name);
        imageNode.setAttribute("src", project.icon);
      }
    }
  },

  

  destroyProjectEditor: function() {
    if (this.projecteditor) {
      this.projecteditor.destroy();
      this.projecteditor = null;
    }
  },

  updateProjectEditorMenusVisibility: function() {
    if (this.projecteditor) {
      let panel = document.querySelector("#deck").selectedPanel;
      if (panel && panel.id == "deck-panel-projecteditor") {
        this.projecteditor.menuEnabled = true;
      } else {
        this.projecteditor.menuEnabled = false;
      }
    }
  },

  getProjectEditor: function() {
    if (this.projecteditor) {
      return this.projecteditor.loaded;
    }

    let projecteditorIframe = document.querySelector("#deck-panel-projecteditor");
    this.projecteditor = ProjectEditor.ProjectEditor(projecteditorIframe, {
      menubar: document.querySelector("#main-menubar"),
      menuindex: 1
    });
    this.projecteditor.on("onEditorSave", (editor, resource) => {
      AppManager.validateAndUpdateProject(AppManager.selectedProject);
    });
    return this.projecteditor.loaded;
  },

  updateProjectEditorHeader: function() {
    let project = AppManager.selectedProject;
    if (!project || !this.projecteditor) {
      return;
    }
    let status = project.validationStatus || "unknown";
    if (status == "error warning") {
      status = "error";
    }
    this.getProjectEditor().then((projecteditor) => {
      projecteditor.setProjectToAppPath(project.location, {
        name: project.name,
        iconUrl: project.icon,
        projectOverviewURL: "chrome://webide/content/details.xhtml",
        validationStatus: status
      }).then(null, console.error);
    }, console.error);
  },

  isProjectEditorEnabled: function() {
    return Services.prefs.getBoolPref("devtools.webide.showProjectEditor");
  },

  isRuntimeConfigurationEnabled: function() {
    return Services.prefs.getBoolPref("devtools.webide.enableRuntimeConfiguration");
  },

  openProject: function() {
    let project = AppManager.selectedProject;

    

    if (!project) {
      this.resetDeck();
      return;
    }

    

    let forceDetailsOnly = false;
    if (project.type == "packaged") {
      forceDetailsOnly = !utils.doesFileExist(project.location);
    }

    

    if (project.type != "packaged" ||
        !this.isProjectEditorEnabled() ||
        forceDetailsOnly) {
      this.selectDeckPanel("details");
      return;
    }

    

    this.selectDeckPanel("projecteditor");

    this.getProjectEditor().then(() => {
      this.updateProjectEditorHeader();
    }, console.error);
  },

  autoStartProject: Task.async(function*() {
    let project = AppManager.selectedProject;

    if (!project) {
      return;
    }
    if (!(project.type == "runtimeApp" ||
          project.type == "mainProcess" ||
          project.type == "tab")) {
      return; 
    }

    
    
    if (project.type == "runtimeApp" && !AppManager.isProjectRunning()) {
      yield UI.busyUntil(AppManager.launchRuntimeApp(), "running app");
    }
  }),

  autoOpenToolbox: Task.async(function*() {
    let project = AppManager.selectedProject;

    if (!project) {
      return;
    }
    if (!(project.type == "runtimeApp" ||
          project.type == "mainProcess" ||
          project.type == "tab")) {
      return; 
    }

    yield UI.createToolbox();
  }),

  importAndSelectApp: Task.async(function* (source) {
    let isPackaged = !!source.path;
    let project;
    try {
      project = yield AppProjects[isPackaged ? "addPackaged" : "addHosted"](source);
    } catch (e) {
      if (e === "Already added") {
        
        
        project = AppProjects.get(isPackaged ? source.path : source);
      } else {
        throw e;
      }
    }

    
    AppManager.selectedProject = project;
  }),

  
  saveLastSelectedProject: function() {
    let shouldRestore = Services.prefs.getBoolPref("devtools.webide.restoreLastProject");
    if (!shouldRestore) {
      return;
    }

    
    if (!AppManager.connected) {
      return;
    }

    let project = "", type = "";
    let selected = AppManager.selectedProject;
    if (selected) {
      if (selected.type == "runtimeApp") {
        type = "runtimeApp";
        project = selected.app.manifestURL;
      } else if (selected.type == "mainProcess") {
        type = "mainProcess";
      } else if (selected.type == "packaged" ||
                 selected.type == "hosted") {
        type = "local";
        project = selected.location;
      }
    }
    if (type) {
      Services.prefs.setCharPref("devtools.webide.lastSelectedProject",
                                 type + ":" + project);
    } else {
      Services.prefs.clearUserPref("devtools.webide.lastSelectedProject");
    }
  },

  autoSelectProject: function() {
    if (AppManager.selectedProject) {
      return;
    }
    let shouldRestore = Services.prefs.getBoolPref("devtools.webide.restoreLastProject");
    if (!shouldRestore) {
      return;
    }
    let pref = Services.prefs.getCharPref("devtools.webide.lastSelectedProject");
    if (!pref) {
      return;
    }
    let m = pref.match(/^(\w+):(.*)$/);
    if (!m) {
      return;
    }
    let [_, type, project] = m;

    if (type == "local") {
      let lastProject = AppProjects.get(project);
      if (lastProject) {
        AppManager.selectedProject = lastProject;
      }
    }

    
    if (!AppManager.connected) {
      return;
    }

    if (type == "mainProcess" && AppManager.isMainProcessDebuggable()) {
      AppManager.selectedProject = {
        type: "mainProcess",
        name: Strings.GetStringFromName("mainProcess_label"),
        icon: AppManager.DEFAULT_PROJECT_ICON
      }
    } else if (type == "runtimeApp") {
      let app = AppManager.apps.get(project);
      if (app) {
        AppManager.selectedProject = {
          type: "runtimeApp",
          app: app.manifest,
          icon: app.iconURL,
          name: app.manifest.name
        };
      }
    }
  },

  

  setupDeck: function() {
    let iframes = document.querySelectorAll("#deck > iframe");
    for (let iframe of iframes) {
      iframe.tooltip = "aHTMLTooltip";
    }
  },

  resetFocus: function() {
    document.commandDispatcher.focusedElement = document.documentElement;
  },

  selectDeckPanel: function(id) {
    let deck = document.querySelector("#deck");
    if (deck.selectedPanel && deck.selectedPanel.id === "deck-panel-" + id) {
      
      return;
    }
    this.hidePanels();
    this.resetFocus();
    let panel = deck.querySelector("#deck-panel-" + id);
    let lazysrc = panel.getAttribute("lazysrc");
    if (lazysrc) {
      panel.removeAttribute("lazysrc");
      panel.setAttribute("src", lazysrc);
    }
    deck.selectedPanel = panel;
    this.updateProjectEditorMenusVisibility();
    this.updateToolboxFullscreenState();
  },

  resetDeck: function() {
    this.resetFocus();
    let deck = document.querySelector("#deck");
    deck.selectedPanel = null;
    this.updateProjectEditorMenusVisibility();
  },

  

  updateCommands: function() {

    if (document.querySelector("window").classList.contains("busy")) {
      document.querySelector("#cmd_newApp").setAttribute("disabled", "true");
      document.querySelector("#cmd_importPackagedApp").setAttribute("disabled", "true");
      document.querySelector("#cmd_importHostedApp").setAttribute("disabled", "true");
      document.querySelector("#cmd_showProjectPanel").setAttribute("disabled", "true");
      document.querySelector("#cmd_showRuntimePanel").setAttribute("disabled", "true");
      document.querySelector("#cmd_removeProject").setAttribute("disabled", "true");
      document.querySelector("#cmd_disconnectRuntime").setAttribute("disabled", "true");
      document.querySelector("#cmd_showPermissionsTable").setAttribute("disabled", "true");
      document.querySelector("#cmd_takeScreenshot").setAttribute("disabled", "true");
      document.querySelector("#cmd_showRuntimeDetails").setAttribute("disabled", "true");
      document.querySelector("#cmd_play").setAttribute("disabled", "true");
      document.querySelector("#cmd_stop").setAttribute("disabled", "true");
      document.querySelector("#cmd_toggleToolbox").setAttribute("disabled", "true");
      document.querySelector("#cmd_showDevicePrefs").setAttribute("disabled", "true");
      document.querySelector("#cmd_showSettings").setAttribute("disabled", "true");
      return;
    }

    document.querySelector("#cmd_newApp").removeAttribute("disabled");
    document.querySelector("#cmd_importPackagedApp").removeAttribute("disabled");
    document.querySelector("#cmd_importHostedApp").removeAttribute("disabled");
    document.querySelector("#cmd_showProjectPanel").removeAttribute("disabled");
    document.querySelector("#cmd_showRuntimePanel").removeAttribute("disabled");

    
    let playCmd = document.querySelector("#cmd_play");
    let stopCmd = document.querySelector("#cmd_stop");
    let debugCmd = document.querySelector("#cmd_toggleToolbox");
    let playButton = document.querySelector('#action-button-play');

    if (!AppManager.selectedProject || !AppManager.connected) {
      playCmd.setAttribute("disabled", "true");
      stopCmd.setAttribute("disabled", "true");
      debugCmd.setAttribute("disabled", "true");
    } else {
      let isProjectRunning = AppManager.isProjectRunning();
      if (isProjectRunning) {
        playButton.classList.add("reload");
        stopCmd.removeAttribute("disabled");
        debugCmd.removeAttribute("disabled");
      } else {
        playButton.classList.remove("reload");
        stopCmd.setAttribute("disabled", "true");
        debugCmd.setAttribute("disabled", "true");
      }

      
      if (AppManager.selectedProject.type == "runtimeApp") {
        playCmd.removeAttribute("disabled");
      } else if (AppManager.selectedProject.type == "tab") {
        playCmd.removeAttribute("disabled");
        stopCmd.setAttribute("disabled", "true");
      } else if (AppManager.selectedProject.type == "mainProcess") {
        playCmd.setAttribute("disabled", "true");
        stopCmd.setAttribute("disabled", "true");
      } else {
        if (AppManager.selectedProject.errorsCount == 0 &&
            AppManager.runtimeCanHandleApps()) {
          playCmd.removeAttribute("disabled");
        } else {
          playCmd.setAttribute("disabled", "true");
        }
      }
    }

    
    let removeCmdNode = document.querySelector("#cmd_removeProject");
    if (AppManager.selectedProject) {
      removeCmdNode.removeAttribute("disabled");
    } else {
      removeCmdNode.setAttribute("disabled", "true");
    }

    
    let screenshotCmd = document.querySelector("#cmd_takeScreenshot");
    let permissionsCmd = document.querySelector("#cmd_showPermissionsTable");
    let detailsCmd = document.querySelector("#cmd_showRuntimeDetails");
    let disconnectCmd = document.querySelector("#cmd_disconnectRuntime");
    let devicePrefsCmd = document.querySelector("#cmd_showDevicePrefs");
    let settingsCmd = document.querySelector("#cmd_showSettings");

    let box = document.querySelector("#runtime-actions");

    let runtimePanelButton = document.querySelector("#runtime-panel-button");
    if (AppManager.connected) {
      if (AppManager.deviceFront) {
        detailsCmd.removeAttribute("disabled");
        permissionsCmd.removeAttribute("disabled");
        screenshotCmd.removeAttribute("disabled");
      }
      if (AppManager.preferenceFront) {
        devicePrefsCmd.removeAttribute("disabled");
      }
      if (AppManager.settingsFront) {
        settingsCmd.removeAttribute("disabled");
      }
      disconnectCmd.removeAttribute("disabled");
      runtimePanelButton.setAttribute("active", "true");
    } else {
      detailsCmd.setAttribute("disabled", "true");
      permissionsCmd.setAttribute("disabled", "true");
      screenshotCmd.setAttribute("disabled", "true");
      disconnectCmd.setAttribute("disabled", "true");
      devicePrefsCmd.setAttribute("disabled", "true");
      settingsCmd.setAttribute("disabled", "true");
      runtimePanelButton.removeAttribute("active");
    }

  },

  

  onMessage: function(event) {
    
    
    try {
      let json = JSON.parse(event.data);
      switch (json.name) {
        case "toolbox-close":
          
          
          
          
          
          
          
          
          if (this.toolboxIframe && this.toolboxIframe.uid == json.uid) {
            this.toolboxPromise = null;
            this._closeToolboxUI();
          }
          break;
      }
    } catch(e) { console.error(e); }
  },

  destroyToolbox: function() {
    
    if (this.toolboxPromise) {
      let toolboxPromise = this.toolboxPromise;
      this.toolboxPromise = null;
      return toolboxPromise.then(toolbox => {
        return toolbox.destroy();
      }).then(null, console.error)
        .then(() => this._closeToolboxUI())
        .then(null, console.error);
    }
    return promise.resolve();
  },

  createToolbox: function() {
    
    if (this.toolboxPromise) {
      return this.toolboxPromise;
    }
    this.toolboxPromise = AppManager.getTarget().then((target) => {
      return this._showToolbox(target);
    }, console.error);
    return this.busyUntil(this.toolboxPromise, "opening toolbox");
  },

  _showToolbox: function(target) {
    let splitter = document.querySelector(".devtools-horizontal-splitter");
    splitter.removeAttribute("hidden");

    let iframe = document.createElement("iframe");
    iframe.id = "toolbox";

    
    
    iframe.uid = new Date().getTime();

    document.querySelector("notificationbox").insertBefore(iframe, splitter.nextSibling);
    let host = devtools.Toolbox.HostType.CUSTOM;
    let options = { customIframe: iframe, zoom: false, uid: iframe.uid };
    this.toolboxIframe = iframe;

    let height = Services.prefs.getIntPref("devtools.toolbox.footer.height");
    iframe.height = height;

    document.querySelector("#action-button-debug").setAttribute("active", "true");

    this.updateToolboxFullscreenState();
    return gDevTools.showToolbox(target, null, host, options);
  },

  updateToolboxFullscreenState: function() {
    let panel = document.querySelector("#deck").selectedPanel;
    let nbox = document.querySelector("#notificationbox");
    if (panel && panel.id == "deck-panel-details" &&
        AppManager.selectedProject &&
        AppManager.selectedProject.type != "packaged" &&
        this.toolboxIframe) {
      nbox.setAttribute("toolboxfullscreen", "true");
    } else {
      nbox.removeAttribute("toolboxfullscreen");
    }
  },

  _closeToolboxUI: function() {
    if (!this.toolboxIframe) {
      return;
    }

    this.resetFocus();
    Services.prefs.setIntPref("devtools.toolbox.footer.height", this.toolboxIframe.height);

    
    
    this.toolboxIframe.remove();
    this.toolboxIframe = null;

    let splitter = document.querySelector(".devtools-horizontal-splitter");
    splitter.setAttribute("hidden", "true");
    document.querySelector("#action-button-debug").removeAttribute("active");
    this.updateToolboxFullscreenState();
  },

  prePackageLog: function (msg) {
    if (msg == "start") {
      UI.selectDeckPanel("logs");
    }
  }
};

let Cmds = {
  quit: function() {
    if (UI.canCloseProject()) {
      window.close();
    }
  },

  






  newApp: function(testOptions) {
    projectList.newApp(testOptions);
  },

  importPackagedApp: function(location) {
    projectList.importPackagedApp(location);
  },

  importHostedApp: function(location) {
    projectList.importHostedApp(location);
  },

  showProjectPanel: function() {
    ProjectPanel.toggle(projectList.sidebarsEnabled, true);

    
    
    
    
    if (!projectList.sidebarsEnabled && AppManager.connected) {
      return AppManager.listTabs().then(() => {
        projectList.updateTabs();
      }).catch(console.error);
    }

    return promise.resolve();
  },

  showRuntimePanel: function() {
    RuntimeScanners.scan();

    let panel = document.querySelector("#runtime-panel");
    let anchor = document.querySelector("#runtime-panel-button > .panel-button-anchor");

    let deferred = promise.defer();
    function onPopupShown() {
      panel.removeEventListener("popupshown", onPopupShown);
      deferred.resolve();
    }
    panel.addEventListener("popupshown", onPopupShown);

    panel.openPopup(anchor);
    return deferred.promise;
  },

  disconnectRuntime: function() {
    let disconnecting = Task.spawn(function*() {
      yield UI.destroyToolbox();
      yield AppManager.disconnectRuntime();
    });
    return UI.busyUntil(disconnecting, "disconnecting from runtime");
  },

  takeScreenshot: function() {
    return UI.busyUntil(AppManager.deviceFront.screenshotToDataURL().then(longstr => {
       return longstr.string().then(dataURL => {
         longstr.release().then(null, console.error);
         UI.openInBrowser(dataURL);
       });
    }), "taking screenshot");
  },

  showPermissionsTable: function() {
    UI.selectDeckPanel("permissionstable");
  },

  showRuntimeDetails: function() {
    UI.selectDeckPanel("runtimedetails");
  },

  showDevicePrefs: function() {
    UI.selectDeckPanel("devicepreferences");
  },

  showSettings: function() {
    UI.selectDeckPanel("devicesettings");
  },

  showMonitor: function() {
    UI.selectDeckPanel("monitor");
  },

  play: function() {
    let busy;
    switch(AppManager.selectedProject.type) {
      case "packaged":
        busy = UI.busyWithProgressUntil(AppManager.installAndRunProject(),
                                        "installing and running app");
        break;
      case "hosted":
        busy = UI.busyUntil(AppManager.installAndRunProject(),
                            "installing and running app");
        break;
      case "runtimeApp":
        busy = UI.busyUntil(AppManager.launchOrReloadRuntimeApp(), "launching / reloading app");
        break;
      case "tab":
        busy = UI.busyUntil(AppManager.reloadTab(), "reloading tab");
        break;
    }
    if (!busy) {
      return promise.reject();
    }
    UI.onAction("play");
    return busy;
  },

  stop: function() {
    return UI.busyUntil(AppManager.stopRunningApp(), "stopping app");
  },

  toggleToolbox: function() {
    UI.onAction("debug");
    if (UI.toolboxIframe) {
      UI.destroyToolbox();
      return promise.resolve();
    } else {
      return UI.createToolbox();
    }
  },

  removeProject: function() {
    AppManager.removeSelectedProject();
  },

  toggleEditors: function() {
    let isNowEnabled = !UI.isProjectEditorEnabled();
    Services.prefs.setBoolPref("devtools.webide.showProjectEditor", isNowEnabled);
    if (!isNowEnabled) {
      UI.destroyProjectEditor();
    }
    UI.openProject();
  },

  showTroubleShooting: function() {
    UI.openInBrowser(HELP_URL);
  },

  showAddons: function() {
    UI.selectDeckPanel("addons");
  },

  showPrefs: function() {
    UI.selectDeckPanel("prefs");
  },

  zoomIn: function() {
    if (UI.contentViewer.fullZoom < MAX_ZOOM) {
      UI.contentViewer.fullZoom += 0.1;
      Services.prefs.setCharPref("devtools.webide.zoom", UI.contentViewer.fullZoom);
    }
  },

  zoomOut: function() {
    if (UI.contentViewer.fullZoom > MIN_ZOOM) {
      UI.contentViewer.fullZoom -= 0.1;
      Services.prefs.setCharPref("devtools.webide.zoom", UI.contentViewer.fullZoom);
    }
  },

  resetZoom: function() {
    UI.contentViewer.fullZoom = 1;
    Services.prefs.setCharPref("devtools.webide.zoom", 1);
  },
};
