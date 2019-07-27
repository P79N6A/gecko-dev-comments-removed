

 



"use strict";








Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource:///modules/MigrationUtils.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PlacesBackups",
                                  "resource://gre/modules/PlacesBackups.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "SessionMigration",
                                  "resource:///modules/sessionstore/SessionMigration.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "OS",
                                  "resource://gre/modules/osfile.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "FileUtils",
                                  "resource://gre/modules/FileUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "ProfileTimesAccessor",
                                  "resource://gre/modules/services/healthreport/profile.jsm");


function FirefoxProfileMigrator() {
  this.wrappedJSObject = this; 
}

FirefoxProfileMigrator.prototype = Object.create(MigratorPrototype);

FirefoxProfileMigrator.prototype._getFileObject = function(dir, fileName) {
  let file = dir.clone();
  file.append(fileName);

  
  
  
  return file.exists() ? file : null;
}

FirefoxProfileMigrator.prototype.getResources = function() {
  
  
  let sourceProfile =
    Components.classes["@mozilla.org/toolkit/profile-service;1"]
              .getService(Components.interfaces.nsIToolkitProfileService)
              .selectedProfile;
  if (!sourceProfile)
    return null;

  let sourceProfileDir = sourceProfile.rootDir;
  if (!sourceProfileDir || !sourceProfileDir.exists() ||
      !sourceProfileDir.isReadable())
    return null;

  
  
  let currentProfileDir = MigrationUtils.profileStartup.directory;

  
  if (sourceProfileDir.equals(currentProfileDir))
    return null;

  return this._getResourcesInternal(sourceProfileDir, currentProfileDir);
}

FirefoxProfileMigrator.prototype._getResourcesInternal = function(sourceProfileDir, currentProfileDir) {
  let getFileResource = function(aMigrationType, aFileNames) {
    let files = [];
    for (let fileName of aFileNames) {
      let file = this._getFileObject(sourceProfileDir, fileName);
      if (file)
        files.push(file);
    }
    if (!files.length) {
      return null;
    }
    return {
      type: aMigrationType,
      migrate: function(aCallback) {
        for (let file of files) {
          file.copyTo(currentProfileDir, "");
        }
        aCallback(true);
      }
    };
  }.bind(this);

  let types = MigrationUtils.resourceTypes;
  let places = getFileResource(types.HISTORY, ["places.sqlite"]);
  let cookies = getFileResource(types.COOKIES, ["cookies.sqlite"]);
  let passwords = getFileResource(types.PASSWORDS,
                                  ["signons.sqlite", "logins.json", "key3.db"]);
  let formData = getFileResource(types.FORMDATA, ["formhistory.sqlite"]);
  let bookmarksBackups = getFileResource(types.OTHERDATA,
    [PlacesBackups.profileRelativeFolderPath]);
  let dictionary = getFileResource(types.OTHERDATA, ["persdict.dat"]);

  let sessionCheckpoints = this._getFileObject(sourceProfileDir, "sessionCheckpoints.json");
  let sessionFile = this._getFileObject(sourceProfileDir, "sessionstore.js");
  let session;
  if (sessionFile) {
    session = {
      type: types.SESSION,
      migrate: function(aCallback) {
        sessionCheckpoints.copyTo(currentProfileDir, "sessionCheckpoints.json");
        let newSessionFile = currentProfileDir.clone();
        newSessionFile.append("sessionstore.js");
        let migrationPromise = SessionMigration.migrate(sessionFile.path, newSessionFile.path);
        migrationPromise.then(function() {
          let buildID = Services.appinfo.platformBuildID;
          let mstone = Services.appinfo.platformVersion;
          
          Services.prefs.setBoolPref("browser.sessionstore.resume_session_once", true);
          
          
          Services.prefs.setCharPref("browser.startup.homepage_override.mstone", mstone);
          Services.prefs.setCharPref("browser.startup.homepage_override.buildID", buildID);
          
          
          let newPrefsFile = currentProfileDir.clone();
          newPrefsFile.append("prefs.js");
          Services.prefs.savePrefFile(newPrefsFile);
          aCallback(true);
        }, function() {
          aCallback(false);
        });
      }
    }
  }

  
  let times = {
    name: "times", 
    type: types.OTHERDATA,
    migrate: aCallback => {
      let file = this._getFileObject(sourceProfileDir, "times.json");
      if (file) {
        file.copyTo(currentProfileDir, "");
      }
      
      let timesAccessor = new ProfileTimesAccessor(currentProfileDir.path);
      timesAccessor.recordProfileReset().then(
        () => aCallback(true),
        () => aCallback(false)
      );
    }
  };
  let healthReporter = {
    name: "healthreporter", 
    type: types.OTHERDATA,
    migrate: aCallback => {
      
      

      
      const DEFAULT_DATABASE_NAME = "healthreport.sqlite";
      let path = OS.Path.join(sourceProfileDir.path, DEFAULT_DATABASE_NAME);
      let sqliteFile = FileUtils.File(path);
      if (sqliteFile.exists()) {
        sqliteFile.copyTo(currentProfileDir, "");
      }
      
      
      
      
      
      
      
      
      path = OS.Path.join(sourceProfileDir.path, DEFAULT_DATABASE_NAME + "-wal");
      let sqliteWal = FileUtils.File(path);
      if (sqliteWal.exists()) {
        sqliteWal.copyTo(currentProfileDir, "");
      }

      
      let subdir = this._getFileObject(sourceProfileDir, "healthreport");
      if (subdir && subdir.isDirectory()) {
        
        let dest = currentProfileDir.clone();
        dest.append("healthreport");
        dest.create(Components.interfaces.nsIFile.DIRECTORY_TYPE,
                    FileUtils.PERMS_DIRECTORY);
        let enumerator = subdir.directoryEntries;
        while (enumerator.hasMoreElements()) {
          let file = enumerator.getNext().QueryInterface(Components.interfaces.nsIFile);
          if (file.isDirectory()) {
            continue;
          }
          file.copyTo(dest, "");
        }
      }
      
      subdir = this._getFileObject(sourceProfileDir, "datareporting");
      if (subdir && subdir.isDirectory()) {
        let stateFile = this._getFileObject(subdir, "state.json");
        if (stateFile) {
          let dest = currentProfileDir.clone();
          dest.append("datareporting");
          dest.create(Components.interfaces.nsIFile.DIRECTORY_TYPE,
                      FileUtils.PERMS_DIRECTORY);
          stateFile.copyTo(dest, "");
        }
      }
      aCallback(true);
    }
  }

  return [r for each (r in [places, cookies, passwords, formData,
                            dictionary, bookmarksBackups, session,
                            times, healthReporter]) if (r)];
}

Object.defineProperty(FirefoxProfileMigrator.prototype, "startupOnlyMigrator", {
  get: function() true
});


FirefoxProfileMigrator.prototype.classDescription = "Firefox Profile Migrator";
FirefoxProfileMigrator.prototype.contractID = "@mozilla.org/profile/migrator;1?app=browser&type=firefox";
FirefoxProfileMigrator.prototype.classID = Components.ID("{91185366-ba97-4438-acba-48deaca63386}");

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([FirefoxProfileMigrator]);
