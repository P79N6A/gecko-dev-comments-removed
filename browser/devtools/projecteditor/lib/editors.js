





const { Cu } = require("chrome");
const { Class } = require("sdk/core/heritage");
const { EventTarget } = require("sdk/event/target");
const { emit } = require("sdk/event/core");
const promise = require("projecteditor/helpers/promise");
const Editor  = require("devtools/sourceeditor/editor");
const HTML_NS = "http://www.w3.org/1999/xhtml";
const XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";






var ItchEditor = Class({
  extends: EventTarget,

  


  hidesToolbar: false,

  



  isEditable: false,

  toString: function() {
    return this.label || "";
  },

  emit: function(name, ...args) {
    emit(this, name, ...args);
  },

  
  isClean: function() {
    return true;
  },

  




  initialize: function(host) {
    this.host = host;
    this.doc = host.document;
    this.label = "";
    this.elt = this.doc.createElement("vbox");
    this.elt.setAttribute("flex", "1");
    this.elt.editor = this;
    this.toolbar = this.doc.querySelector("#projecteditor-toolbar");
    this.projectEditorKeyset = host.projectEditorKeyset;
    this.projectEditorCommandset = host.projectEditorCommandset;
  },

  



  setToolbarVisibility: function() {
    if (this.hidesToolbar) {
      this.toolbar.setAttribute("hidden", "true");
    } else {
      this.toolbar.removeAttribute("hidden");
    }
  },


  








  load: function(resource) {
    return promise.resolve();
  },

  



  destroy: function() {

  },

  






  focus: function() {
    return promise.resolve();
  }
});
exports.ItchEditor = ItchEditor;






var TextEditor = Class({
  extends: ItchEditor,

  isEditable: true,

  





  get extraKeys() {
    let extraKeys = {};

    
    
    [...this.projectEditorKeyset.querySelectorAll("key")].forEach((key) => {
      let keyUpper = key.getAttribute("key").toUpperCase();
      let toolModifiers = key.getAttribute("modifiers");
      let modifiers = {
        alt: toolModifiers.includes("alt"),
        shift: toolModifiers.includes("shift")
      };

      
      extraKeys[Editor.accel(keyUpper, modifiers)] = () => {
        let doc = this.projectEditorCommandset.ownerDocument;
        let event = doc.createEvent('Event');
        event.initEvent('command', true, true);
        let command = this.projectEditorCommandset.querySelector("#" + key.getAttribute("command"));
        command.dispatchEvent(event);
      };
    });

    return extraKeys;
  },

  isClean: function() {
    if (!this.editor.isAppended()) {
      return true;
    }
    return this.editor.getText() === this._savedResourceContents;
  },

  initialize: function(document, mode=Editor.modes.text) {
    ItchEditor.prototype.initialize.apply(this, arguments);
    this.label = mode.name;
    this.editor = new Editor({
      mode: mode,
      lineNumbers: true,
      extraKeys: this.extraKeys,
      themeSwitching: false,
      autocomplete: true,
      contextMenu:  this.host.textEditorContextMenuPopup
    });

    
    this.editor.on("change", (...args) => {
      this.emit("change", ...args);
    });
    this.editor.on("cursorActivity", (...args) => {
      this.emit("cursorActivity", ...args);
    });
    this.editor.on("focus", (...args) => {
      this.emit("focus", ...args);
    });

    this.appended = this.editor.appendTo(this.elt);
  },

  



  destroy: function() {
    this.editor.destroy();
    this.editor = null;
  },

  








  load: function(resource) {
    
    
    return promise.all([
      resource.load(),
      this.appended
    ]).then(([resourceContents])=> {
      if (!this.editor) {
        return;
      }
      this._savedResourceContents = resourceContents;
      this.editor.setText(resourceContents);
      this.editor.clearHistory();
      this.editor.setClean();
      this.emit("load");
    }, console.error);
  },

  








  save: function(resource) {
    let newText = this.editor.getText();
    return resource.save(newText).then(() => {
      this._savedResourceContents = newText;
      this.emit("save", resource);
    });
  },

  





  focus: function() {
    return this.appended.then(() => {
      if (this.editor) {
        this.editor.focus();
      }
    });
  }
});




function JSEditor(host) {
  return TextEditor(host, Editor.modes.js);
}




function CSSEditor(host) {
  return TextEditor(host, Editor.modes.css);
}




function HTMLEditor(host) {
  return TextEditor(host, Editor.modes.html);
}









function EditorTypeForResource(resource) {
  const categoryMap = {
    "txt": TextEditor,
    "html": HTMLEditor,
    "xml": HTMLEditor,
    "css": CSSEditor,
    "js": JSEditor,
    "json": JSEditor
  };
  return categoryMap[resource.contentCategory] || TextEditor;
}

exports.TextEditor = TextEditor;
exports.JSEditor = JSEditor;
exports.CSSEditor = CSSEditor;
exports.HTMLEditor = HTMLEditor;
exports.EditorTypeForResource = EditorTypeForResource;
