



"use strict";

const MAX_ORDINAL = 99;
const ZOOM_PREF = "devtools.toolbox.zoomValue";
const MIN_ZOOM = 0.5;
const MAX_ZOOM = 2;

let {Cc, Ci, Cu} = require("chrome");
let promise = require("sdk/core/promise");
let EventEmitter = require("devtools/shared/event-emitter");
let Telemetry = require("devtools/shared/telemetry");
let HUDService = require("devtools/webconsole/hudservice");

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource:///modules/devtools/gDevTools.jsm");
Cu.import("resource:///modules/devtools/scratchpad-manager.jsm");

loader.lazyGetter(this, "Hosts", () => require("devtools/framework/toolbox-hosts").Hosts);

loader.lazyImporter(this, "CommandUtils", "resource:///modules/devtools/DeveloperToolbar.jsm");

loader.lazyGetter(this, "toolboxStrings", () => {
  let bundle = Services.strings.createBundle("chrome://browser/locale/devtools/toolbox.properties");
  return (name, ...args) => {
    try {
      if (!args.length) {
        return bundle.GetStringFromName(name);
      }
      return bundle.formatStringFromName(name, args, args.length);
    } catch (ex) {
      Services.console.logStringMessage("Error reading '" + name + "'");
    }
  };
});

loader.lazyGetter(this, "Requisition", () => {
  let {require} = Cu.import("resource://gre/modules/devtools/Require.jsm", {});
  Cu.import("resource://gre/modules/devtools/gcli.jsm", {});
  return require("gcli/cli").Requisition;
});













function Toolbox(target, selectedTool, hostType) {
  this._target = target;
  this._toolPanels = new Map();
  this._telemetry = new Telemetry();

  this._toolRegistered = this._toolRegistered.bind(this);
  this._toolUnregistered = this._toolUnregistered.bind(this);
  this._refreshHostTitle = this._refreshHostTitle.bind(this);
  this.destroy = this.destroy.bind(this);

  this._target.on("close", this.destroy);

  if (!hostType) {
    hostType = Services.prefs.getCharPref(this._prefs.LAST_HOST);
  }
  if (!selectedTool) {
    selectedTool = Services.prefs.getCharPref(this._prefs.LAST_TOOL);
  }
  if (!gDevTools.getToolDefinition(selectedTool)) {
    selectedTool = "webconsole";
  }
  this._defaultToolId = selectedTool;

  this._host = this._createHost(hostType);

  EventEmitter.decorate(this);

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
};

Toolbox.prototype = {
  _URL: "chrome://browser/content/devtools/framework/toolbox.xul",

  _prefs: {
    LAST_HOST: "devtools.toolbox.host",
    LAST_TOOL: "devtools.toolbox.selectedTool",
    SIDE_ENABLED: "devtools.toolbox.sideEnabled"
  },

  currentToolId: null,

  





  getToolPanels: function() {
    return new Map(this._toolPanels);
  },

  


  getPanel: function(id) {
    return this._toolPanels.get(id);
  },

  




  getCurrentPanel: function() {
    return this._toolPanels.get(this.currentToolId);
  },

  




  get target() {
    return this._target;
  },

  



  get hostType() {
    return this._host.type;
  },

  


  get frame() {
    return this._host.frame;
  },

  


  get doc() {
    return this.frame.contentDocument;
  },

  


  get zoomValue() {
    return parseFloat(Services.prefs.getCharPref(ZOOM_PREF));
  },

  


  open: function() {
    let deferred = promise.defer();

    return this._host.create().then(iframe => {
      let deferred = promise.defer();

      let domReady = () => {
        iframe.removeEventListener("DOMContentLoaded", domReady, true);

        this.isReady = true;

        let closeButton = this.doc.getElementById("toolbox-close");
        closeButton.addEventListener("command", this.destroy, true);

        this._buildDockButtons();
        this._buildOptions();
        this._buildTabs();
        this._buildButtons();
        this._addKeysToWindow();
        this._addToolSwitchingKeys();
        this._addZoomKeys();
        this._loadInitialZoom();

        this._telemetry.toolOpened("toolbox");

        this.selectTool(this._defaultToolId).then(panel => {
          this.emit("ready");
          deferred.resolve();
        });
      };

      iframe.addEventListener("DOMContentLoaded", domReady, true);
      iframe.setAttribute("src", this._URL);

      return deferred.promise;
    });
  },

  _buildOptions: function() {
    let key = this.doc.getElementById("toolbox-options-key");
    key.addEventListener("command", () => {
      this.selectTool("options");
    }, true);
  },

  _addToolSwitchingKeys: function() {
    let nextKey = this.doc.getElementById("toolbox-next-tool-key");
    nextKey.addEventListener("command", this.selectNextTool.bind(this), true);
    let prevKey = this.doc.getElementById("toolbox-previous-tool-key");
    prevKey.addEventListener("command", this.selectPreviousTool.bind(this), true);
  },

  


  _addZoomKeys: function() {
    let inKey = this.doc.getElementById("toolbox-zoom-in-key");
    inKey.addEventListener("command", this.zoomIn.bind(this), true);

    let inKey2 = this.doc.getElementById("toolbox-zoom-in-key2");
    inKey2.addEventListener("command", this.zoomIn.bind(this), true);

    let outKey = this.doc.getElementById("toolbox-zoom-out-key");
    outKey.addEventListener("command", this.zoomOut.bind(this), true);

    let resetKey = this.doc.getElementById("toolbox-zoom-reset-key");
    resetKey.addEventListener("command", this.zoomReset.bind(this), true);
  },

  


  _loadInitialZoom: function() {
    this.setZoom(this.zoomValue);
  },

  


  zoomIn: function() {
    this.setZoom(this.zoomValue + 0.1);
  },

  


  zoomOut: function() {
    this.setZoom(this.zoomValue - 0.1);
  },

  


  zoomReset: function() {
    this.setZoom(1);
  },

  





  setZoom: function(zoomValue) {
    
    zoomValue = Math.max(zoomValue, MIN_ZOOM);
    zoomValue = Math.min(zoomValue, MAX_ZOOM);

    let contViewer = this.frame.docShell.contentViewer;
    let docViewer = contViewer.QueryInterface(Ci.nsIMarkupDocumentViewer);

    docViewer.fullZoom = zoomValue;

    Services.prefs.setCharPref(ZOOM_PREF, zoomValue);
  },

  


  _addKeysToWindow: function() {
    if (this.hostType != Toolbox.HostType.WINDOW) {
      return;
    }

    let doc = this.doc.defaultView.parent.document;

    for (let [id, toolDefinition] of gDevTools.getToolDefinitionMap()) {
      
      if (!toolDefinition.key || doc.getElementById("key_" + id)) {
        continue;
      }

      let toolId = id;
      let key = doc.createElement("key");

      key.id = "key_" + toolId;

      if (toolDefinition.key.startsWith("VK_")) {
        key.setAttribute("keycode", toolDefinition.key);
      } else {
        key.setAttribute("key", toolDefinition.key);
      }

      key.setAttribute("modifiers", toolDefinition.modifiers);
      key.setAttribute("oncommand", "void(0);"); 
      key.addEventListener("command", () => {
        this.selectTool(toolId).then(() => this.fireCustomKey(toolId));
      }, true);
      doc.getElementById("toolbox-keyset").appendChild(key);
    }

    
    if (!doc.getElementById("key_browserconsole")) {
      let key = doc.createElement("key");
      key.id = "key_browserconsole";

      key.setAttribute("key", toolboxStrings("browserConsoleCmd.commandkey"));
      key.setAttribute("modifiers", "accel,shift");
      key.setAttribute("oncommand", "void(0)"); 
      key.addEventListener("command", () => {
        HUDService.toggleBrowserConsole();
      }, true);
      doc.getElementById("toolbox-keyset").appendChild(key);
    }
  },


  




  fireCustomKey: function(toolId) {
    let toolDefinition = gDevTools.getToolDefinition(toolId);

    if (toolDefinition.onkey && this.currentToolId === toolId) {
      toolDefinition.onkey(this.getCurrentPanel());
    }
  },

  



  _buildDockButtons: function() {
    let dockBox = this.doc.getElementById("toolbox-dock-buttons");

    while (dockBox.firstChild) {
      dockBox.removeChild(dockBox.firstChild);
    }

    if (!this._target.isLocalTab) {
      return;
    }

    let closeButton = this.doc.getElementById("toolbox-close");
    if (this.hostType == Toolbox.HostType.WINDOW) {
      closeButton.setAttribute("hidden", "true");
    } else {
      closeButton.removeAttribute("hidden");
    }

    let sideEnabled = Services.prefs.getBoolPref(this._prefs.SIDE_ENABLED);

    for (let type in Toolbox.HostType) {
      let position = Toolbox.HostType[type];
      if (position == this.hostType ||
          (!sideEnabled && position == Toolbox.HostType.SIDE)) {
        continue;
      }

      let button = this.doc.createElement("toolbarbutton");
      button.id = "toolbox-dock-" + position;
      button.className = "toolbox-dock-button";
      button.setAttribute("tooltiptext", toolboxStrings("toolboxDockButtons." +
                                                        position + ".tooltip"));
      button.addEventListener("command", () => {
        this.switchHost(position);
      });

      dockBox.appendChild(button);
    }
  },

  


  _buildTabs: function() {
    for (let definition of gDevTools.getToolDefinitionArray()) {
      this._buildTabForTool(definition);
    }
  },

  


  _buildButtons: function() {
    if (!this.target.isLocalTab) {
      return;
    }

    let spec = CommandUtils.getCommandbarSpec("devtools.toolbox.toolbarSpec");
    let env = CommandUtils.createEnvironment(this.target.tab.ownerDocument,
                                             this.target.window.document);
    let req = new Requisition(env);
    let buttons = CommandUtils.createButtons(spec, this._target, this.doc, req);
    let container = this.doc.getElementById("toolbox-buttons");
    buttons.forEach(container.appendChild.bind(container));
  },

  





  _buildTabForTool: function(toolDefinition) {
    if (!toolDefinition.isTargetSupported(this._target)) {
      return;
    }

    let tabs = this.doc.getElementById("toolbox-tabs");
    let deck = this.doc.getElementById("toolbox-deck");

    let id = toolDefinition.id;

    if (toolDefinition.ordinal == undefined || toolDefinition.ordinal < 0) {
      toolDefinition.ordinal = MAX_ORDINAL;
    }

    let radio = this.doc.createElement("radio");
    
    
    
    radio.className = "toolbox-tab devtools-tab";
    radio.id = "toolbox-tab-" + id;
    radio.setAttribute("toolid", id);
    radio.setAttribute("ordinal", toolDefinition.ordinal);
    radio.setAttribute("tooltiptext", toolDefinition.tooltip);

    radio.addEventListener("command", () => {
      this.selectTool(id);
    });

    
    let spacer = this.doc.createElement("spacer");
    spacer.setAttribute("flex", "1");
    radio.appendChild(spacer);

    if (toolDefinition.icon) {
      let image = this.doc.createElement("image");
      image.className = "default-icon";
      image.setAttribute("src",
                         toolDefinition.icon || toolDefinition.highlightedicon);
      radio.appendChild(image);
      
      image = this.doc.createElement("image");
      image.className = "highlighted-icon";
      image.setAttribute("src",
                         toolDefinition.highlightedicon || toolDefinition.icon);
      radio.appendChild(image);
    }

    if (toolDefinition.label) {
      let label = this.doc.createElement("label");
      label.setAttribute("value", toolDefinition.label)
      label.setAttribute("crop", "end");
      label.setAttribute("flex", "1");
      radio.appendChild(label);
      radio.setAttribute("flex", "1");
    }

    let vbox = this.doc.createElement("vbox");
    vbox.className = "toolbox-panel";
    vbox.id = "toolbox-panel-" + id;


    
    if (tabs.childNodes.length == 0 ||
        +tabs.lastChild.getAttribute("ordinal") <= toolDefinition.ordinal) {
      tabs.appendChild(radio);
      deck.appendChild(vbox);
    } else {
      
      Array.some(tabs.childNodes, (node, i) => {
        if (+node.getAttribute("ordinal") > toolDefinition.ordinal) {
          tabs.insertBefore(radio, node);
          deck.insertBefore(vbox, deck.childNodes[i]);
          return true;
        }
      });
    }

    this._addKeysToWindow();
  },

  





  loadTool: function(id) {
    let deferred = promise.defer();
    let iframe = this.doc.getElementById("toolbox-panel-iframe-" + id);

    if (iframe) {
      let panel = this._toolPanels.get(id);
      if (panel) {
        deferred.resolve(panel);
      } else {
        this.once(id + "-ready", panel => {
          deferred.resolve(panel);
        });
      }
      return deferred.promise;
    }

    let definition = gDevTools.getToolDefinition(id);
    if (!definition) {
      deferred.reject(new Error("no such tool id "+id));
      return deferred.promise;
    }

    iframe = this.doc.createElement("iframe");
    iframe.className = "toolbox-panel-iframe";
    iframe.id = "toolbox-panel-iframe-" + id;
    iframe.setAttribute("flex", 1);
    iframe.setAttribute("forceOwnRefreshDriver", "");
    iframe.tooltip = "aHTMLTooltip";

    let vbox = this.doc.getElementById("toolbox-panel-" + id);
    vbox.appendChild(iframe);

    let onLoad = () => {
      iframe.removeEventListener("DOMContentLoaded", onLoad, true);

      let built = definition.build(iframe.contentWindow, this);
      promise.resolve(built).then((panel) => {
        this._toolPanels.set(id, panel);
        this.emit(id + "-ready", panel);
        gDevTools.emit(id + "-ready", this, panel);
        deferred.resolve(panel);
      });
    };

    iframe.addEventListener("DOMContentLoaded", onLoad, true);
    iframe.setAttribute("src", definition.url);
    return deferred.promise;
  },

  





  selectTool: function(id) {
    let selected = this.doc.querySelector(".devtools-tab[selected]");
    if (selected) {
      selected.removeAttribute("selected");
    }

    let tab = this.doc.getElementById("toolbox-tab-" + id);
    tab.setAttribute("selected", "true");


    if (this.currentToolId == id) {
      
      this.focusTool(id);

      
      return promise.resolve(this._toolPanels.get(id));
    }

    if (!this.isReady) {
      throw new Error("Can't select tool, wait for toolbox 'ready' event");
    }

    tab = this.doc.getElementById("toolbox-tab-" + id);

    if (tab) {
      if (this.currentToolId) {
        this._telemetry.toolClosed(this.currentToolId);
      }
      this._telemetry.toolOpened(id);
    } else {
      throw new Error("No tool found");
    }

    let tabstrip = this.doc.getElementById("toolbox-tabs");

    
    
    let index = 0;
    let tabs = tabstrip.childNodes;
    for (let i = 0; i < tabs.length; i++) {
      if (tabs[i] === tab) {
        index = i;
        break;
      }
    }
    tabstrip.selectedItem = tab;

    
    let deck = this.doc.getElementById("toolbox-deck");
    deck.selectedIndex = index;

    this.currentToolId = id;
    if (id != "options") {
      Services.prefs.setCharPref(this._prefs.LAST_TOOL, id);
    }

    return this.loadTool(id).then(panel => {
      
      this.focusTool(id);

      this.emit("select", id);
      this.emit(id + "-selected", panel);
      return panel;
    });
  },

  




  focusTool: function(id) {
    let iframe = this.doc.getElementById("toolbox-panel-iframe-" + id);
    iframe.focus();
  },

  


  selectNextTool: function() {
    let selected = this.doc.querySelector(".devtools-tab[selected]");
    let next = selected.nextSibling || selected.parentNode.firstChild;
    let tool = next.getAttribute("toolid");
    return this.selectTool(tool);
  },

  


  selectPreviousTool: function() {
    let selected = this.doc.querySelector(".devtools-tab[selected]");
    let previous = selected.previousSibling || selected.parentNode.lastChild;
    let tool = previous.getAttribute("toolid");
    return this.selectTool(tool);
  },

  





  highlightTool: function(id) {
    let tab = this.doc.getElementById("toolbox-tab-" + id);
    tab && tab.classList.add("highlighted");
  },

  





  unhighlightTool: function(id) {
    let tab = this.doc.getElementById("toolbox-tab-" + id);
    tab && tab.classList.remove("highlighted");
  },

  


  raise: function() {
    this._host.raise();
  },

  


  _refreshHostTitle: function() {
    let toolName;
    let toolDef = gDevTools.getToolDefinition(this.currentToolId);
    if (toolDef) {
      toolName = toolDef.label;
    } else {
      
      toolName = toolboxStrings("toolbox.defaultTitle");
    }
    let title = toolboxStrings("toolbox.titleTemplate",
                               toolName, this.target.url);
    this._host.setTitle(title);
  },

  












  _createHost: function(hostType) {
    if (!Hosts[hostType]) {
      throw new Error("Unknown hostType: " + hostType);
    }

    
    let newHost = new Hosts[hostType](this.target.tab);
    newHost.on("window-closed", this.destroy);
    return newHost;
  },

  






  switchHost: function(hostType) {
    if (hostType == this._host.type || !this._target.isLocalTab) {
      return;
    }

    let newHost = this._createHost(hostType);
    return newHost.create().then(iframe => {
      
      iframe.QueryInterface(Ci.nsIFrameLoaderOwner);
      iframe.swapFrameLoaders(this.frame);

      this._host.off("window-closed", this.destroy);
      this._host.destroy();

      this._host = newHost;

      Services.prefs.setCharPref(this._prefs.LAST_HOST, this._host.type);

      this._buildDockButtons();
      this._addKeysToWindow();

      this.emit("host-changed");
    });
  },

  






  _toolRegistered: function(event, toolId) {
    let tool = gDevTools.getToolDefinition(toolId);
    this._buildTabForTool(tool);
  },

  







  _toolUnregistered: function(event, toolId) {
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
      if (this.currentToolId == toolId) {
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
      let key = doc.getElementById("key_" + toolId);
      if (key) {
        key.parentNode.removeChild(key);
      }
    }
  },

  




  getNotificationBox: function() {
    return this.doc.getElementById("toolbox-notificationbox");
  },

  


  destroy: function() {
    
    
    if (this._destroyer) {
      return this._destroyer;
    }

    this._target.off("navigate", this._refreshHostTitle);
    this.off("select", this._refreshHostTitle);
    this.off("host-changed", this._refreshHostTitle);

    gDevTools.off("tool-registered", this._toolRegistered);
    gDevTools.off("tool-unregistered", this._toolUnregistered);

    
    
    if (typeof this._origAllowJavascript != "undefined") {
      let docShell = this._host.hostTab.linkedBrowser.docShell;
      docShell.allowJavascript = this._origAllowJavascript;
      this._origAllowJavascript = undefined;
    }

    let outstanding = [];

    for (let [id, panel] of this._toolPanels) {
      try {
        outstanding.push(panel.destroy());
      } catch (e) {
        
        console.error(e);
      }
    }

    let container = this.doc.getElementById("toolbox-buttons");
    while (container.firstChild) {
      container.removeChild(container.firstChild);
    }

    outstanding.push(this._host.destroy());

    this._telemetry.destroy();

    return this._destroyer = promise.all(outstanding).then(() => {
      
      
      
      
      if (this._target) {
        let target = this._target;
        this._target = null;
        target.off("close", this.destroy);
        return target.destroy();
      }
    }).then(() => {
      this.emit("destroyed");
      
      
      this._host = null;
    });
  }
};
