



"use strict";

const { classes: Cc, interfaces: Ci, results: Cr, utils: Cu } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/FileUtils.jsm");
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource:///modules/MigrationUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "PlacesUtils",
                                  "resource://gre/modules/PlacesUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Sqlite",
                                  "resource://gre/modules/Sqlite.jsm");

function parseINIStrings(file) {
  let factory = Cc["@mozilla.org/xpcom/ini-parser-factory;1"].
                getService(Ci.nsIINIParserFactory);
  let parser = factory.createINIParser(file);
  let obj = {};
  let sections = parser.getSections();
  while (sections.hasMore()) {
    let section = sections.getNext();
    obj[section] = {};

    let keys = parser.getKeys(section);
    while (keys.hasMore()) {
      let key = keys.getNext();
      obj[section][key] = parser.getString(section, key);
    }
  }
  return obj;
}

function getHash(aStr) {
  
  function toHexString(charCode)
    ("0" + charCode.toString(16)).slice(-2);

  let hasher = Cc["@mozilla.org/security/hash;1"].
               createInstance(Ci.nsICryptoHash);
  hasher.init(Ci.nsICryptoHash.MD5);
  let stringStream = Cc["@mozilla.org/io/string-input-stream;1"].
                     createInstance(Ci.nsIStringInputStream);
  stringStream.data = aStr;
  hasher.updateFromStream(stringStream, -1);

  
  let binary = hasher.finish(false);
  return [toHexString(binary.charCodeAt(i)) for (i in binary)].join("").toLowerCase();
}

function Bookmarks(aProfileFolder) {
  let file = aProfileFolder.clone();
  file.append("360sefav.db");

  this._file = file;
}
Bookmarks.prototype = {
  type: MigrationUtils.resourceTypes.BOOKMARKS,

  get exists() {
    return this._file.exists() && this._file.isReadable();
  },

  migrate: function (aCallback) {
    return Task.spawn(function* () {
      let idToGuid = new Map();
      let folderGuid = PlacesUtils.bookmarks.toolbarGuid;
      if (!MigrationUtils.isStartupMigration) {
        folderGuid =
          yield MigrationUtils.createImportedBookmarksFolder("360se", folderGuid);
      }
      idToGuid.set(0, folderGuid);

      let connection = yield Sqlite.openConnection({
        path: this._file.path
      });

      try {
        let rows = yield connection.execute(
          `WITH RECURSIVE
           bookmark(id, parent_id, is_folder, title, url, pos) AS (
             VALUES(0, -1, 1, '', '', 0)
             UNION
             SELECT f.id, f.parent_id, f.is_folder, f.title, f.url, f.pos
             FROM tb_fav AS f
             JOIN bookmark AS b ON f.parent_id = b.id
             ORDER BY f.pos ASC
           )
           SELECT id, parent_id, is_folder, title, url FROM bookmark WHERE id`);

        for (let row of rows) {
          let id = parseInt(row.getResultByName("id"), 10),
              parent_id = parseInt(row.getResultByName("parent_id"), 10),
              is_folder = parseInt(row.getResultByName("is_folder"), 10),
              title = row.getResultByName("title"),
              url = row.getResultByName("url");

          let parentGuid = idToGuid.get(parent_id) || idToGuid.get("fallback");
          if (!parentGuid) {
            parentGuid = PlacesUtils.bookmarks.unfiledGuid;
            if (!MigrationUtils.isStartupMigration) {
              parentGuid =
                yield MigrationUtils.createImportedBookmarksFolder("360se", parentGuid);
            }
            idToGuid.set("fallback", parentGuid);
          }

          try {
            if (is_folder == 1) {
              let newFolderGuid = (yield PlacesUtils.bookmarks.insert({
                parentGuid,
                type: PlacesUtils.bookmarks.TYPE_FOLDER,
                title
              })).guid;

              idToGuid.set(id, newFolderGuid);
            } else {
              yield PlacesUtils.bookmarks.insert({
                parentGuid,
                url,
                title
              });
            }
          } catch (ex) {
            Cu.reportError(ex);
          }
        }
      } finally {
        yield connection.close();
      }
    }.bind(this)).then(() => aCallback(true),
                        e => { Cu.reportError(e); aCallback(false) });
  }
};

function Qihoo360seProfileMigrator() {
  let paths = [
    
    {
      users: ["360se6", "apps", "data", "users"],
      defaultUser: "default"
    },
    
    {
      users: ["360se"],
      defaultUser: "data"
    }
  ];
  this._usersDir = null;
  this._defaultUserPath = null;
  for (let path of paths) {
    let usersDir = FileUtils.getDir("AppData", path.users, false);
    if (usersDir.exists()) {
      this._usersDir = usersDir;
      this._defaultUserPath = path.defaultUser;
      break;
    }
  }
}

Qihoo360seProfileMigrator.prototype = Object.create(MigratorPrototype);

Object.defineProperty(Qihoo360seProfileMigrator.prototype, "sourceProfiles", {
  get: function() {
    if ("__sourceProfiles" in this)
      return this.__sourceProfiles;

    if (!this._usersDir)
      return this.__sourceProfiles = [];

    let profiles = [];
    let noLoggedInUser = true;
    try {
      let loginIni = this._usersDir.clone();
      loginIni.append("login.ini");
      if (!loginIni.exists()) {
        throw new Error("360 Secure Browser's 'login.ini' does not exist.");
      }
      if (!loginIni.isReadable()) {
        throw new Error("360 Secure Browser's 'login.ini' file could not be read.");
      }
      let loginIniObj = parseINIStrings(loginIni);
      let nowLoginEmail = loginIniObj.NowLogin && loginIniObj.NowLogin.email;

      








      if (nowLoginEmail) {
        if (loginIniObj.NowLogin.IsLogined === "1") {
          noLoggedInUser = false;
        }

        profiles.push({
          id: this._getIdFromConfig(loginIniObj.NowLogin),
          name: nowLoginEmail,
        });
      }

      for (let section in loginIniObj) {
        if (!loginIniObj[section].email ||
            (nowLoginEmail && loginIniObj[section].email == nowLoginEmail)) {
          continue;
        }

        profiles.push({
          id: this._getIdFromConfig(loginIniObj[section]),
          name: loginIniObj[section].email,
        });
      }
    } catch (e) {
      Cu.reportError("Error detecting 360 Secure Browser profiles: " + e);
    } finally {
      profiles[noLoggedInUser ? "unshift" : "push"]({
        id: this._defaultUserPath,
        name: "Default",
      });
    }

    return this.__sourceProfiles = profiles.filter(profile => {
      let resources = this.getResources(profile);
      return resources && resources.length > 0;
    });
  }
});

Qihoo360seProfileMigrator.prototype._getIdFromConfig = function(aConfig) {
  return aConfig.UserMd5 || getHash(aConfig.email);
};

Qihoo360seProfileMigrator.prototype.getResources = function(aProfile) {
  let profileFolder = this._usersDir.clone();
  profileFolder.append(aProfile.id);

  if (!profileFolder.exists()) {
    return [];
  }

  let resources = [
    new Bookmarks(profileFolder)
  ];
  return [r for each (r in resources) if (r.exists)];
};

Qihoo360seProfileMigrator.prototype.classDescription = "360 Secure Browser Profile Migrator";
Qihoo360seProfileMigrator.prototype.contractID = "@mozilla.org/profile/migrator;1?app=browser&type=360se";
Qihoo360seProfileMigrator.prototype.classID = Components.ID("{d0037b95-296a-4a4e-94b2-c3d075d20ab1}");

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([Qihoo360seProfileMigrator]);
