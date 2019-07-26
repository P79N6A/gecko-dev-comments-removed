



"use strict";

const {Cc, Ci, Cu} = require("chrome");
const MAX_ORDINAL = 99;
const ZOOM_PREF = "devtools.toolbox.zoomValue";
const MIN_ZOOM = 0.5;
const MAX_ZOOM = 2;

let promise = require("sdk/core/promise");
let EventEmitter = require("devtools/shared/event-emitter");
let Telemetry = require("devtools/shared/telemetry");
let HUDService = require("devtools/webconsole/hudservice");

Cu.import('resource://gre/modules/XPCOMUtils.jsm');
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource:///modules/devtools/gDevTools.jsm");
Cu.import("resource:///modules/devtools/scratchpad-manager.jsm");

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
  Cu.import("resource://gre/modules/devtools/gcli.jsm", {});

  let req = scope.require;
  return req('gcli/cli').Requisition;
});













function Toolbox(target, selectedTool, hostType) {
  this._target = target;
  this._toolPanels = new Map();
  this._telemetry = new Telemetry();

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

  


  get zoomValue() {
    return parseFloat(Services.prefs.getCharPref(ZOOM_PREF));
  },

  


  open: function TBOX_open() {
    let deferred = promise.defer();

    this._host.create().then(iframe => {
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

        this.selectTool(this._defaultToolId).then(function(panel) {
          this.emit("ready");
          deferred.resolve();
        }.bind(this));
      };

      iframe.addEventListener("DOMContentLoaded", domReady, true);
      iframe.setAttribute("src", this._URL);
    });

    return deferred.promise;
  },

  _buildOptions: function TBOX__buildOptions() {
    let key = this.doc.getElementById("toolbox-options-key");
    key.addEventListener("command", function(toolId) {
      this.selectTool(toolId);
    }.bind(this, "options"), true);
  },

  _addToolSwitchingKeys: function TBOX__addToolSwitchingKeys() {
    let nextKey = this.doc.getElementById("toolbox-next-tool-key");
    nextKey.addEventListener("command", this.selectNextTool.bind(this), true);
    let prevKey = this.doc.getElementById("toolbox-previous-tool-key");
    prevKey.addEventListener("command", this.selectPreviousTool.bind(this), true);
  },

  


  _addZoomKeys: function TBOX__addZoomKeys() {
    let inKey = this.doc.getElementById("toolbox-zoom-in-key");
    inKey.addEventListener("command", this.zoomIn.bind(this), true);

    let inKey2 = this.doc.getElementById("toolbox-zoom-in-key2");
    inKey2.addEventListener("command", this.zoomIn.bind(this), true);

    let outKey = this.doc.getElementById("toolbox-zoom-out-key");
    outKey.addEventListener("command", this.zoomOut.bind(this), true);

    let resetKey = this.doc.getElementById("toolbox-zoom-reset-key");
    resetKey.addEventListener("command", this.zoomReset.bind(this), true);
  },

  


  _loadInitialZoom: function TBOX__loadInitialZoom() {
    this.setZoom(this.zoomValue);
  },

  


  zoomIn: function TBOX__zoomIn() {
    this.setZoom(this.zoomValue + 0.1);
  },

  


  zoomOut: function TBOX__zoomOut() {
    this.setZoom(this.zoomValue - 0.1);
  },

  


  zoomReset: function TBOX__zoomReset() {
    this.setZoom(1);
  },

  





  setZoom: function TBOX__setZoom(zoomValue) {
    
    zoomValue = Math.max(zoomValue, MIN_ZOOM);
    zoomValue = Math.min(zoomValue, MAX_ZOOM);

    let contViewer = this.frame.docShell.contentViewer;
    let docViewer = contViewer.QueryInterface(Ci.nsIMarkupDocumentViewer);

    docViewer.fullZoom = zoomValue;

    Services.prefs.setCharPref(ZOOM_PREF, zoomValue);
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
          this.selectTool(toolId).then(() => {
              this.fireCustomKey(toolId);
          });
        }.bind(this, id), true);
        doc.getElementById("toolbox-keyset").appendChild(key);
      }
    }

    
    if(doc.getElementById("key_scratchpad") == null) {
      let key = doc.createElement("key");
      key.id = "key_scratchpad";

      key.setAttribute("keycode", toolboxStrings("scratchpad.keycode"));
      key.setAttribute("modifiers", "shift");
      key.setAttribute("oncommand", "void(0)"); 
      key.addEventListener("command", function() {
        ScratchpadManager.openScratchpad();
      }, true);
      doc.getElementById("toolbox-keyset").appendChild(key);
    }

    
    if(doc.getElementById("key_browserconsole") == null) {
      let key = doc.createElement("key");
      key.id = "key_browserconsole";

      key.setAttribute("key", toolboxStrings("browserConsoleCmd.commandkey"));
      key.setAttribute("modifiers", "accel,shift");
      key.setAttribute("oncommand", "void(0)"); 
      key.addEventListener("command", function() {
        HUDService.toggleBrowserConsole();
      }, true);
      doc.getElementById("toolbox-keyset").appendChild(key);
    }
  },


  




  fireCustomKey: function TBOX_fireCustomKey(toolId) {
    let tools = gDevTools.getToolDefinitionMap();
    let activeToolDefinition = tools.get(toolId);

    if (activeToolDefinition.onkey && this.currentToolId === toolId) {
        activeToolDefinition.onkey(this.getCurrentPanel());
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
    let env = CommandUtils.createEnvironment(this.target.tab.ownerDocument,
                                             this.target.window.document);
    let requisition = new Requisition(env);

    let buttons = CommandUtils.createButtons(toolbarSpec, this._target, this.doc, requisition);

    let container = this.doc.getElementById("toolbox-buttons");
    buttons.forEach(container.appendChild.bind(container));
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
    radio.setAttribute("toolid", id);
    if (toolDefinition.ordinal == undefined || toolDefinition.ordinal < 0) {
      toolDefinition.ordinal = MAX_ORDINAL;
    }
    radio.setAttribute("ordinal", toolDefinition.ordinal);
    radio.setAttribute("tooltiptext", toolDefinition.tooltip);

    radio.addEventListener("command", function(id) {
      this.selectTool(id);
    }.bind(this, id));

    
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
    }
    
    else {
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

  





  loadTool: function TBOX_loadTool(id) {
    let deferred = promise.defer();
    let iframe = this.doc.getElementById("toolbox-panel-iframe-" + id);

    if (iframe) {
      let panel = this._toolPanels.get(id);
      if (panel) {
        deferred.resolve(panel);
      } else {
        this.once(id + "-ready", (panel) => {
          deferred.resolve(panel);
        });
      }
      return deferred.promise;
    }

    let definition = gDevTools.getToolDefinitionMap().get(id);
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

  





  selectTool: function TBOX_selectTool(id) {
    let selected = this.doc.querySelector(".devtools-tab[selected]");
    if (selected) {
      selected.removeAttribute("selected");
    }
    let tab = this.doc.getElementById("toolbox-tab-" + id);
    tab.setAttribute("selected", "true");

    let prevToolId = this._currentToolId;

    if (this._currentToolId == id) {
      
      this.focusTool(id);

      
      return promise.resolve(this._toolPanels.get(id));
    }

    if (!this.isReady) {
      throw new Error("Can't select tool, wait for toolbox 'ready' event");
    }
    let tab = this.doc.getElementById("toolbox-tab-" + id);

    if (tab) {
      if (prevToolId) {
        this._telemetry.toolClosed(prevToolId);
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

    this._currentToolId = id;
    if (id != "options") {
      Services.prefs.setCharPref(this._prefs.LAST_TOOL, id);
    }

    return this.loadTool(id).then((panel) => {
      
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

  


  selectNextTool: function TBOX_selectNextTool() {
    let selected = this.doc.querySelector(".devtools-tab[selected]");
    let next = selected.nextSibling || selected.parentNode.firstChild;
    let tool = next.getAttribute("toolid");
    return this.selectTool(tool);
  },

  


  selectPreviousTool: function TBOX_selectPreviousTool() {
    let selected = this.doc.querySelector(".devtools-tab[selected]");
    let previous = selected.previousSibling || selected.parentNode.lastChild;
    let tool = previous.getAttribute("toolid");
    return this.selectTool(tool);
  },

  





  highlightTool: function TBOX_highlightTool(id) {
    let tab = this.doc.getElementById("toolbox-tab-" + id);
    tab && tab.classList.add("highlighted");
  },

  





  unhighlightTool: function TBOX_unhighlightTool(id) {
    let tab = this.doc.getElementById("toolbox-tab-" + id);
    tab && tab.classList.remove("highlighted");
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
      let key = doc.getElementById("key_" + toolId);
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
    
    
    
    let deferred = promise.defer();
    this._destroyer = deferred.promise;

    this._target.off("navigate", this._refreshHostTitle);
    this.off("select", this._refreshHostTitle);
    this.off("host-changed", this._refreshHostTitle);

    gDevTools.off("tool-registered", this._toolRegistered);
    gDevTools.off("tool-unregistered", this._toolUnregistered);

    
    
    if (typeof this._origAllowJavascript != "undefined") {
      let docShell = this._host.hostTab.linkedBrowser.docShell;
      docShell.allowJavascript = this._origAllowJavascript;
      delete this._origAllowJavascript;
    }

    let outstanding = [];

    for (let [id, panel] of this._toolPanels) {
      try {
        outstanding.push(panel.destroy());
      } catch(e) {
        
        console.error(e);
      }
    }

    let container = this.doc.getElementById("toolbox-buttons");
    while(container.firstChild) {
      container.removeChild(container.firstChild);
    }

    outstanding.push(this._host.destroy());

    this._telemetry.destroy();

    
    if (this._target) {
      this._target.off("close", this.destroy);
      outstanding.push(this._target.destroy());
    }
    this._target = null;

    promise.all(outstanding).then(function() {
      this.emit("destroyed");
      
      
      this._host = null;
      deferred.resolve();
    }.bind(this));

    return this._destroyer;
  }
};
