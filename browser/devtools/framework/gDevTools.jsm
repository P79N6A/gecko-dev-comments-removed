



"use strict";

this.EXPORTED_SYMBOLS = [ "gDevTools", "DevTools", "gDevToolsBrowser" ];

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource:///modules/devtools/shared/event-emitter.js");
Cu.import("resource://gre/modules/commonjs/sdk/core/promise.js");
Cu.import("resource://gre/modules/devtools/Loader.jsm");

var ProfilerController = devtools.require("devtools/profiler/controller");

const FORBIDDEN_IDS = new Set(["toolbox", ""]);
const MAX_ORDINAL = 99;





this.DevTools = function DevTools() {
  this._tools = new Map();     
  this._toolboxes = new Map(); 

  
  this.destroy = this.destroy.bind(this);
  this._teardown = this._teardown.bind(this);

  EventEmitter.decorate(this);

  Services.obs.addObserver(this._teardown, "devtools-unloaded", false);
  Services.obs.addObserver(this.destroy, "quit-application", false);
}

DevTools.prototype = {
  






















  registerTool: function DT_registerTool(toolDefinition) {
    let toolId = toolDefinition.id;

    if (!toolId || FORBIDDEN_IDS.has(toolId)) {
      throw new Error("Invalid definition.id");
    }

    toolDefinition.visibilityswitch = toolDefinition.visibilityswitch ||
        "devtools." + toolId + ".enabled";
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

  






  getToolDefinitionMap: function DT_getToolDefinitionMap() {
    let tools = new Map();

    for (let [key, value] of this._tools) {
      let enabled;

      try {
        enabled = Services.prefs.getBoolPref(value.visibilityswitch);
      } catch(e) {
        enabled = true;
      }

      if (enabled || value.id == "options") {
        tools.set(key, value);
      }
    }
    return tools;
  },

  







  getToolDefinitionArray: function DT_getToolDefinitionArray() {
    let definitions = [];
    for (let [id, definition] of this.getToolDefinitionMap()) {
      definitions.push(definition);
    }

    return definitions.sort(this.ordinalSort);
  },

  

















  showToolbox: function(target, toolId, hostType) {
    let deferred = Promise.defer();

    let toolbox = this._toolboxes.get(target);
    if (toolbox) {

      let promise = (hostType != null && toolbox.hostType != hostType) ?
          toolbox.switchHost(hostType) :
          Promise.resolve(null);

      if (toolId != null && toolbox.currentToolId != toolId) {
        promise = promise.then(function() {
          return toolbox.selectTool(toolId);
        });
      }

      return promise.then(function() {
        toolbox.raise();
        return toolbox;
      });
    }
    else {
      
      toolbox = new devtools.Toolbox(target, toolId, hostType);

      this._toolboxes.set(target, toolbox);

      toolbox.once("destroyed", function() {
        this._toolboxes.delete(target);
        this.emit("toolbox-destroyed", target);
      }.bind(this));

      
      
      if (toolId != null) {
        toolbox.once(toolId + "-ready", function(event, panel) {
          this.emit("toolbox-ready", toolbox);
          deferred.resolve(toolbox);
        }.bind(this));
        toolbox.open();
      }
      else {
        toolbox.open().then(function() {
          deferred.resolve(toolbox);
          this.emit("toolbox-ready", toolbox);
        }.bind(this));
      }
    }

    return deferred.promise;
  },

  








  getToolbox: function DT_getToolbox(target) {
    return this._toolboxes.get(target);
  },

  


  closeToolbox: function DT_closeToolbox(target) {
    let toolbox = this._toolboxes.get(target);
    if (toolbox == null) {
      return;
    }
    return toolbox.destroy();
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

    
    
    
  },
};







let gDevTools = new DevTools();
this.gDevTools = gDevTools;





let gDevToolsBrowser = {
  



  _trackedBrowserWindows: new Set(),

  




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

  













  selectToolCommand: function(gBrowser, toolId) {
    let target = devtools.TargetFactory.forTab(gBrowser.selectedTab);
    let toolbox = gDevTools.getToolbox(target);

    if (toolbox && toolbox.currentToolId == toolId) {
      if (toolbox.hostType == devtools.Toolbox.HostType.WINDOW) {
        toolbox.raise();
      } else {
        toolbox.destroy();
      }
    } else {
      gDevTools.showToolbox(target, toolId);
    }
  },

  


  openConnectScreen: function(gBrowser) {
    gBrowser.selectedTab = gBrowser.addTab("chrome://browser/content/devtools/connect.xhtml");
  },

  





  registerBrowserWindow: function DT_registerBrowserWindow(win) {
    gDevToolsBrowser._trackedBrowserWindows.add(win);
    gDevToolsBrowser._addAllToolsToMenu(win.document);

    let tabContainer = win.document.getElementById("tabbrowser-tabs")
    tabContainer.addEventListener("TabSelect",
                                  gDevToolsBrowser._updateMenuCheckbox, false);
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

  





  _addToolToWindows: function DT_addToolToWindows(toolDefinition) {
    
    if (toolDefinition.id == "options") {
      return;
    }

    
    try {
      if (!Services.prefs.getBoolPref(toolDefinition.visibilityswitch)) {
        return;
      }
    } catch(e) {}

    
    
    let allDefs = gDevTools.getToolDefinitionArray();
    let prevDef;
    for (let def of allDefs) {
      if (def.id == "options") {
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
  },

  





  _addAllToolsToMenu: function DT_addAllToolsToMenu(doc) {
    let fragCommands = doc.createDocumentFragment();
    let fragKeys = doc.createDocumentFragment();
    let fragBroadcasters = doc.createDocumentFragment();
    let fragAppMenuItems = doc.createDocumentFragment();
    let fragMenuItems = doc.createDocumentFragment();

    for (let toolDefinition of gDevTools.getToolDefinitionArray()) {
      if (toolDefinition.id == "options") {
        continue;
      }

      
      try {
        if (!Services.prefs.getBoolPref(toolDefinition.visibilityswitch)) {
          continue;
        }
      } catch(e) {}

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

  


  _connectToProfiler: function DT_connectToProfiler() {
    for (let win of gDevToolsBrowser._trackedBrowserWindows) {
      if (devtools.TargetFactory.isKnownTab(win.gBrowser.selectedTab)) {
        let target = devtools.TargetFactory.forTab(win.gBrowser.selectedTab);
        if (gDevTools._toolboxes.has(target)) {
          target.makeRemote().then(() => {
            let profiler = new ProfilerController(target);
            profiler.connect();
          }).then(null, Cu.reportError);

          return;
        }
      }
    }
  },

  





  _removeToolFromWindows: function DT_removeToolFromWindows(toolId) {
    for (let win of gDevToolsBrowser._trackedBrowserWindows) {
      gDevToolsBrowser._removeToolFromMenu(toolId, win.document);
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

    let tabContainer = win.document.getElementById("tabbrowser-tabs")
    tabContainer.removeEventListener("TabSelect",
                                     gDevToolsBrowser._updateMenuCheckbox, false);
  },

  


  destroy: function() {
    gDevTools.off("toolbox-ready", gDevToolsBrowser._connectToProfiler);
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
gDevTools.on("toolbox-ready", gDevToolsBrowser._connectToProfiler);
gDevTools.on("toolbox-destroyed", gDevToolsBrowser._updateMenuCheckbox);

Services.obs.addObserver(gDevToolsBrowser.destroy, "quit-application", false);


devtools.main("main");
