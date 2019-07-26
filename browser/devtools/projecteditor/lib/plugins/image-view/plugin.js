





const { Cu } = require("chrome");
const { Class } = require("sdk/core/heritage");
const promise = require("projecteditor/helpers/promise");
const { ImageEditor } = require("./image-editor");
const { registerPlugin, Plugin } = require("projecteditor/plugins/core");

var ImageEditorPlugin = Class({
  extends: Plugin,

  editorForResource: function(node) {
    if (node.contentCategory === "image") {
      return ImageEditor;
    }
  },

  init: function(host) {

  }
});

exports.ImageEditorPlugin = ImageEditorPlugin;
registerPlugin(ImageEditorPlugin);
