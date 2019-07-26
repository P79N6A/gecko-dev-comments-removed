



"use strict";

const MAX_ORDINAL = 99;
const ZOOM_PREF = "devtools.toolbox.zoomValue";
const MIN_ZOOM = 0.5;
const MAX_ZOOM = 2;

let {Cc, Ci, Cu} = require("chrome");
let {Promise: promise} = require("resource://gre/modules/Promise.jsm");
let EventEmitter = require("devtools/toolkit/event-emitter");
let Telemetry = require("devtools/shared/telemetry");
let HUDService = require("devtools/webconsole/hudservice");

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource:///modules/devtools/gDevTools.jsm");
Cu.import("resource:///modules/devtools/scratchpad-manager.jsm");
Cu.import("resource:///modules/devtools/DOMHelpers.jsm");
Cu.import("resource://gre/modules/Task.jsm");

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
      return null;
    }
  };
});

loader.lazyGetter(this, "Selection", () => require("devtools/framework/selection").Selection);
loader.lazyGetter(this, "InspectorFront", () => require("devtools/server/actors/inspector").InspectorFront);















function Toolbox(target, selectedTool, hostType, hostOptions) {
  this._target = target;
  this._toolPanels = new Map();
  this._telemetry = new Telemetry();

  this._toolRegistered = this._toolRegistered.bind(this);
  this._toolUnregistered = this._toolUnregistered.bind(this);
  this._refreshHostTitle = this._refreshHostTitle.bind(this);
  this._splitConsoleOnKeypress = this._splitConsoleOnKeypress.bind(this)
  this.destroy = this.destroy.bind(this);
  this.highlighterUtils = new ToolboxHighlighterUtils(this);
  this._highlighterReady = this._highlighterReady.bind(this);
  this._highlighterHidden = this._highlighterHidden.bind(this);

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

  this._host = this._createHost(hostType, hostOptions);

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
  WINDOW: "window",
  CUSTOM: "custom"
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

  



  get highlighter() {
    if (this.highlighterUtils.isRemoteHighlightable) {
      return this._highlighter;
    } else {
      return null;
    }
  },

  



  get inspector() {
    return this._inspector;
  },

  



  get walker() {
    return this._walker;
  },

  



  get selection() {
    return this._selection;
  },

  


  get splitConsole() {
    return this._splitConsole;
  },

  


  open: function() {
    let deferred = promise.defer();

    return this._host.create().then(iframe => {
      let deferred = promise.defer();

      let domReady = () => {
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

      
      this._target.makeRemote().then(() => {
        iframe.setAttribute("src", this._URL);
        let domHelper = new DOMHelpers(iframe.contentWindow);
        domHelper.onceDOMReady(domReady);
      });

      return deferred.promise;
    });
  },

  _buildOptions: function() {
    let key = this.doc.getElementById("toolbox-options-key");
    key.addEventListener("command", () => {
      this.selectTool("options");
    }, true);
  },

  _isResponsiveModeActive: function() {
    let responsiveModeActive = false;
    if (this.target.isLocalTab) {
      let tab = this.target.tab;
      let browserWindow = tab.ownerDocument.defaultView;
      let responsiveUIManager = browserWindow.ResponsiveUI.ResponsiveUIManager;
      responsiveModeActive = responsiveUIManager.isActiveForTab(tab);
    }
    return responsiveModeActive;
  },

  _splitConsoleOnKeypress: function(e) {
    let responsiveModeActive = this._isResponsiveModeActive();
    if (e.keyCode === e.DOM_VK_ESCAPE && !responsiveModeActive) {
      this.toggleSplitConsole();
    }
  },

  _addToolSwitchingKeys: function() {
    let nextKey = this.doc.getElementById("toolbox-next-tool-key");
    nextKey.addEventListener("command", this.selectNextTool.bind(this), true);
    let prevKey = this.doc.getElementById("toolbox-previous-tool-key");
    prevKey.addEventListener("command", this.selectPreviousTool.bind(this), true);

    
    
    this.doc.addEventListener("keypress", this._splitConsoleOnKeypress, false);
  },

  











  _refreshConsoleDisplay: function() {
    let deck = this.doc.getElementById("toolbox-deck");
    let webconsolePanel = this.doc.getElementById("toolbox-panel-webconsole");
    let splitter = this.doc.getElementById("toolbox-console-splitter");
    let openedConsolePanel = this.currentToolId === "webconsole";

    if (openedConsolePanel) {
      deck.setAttribute("collapsed", "true");
      splitter.setAttribute("hidden", "true");
      webconsolePanel.removeAttribute("collapsed");
    } else {
      deck.removeAttribute("collapsed");
      if (this._splitConsole) {
        webconsolePanel.removeAttribute("collapsed");
        splitter.removeAttribute("hidden");
      } else {
        webconsolePanel.setAttribute("collapsed", "true");
        splitter.setAttribute("hidden", "true");
      }
    }
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

    if (toolDefinition.onkey &&
        ((this.currentToolId === toolId) ||
          (toolId == "webconsole" && this.splitConsole))) {
      toolDefinition.onkey(this.getCurrentPanel(), this);
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
          position == Toolbox.HostType.CUSTOM ||
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
    if (!this.target.isAddon) {
      this._buildPickerButton();
    }

    if (!this.target.isLocalTab) {
      return;
    }

    let spec = CommandUtils.getCommandbarSpec("devtools.toolbox.toolbarSpec");
    let environment = CommandUtils.createEnvironment(this, '_target');
    this._requisition = CommandUtils.createRequisition(environment);
    let buttons = CommandUtils.createButtons(spec, this._target,
                                             this.doc, this._requisition);
    let container = this.doc.getElementById("toolbox-buttons");
    buttons.forEach(container.appendChild.bind(container));
    this.setToolboxButtonsVisibility();
  },

  



  _buildPickerButton: function() {
    this._pickerButton = this.doc.createElement("toolbarbutton");
    this._pickerButton.id = "command-button-pick";
    this._pickerButton.className = "command-button command-button-invertable";
    this._pickerButton.setAttribute("tooltiptext", toolboxStrings("pickButton.tooltip"));

    let container = this.doc.querySelector("#toolbox-picker-container");
    container.appendChild(this._pickerButton);

    this._togglePicker = this.highlighterUtils.togglePicker.bind(this.highlighterUtils);
    this._pickerButton.addEventListener("command", this._togglePicker, false);
  },

  



  get toolboxButtons() {
    
    
    return [
      "command-button-pick",
      "command-button-splitconsole",
      "command-button-responsive",
      "command-button-paintflashing",
      "command-button-tilt",
      "command-button-scratchpad",
      "command-button-eyedropper"
    ].map(id => {
      let button = this.doc.getElementById(id);
      
      if (!button) {
        return false;
      }
      return {
        id: id,
        button: button,
        label: button.getAttribute("tooltiptext"),
        visibilityswitch: "devtools." + id + ".enabled"
      }
    }).filter(button=>button);
  },

  



  setToolboxButtonsVisibility: function() {
    this.toolboxButtons.forEach(buttonSpec => {
      let {visibilityswitch, id, button}=buttonSpec;
      let on = true;
      try {
        on = Services.prefs.getBoolPref(visibilityswitch);
      } catch (ex) { }

      if (button) {
        if (on) {
          button.removeAttribute("hidden");
        } else {
          button.setAttribute("hidden", "true");
        }
      }
    });
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
    
    
    
    radio.className = "devtools-tab";
    radio.id = "toolbox-tab-" + id;
    radio.setAttribute("toolid", id);
    radio.setAttribute("ordinal", toolDefinition.ordinal);
    radio.setAttribute("tooltiptext", toolDefinition.tooltip);
    if (toolDefinition.invertIconForLightTheme) {
      radio.setAttribute("icon-invertable", "true");
    }

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

    if (!toolDefinition.bgTheme) {
      toolDefinition.bgTheme = "theme-toolbar";
    }
    let vbox = this.doc.createElement("vbox");
    vbox.className = "toolbox-panel " + toolDefinition.bgTheme;

    
    if (!this.doc.getElementById("toolbox-panel-" + id)) {
      vbox.id = "toolbox-panel-" + id;
    }

    if (id === "options") {
      
      
      let optionTabContainer = this.doc.getElementById("toolbox-option-container");
      optionTabContainer.appendChild(radio);
      deck.appendChild(vbox);
    } else {
      
      if (tabs.childNodes.length == 0 ||
          tabs.lastChild.getAttribute("ordinal") <= toolDefinition.ordinal) {
        tabs.appendChild(radio);
        deck.appendChild(vbox);
      } else {
        
        Array.some(tabs.childNodes, (node, i) => {
          if (+node.getAttribute("ordinal") > toolDefinition.ordinal) {
            tabs.insertBefore(radio, node);
            deck.insertBefore(vbox, deck.childNodes[i]);
            return true;
          }
          return false;
        });
      }
    }

    this._addKeysToWindow();
  },

  





  loadTool: function(id) {
    if (id === "inspector" && !this._inspector) {
      return this.initInspector().then(() => {
        return this.loadTool(id);
      });
    }

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
    iframe.style.visibility = "hidden";

    let vbox = this.doc.getElementById("toolbox-panel-" + id);
    vbox.appendChild(iframe);

    let onLoad = () => {
      
      iframe.style.visibility = "visible";

      let built = definition.build(iframe.contentWindow, this);
      promise.resolve(built).then((panel) => {
        this._toolPanels.set(id, panel);
        this.emit(id + "-ready", panel);
        gDevTools.emit(id + "-ready", this, panel);
        deferred.resolve(panel);
      }, console.error);
    };

    iframe.setAttribute("src", definition.url);

    
    
    
    
    
    
    if (iframe.contentWindow) {
      let domHelper = new DOMHelpers(iframe.contentWindow);
      domHelper.onceDOMReady(onLoad);
    } else {
      let callback = () => {
        iframe.removeEventListener("DOMContentLoaded", callback);
        onLoad();
      }
      iframe.addEventListener("DOMContentLoaded", callback);
    }

    return deferred.promise;
  },

  





  selectTool: function(id) {
    let selected = this.doc.querySelector(".devtools-tab[selected]");
    if (selected) {
      selected.removeAttribute("selected");
    }

    let tab = this.doc.getElementById("toolbox-tab-" + id);
    tab.setAttribute("selected", "true");

    
    
    let sep = this.doc.getElementById("toolbox-controls-separator");
    if (id === "options") {
      sep.setAttribute("invisible", "true");
    } else {
      sep.removeAttribute("invisible");
    }

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

    
    
    tabstrip.selectedItem = tab || tabstrip.childNodes[0];

    
    let deck = this.doc.getElementById("toolbox-deck");
    let panel = this.doc.getElementById("toolbox-panel-" + id);
    deck.selectedPanel = panel;

    this.currentToolId = id;
    this._refreshConsoleDisplay();
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

  


  focusConsoleInput: function() {
    let hud = this.getPanel("webconsole").hud;
    if (hud && hud.jsterm) {
      hud.jsterm.inputNode.focus();
    }
  },

  



  toggleSplitConsole: function() {
    let openedConsolePanel = this.currentToolId === "webconsole";

    
    if (!openedConsolePanel) {
      this._splitConsole = !this._splitConsole;
      this._refreshConsoleDisplay();
      this.emit("split-console");

      if (this._splitConsole) {
        this.loadTool("webconsole").then(() => {
          this.focusConsoleInput();
        });
      }
    }
  },

  


  selectNextTool: function() {
    let tools = this.doc.querySelectorAll(".devtools-tab");
    let selected = this.doc.querySelector(".devtools-tab[selected]");
    let nextIndex = [...tools].indexOf(selected) + 1;
    let next = tools[nextIndex] || tools[0];
    let tool = next.getAttribute("toolid");
    return this.selectTool(tool);
  },

  


  selectPreviousTool: function() {
    let tools = this.doc.querySelectorAll(".devtools-tab");
    let selected = this.doc.querySelector(".devtools-tab[selected]");
    let prevIndex = [...tools].indexOf(selected) - 1;
    let prev = tools[prevIndex] || tools[tools.length - 1];
    let tool = prev.getAttribute("toolid");
    return this.selectTool(tool);
  },

  





  highlightTool: function(id) {
    let tab = this.doc.getElementById("toolbox-tab-" + id);
    tab && tab.setAttribute("highlighted", "true");
  },

  





  unhighlightTool: function(id) {
    let tab = this.doc.getElementById("toolbox-tab-" + id);
    tab && tab.removeAttribute("highlighted");
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
                               toolName, this.target.url || this.target.name);
    this._host.setTitle(title);
  },

  












  _createHost: function(hostType, options) {
    if (!Hosts[hostType]) {
      throw new Error("Unknown hostType: " + hostType);
    }

    
    let newHost = new Hosts[hostType](this.target.tab, options);
    newHost.on("window-closed", this.destroy);
    return newHost;
  },

  






  switchHost: function(hostType) {
    if (hostType == this._host.type || !this._target.isLocalTab) {
      return null;
    }

    let newHost = this._createHost(hostType);
    return newHost.create().then(iframe => {
      
      iframe.QueryInterface(Ci.nsIFrameLoaderOwner);
      iframe.swapFrameLoaders(this.frame);

      this._host.off("window-closed", this.destroy);
      this.destroyHost();

      this._host = newHost;

      if (this.hostType != Toolbox.HostType.CUSTOM) {
        Services.prefs.setCharPref(this._prefs.LAST_HOST, this._host.type);
      }

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

  



  initInspector: function() {
    if (!this._initInspector) {
      this._initInspector = Task.spawn(function*() {
        this._inspector = InspectorFront(this._target.client, this._target.form);
        this._walker = yield this._inspector.getWalker();
        this._selection = new Selection(this._walker);

        if (this.highlighterUtils.isRemoteHighlightable) {
          let autohide = !gDevTools.testing;

          this.walker.on("highlighter-ready", this._highlighterReady);
          this.walker.on("highlighter-hide", this._highlighterHidden);

          this._highlighter = yield this._inspector.getHighlighter(autohide);
        }
      }.bind(this));
    }
    return this._initInspector;
  },

  



  destroyInspector: function() {
    if (this._destroying) {
      return this._destroying;
    }

    if (!this._inspector) {
      return promise.resolve();
    }

    let outstanding = () => {
      return Task.spawn(function*() {
        yield this.highlighterUtils.stopPicker();
        yield this._inspector.destroy();
        if (this._highlighter) {
          yield this._highlighter.destroy();
        }
        if (this._selection) {
          this._selection.destroy();
        }

        if (this.walker) {
          this.walker.off("highlighter-ready", this._highlighterReady);
          this.walker.off("highlighter-hide", this._highlighterHidden);
        }

        this._inspector = null;
        this._highlighter = null;
        this._selection = null;
        this._walker = null;
      }.bind(this));
    };

    
    
    
    let walker = (this._destroying = this._walker) ?
                 this._walker.release() :
                 promise.resolve();
    return walker.then(outstanding, outstanding);
  },

  




  getNotificationBox: function() {
    return this.doc.getElementById("toolbox-notificationbox");
  },

  




  destroyHost: function() {
    this.doc.removeEventListener("keypress",
      this._splitConsoleOnKeypress, false);
    return this._host.destroy();
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

    let outstanding = [];
    for (let [id, panel] of this._toolPanels) {
      try {
        outstanding.push(panel.destroy());
      } catch (e) {
        
        console.error("Panel " + id + ":", e);
      }
    }

    
    outstanding.push(this.destroyInspector());
    
    outstanding.push(() => {
      this._pickerButton.removeEventListener("command", this._togglePicker, false);
      this._pickerButton = null;
      let container = this.doc.getElementById("toolbox-buttons");
      while (container.firstChild) {
        container.removeChild(container.firstChild);
      }
    });
    
    outstanding.push(this.destroyHost());

    if (this.target.isLocalTab) {
      this._requisition.destroy();
    }
    this._telemetry.toolClosed("toolbox");
    this._telemetry.destroy();

    return this._destroyer = promise.all(outstanding).then(() => {
      
      
      
      
      if (!this._target) {
        return null;
      }
      let target = this._target;
      this._target = null;
      target.off("close", this.destroy);
      return target.destroy();
    }).then(() => {
      this.emit("destroyed");
      
      
      this._host = null;
      this._toolPanels.clear();
    }).then(null, console.error);
  },

  _highlighterReady: function() {
    this.emit("highlighter-ready");
  },

  _highlighterHidden: function() {
    this.emit("highlighter-hide");
  },
};






function ToolboxHighlighterUtils(toolbox) {
  this.toolbox = toolbox;
  this._onPickerNodeHovered = this._onPickerNodeHovered.bind(this);
  this._onPickerNodePicked = this._onPickerNodePicked.bind(this);
  this.stopPicker = this.stopPicker.bind(this);
}

ToolboxHighlighterUtils.prototype = {
  


  get isRemoteHighlightable() {
    return this.toolbox._target.client.traits.highlightable;
  },

  


  togglePicker: function() {
    if (this._isPicking) {
      return this.stopPicker();
    } else {
      return this.startPicker();
    }
  },

  _onPickerNodeHovered: function(res) {
    this.toolbox.emit("picker-node-hovered", res.node);
  },

  _onPickerNodePicked: function(res) {
    this.toolbox.selection.setNodeFront(res.node, "picker-node-picked");
    this.stopPicker();
  },

  








  startPicker: function() {
    if (this._isPicking) {
      return promise.resolve();
    }

    let deferred = promise.defer();

    let done = () => {
      this._isPicking = true;
      this.toolbox.emit("picker-started");
      this.toolbox.on("select", this.stopPicker);
      deferred.resolve();
    };

    promise.all([
      this.toolbox.initInspector(),
      this.toolbox.selectTool("inspector")
    ]).then(() => {
      this.toolbox._pickerButton.setAttribute("checked", "true");

      if (this.isRemoteHighlightable) {
        this.toolbox.walker.on("picker-node-hovered", this._onPickerNodeHovered);
        this.toolbox.walker.on("picker-node-picked", this._onPickerNodePicked);

        this.toolbox.highlighter.pick().then(done);
      } else {
        return this.toolbox.walker.pick().then(node => {
          this.toolbox.selection.setNodeFront(node, "picker-node-picked").then(() => {
            this.stopPicker();
            done();
          });
        });
      }
    });

    return deferred.promise;
  },

  




  stopPicker: function() {
    if (!this._isPicking) {
      return promise.resolve();
    }

    let deferred = promise.defer();

    let done = () => {
      this.toolbox.emit("picker-stopped");
      this.toolbox.off("select", this.stopPicker);
      deferred.resolve();
    };

    this.toolbox.initInspector().then(() => {
      this._isPicking = false;
      this.toolbox._pickerButton.removeAttribute("checked");
      if (this.isRemoteHighlightable) {
        this.toolbox.highlighter.cancelPick().then(done);
        this.toolbox.walker.off("picker-node-hovered", this._onPickerNodeHovered);
        this.toolbox.walker.off("picker-node-picked", this._onPickerNodePicked);
      } else {
        this.toolbox.walker.cancelPick().then(done);
      }
    });

    return deferred.promise;
  },

  





  highlightNodeFront: function(nodeFront, options={}) {
    let deferred = promise.defer();

    
    if (this.isRemoteHighlightable) {
      this.toolbox.initInspector().then(() => {
        this.toolbox.highlighter.showBoxModel(nodeFront, options).then(() => {
          this.toolbox.emit("node-highlight", nodeFront);
          deferred.resolve(nodeFront);
        });
      });
    }
    
    
    else {
      this.toolbox.walker.highlight(nodeFront).then(() => {
        this.toolbox.emit("node-highlight", nodeFront);
        deferred.resolve(nodeFront);
      });
    }

    return deferred.promise;
  },

  







  highlightDomValueGrip: function(valueGrip, options={}) {
    return this._translateGripToNodeFront(valueGrip).then(nodeFront => {
      if (nodeFront) {
        return this.highlightNodeFront(nodeFront, options);
      } else {
        return promise.reject();
      }
    });
  },

  _translateGripToNodeFront: function(grip) {
    return this.toolbox.initInspector().then(() => {
      return this.toolbox.walker.getNodeActorFromObjectActor(grip.actor);
    });
  },

  



  unhighlight: function(forceHide=false) {
    let unhighlightPromise;
    forceHide = forceHide || !gDevTools.testing;

    if (forceHide && this.isRemoteHighlightable && this.toolbox.highlighter) {
      
      unhighlightPromise = this.toolbox.highlighter.hideBoxModel();
    } else {
      
      
      unhighlightPromise = promise.resolve();
    }

    return unhighlightPromise.then(() => {
      this.toolbox.emit("node-unhighlight");
    });
  }
};
