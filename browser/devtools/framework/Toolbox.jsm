



"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import('resource://gre/modules/XPCOMUtils.jsm');
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/commonjs/promise/core.js");
Cu.import("resource:///modules/devtools/EventEmitter.jsm");
Cu.import("resource:///modules/devtools/gDevTools.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Hosts",
                                  "resource:///modules/devtools/ToolboxHosts.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "CommandUtils",
                                  "resource:///modules/devtools/DeveloperToolbar.jsm");



Cu.import("resource:///modules/devtools/gcli.jsm");
Cu.import("resource://gre/modules/devtools/Require.jsm");

let Requisition = require('gcli/cli').Requisition;
let CommandOutputManager = require('gcli/canon').CommandOutputManager;

this.EXPORTED_SYMBOLS = [ "Toolbox" ];









Promise.promised = (function() {
  
  
  

  var call = Function.call;
  var concat = Array.prototype.concat;

  
  
  function execute(args) { return call.apply(call, args); }

  
  
  function promisedConcat(promises, unknown) {
    return promises.then(function(values) {
      return Promise.resolve(unknown).then(function(value) {
        return values.concat([ value ]);
      });
    });
  }

  return function promised(f, prototype) {
    











    return function promised() {
      
      return concat.apply([ f, this ], arguments).
          
          reduce(promisedConcat, Promise.resolve([], prototype)).
          
          then(execute);
    };
  };
})();






Promise.all = Promise.promised(Array);
















this.Toolbox = function Toolbox(target, selectedTool, hostType) {
  this._target = target;
  this._toolPanels = new Map();

  this._toolRegistered = this._toolRegistered.bind(this);
  this._toolUnregistered = this._toolUnregistered.bind(this);
  this.destroy = this.destroy.bind(this);

  this._target.once("close", this.destroy);

  if (!hostType) {
    hostType = Services.prefs.getCharPref(this._prefs.LAST_HOST);
  }
  if (!selectedTool) {
    selectedTool = Services.prefs.getCharPref(this._prefs.LAST_TOOL);
  }
  let definitions = gDevTools.getToolDefinitions();
  if (!definitions.get(selectedTool)) {
    selectedTool = "webconsole";
  }
  this._defaultToolId = selectedTool;

  this._host = this._createHost(hostType);

  new EventEmitter(this);

  gDevTools.on("tool-registered", this._toolRegistered);
  gDevTools.on("tool-unregistered", this._toolUnregistered);
}





Toolbox.HostType = {
  BOTTOM: "bottom",
  SIDE: "side",
  WINDOW: "window"
}

Toolbox.prototype = {
  _URL: "chrome://browser/content/devtools/framework/toolbox.xul",

  _prefs: {
    LAST_HOST: "devtools.toolbox.host",
    LAST_TOOL: "devtools.toolbox.selectedTool",
    SIDE_ENABLED: "devtools.toolbox.sideEnabled"
  },

  HostType: Toolbox.HostType,

  





  getToolPanels: function TB_getToolPanels() {
    let panels = new Map();

    for (let [key, value] of this._toolPanels) {
      panels.set(key, value);
    }
    return panels;
  },

  


  getPanel: function TBOX_getPanel(id) {
    return this.getToolPanels().get(id);
  },

  




  getCurrentPanel: function TBOX_getCurrentPanel() {
    return this.getToolPanels().get(this.currentToolId);
  },

  




  get target() {
    return this._target;
  },

  



  get hostType() {
    return this._host.type;
  },

  


  get currentToolId() {
    return this._currentToolId;
  },

  set currentToolId(value) {
    this._currentToolId = value;
  },

  


  get frame() {
    return this._host.frame;
  },

  


  get doc() {
    return this.frame.contentDocument;
  },

  


  open: function TBOX_open() {
    let deferred = Promise.defer();

    this._host.open().then(function(iframe) {
      let onload = function() {
        iframe.removeEventListener("DOMContentLoaded", onload, true);

        this.isReady = true;

        let closeButton = this.doc.getElementById("toolbox-close");
        closeButton.addEventListener("command", this.destroy, true);

        this._buildDockButtons();
        this._buildTabs();
        this._buildButtons(this.frame);

        this.selectTool(this._defaultToolId).then(function(panel) {
          this.emit("ready");
          deferred.resolve();
        }.bind(this));
      }.bind(this);

      iframe.addEventListener("DOMContentLoaded", onload, true);
      iframe.setAttribute("src", this._URL);
    }.bind(this));

    return deferred.promise;
  },

  



  _buildDockButtons: function TBOX_createDockButtons() {
    let dockBox = this.doc.getElementById("toolbox-dock-buttons");

    while (dockBox.firstChild) {
      dockBox.removeChild(dockBox.firstChild);
    }

    let sideEnabled = Services.prefs.getBoolPref(this._prefs.SIDE_ENABLED);

    for each (let position in this.HostType) {
      if (position == this.hostType ||
         (!sideEnabled && position == this.HostType.SIDE)) {
        continue;
      }

      let button = this.doc.createElement("toolbarbutton");
      button.id = "toolbox-dock-" + position;
      button.className = "toolbox-dock-button";
      button.addEventListener("command", function(position) {
        this.switchHost(position);
      }.bind(this, position));

      dockBox.appendChild(button);
    }
  },

  


  _buildTabs: function TBOX_buildTabs() {
    for (let [id, definition] of gDevTools.getToolDefinitions()) {
      this._buildTabForTool(definition);
    }
  },

  





  _buildButtons: function TBOX_buildButtons(frame) {
    if (this.target.isRemote) {
      return;
    }

    let toolbarSpec = CommandUtils.getCommandbarSpec("devtools.toolbox.toolbarSpec");
    let environment = { chromeDocument: frame.ownerDocument };
    let requisition = new Requisition(environment);
    requisition.commandOutputManager = new CommandOutputManager();

    let buttons = CommandUtils.createButtons(toolbarSpec, this.doc, requisition);

    let container = this.doc.getElementById("toolbox-buttons");
    buttons.forEach(function(button) {
      container.appendChild(button);
    }.bind(this));
  },

  





  _buildTabForTool: function TBOX_buildTabForTool(toolDefinition) {
    const MAX_ORDINAL = 99;
    if (!toolDefinition.isTargetSupported(this._target)) {
      return;
    }

    let tabs = this.doc.getElementById("toolbox-tabs");
    let deck = this.doc.getElementById("toolbox-deck");

    let id = toolDefinition.id;

    let radio = this.doc.createElement("radio");
    radio.setAttribute("label", toolDefinition.label);
    radio.className = "toolbox-tab devtools-tab";
    radio.id = "toolbox-tab-" + id;
    radio.setAttribute("toolid", id);

    let ordinal = (typeof toolDefinition.ordinal == "number") ?
                  toolDefinition.ordinal : MAX_ORDINAL;
    radio.setAttribute("ordinal", ordinal);

    radio.addEventListener("command", function(id) {
      this.selectTool(id);
    }.bind(this, id));

    let vbox = this.doc.createElement("vbox");
    vbox.className = "toolbox-panel";
    vbox.id = "toolbox-panel-" + id;

    tabs.appendChild(radio);
    deck.appendChild(vbox);
  },

  





  selectTool: function TBOX_selectTool(id) {
    let deferred = Promise.defer();

    if (!this.isReady) {
      throw new Error("Can't select tool, wait for toolbox 'ready' event");
    }
    let tab = this.doc.getElementById("toolbox-tab-" + id);

    if (!tab) {
      throw new Error("No tool found");
    }

    let tabstrip = this.doc.getElementById("toolbox-tabs");

    
    let index = -1;
    let tabs = tabstrip.childNodes;
    for (let i = 0; i < tabs.length; i++) {
      if (tabs[i] === tab) {
        index = i;
        break;
      }
    }
    tabstrip.selectedIndex = index;

    
    let deck = this.doc.getElementById("toolbox-deck");
    deck.selectedIndex = index;

    let definition = gDevTools.getToolDefinitions().get(id);

    let iframe = this.doc.getElementById("toolbox-panel-iframe-" + id);
    if (!iframe) {
      iframe = this.doc.createElement("iframe");
      iframe.className = "toolbox-panel-iframe";
      iframe.id = "toolbox-panel-iframe-" + id;
      iframe.setAttribute("flex", 1);

      let vbox = this.doc.getElementById("toolbox-panel-" + id);
      vbox.appendChild(iframe);

      let boundLoad = function() {
        iframe.removeEventListener("DOMContentLoaded", boundLoad, true);

        definition.build(iframe.contentWindow, this).then(function(panel) {
          this._toolPanels.set(id, panel);

          this.emit(id + "-ready", panel);
          this.emit("select", id);
          this.emit(id + "-selected", panel);
          gDevTools.emit(id + "-ready", this, panel);

          deferred.resolve(panel);
        }.bind(this));
      }.bind(this);

      iframe.addEventListener("DOMContentLoaded", boundLoad, true);
      iframe.setAttribute("src", definition.url);
    } else {
      let panel = this._toolPanels.get(id);
      
      if (panel) {
        this.emit("select", id);
        this.emit(id + "-selected", panel);
        deferred.resolve(panel);
      }
    }

    Services.prefs.setCharPref(this._prefs.LAST_TOOL, id);

    this._currentToolId = id;

    return deferred.promise;
  },

  








  _createHost: function TBOX_createHost(hostType) {
    let hostTab = this._getHostTab();
    if (!Hosts[hostType]) {
      throw new Error('Unknown hostType: '+ hostType);
    }
    let newHost = new Hosts[hostType](hostTab);

    
    newHost.on("window-closed", this.destroy);

    return newHost;
  },

  






  switchHost: function TBOX_switchHost(hostType) {
    if (hostType == this._host.type) {
      return;
    }

    let newHost = this._createHost(hostType);
    return newHost.open().then(function(iframe) {
      
      iframe.QueryInterface(Ci.nsIFrameLoaderOwner);
      iframe.swapFrameLoaders(this.frame);

      this._host.off("window-closed", this.destroy);
      this._host.destroy();

      this._host = newHost;

      Services.prefs.setCharPref(this._prefs.LAST_HOST, this._host.type);

      this._buildDockButtons();

      this.emit("host-changed");
    }.bind(this));
  },

  


  _getHostTab: function TBOX_getHostTab() {
    if (!this._target.isRemote && !this._target.isChrome) {
      return this._target.tab;
    } else {
      let win = Services.wm.getMostRecentWindow("navigator:browser");
      return win.gBrowser.selectedTab;
    }
  },

  






  _toolRegistered: function TBOX_toolRegistered(event, toolId) {
    let defs = gDevTools.getToolDefinitions();
    let tool = defs.get(toolId);

    this._buildTabForTool(tool);
  },

  






  _toolUnregistered: function TBOX_toolUnregistered(event, toolId) {
    let radio = this.doc.getElementById("toolbox-tab-" + toolId);
    let panel = this.doc.getElementById("toolbox-panel-" + toolId);

    if (this._currentToolId == toolId) {
      let nextToolName = null;
      if (radio.nextSibling) {
        nextToolName = radio.nextSibling.getAttribute("toolid");
      }
      if (radio.previousSibling) {
        nextToolName = radio.previousSibling.getAttribute("toolid");
      }
      if (nextToolName) {
        this.selectTool(nextToolName);
      }
    }

    if (radio) {
      radio.parentNode.removeChild(radio);
    }

    if (panel) {
      panel.parentNode.removeChild(panel);
    }

    if (this._toolPanels.has(toolId)) {
      let instance = this._toolPanels.get(toolId);
      instance.destroy();
      this._toolPanels.delete(toolId);
    }
  },


  




  getNotificationBox: function TBOX_getNotificationBox() {
    return this.doc.getElementById("toolbox-notificationbox");
  },

  


  destroy: function TBOX_destroy() {
    
    
    if (this._destroyer) {
      return this._destroyer;
    }

    let outstanding = [];

    
    if (this._target && this._target.isRemote) {
      outstanding.push(this._target.destroy());
    }
    this._target = null;

    for (let [id, panel] of this._toolPanels) {
      outstanding.push(panel.destroy());
    }

    outstanding.push(this._host.destroy());

    gDevTools.off("tool-registered", this._toolRegistered);
    gDevTools.off("tool-unregistered", this._toolUnregistered);

    this._destroyer = Promise.all(outstanding);
    this._destroyer.then(function() {
      this.emit("destroyed");
    }.bind(this));

    return this._destroyer.then(function() {
      
      
      return undefined;
    });
  }
};
