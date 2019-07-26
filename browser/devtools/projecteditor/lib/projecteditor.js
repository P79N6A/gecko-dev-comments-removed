





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

  







  initialize: function(iframe) {
    this._onTreeSelected = this._onTreeSelected.bind(this);
    this._onEditorCreated = this._onEditorCreated.bind(this);
    this._onEditorActivated = this._onEditorActivated.bind(this);
    this._onEditorDeactivated = this._onEditorDeactivated.bind(this);
    this._updateEditorMenuItems = this._updateEditorMenuItems.bind(this);

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
      this._onLoad();
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

    this._buildSidebar();

    this.window.addEventListener("unload", this.destroy.bind(this));

    
    this.shells = new ShellDeck(this, this.document);
    this.shells.on("editor-created", this._onEditorCreated);
    this.shells.on("editor-activated", this._onEditorActivated);
    this.shells.on("editor-deactivated", this._onEditorDeactivated);

    let shellContainer = this.document.querySelector("#shells-deck-container");
    shellContainer.appendChild(this.shells.elt);

    let popup = this.document.querySelector("#edit-menu-popup");
    popup.addEventListener("popupshowing", this.updateEditorMenuItems);

    
    
    this.setProject(new Project({
      id: "",
      name: "",
      directories: [],
      openFiles: []
    }));

    this._initCommands();
    this._initPlugins();
  },


  


  _buildSidebar: function() {
    this.projectTree = new ProjectTreeView(this.document, {
      resourceVisible: this.resourceVisible.bind(this),
      resourceFormatter: this.resourceFormatter.bind(this)
    });
    this.projectTree.on("selection", this._onTreeSelected);

    let sourcesBox = this.document.querySelector("#sources");
    sourcesBox.appendChild(this.projectTree.elt);
  },

  


  _initCommands: function() {
    this.commands = this.document.querySelector("#projecteditor-commandset");
    this.commands.addEventListener("command", (evt) => {
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

  


  _updateEditorMenuItems: function() {
    this.window.goUpdateGlobalEditMenuItems();
    this.window.goUpdateGlobalEditMenuItems();
    let commands = ['cmd_undo', 'cmd_redo', 'cmd_delete', 'cmd_findAgain'];
    commands.forEach(this.window.goUpdateCommand);
  },

  


  destroy: function() {
    this._plugins.forEach(plugin => { plugin.destroy(); });

    this.project.allResources().forEach((resource) => {
      let editor = this.editorFor(resource);
      if (editor) {
        editor.destroy();
      }
    });

    forget(this, this.project);
    this.project.destroy();
    this.project = null;
    this.projectTree.destroy();
    this.projectTree = null;
  },

  





  setProject: function(project) {
    if (this.project) {
      forget(this, this.project);
    }
    this.project = project;
    this.projectTree.setProject(project);

    
    
    on(this, project, "store-removed", (store) => {
      store.allResources().forEach((resource) => {
        let editor = this.editorFor(resource);
        if (editor) {
          editor.destroy();
        }
      });
    });
  },

  










  setProjectToAppPath: function(path, opts = {}) {
    this.project.appManagerOpts = opts;
    this.project.removeAllStores();
    this.project.addPath(path);
    return this.project.refresh();
  },

  





  openResource: function(resource) {
    this.shells.open(resource);
    this.projectTree.selectResource(resource);
  },

  





  _onTreeSelected: function(resource) {
    
    if (resource.isDir && resource.parent) {
      return;
    }
    this.pluginDispatch("onTreeSelected", resource);
    this.openResource(resource);
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

  










  addCommand: function(definition) {
    let command = this.document.createElement("command");
    command.setAttribute("id", definition.id);
    if (definition.key) {
      let key = this.document.createElement("key");
      key.id = "key_" + definition.id;

      let keyName = definition.key;
      if (keyName.startsWith("VK_")) {
        key.setAttribute("keycode", keyName);
      } else {
        key.setAttribute("key", keyName);
      }
      key.setAttribute("modifiers", definition.modifiers);
      key.setAttribute("command", definition.id);
      this.document.getElementById("projecteditor-keyset").appendChild(key);
    }
    command.setAttribute("oncommand", "void(0);"); 
    this.document.getElementById("projecteditor-commandset").appendChild(command);
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
});

exports.ProjectEditor = ProjectEditor;
