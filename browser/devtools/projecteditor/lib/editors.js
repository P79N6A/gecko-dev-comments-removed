





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

  toString: function() {
    return this.label || "";
  },

  emit: function(name, ...args) {
    emit(this, name, ...args);
  },

  




  initialize: function(document) {
    this.doc = document;
    this.label = "";
    this.elt = this.doc.createElement("vbox");
    this.elt.setAttribute("flex", "1");
    this.elt.editor = this;
    this.toolbar = this.doc.querySelector("#projecteditor-toolbar");
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

  





  get extraKeys() {
    let extraKeys = {};

    
    
    [...this.doc.querySelectorAll("#projecteditor-keyset key")].forEach((key) => {
      let keyUpper = key.getAttribute("key").toUpperCase();
      let toolModifiers = key.getAttribute("modifiers");
      let modifiers = {
        alt: toolModifiers.contains("alt"),
        shift: toolModifiers.contains("shift")
      };

      
      extraKeys[Editor.accel(keyUpper, modifiers)] = () => {
        let event = this.doc.createEvent('Event');
        event.initEvent('command', true, true);
        let command = this.doc.querySelector("#" + key.getAttribute("command"));
        command.dispatchEvent(event);
      };
    });

    return extraKeys;
  },

  initialize: function(document, mode=Editor.modes.text) {
    ItchEditor.prototype.initialize.apply(this, arguments);
    this.label = mode.name;
    this.editor = new Editor({
      mode: mode,
      lineNumbers: true,
      extraKeys: this.extraKeys,
      themeSwitching: false
    });

    
    this.editor.on("change", (...args) => {
      this.emit("change", ...args);
    });
    this.editor.on("cursorActivity", (...args) => {
      this.emit("cursorActivity", ...args);
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
      this.editor.setText(resourceContents);
      this.editor.setClean();
      this.emit("load");
    }, console.error);
  },

  








  save: function(resource) {
    return resource.save(this.editor.getText()).then(() => {
      this.editor.setClean();
      this.emit("save", resource);
    });
  },

  





  focus: function() {
    return this.appended.then(() => {
      this.editor.focus();
    });
  }
});




function JSEditor(document) {
  return TextEditor(document, Editor.modes.js);
}




function CSSEditor(document) {
  return TextEditor(document, Editor.modes.css);
}




function HTMLEditor(document) {
  return TextEditor(document, Editor.modes.html);
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
