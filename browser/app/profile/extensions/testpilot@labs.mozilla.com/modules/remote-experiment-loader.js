



































const BASE_URL_PREF = "extensions.testpilot.indexBaseURL";
var Cuddlefish = require("cuddlefish");
var resolveUrl = require("url").resolve;
var SecurableModule = require("securable-module");
let JarStore = require("jar-code-store").JarStore;

















function verifyChannelSecurity(channel) {
  
  
  
  console.info("Verifying SSL channel security info before download...");

  try {
    if (! channel instanceof  Ci.nsIChannel) {
      console.warn("Not a channel.  This should never happen.");
      return false;
    }
    let secInfo = channel.securityInfo;

    if (secInfo instanceof Ci.nsITransportSecurityInfo) {
      secInfo.QueryInterface(Ci.nsITransportSecurityInfo);
      let secState = secInfo.securityState & Ci.nsIWebProgressListener.STATE_IS_SECURE;
      if (secState != Ci.nsIWebProgressListener.STATE_IS_SECURE) {
        console.warn("Failing security check: Security state is not secure.");
        return false;
      }
    } else {
      console.warn("Failing secuity check: No TransportSecurityInfo.");
      return false;
    }

    
    if (secInfo instanceof Ci.nsISSLStatusProvider) {
      let cert = secInfo.QueryInterface(Ci.nsISSLStatusProvider).
	SSLStatus.QueryInterface(Ci.nsISSLStatus).serverCert;

      let verificationResult = cert.verifyForUsage(
        Ci.nsIX509Cert.CERT_USAGE_SSLServer);
      if (verificationResult != Ci.nsIX509Cert.VERIFIED_OK) {
        console.warn("Failing security check: Cert not verified OK.");
        return false;
      }
      if (cert.commonName != "*.mozillalabs.com") {
        console.warn("Failing security check: Cert not for *.mozillalabs.com");
        return false;
      }
      if (cert.organization != "Mozilla Corporation") {
        console.warn("Failing security check: Cert not for Mozilla corporation.");
        return false;
      }
    } else {
      console.warn("Failing security check: No SSL cert info.");
      return false;
    }

    
    console.info("Channel passed SSL security check.");
    return true;
  } catch(err) {
    console.warn("Failing security check:  Error: " + err);
    return false;
  }
}

function downloadFile(url, cb, lastModified) {
  
  
  var req = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
              .createInstance( Ci.nsIXMLHttpRequest );
  req.open('GET', url, true);
  if (lastModified != undefined) {
    let d = new Date();
    d.setTime(lastModified);
    
    req.setRequestHeader("If-Modified-Since", d.toGMTString());
    console.info("Setting if-modified-since header to " + d.toGMTString());
  }
  
  if (url.indexOf(".jar") == url.length - 4) {
    console.info("Using binary mode to download jar file.");
    req.overrideMimeType('text/plain; charset=x-user-defined');
  }
  req.onreadystatechange = function(aEvt) {
    if (req.readyState == 4) {
      if (req.status == 200) {
        
        if (verifyChannelSecurity(req.channel)) {
          cb(req.responseText);
        } else {
          cb(null);
        }
      } else if (req.status == 304) {
        
        
        console.info("File " + url + " not modified; using cached version.");
        cb(null);
        
        
      } else {
        
        console.warn("Got a " + req.status + " error code downloading " + url);
	cb(null);
      }
    }
  };
  req.send();
}




    
    
    

exports.RemoteExperimentLoader = function(logRepo, fileGetterFunction ) {
  


  this._init(logRepo, fileGetterFunction);
};

exports.RemoteExperimentLoader.prototype = {
  _init: function(logRepo, fileGetterFunction) {
    this._logger = logRepo.getLogger("TestPilot.Loader");
    this._expLogger = logRepo.getLogger("TestPilot.RemoteCode");
    let prefs = require("preferences-service");
    this._baseUrl = prefs.get(BASE_URL_PREF, "");
    if (fileGetterFunction != undefined) {
      this._fileGetter = fileGetterFunction;
    } else {
      this._fileGetter = downloadFile;
    }
    this._logger.trace("About to instantiate jar store.");
    this._jarStore = new JarStore();
    let self = this;
    this._logger.trace("About to instantiate cuddlefish loader.");
    this._refreshLoader();
    
    require("unload").when( function() {
                              self._loader.unload();
                            });
    this._logger.trace("Done instantiating remoteExperimentLoader.");
  },

  _refreshLoader: function() {
    if (this._loader) {
      this._loader.unload();
    }
    





    


    let self = this;
    this._loader = Cuddlefish.Loader(
      {fs: new SecurableModule.CompositeFileSystem(
         [self._jarStore, Cuddlefish.parentLoader.fs]),
       console: this._expLogger
      });

    
    this._studyResults = [];
    this._legacyStudies = [];
    this._experimentFileNames = [];
    this._loadErrors = [];
  },

  getLocalizedStudyInfo: function(studiesIndex) {
    let prefs = require("preferences-service");
    let myLocale = prefs.get("general.useragent.locale", "");
    let studiesToLoad = [];
    for each (let set in studiesIndex) {
      
      if (set[myLocale]) {
        studiesToLoad.push(set[myLocale]);
        continue;
      }
      
      let hyphen = myLocale.indexOf("-");
      if (hyphen > -1) {
        let lang = myLocale.slice(0, hyphen);
        if (set[lang]) {
          studiesToLoad.push(set[lang]);
          continue;
        }
      }
      
      if(set["default"]) {
        studiesToLoad.push(set["default"]);
      }
      
    }
    return studiesToLoad;
  },

  _executeFreshIndexFile: function(data, callback) {
    try {
      data = JSON.parse(data);
    } catch (e) {
      this._logger.warn("Error parsing index.json: " + e );
      callback(false);
      return;
    }

    
    this._studyResults = data.results;
    this._legacyStudies = data.legacy;


    



    if (data.maintain_experiments) {
      this._logger.trace(data.maintain_experiments.length + " files to maintain.\n");
      for each (let studyFile in data.maintain_experiments) {
        this._experimentFileNames.push(studyFile);
      }
    }

    




    let jarFiles = this.getLocalizedStudyInfo(data.new_experiments);
    let numFilesToDload = jarFiles.length;
    this._logger.trace(numFilesToDload + " files to download.\n");
    let self = this;

    if (numFilesToDload == 0) {
      this._logger.trace("Num files to download is 0, bailing\n");
      
      callback(false);
      return;
    }

    for each (let j in jarFiles) {
      let filename = j.jarfile;
      let hash = j.hash;
      if (j.studyfile) {
        this._experimentFileNames.push(j.studyfile);
      }
      this._logger.trace("I'm gonna go try to get the code for " + filename);
      let modDate = this._jarStore.getFileModifiedDate(filename);

      this._fileGetter(resolveUrl(this._baseUrl, filename),
      function onDone(code) {
        
        if (code) {
          self._logger.info("Downloaded jar file " + filename);
          self._jarStore.saveJarFile(filename, code, hash);
          self._logger.trace("Saved code for: " + filename);
        } else {
          self._logger.info("Nothing to download for " + filename);
        }
        numFilesToDload--;
        if (numFilesToDload == 0) {
          self._logger.trace("Calling callback.");
          callback(true);
        }
      }, modDate);
    }
  },

  _executeCachedIndexFile: function(data) {
    



    try {
      data = JSON.parse(data);
    } catch (e) {
      this._logger.warn("Error parsing index.json: " + e );
      return false;
    }
    
    this._studyResults = data.results;
    this._legacyStudies = data.legacy;

    
    if (data.maintain_experiments) {
      for each (let studyFile in data.maintain_experiments) {
        this._experimentFileNames.push(studyFile);
      }
    }

    
    let jarFiles = this.getLocalizedStudyInfo(data.new_experiments);
    for each (let j in jarFiles) {
      let filename = j.jarfile;
      let hash = j.hash;
      if (j.studyfile) {
        this._experimentFileNames.push(j.studyfile);
      }
    }
    return true;
  },

  
  
  
  

  
  
  
  

  
  
  

  _cachedIndexNsiFile: null,
  get cachedIndexNsiFile() {
    if (!this._cachedIndexNsiFile) {
      try {
        let file = Cc["@mozilla.org/file/directory_service;1"].
                         getService(Ci.nsIProperties).
                         get("ProfD", Ci.nsIFile);
        file.append("TestPilotExperimentFiles"); 
        
        
        if (file.exists() && !file.isDirectory()) {
          file.remove(false);
        }
        if (!file.exists()) {
          file.create(Ci.nsIFile.DIRECTORY_TYPE, 0777);
        }
        file.append("index.json");
        this._cachedIndexNsiFile = file;
      } catch(e) {
        console.warn("Error creating directory for cached index file: " + e);
      }
    }
    return this._cachedIndexNsiFile;
  },

  _cacheIndexFile: function(data) {
    
    try {
      let file = this.cachedIndexNsiFile;
      if (file == null) {
        console.warn("Can't cache index file because directory does not exist.");
        return;
      }
      if (file.exists()) {
        file.remove(false);
      }
      file.create(Ci.nsIFile.NORMAL_FILE_TYPE, 0666);
      
      let foStream = Cc["@mozilla.org/network/file-output-stream;1"].
                               createInstance(Ci.nsIFileOutputStream);

      foStream.init(file, 0x02 | 0x08 | 0x20, 0666, 0);
      
      let converter = Cc["@mozilla.org/intl/converter-output-stream;1"].
                                createInstance(Ci.nsIConverterOutputStream);
      converter.init(foStream, "UTF-8", 0, 0);
      converter.writeString(data);
      converter.close(); 
    } catch(e) {
      console.warn("Error cacheing index file: " + e);
    }
  },

  
  _loadCachedIndexFile: function() {
    
    
    let file = this.cachedIndexNsiFile;
    if (file == null) {
      console.warn("Can't load cached index file because directory does not exist.");
      return false;
    }
    if (file.exists()) {
      try {
        let data = "";
        let fstream = Cc["@mozilla.org/network/file-input-stream;1"].
                          createInstance(Ci.nsIFileInputStream);
        let cstream = Cc["@mozilla.org/intl/converter-input-stream;1"].
                          createInstance(Ci.nsIConverterInputStream);
        fstream.init(file, -1, 0, 0);
        cstream.init(fstream, "UTF-8", 0, 0);
        let str = {};
        while (cstream.readString(4096, str) != 0) {
          data += str.value;
        }
        cstream.close(); 
        return data;
      } catch(e) {
        console.warn("Error occured in reading cached index file: " + e);
        return false;
      }
    } else {
      console.warn("Trying to load cached index file but it does not exist.");
      return false;
    }
  },

  checkForUpdates: function(callback) {
    
    
    


    let prefs = require("preferences-service");
    let indexFileName = prefs.get("extensions.testpilot.indexFileName",
                                  "index.json");
    let self = this;
    
    
    this._logger.info("Unloading everything to prepare to check for updates.");
    this._refreshLoader();

    let modDate = 0;
    if (this.cachedIndexNsiFile) {
      if (this.cachedIndexNsiFile.exists()) {
        modDate = this.cachedIndexNsiFile.lastModifiedTime;
      }
    }
    let url = resolveUrl(self._baseUrl, indexFileName);
    self._fileGetter(url, function onDone(data) {
      if (data) {
        self._logger.trace("Index file updated on server.\n");
        self._executeFreshIndexFile(data, callback);
        
        self._cacheIndexFile(data);
        
      } else {
        self._logger.info("Could not download index.json, using cached version.");
        let data = self._loadCachedIndexFile();
        if (data) {
          let success = self._executeCachedIndexFile(data);
          callback(success);
        } else {
          self._logger.warn("Could not download index.json and no cached version.");
          
          callback(false);
        }
      }
    }, modDate);
  },

  getExperiments: function() {
    


    this._logger.trace("GetExperiments called.");
    let remoteExperiments = {};
    this._loadErrors = [];
    for each (filename in this._experimentFileNames) {
      this._logger.debug("GetExperiments is loading " + filename);
      try {
        remoteExperiments[filename] = this._loader.require(filename);
        this._logger.info("Loaded " + filename + " OK.");
      } catch(e) {
        



        let errStr = e.name + " on line " + e.lineNumber + " of file " +
          e.fileName + ": " + e.message;
        this._loadErrors.push(errStr);
        this._logger.warn("Error loading " + filename);
        this._logger.warn(errStr);
      }
    }
    return remoteExperiments;
  },

  getStudyResults: function() {
    return this._studyResults;
  },

  getLegacyStudies: function() {
    return this._legacyStudies;
  },

  getLoadErrors: function() {
    return this._loadErrors;
  }
};









