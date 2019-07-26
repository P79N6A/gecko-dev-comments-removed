



"use strict";

const {Cc, Ci, Cu} = require("chrome");
const MAX_ORDINAL = 99;
let Promise = require("sdk/core/promise");
let EventEmitter = require("devtools/shared/event-emitter");

Cu.import('resource://gre/modules/XPCOMUtils.jsm');
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource:///modules/devtools/gDevTools.jsm");

loader.lazyGetter(this, "Hosts", () => require("devtools/framework/toolbox-hosts").Hosts);

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
  let scope = {};
  Cu.import("resource://gre/modules/devtools/Require.jsm", scope);
  Cu.import("resource:///modules/devtools/gcli.jsm", scope);

  let req = scope.require;
  return req('gcli/cli').Requisition;
});













function Toolbox(target, selectedTool, hostType) {
  this._target = target;
  this._toolPanels = new Map();

  this._toolRegistered = this._toolRegistered.bind(this);
  this._toolUnregistered = this._toolUnregistered.bind(this);
  this.destroy = this.destroy.bind(this);

  this._target.on("close", this.destroy);

  if (!hostType) {
    hostType = Services.prefs.getCharPref(this._prefs.LAST_HOST);
  }
  if (!selectedTool) {
    selectedTool = Services.prefs.getCharPref(this._prefs.LAST_TOOL);
  }
  let definitions = gDevTools.getToolDefinitionMap();
  if (!definitions.get(selectedTool) && selectedTool != "options") {
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
exports.Toolbox = Toolbox;





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
        this._buildOptions();
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

  _buildOptions: function TBOX__buildOptions() {
    this.optionsButton = this.doc.getElementById("toolbox-tab-options");
    this.optionsButton.addEventListener("command", function() {
      this.selectTool("options");
    }.bind(this), false);

    let iframe = this.doc.getElementById("toolbox-panel-iframe-options");
    this._toolPanels.set("options", iframe);

    let key = this.doc.getElementById("toolbox-options-key");
    key.addEventListener("command", function(toolId) {
      this.selectTool(toolId);
    }.bind(this, "options"), true);
  },

  


  _addKeysToWindow: function TBOX__addKeysToWindow() {
    if (this.hostType != Toolbox.HostType.WINDOW) {
      return;
    }
    let doc = this.doc.defaultView.parent.document;
    for (let [id, toolDefinition] of gDevTools.getToolDefinitionMap()) {
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
    radio.className = "toolbox-tab devtools-tab";
    radio.id = "toolbox-tab-" + id;
    radio.setAttribute("flex", "1");
    radio.setAttribute("toolid", id);
    if (toolDefinition.ordinal == undefined || toolDefinition.ordinal < 0) {
      toolDefinition.ordinal = MAX_ORDINAL;
    }
    radio.setAttribute("ordinal", toolDefinition.ordinal);
    radio.setAttribute("tooltiptext", toolDefinition.tooltip);

    radio.addEventListener("command", function(id) {
      this.selectTool(id);
    }.bind(this, id));

    if (toolDefinition.icon) {
      let image = this.doc.createElement("image");
      image.setAttribute("src", toolDefinition.icon);
      radio.appendChild(image);
    }

    let label = this.doc.createElement("label");
    label.setAttribute("value", toolDefinition.label)
    label.setAttribute("crop", "end");
    label.setAttribute("flex", "1");

    let vbox = this.doc.createElement("vbox");
    vbox.className = "toolbox-panel";
    vbox.id = "toolbox-panel-" + id;

    radio.appendChild(label);

    
    if (tabs.childNodes.length == 0 ||
        +tabs.lastChild.getAttribute("ordinal") <= toolDefinition.ordinal) {
      tabs.appendChild(radio);
      deck.appendChild(vbox);
    }
    
    else {
      Array.some(tabs.childNodes, (node, i) => {
        if (+node.getAttribute("ordinal") > toolDefinition.ordinal) {
          tabs.insertBefore(radio, node);
          deck.insertBefore(vbox, deck.childNodes[i + 1]);
          
          return true;
        }
      });
    }

    this._addKeysToWindow();
  },

  





  selectTool: function TBOX_selectTool(id) {
    let deferred = Promise.defer();

    let selected = this.doc.querySelector(".devtools-tab[selected]");
    if (selected) {
      selected.removeAttribute("selected");
    }
    let tab = this.doc.getElementById("toolbox-tab-" + id);
    tab.setAttribute("selected", "true");

    if (this._currentToolId == id) {
      
      return Promise.resolve(this._toolPanels.get(id));
    }

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
    tabstrip.selectedItem = tab;

    
    let deck = this.doc.getElementById("toolbox-deck");
    
    if (id == "options") {
      deck.selectedIndex = 0;
      this.optionsButton.setAttribute("checked", true);
    }
    else {
      deck.selectedIndex = index != -1 ? index + 1: -1;
      this.optionsButton.removeAttribute("checked");
    }

    let definition = gDevTools.getToolDefinitionMap().get(id);

    this._currentToolId = id;

    let resolveSelected = panel => {
      this.emit("select", id);
      this.emit(id + "-selected", panel);
      deferred.resolve(panel);
    };

    let iframe = this.doc.getElementById("toolbox-panel-iframe-" + id);
    if (!iframe) {
      iframe = this.doc.createElement("iframe");
      iframe.className = "toolbox-panel-iframe";
      iframe.id = "toolbox-panel-iframe-" + id;
      iframe.setAttribute("flex", 1);
      iframe.setAttribute("forceOwnRefreshDriver", "");
      iframe.tooltip = "aHTMLTooltip";

      let vbox = this.doc.getElementById("toolbox-panel-" + id);
      vbox.appendChild(iframe);

      let boundLoad = function() {
        iframe.removeEventListener("DOMContentLoaded", boundLoad, true);

        let built = definition.build(iframe.contentWindow, this);
        Promise.resolve(built).then(function(panel) {
          this._toolPanels.set(id, panel);

          this.emit(id + "-ready", panel);
          gDevTools.emit(id + "-ready", this, panel);

          resolveSelected(panel);
        }.bind(this));
      }.bind(this);

      iframe.addEventListener("DOMContentLoaded", boundLoad, true);
      iframe.setAttribute("src", definition.url);
    } else {
      let panel = this._toolPanels.get(id);
      
      if (panel && (!panel.contentDocument ||
                    panel.contentDocument.readyState == "complete")) {
        resolveSelected(panel);
      }
      else if (panel) {
        let boundLoad = function() {
          panel.removeEventListener("DOMContentLoaded", boundLoad, true);
          resolveSelected(panel);
        };
        panel.addEventListener("DOMContentLoaded", boundLoad, true);
      }
    }

    if (id != "options") {
      Services.prefs.setCharPref(this._prefs.LAST_TOOL, id);
    }

    return deferred.promise;
  },

  


  raise: function TBOX_raise() {
    this._host.raise();
  },

  


  _refreshHostTitle: function TBOX_refreshHostTitle() {
    let toolName;
    let toolId = this.currentToolId;
    let toolDef = gDevTools.getToolDefinitionMap().get(toolId);
    if (toolDef) {
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
    if (typeof toolId != "string") {
      toolId = toolId.id;
    }

    if (this._toolPanels.has(toolId)) {
      let instance = this._toolPanels.get(toolId);
      instance.destroy();
      this._toolPanels.delete(toolId);
    }

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

    gDevTools.off("tool-registered", this._toolRegistered);
    gDevTools.off("tool-unregistered", this._toolUnregistered);

    let outstanding = [];

    this._toolPanels.delete("options");
    for (let [id, panel] of this._toolPanels) {
      outstanding.push(panel.destroy());
    }

    let container = this.doc.getElementById("toolbox-buttons");
    while(container.firstChild) {
      container.removeChild(container.firstChild);
    }

    outstanding.push(this._host.destroy());

    
    
    if (this._target) {
      this._target.off("close", this.destroy);
      outstanding.push(this._target.destroy());
    }
    this._target = null;

    Promise.all(outstanding).then(function() {
      this.emit("destroyed");
      
      
      this._host = null;
      deferred.resolve();
    }.bind(this));

    return this._destroyer;
  }
};
