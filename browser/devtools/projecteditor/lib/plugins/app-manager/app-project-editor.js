





const { Cu } = require("chrome");
const { Class } = require("sdk/core/heritage");
const promise = require("projecteditor/helpers/promise");
const { ItchEditor } = require("projecteditor/editors");

var AppProjectEditor = Class({
  extends: ItchEditor,

  hidesToolbar: true,

  initialize: function(document, host) {
    ItchEditor.prototype.initialize.apply(this, arguments);
    this.appended = promise.resolve();
    this.host = host;
    this.label = "app-manager";
  },

  destroy: function() {
    this.elt.remove();
    this.elt = null;
  },

  load: function(resource) {
    this.elt.textContent = "";
    let {appManagerOpts} = this.host.project;
    let iframe = this.iframe = this.elt.ownerDocument.createElement("iframe");
    iframe.setAttribute("flex", "1");
    iframe.setAttribute("src", appManagerOpts.projectOverviewURL);
    this.elt.appendChild(iframe);

    
    this.appended.then(() => {
      this.emit("load");
    });
  }
});

exports.AppProjectEditor = AppProjectEditor;
