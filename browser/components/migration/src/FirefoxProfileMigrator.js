











const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;
const MIGRATOR = Ci.nsIBrowserProfileMigrator;

const LOCAL_FILE_CID = "@mozilla.org/file/local;1";

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");
Components.utils.import("resource://gre/modules/PlacesUtils.jsm");
Components.utils.import("resource://gre/modules/NetUtil.jsm");
Components.utils.import("resource://gre/modules/FileUtils.jsm");

function FirefoxProfileMigrator()
{
  
  this._paths.currentProfile = FileUtils.getDir("ProfDS", []);
}

FirefoxProfileMigrator.prototype = {
  _paths: {
    bookmarks : null,
    cookies : null,
    currentProfile: null,    
    encryptionKey: null,
    history : null,
    passwords: null,
  },

  _homepageURL : null,
  _replaceBookmarks : false,
  _sourceProfile: null,
  _profilesCache: null,

  





  _notifyStart : function Firefox_notifyStart(aType)
  {
    Services.obs.notifyObservers(null, "Migration:ItemBeforeMigrate", aType);
    this._pendingCount++;
  },

  





  _notifyError : function Firefox_notifyError(aType)
  {
    Services.obs.notifyObservers(null, "Migration:ItemError", aType);
  },

  






  _notifyCompleted : function Firefox_notifyIfCompleted(aType)
  {
    Services.obs.notifyObservers(null, "Migration:ItemAfterMigrate", aType);
    if (--this._pendingCount == 0) {
      
      Services.obs.notifyObservers(null, "Migration:Ended", null);
    }
  },

  



  get _bookmarks_backup_folder()
  {
    let bookmarksBackupRelativePath = PlacesUtils.backups.profileRelativeFolderPath;
    let bookmarksBackupDir = this._sourceProfile.clone().QueryInterface(Ci.nsILocalFile);
    bookmarksBackupDir.appendRelativePath(bookmarksBackupRelativePath);
    return bookmarksBackupDir;
  },

  


  _migrateBookmarks : function Firefox_migrateBookmarks()
  {
    this._notifyStart(MIGRATOR.BOOKMARKS);

    try {
      let srcBackupsDir = this._bookmarks_backup_folder;
      let backupFolder = this._paths.currentProfile.clone();
      backupFolder.append(srcBackupsDir.leafName);
      if (!backupFolder.exists())
        srcBackupsDir.copyTo(this._paths.currentProfile, null);
    } catch (e) {
      Cu.reportError(e);
      
      
    } finally {
      this._notifyCompleted(MIGRATOR.BOOKMARKS);
    }
  },

  


  _migrateHistory : function Firefox_migrateHistory()
  {
    this._notifyStart(MIGRATOR.HISTORY);

    try {
      
      let file = Cc[LOCAL_FILE_CID].createInstance(Ci.nsILocalFile);
      file.initWithPath(this._paths.history);
      file.copyTo(this._paths.currentProfile, null);
    } catch (e) {
      Cu.reportError(e);
      this._notifyError(MIGRATOR.HISTORY);
    } finally {
      this._notifyCompleted(MIGRATOR.HISTORY);
    }
  },

  





  _migrateCookies : function Firefox_migrateCookies()
  {
    this._notifyStart(MIGRATOR.COOKIES);

    try {
      
      let file = Cc[LOCAL_FILE_CID].createInstance(Ci.nsILocalFile);
      file.initWithPath(this._paths.cookies);
      file.copyTo(this._paths.currentProfile, null);
    } catch (e) {
      Cu.reportError(e);
      this._notifyError(MIGRATOR.COOKIES);
    } finally {
      this._notifyCompleted(MIGRATOR.COOKIES);
    }
  },

  


  _migratePasswords : function Firefox_migratePasswords()
  {
    this._notifyStart(MIGRATOR.PASSWORDS);

    try {
      
      let file = Cc[LOCAL_FILE_CID].createInstance(Ci.nsILocalFile);
      file.initWithPath(this._paths.passwords);
      file.copyTo(this._paths.currentProfile, null);

      let encryptionKey = Cc[LOCAL_FILE_CID].createInstance(Ci.nsILocalFile);
      encryptionKey.initWithPath(this._paths.encryptionKey);
      encryptionKey.copyTo(this._paths.currentProfile, null);
    } catch (e) {
      Cu.reportError(e);
      this._notifyError(MIGRATOR.PASSWORDS);
    } finally {
      this._notifyCompleted(MIGRATOR.PASSWORDS);
    }
  },


  



  









  migrate : function Firefox_migrate(aItems, aStartup, aProfile)
  {
    if (aStartup) {
      aStartup.doStartup();
      this._replaceBookmarks = true;
    }

    Services.obs.notifyObservers(null, "Migration:Started", null);

    
    
    this._pendingCount = 1;

    if (aItems & MIGRATOR.HISTORY)
      this._migrateHistory();

    if (aItems & MIGRATOR.COOKIES)
      this._migrateCookies();

    if (aItems & MIGRATOR.BOOKMARKS)
      this._migrateBookmarks();

    if (aItems & MIGRATOR.PASSWORDS)
      this._migratePasswords();

    if (--this._pendingCount == 0) {
      
      
      
      Services.obs.notifyObservers(null, "Migration:Ended", null);
    }
  },

  










  getMigrateData: function Firefox_getMigrateData(aProfile, aDoingStartup)
  {
    this._sourceProfile = Cc[LOCAL_FILE_CID].createInstance(Ci.nsILocalFile);
    this._sourceProfile.initWithPath(aProfile);

    let result = 0;
    if (!this._sourceProfile.exists() || !this._sourceProfile.isReadable()) {
      Cu.reportError("source profile directory doesn't exist or is not readable");
      return result;
    }

    
    if (!aDoingStartup)
      return result;

    
    try {
      let file = this._bookmarks_backup_folder;
      if (file.exists()) {
        this._paths.bookmarks = file.path;
        result += MIGRATOR.BOOKMARKS;
      }
    } catch (e) {
      Cu.reportError(e);
    }

    
    try {
      let file = this._sourceProfile.clone();
      file.append("places.sqlite");
      if (file.exists()) {
        this._paths.history = file.path;
        result += MIGRATOR.HISTORY;
        result |= MIGRATOR.BOOKMARKS;
      }
    } catch (e) {
      Cu.reportError(e);
    }

    
    try {
      let file = this._sourceProfile.clone();
      file.append("cookies.sqlite");
      if (file.exists()) {
        this._paths.cookies = file.path;
        result += MIGRATOR.COOKIES;
      }
    } catch (e) {
      Cu.reportError(e);
    }

    
    try {
      let passwords = this._sourceProfile.clone();
      passwords.append("signons.sqlite");
      let encryptionKey = this._sourceProfile.clone();
      encryptionKey.append("key3.db");
      if (passwords.exists() && encryptionKey.exists()) {
        this._paths.passwords = passwords.path;
        this._paths.encryptionKey = encryptionKey.path;
        result += MIGRATOR.PASSWORDS;
      }
    } catch (e) {
      Cu.reportError(e);
    }

    return result;
  },

  




  get sourceExists()
  {
    let userData = Services.dirsvc.get("DefProfRt", Ci.nsIFile).path;
    let result = 0;
    try {
      let userDataDir = Cc[LOCAL_FILE_CID].createInstance(Ci.nsILocalFile);
      userDataDir.initWithPath(userData);
      if (!userDataDir.exists() || !userDataDir.isReadable())
        return false;

      let profiles = this.sourceProfiles;
      if (profiles.length < 1)
        return false;
      
      
      for (let i = 0; i < profiles.length; i++) {
        result = this.getMigrateData(profiles.queryElementAt(i, Ci.nsISupportsString), true);
        if (result)
          break;
      }
    } catch (e) {
      Cu.reportError(e);
    }
    return result > 0;
  },

  get sourceHasMultipleProfiles()
  {
    return this.sourceProfiles.length > 1;
  },

  get sourceProfiles()
  {
    try
    {
      if (!this._profilesCache)
      {
        this._profilesCache = Cc["@mozilla.org/array;1"].createInstance(Ci.nsIMutableArray);
        let profileService = Cc["@mozilla.org/toolkit/profile-service;1"]
                               .getService(Ci.nsIToolkitProfileService);
        
        
        var profile = profileService.selectedProfile;
        if (profile.rootDir.path === this._paths.currentProfile.path)
          return null;

        let str = Cc["@mozilla.org/supports-string;1"].createInstance(Ci.nsISupportsString);
        str.data = profile.rootDir.path;
        this._profilesCache.appendElement(str, false);
      }
    } catch (e) {
      Cu.reportError("Error detecting Firefox profiles: " + e);
    }
    return this._profilesCache;
  },

  






  get sourceHomePageURL()
  {
    try  {
      if (this._homepageURL)
        return this._homepageURL;
    } catch (e) {
      Cu.reportError(e);
    }
    return "";
  },

  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsIBrowserProfileMigrator
  ]),

  classDescription: "Firefox Profile Migrator",
  contractID: "@mozilla.org/profile/migrator;1?app=browser&type=firefox",
  classID: Components.ID("{91185366-ba97-4438-acba-48deaca63386}")
};

const NSGetFactory = XPCOMUtils.generateNSGetFactory([FirefoxProfileMigrator]);
