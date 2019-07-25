





































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

const LOCAL_FILE_CID = "@mozilla.org/file/local;1";
const FILE_INPUT_STREAM_CID = "@mozilla.org/network/file-input-stream;1";

const BUNDLE_MIGRATION = "chrome://browser/locale/migration/migration.properties";

const MIGRATE_ALL = 0x0000;
const MIGRATE_SETTINGS = 0x0001;
const MIGRATE_COOKIES = 0x0002;
const MIGRATE_HISTORY = 0x0004;
const MIGRATE_FORMDATA = 0x0008;
const MIGRATE_PASSWORDS = 0x0010;
const MIGRATE_BOOKMARKS = 0x0020;
const MIGRATE_OTHERDATA = 0x0040;

const S100NS_FROM1601TO1970 = 0x19DB1DED53E8000;
const S100NS_PER_MS = 10;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");
Components.utils.import("resource://gre/modules/PlacesUtils.jsm");
Components.utils.import("resource://gre/modules/NetUtil.jsm");

XPCOMUtils.defineLazyGetter(this, "bookmarksSubfolderTitle", function () {
  
  let strbundle =
    Services.strings.createBundle(BUNDLE_MIGRATION);
  let sourceNameChrome = strbundle.GetStringFromName("sourceNameChrome");
  return strbundle.formatStringFromName("importedBookmarksFolder",
                                        [sourceNameChrome],
                                        1);
});










function chromeTimeToDate(aTime)
{
  return new Date((aTime * S100NS_PER_MS - S100NS_FROM1601TO1970 ) / 10000);
}









function insertBookmarkItems(aFolderId, aItems)
{
  for (let i = 0; i < aItems.length; i++) {
    let item = aItems[i];

    try {
      if (item.type == "url") {
        PlacesUtils.bookmarks.insertBookmark(aFolderId,
                                             NetUtil.newURI(item.url),
                                             PlacesUtils.bookmarks.DEFAULT_INDEX,
                                             item.name);
      } else if (item.type == "folder") {
        let newFolderId =
          PlacesUtils.bookmarks.createFolder(aFolderId,
                                             item.name,
                                             PlacesUtils.bookmarks.DEFAULT_INDEX);

        insertBookmarkItems(newFolderId, item.children);
      }
    } catch (e) {
      Cu.reportError(e);
    }
  }
}

function ChromeProfileMigrator()
{
}

ChromeProfileMigrator.prototype = {
  _paths: {
    bookmarks : null,
    cookies : null,
    history : null,
    prefs : null,
  },

  _homepageURL : null,
  _replaceBookmarks : false,

  






  _notifyStart : function Chrome_notifyStart(aType)
  {
    Services.obs.notifyObservers(null, "Migration:ItemBeforeMigrate", aType);
    this._pendingCount++;
  },

  






  _notifyCompleted : function Chrome_notifyIfCompleted(aType)
  {
    Services.obs.notifyObservers(null, "Migration:ItemAfterMigrate", aType);
    if (--this._pendingCount == 0) {
      
      Services.obs.notifyObservers(null, "Migration:Ended", null);
    }
  },

  


  _migrateBookmarks : function Chrome_migrateBookmarks()
  {
    this._notifyStart(MIGRATE_BOOKMARKS);

    try {
      PlacesUtils.bookmarks.runInBatchMode({
        _self : this,
        runBatched : function (aUserData) {
          let migrator = this._self;
          let file = Cc[LOCAL_FILE_CID].createInstance(Ci.nsILocalFile);
          file.initWithPath(migrator._paths.bookmarks);

          NetUtil.asyncFetch(file, function(aInputStream, aResultCode) {
            if (!Components.isSuccessCode(aResultCode)) {
              migrator._notifyCompleted(MIGRATE_BOOKMARKS);
              return;
            }

            
            let bookmarkJSON = NetUtil.readInputStreamToString(aInputStream,
                                                               aInputStream.available(),
                                                               { charset : "UTF-8" });
            let roots = JSON.parse(bookmarkJSON).roots;

            
            if (roots.bookmark_bar.children &&
                roots.bookmark_bar.children.length > 0) {
              
              let parentId = PlacesUtils.toolbarFolderId;
              if (!migrator._replaceBookmarks) { 
                parentId =
                  PlacesUtils.bookmarks.createFolder(parentId,
                                                     bookmarksSubfolderTitle,
                                                     PlacesUtils.bookmarks.DEFAULT_INDEX);
              }
              insertBookmarkItems(parentId, roots.bookmark_bar.children);
            }

            
            if (roots.other.children &&
                roots.other.children.length > 0) {
              
              let parentId = PlacesUtils.bookmarksMenuFolderId;
              if (!migrator._replaceBookmarks) { 
                parentId =
                  PlacesUtils.bookmarks.createFolder(parentId,
                                                     bookmarksSubfolderTitle,
                                                     PlacesUtils.bookmarks.DEFAULT_INDEX);
              }
              insertBookmarkItems(parentId, roots.other.children);
            }

            migrator._notifyCompleted(MIGRATE_BOOKMARKS);
          });
        }
      }, null);
    } catch (e) {
      Cu.reportError(e);
      this._notifyCompleted(MIGRATE_BOOKMARKS);
    }
  },

  


  _migrateHistory : function Chrome_migrateHistory()
  {
    this._notifyStart(MIGRATE_HISTORY);

    try {
      PlacesUtils.history.runInBatchMode({
        _self : this,
        runBatched : function (aUserData) {
          
          let file = Cc[LOCAL_FILE_CID].createInstance(Ci.nsILocalFile);
          file.initWithPath(this._self._paths.history);

          let dbConn = Services.storage.openUnsharedDatabase(file);
          let stmt = dbConn.createAsyncStatement(
              "SELECT url, title, last_visit_time, typed_count FROM urls WHERE hidden = 0");

          stmt.executeAsync({
            _asyncHistory : Cc["@mozilla.org/browser/history;1"]
                            .getService(Ci.mozIAsyncHistory),
            _db : dbConn,
            _self : this._self,
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
                this._asyncHistory.updatePlaces(places);
              } catch (e) {
                Cu.reportError(e);
              }
            },

            handleError : function(aError) {
              Cu.reportError("Async statement execution returned with '" +
                             aError.result + "', '" + aError.message + "'");
            },

            handleCompletion : function(aReason) {
              this._db.asyncClose();
              this._self._notifyCompleted(MIGRATE_HISTORY);
            }
          });
          stmt.finalize();
        }
      }, null);
    } catch (e) {
      Cu.reportError(e);
      this._notifyCompleted(MIGRATE_HISTORY);
    }
  },

  


  _migrateCookies : function Chrome_migrateCookies()
  {
    this._notifyStart(MIGRATE_COOKIES);

    try {
      
      let file = Cc[LOCAL_FILE_CID].createInstance(Ci.nsILocalFile);
      file.initWithPath(this._paths.cookies);

      let dbConn = Services.storage.openUnsharedDatabase(file);
      let stmt = dbConn.createAsyncStatement(
          "SELECT host_key, path, name, value, secure, httponly, expires_utc FROM cookies");

      stmt.executeAsync({
        _db : dbConn,
        _self : this,
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
          this._db.asyncClose();
          this._self._notifyCompleted(MIGRATE_COOKIES);
        },
      });
      stmt.finalize();
    } catch (e) {
      Cu.reportError(e);
      this._notifyCompleted(MIGRATE_COOKIES);
    }
  },

  



  









  migrate : function Chrome_migrate(aItems, aStartup, aProfile)
  {
    if (aStartup) {
      aStartup.doStartup();
      this._replaceBookmarks = true;
    }

    Services.obs.notifyObservers(null, "Migration:Started", null);

    
    
    this._pendingCount = 1;

    if (aItems & MIGRATE_HISTORY)
      this._migrateHistory();

    if (aItems & MIGRATE_COOKIES)
      this._migrateCookies();

    if (aItems & MIGRATE_BOOKMARKS)
      this._migrateBookmarks();

    if (--this._pendingCount == 0) {
      
      
      
      Services.obs.notifyObservers(null, "Migration:Ended", null);
    }
  },

  








  getMigrateData: function Chrome_getMigrateData(aProfile, aDoingStartup)
  {
#ifdef XP_WIN
    let chromepath = Services.dirsvc.get("LocalAppData", Ci.nsIFile).path +
                     "\\Google\\Chrome\\User Data\\Default\\";
#elifdef XP_MACOSX
    let chromepath = Services.dirsvc.get("Home", Ci.nsIFile).path +
                     "/Library/Application Support/Google/Chrome/Default/";
#else
    let chromepath = Services.dirsvc.get("Home", Ci.nsIFile).path +
                     "/.config/google-chrome/Default/";
#endif

    let result = 0;

    

    try {
      let file = Cc[LOCAL_FILE_CID].createInstance(Ci.nsILocalFile);
      file.initWithPath(chromepath + "Bookmarks");
      if (file.exists()) {
        this._paths.bookmarks = file.path
        result += MIGRATE_BOOKMARKS;
      }
    } catch (e) {
      Cu.reportError(e);
    }

    if (!this._paths.prefs)
      this._paths.prefs = chromepath + "Preferences";

    

    try {
      let file = Cc[LOCAL_FILE_CID].createInstance(Ci.nsILocalFile);
      file.initWithPath(chromepath + "History");
      if (file.exists()) {
        this._paths.history = file.path
        result += MIGRATE_HISTORY;
      }
    } catch (e) {
      Cu.reportError(e);
    }

    try {
      let file = Cc[LOCAL_FILE_CID].createInstance(Ci.nsILocalFile);
      file.initWithPath(chromepath + "Cookies");
      if (file.exists()) {
        this._paths.cookies = file.path
        result += MIGRATE_COOKIES;
      }
    } catch (e) {
      Cu.reportError(e);
    }

    return result;
  },

  




  get sourceExists()
  {
    let result = this.getMigrateData(null, false);
    return result != 0;
  },

  
  
  sourceHasMultipleProfiles: false,
  sourceProfiles: null,

  




  get sourceHomePageURL()
  {
    try  {
      if (this._homepageURL)
        return this._homepageURL;

      if (!this._paths.prefs)
        this.getMigrateData(null, false);

      
      let file = Cc[LOCAL_FILE_CID].createInstance(Ci.nsILocalFile);
      file.initWithPath(this._paths.prefs);
      let fstream = Cc[FILE_INPUT_STREAM_CID].
                    createInstance(Ci.nsIFileInputStream);
      fstream.init(file, -1, 0, 0); 
      this._homepageURL = JSON.parse(
        NetUtil.readInputStreamToString(fstream, fstream.available(),
                                        { charset: "UTF-8" })).homepage;
      return this._homepageURL;
    } catch (e) {
      Cu.reportError(e);
    }
    return "";
  },

  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsIBrowserProfileMigrator
  ]),

  classDescription: "Chrome Profile Migrator",
  contractID: "@mozilla.org/profile/migrator;1?app=browser&type=chrome",
  classID: Components.ID("{4cec1de4-1671-4fc3-a53e-6c539dc77a26}")
};

const NSGetFactory = XPCOMUtils.generateNSGetFactory([ChromeProfileMigrator]);
