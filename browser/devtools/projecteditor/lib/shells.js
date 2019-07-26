





const { Cu } = require("chrome");
const { Class } = require("sdk/core/heritage");
const { EventTarget } = require("sdk/event/target");
const { emit } = require("sdk/event/core");
const { EditorTypeForResource } = require("projecteditor/editors");
const NetworkHelper = require("devtools/toolkit/webconsole/network-helper");










var Shell = Class({
  extends: EventTarget,

  



  initialize: function(host, resource) {
    this.host = host;
    this.doc = host.document;
    this.resource = resource;
    this.elt = this.doc.createElement("vbox");
    this.elt.shell = this;

    let constructor = this._editorTypeForResource();

    this.editor = constructor(this.doc, this.host);
    this.editor.shell = this;
    this.editorAppended = this.editor.appended;

    let loadDefer = promise.defer();
    this.editor.on("load", () => {
      loadDefer.resolve();
    });

    this.editorLoaded = loadDefer.promise;

    this.elt.appendChild(this.editor.elt);
  },

  




  load: function() {
    this.editor.load(this.resource);
  },

  



  _editorTypeForResource: function() {
    let resource = this.resource;
    let constructor = EditorTypeForResource(resource);

    if (this.host.plugins) {
      this.host.plugins.forEach(plugin => {
        if (plugin.editorForResource) {
          let pluginEditor = plugin.editorForResource(resource);
          if (pluginEditor) {
            constructor = pluginEditor;
          }
        }
      });
    }

    return constructor;
  }
});










var ShellDeck = Class({
  extends: EventTarget,

  



  initialize: function(host, document) {
    this.doc = document;
    this.host = host;
    this.deck = this.doc.createElement("deck");
    this.deck.setAttribute("flex", "1");
    this.elt = this.deck;

    this.shells = new Map();

    this._activeShell = null;
  },

  







  open: function(defaultResource) {
    let shell = this.shellFor(defaultResource);
    if (!shell) {
      shell = this._createShell(defaultResource);
      this.shells.set(defaultResource, shell);
    }
    this.selectShell(shell);
    return shell;
  },

  




  _createShell: function(defaultResource) {
    let shell = Shell(this.host, defaultResource);

    shell.editorAppended.then(() => {
      this.shells.set(shell.resource, shell);
      emit(this, "editor-created", shell.editor);
      if (this.currentShell === shell) {
        this.selectShell(shell);
      }

    });

    shell.load();
    this.deck.appendChild(shell.elt);
    return shell;
  },

  






  selectShell: function(shell) {
    
    if (this._activeShell != shell) {
      if (this._activeShell) {
        emit(this, "editor-deactivated", this._activeShell.editor, this._activeShell.resource);
      }
      this.deck.selectedPanel = shell.elt;
      this._activeShell = shell;
      shell.editorLoaded.then(() => {
        
        
        if (this._activeShell === shell) {
          emit(this, "editor-activated", shell.editor, shell.resource);
        }
      });
    }
  },

  





  shellFor: function(resource) {
    return this.shells.get(resource);
  },

  






  get currentShell() {
    return this._activeShell;
  },

  




  get currentEditor() {
    let shell = this.currentShell;
    return shell ? shell.editor : null;
  },

});
exports.ShellDeck = ShellDeck;
