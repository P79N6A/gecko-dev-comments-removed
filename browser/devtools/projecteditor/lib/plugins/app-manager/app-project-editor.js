





const { Cu } = require("chrome");
const { Class } = require("sdk/core/heritage");
const promise = require("projecteditor/helpers/promise");
const { ItchEditor } = require("projecteditor/editors");

var AppProjectEditor = Class({
  extends: ItchEditor,

  hidesToolbar: true,

  initialize: function(host) {
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
    let {appManagerOpts} = this.host.project;

    
    if (!this.iframe || this.iframe.getAttribute("src") !== appManagerOpts.projectOverviewURL) {

      this.elt.textContent = "";
      let iframe = this.iframe = this.elt.ownerDocument.createElement("iframe");
      let iframeLoaded = this.iframeLoaded = promise.defer();

      iframe.addEventListener("load", function onLoad() {
        iframe.removeEventListener("load", onLoad);
        iframeLoaded.resolve();
      });

      iframe.setAttribute("flex", "1");
      iframe.setAttribute("src", appManagerOpts.projectOverviewURL);
      this.elt.appendChild(iframe);

    }

    promise.all([this.iframeLoaded.promise, this.appended]).then(() => {
      this.emit("load");
    });
  }
});

exports.AppProjectEditor = AppProjectEditor;
