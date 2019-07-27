



"use strict";

const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/AppsUtils.jsm");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/Webapps.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "FileUtils",
  "resource://gre/modules/FileUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "NetUtil",
  "resource://gre/modules/NetUtil.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "PermissionsInstaller",
  "resource://gre/modules/PermissionsInstaller.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Task",
  "resource://gre/modules/Task.jsm");

this.EXPORTED_SYMBOLS = ["ImportExport"];

const kAppArchiveMimeType  = "application/openwebapp+zip";
const kAppArchiveExtension = ".wpk"; 
const kAppArchiveVersion   = 1;


const PR_RDWR        = 0x04;
const PR_CREATE_FILE = 0x08;
const PR_TRUNCATE    = 0x20;

function debug(aMsg) {
#ifdef DEBUG
  dump("-*- ImportExport.jsm : " + aMsg + "\n");
#endif
}








function readObjectFromZip(aZipReader, aPath) {
  if (!aZipReader.hasEntry(aPath)) {
    debug("ZIP doesn't have entry " + aPath);
    return;
  }

  let istream = aZipReader.getInputStream(aPath);

  
  let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"]
                    .createInstance(Ci.nsIScriptableUnicodeConverter);
  converter.charset = "UTF-8";

  let res;
  try {
    res = JSON.parse(converter.ConvertToUnicode(
      NetUtil.readInputStreamToString(istream, istream.available()) || ""));
  } catch(e) {
    debug("error reading " + aPath + " from ZIP: " + e);
  }
  return res;
}

this.ImportExport = {
  getUUID: function() {
    let uuidGenerator = Cc["@mozilla.org/uuid-generator;1"]
                          .getService(Ci.nsIUUIDGenerator);
    return uuidGenerator.generateUUID().toString();
  },

  
  
  
  
  
  export: Task.async(function*(aApp) {
    if (!aApp) {
      
      throw "NoSuchApp";
    }

    debug("Exporting " + aApp.manifestURL);

    if (aApp.installState != "installed") {
      throw "AppNotFullyInstalled";
    }

    
    
    if (aApp.appStatus == Ci.nsIPrincipal.APP_STATUS_CERTIFIED) {
      throw "CertifiedAppExportForbidden";
    }

    
    let meta = {
      installOrigin: aApp.InstallOrigin,
      manifestURL: aApp.manifestURL,
      version: kAppArchiveVersion
    };

    

    debug("Adding files from " + aApp.basePath + "/" + aApp.id);
    let dir = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsIFile);
    dir.initWithPath(aApp.basePath);
    dir.append(aApp.id);
    if (!dir.exists() || !dir.isDirectory()) {
      throw "NoAppDirectory";
    }

    let files = [];
    if (aApp.origin.startsWith("app://")) {
      files.push("update.webapp");
      files.push("application.zip");
    } else {
      files.push("manifest.webapp");
    }

    
    
    let zipWriter = Cc["@mozilla.org/zipwriter;1"]
                      .createInstance(Ci.nsIZipWriter);
    let uuid = this.getUUID();

    
    
    
    let zipFile = FileUtils.getFile("TmpD",
      ["mozilla-temp-files", uuid + kAppArchiveExtension]);
    debug("Creating archive " + zipFile.path);

    zipWriter.open(zipFile, PR_RDWR | PR_CREATE_FILE | PR_TRUNCATE);

    let blob;

    try {
      debug("Adding metadata.json to exported blob.");
      let stream = Cc["@mozilla.org/io/string-input-stream;1"]
                     .createInstance(Ci.nsIStringInputStream);
      let s = JSON.stringify(meta);
      stream.setData(s, s.length);
      zipWriter.addEntryStream("metadata.json", Date.now(),
                               Ci.nsIZipWriter.COMPRESSION_DEFAULT,
                               stream, false);

      files.forEach((aName) => {
        let file = dir.clone();
        file.append(aName);
        debug("Adding " + file.leafName + " to export blob.");
        zipWriter.addEntryFile(file.leafName,
                               Ci.nsIZipWriter.COMPRESSION_DEFAULT,
                               file, false);
      });

      zipWriter.close();
      
      
      
      let win = Services.wm.getMostRecentWindow("navigator:browser");
      if (!win) {
        throw "NoWindowAvailable";
      }
      blob = new win.File(zipFile, { name: aApp.id + kAppArchiveExtension,
                                     type: kAppArchiveMimeType,
                                     temporary: true });
    } catch(e) {
      debug("Error: " + e);
      zipWriter.close();
      zipFile.remove(false);
      throw "ZipWriteError";
    }

    return blob;
  }),

  
  _importHostedApp: function(aZipReader, aManifestURL) {
    debug("Importing hosted app " + aManifestURL);

    if (!aZipReader.hasEntry("manifest.webapp")) {
      throw "NoManifestFound";
    }

    let manifest = readObjectFromZip(aZipReader, "manifest.webapp");
    if (!manifest) {
      throw "NoManifestFound";
    }

    return manifest;
  },

  
  _importPackagedApp: function(aZipReader, aManifestURL, aDir) {
    debug("Importing packaged app " + aManifestURL);

    if (!aZipReader.hasEntry("update.webapp")) {
      throw "NoUpdateManifestFound";
    }

    if (!aZipReader.hasEntry("application.zip")) {
      throw "NoPackageFound";
    }

    
    
    let file;
    ["update.webapp", "application.zip"].forEach((aName) => {
      file = aDir.clone();
      file.append(aName);
      aZipReader.extract(aName, file);
    });

    
    let appZipReader = Cc["@mozilla.org/libjar/zip-reader;1"]
                         .createInstance(Ci.nsIZipReader);
    appZipReader.open(file);
    if (!appZipReader.hasEntry("manifest.webapp")) {
      throw "NoManifestFound";
    }

    return [readObjectFromZip(appZipReader, "manifest.webapp"), file];
  },

  
  
  
  
  
  
  import: Task.async(function*(aBlob) {

    
    if (!aBlob || !aBlob instanceof Ci.nsIDOMBlob) {
      throw "NoBlobFound";
    }

    let isFile = aBlob instanceof Ci.nsIDOMFile;
    if (!isFile) {
      
      throw "UnsupportedBlobArchive";
    }

    
    let zipFile = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsIFile);
    zipFile.initWithPath(aBlob.mozFullPath);

    debug("Importing from " + zipFile.path);

    let meta;
    let appDir;
    let manifest;
    let zipReader = Cc["@mozilla.org/libjar/zip-reader;1"]
                      .createInstance(Ci.nsIZipReader);
    zipReader.open(zipFile);
    try {
      
      if (!zipReader.hasEntry("metadata.json")) {
        throw "MissingMetadataFile";
      }

      meta = readObjectFromZip(zipReader, "metadata.json");
      if (!meta) {
        throw "NoMetadata";
      }
      debug("metadata: " + uneval(meta));

      
      if (meta.version !== 1) {
        throw "IncorrectVersion";
      }

      
      let app = DOMApplicationRegistry.getAppByManifestURL(meta.manifestURL);
      if (app) {
        throw "AppAlreadyInstalled";
      }

      
      
      meta.localId = DOMApplicationRegistry._nextLocalId();
      meta.id = this.getUUID();
      meta.basePath = FileUtils.getDir(DOMApplicationRegistry.dirKey,
                                       ["webapps"], false).path;

      appDir = FileUtils.getDir(DOMApplicationRegistry.dirKey,
                                ["webapps", meta.id], true);

      let isPackage = zipReader.hasEntry("application.zip");

      let appFile;

      if (isPackage) {
        [manifest, appFile] =
          this._importPackagedApp(zipReader, meta.manifestURL, appDir);
      } else {
        manifest = this._importHostedApp(zipReader, meta.manifestURL);
      }

      if (!AppsUtils.checkManifest(manifest)) {
        throw "InvalidManifest";
      }

      let manifestFile = appDir.clone();
      manifestFile.append("manifest.webapp");

      let manifestString = JSON.stringify(manifest);

      
      
      yield DOMApplicationRegistry._writeFile(manifestFile.path,
                                              manifestString)
      .then(() => { debug("Manifest saved."); },
            aError => { debug("Error saving manifest: " + aError )});

      
      
      
      meta.name = manifest.name;
      meta.csp = manifest.csp;
      meta.installTime = Date.now();
      meta.removable = true;
      meta.progress = 0.0;
      meta.installState = "installed";
      meta.downloadAvailable = false;
      meta.downloading = false;
      meta.readyToApplyDownload = false;
      meta.downloadSize = 0;
      meta.lastUpdateCheck = Date.now();
      meta.updateTime = Date.now();
      meta.manifestHash = AppsUtils.computeHash(manifestString);
      meta.installerAppId = Ci.nsIScriptSecurityManager.NO_APP_ID;
      meta.installerIsBrowser = false;
      meta.role = manifest.role;

      
      if (isPackage) {
        meta.origin = "app://" + meta.id;
        
        
        let [reader, isSigned] =
          yield DOMApplicationRegistry._openPackage(appFile, meta, false);
        let maxStatus = isSigned ? Ci.nsIPrincipal.APP_STATUS_PRIVILEGED
                                 : Ci.nsIPrincipal.APP_STATUS_INSTALLED;
        meta.appStatus = AppsUtils.getAppManifestStatus(manifest);
        debug("Signed app? " + isSigned);
        if (meta.appStatus > maxStatus) {
          throw "InvalidPrivilegeLevel";
        }

        
        
        if (isSigned &&
            meta.appStatus == Ci.nsIPrincipal.APP_STATUS_PRIVILEGED &&
            manifest.origin) {
          let uri;
          try {
            uri = Services.io.newURI(aManifest.origin, null, null);
          } catch(e) {
            throw "InvalidOrigin";
          }
          if (uri.scheme != "app") {
            throw "InvalidOrigin";
          }
          meta.id = uri.prePath.substring(6); 
          if (meta.id in DOMApplicationRegistry.webapps) {
            throw "DuplicateOrigin";
          }
          
          appDir.moveTo(appDir.parent, meta.id);

          meta.origin = uri.prePath;
        }
      } else {
        let uri = Services.io.newURI(meta.manifestURL, null, null);
        meta.origin = uri.resolve("/");
        meta.appStatus = Ci.nsIPrincipal.APP_STATUS_INSTALLED;
        if (manifest.appcache_path) {
          
          
          meta.installState = "pending";
          meta.downloadAvailable = true;
        }
      }
      meta.kind = DOMApplicationRegistry.appKind(meta, manifest);

      DOMApplicationRegistry.webapps[meta.id] = meta;

      
      PermissionsInstaller.installPermissions(
        {
          origin: meta.origin,
          manifestURL: meta.manifestURL,
          manifest: manifest
        },
        false,
        null);
      DOMApplicationRegistry.updateAppHandlers(null ,
                                               manifest,
                                               meta);

      
      
      yield DOMApplicationRegistry._saveApps();

      app = AppsUtils.cloneAppObject(meta);
      app.manifest = manifest;
      DOMApplicationRegistry.broadcastMessage("Webapps:AddApp",
                                              { id: meta.id, app: app });
      DOMApplicationRegistry.broadcastMessage("Webapps:Install:Return:OK",
                                              { app: app });
      Services.obs.notifyObservers(null, "webapps-installed",
        JSON.stringify({ manifestURL: meta.manifestURL }));

    } catch(e) {
      debug("Import failed: " + e);
      if (appDir && appDir.exists()) {
        appDir.remove(true);
      }
      throw e;
    } finally {
      zipReader.close();
    }

    return [meta.manifestURL, manifest];
  }),

  
  
  
  
  extractManifest: Task.async(function*(aBlob) {
    
    if (!aBlob || !aBlob instanceof Ci.nsIDOMBlob) {
      throw "NoBlobFound";
    }

    let isFile = aBlob instanceof Ci.nsIDOMFile;
    if (!isFile) {
      
      throw "UnsupportedBlobArchive";
    }

    
    let zipFile = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsIFile);
    zipFile.initWithPath(aBlob.mozFullPath);
    debug("extractManifest from " + zipFile.path);

    
    let zipReader = Cc["@mozilla.org/libjar/zip-reader;1"]
                      .createInstance(Ci.nsIZipReader);
    zipReader.open(zipFile);

    let manifest;
    try {
      if (zipReader.hasEntry("manifest.webapp")) {
        manifest = readObjectFromZip(zipReader, "manifest.webapp");
        if (!manifest) {
          throw "NoManifest";
        }
      } else if (zipReader.hasEntry("application.zip")) {
        
        let innerReader = Cc["@mozilla.org/libjar/zip-reader;1"]
                            .createInstance(Ci.nsIZipReader);
        innerReader.openInner(zipReader, "application.zip");
        manifest = readObjectFromZip(innerReader, "manifest.webapp");
        innerReader.close();
        if (!manifest) {
          throw "NoManifest";
        }
      } else {
        throw "MissingManifestFile";
      }
    } finally {
      zipReader.close();
    }

    return manifest;
  }),
};
