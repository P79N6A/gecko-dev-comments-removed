



this.EXPORTED_SYMBOLS = ["WebappRT"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/AppsUtils.jsm");

XPCOMUtils.defineLazyGetter(this, "FileUtils", function() {
  Cu.import("resource://gre/modules/FileUtils.jsm");
  return FileUtils;
});

this.WebappRT = {
  _config: null,

  get config() {
    if (this._config)
      return this._config;

    let webappFile = FileUtils.getFile("AppRegD", ["webapp.json"]);

    let inputStream = Cc["@mozilla.org/network/file-input-stream;1"].
                      createInstance(Ci.nsIFileInputStream);
    inputStream.init(webappFile, -1, 0, Ci.nsIFileInputStream.CLOSE_ON_EOF);
    let json = Cc["@mozilla.org/dom/json;1"].createInstance(Ci.nsIJSON);
    let config = json.decodeFromStream(inputStream, webappFile.fileSize);

    return this._config = config;
  },

  
  
  
  
  
  
  set config(newVal) {
    this._config = JSON.parse(JSON.stringify(newVal));
  },

  get launchURI() {
    let manifest = new ManifestHelper(this.config.app.manifest,
                                      this.config.app.origin);
    return manifest.fullLaunchPath();
  }
};
