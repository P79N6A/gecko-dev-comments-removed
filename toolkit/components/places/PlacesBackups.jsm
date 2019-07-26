





this.EXPORTED_SYMBOLS = ["PlacesBackups"];

const Ci = Components.interfaces;
const Cu = Components.utils;
const Cc = Components.classes;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/PlacesUtils.jsm");
Cu.import("resource://gre/modules/BookmarkJSONUtils.jsm");
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/osfile.jsm");
Cu.import("resource://gre/modules/Deprecated.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "OS",
  "resource://gre/modules/osfile.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "FileUtils",
  "resource://gre/modules/FileUtils.jsm");

this.PlacesBackups = {
  get _filenamesRegex() {
    
    
    let localizedFilename =
      PlacesUtils.getFormattedString("bookmarksArchiveFilename", [new Date()]);
    let localizedFilenamePrefix =
      localizedFilename.substr(0, localizedFilename.indexOf("-"));
    delete this._filenamesRegex;
    return this._filenamesRegex =
      new RegExp("^(bookmarks|" + localizedFilenamePrefix + ")-([0-9-]+)(_[0-9]+)*\.(json|html)");
  },

  get folder() {
    Deprecated.warning(
      "PlacesBackups.folder is deprecated and will be removed in a future version",
      "https://bugzilla.mozilla.org/show_bug.cgi?id=859695");

    let bookmarksBackupDir = Services.dirsvc.get("ProfD", Ci.nsILocalFile);
    bookmarksBackupDir.append(this.profileRelativeFolderPath);
    if (!bookmarksBackupDir.exists()) {
      bookmarksBackupDir.create(Ci.nsIFile.DIRECTORY_TYPE, parseInt("0700", 8));
      if (!bookmarksBackupDir.exists())
        throw("Unable to create bookmarks backup folder");
    }
    delete this.folder;
    return this.folder = bookmarksBackupDir;
  },

  




  getBackupFolder: function PB_getBackupFolder() {
    return Task.spawn(function() {
      if (this._folder) {
        throw new Task.Result(this._folder);
      }
      let profileDir = OS.Constants.Path.profileDir;
      let backupsDirPath = OS.Path.join(profileDir, this.profileRelativeFolderPath);
      yield OS.File.makeDir(backupsDirPath, { ignoreExisting: true }).then(
        function onSuccess() {
          this._folder = backupsDirPath;
         }.bind(this),
         function onError() {
           throw("Unable to create bookmarks backup folder");
         });
       throw new Task.Result(this._folder);
     }.bind(this));
  },

  get profileRelativeFolderPath() "bookmarkbackups",

  


  get entries() {
    Deprecated.warning(
      "PlacesBackups.entries is deprecated and will be removed in a future version",
      "https://bugzilla.mozilla.org/show_bug.cgi?id=859695");

    delete this.entries;
    this.entries = [];
    let files = this.folder.directoryEntries;
    while (files.hasMoreElements()) {
      let entry = files.getNext().QueryInterface(Ci.nsIFile);
      
      
      let matches = entry.leafName.match(this._filenamesRegex);
      if (!entry.isHidden() && matches) {
        
        if (this.getDateForFile(entry) > new Date()) {
          entry.remove(false);
          continue;
        }
        this.entries.push(entry);
      }
    }
    this.entries.sort((a, b) => {
      let aDate = this.getDateForFile(a);
      let bDate = this.getDateForFile(b);
      return aDate < bDate ? 1 : aDate > bDate ? -1 : 0;
    });
    return this.entries;
  },

  




  getBackupFiles: function PB_getBackupFiles() {
    return Task.spawn(function() {
      if (this._backupFiles) {
        throw new Task.Result(this._backupFiles);
      }
      this._backupFiles = [];

      let backupFolderPath = yield this.getBackupFolder();
      let iterator = new OS.File.DirectoryIterator(backupFolderPath);
      yield iterator.forEach(function(aEntry) {
        let matches = aEntry.name.match(this._filenamesRegex);
        if (matches) {
          
          let filePath = aEntry.path;
          if (this.getDateForFile(filePath) > new Date()) {
            return OS.File.remove(filePath);
          } else {
            this._backupFiles.push(filePath);
          }
        }
      }.bind(this));
      iterator.close();

      this._backupFiles.sort(function(a, b) {
        let aDate = this.getDateForFile(a);
        let bDate = this.getDateForFile(b);
        return aDate < bDate ? 1 : aDate > bDate ? -1 : 0;
      }.bind(this));

      throw new Task.Result(this._backupFiles);
    }.bind(this));
  },

  







  getFilenameForDate: function PB_getFilenameForDate(aDateObj) {
    let dateObj = aDateObj || new Date();
    
    
    return "bookmarks-" + dateObj.toLocaleFormat("%Y-%m-%d") + ".json";
  },

  







  getDateForFile: function PB_getDateForFile(aBackupFile) {
    let filename = (aBackupFile instanceof Ci.nsIFile) ? aBackupFile.leafName
                                                       : OS.Path.basename(aBackupFile);
    let matches = filename.match(this._filenamesRegex);
    if (!matches)
      throw("Invalid backup file name: " + filename);
    return new Date(matches[2].replace(/-/g, "/"));
  },

  







  getMostRecent: function PB_getMostRecent(aFileExt) {
    Deprecated.warning(
      "PlacesBackups.getMostRecent is deprecated and will be removed in a future version",
      "https://bugzilla.mozilla.org/show_bug.cgi?id=859695");

    let fileExt = aFileExt || "(json|html)";
    for (let i = 0; i < this.entries.length; i++) {
      let rx = new RegExp("\." + fileExt + "$");
      if (this.entries[i].leafName.match(rx))
        return this.entries[i];
    }
    return null;
  },

   








   getMostRecentBackup: function PB_getMostRecentBackup(aFileExt) {
     return Task.spawn(function() {
       let fileExt = aFileExt || "(json|html)";
       let entries = yield this.getBackupFiles();
       for (let entry of entries) {
         let rx = new RegExp("\." + fileExt + "$");
         if (OS.Path.basename(entry).match(rx)) {
           throw new Task.Result(entry);
         }
       }
       throw new Task.Result(null);
    }.bind(this));
  },

  









  saveBookmarksToJSONFile: function PB_saveBookmarksToJSONFile(aFile) {
    return Task.spawn(function() {
      let nodeCount = yield BookmarkJSONUtils.exportToFile(aFile);

      let backupFolderPath = yield this.getBackupFolder();
      if (aFile.parent.path == backupFolderPath) {
        
        this.entries.push(aFile);
        if (!this._backupFiles) {
          yield this.getBackupFiles();
        }
        this._backupFiles.push(aFile.path);
      } else {
        
        
        
        
        let name = this.getFilenameForDate();
        let newFilename = this._appendMetaDataToFilename(name,
                                                         { nodeCount: nodeCount });
        let newFilePath = OS.Path.join(backupFolderPath, newFilename);
        let backupFile = yield this._getBackupFileForSameDate(name);

        if (backupFile) {
          yield OS.File.remove(backupFile, { ignoreAbsent: true });
        } else {
          let file = new FileUtils.File(newFilePath);

          
          
          this.entries.push(file);
          if (!this._backupFiles) {
            yield this.getBackupFiles();
          }
          this._backupFiles.push(file.path);
        }
        yield OS.File.copy(aFile.path, newFilePath);
      }

      throw new Task.Result(nodeCount);
    }.bind(this));
  },

  












  create: function PB_create(aMaxBackups, aForceBackup) {
    return Task.spawn(function() {
      
      let newBackupFilename = this.getFilenameForDate();
      let mostRecentBackupFile = yield this.getMostRecentBackup();

      if (!aForceBackup) {
        let numberOfBackupsToDelete = 0;
        if (aMaxBackups !== undefined && aMaxBackups > -1) {
          let backupFiles = yield this.getBackupFiles();
          numberOfBackupsToDelete = backupFiles.length - aMaxBackups;
        }

        if (numberOfBackupsToDelete > 0) {
          
          
          
          if (!mostRecentBackupFile ||
              !this._isFilenameWithSameDate(OS.Path.basename(mostRecentBackupFile),
                                            newBackupFilename))
            numberOfBackupsToDelete++;

          while (numberOfBackupsToDelete--) {
            this.entries.pop();
            if (!this._backupFiles) {
              yield this.getBackupFiles();
            }
            let oldestBackup = this._backupFiles.pop();
            yield OS.File.remove(oldestBackup);
          }
        }

        
        if (aMaxBackups === 0 ||
            (mostRecentBackupFile &&
             this._isFilenameWithSameDate(OS.Path.basename(mostRecentBackupFile),
                                          newBackupFilename)))
          return;
      }

      let backupFile = yield this._getBackupFileForSameDate(newBackupFilename);
      if (backupFile) {
        if (aForceBackup) {
          yield OS.File.remove(backupFile, { ignoreAbsent: true });
        } else {
          return;
        }
      }

      
      let backupFolderPath = yield this.getBackupFolder();
      let backupFolder = new FileUtils.File(backupFolderPath);
      let newBackupFile = backupFolder.clone();
      newBackupFile.append(newBackupFilename);

      let nodeCount = yield this.saveBookmarksToJSONFile(newBackupFile);
      
      let newFilenameWithMetaData = this._appendMetaDataToFilename(
                                      newBackupFilename,
                                      { nodeCount: nodeCount });
      newBackupFile.moveTo(backupFolder, newFilenameWithMetaData);

      
      let newFileWithMetaData = backupFolder.clone();
      newFileWithMetaData.append(newFilenameWithMetaData);
      this.entries.pop();
      this.entries.push(newFileWithMetaData);
      this._backupFiles.pop();
      this._backupFiles.push(newFileWithMetaData.path);
    }.bind(this));
  },

  _appendMetaDataToFilename:
  function PB__appendMetaDataToFilename(aFilename, aMetaData) {
    let matches = aFilename.match(this._filenamesRegex);
    let newFilename = matches[1] + "-" + matches[2] + "_" +
                      aMetaData.nodeCount + "." + matches[4];
    return newFilename;
  },

  







  getBookmarkCountForFile: function PB_getBookmarkCountForFile(aFilePath) {
    let count = null;
    let filename = OS.Path.basename(aFilePath);
    let matches = filename.match(this._filenamesRegex);

    if (matches && matches[3])
      count = matches[3].replace(/_/g, "");
    return count;
  },

  _isFilenameWithSameDate:
  function PB__isFilenameWithSameDate(aSourceName, aTargetName) {
    let sourceMatches = aSourceName.match(this._filenamesRegex);
    let targetMatches = aTargetName.match(this._filenamesRegex);

    return (sourceMatches && targetMatches &&
            sourceMatches[1] == targetMatches[1] &&
            sourceMatches[2] == targetMatches[2] &&
            sourceMatches[4] == targetMatches[4]);
    },

   _getBackupFileForSameDate:
   function PB__getBackupFileForSameDate(aFilename) {
     return Task.spawn(function() {
       let backupFolderPath = yield this.getBackupFolder();
       let iterator = new OS.File.DirectoryIterator(backupFolderPath);
       let backupFile;

       yield iterator.forEach(function(aEntry) {
         if (this._isFilenameWithSameDate(aEntry.name, aFilename)) {
           backupFile = aEntry.path;
           return iterator.close();
         }
       }.bind(this));
       yield iterator.close();

       throw new Task.Result(backupFile);
     }.bind(this));
   }
}
