


"use strict";

Cu.import("resource://gre/modules/osfile.jsm");
const {VariablesView} =
  Cu.import("resource:///modules/devtools/VariablesView.jsm", {});

const VARIABLES_VIEW_URL =
  "chrome://browser/content/devtools/widgets/VariablesView.xul";

function ManifestEditor(project) {
  this.project = project;
  this._onContainerReady = this._onContainerReady.bind(this);
  this._onEval = this._onEval.bind(this);
  this._onSwitch = this._onSwitch.bind(this);
  this._onDelete = this._onDelete.bind(this);
}

ManifestEditor.prototype = {
  get manifest() { return this.project.manifest; },

  show: function(containerElement) {
    let deferred = promise.defer();
    let iframe = document.createElement("iframe");

    iframe.addEventListener("load", function onIframeLoad() {
      iframe.removeEventListener("load", onIframeLoad, true);
      deferred.resolve(iframe.contentWindow);
    }, true);

    iframe.setAttribute("src", VARIABLES_VIEW_URL);
    iframe.classList.add("variables-view");
    containerElement.appendChild(iframe);

    return deferred.promise.then(this._onContainerReady);
  },

  _onContainerReady: function(varWindow) {
    let variablesContainer = varWindow.document.querySelector("#variables");

    let editor = this.editor = new VariablesView(variablesContainer);

    editor.onlyEnumVisible = true;
    editor.eval = this._onEval;
    editor.switch = this._onSwitch;
    editor.delete = this._onDelete;

    return this.update();
  },

  _onEval: function(evalString) {
    let manifest = this.manifest;
    eval("manifest" + evalString);
    this.update();
  },

  _onSwitch: function(variable, newName) {
    let manifest = this.manifest;
    let newSymbolicName = variable.ownerView.symbolicName +
                          "['" + newName + "']";
    if (newSymbolicName == variable.symbolicName) {
      return;
    }

    let evalString = "manifest" + newSymbolicName + " = " +
                     "manifest" + variable.symbolicName + ";" +
                     "delete manifest" + variable.symbolicName;

    eval(evalString);
    this.update();
  },

  _onDelete: function(variable) {
    let manifest = this.manifest;
    let evalString = "delete manifest" + variable.symbolicName;
    eval(evalString);
  },

  update: function() {
    this.editor.createHierarchy();
    this.editor.rawObject = this.manifest;
    this.editor.commitHierarchy();

    
    let deferred = promise.defer();
    setTimeout(deferred.resolve, this.editor.lazyEmptyDelay + 1);
    return deferred.promise;
  },

  save: function() {
    if (this.project.type == "packaged") {
      let validator = new AppValidator(this.project);
      let manifestFile = validator._getPackagedManifestFile();
      let path = manifestFile.path;

      let encoder = new TextEncoder();
      let data = encoder.encode(JSON.stringify(this.manifest, null, 2));

      return OS.File.writeAtomic(path, data, { tmpPath: path + ".tmp" });
    }

    return promise.resolve();
  }
};
