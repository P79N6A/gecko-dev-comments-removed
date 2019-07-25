



































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
    this._studyResults = [];
    this._legacyStudies = [];
    let prefs = require("preferences-service");
    this._baseUrl = prefs.get(BASE_URL_PREF, "");
    if (fileGetterFunction != undefined) {
      this._fileGetter = fileGetterFunction;
    } else {
      this._fileGetter = downloadFile;
    }
    this._logger.trace("About to instantiate preferences store.");
    this._jarStore = new JarStore();
    this._experimentFileNames = [];
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

  checkForUpdates: function(callback) {
    


    let prefs = require("preferences-service");
    let indexFileName = prefs.get("extensions.testpilot.indexFileName",
                                  "index.json");
    let self = this;
    
    
    this._logger.info("Unloading everything to prepare to check for updates.");
    this._refreshLoader();

    
    let url = resolveUrl(self._baseUrl, indexFileName);
    self._fileGetter(url, function onDone(data) {
      if (data) {
        try {
          data = JSON.parse(data);
        } catch (e) {
          self._logger.warn("Error parsing index.json: " + e );
          callback(false);
          return;
        }

        
        self._studyResults = data.results;
        self._legacyStudies = data.legacy;

        


        let jarFiles = self.getLocalizedStudyInfo(data.new_experiments);
        let numFilesToDload = jarFiles.length;

        for each (let j in jarFiles) {
          let filename = j.jarfile;
          let hash = j.hash;
          if (j.studyfile) {
            self._experimentFileNames.push(j.studyfile);
          }
          self._logger.trace("I'm gonna go try to get the code for " + filename);
          let modDate = self._jarStore.getFileModifiedDate(filename);

          self._fileGetter(resolveUrl(self._baseUrl, filename),
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

      } else {
        self._logger.warn("Could not download index.json from test pilot server.");
        callback(false);
      }
    });
  },

  getExperiments: function() {
    


    this._logger.trace("GetExperiments called.");
    let remoteExperiments = {};
    for each (filename in this._experimentFileNames) {
      this._logger.debug("GetExperiments is loading " + filename);
      try {
        remoteExperiments[filename] = this._loader.require(filename);
        this._logger.info("Loaded " + filename + " OK.");
      } catch(e) {
        this._logger.warn("Error loading " + filename);
        this._logger.warn(e);
      }
    }
    return remoteExperiments;
  },

  getStudyResults: function() {
    return this._studyResults;
  },

  getLegacyStudies: function() {
    return this._legacyStudies;
  }
};






















