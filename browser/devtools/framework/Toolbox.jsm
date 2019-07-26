



"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import('resource://gre/modules/XPCOMUtils.jsm');
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/commonjs/sdk/core/promise.js");
Cu.import("resource:///modules/devtools/EventEmitter.jsm");
Cu.import("resource:///modules/devtools/gDevTools.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Hosts",
                                  "resource:///modules/devtools/ToolboxHosts.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "CommandUtils",
                                  "resource:///modules/devtools/DeveloperToolbar.jsm");

XPCOMUtils.defineLazyGetter(this, "toolboxStrings", function() {
  let bundle = Services.strings.createBundle("chrome://browser/locale/devtools/toolbox.properties");
  let l10n = function(aName, ...aArgs) {
    try {
      if (aArgs.length == 0) {
        return bundle.GetStringFromName(aName);
      } else {
        return bundle.formatStringFromName(aName, aArgs, aArgs.length);
      }
    } catch (ex) {
      Services.console.logStringMessage("Error reading '" + aName + "'");
    }
  };
  return l10n;
});

XPCOMUtils.defineLazyGetter(this, "Requisition", function() {
  Cu.import("resource://gre/modules/devtools/Require.jsm");
  Cu.import("resource:///modules/devtools/gcli.jsm");

  return require('gcli/cli').Requisition;
});

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
  let definitions = gDevTools.getToolDefinitionMap();
  if (!definitions.get(selectedTool)) {
    selectedTool = "webconsole";
  }
  this._defaultToolId = selectedTool;

  this._host = this._createHost(hostType);

  EventEmitter.decorate(this);

  this._refreshHostTitle = this._refreshHostTitle.bind(this);
  this._target.on("navigate", this._refreshHostTitle);
  this.on("host-changed", this._refreshHostTitle);
  this.on("select", this._refreshHostTitle);

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

    this._host.create().then(function(iframe) {
      let domReady = function() {
        iframe.removeEventListener("DOMContentLoaded", domReady, true);

        this.isReady = true;

        let closeButton = this.doc.getElementById("toolbox-close");
        closeButton.addEventListener("command", this.destroy, true);

        this._buildDockButtons();
        this._buildTabs();
        this._buildButtons();
        this._addKeysToWindow();

        this.selectTool(this._defaultToolId).then(function(panel) {
          this.emit("ready");
          deferred.resolve();
        }.bind(this));
      }.bind(this);

      iframe.addEventListener("DOMContentLoaded", domReady, true);
      iframe.setAttribute("src", this._URL);
    }.bind(this));

    return deferred.promise;
  },

  


  _addKeysToWindow: function TBOX__addKeysToWindow() {
    if (this.hostType != Toolbox.HostType.WINDOW) {
      return;
    }
    let doc = this.doc.defaultView.parent.document;
    for (let [id, toolDefinition] of gDevTools._tools) {
      if (toolDefinition.key) {
        
        if (doc.getElementById("key_" + id)) {
          continue;
        }
        let key = doc.createElement("key");
        key.id = "key_" + id;

        if (toolDefinition.key.startsWith("VK_")) {
          key.setAttribute("keycode", toolDefinition.key);
        } else {
          key.setAttribute("key", toolDefinition.key);
        }

        key.setAttribute("modifiers", toolDefinition.modifiers);
        key.setAttribute("oncommand", "void(0);"); 
        key.addEventListener("command", function(toolId) {
          this.selectTool(toolId);
        }.bind(this, id), true);
        doc.getElementById("toolbox-keyset").appendChild(key);
      }
    }
  },

  



  _buildDockButtons: function TBOX_createDockButtons() {
    let dockBox = this.doc.getElementById("toolbox-dock-buttons");

    while (dockBox.firstChild) {
      dockBox.removeChild(dockBox.firstChild);
    }

    if (!this._target.isLocalTab) {
      return;
    }

    let closeButton = this.doc.getElementById("toolbox-close");
    if (this.hostType === this.HostType.WINDOW) {
      closeButton.setAttribute("hidden", "true");
    } else {
      closeButton.removeAttribute("hidden");
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
      button.setAttribute("tooltiptext", toolboxStrings("toolboxDockButtons." +
                                                        position + ".tooltip"));
      button.addEventListener("command", function(position) {
        this.switchHost(position);
      }.bind(this, position));

      dockBox.appendChild(button);
    }
  },

  


  _buildTabs: function TBOX_buildTabs() {
    for (let definition of gDevTools.getToolDefinitionArray()) {
      this._buildTabForTool(definition);
    }
  },

  


  _buildButtons: function TBOX_buildButtons() {
    if (!this.target.isLocalTab) {
      return;
    }

    let toolbarSpec = CommandUtils.getCommandbarSpec("devtools.toolbox.toolbarSpec");
    let environment = { chromeDocument: this.target.tab.ownerDocument };
    let requisition = new Requisition(environment);

    let buttons = CommandUtils.createButtons(toolbarSpec, this._target, this.doc, requisition);

    let container = this.doc.getElementById("toolbox-buttons");
    buttons.forEach(function(button) {
      container.appendChild(button);
    }.bind(this));
  },

  





  _buildTabForTool: function TBOX_buildTabForTool(toolDefinition) {
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
    radio.setAttribute("tooltiptext", toolDefinition.tooltip);
    if (toolDefinition.icon) {
      radio.setAttribute("src", toolDefinition.icon);
    }

    radio.addEventListener("command", function(id) {
      this.selectTool(id);
    }.bind(this, id));

    let vbox = this.doc.createElement("vbox");
    vbox.className = "toolbox-panel";
    vbox.id = "toolbox-panel-" + id;

    tabs.appendChild(radio);
    deck.appendChild(vbox);

    this._addKeysToWindow();
  },

  





  selectTool: function TBOX_selectTool(id) {
    if (this._currentToolId == id) {
      return;
    }

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

    let definition = gDevTools.getToolDefinitionMap().get(id);

    this._currentToolId = id;

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

    return deferred.promise;
  },

  


  raise: function TBOX_raise() {
    this._host.raise();
  },

  


  _refreshHostTitle: function TBOX_refreshHostTitle() {
    let toolName;
    let toolId = this.currentToolId;
    if (toolId) {
      let toolDef = gDevTools.getToolDefinitionMap().get(toolId);
      toolName = toolDef.label;
    } else {
      
      toolName = toolboxStrings("toolbox.defaultTitle");
    }
    let title = toolboxStrings("toolbox.titleTemplate",
                               toolName, this.target.url);
    this._host.setTitle(title);
  },

  












  _createHost: function TBOX_createHost(hostType) {
    if (!Hosts[hostType]) {
      throw new Error('Unknown hostType: '+ hostType);
    }
    let newHost = new Hosts[hostType](this.target.tab);

    
    newHost.on("window-closed", this.destroy);

    return newHost;
  },

  






  switchHost: function TBOX_switchHost(hostType) {
    if (hostType == this._host.type) {
      return;
    }

    if (!this._target.isLocalTab) {
      return;
    }

    let newHost = this._createHost(hostType);
    return newHost.create().then(function(iframe) {
      
      iframe.QueryInterface(Ci.nsIFrameLoaderOwner);
      iframe.swapFrameLoaders(this.frame);

      this._host.off("window-closed", this.destroy);
      this._host.destroy();

      this._host = newHost;

      Services.prefs.setCharPref(this._prefs.LAST_HOST, this._host.type);

      this._buildDockButtons();
      this._addKeysToWindow();

      this.emit("host-changed");
    }.bind(this));
  },

  






  _toolRegistered: function TBOX_toolRegistered(event, toolId) {
    let defs = gDevTools.getToolDefinitionMap();
    let tool = defs.get(toolId);

    this._buildTabForTool(tool);
  },

  






  _toolUnregistered: function TBOX_toolUnregistered(event, toolId) {
    let radio = this.doc.getElementById("toolbox-tab-" + toolId);
    let panel = this.doc.getElementById("toolbox-panel-" + toolId);

    if (radio) {
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
      radio.parentNode.removeChild(radio);
    }

    if (panel) {
      panel.parentNode.removeChild(panel);
    }

    if (this.hostType == Toolbox.HostType.WINDOW) {
      let doc = this.doc.defaultView.parent.document;
      let key = doc.getElementById("key_" + id);
      if (key) {
        key.parentNode.removeChild(key);
      }
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
    
    
    
    let deferred = Promise.defer();
    this._destroyer = deferred.promise;

    this._target.off("navigate", this._refreshHostTitle);
    this.off("select", this._refreshHostTitle);
    this.off("host-changed", this._refreshHostTitle);

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

    Promise.all(outstanding).then(function() {
      this.emit("destroyed");
      deferred.resolve();
    }.bind(this));

    return this._destroyer;
  }
};
