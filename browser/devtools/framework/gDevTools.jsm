



"use strict";

this.EXPORTED_SYMBOLS = [ "gDevTools", "DevTools", "gDevToolsBrowser" ];

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/devtools/Loader.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "promise",
                                  "resource://gre/modules/Promise.jsm", "Promise");
XPCOMUtils.defineLazyModuleGetter(this, "console",
                                  "resource://gre/modules/devtools/Console.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "CustomizableUI",
                                  "resource:///modules/CustomizableUI.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "DebuggerServer",
                                  "resource://gre/modules/devtools/dbg-server.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "DebuggerClient",
                                  "resource://gre/modules/devtools/dbg-client.jsm");

const EventEmitter = devtools.require("devtools/toolkit/event-emitter");
const Telemetry = devtools.require("devtools/shared/telemetry");

const TABS_OPEN_PEAK_HISTOGRAM = "DEVTOOLS_TABS_OPEN_PEAK_LINEAR";
const TABS_OPEN_AVG_HISTOGRAM = "DEVTOOLS_TABS_OPEN_AVERAGE_LINEAR";
const TABS_PINNED_PEAK_HISTOGRAM = "DEVTOOLS_TABS_PINNED_PEAK_LINEAR";
const TABS_PINNED_AVG_HISTOGRAM = "DEVTOOLS_TABS_PINNED_AVERAGE_LINEAR";

const FORBIDDEN_IDS = new Set(["toolbox", ""]);
const MAX_ORDINAL = 99;

const bundle = Services.strings.createBundle("chrome://browser/locale/devtools/toolbox.properties");





this.DevTools = function DevTools() {
  this._tools = new Map();     
  this._themes = new Map();    
  this._toolboxes = new Map(); 
  this._telemetry = new Telemetry();

  
  this.destroy = this.destroy.bind(this);
  this._teardown = this._teardown.bind(this);

  this._testing = false;

  EventEmitter.decorate(this);

  Services.obs.addObserver(this._teardown, "devtools-unloaded", false);
  Services.obs.addObserver(this.destroy, "quit-application", false);
};

DevTools.prototype = {
  





  get testing() {
    return this._testing;
  },

  set testing(state) {
    let oldState = this._testing;
    this._testing = state;

    if (state !== oldState) {
      if (state) {
        this._savedSendAfterPaintToContentPref =
          Services.prefs.getBoolPref("dom.send_after_paint_to_content");

        
        
        
        Services.prefs.setBoolPref("dom.send_after_paint_to_content", false);
      } else {
        Services.prefs.setBoolPref("dom.send_after_paint_to_content",
                                   this._savedSendAfterPaintToContentPref);
      }
    }
  },

  




























  registerTool: function DT_registerTool(toolDefinition) {
    let toolId = toolDefinition.id;

    if (!toolId || FORBIDDEN_IDS.has(toolId)) {
      throw new Error("Invalid definition.id");
    }

    
    
    
    if (devtools.defaultTools && devtools.defaultTools.indexOf(toolDefinition) == -1) {
      toolDefinition.visibilityswitch = "devtools." + toolId + ".enabled";
    }

    this._tools.set(toolId, toolDefinition);

    this.emit("tool-registered", toolId);
  },

  










  unregisterTool: function DT_unregisterTool(tool, isQuitApplication) {
    let toolId = null;
    if (typeof tool == "string") {
      toolId = tool;
      tool = this._tools.get(tool);
    }
    else {
      toolId = tool.id;
    }
    this._tools.delete(toolId);

    if (!isQuitApplication) {
      this.emit("tool-unregistered", tool);
    }
  },

  


  ordinalSort: function DT_ordinalSort(d1, d2) {
    let o1 = (typeof d1.ordinal == "number") ? d1.ordinal : MAX_ORDINAL;
    let o2 = (typeof d2.ordinal == "number") ? d2.ordinal : MAX_ORDINAL;
    return o1 - o2;
  },

  getDefaultTools: function DT_getDefaultTools() {
    return devtools.defaultTools.sort(this.ordinalSort);
  },

  getAdditionalTools: function DT_getAdditionalTools() {
    let tools = [];
    for (let [key, value] of this._tools) {
      if (devtools.defaultTools.indexOf(value) == -1) {
        tools.push(value);
      }
    }
    return tools.sort(this.ordinalSort);
  },

  








  getToolDefinition: function DT_getToolDefinition(toolId) {
    let tool = this._tools.get(toolId);
    if (!tool) {
      return null;
    } else if (!tool.visibilityswitch) {
      return tool;
    }

    let enabled;
    try {
      enabled = Services.prefs.getBoolPref(tool.visibilityswitch);
    } catch (e) {
      enabled = true;
    }

    return enabled ? tool : null;
  },

  






  getToolDefinitionMap: function DT_getToolDefinitionMap() {
    let tools = new Map();

    for (let [id, definition] of this._tools) {
      if (this.getToolDefinition(id)) {
        tools.set(id, definition);
      }
    }

    return tools;
  },

  







  getToolDefinitionArray: function DT_getToolDefinitionArray() {
    let definitions = [];

    for (let [id, definition] of this._tools) {
      if (this.getToolDefinition(id)) {
        definitions.push(definition);
      }
    }

    return definitions.sort(this.ordinalSort);
  },

  





















  registerTheme: function DT_registerTheme(themeDefinition) {
    let themeId = themeDefinition.id;

    if (!themeId) {
      throw new Error("Invalid theme id");
    }

    if (this._themes.get(themeId)) {
      throw new Error("Theme with the same id is already registered");
    }

    this._themes.set(themeId, themeDefinition);

    this.emit("theme-registered", themeId);
  },

  






  unregisterTheme: function DT_unregisterTheme(theme) {
    let themeId = null;
    if (typeof theme == "string") {
      themeId = theme;
      theme = this._themes.get(theme);
    }
    else {
      themeId = theme.id;
    }

    let currTheme = Services.prefs.getCharPref("devtools.theme");

    
    
    
    if (!Services.startup.shuttingDown && theme.id == currTheme) {
      Services.prefs.setCharPref("devtools.theme", "light");

      let data = {
        pref: "devtools.theme",
        newValue: "light",
        oldValue: currTheme
      };

      gDevTools.emit("pref-changed", data);

      this.emit("theme-unregistered", theme);
    }

    this._themes.delete(themeId);
  },

  








  getThemeDefinition: function DT_getThemeDefinition(themeId) {
    let theme = this._themes.get(themeId);
    if (!theme) {
      return null;
    }
    return theme;
  },

  





  getThemeDefinitionMap: function DT_getThemeDefinitionMap() {
    let themes = new Map();

    for (let [id, definition] of this._themes) {
      if (this.getThemeDefinition(id)) {
        themes.set(id, definition);
      }
    }

    return themes;
  },

  





  getThemeDefinitionArray: function DT_getThemeDefinitionArray() {
    let definitions = [];

    for (let [id, definition] of this._themes) {
      if (this.getThemeDefinition(id)) {
        definitions.push(definition);
      }
    }

    return definitions.sort(this.ordinalSort);
  },

  



















  showToolbox: function(target, toolId, hostType, hostOptions) {
    let deferred = promise.defer();

    let toolbox = this._toolboxes.get(target);
    if (toolbox) {

      let hostPromise = (hostType != null && toolbox.hostType != hostType) ?
          toolbox.switchHost(hostType) :
          promise.resolve(null);

      if (toolId != null && toolbox.currentToolId != toolId) {
        hostPromise = hostPromise.then(function() {
          return toolbox.selectTool(toolId);
        });
      }

      return hostPromise.then(function() {
        toolbox.raise();
        return toolbox;
      });
    }
    else {
      
      toolbox = new devtools.Toolbox(target, toolId, hostType, hostOptions);

      this.emit("toolbox-created", toolbox);

      this._toolboxes.set(target, toolbox);

      toolbox.once("destroy", () => {
        this.emit("toolbox-destroy", target);
      });

      toolbox.once("destroyed", () => {
        this._toolboxes.delete(target);
        this.emit("toolbox-destroyed", target);
      });

      
      
      toolbox.open().then(() => {
        deferred.resolve(toolbox);
        this.emit("toolbox-ready", toolbox);
      });
    }

    return deferred.promise;
  },

  








  getToolbox: function DT_getToolbox(target) {
    return this._toolboxes.get(target);
  },

  







  closeToolbox: function DT_closeToolbox(target) {
    let toolbox = this._toolboxes.get(target);
    if (toolbox == null) {
      return promise.resolve(false);
    }
    return toolbox.destroy().then(() => true);
  },

  _pingTelemetry: function() {
    let mean = function(arr) {
      if (arr.length === 0) {
        return 0;
      }

      let total = arr.reduce((a, b) => a + b);
      return Math.ceil(total / arr.length);
    };

    let tabStats = gDevToolsBrowser._tabStats;
    this._telemetry.log(TABS_OPEN_PEAK_HISTOGRAM, tabStats.peakOpen);
    this._telemetry.log(TABS_OPEN_AVG_HISTOGRAM, mean(tabStats.histOpen));
    this._telemetry.log(TABS_PINNED_PEAK_HISTOGRAM, tabStats.peakPinned);
    this._telemetry.log(TABS_PINNED_AVG_HISTOGRAM, mean(tabStats.histPinned));
  },

  


  _teardown: function DT_teardown() {
    for (let [target, toolbox] of this._toolboxes) {
      toolbox.destroy();
    }
  },

  


  destroy: function() {
    Services.obs.removeObserver(this.destroy, "quit-application");
    Services.obs.removeObserver(this._teardown, "devtools-unloaded");

    for (let [key, tool] of this.getToolDefinitionMap()) {
      this.unregisterTool(key, true);
    }

    this._pingTelemetry();
    this._telemetry = null;

    
    
    
  },

  


  *[Symbol.iterator]() {
    for (let toolbox of this._toolboxes) {
      yield toolbox;
    }
  }
};







let gDevTools = new DevTools();
this.gDevTools = gDevTools;





let gDevToolsBrowser = {
  



  _trackedBrowserWindows: new Set(),

  _tabStats: {
    peakOpen: 0,
    peakPinned: 0,
    histOpen: [],
    histPinned: []
  },

  




  toggleToolboxCommand: function(gBrowser) {
    let target = devtools.TargetFactory.forTab(gBrowser.selectedTab);
    let toolbox = gDevTools.getToolbox(target);

    toolbox ? toolbox.destroy() : gDevTools.showToolbox(target);
  },

  toggleBrowserToolboxCommand: function(gBrowser) {
    let target = devtools.TargetFactory.forWindow(gBrowser.ownerDocument.defaultView);
    let toolbox = gDevTools.getToolbox(target);

    toolbox ? toolbox.destroy()
     : gDevTools.showToolbox(target, "inspector", Toolbox.HostType.WINDOW);
  },

  




  updateCommandAvailability: function(win) {
    let doc = win.document;

    function toggleCmd(id, isEnabled) {
      let cmd = doc.getElementById(id);
      if (isEnabled) {
        cmd.removeAttribute("disabled");
        cmd.removeAttribute("hidden");
      } else {
        cmd.setAttribute("disabled", "true");
        cmd.setAttribute("hidden", "true");
      }
    };

    
    let devToolbarEnabled = Services.prefs.getBoolPref("devtools.toolbar.enabled");
    toggleCmd("Tools:DevToolbar", devToolbarEnabled);
    let focusEl = doc.getElementById("Tools:DevToolbarFocus");
    if (devToolbarEnabled) {
      focusEl.removeAttribute("disabled");
    } else {
      focusEl.setAttribute("disabled", "true");
    }
    if (devToolbarEnabled && Services.prefs.getBoolPref("devtools.toolbar.visible")) {
      win.DeveloperToolbar.show(false).catch(console.error);
    }

    
    let webIDEEnabled = Services.prefs.getBoolPref("devtools.webide.enabled");
    toggleCmd("Tools:WebIDE", webIDEEnabled);

    let showWebIDEWidget = Services.prefs.getBoolPref("devtools.webide.widget.enabled");
    if (webIDEEnabled && showWebIDEWidget) {
      gDevToolsBrowser.installWebIDEWidget();
    } else {
      gDevToolsBrowser.uninstallWebIDEWidget();
    }

    
    let appMgrEnabled = Services.prefs.getBoolPref("devtools.appmanager.enabled");
    toggleCmd("Tools:DevAppMgr", !webIDEEnabled && appMgrEnabled);

    
    let chromeEnabled = Services.prefs.getBoolPref("devtools.chrome.enabled");
    let devtoolsRemoteEnabled = Services.prefs.getBoolPref("devtools.debugger.remote-enabled");
    let remoteEnabled = chromeEnabled && devtoolsRemoteEnabled;
    toggleCmd("Tools:BrowserToolbox", remoteEnabled);
    toggleCmd("Tools:BrowserContentToolbox", remoteEnabled && win.gMultiProcessBrowser);

    
    let consoleEnabled = Services.prefs.getBoolPref("devtools.errorconsole.enabled");
    toggleCmd("Tools:ErrorConsole", consoleEnabled);

    
    toggleCmd("Tools:DevToolsConnect", devtoolsRemoteEnabled);
  },

  observe: function(subject, topic, prefName) {
    if (prefName.endsWith("enabled")) {
      for (let win of this._trackedBrowserWindows) {
        this.updateCommandAvailability(win);
      }
    }
  },

  _prefObserverRegistered: false,

  ensurePrefObserver: function() {
    if (!this._prefObserverRegistered) {
      this._prefObserverRegistered = true;
      Services.prefs.addObserver("devtools.", this, false);
    }
  },


  













  selectToolCommand: function(gBrowser, toolId) {
    let target = devtools.TargetFactory.forTab(gBrowser.selectedTab);
    let toolbox = gDevTools.getToolbox(target);
    let toolDefinition = gDevTools.getToolDefinition(toolId);

    if (toolbox &&
        (toolbox.currentToolId == toolId ||
          (toolId == "webconsole" && toolbox.splitConsole)))
    {
      toolbox.fireCustomKey(toolId);

      if (toolDefinition.preventClosingOnKey || toolbox.hostType == devtools.Toolbox.HostType.WINDOW) {
        toolbox.raise();
      } else {
        toolbox.destroy();
      }
      gDevTools.emit("select-tool-command", toolId);
    } else {
      gDevTools.showToolbox(target, toolId).then(() => {
        let target = devtools.TargetFactory.forTab(gBrowser.selectedTab);
        let toolbox = gDevTools.getToolbox(target);

        toolbox.fireCustomKey(toolId);
        gDevTools.emit("select-tool-command", toolId);
      });
    }
  },

  


  openConnectScreen: function(gBrowser) {
    gBrowser.selectedTab = gBrowser.addTab("chrome://browser/content/devtools/connect.xhtml");
  },

  


  openAppManager: function(gBrowser) {
    gBrowser.selectedTab = gBrowser.addTab("about:app-manager");
  },

  


  openWebIDE: function() {
    let win = Services.wm.getMostRecentWindow("devtools:webide");
    if (win) {
      win.focus();
    } else {
      Services.ww.openWindow(null, "chrome://webide/content/", "webide", "chrome,centerscreen,resizable", null);
    }
  },

  _getContentProcessTarget: function () {
    
    if (!DebuggerServer.initialized) {
      DebuggerServer.init();
      DebuggerServer.addBrowserActors();
    }
    DebuggerServer.allowChromeProcess = true;

    let transport = DebuggerServer.connectPipe();
    let client = new DebuggerClient(transport);

    let deferred = promise.defer();
    client.connect(() => {
      client.mainRoot.listProcesses(response => {
        
        let contentProcesses = response.processes.filter(p => (!p.parent));
        if (contentProcesses.length < 1) {
          let msg = bundle.GetStringFromName("toolbox.noContentProcess.message");
          Services.prompt.alert(null, "", msg);
          deferred.reject("No content processes available.");
          return;
        }
        
        client.getProcess(contentProcesses[0].id)
              .then(response => {
                let options = {
                  form: response.form,
                  client: client,
                  chrome: true,
                  isTabActor: false
                };
                return devtools.TargetFactory.forRemoteTab(options);
              })
              .then(target => {
                
                
                
                target.on("close", () => {
                  client.close();
                });
                deferred.resolve(target);
              });
      });
    });

    return deferred.promise;
  },

  openContentProcessToolbox: function () {
    this._getContentProcessTarget()
        .then(target => {
          
          return gDevTools.showToolbox(target, "jsdebugger",
                                       devtools.Toolbox.HostType.WINDOW);
        });
  },

  


  installWebIDEWidget: function() {
    if (this.isWebIDEWidgetInstalled()) {
      return;
    }

    let defaultArea;
    if (Services.prefs.getBoolPref("devtools.webide.widget.inNavbarByDefault")) {
      defaultArea = CustomizableUI.AREA_NAVBAR;
    } else {
      defaultArea = CustomizableUI.AREA_PANEL;
    }

    CustomizableUI.createWidget({
      id: "webide-button",
      shortcutId: "key_webide",
      label: "devtools-webide-button2.label",
      tooltiptext: "devtools-webide-button2.tooltiptext",
      defaultArea: defaultArea,
      onCommand: function(aEvent) {
        gDevToolsBrowser.openWebIDE();
      }
    });
  },

  isWebIDEWidgetInstalled: function() {
    let widgetWrapper = CustomizableUI.getWidget("webide-button");
    return !!(widgetWrapper && widgetWrapper.provider == CustomizableUI.PROVIDER_API);
  },

  


  isWebIDEInitialized: promise.defer(),

  


  uninstallWebIDEWidget: function() {
    if (this.isWebIDEWidgetInstalled()) {
      CustomizableUI.removeWidgetFromArea("webide-button");
    }
    CustomizableUI.destroyWidget("webide-button");
  },

  


  moveWebIDEWidgetInNavbar: function() {
    CustomizableUI.addWidgetToArea("webide-button", CustomizableUI.AREA_NAVBAR);
  },

  





  registerBrowserWindow: function DT_registerBrowserWindow(win) {
    this.updateCommandAvailability(win);
    this.ensurePrefObserver();
    gDevToolsBrowser._trackedBrowserWindows.add(win);
    gDevToolsBrowser._addAllToolsToMenu(win.document);

    if (this._isFirebugInstalled()) {
      let broadcaster = win.document.getElementById("devtoolsMenuBroadcaster_DevToolbox");
      broadcaster.removeAttribute("key");
    }

    let tabContainer = win.document.getElementById("tabbrowser-tabs");
    tabContainer.addEventListener("TabSelect", this, false);
    tabContainer.addEventListener("TabOpen", this, false);
    tabContainer.addEventListener("TabClose", this, false);
    tabContainer.addEventListener("TabPinned", this, false);
    tabContainer.addEventListener("TabUnpinned", this, false);
  },

  










  attachKeybindingsToBrowser: function DT_attachKeybindingsToBrowser(doc, keys) {
    let devtoolsKeyset = doc.getElementById("devtoolsKeyset");

    if (!devtoolsKeyset) {
      devtoolsKeyset = doc.createElement("keyset");
      devtoolsKeyset.setAttribute("id", "devtoolsKeyset");
    }
    devtoolsKeyset.appendChild(keys);
    let mainKeyset = doc.getElementById("mainKeyset");
    mainKeyset.parentNode.insertBefore(devtoolsKeyset, mainKeyset);
  },

  



  setSlowScriptDebugHandler: function DT_setSlowScriptDebugHandler() {
    let debugService = Cc["@mozilla.org/dom/slow-script-debug;1"]
                         .getService(Ci.nsISlowScriptDebug);
    let tm = Cc["@mozilla.org/thread-manager;1"].getService(Ci.nsIThreadManager);

    function slowScriptDebugHandler(aTab, aCallback) {
      let target = devtools.TargetFactory.forTab(aTab);

      gDevTools.showToolbox(target, "jsdebugger").then(toolbox => {
        let threadClient = toolbox.getCurrentPanel().panelWin.gThreadClient;

        
        
        switch (threadClient.state) {
          case "paused":
            
            threadClient.breakOnNext();
            aCallback();
            break;
          case "attached":
            
            threadClient.interrupt(() => {
              threadClient.breakOnNext();
              aCallback();
            });
            break;
          case "resuming":
            
            threadClient.addOneTimeListener("resumed", () => {
              threadClient.interrupt(() => {
                threadClient.breakOnNext();
                aCallback();
              });
            });
            break;
          default:
            throw Error("invalid thread client state in slow script debug handler: " +
                        threadClient.state);
          }
      });
    }

    debugService.activationHandler = function(aWindow) {
      let chromeWindow = aWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                                .getInterface(Ci.nsIWebNavigation)
                                .QueryInterface(Ci.nsIDocShellTreeItem)
                                .rootTreeItem
                                .QueryInterface(Ci.nsIInterfaceRequestor)
                                .getInterface(Ci.nsIDOMWindow)
                                .QueryInterface(Ci.nsIDOMChromeWindow);

      let setupFinished = false;
      slowScriptDebugHandler(chromeWindow.gBrowser.selectedTab,
                             () => { setupFinished = true; });

      
      
      let utils = aWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                         .getInterface(Ci.nsIDOMWindowUtils);
      utils.enterModalState();
      while (!setupFinished) {
        tm.currentThread.processNextEvent(true);
      }
      utils.leaveModalState();
    };

    debugService.remoteActivationHandler = function(aBrowser, aCallback) {
      let chromeWindow = aBrowser.ownerDocument.defaultView;
      let tab = chromeWindow.gBrowser.getTabForBrowser(aBrowser);
      chromeWindow.gBrowser.selected = tab;

      function callback() {
        aCallback.finishDebuggerStartup();
      }

      slowScriptDebugHandler(tab, callback);
    };
  },

  


  unsetSlowScriptDebugHandler: function DT_unsetSlowScriptDebugHandler() {
    let debugService = Cc["@mozilla.org/dom/slow-script-debug;1"]
                         .getService(Ci.nsISlowScriptDebug);
    debugService.activationHandler = undefined;
  },

  




  _isFirebugInstalled: function DT_isFirebugInstalled() {
    let bootstrappedAddons = Services.prefs.getCharPref("extensions.bootstrappedAddons");
    return bootstrappedAddons.indexOf("firebug@software.joehewitt.com") != -1;
  },

  





  _addToolToWindows: function DT_addToolToWindows(toolDefinition) {
    
    if (!toolDefinition.inMenu) {
      return;
    }

    
    try {
      if (toolDefinition.visibilityswitch &&
         !Services.prefs.getBoolPref(toolDefinition.visibilityswitch)) {
        return;
      }
    } catch(e) {}

    
    
    let allDefs = gDevTools.getToolDefinitionArray();
    let prevDef;
    for (let def of allDefs) {
      if (!def.inMenu) {
        continue;
      }
      if (def === toolDefinition) {
        break;
      }
      prevDef = def;
    }

    for (let win of gDevToolsBrowser._trackedBrowserWindows) {
      let doc = win.document;
      let elements = gDevToolsBrowser._createToolMenuElements(toolDefinition, doc);

      doc.getElementById("mainCommandSet").appendChild(elements.cmd);

      if (elements.key) {
        this.attachKeybindingsToBrowser(doc, elements.key);
      }

      doc.getElementById("mainBroadcasterSet").appendChild(elements.bc);

      let amp = doc.getElementById("appmenu_webDeveloper_popup");
      if (amp) {
        let ref;

        if (prevDef != null) {
          let menuitem = doc.getElementById("appmenuitem_" + prevDef.id);
          ref = menuitem && menuitem.nextSibling ? menuitem.nextSibling : null;
        } else {
          ref = doc.getElementById("appmenu_devtools_separator");
        }

        if (ref) {
          amp.insertBefore(elements.appmenuitem, ref);
        }
      }

      let mp = doc.getElementById("menuWebDeveloperPopup");
      if (mp) {
        let ref;

        if (prevDef != null) {
          let menuitem = doc.getElementById("menuitem_" + prevDef.id);
          ref = menuitem && menuitem.nextSibling ? menuitem.nextSibling : null;
        } else {
          ref = doc.getElementById("menu_devtools_separator");
        }

        if (ref) {
          mp.insertBefore(elements.menuitem, ref);
        }
      }
    }

    if (toolDefinition.id === "jsdebugger") {
      gDevToolsBrowser.setSlowScriptDebugHandler();
    }
  },

  





  _addAllToolsToMenu: function DT_addAllToolsToMenu(doc) {
    let fragCommands = doc.createDocumentFragment();
    let fragKeys = doc.createDocumentFragment();
    let fragBroadcasters = doc.createDocumentFragment();
    let fragAppMenuItems = doc.createDocumentFragment();
    let fragMenuItems = doc.createDocumentFragment();

    for (let toolDefinition of gDevTools.getToolDefinitionArray()) {
      if (!toolDefinition.inMenu) {
        continue;
      }

      let elements = gDevToolsBrowser._createToolMenuElements(toolDefinition, doc);

      if (!elements) {
        return;
      }

      fragCommands.appendChild(elements.cmd);
      if (elements.key) {
        fragKeys.appendChild(elements.key);
      }
      fragBroadcasters.appendChild(elements.bc);
      fragAppMenuItems.appendChild(elements.appmenuitem);
      fragMenuItems.appendChild(elements.menuitem);
    }

    let mcs = doc.getElementById("mainCommandSet");
    mcs.appendChild(fragCommands);

    this.attachKeybindingsToBrowser(doc, fragKeys);

    let mbs = doc.getElementById("mainBroadcasterSet");
    mbs.appendChild(fragBroadcasters);

    let amp = doc.getElementById("appmenu_webDeveloper_popup");
    if (amp) {
      let amps = doc.getElementById("appmenu_devtools_separator");
      amp.insertBefore(fragAppMenuItems, amps);
    }

    let mp = doc.getElementById("menuWebDeveloperPopup");
    let mps = doc.getElementById("menu_devtools_separator");
    mp.insertBefore(fragMenuItems, mps);
  },

  







  _createToolMenuElements: function DT_createToolMenuElements(toolDefinition, doc) {
    let id = toolDefinition.id;

    
    if (doc.getElementById("Tools:" + id)) {
      return;
    }

    let cmd = doc.createElement("command");
    cmd.id = "Tools:" + id;
    cmd.setAttribute("oncommand",
        'gDevToolsBrowser.selectToolCommand(gBrowser, "' + id + '");');

    let key = null;
    if (toolDefinition.key) {
      key = doc.createElement("key");
      key.id = "key_" + id;

      if (toolDefinition.key.startsWith("VK_")) {
        key.setAttribute("keycode", toolDefinition.key);
      } else {
        key.setAttribute("key", toolDefinition.key);
      }

      key.setAttribute("command", cmd.id);
      key.setAttribute("modifiers", toolDefinition.modifiers);
    }

    let bc = doc.createElement("broadcaster");
    bc.id = "devtoolsMenuBroadcaster_" + id;
    bc.setAttribute("label", toolDefinition.menuLabel || toolDefinition.label);
    bc.setAttribute("command", cmd.id);

    if (key) {
      bc.setAttribute("key", "key_" + id);
    }

    let appmenuitem = doc.createElement("menuitem");
    appmenuitem.id = "appmenuitem_" + id;
    appmenuitem.setAttribute("observes", "devtoolsMenuBroadcaster_" + id);

    let menuitem = doc.createElement("menuitem");
    menuitem.id = "menuitem_" + id;
    menuitem.setAttribute("observes", "devtoolsMenuBroadcaster_" + id);

    if (toolDefinition.accesskey) {
      menuitem.setAttribute("accesskey", toolDefinition.accesskey);
    }

    return {
      cmd: cmd,
      key: key,
      bc: bc,
      appmenuitem: appmenuitem,
      menuitem: menuitem
    };
  },

  



  _updateMenuCheckbox: function DT_updateMenuCheckbox() {
    for (let win of gDevToolsBrowser._trackedBrowserWindows) {

      let hasToolbox = false;
      if (devtools.TargetFactory.isKnownTab(win.gBrowser.selectedTab)) {
        let target = devtools.TargetFactory.forTab(win.gBrowser.selectedTab);
        if (gDevTools._toolboxes.has(target)) {
          hasToolbox = true;
        }
      }

      let broadcaster = win.document.getElementById("devtoolsMenuBroadcaster_DevToolbox");
      if (hasToolbox) {
        broadcaster.setAttribute("checked", "true");
      } else {
        broadcaster.removeAttribute("checked");
      }
    }
  },

  





  _removeToolFromWindows: function DT_removeToolFromWindows(toolId) {
    for (let win of gDevToolsBrowser._trackedBrowserWindows) {
      gDevToolsBrowser._removeToolFromMenu(toolId, win.document);
    }

    if (toolId === "jsdebugger") {
      gDevToolsBrowser.unsetSlowScriptDebugHandler();
    }
  },

  







  _removeToolFromMenu: function DT_removeToolFromMenu(toolId, doc) {
    let command = doc.getElementById("Tools:" + toolId);
    if (command) {
      command.parentNode.removeChild(command);
    }

    let key = doc.getElementById("key_" + toolId);
    if (key) {
      key.parentNode.removeChild(key);
    }

    let bc = doc.getElementById("devtoolsMenuBroadcaster_" + toolId);
    if (bc) {
      bc.parentNode.removeChild(bc);
    }

    let appmenuitem = doc.getElementById("appmenuitem_" + toolId);
    if (appmenuitem) {
      appmenuitem.parentNode.removeChild(appmenuitem);
    }

    let menuitem = doc.getElementById("menuitem_" + toolId);
    if (menuitem) {
      menuitem.parentNode.removeChild(menuitem);
    }
  },

  






  forgetBrowserWindow: function DT_forgetBrowserWindow(win) {
    gDevToolsBrowser._trackedBrowserWindows.delete(win);

    
    for (let [target, toolbox] of gDevTools._toolboxes) {
      if (toolbox.frame && toolbox.frame.ownerDocument.defaultView == win) {
        toolbox.destroy();
      }
    }

    let tabContainer = win.document.getElementById("tabbrowser-tabs");
    tabContainer.removeEventListener("TabSelect", this, false);
    tabContainer.removeEventListener("TabOpen", this, false);
    tabContainer.removeEventListener("TabClose", this, false);
    tabContainer.removeEventListener("TabPinned", this, false);
    tabContainer.removeEventListener("TabUnpinned", this, false);
  },

  handleEvent: function(event) {
    switch (event.type) {
      case "TabOpen":
      case "TabClose":
      case "TabPinned":
      case "TabUnpinned":
        let open = 0;
        let pinned = 0;

        for (let win of this._trackedBrowserWindows) {
          let tabContainer = win.gBrowser.tabContainer;
          let numPinnedTabs = tabContainer.tabbrowser._numPinnedTabs;
          let numTabs = tabContainer.itemCount - numPinnedTabs;

          open += numTabs;
          pinned += numPinnedTabs;
        }

        this._tabStats.histOpen.push(open);
        this._tabStats.histPinned.push(pinned);
        this._tabStats.peakOpen = Math.max(open, this._tabStats.peakOpen);
        this._tabStats.peakPinned = Math.max(pinned, this._tabStats.peakPinned);
      break;
      case "TabSelect":
        gDevToolsBrowser._updateMenuCheckbox();
    }
  },

  


  destroy: function() {
    Services.prefs.removeObserver("devtools.", gDevToolsBrowser);
    Services.obs.removeObserver(gDevToolsBrowser.destroy, "quit-application");
  },
}

this.gDevToolsBrowser = gDevToolsBrowser;

gDevTools.on("tool-registered", function(ev, toolId) {
  let toolDefinition = gDevTools._tools.get(toolId);
  gDevToolsBrowser._addToolToWindows(toolDefinition);
});

gDevTools.on("tool-unregistered", function(ev, toolId) {
  if (typeof toolId != "string") {
    toolId = toolId.id;
  }
  gDevToolsBrowser._removeToolFromWindows(toolId);
});

gDevTools.on("toolbox-ready", gDevToolsBrowser._updateMenuCheckbox);
gDevTools.on("toolbox-destroyed", gDevToolsBrowser._updateMenuCheckbox);

Services.obs.addObserver(gDevToolsBrowser.destroy, "quit-application", false);


devtools.main("main");
