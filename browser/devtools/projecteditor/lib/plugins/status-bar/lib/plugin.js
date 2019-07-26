





const { Cu } = require("chrome");
const { Class } = require("sdk/core/heritage");
const promise = require("projecteditor/helpers/promise");
const { registerPlugin, Plugin } = require("projecteditor/plugins/core");





var StatusBarPlugin = Class({
  extends: Plugin,

  init: function() {
    this.box = this.host.createElement("hbox", {
      parent: "#projecteditor-toolbar-bottom"
    });

    this.activeMode = this.host.createElement("label", {
      parent: this.box,
      class: "projecteditor-basic-display"
    });

    this.cursorPosition = this.host.createElement("label", {
      parent: this.box,
      class: "projecteditor-basic-display"
    });

    this.fileLabel = this.host.createElement("label", {
      parent: "#plugin-toolbar-left",
      class: "projecteditor-file-label"
    });
  },

  destroy: function() {
  },

  




  render: function(editor, resource) {
    if (!resource || resource.isDir) {
      this.fileLabel.textContent = "";
      this.cursorPosition.value = "";
      return;
    }

    this.fileLabel.textContent = resource.basename;
    this.activeMode.value = editor.toString();
    if (editor.editor) {
      let cursorStart = editor.editor.getCursor("start");
      let cursorEnd = editor.editor.getCursor("end");
      if (cursorStart.line === cursorEnd.line && cursorStart.ch === cursorEnd.ch) {
        this.cursorPosition.value = cursorStart.line + " " + cursorStart.ch;
      } else {
        this.cursorPosition.value = cursorStart.line + " " + cursorStart.ch + " | " +
                                    cursorEnd.line + " " + cursorEnd.ch;
      }
    } else {
      this.cursorPosition.value = "";
    }
  },


  




  onTreeSelected: function(resource) {
    if (!resource || resource.isDir) {
      this.fileLabel.textContent = "";
      return;
    }
    this.fileLabel.textContent = resource.basename;
  },

  onEditorDeactivated: function(editor) {
    this.fileLabel.textContent = "";
    this.cursorPosition.value = "";
  },

  onEditorChange: function(editor, resource) {
    this.render(editor, resource);
  },

  onEditorCursorActivity: function(editor, resource) {
    this.render(editor, resource);
  },

  onEditorActivated: function(editor, resource) {
    this.render(editor, resource);
  },

});

exports.StatusBarPlugin = StatusBarPlugin;
registerPlugin(StatusBarPlugin);
