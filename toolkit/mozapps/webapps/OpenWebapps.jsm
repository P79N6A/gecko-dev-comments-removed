




































const Cu = Components.utils; 
const Cc = Components.classes;
const Ci = Components.interfaces;

let EXPORTED_SYMBOLS = ["OpenWebapps"];

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyGetter(this, "NetUtil", function() {
  Cu.import("resource://gre/modules/NetUtil.jsm");
  return NetUtil;
});

let OpenWebapps = {
  appsDir: null,
  appsFile: null,
  webapps: { },

  init: function() {
    let file =  Cc["@mozilla.org/file/directory_service;1"].getService(Ci.nsIProperties).get("ProfD", Ci.nsIFile);
    file.append("webapps");
    if (!file.exists() || !file.isDirectory()) {
      file.create(Ci.nsIFile.DIRECTORY_TYPE, 0700);
    }
    this.appsDir = file;
    this.appsFile = file.clone();
    this.appsFile.append("webapps.json");
    if (!this.appsFile.exists())
      return;
    
    try {
      let channel = NetUtil.newChannel(this.appsFile);
      channel.contentType = "application/json";
      let self = this;
      NetUtil.asyncFetch(channel, function(aStream, aResult) {
        if (!Components.isSuccessCode(aResult)) {
          Cu.reportError("OpenWebappsSupport: Could not read from json file " + this.appsFile.path);
          return;
        }

        
        let data = null;
        try {
          self.webapps = JSON.parse(NetUtil.readInputStreamToString(aStream, aStream.available()) || "");
          aStream.close();
        } catch (ex) {
          Cu.reportError("OpenWebsappsStore: Could not parse JSON: " + ex);
        }
      });
    } catch (ex) {
      Cu.reportError("OpenWebappsSupport: Could not read from " + aFile.path + " : " + ex);
    }
  },

  _writeFile: function ss_writeFile(aFile, aData) {
    
    let ostream = Cc["@mozilla.org/network/safe-file-output-stream;1"].createInstance(Ci.nsIFileOutputStream);
    ostream.init(aFile, 0x02 | 0x08 | 0x20, 0600, ostream.DEFER_OPEN);

    
    let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"].createInstance(Ci.nsIScriptableUnicodeConverter);
    converter.charset = "UTF-8";

    
    let istream = converter.convertToInputStream(aData);
    NetUtil.asyncCopy(istream, ostream, function(rc) {
      
    });
  },
  
  install: function(aApplication) {
    let id = this._appId(aApplication.appURI);

    
    if (id) {
      let dir = this.appsDir.clone();
      dir.append(id);
      try {
        dir.remove(true);
      } catch(e) {
      }
    }
    else {
      let uuidGenerator = Cc["@mozilla.org/uuid-generator;1"].getService(Ci.nsIUUIDGenerator);
      id = uuidGenerator.generateUUID().toString();
    }

    let dir = this.appsDir.clone();
    dir.append(id);
    dir.create(Ci.nsIFile.DIRECTORY_TYPE, 0700);
    
    let manFile = dir.clone();
    manFile.append("manifest.json");
    this._writeFile(manFile, JSON.stringify(aApplication.manifest));
    
    this.webapps[id] = {
      title: aApplication.title,
      storeURI: aApplication.storeURI,
      appURI: aApplication.appURI,
      installData: aApplication.installData,
      installTime: (new Date()).getTime()
    };
    this._writeFile(this.appsFile, JSON.stringify(this.webapps));
  },
 
  _appId: function(aURI) {
    for (let id in this.webapps) {
      if (this.webapps[id].appURI == aURI)
        return id;
    }
    return null;
  },

  _readManifest: function(aId) {
    let file = this.appsDir.clone();
    file.append(aId);
    file.append("manifest.json");
    let data = "";  
    let fstream = Cc["@mozilla.org/network/file-input-stream;1"].createInstance(Ci.nsIFileInputStream);
    var cstream = Cc["@mozilla.org/intl/converter-input-stream;1"].createInstance(Ci.nsIConverterInputStream);
    fstream.init(file, -1, 0, 0);
    cstream.init(fstream, "UTF-8", 0, 0);
    let (str = {}) {  
      let read = 0;  
      do {   
        read = cstream.readString(0xffffffff, str); 
        data += str.value;  
      } while (read != 0);  
    }  
    cstream.close(); 
    try {
      return JSON.parse(data);
    } catch(e) {
      return null;
    }
  },

  amInstalled: function(aURI) {
    for (let id in this.webapps) {
      let app = this.webapps[id];
      if (app.appURI == aURI) {
        return { origin: app.appURI,
                 install_origin: app.storeURI,
                 install_data: app.installData,
                 install_time: app.installTime,
                 manifest: this._readManifest(id) };
      }
    }
    return null;
  },

  getInstalledBy: function(aStoreURI) {
    let res = [];
    for (let id in this.webapps) {
      let app = this.webapps[id];
      if (app.storeURI == aStoreURI)
        res.push({ origin: app.appURI,
                   install_origin: app.storeURI,
                   install_data: app.installData,
                   install_time: app.installTime,
                   manifest: this._readManifest(id) });
    }
    return res;
  },
  
  mgmtList: function() {
    let res = new Array();
    for (let id in this.webapps) {
      let app = this.webapps[id];
      res.push({ origin: app.appURI,
                 install_origin: app.storeURI,
                 install_data: app.installData,
                 install_time: app.installTime,
                 manifest: this._readManifest(id) });
    }
    return res;
  },
  
  mgmtLaunch: function(aOrigin) {
    for (let id in this.webapps) {
      let app = this.webapps[id];
      app.manifest = this._readManifest(id);
      if (app.appURI == aOrigin) {
        let browserWin = Services.wm.getMostRecentWindow("navigator:browser");
        let uri = Services.io.newURI(aOrigin + (app.manifest.launch_path ? app.manifest.launch_path : ""), null, null);
        browserWin.browserDOMWindow.openURI(uri, null, browserWin.OPEN_APPTAB, Ci.nsIBrowserDOMWindow.OPEN_NEW);
        return true;
      }
    }
    return false;
  },
  
  mgmtUninstall: function(aOrigin) {
    for (let id in this.webapps) {
      let app = this.webapps[id];
      if (app.appURI == aOrigin) {
        delete this.webapps[id];
        this._writeFile(this.appsFile, JSON.stringify(this.webapps));
        let dir = this.appsDir.clone();
        dir.append(id);
        try {
          dir.remove(true);
        } catch (e) {
        }
        return true;
      }
    }
    return false;
  }
};

OpenWebapps.init();

