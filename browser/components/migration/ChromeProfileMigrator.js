





"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

const FILE_INPUT_STREAM_CID = "@mozilla.org/network/file-input-stream;1";

const S100NS_FROM1601TO1970 = 0x19DB1DED53E8000;
const S100NS_PER_MS = 10;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/NetUtil.jsm");
Cu.import("resource://gre/modules/FileUtils.jsm");
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource:///modules/MigrationUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "PlacesUtils",
                                  "resource://gre/modules/PlacesUtils.jsm");










function chromeTimeToDate(aTime)
{
  return new Date((aTime * S100NS_PER_MS - S100NS_FROM1601TO1970 ) / 10000);
}









function* insertBookmarkItems(parentGuid, items) {
  for (let item of items) {
    try {
      if (item.type == "url") {
        yield PlacesUtils.bookmarks.insert({
          parentGuid, url: item.url, title: item.name
        });
      } else if (item.type == "folder") {
        let newFolderGuid = (yield PlacesUtils.bookmarks.insert({
          parentGuid, type: PlacesUtils.bookmarks.TYPE_FOLDER, title: item.name
        })).guid;

        yield insertBookmarkItems(newFolderGuid, item.children);
      }
    } catch (e) {
      Cu.reportError(e);
    }
  }
}


function ChromeProfileMigrator() {
  let chromeUserDataFolder = FileUtils.getDir(
#ifdef XP_WIN
    "LocalAppData", ["Google", "Chrome", "User Data"]
#elifdef XP_MACOSX
    "ULibDir", ["Application Support", "Google", "Chrome"]
#else
    "Home", [".config", "google-chrome"]
#endif
    , false);
  this._chromeUserDataFolder = chromeUserDataFolder.exists() ?
    chromeUserDataFolder : null;
}

ChromeProfileMigrator.prototype = Object.create(MigratorPrototype);

ChromeProfileMigrator.prototype.getResources =
  function Chrome_getResources(aProfile) {
    if (this._chromeUserDataFolder) {
      let profileFolder = this._chromeUserDataFolder.clone();
      profileFolder.append(aProfile.id);
      if (profileFolder.exists()) {
        let possibleResources = [GetBookmarksResource(profileFolder),
                                 GetHistoryResource(profileFolder),
                                 GetCookiesResource(profileFolder)];
        return [r for each (r in possibleResources) if (r != null)];
      }
    }
    return [];
  };

Object.defineProperty(ChromeProfileMigrator.prototype, "sourceProfiles", {
  get: function Chrome_sourceProfiles() {
    if ("__sourceProfiles" in this)
      return this.__sourceProfiles;

    if (!this._chromeUserDataFolder)
      return [];

    let profiles = [];
    try {
      
      let localState = this._chromeUserDataFolder.clone();
      localState.append("Local State");
      if (!localState.exists())
        throw new Error("Chrome's 'Local State' file does not exist.");
      if (!localState.isReadable())
        throw new Error("Chrome's 'Local State' file could not be read.");

      let fstream = Cc[FILE_INPUT_STREAM_CID].createInstance(Ci.nsIFileInputStream);
      fstream.init(localState, -1, 0, 0);
      let inputStream = NetUtil.readInputStreamToString(fstream, fstream.available(),
                                                        { charset: "UTF-8" });
      let info_cache = JSON.parse(inputStream).profile.info_cache;
      for (let profileFolderName in info_cache) {
        let profileFolder = this._chromeUserDataFolder.clone();
        profileFolder.append(profileFolderName);
        profiles.push({
          id: profileFolderName,
          name: info_cache[profileFolderName].name || profileFolderName,
        });
      }
    } catch (e) {
      Cu.reportError("Error detecting Chrome profiles: " + e);
      
      let defaultProfileFolder = this._chromeUserDataFolder.clone();
      defaultProfileFolder.append("Default");
      if (defaultProfileFolder.exists()) {
        profiles = [{
          id: "Default",
          name: "Default",
        }];
      }
    }

    
    return this.__sourceProfiles = profiles.filter(function(profile) {
      let resources = this.getResources(profile);
      return resources && resources.length > 0;
    }, this);
  }
});

Object.defineProperty(ChromeProfileMigrator.prototype, "sourceHomePageURL", {
  get: function Chrome_sourceHomePageURL() {
    let prefsFile = this._chromeUserDataFolder.clone();
    prefsFile.append("Preferences");
    if (prefsFile.exists()) {
      
      let fstream = Cc[FILE_INPUT_STREAM_CID].
                    createInstance(Ci.nsIFileInputStream);
      fstream.init(file, -1, 0, 0);
      try {
        return JSON.parse(
          NetUtil.readInputStreamToString(fstream, fstream.available(),
                                          { charset: "UTF-8" })
            ).homepage;
      }
      catch(e) {
        Cu.reportError("Error parsing Chrome's preferences file: " + e);
      }
    }
    return "";
  }
});

function GetBookmarksResource(aProfileFolder) {
  let bookmarksFile = aProfileFolder.clone();
  bookmarksFile.append("Bookmarks");
  if (!bookmarksFile.exists())
    return null;

  return {
    type: MigrationUtils.resourceTypes.BOOKMARKS,

    migrate: function(aCallback) {
      return Task.spawn(function* () {
        let jsonStream = yield new Promise(resolve =>
          NetUtil.asyncFetch({ uri: NetUtil.newURI(bookmarksFile),
                               loadUsingSystemPrincipal: true
                             },
                             (inputStream, resultCode) => {
                               if (Components.isSuccessCode(resultCode)) {
                                 resolve(inputStream);
                               } else {
                                 reject(new Error("Could not read Bookmarks file"));
                               }
                             }
          )
        );

        
        let bookmarkJSON = NetUtil.readInputStreamToString(
          jsonStream, jsonStream.available(), { charset : "UTF-8" });
        let roots = JSON.parse(bookmarkJSON).roots;

        
        if (roots.bookmark_bar.children &&
            roots.bookmark_bar.children.length > 0) {
          
          let parentGuid = PlacesUtils.bookmarks.toolbarGuid;
          if (!MigrationUtils.isStartupMigration) {
            parentGuid =
              yield MigrationUtils.createImportedBookmarksFolder("Chrome", parentGuid);
          }
          yield insertBookmarkItems(parentGuid, roots.bookmark_bar.children);
        }

        
        if (roots.other.children &&
            roots.other.children.length > 0) {
          
          let parentGuid = PlacesUtils.bookmarks.menuGuid;
          if (!MigrationUtils.isStartupMigration) {
            parentGuid =
              yield MigrationUtils.createImportedBookmarksFolder("Chrome", parentGuid);
          }
          yield insertBookmarkItems(parentGuid, roots.other.children);
        }
      }.bind(this)).then(() => aCallback(true),
                          e => { Cu.reportError(e); aCallback(false) });
    }
  };
}

function GetHistoryResource(aProfileFolder) {
  let historyFile = aProfileFolder.clone();
  historyFile.append("History");
  if (!historyFile.exists())
    return null;

  return {
    type: MigrationUtils.resourceTypes.HISTORY,

    migrate: function(aCallback) {
      let dbConn = Services.storage.openUnsharedDatabase(historyFile);
      let stmt = dbConn.createAsyncStatement(
        "SELECT url, title, last_visit_time, typed_count FROM urls WHERE hidden = 0");

      stmt.executeAsync({
        handleResult : function(aResults) {
          let places = [];
          for (let row = aResults.getNextRow(); row; row = aResults.getNextRow()) {
            try {
              
              let transType = PlacesUtils.history.TRANSITION_LINK;
              if (row.getResultByName("typed_count") > 0)
                transType = PlacesUtils.history.TRANSITION_TYPED;

              places.push({
                uri: NetUtil.newURI(row.getResultByName("url")),
                title: row.getResultByName("title"),
                visits: [{
                  transitionType: transType,
                  visitDate: chromeTimeToDate(
                               row.getResultByName(
                                 "last_visit_time")) * 1000,
                }],
              });
            } catch (e) {
              Cu.reportError(e);
            }
          }

          try {
            PlacesUtils.asyncHistory.updatePlaces(places);
          } catch (e) {
            Cu.reportError(e);
          }
        },

        handleError : function(aError) {
          Cu.reportError("Async statement execution returned with '" +
                         aError.result + "', '" + aError.message + "'");
        },

        handleCompletion : function(aReason) {
          dbConn.asyncClose();
          aCallback(aReason == Ci.mozIStorageStatementCallback.REASON_FINISHED);
        }
      });
      stmt.finalize();
    }
  };
}

function GetCookiesResource(aProfileFolder) {
  let cookiesFile = aProfileFolder.clone();
  cookiesFile.append("Cookies");
  if (!cookiesFile.exists())
    return null;

  return {
    type: MigrationUtils.resourceTypes.COOKIES,

    migrate: function(aCallback) {
      let dbConn = Services.storage.openUnsharedDatabase(cookiesFile);
      let stmt = dbConn.createAsyncStatement(
          "SELECT host_key, path, name, value, secure, httponly, expires_utc FROM cookies");

      stmt.executeAsync({
        handleResult : function(aResults) {
          for (let row = aResults.getNextRow(); row; row = aResults.getNextRow()) {
            let host_key = row.getResultByName("host_key");
            if (host_key.match(/^\./)) {
              
              host_key = host_key.substr(1);
            }

            try {
              let expiresUtc =
                chromeTimeToDate(row.getResultByName("expires_utc")) / 1000;
              Services.cookies.add(host_key,
                                   row.getResultByName("path"),
                                   row.getResultByName("name"),
                                   row.getResultByName("value"),
                                   row.getResultByName("secure"),
                                   row.getResultByName("httponly"),
                                   false,
                                   parseInt(expiresUtc));
            } catch (e) {
              Cu.reportError(e);
            }
          }
        },

        handleError : function(aError) {
          Cu.reportError("Async statement execution returned with '" +
                         aError.result + "', '" + aError.message + "'");
        },

        handleCompletion : function(aReason) {
          dbConn.asyncClose();
          aCallback(aReason == Ci.mozIStorageStatementCallback.REASON_FINISHED);
        },
      });
      stmt.finalize();
    }
  }
}

ChromeProfileMigrator.prototype.classDescription = "Chrome Profile Migrator";
ChromeProfileMigrator.prototype.contractID = "@mozilla.org/profile/migrator;1?app=browser&type=chrome";
ChromeProfileMigrator.prototype.classID = Components.ID("{4cec1de4-1671-4fc3-a53e-6c539dc77a26}");

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([ChromeProfileMigrator]);
