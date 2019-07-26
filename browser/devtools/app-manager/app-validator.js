



let {Ci,Cu,CC,Cc} = require("chrome");
const promise = require("sdk/core/promise");

const {FileUtils} = Cu.import("resource://gre/modules/FileUtils.jsm");
const {Services} = Cu.import("resource://gre/modules/Services.jsm");
let XMLHttpRequest = CC("@mozilla.org/xmlextras/xmlhttprequest;1");
let strings = Services.strings.createBundle("chrome://browser/locale/devtools/app-manager.properties");

function AppValidator(project) {
  this.project = project;
  this.errors = [];
  this.warnings = [];
}

AppValidator.prototype.error = function (message) {
  this.errors.push(message);
}

AppValidator.prototype.warning = function (message) {
  this.warnings.push(message);
}

AppValidator.prototype._getPackagedManifestURL = function () {
  let manifestFile = FileUtils.File(this.project.location);
  if (!manifestFile.exists()) {
    this.error(strings.GetStringFromName("validator.nonExistingFolder"));
    return;
  }
  if (!manifestFile.isDirectory()) {
    this.error(strings.GetStringFromName("validator.expectProjectFolder"));
    return;
  }
  manifestFile.append("manifest.webapp");
  if (!manifestFile.exists() || !manifestFile.isFile()) {
    this.error(strings.GetStringFromName("validator.wrongManifestFileName"));
    return;
  }

  return Services.io.newFileURI(manifestFile).spec;
}

AppValidator.prototype._fetchManifest = function (manifestURL) {
  let deferred = promise.defer();

  let req = new XMLHttpRequest();
  try {
    req.open("GET", manifestURL, true);
  } catch(e) {
    this.error(strings.formatStringFromName("validator.invalidManifestURL", [manifestURL], 1));
    deferred.resolve(null);
    return deferred.promise;
  }
  req.channel.loadFlags |= Ci.nsIRequest.INHIBIT_CACHING;
  req.onload = (function () {
    let manifest = null;
    try {
      manifest = JSON.parse(req.responseText);
    } catch(e) {
      this.error(strings.formatStringFromName("validator.invalidManifestJSON", [e, manifestURL], 2));
    }
    deferred.resolve(manifest);
  }).bind(this);
  req.onerror = (function () {
    this.error(strings.formatStringFromName("validator.noAccessManifestURL", [req.statusText, manifestURL], 2));
    deferred.resolve(null);
  }).bind(this);

  try {
    req.send(null);
  } catch(e) {
    this.error(strings.formatStringFromName("validator.noAccessManifestURL", [e, manifestURL], 2));
    deferred.resolve();
  }

  return deferred.promise;
}

AppValidator.prototype._getManifest = function () {
  let manifestURL;
  if (this.project.type == "packaged") {
    manifestURL = this._getPackagedManifestURL();
    if (!manifestURL)
      return promise.resolve(null);
  } else if (this.project.type == "hosted") {
    manifestURL = this.project.location;
    try {
      Services.io.newURI(manifestURL, null, null);
    } catch(e) {
      this.error(strings.formatStringFromName("validator.invalidHostedManifestURL", [manifestURL, e.message]));
      return promise.resolve(null);
    }
  } else {
    this.error(strings.formatStringFromName("validator.invalidProjectType", [this.project.type], 1));
    return promise.resolve(null);
  }
  return this._fetchManifest(manifestURL);
}

AppValidator.prototype.validateManifest = function (manifest) {
  if (!manifest.name) {
    this.error(strings.GetStringFromName("validator.missNameManifestProperty"));
    return;
  }

  if (!manifest.icons || Object.keys(manifest.icons).length == 0) {
    this.warning(strings.GetStringFromName("validator.missIconsManifestProperty"));
  } else if (!manifest.icons["128"]) {
    this.warning(strings.GetStringFromName("validator.missIconMarketplace"));
  }
}

AppValidator.prototype.validateType = function (manifest) {
  let appType = manifest.type || "web";
  if (["web", "privileged", "certified"].indexOf(appType) === -1) {
    this.error(strings.formatStringFromName("validator.invalidAppType", [appType], 1));
  } else if (this.project.type == "hosted" &&
             ["certified", "privileged"].indexOf(appType) !== -1) {
    this.error(strings.formatStringFromName("validator.invalidHostedPriviledges", [appType], 1));
  }

  
  if (appType === "certified") {
    this.warning(strings.GetStringFromName("validator.noCertifiedSupport"));
  }
}

AppValidator.prototype.validate = function () {
  this.errors = [];
  this.warnings = [];
  return this._getManifest().
    then((function (manifest) {
      if (manifest) {
        this.manifest = manifest;
        this.validateManifest(manifest);
        this.validateType(manifest);
      }
    }).bind(this));
}

exports.AppValidator = AppValidator;

