





const { Cc, Ci, Cu } = require("chrome");
const { Class } = require("sdk/core/heritage");
const { Project } = require("projecteditor/project");
const { ProjectTreeView } = require("projecteditor/tree");
const { ShellDeck } = require("projecteditor/shells");
const { Resource } = require("projecteditor/stores/resource");
const { registeredPlugins } = require("projecteditor/plugins/core");
const { EventTarget } = require("sdk/event/target");
const { on, forget } = require("projecteditor/helpers/event");
const { emit } = require("sdk/event/core");
const { merge } = require("sdk/util/object");
const promise = require("projecteditor/helpers/promise");
const { ViewHelpers } = Cu.import("resource:///modules/devtools/ViewHelpers.jsm", {});
const { DOMHelpers } = Cu.import("resource:///modules/devtools/DOMHelpers.jsm");
const { Services } = Cu.import("resource://gre/modules/Services.jsm", {});
const ITCHPAD_URL = "chrome://browser/content/devtools/projecteditor.xul";


require("projecteditor/plugins/dirty/dirty");
require("projecteditor/plugins/delete/delete");
require("projecteditor/plugins/new/new");
require("projecteditor/plugins/save/save");
require("projecteditor/plugins/image-view/plugin");
require("projecteditor/plugins/app-manager/plugin");
require("projecteditor/plugins/status-bar/plugin");






































var ProjectEditor = Class({
  extends: EventTarget,

  










  initialize: function(iframe, options = {}) {
    this._onTreeSelected = this._onTreeSelected.bind(this);
    this._onTreeResourceRemoved = this._onTreeResourceRemoved.bind(this);
    this._onEditorCreated = this._onEditorCreated.bind(this);
    this._onEditorActivated = this._onEditorActivated.bind(this);
    this._onEditorDeactivated = this._onEditorDeactivated.bind(this);
    this._updateMenuItems = this._updateMenuItems.bind(this);
    this._updateContextMenuItems = this._updateContextMenuItems.bind(this);
    this.destroy = this.destroy.bind(this);
    this.menubar = options.menubar || null;
    this.menuindex = options.menuindex || null;
    this._menuEnabled = true;
    this._destroyed = false;
    this._loaded = false;
    this._pluginCommands = new Map();
    if (iframe) {
      this.load(iframe);
    }
  },

  










  load: function(iframe) {
    if (this.loaded) {
      return this.loaded;
    }

    let deferred = promise.defer();
    this.loaded = deferred.promise;
    this.iframe = iframe;

    let domReady = () => {
      if (this._destroyed) {
        deferred.reject("Error: ProjectEditor has been destroyed before loading");
        return;
      }
      this._onLoad();
      this._loaded = true;
      deferred.resolve(this);
    };

    let domHelper = new DOMHelpers(this.iframe.contentWindow);
    domHelper.onceDOMReady(domReady);

    this.iframe.setAttribute("src", ITCHPAD_URL);

    return this.loaded;
  },

  


  _onLoad: function() {
    this.document = this.iframe.contentDocument;
    this.window = this.iframe.contentWindow;

    this._initCommands();
    this._buildMenubar();
    this._buildSidebar();

    this.window.addEventListener("unload", this.destroy, false);

    
    this.shells = new ShellDeck(this, this.document);
    this.shells.on("editor-created", this._onEditorCreated);
    this.shells.on("editor-activated", this._onEditorActivated);
    this.shells.on("editor-deactivated", this._onEditorDeactivated);

    let shellContainer = this.document.querySelector("#shells-deck-container");
    shellContainer.appendChild(this.shells.elt);

    
    
    this.setProject(new Project({
      id: "",
      name: "",
      directories: [],
      openFiles: []
    }));

    this._initPlugins();
  },

  _buildMenubar: function() {

    this.contextMenuPopup = this.document.getElementById("context-menu-popup");
    this.contextMenuPopup.addEventListener("popupshowing", this._updateContextMenuItems);

    this.textEditorContextMenuPopup = this.document.getElementById("texteditor-context-popup");
    this.textEditorContextMenuPopup.addEventListener("popupshowing", this._updateMenuItems);

    this.editMenu = this.document.getElementById("edit-menu");
    this.fileMenu = this.document.getElementById("file-menu");

    this.editMenuPopup = this.document.getElementById("edit-menu-popup");
    this.fileMenuPopup = this.document.getElementById("file-menu-popup");
    this.editMenu.addEventListener("popupshowing", this._updateMenuItems);
    this.fileMenu.addEventListener("popupshowing", this._updateMenuItems);

    if (this.menubar) {
      let body = this.menubar.ownerDocument.body ||
                 this.menubar.ownerDocument.querySelector("window");
      body.appendChild(this.projectEditorCommandset);
      body.appendChild(this.projectEditorKeyset);
      body.appendChild(this.editorCommandset);
      body.appendChild(this.editorKeyset);
      body.appendChild(this.contextMenuPopup);
      body.appendChild(this.textEditorContextMenuPopup);

      let index = this.menuindex || 0;
      this.menubar.insertBefore(this.editMenu, this.menubar.children[index]);
      this.menubar.insertBefore(this.fileMenu, this.menubar.children[index]);
    } else {
      this.document.getElementById("projecteditor-menubar").style.display = "block";
    }

    
    this._commandWindow = this.editorCommandset.ownerDocument.defaultView;
    this._commandController = getCommandController(this);
    this._commandWindow.controllers.insertControllerAt(0, this._commandController);
  },

  


  _buildSidebar: function() {
    this.projectTree = new ProjectTreeView(this.document, {
      resourceVisible: this.resourceVisible.bind(this),
      resourceFormatter: this.resourceFormatter.bind(this),
      contextMenuPopup: this.contextMenuPopup
    });
    on(this, this.projectTree, "selection", this._onTreeSelected);
    on(this, this.projectTree, "resource-removed", this._onTreeResourceRemoved);

    let sourcesBox = this.document.querySelector("#sources > vbox");
    sourcesBox.appendChild(this.projectTree.elt);
  },

  


  _initCommands: function() {

    this.projectEditorCommandset = this.document.getElementById("projecteditor-commandset");
    this.projectEditorKeyset = this.document.getElementById("projecteditor-keyset");

    this.editorCommandset = this.document.getElementById("editMenuCommands");
    this.editorKeyset = this.document.getElementById("editMenuKeys");

    this.projectEditorCommandset.addEventListener("command", (evt) => {
      evt.stopPropagation();
      evt.preventDefault();
      this.pluginDispatch("onCommand", evt.target.id, evt.target);
    });
  },

  


  _initPlugins: function() {
    this._plugins = [];

    for (let plugin of registeredPlugins) {
      try {
        this._plugins.push(plugin(this));
      } catch(ex) {
        console.exception(ex);
      }
    }

    this.pluginDispatch("lateInit");
  },

  


  _updateMenuItems: function() {
    let window = this.editMenu.ownerDocument.defaultView;
    let commands = ['cmd_undo', 'cmd_redo', 'cmd_delete', 'cmd_cut', 'cmd_copy', 'cmd_paste'];
    commands.forEach(window.goUpdateCommand);

    for (let c of this._pluginCommands.keys()) {
      window.goUpdateCommand(c);
    }
  },

  



  _updateContextMenuItems: function() {
    let resource = this.projectTree.getSelectedResource();
    this.pluginDispatch("onContextMenuOpen", resource);
  },

  


  destroy: function() {
    this._destroyed = true;


    
    
    if (!this._loaded) {
      this.iframe.setAttribute("src", "about:blank");
      return;
    }

    
    
    this.window.removeEventListener("unload", this.destroy, false);
    this.iframe.setAttribute("src", "about:blank");

    this._plugins.forEach(plugin => { plugin.destroy(); });

    forget(this, this.projectTree);
    this.projectTree.destroy();
    this.projectTree = null;

    this.shells.destroy();

    this.projectEditorCommandset.remove();
    this.projectEditorKeyset.remove();
    this.editorCommandset.remove();
    this.editorKeyset.remove();
    this.contextMenuPopup.remove();
    this.textEditorContextMenuPopup.remove();
    this.editMenu.remove();
    this.fileMenu.remove();

    this._commandWindow.controllers.removeController(this._commandController);
    this._commandController = null;

    forget(this, this.project);
    this.project.destroy();
    this.project = null;
  },

  





  setProject: function(project) {
    if (this.project) {
      forget(this, this.project);
    }
    this.project = project;
    this.projectTree.setProject(project);

    
    
    on(this, project, "store-removed", (store) => {
      store.allResources().forEach((resource) => {
        this.shells.removeResource(resource);
      });
    });
  },

  















  setProjectToAppPath: function(path, opts = {}) {
    this.project.appManagerOpts = opts;

    let existingPaths = this.project.allPaths();
    if (existingPaths.length !== 1 || existingPaths[0] !== path) {
      
      this.project.removeAllStores();
      this.project.addPath(path);
    } else {
      
      let rootResource = this.project.localStores.get(path).root;
      emit(rootResource, "label-change", rootResource);
    }

    return this.project.refresh();
  },

  





  openResource: function(resource) {
    let shell = this.shells.open(resource);
    this.projectTree.selectResource(resource);
    shell.editor.focus();
  },

  





  _onTreeSelected: function(resource) {
    
    if (resource.isDir && resource.parent) {
      return;
    }
    this.pluginDispatch("onTreeSelected", resource);
    this.openResource(resource);
  },

  





  _onTreeResourceRemoved: function(resource) {
    this.shells.removeResource(resource);
  },

  











  createElement: function(type, options) {
    let elt = this.document.createElement(type);

    let parent;

    for (let opt in options) {
      if (opt === "command") {
        let command = typeof(options.command) === "string" ? options.command : options.command.id;
        elt.setAttribute("command", command);
      } else if (opt === "parent") {
        continue;
      } else {
        elt.setAttribute(opt, options[opt]);
      }
    }

    if (options.parent) {
      let parent = options.parent;
      if (typeof(parent) === "string") {
        parent = this.document.querySelector(parent);
      }
      parent.appendChild(elt);
    }

    return elt;
  },

  







  createMenuItem: function(options) {
    return this.createElement("menuitem", options);
  },

  










  addCommand: function(plugin, definition) {
    this._pluginCommands.set(definition.id, plugin);
    let document = this.projectEditorKeyset.ownerDocument;
    let command = document.createElement("command");
    command.setAttribute("id", definition.id);
    if (definition.key) {
      let key = document.createElement("key");
      key.id = "key_" + definition.id;

      let keyName = definition.key;
      if (keyName.startsWith("VK_")) {
        key.setAttribute("keycode", keyName);
      } else {
        key.setAttribute("key", keyName);
      }
      key.setAttribute("modifiers", definition.modifiers);
      key.setAttribute("command", definition.id);
      this.projectEditorKeyset.appendChild(key);
    }
    command.setAttribute("oncommand", "void(0);"); 
    this.projectEditorCommandset.appendChild(command);
    return command;
  },

  







  getPlugin: function(pluginType) {
    for (let plugin of this.plugins) {
      if (plugin.constructor === pluginType) {
        return plugin;
      }
    }
    return null;
  },

  




  get plugins() {
    if (!this._plugins) {
      console.log("plugins requested before _plugins was set");
      return [];
    }
    
    
    return this._plugins;
  },

  






  _onEditorCreated: function(editor) {
    this.pluginDispatch("onEditorCreated", editor);
    this._editorListenAndDispatch(editor, "change", "onEditorChange");
    this._editorListenAndDispatch(editor, "cursorActivity", "onEditorCursorActivity");
    this._editorListenAndDispatch(editor, "load", "onEditorLoad");
    this._editorListenAndDispatch(editor, "save", "onEditorSave");

    editor.on("focus", () => {
      this.projectTree.selectResource(this.resourceFor(editor));
    });
  },

  








  _onEditorActivated: function(editor, resource) {
    editor.setToolbarVisibility();
    this.pluginDispatch("onEditorActivated", editor, resource);
  },

  







  _onEditorDeactivated: function(editor, resource) {
    this.pluginDispatch("onEditorDeactivated", editor, resource);
  },

  








  pluginDispatch: function(handler, ...args) {
    emit(this, handler, ...args);
    this.plugins.forEach(plugin => {
      try {
        if (handler in plugin) plugin[handler](...args);
      } catch(ex) {
        console.error(ex);
      }
    })
  },

  










  _editorListenAndDispatch: function(editor, event, handler) {
    editor.on(event, (...args) => {
      this.pluginDispatch(handler, editor, this.resourceFor(editor), ...args);
    });
  },

  






  shellFor: function(resource) {
    return this.shells.shellFor(resource);
  },

  







  editorFor: function(resource) {
    let shell = this.shellFor(resource);
    return shell ? shell.editor : shell;
  },

  







  resourceFor: function(editor) {
    if (editor && editor.shell && editor.shell.resource) {
      return editor.shell.resource;
    }
    return null;
  },

  







  resourceVisible: function(resource) {
    return true;
  },

  







  resourceFormatter: function(resource, elt) {
    let editor = this.editorFor(resource);
    let renderedByPlugin = false;

    
    this.plugins.forEach(plugin => {
      if (!plugin.onAnnotate) {
        return;
      }
      if (plugin.onAnnotate(resource, editor, elt)) {
        renderedByPlugin = true;
      }
    });

    
    if (!renderedByPlugin) {
      elt.textContent = resource.displayName;
    }
  },

  get sourcesVisible() {
    return this.sourceToggle.hasAttribute("pane-collapsed");
  },

  get currentShell() {
    return this.shells.currentShell;
  },

  get currentEditor() {
    return this.shells.currentEditor;
  },

  






  set menuEnabled(val) {
    this._menuEnabled = val;
    if (this._loaded) {
      this._updateMenuItems();
    }
  },

  get menuEnabled() {
    return this._menuEnabled;
  }
});







function getCommandController(host) {
  return {
    supportsCommand: function (cmd) {
      return host._pluginCommands.get(cmd);
    },

    isCommandEnabled: function (cmd) {
      if (!host.menuEnabled) {
        return false;
      }
      let plugin = host._pluginCommands.get(cmd);
      if (plugin && plugin.isCommandEnabled) {
        return plugin.isCommandEnabled(cmd);
      }
      return true;
    },
    doCommand: function(cmd) {
    }
  };
}

exports.ProjectEditor = ProjectEditor;
