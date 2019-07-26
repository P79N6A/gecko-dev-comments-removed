



"use strict";

this.EXPORTED_SYMBOLS = [ "gDevTools", "DevTools", "DevToolsXULCommands" ];

const Cu = Components.utils;
const Ci = Components.interfaces;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource:///modules/devtools/EventEmitter.jsm");
Cu.import("resource:///modules/devtools/ToolDefinitions.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Toolbox",
  "resource:///modules/devtools/Toolbox.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "TargetFactory",
  "resource:///modules/devtools/Target.jsm");

const FORBIDDEN_IDS = new Set("toolbox", "");





this.DevTools = function DevTools() {
  this._tools = new Map();
  this._toolboxes = new Map();

  
  this.destroy = this.destroy.bind(this);

  this._trackedBrowserWindows = new Set();

  
  this._updateMenuCheckbox = this._updateMenuCheckbox.bind(this);

  new EventEmitter(this);

  Services.obs.addObserver(this.destroy, "quit-application", false);

  


  for (let definition of defaultTools) {
    this.registerTool(definition);
  }
}

DevTools.prototype = {
  






















  registerTool: function DT_registerTool(toolDefinition) {
    let toolId = toolDefinition.id;

    if (!toolId || FORBIDDEN_IDS.has(toolId)) {
      throw new Error("Invalid definition.id");
    }

    toolDefinition.killswitch = toolDefinition.killswitch ||
      "devtools." + toolId + ".enabled";
    this._tools.set(toolId, toolDefinition);

    this._addToolToWindows(toolDefinition);

    this.emit("tool-registered", toolId);
  },

  






  unregisterTool: function DT_unregisterTool(toolId) {
    this._tools.delete(toolId);

    this._removeToolFromWindows(toolId);

    this.emit("tool-unregistered", toolId);
  },

  






  getToolDefinitions: function DT_getToolDefinitions() {
    let tools = new Map();

    for (let [key, value] of this._tools) {
      let enabled;

      try {
        enabled = Services.prefs.getBoolPref(value.killswitch);
      } catch(e) {
        enabled = true;
      }

      if (enabled) {
        tools.set(key, value);
      }
    }
    return tools;
  },

  













  openToolbox: function DT_openToolbox(target, hostType, defaultToolId) {
    if (this._toolboxes.has(target)) {
      
      return this._toolboxes.get(target);
    }

    let tb = new Toolbox(target, hostType, defaultToolId);

    this._toolboxes.set(target, tb);
    tb.once("destroyed", function() {
      this._toolboxes.delete(target);
      this._updateMenuCheckbox();
      this.emit("toolbox-destroyed", target);
    }.bind(this));

    tb.once("ready", function() {
      this.emit("toolbox-ready", tb);
      this._updateMenuCheckbox();
    }.bind(this));

    tb.open();

    return tb;
  },

  


  closeToolbox: function DT_closeToolbox(target) {
    let toolbox = this._toolboxes.get(target);
    if (toolbox == null) {
      return;
    }
    toolbox.destroy();
  },

  











  openToolboxForTab: function DT_openToolboxForTab(target, toolId) {
    let tb = this.getToolboxForTarget(target);

    if (tb) {
      tb.selectTool(toolId);
    } else {
      tb = this.openToolbox(target, null, toolId);
    }
    return tb;
  },

  




  toggleToolboxCommand: function(gBrowser, toolId=null) {
    let target = TargetFactory.forTab(gBrowser.selectedTab);
    this.toggleToolboxForTarget(target, toolId);
  },

  







  toggleToolboxForTarget: function DT_toggleToolboxForTarget(target, toolId) {
    let tb = this.getToolboxForTarget(target);

    if (tb  ) {
      tb.destroy();
    } else {
      this.openToolboxForTab(target, toolId);
    }
  },

  








  getToolboxForTarget: function DT_getToolboxForTarget(target) {
    return this._toolboxes.get(target);
  },

  










  getPanelForTarget: function DT_getPanelForTarget(toolId, target) {
    let toolbox = this.getToolboxForTarget(target);
    if (!toolbox) {
      return undefined;
    }
    return toolbox.getToolPanels().get(toolId);
  },

  





  registerBrowserWindow: function DT_registerBrowserWindow(win) {
    this._trackedBrowserWindows.add(win);
    this._addAllToolsToMenu(win.document);

    let tabContainer = win.document.getElementById("tabbrowser-tabs")
    tabContainer.addEventListener("TabSelect", this._updateMenuCheckbox, false);
  },

  





  _addToolToWindows: function DT_addToolToWindows(toolDefinition) {
    for (let win of this._trackedBrowserWindows) {
      this._addToolToMenu(toolDefinition, win.document);
    }
  },

  





  _addAllToolsToMenu: function DT_addAllToolsToMenu(doc) {
    let fragCommands = doc.createDocumentFragment();
    let fragKeys = doc.createDocumentFragment();
    let fragBroadcasters = doc.createDocumentFragment();
    let fragAppMenuItems = doc.createDocumentFragment();
    let fragMenuItems = doc.createDocumentFragment();

    for (let [key, toolDefinition] of this._tools) {
      let frags = this._addToolToMenu(toolDefinition, doc, true);

      if (!frags) {
        return;
      }

      let [cmd, key, bc, appmenuitem, menuitem] = frags;

      fragCommands.appendChild(cmd);
      if (key) {
        fragKeys.appendChild(key);
      }
      fragBroadcasters.appendChild(bc);
      fragAppMenuItems.appendChild(appmenuitem);
      fragMenuItems.appendChild(menuitem);
    }

    let mcs = doc.getElementById("mainCommandSet");
    mcs.appendChild(fragCommands);

    let mks = doc.getElementById("mainKeyset");
    mks.appendChild(fragKeys);

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

  










  _addToolToMenu: function DT_addToolToMenu(toolDefinition, doc, noAppend) {
    let id = toolDefinition.id;

    
    if (doc.getElementById("Tools:" + id)) {
      return;
    }

    let cmd = doc.createElement("command");
    cmd.id = "Tools:" + id;
    cmd.setAttribute("oncommand",
        'gDevTools.toggleToolboxCommand(gBrowser, "' + id + '");');

    let key = null;
    if (toolDefinition.key) {
      key = doc.createElement("key");
      key.id = "key_" + id;

      if (toolDefinition.key.startsWith("VK_")) {
        key.setAttribute("keycode", toolDefinition.key);
      } else {
        key.setAttribute("key", toolDefinition.key);
      }

      key.setAttribute("oncommand",
          'gDevTools.toggleToolboxCommand(gBrowser, "' + id + '");');
      key.setAttribute("modifiers", toolDefinition.modifiers);
    }

    let bc = doc.createElement("broadcaster");
    bc.id = "devtoolsMenuBroadcaster_" + id;
    bc.setAttribute("label", toolDefinition.label);
    bc.setAttribute("command", "Tools:" + id);

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

    if (noAppend) {
      return [cmd, key, bc, appmenuitem, menuitem];
    } else {
      let mcs = doc.getElementById("mainCommandSet");
      mcs.appendChild(cmd);

      if (key) {
        let mks = doc.getElementById("mainKeyset");
        mks.appendChild(key);
      }

      let mbs = doc.getElementById("mainBroadcasterSet");
      mbs.appendChild(bc);

      let amp = doc.getElementById("appmenu_webDeveloper_popup");
      if (amp) {
        let amps = doc.getElementById("appmenu_devtools_separator");
        amp.insertBefore(appmenuitem, amps);
      }

      let mp = doc.getElementById("menuWebDeveloperPopup");
      let mps = doc.getElementById("menu_devtools_separator");
      mp.insertBefore(menuitem, mps);
    }
  },

  



  _updateMenuCheckbox: function DT_updateMenuCheckbox() {
    for (let win of this._trackedBrowserWindows) {

      let hasToolbox = false;
      if (TargetFactory.isKnownTab(win.gBrowser.selectedTab)) {
        let target = TargetFactory.forTab(win.gBrowser.selectedTab);
        if (this._toolboxes.has(target)) {
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
    for (let win of this._trackedBrowserWindows) {
      this._removeToolFromMenu(toolId, win.document);
    }
  },

  







  _removeToolFromMenu: function DT_removeToolFromMenu(toolId, doc) {
    let command = doc.getElementById("Tools:" + toolId);
    command.parentNode.removeChild(command);

    let key = doc.getElementById("key_" + toolId);
    if (key) {
      key.parentNode.removeChild(key);
    }

    let bc = doc.getElementById("devtoolsMenuBroadcaster_" + toolId);
    bc.parentNode.removeChild(bc);

    





  },

  






  forgetBrowserWindow: function DT_forgetBrowserWindow(win) {
    if (!this._tools) {
      return;
    }

    this._trackedBrowserWindows.delete(win);

    
    for (let [target, toolbox] of this._toolboxes) {
      if (toolbox.frame.ownerDocument.defaultView == win) {
        toolbox.destroy();
      }
    }

    let tabContainer = win.document.getElementById("tabbrowser-tabs")
    tabContainer.removeEventListener("TabSelect",
                                     this._updateMenuCheckbox, false);
  },

  


  destroy: function() {
    Services.obs.removeObserver(this.destroy, "quit-application");

    delete this._trackedBrowserWindows;
    delete this._tools;
    delete this._toolboxes;
  },
};







this.gDevTools = new DevTools();




this.DevToolsXULCommands = {
  openConnectScreen: function(gBrowser) {
    gBrowser.selectedTab = gBrowser.addTab("chrome://browser/content/devtools/connect.xhtml");
  },
}
