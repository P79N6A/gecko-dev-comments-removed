



"use strict";

this.EXPORTED_SYMBOLS = [ "gDevTools", "DevTools", "gDevToolsBrowser" ];

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/commonjs/promise/core.js");
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

  EventEmitter.decorate(this);

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

    this.emit("tool-registered", toolId);
  },

  






  unregisterTool: function DT_unregisterTool(toolId) {
    this._tools.delete(toolId);

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
      
      toolbox = new Toolbox(target, toolId, hostType);

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

  


  destroy: function() {
    Services.obs.removeObserver(this.destroy, "quit-application");

    delete this._trackedBrowserWindows;
    delete this._toolboxes;
  },
};







let gDevTools = new DevTools();
this.gDevTools = gDevTools;





let gDevToolsBrowser = {
  



  _trackedBrowserWindows: new Set(),

  




  toggleToolboxCommand: function(gBrowser) {
    let target = TargetFactory.forTab(gBrowser.selectedTab);
    let toolbox = gDevTools.getToolbox(target);

    toolbox ? toolbox.destroy() : gDevTools.showToolbox(target);
  },

  













  selectToolCommand: function(gBrowser, toolId) {
    let target = TargetFactory.forTab(gBrowser.selectedTab);
    let toolbox = gDevTools.getToolbox(target);

    if (toolbox && toolbox.currentToolId == toolId) {
      if (toolbox.hostType == Toolbox.HostType.WINDOW) {
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

  





  _addToolToWindows: function DT_addToolToWindows(toolDefinition) {
    for (let win of gDevToolsBrowser._trackedBrowserWindows) {
      gDevToolsBrowser._addToolToMenu(toolDefinition, win.document);
    }
  },

  





  _addAllToolsToMenu: function DT_addAllToolsToMenu(doc) {
    let fragCommands = doc.createDocumentFragment();
    let fragKeys = doc.createDocumentFragment();
    let fragBroadcasters = doc.createDocumentFragment();
    let fragAppMenuItems = doc.createDocumentFragment();
    let fragMenuItems = doc.createDocumentFragment();

    for (let [key, toolDefinition] of gDevTools._tools) {
      let frags = gDevToolsBrowser._addToolToMenu(toolDefinition, doc, true);

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
    bc.setAttribute("label", toolDefinition.label);
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
    for (let win of gDevToolsBrowser._trackedBrowserWindows) {

      let hasToolbox = false;
      if (TargetFactory.isKnownTab(win.gBrowser.selectedTab)) {
        let target = TargetFactory.forTab(win.gBrowser.selectedTab);
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
    if (!gDevToolsBrowser._trackedBrowserWindows) {
      return;
    }

    gDevToolsBrowser._trackedBrowserWindows.delete(win);

    
    for (let [target, toolbox] of gDevTools._toolboxes) {
      if (toolbox.frame.ownerDocument.defaultView == win) {
        toolbox.destroy();
      }
    }

    let tabContainer = win.document.getElementById("tabbrowser-tabs")
    tabContainer.removeEventListener("TabSelect",
                                     gDevToolsBrowser._updateMenuCheckbox, false);
  },

  


  destroy: function() {
    Services.obs.removeObserver(gDevToolsBrowser.destroy, "quit-application");
    delete gDevToolsBrowser._trackedBrowserWindows;
  },
}
this.gDevToolsBrowser = gDevToolsBrowser;

gDevTools.on("tool-registered", function(ev, toolId) {
  let toolDefinition = gDevTools._tools.get(toolId);
  gDevToolsBrowser._addToolToWindows(toolDefinition);
});

gDevTools.on("tool-unregistered", function(ev, toolId) {
  gDevToolsBrowser._removeToolFromWindows(toolId);
});

gDevTools.on("toolbox-ready", gDevToolsBrowser._updateMenuCheckbox);
gDevTools.on("toolbox-destroyed", gDevToolsBrowser._updateMenuCheckbox);

Services.obs.addObserver(gDevToolsBrowser.destroy, "quit-application", false);
