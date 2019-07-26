





var { Class } = require("sdk/core/heritage");
var { registerPlugin, Plugin } = require("projecteditor/plugins/core");

var LoggingPlugin = Class({
  extends: Plugin,

  
  onEditorCreated: function(editor) { console.log("editor created: " + editor) },
  onEditorDestroyed: function(editor) { console.log("editor destroyed: " + editor )},

  onEditorSave: function(editor) { console.log("editor saved: " + editor) },
  onEditorLoad: function(editor) { console.log("editor loaded: " + editor) },

  onEditorActivated: function(editor) { console.log("editor activated: " + editor )},
  onEditorDeactivated: function(editor) { console.log("editor deactivated: " + editor )},

  onEditorChange: function(editor) { console.log("editor changed: " + editor )},

  onCommand: function(cmd) { console.log("Command: " + cmd); }
});
exports.LoggingPlugin = LoggingPlugin;

registerPlugin(LoggingPlugin);
