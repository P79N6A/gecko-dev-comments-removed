



"use strict";

this.EXPORTED_SYMBOLS = [];

const {classes: Cc, interfaces: Ci, results: Cr, utils: Cu, manager: Cm} =
  Components;

const DOWNLOAD_CHUNK_BYTES_SIZE = 300000;

const DOWNLOAD_INTERVAL  = 0;

const DEFAULT_SECONDS_BETWEEN_CHECKS = 60 * 60 * 24;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/FileUtils.jsm");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/Preferences.jsm");
Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://gre/modules/osfile.jsm");
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/ctypes.jsm");
Cu.import("resource://gre/modules/GMPUtils.jsm");
Cu.import("resource://gre/modules/AppConstants.jsm");

this.EXPORTED_SYMBOLS = ["GMPInstallManager", "GMPExtractor", "GMPDownloader",
                         "GMPAddon"];

var gLocale = null;


XPCOMUtils.defineLazyGetter(this, "gCertUtils", function() {
  let temp = { };
  Cu.import("resource://gre/modules/CertUtils.jsm", temp);
  return temp;
});

XPCOMUtils.defineLazyModuleGetter(this, "UpdateChannel",
                                  "resource://gre/modules/UpdateChannel.jsm");









const CHECK_FOR_ADDONS_TIMEOUT_DELAY_MS = 20000;

function getScopedLogger(prefix) {
  
  
  return Log.repository.getLoggerWithMessagePrefix("Toolkit.GMP", prefix + " ");
}




XPCOMUtils.defineLazyGetter(this, "gOSVersion", function aus_gOSVersion() {
  let osVersion;
  let sysInfo = Cc["@mozilla.org/system-info;1"].
                getService(Ci.nsIPropertyBag2);
  try {
    osVersion = sysInfo.getProperty("name") + " " + sysInfo.getProperty("version");
  }
  catch (e) {
    LOG("gOSVersion - OS Version unknown: updates are not possible.");
  }

  if (osVersion) {
    if (AppConstants.platform == "win") {
      const BYTE = ctypes.uint8_t;
      const WORD = ctypes.uint16_t;
      const DWORD = ctypes.uint32_t;
      const WCHAR = ctypes.char16_t;
      const BOOL = ctypes.int;
  
      
      
      const SZCSDVERSIONLENGTH = 128;
      const OSVERSIONINFOEXW = new ctypes.StructType('OSVERSIONINFOEXW',
          [
          {dwOSVersionInfoSize: DWORD},
          {dwMajorVersion: DWORD},
          {dwMinorVersion: DWORD},
          {dwBuildNumber: DWORD},
          {dwPlatformId: DWORD},
          {szCSDVersion: ctypes.ArrayType(WCHAR, SZCSDVERSIONLENGTH)},
          {wServicePackMajor: WORD},
          {wServicePackMinor: WORD},
          {wSuiteMask: WORD},
          {wProductType: BYTE},
          {wReserved: BYTE}
          ]);
  
      
      
      const SYSTEM_INFO = new ctypes.StructType('SYSTEM_INFO',
          [
          {wProcessorArchitecture: WORD},
          {wReserved: WORD},
          {dwPageSize: DWORD},
          {lpMinimumApplicationAddress: ctypes.voidptr_t},
          {lpMaximumApplicationAddress: ctypes.voidptr_t},
          {dwActiveProcessorMask: DWORD.ptr},
          {dwNumberOfProcessors: DWORD},
          {dwProcessorType: DWORD},
          {dwAllocationGranularity: DWORD},
          {wProcessorLevel: WORD},
          {wProcessorRevision: WORD}
          ]);
  
      let kernel32 = false;
      try {
        kernel32 = ctypes.open("Kernel32");
      } catch (e) {
        LOG("gOSVersion - Unable to open kernel32! " + e);
        osVersion += ".unknown (unknown)";
      }
  
      if(kernel32) {
        try {
          
          try {
            let GetVersionEx = kernel32.declare("GetVersionExW",
                                                ctypes.default_abi,
                                                BOOL,
                                                OSVERSIONINFOEXW.ptr);
            let winVer = OSVERSIONINFOEXW();
            winVer.dwOSVersionInfoSize = OSVERSIONINFOEXW.size;
  
            if(0 !== GetVersionEx(winVer.address())) {
              osVersion += "." + winVer.wServicePackMajor
                        +  "." + winVer.wServicePackMinor;
            } else {
              LOG("gOSVersion - Unknown failure in GetVersionEX (returned 0)");
              osVersion += ".unknown";
            }
          } catch (e) {
            LOG("gOSVersion - error getting service pack information. Exception: " + e);
            osVersion += ".unknown";
          }
  
          
          let arch = "unknown";
          try {
            let GetNativeSystemInfo = kernel32.declare("GetNativeSystemInfo",
                                                       ctypes.default_abi,
                                                       ctypes.void_t,
                                                       SYSTEM_INFO.ptr);
            let sysInfo = SYSTEM_INFO();
            
            sysInfo.wProcessorArchitecture = 0xffff;
  
            GetNativeSystemInfo(sysInfo.address());
            switch(sysInfo.wProcessorArchitecture) {
              case 9:
                arch = "x64";
                break;
              case 6:
                arch = "IA64";
                break;
              case 0:
                arch = "x86";
                break;
            }
          } catch (e) {
            LOG("gOSVersion - error getting processor architecture.  Exception: " + e);
          } finally {
            osVersion += " (" + arch + ")";
          }
        } finally {
          kernel32.close();
        }
      }
    }

    try {
      osVersion += " (" + sysInfo.getProperty("secondaryLibrary") + ")";
    }
    catch (e) {
      
    }
    osVersion = encodeURIComponent(osVersion);
  }
  return osVersion;
});




XPCOMUtils.defineLazyGetter(this, "gABI", function aus_gABI() {
  let abi = null;
  try {
    abi = Services.appinfo.XPCOMABI;
  }
  catch (e) {
    LOG("gABI - XPCOM ABI unknown: updates are not possible.");
  }
  if (AppConstants.platform == "macosx") {
    
    
    let macutils = Cc["@mozilla.org/xpcom/mac-utils;1"].
                   getService(Ci.nsIMacUtils);

    if (macutils.isUniversalBinary)
      abi += "-u-" + macutils.architecturesInBinary;
    if (AppConstants.MOZ_SHARK) {
      
      abi += "-shark"
    }
  }
  return abi;
});




function GMPInstallManager() {
}



GMPInstallManager.prototype = {
  


  _getURL: function() {
    let log = getScopedLogger("GMPInstallManager._getURL");
    
    
    let url = GMPPrefs.get(GMPPrefs.KEY_URL_OVERRIDE);
    if (url) {
      log.info("Using override url: " + url);
    } else {
      url = GMPPrefs.get(GMPPrefs.KEY_URL);
      log.info("Using url: " + url);
    }

    url =
      url.replace(/%PRODUCT%/g, Services.appinfo.name)
         .replace(/%VERSION%/g, Services.appinfo.version)
         .replace(/%BUILD_ID%/g, Services.appinfo.appBuildID)
         .replace(/%BUILD_TARGET%/g, Services.appinfo.OS + "_" + gABI)
         .replace(/%OS_VERSION%/g, gOSVersion);
    if (/%LOCALE%/.test(url)) {
      
      url = url.replace(/%LOCALE%/g, "en-US");
    }
    url =
      url.replace(/%CHANNEL%/g, UpdateChannel.get())
         .replace(/%PLATFORM_VERSION%/g, Services.appinfo.platformVersion)
         .replace(/%DISTRIBUTION%/g,
                  GMPPrefs.get(GMPPrefs.KEY_APP_DISTRIBUTION))
         .replace(/%DISTRIBUTION_VERSION%/g,
                  GMPPrefs.get(GMPPrefs.KEY_APP_DISTRIBUTION_VERSION))
         .replace(/\+/g, "%2B");
    log.info("Using url (with replacement): " + url);
    return url;
  },
  








  checkForAddons: function() {
    let log = getScopedLogger("GMPInstallManager.checkForAddons");
    if (this._deferred) {
        log.error("checkForAddons already called");
        return Promise.reject({type: "alreadycalled"});
    }
    this._deferred = Promise.defer();
    let url = this._getURL();

    this._request = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"].
                    createInstance(Ci.nsISupports);
    
    if (this._request.wrappedJSObject) {
      this._request = this._request.wrappedJSObject;
    }
    this._request.open("GET", url, true);
    let allowNonBuiltIn = !GMPPrefs.get(GMPPrefs.KEY_CERT_CHECKATTRS, true);
    this._request.channel.notificationCallbacks =
      new gCertUtils.BadCertHandler(allowNonBuiltIn);
    
    this._request.channel.loadFlags |= Ci.nsIRequest.LOAD_BYPASS_CACHE;
    
    this._request.channel.loadFlags |= Ci.nsIRequest.INHIBIT_CACHING;

    this._request.overrideMimeType("text/xml");
    
    
    
    this._request.setRequestHeader("Cache-Control", "no-cache");
    
    
    this._request.setRequestHeader("Pragma", "no-cache");

    this._request.timeout = CHECK_FOR_ADDONS_TIMEOUT_DELAY_MS;
    this._request.addEventListener("error", event => this.onFailXML("onErrorXML", event), false);
    this._request.addEventListener("abort", event => this.onFailXML("onAbortXML", event), false);
    this._request.addEventListener("timeout", event => this.onFailXML("onTimeoutXML", event), false);
    this._request.addEventListener("load", event => this.onLoadXML(event), false);

    log.info("sending request to: " + url);
    this._request.send(null);

    return this._deferred.promise;
  },
  










  installAddon: function(gmpAddon) {
    if (this._deferred) {
        log.error("previous error encountered");
        return Promise.reject({type: "previouserrorencountered"});
    }
    this.gmpDownloader = new GMPDownloader(gmpAddon);
    return this.gmpDownloader.start();
  },
  _getTimeSinceLastCheck: function() {
    let now = Math.round(Date.now() / 1000);
    
    
    let lastCheck = GMPPrefs.get(GMPPrefs.KEY_UPDATE_LAST_CHECK, 0);
    
    
    if (now < lastCheck) {
      return now;
    }
    return now - lastCheck;
  },
  get _isEMEEnabled() {
    return GMPPrefs.get(GMPPrefs.KEY_EME_ENABLED, true);
  },
  _isAddonUpdateEnabled: function(aAddon) {
    return GMPPrefs.get(GMPPrefs.KEY_PLUGIN_ENABLED, true, aAddon) &&
           GMPPrefs.get(GMPPrefs.KEY_PLUGIN_AUTOUPDATE, true, aAddon);
  },
  _updateLastCheck: function() {
    let now = Math.round(Date.now() / 1000);
    GMPPrefs.set(GMPPrefs.KEY_UPDATE_LAST_CHECK, now);
  },
  _versionchangeOccurred: function() {
    let savedBuildID = GMPPrefs.get(GMPPrefs.KEY_BUILDID, null);
    let buildID = Services.appinfo.platformBuildID;
    if (savedBuildID == buildID) {
      return false;
    }
    GMPPrefs.set(GMPPrefs.KEY_BUILDID, buildID);
    return true;
  },
  






  simpleCheckAndInstall: Task.async(function*() {
    let log = getScopedLogger("GMPInstallManager.simpleCheckAndInstall");

    if (this._versionchangeOccurred()) {
      log.info("A version change occurred. Ignoring " +
               "media.gmp-manager.lastCheck to check immediately for " +
               "new or updated GMPs.");
    } else {
      let secondsBetweenChecks =
        GMPPrefs.get(GMPPrefs.KEY_SECONDS_BETWEEN_CHECKS,
                     DEFAULT_SECONDS_BETWEEN_CHECKS)
      let secondsSinceLast = this._getTimeSinceLastCheck();
      log.info("Last check was: " + secondsSinceLast +
               " seconds ago, minimum seconds: " + secondsBetweenChecks);
      if (secondsBetweenChecks > secondsSinceLast) {
        log.info("Will not check for updates.");
        return {status: "too-frequent-no-check"};
      }
    }

    try {
      let gmpAddons = yield this.checkForAddons();
      this._updateLastCheck();
      log.info("Found " + gmpAddons.length + " addons advertised.");
      let addonsToInstall = gmpAddons.filter(function(gmpAddon) {
        log.info("Found addon: " + gmpAddon.toString());

        if (!gmpAddon.isValid || GMPUtils.isPluginHidden(gmpAddon) ||
            gmpAddon.isInstalled) {
          log.info("Addon invalid, hidden or already installed.");
          return false;
        }

        let addonUpdateEnabled = false;
        if (GMP_PLUGIN_IDS.indexOf(gmpAddon.id) >= 0) {
          addonUpdateEnabled = this._isAddonUpdateEnabled(gmpAddon.id);
          if (!addonUpdateEnabled) {
            log.info("Auto-update is off for " + gmpAddon.id +
                     ", skipping check.");
          }
        } else {
          
          log.info("Auto-update is off for unknown plugin '" + gmpAddon.id +
                   "', skipping check.");
        }

        return addonUpdateEnabled;
      }, this);

      if (!addonsToInstall.length) {
        log.info("No new addons to install, returning");
        return {status: "nothing-new-to-install"};
      }

      let installResults = [];
      let failureEncountered = false;
      for (let addon of addonsToInstall) {
        try {
          yield this.installAddon(addon);
          installResults.push({
            id:     addon.id,
            result: "succeeded",
          });
        } catch (e) {
          failureEncountered = true;
          installResults.push({
            id:     addon.id,
            result: "failed",
          });
        }
      }
      if (failureEncountered) {
        throw {status:  "failed",
               results: installResults};
      }
      return {status:  "succeeded",
              results: installResults};
    } catch(e) {
      log.error("Could not check for addons", e);
      throw e;
    }
  }),

  


  uninit: function() {
    let log = getScopedLogger("GMPInstallManager.uninit");
    if (this._request) {
      log.info("Aborting request");
      this._request.abort();
    }
    if (this._deferred) {
        log.info("Rejecting deferred");
        this._deferred.reject({type: "uninitialized"});
    }
    log.info("Done cleanup");
  },

  



  overrideLeaveDownloadedZip: false,

  



  onLoadXML: function(event) {
    let log = getScopedLogger("GMPInstallManager.onLoadXML");
    try {
      log.info("request completed downloading document");
      let certs = null;
      if (!Services.prefs.prefHasUserValue(GMPPrefs.KEY_URL_OVERRIDE) &&
          GMPPrefs.get(GMPPrefs.KEY_CERT_CHECKATTRS, true)) {
        certs = gCertUtils.readCertPrefs(GMPPrefs.KEY_CERTS_BRANCH);
      }

      let allowNonBuiltIn = !GMPPrefs.get(GMPPrefs.KEY_CERT_REQUIREBUILTIN,
                                          true);
      log.info("allowNonBuiltIn: " + allowNonBuiltIn);

      gCertUtils.checkCert(this._request.channel, allowNonBuiltIn, certs);

      this.parseResponseXML();
    } catch (ex) {
      log.error("could not load xml: " + ex);
      this._deferred.reject({
        target: event.target,
        status: this._getChannelStatus(event.target),
        message: "" + ex,
      });
      delete this._deferred;
    }
  },

  


  _getChannelStatus: function(request) {
    let log = getScopedLogger("GMPInstallManager._getChannelStatus");
    let status = null;
    try {
      status = request.status;
      log.info("request.status is: " + request.status);
    }
    catch (e) {
    }

    if (status == null) {
      status = request.channel.QueryInterface(Ci.nsIRequest).status;
    }
    return status;
  },

  






  onFailXML: function(failure, event) {
    let log = getScopedLogger("GMPInstallManager.onFailXML " + failure);
    let request = event.target;
    let status = this._getChannelStatus(request);
    let message = "request.status: " + status +  " (" + event.type + ")";
    log.warn(message);
    this._deferred.reject({
      target: request,
      status: status,
      message: message
    });
    delete this._deferred;
  },

  




  parseResponseXML: function() {
    try {
      let log = getScopedLogger("GMPInstallManager.parseResponseXML");
      let updatesElement = this._request.responseXML.documentElement;
      if (!updatesElement) {
        let message = "empty updates document";
        log.warn(message);
        this._deferred.reject({
          target: this._request,
          message: message
        });
        delete this._deferred;
        return;
      }

      if (updatesElement.nodeName != "updates") {
        let message = "got node name: " + updatesElement.nodeName +
          ", expected: updates";
        log.warn(message);
        this._deferred.reject({
          target: this._request,
          message: message
        });
        delete this._deferred;
        return;
      }

      const ELEMENT_NODE = Ci.nsIDOMNode.ELEMENT_NODE;
      let gmpResults = [];
      for (let i = 0; i < updatesElement.childNodes.length; ++i) {
        let updatesChildElement = updatesElement.childNodes.item(i);
        if (updatesChildElement.nodeType != ELEMENT_NODE) {
          continue;
        }
        if (updatesChildElement.localName == "addons") {
          gmpResults = GMPAddon.parseGMPAddonsNode(updatesChildElement);
        }
      }
      this._deferred.resolve(gmpResults);
      delete this._deferred;
    } catch (e) {
      this._deferred.reject({
        target: this._request,
        message: e
      });
      delete this._deferred;
    }
  },
};








function GMPAddon(gmpAddon) {
  let log = getScopedLogger("GMPAddon.constructor");
  gmpAddon.QueryInterface(Ci.nsIDOMElement);
  ["id", "URL", "hashFunction",
   "hashValue", "version", "size"].forEach(name => {
    if (gmpAddon.hasAttribute(name)) {
      this[name] = gmpAddon.getAttribute(name);
    }
  });
  this.size = Number(this.size) || undefined;
  log.info ("Created new addon: " + this.toString());
}





GMPAddon.parseGMPAddonsNode = function(addonsElement) {
  let log = getScopedLogger("GMPAddon.parseGMPAddonsNode");
  let gmpResults = [];
  if (addonsElement.localName !== "addons") {
    return;
  }

  addonsElement.QueryInterface(Ci.nsIDOMElement);
  let addonCount = addonsElement.childNodes.length;
  for (let i = 0; i < addonCount; ++i) {
    let addonElement = addonsElement.childNodes.item(i);
    if (addonElement.localName !== "addon") {
      continue;
    }
    addonElement.QueryInterface(Ci.nsIDOMElement);
    try {
      gmpResults.push(new GMPAddon(addonElement));
    } catch (e) {
      log.warn("invalid addon: " + e);
      continue;
    }
  }
  return gmpResults;
};
GMPAddon.prototype = {
  


  toString: function() {
    return this.id + " (" +
           "isValid: " + this.isValid +
           ", isInstalled: " + this.isInstalled +
           ", hashFunction: " + this.hashFunction+
           ", hashValue: " + this.hashValue +
           (this.size !== undefined ? ", size: " + this.size : "" ) +
           ")";
  },
  



  get isValid() {
    return this.id && this.URL && this.version &&
      this.hashFunction && !!this.hashValue;
  },
  get isInstalled() {
    return this.version &&
      GMPPrefs.get(GMPPrefs.KEY_PLUGIN_VERSION, "", this.id) === this.version;
  },
  get isEME() {
    return this.id.indexOf("gmp-eme-") == 0;
  },
};





function GMPExtractor(zipPath, installToDirPath) {
    this.zipPath = zipPath;
    this.installToDirPath = installToDirPath;
}
GMPExtractor.prototype = {
  







  _getZipEntries: function(zipReader) {
    let entries = [];
    let enumerator = zipReader.findEntries("*.*");
    while (enumerator.hasMore()) {
      entries.push(enumerator.getNext());
    }
    return entries;
  },
  






  install: function() {
    try {
      let log = getScopedLogger("GMPExtractor.install");
      this._deferred = Promise.defer();
      log.info("Installing " + this.zipPath + "...");
      
      let zipFile = Cc["@mozilla.org/file/local;1"].
                    createInstance(Ci.nsIFile);
      zipFile.initWithPath(this.zipPath);

      
      var zipReader = Cc["@mozilla.org/libjar/zip-reader;1"].
                      createInstance(Ci.nsIZipReader);
      zipReader.open(zipFile)
      let entries = this._getZipEntries(zipReader);
      let extractedPaths = [];

      
      entries.forEach(entry => {
        
        if (entry.contains("__MACOSX")) {
          return;
        }
        let outFile = Cc["@mozilla.org/file/local;1"].
                      createInstance(Ci.nsILocalFile);
        outFile.initWithPath(this.installToDirPath);
        outFile.appendRelativePath(entry);

        
        if(!outFile.parent.exists()) {
          outFile.parent.create(Ci.nsIFile.DIRECTORY_TYPE, parseInt("0755", 8));
        }
        zipReader.extract(entry, outFile);
        extractedPaths.push(outFile.path);
        log.info(entry + " was successfully extracted to: " +
            outFile.path);
      });
      zipReader.close();
      if (!GMPInstallManager.overrideLeaveDownloadedZip) {
        zipFile.remove(false);
      }

      log.info(this.zipPath + " was installed successfully");
      this._deferred.resolve(extractedPaths);
    } catch (e) {
      if (zipReader) {
        zipReader.close();
      }
      this._deferred.reject({
        target: this,
        status: e,
        type: "exception"
      });
    }
    return this._deferred.promise;
  }
};







function GMPDownloader(gmpAddon)
{
  this._gmpAddon = gmpAddon;
}






GMPDownloader.computeHash = function(hashFunctionName, fileToHash) {
  let log = getScopedLogger("GMPDownloader.computeHash");
  let digest;
  let fileStream = Cc["@mozilla.org/network/file-input-stream;1"].
                   createInstance(Ci.nsIFileInputStream);
  fileStream.init(fileToHash, FileUtils.MODE_RDONLY,
                  FileUtils.PERMS_FILE, 0);
  try {
    let hash = Cc["@mozilla.org/security/hash;1"].
               createInstance(Ci.nsICryptoHash);
    let hashFunction =
      Ci.nsICryptoHash[hashFunctionName.toUpperCase()];
    if (!hashFunction) {
      log.error("could not get hash function");
      return Promise.reject();
    }
    hash.init(hashFunction);
    hash.updateFromStream(fileStream, -1);
    digest = binaryToHex(hash.finish(false));
  } catch (e) {
    log.warn("failed to compute hash: " + e);
    digest = "";
  }
  fileStream.close();
  return Promise.resolve(digest);
},
GMPDownloader.prototype = {
  




  start: function() {
    let log = getScopedLogger("GMPDownloader.start");
    this._deferred = Promise.defer();
    if (!this._gmpAddon.isValid) {
      log.info("gmpAddon is not valid, will not continue");
      return Promise.reject({
        target: this,
        status: status,
        type: "downloaderr"
      });
    }

    let uri = Services.io.newURI(this._gmpAddon.URL, null, null);
    this._request = Cc["@mozilla.org/network/incremental-download;1"].
                    createInstance(Ci.nsIIncrementalDownload);
    let gmpFile = FileUtils.getFile("TmpD", [this._gmpAddon.id + ".zip"]);
    if (gmpFile.exists()) {
      gmpFile.remove(false);
    }

    log.info("downloading from " + uri.spec + " to " + gmpFile.path);
    this._request.init(uri, gmpFile, DOWNLOAD_CHUNK_BYTES_SIZE,
                       DOWNLOAD_INTERVAL);
    this._request.start(this, null);
    return this._deferred.promise;
  },
  
  onStartRequest: function(request, context) {
  },
  
  
  onStopRequest: function(request, context, status) {
    let log = getScopedLogger("GMPDownloader.onStopRequest");
    log.info("onStopRequest called");
    if (!Components.isSuccessCode(status)) {
      log.info("status failed: " + status);
      this._deferred.reject({
        target: this,
        status: status,
        type: "downloaderr"
      });
      return;
    }

    let promise = this._verifyDownload();
    promise.then(() => {
      log.info("GMP file is ready to unzip");
      let destination = this._request.destination;

      let zipPath = destination.path;
      let gmpAddon = this._gmpAddon;
      let installToDirPath = Cc["@mozilla.org/file/local;1"].
                          createInstance(Ci.nsIFile);
      let path = OS.Path.join(OS.Constants.Path.profileDir,
                              gmpAddon.id,
                              gmpAddon.version);
      installToDirPath.initWithPath(path);
      log.info("install to directory path: " + installToDirPath.path);
      let gmpInstaller = new GMPExtractor(zipPath, installToDirPath.path);
      let installPromise = gmpInstaller.install();
      installPromise.then(extractedPaths => {
        
        let now = Math.round(Date.now() / 1000);
        GMPPrefs.set(GMPPrefs.KEY_PLUGIN_LAST_UPDATE, now, gmpAddon.id);
        
        
        GMPPrefs.set(GMPPrefs.KEY_PLUGIN_VERSION, gmpAddon.version,
                     gmpAddon.id);
        this._deferred.resolve(extractedPaths);
      }, err => {
        this._deferred.reject(err);
      });
    }, err => {
      log.warn("verifyDownload check failed");
      this._deferred.reject({
        target: this,
        status: 200,
        type: "verifyerr"
      });
    });
  },
  



  _verifyDownload: function() {
    let verifyDownloadDeferred = Promise.defer();
    let log = getScopedLogger("GMPDownloader._verifyDownload");
    log.info("_verifyDownload called");
    if (!this._request) {
      return Promise.reject();
    }

    let destination = this._request.destination;
    log.info("for path: " + destination.path);

    
    if (this._gmpAddon.size !== undefined &&
        destination.fileSize != this._gmpAddon.size) {
      log.warn("Downloader:_verifyDownload downloaded size " +
               destination.fileSize + " != expected size " +
               this._gmpAddon.size + ".");
      return Promise.reject();
    }

    let promise = GMPDownloader.computeHash(this._gmpAddon.hashFunction, destination);
    promise.then(digest => {
        let expectedDigest = this._gmpAddon.hashValue.toLowerCase();
        if (digest !== expectedDigest) {
          log.warn("hashes do not match! Got: `" +
                   digest + "`, expected: `" + expectedDigest +  "`");
          this._deferred.reject();
          return;
        }

        log.info("hashes match!");
        verifyDownloadDeferred.resolve();
    }, err => {
        verifyDownloadDeferred.reject();
    });
    return verifyDownloadDeferred.promise;
  },
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIRequestObserver])
};




function binaryToHex(input) {
  let result = "";
  for (let i = 0; i < input.length; ++i) {
    let hex = input.charCodeAt(i).toString(16);
    if (hex.length == 1)
      hex = "0" + hex;
    result += hex;
  }
  return result;
}
