



const {Cu} = require("chrome");

const {Services} = Cu.import("resource://gre/modules/Services.jsm");
const {AppProjects} = require("devtools/app-manager/app-projects");
const {AppManager} = require("devtools/webide/app-manager");
const {Promise: promise} = Cu.import("resource://gre/modules/Promise.jsm", {});
const EventEmitter = require("devtools/toolkit/event-emitter");
const {Task} = Cu.import("resource://gre/modules/Task.jsm", {});
const utils = require("devtools/webide/utils");

const Strings = Services.strings.createBundle("chrome://browser/locale/devtools/webide.properties");

let ProjectList;

module.exports = ProjectList = function(window, parentWindow) {
  EventEmitter.decorate(this);
  this._doc = window.document;
  this._UI = parentWindow.UI;
  this._parentWindow = parentWindow;
  this._panelNodeEl = "toolbarbutton";
  this._sidebarsEnabled = Services.prefs.getBoolPref("devtools.webide.sidebars");

  if (this._sidebarsEnabled) {
    this._panelNodeEl = "div";
  }

  AppManager.init();

  return this;
};

ProjectList.prototype = {
  get doc() {
    return this._doc;
  },

  get sidebarsEnabled() {
    return this._sidebarsEnabled;
  },

  






  newApp: function(testOptions) {
    let parentWindow = this._parentWindow;
    return this._UI.busyUntil(Task.spawn(function*() {
      
      let ret = {location: null, testOptions: testOptions};
      parentWindow.openDialog("chrome://webide/content/newapp.xul", "newapp", "chrome,modal", ret);
      if (!ret.location)
        return;

      
      let project = AppProjects.get(ret.location);

      
      AppManager.selectedProject = project;
    }), "creating new app");
  },

  importPackagedApp: function(location) {
    let parentWindow = this._parentWindow;
    let UI = this._UI;
    return UI.busyUntil(Task.spawn(function*() {
      let directory = utils.getPackagedDirectory(parentWindow, location);

      if (!directory) {
        
        return;
      }

      yield UI.importAndSelectApp(directory);
    }), "importing packaged app");
  },

  importHostedApp: function(location) {
    let parentWindow = this._parentWindow;
    let UI = this._UI;
    return UI.busyUntil(Task.spawn(function*() {
      let url = utils.getHostedURL(parentWindow, location);

      if (!url) {
        return;
      }

      yield UI.importAndSelectApp(url);
    }), "importing hosted app");
  },

  






  _renderProjectItem: function(opts) {
    if (this._sidebarsEnabled) {
      let span = this._doc.createElement("span");
      span.textContent = opts.name;
      let icon = this._doc.createElement("img");
      icon.className = "project-image";
      icon.setAttribute("src", opts.icon);
      opts.panel.appendChild(icon);
      opts.panel.appendChild(span);
    } else {
      opts.panel.setAttribute("label", opts.name);
      opts.panel.setAttribute("image", opts.icon);
    }
  },

  _buildProjectPanelTabs: function() {
    let tabs = AppManager.tabStore.tabs;
    let tabsHeaderNode = this._doc.querySelector("#panel-header-tabs");

    if (AppManager.connected && tabs.length > 0) {
      tabsHeaderNode.removeAttribute("hidden");
    } else {
      tabsHeaderNode.setAttribute("hidden", "true");
    }

    let tabsNode = this._doc.querySelector("#project-panel-tabs");

    while (tabsNode.hasChildNodes()) {
      tabsNode.firstChild.remove();
    }

    for (let i = 0; i < tabs.length; i++) {
      let tab = tabs[i];
      let URL = this._parentWindow.URL;
      let url;
      try {
        url = new URL(tab.url);
      } catch (e) {
        
        continue;
      }
      
      
      
      
      tab.favicon = url.origin + "/favicon.ico";
      tab.name = tab.title || Strings.GetStringFromName("project_tab_loading");
      if (url.protocol.startsWith("http")) {
        tab.name = url.hostname + ": " + tab.name;
      }
      let panelItemNode = this._doc.createElement(this._panelNodeEl);
      panelItemNode.className = "panel-item";
      tabsNode.appendChild(panelItemNode);
      this._renderProjectItem({
        panel: panelItemNode,
        name: tab.name,
        icon: tab.favicon
      });
      panelItemNode.addEventListener("click", () => {
        if (!this._sidebarsEnabled) {
          this._UI.hidePanels();
        }
        AppManager.selectedProject = {
          type: "tab",
          app: tab,
          icon: tab.favicon,
          location: tab.url,
          name: tab.name
        };
      }, true);
    }
  },

  update: function() {
    let deferred = promise.defer();
    let doc = this._doc;
    let panelVboxNode = doc.querySelector("#project-panel > #project-panel-box");
    let anchorNode = doc.querySelector("#project-panel-button > .panel-button-anchor");
    let projectsNode = doc.querySelector("#project-panel-projects");

    while (projectsNode.hasChildNodes()) {
      projectsNode.firstChild.remove();
    }

    AppProjects.load().then(() => {
      let projects = AppProjects.store.object.projects;
      for (let i = 0; i < projects.length; i++) {
        let project = projects[i];
        let panelItemNode = doc.createElement(this._panelNodeEl);
        panelItemNode.className = "panel-item";
        projectsNode.appendChild(panelItemNode);
        this._renderProjectItem({
          panel: panelItemNode,
          name: project.name || AppManager.DEFAULT_PROJECT_NAME,
          icon: project.icon|| AppManager.DEFAULT_PROJECT_ICON
        });
        if (!project.name || !project.icon) {
          
          
          
          AppManager.validateAndUpdateProject(project).then(() => {
            this._renderProjectItem({
              panel: panelItemNode,
              name: project.name,
              icon: project.icon
            });
          });
        }
        panelItemNode.addEventListener("click", () => {
          if (!this._sidebarsEnabled) {
            this._UI.hidePanels();
          }
          AppManager.selectedProject = project;
        }, true);
      }

      deferred.resolve();
    }, deferred.reject);

    let runtimeappsHeaderNode = doc.querySelector("#panel-header-runtimeapps");
    let sortedApps = [];
    for (let [manifestURL, app] of AppManager.apps) {
      sortedApps.push(app);
    }
    sortedApps = sortedApps.sort((a, b) => {
      return a.manifest.name > b.manifest.name;
    });
    let mainProcess = AppManager.isMainProcessDebuggable();
    if (AppManager.connected && (sortedApps.length > 0 || mainProcess)) {
      runtimeappsHeaderNode.removeAttribute("hidden");
    } else {
      runtimeappsHeaderNode.setAttribute("hidden", "true");
    }

    let runtimeAppsNode = doc.querySelector("#project-panel-runtimeapps");
    while (runtimeAppsNode.hasChildNodes()) {
      runtimeAppsNode.firstChild.remove();
    }

    if (mainProcess) {
      let panelItemNode = doc.createElement(this._panelNodeEl);
      panelItemNode.className = "panel-item";
      this._renderProjectItem({
        panel: panelItemNode,
        name: Strings.GetStringFromName("mainProcess_label"),
        icon: AppManager.DEFAULT_PROJECT_ICON
      });
      runtimeAppsNode.appendChild(panelItemNode);
      panelItemNode.addEventListener("click", () => {
        if (!this._sidebarsEnabled) {
          this._UI.hidePanels();
        }
        AppManager.selectedProject = {
          type: "mainProcess",
          name: Strings.GetStringFromName("mainProcess_label"),
          icon: AppManager.DEFAULT_PROJECT_ICON
        };
      }, true);
    }

    for (let i = 0; i < sortedApps.length; i++) {
      let app = sortedApps[i];
      let panelItemNode = doc.createElement(this._panelNodeEl);
      panelItemNode.className = "panel-item";
      this._renderProjectItem({
        panel: panelItemNode,
        name: app.manifest.name,
        icon: app.iconURL
      });
      runtimeAppsNode.appendChild(panelItemNode);
      panelItemNode.addEventListener("click", () => {
        if (!this._sidebarsEnabled) {
          this._UI.hidePanels();
        }
        AppManager.selectedProject = {
          type: "runtimeApp",
          app: app.manifest,
          icon: app.iconURL,
          name: app.manifest.name
        };
      }, true);
    }

    
    this._buildProjectPanelTabs();

    
    
    if (AppManager.connected) {
      AppManager.listTabs().then(() => {
        this._buildProjectPanelTabs();
      }).catch(console.error);
    }

    return deferred.promise;
  }
};
