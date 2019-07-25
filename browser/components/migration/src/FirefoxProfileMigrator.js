

 



"use strict";








Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource:///modules/MigrationUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PlacesUtils",
                                  "resource://gre/modules/PlacesUtils.jsm");

function FirefoxProfileMigrator() { }

FirefoxProfileMigrator.prototype = Object.create(MigratorPrototype);

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

  let getFileResource = function(aMigrationType, aFileNames) {
    let files = [];
    for (let fileName of aFileNames) {
      let file = sourceProfileDir.clone();
      file.append(fileName);

      
      
      if (!file.exists())
        return null;

      files.push(file);
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
  };

  let types = MigrationUtils.resourceTypes;
  let places = getFileResource(types.HISTORY, ["places.sqlite"]);
  let cookies = getFileResource(types.COOKIES, ["cookies.sqlite"]);
  let passwords = getFileResource(types.PASSWORDS,
                                  ["signons.sqlite", "key3.db"]);
  let formData = getFileResource(types.FORMDATA, ["formhistory.sqlite"]);
  let bookmarksBackups = getFileResource(types.OTHERDATA,
    [PlacesUtils.backups.profileRelativeFolderPath]);

  return [r for each (r in [places, cookies, passwords, formData,
                            bookmarksBackups]) if (r)];
}

Object.defineProperty(FirefoxProfileMigrator.prototype, "startupOnlyMigrator", {
  get: function() true
});


FirefoxProfileMigrator.prototype.classDescription = "Firefox Profile Migrator";
FirefoxProfileMigrator.prototype.contractID = "@mozilla.org/profile/migrator;1?app=browser&type=firefox";
FirefoxProfileMigrator.prototype.classID = Components.ID("{91185366-ba97-4438-acba-48deaca63386}");

const NSGetFactory = XPCOMUtils.generateNSGetFactory([FirefoxProfileMigrator]);
