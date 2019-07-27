





const { Class } = require("sdk/core/heritage");
const { registerPlugin, Plugin } = require("projecteditor/plugins/core");
const { emit } = require("sdk/event/core");

var DirtyPlugin = Class({
  extends: Plugin,

  onEditorSave: function(editor) { this.onEditorChange(editor); },
  onEditorLoad: function(editor) { this.onEditorChange(editor); },

  onEditorChange: function(editor) {
    
    if (!editor || !editor.editor) {
      return;
    }

    
    let priv = this.priv(editor);
    let clean = editor.isClean()
    if (priv.isClean !== clean) {
      let resource = editor.shell.resource;
      emit(resource, "label-change", resource);
      priv.isClean = clean;
    }
  },

  onAnnotate: function(resource, editor, elt) {
    
    if (!editor || !editor.editor) {
      return;
    }

    if (!editor.isClean()) {
      elt.textContent = '*' + resource.displayName;
      return true;
    }
  }
});
exports.DirtyPlugin = DirtyPlugin;

registerPlugin(DirtyPlugin);
