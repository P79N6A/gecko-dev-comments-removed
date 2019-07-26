





this.EXPORTED_SYMBOLS = ["PlacesBackups"];

const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/PlacesUtils.jsm");
Cu.import("resource://gre/modules/BookmarkJSONUtils.jsm");
Cu.import("resource://gre/modules/Task.jsm");

this.PlacesBackups = {
  get _filenamesRegex() {
    
    
    let localizedFilename =
      PlacesUtils.getFormattedString("bookmarksArchiveFilename", [new Date()]);
    let localizedFilenamePrefix =
      localizedFilename.substr(0, localizedFilename.indexOf("-"));
    delete this._filenamesRegex;
    return this._filenamesRegex =
      new RegExp("^(bookmarks|" + localizedFilenamePrefix + ")-([0-9-]+)\.(json|html)");
  },

  get folder() {
    let bookmarksBackupDir = Services.dirsvc.get("ProfD", Ci.nsILocalFile);
    bookmarksBackupDir.append("bookmarkbackups");
    if (!bookmarksBackupDir.exists()) {
      bookmarksBackupDir.create(Ci.nsIFile.DIRECTORY_TYPE, parseInt("0700", 8));
      if (!bookmarksBackupDir.exists())
        throw("Unable to create bookmarks backup folder");
    }
    delete this.folder;
    return this.folder = bookmarksBackupDir;
  },

  


  get entries() {
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

  







  getFilenameForDate: function PB_getFilenameForDate(aDateObj) {
    let dateObj = aDateObj || new Date();
    
    
    return "bookmarks-" + dateObj.toLocaleFormat("%Y-%m-%d") + ".json";
  },

  







  getDateForFile: function PB_getDateForFile(aBackupFile) {
    let filename = aBackupFile.leafName;
    let matches = filename.match(this._filenamesRegex);
    if (!matches)
      throw("Invalid backup file name: " + filename);
    return new Date(matches[2].replace(/-/g, "/"));
  },

  







  getMostRecent: function PB_getMostRecent(aFileExt) {
    let fileExt = aFileExt || "(json|html)";
    for (let i = 0; i < this.entries.length; i++) {
      let rx = new RegExp("\." + fileExt + "$");
      if (this.entries[i].leafName.match(rx))
        return this.entries[i];
    }
    return null;
  },

  








  saveBookmarksToJSONFile: function PB_saveBookmarksToJSONFile(aFile) {
    return Task.spawn(function() {
      if (!aFile.exists())
        aFile.create(Ci.nsIFile.NORMAL_FILE_TYPE, parseInt("0600", 8));
      if (!aFile.exists() || !aFile.isWritable()) {
        throw new Error("Unable to create bookmarks backup file: " + aFile.leafName);
      }

      yield BookmarkJSONUtils.exportToFile(aFile);

      if (aFile.parent.equals(this.folder)) {
        
        this.entries.push(aFile);
      } else {
        
        
        
        
        let latestBackup = this.getMostRecent("json");
        if (!latestBackup || latestBackup != aFile) {
          let name = this.getFilenameForDate();
          let file = this.folder.clone();
          file.append(name);
          if (file.exists()) {
            file.remove(false);
          } else {
            
            
            this.entries.push(file);
          }
          aFile.copyTo(this.folder, name);
        }
      }
    }.bind(this));
  },

  












  create: function PB_create(aMaxBackups, aForceBackup) {
    return Task.spawn(function() {
      
      let newBackupFilename = this.getFilenameForDate();
      let mostRecentBackupFile = this.getMostRecent();

      if (!aForceBackup) {
        let numberOfBackupsToDelete = 0;
        if (aMaxBackups !== undefined && aMaxBackups > -1)
          numberOfBackupsToDelete = this.entries.length - aMaxBackups;

        if (numberOfBackupsToDelete > 0) {
          
          
          
          if (!mostRecentBackupFile ||
              mostRecentBackupFile.leafName != newBackupFilename)
            numberOfBackupsToDelete++;

          while (numberOfBackupsToDelete--) {
            let oldestBackup = this.entries.pop();
            oldestBackup.remove(false);
          }
        }

        
        if (aMaxBackups === 0 ||
            (mostRecentBackupFile &&
             mostRecentBackupFile.leafName == newBackupFilename))
          return;
      }

      let newBackupFile = this.folder.clone();
      newBackupFile.append(newBackupFilename);

      if (aForceBackup && newBackupFile.exists())
        newBackupFile.remove(false);

      if (newBackupFile.exists())
        return;

      yield this.saveBookmarksToJSONFile(newBackupFile);
    }.bind(this));
  }
}
