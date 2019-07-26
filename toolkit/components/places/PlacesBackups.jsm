





this.EXPORTED_SYMBOLS = ["PlacesBackups"];

const Ci = Components.interfaces;
const Cu = Components.utils;
const Cc = Components.classes;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/PlacesUtils.jsm");
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/osfile.jsm");
Cu.import("resource://gre/modules/NetUtil.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "BookmarkJSONUtils",
  "resource://gre/modules/BookmarkJSONUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Deprecated",
  "resource://gre/modules/Deprecated.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "OS",
  "resource://gre/modules/osfile.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Sqlite",
  "resource://gre/modules/Sqlite.jsm");

XPCOMUtils.defineLazyGetter(this, "localFileCtor",
  () => Components.Constructor("@mozilla.org/file/local;1",
                               "nsILocalFile", "initWithPath"));

XPCOMUtils.defineLazyGetter(this, "filenamesRegex",
  () => new RegExp("^bookmarks-([0-9\-]+)(?:_([0-9]+)){0,1}(?:_([a-z0-9=\+\-]{24})){0,1}\.(json|html)", "i")
);




function appendMetaDataToFilename(aFilename, aMetaData) {
  let matches = aFilename.match(filenamesRegex);
  return "bookmarks-" + matches[1] +
                  "_" + aMetaData.count +
                  "_" + aMetaData.hash +
                  "." + matches[4];
}






function getHashFromFilename(aFilename) {
  let matches = aFilename.match(filenamesRegex);
  if (matches && matches[3])
    return matches[3];
  return null;
}




function isFilenameWithSameDate(aSourceName, aTargetName) {
  let sourceMatches = aSourceName.match(filenamesRegex);
  let targetMatches = aTargetName.match(filenamesRegex);

  return sourceMatches && targetMatches &&
         sourceMatches[1] == targetMatches[1] &&
         sourceMatches[4] == targetMatches[4];
}






function getBackupFileForSameDate(aFilename) {
  return Task.spawn(function* () {
    let backupFiles = yield PlacesBackups.getBackupFiles();
    for (let backupFile of backupFiles) {
      if (isFilenameWithSameDate(OS.Path.basename(backupFile), aFilename))
        return backupFile;
    }
    return null;
  });
}

this.PlacesBackups = {
  







  get filenamesRegex() filenamesRegex,

  get folder() {
    Deprecated.warning(
      "PlacesBackups.folder is deprecated and will be removed in a future version",
      "https://bugzilla.mozilla.org/show_bug.cgi?id=859695");
    return this._folder;
  },

  



  get _folder() {
    let bookmarksBackupDir = Services.dirsvc.get("ProfD", Ci.nsILocalFile);
    bookmarksBackupDir.append(this.profileRelativeFolderPath);
    if (!bookmarksBackupDir.exists()) {
      bookmarksBackupDir.create(Ci.nsIFile.DIRECTORY_TYPE, parseInt("0700", 8));
      if (!bookmarksBackupDir.exists())
        throw("Unable to create bookmarks backup folder");
    }
    delete this._folder;
    return this._folder = bookmarksBackupDir;
  },

  




  getBackupFolder: function PB_getBackupFolder() {
    return Task.spawn(function* () {
      if (this._backupFolder) {
        return this._backupFolder;
      }
      let profileDir = OS.Constants.Path.profileDir;
      let backupsDirPath = OS.Path.join(profileDir, this.profileRelativeFolderPath);
      yield OS.File.makeDir(backupsDirPath, { ignoreExisting: true });
      return this._backupFolder = backupsDirPath;
    }.bind(this));
  },

  get profileRelativeFolderPath() "bookmarkbackups",

  


  get entries() {
    Deprecated.warning(
      "PlacesBackups.entries is deprecated and will be removed in a future version",
      "https://bugzilla.mozilla.org/show_bug.cgi?id=859695");
    return this._entries;
  },

  



  get _entries() {
    delete this._entries;
    this._entries = [];
    let files = this._folder.directoryEntries;
    while (files.hasMoreElements()) {
      let entry = files.getNext().QueryInterface(Ci.nsIFile);
      
      
      if (!entry.isHidden() && filenamesRegex.test(entry.leafName)) {
        
        if (this.getDateForFile(entry) > new Date()) {
          entry.remove(false);
          continue;
        }
        this._entries.push(entry);
      }
    }
    this._entries.sort((a, b) => {
      let aDate = this.getDateForFile(a);
      let bDate = this.getDateForFile(b);
      return aDate < bDate ? 1 : aDate > bDate ? -1 : 0;
    });
    return this._entries;
  },

  




  getBackupFiles: function PB_getBackupFiles() {
    return Task.spawn(function* () {
      if (this._backupFiles)
        return this._backupFiles;

      this._backupFiles = [];

      let backupFolderPath = yield this.getBackupFolder();
      let iterator = new OS.File.DirectoryIterator(backupFolderPath);
      yield iterator.forEach(function(aEntry) {
        
        
        if (aEntry.name.endsWith(".tmp")) {
          OS.File.remove(aEntry.path);
          return;
        }

        if (filenamesRegex.test(aEntry.name)) {
          
          let filePath = aEntry.path;
          if (this.getDateForFile(filePath) > new Date()) {
            return OS.File.remove(filePath);
          } else {
            this._backupFiles.push(filePath);
          }
        }
      }.bind(this));
      iterator.close();

      this._backupFiles.sort((a, b) => {
        let aDate = this.getDateForFile(a);
        let bDate = this.getDateForFile(b);
        return aDate < bDate ? 1 : aDate > bDate ? -1 : 0;
      });

      return this._backupFiles;
    }.bind(this));
  },

  







  getFilenameForDate: function PB_getFilenameForDate(aDateObj) {
    let dateObj = aDateObj || new Date();
    
    
    return "bookmarks-" + dateObj.toLocaleFormat("%Y-%m-%d") + ".json";
  },

  







  getDateForFile: function PB_getDateForFile(aBackupFile) {
    let filename = (aBackupFile instanceof Ci.nsIFile) ? aBackupFile.leafName
                                                       : OS.Path.basename(aBackupFile);
    let matches = filename.match(filenamesRegex);
    if (!matches)
      throw("Invalid backup file name: " + filename);
    return new Date(matches[1].replace(/-/g, "/"));
  },

  




  getMostRecent: function PB_getMostRecent() {
    Deprecated.warning(
      "PlacesBackups.getMostRecent is deprecated and will be removed in a future version",
      "https://bugzilla.mozilla.org/show_bug.cgi?id=859695");

    for (let i = 0; i < this._entries.length; i++) {
      let rx = new RegExp("\.json$");
      if (this._entries[i].leafName.match(rx))
        return this._entries[i];
    }
    return null;
  },

   





   getMostRecentBackup: function PB_getMostRecentBackup() {
     return Task.spawn(function* () {
       let entries = yield this.getBackupFiles();
       for (let entry of entries) {
         let rx = new RegExp("\.json$");
         if (OS.Path.basename(entry).match(rx)) {
           return entry;
         }
       }
       return null;
    }.bind(this));
  },

  










  saveBookmarksToJSONFile: function PB_saveBookmarksToJSONFile(aFilePath) {
    if (aFilePath instanceof Ci.nsIFile) {
      Deprecated.warning("Passing an nsIFile to PlacesBackups.saveBookmarksToJSONFile " +
                         "is deprecated. Please use an OS.File path instead.",
                         "https://developer.mozilla.org/docs/JavaScript_OS.File");
      aFilePath = aFilePath.path;
    }
    return Task.spawn(function* () {
      let { count: nodeCount, hash: hash } =
        yield BookmarkJSONUtils.exportToFile(aFilePath);

      let backupFolderPath = yield this.getBackupFolder();
      if (OS.Path.dirname(aFilePath) == backupFolderPath) {
        
        
        this._entries.unshift(new localFileCtor(aFilePath));
        if (!this._backupFiles) {
          yield this.getBackupFiles();
        }
        this._backupFiles.unshift(aFilePath);
      } else {
        
        
        
        
        let mostRecentBackupFile = yield this.getMostRecentBackup();
        if (!mostRecentBackupFile ||
            hash != getHashFromFilename(OS.Path.basename(mostRecentBackupFile))) {
          let name = this.getFilenameForDate();
          let newFilename = appendMetaDataToFilename(name,
                                                     { count: nodeCount,
                                                       hash: hash });
          let newFilePath = OS.Path.join(backupFolderPath, newFilename);
          let backupFile = yield getBackupFileForSameDate(name);
          if (backupFile) {
            
            yield OS.File.remove(backupFile, { ignoreAbsent: true });
            if (!this._backupFiles)
              yield this.getBackupFiles();
            else
              this._backupFiles.shift();
            this._backupFiles.unshift(newFilePath);
          } else {
            
            this._entries.unshift(new localFileCtor(newFilePath));
            if (!this._backupFiles)
              yield this.getBackupFiles();
            this._backupFiles.unshift(newFilePath);
          }

          yield OS.File.copy(aFilePath, newFilePath);
        }
      }

      return nodeCount;
    }.bind(this));
  },

  














  create: function PB_create(aMaxBackups, aForceBackup) {
    let limitBackups = function* () {
      let backupFiles = yield this.getBackupFiles();
      if (typeof aMaxBackups == "number" && aMaxBackups > -1 &&
          backupFiles.length >= aMaxBackups) {
        let numberOfBackupsToDelete = backupFiles.length - aMaxBackups;
        while (numberOfBackupsToDelete--) {
          this._entries.pop();
          let oldestBackup = this._backupFiles.pop();
          yield OS.File.remove(oldestBackup);
        }
      }
    }.bind(this);

    return Task.spawn(function* () {
      if (aMaxBackups === 0) {
        
        yield limitBackups(0);
        return;
      }

      
      if (!this._backupFiles)
        yield this.getBackupFiles();
      let newBackupFilename = this.getFilenameForDate();
      
      
      let backupFile = yield getBackupFileForSameDate(newBackupFilename);
      if (backupFile && !aForceBackup)
        return;

      if (backupFile) {
        
        this._backupFiles.shift();
        this._entries.shift();
        yield OS.File.remove(backupFile, { ignoreAbsent: true });
      }

      
      
      let mostRecentBackupFile = yield this.getMostRecentBackup();
      let mostRecentHash = mostRecentBackupFile &&
                           getHashFromFilename(OS.Path.basename(mostRecentBackupFile));

      
      let backupFolder = yield this.getBackupFolder();
      let newBackupFile = OS.Path.join(backupFolder, newBackupFilename);
      let newFilenameWithMetaData;
      try {
        let { count: nodeCount, hash: hash } =
          yield BookmarkJSONUtils.exportToFile(newBackupFile,
                                               { failIfHashIs: mostRecentHash });
        newFilenameWithMetaData = appendMetaDataToFilename(newBackupFilename,
                                                           { count: nodeCount,
                                                             hash: hash });
      } catch (ex if ex.becauseSameHash) {
        
        
        this._backupFiles.shift();
        this._entries.shift();
        newBackupFile = mostRecentBackupFile;
        newFilenameWithMetaData = appendMetaDataToFilename(
          newBackupFilename,
          { count: this.getBookmarkCountForFile(mostRecentBackupFile),
            hash: mostRecentHash });
      }

      
      let newBackupFileWithMetadata = OS.Path.join(backupFolder, newFilenameWithMetaData);
      yield OS.File.move(newBackupFile, newBackupFileWithMetadata);
      this._entries.unshift(new localFileCtor(newBackupFileWithMetadata));
      this._backupFiles.unshift(newBackupFileWithMetadata);

      
      yield limitBackups(aMaxBackups);
    }.bind(this));
  },

  







  getBookmarkCountForFile: function PB_getBookmarkCountForFile(aFilePath) {
    let count = null;
    let filename = OS.Path.basename(aFilePath);
    let matches = filename.match(filenamesRegex);
    if (matches && matches[2])
      count = matches[2];
    return count;
  },

  

























  getBookmarksTree: function () {
    return Task.spawn(function* () {
      let dbFilePath = OS.Path.join(OS.Constants.Path.profileDir, "places.sqlite");
      let conn = yield Sqlite.openConnection({ path: dbFilePath,
                                               sharedMemoryCache: false });
      let rows = [];
      try {
        rows = yield conn.execute(
          "SELECT b.id, h.url, IFNULL(b.title, '') AS title, b.parent, " +
                 "b.position AS [index], b.type, b.dateAdded, b.lastModified, " +
                 "b.guid, f.url AS iconuri, " +
                 "( SELECT GROUP_CONCAT(t.title, ',') " +
                   "FROM moz_bookmarks b2 " +
                   "JOIN moz_bookmarks t ON t.id = +b2.parent AND t.parent = :tags_folder " +
                   "WHERE b2.fk = h.id " +
                 ") AS tags, " +
                 "EXISTS (SELECT 1 FROM moz_items_annos WHERE item_id = b.id LIMIT 1) AS has_annos, " +
                 "( SELECT a.content FROM moz_annos a " +
                   "JOIN moz_anno_attributes n ON a.anno_attribute_id = n.id " +
                   "WHERE place_id = h.id AND n.name = :charset_anno " +
                 ") AS charset " +
          "FROM moz_bookmarks b " +
          "LEFT JOIN moz_bookmarks p ON p.id = b.parent " +
          "LEFT JOIN moz_places h ON h.id = b.fk " +
          "LEFT JOIN moz_favicons f ON f.id = h.favicon_id " +
          "WHERE b.id <> :tags_folder AND b.parent <> :tags_folder AND p.parent <> :tags_folder " +
          "ORDER BY b.parent, b.position",
          { tags_folder: PlacesUtils.tagsFolderId,
            charset_anno: PlacesUtils.CHARSET_ANNO });
      } catch(e) {
        Cu.reportError("Unable to query the database " + e);
      } finally {
        yield conn.close();
      }

      let startTime = Date.now();
      
      let itemsMap = new Map();
      for (let row of rows) {
        let id = row.getResultByName("id");
        try {
          let bookmark = sqliteRowToBookmarkObject(row);
          if (itemsMap.has(id)) {
            
            
            let original = itemsMap.get(id);
            for (let prop of Object.getOwnPropertyNames(bookmark)) {
              original[prop] = bookmark[prop];
            }
            bookmark = original;
          }
          else {
            itemsMap.set(id, bookmark);
          }

          
          if (!itemsMap.has(bookmark.parent))
            itemsMap.set(bookmark.parent, {});
          let parent = itemsMap.get(bookmark.parent);
          if (!("children" in parent))
            parent.children = [];
          parent.children.push(bookmark);
        } catch (e) {
          Cu.reportError("Error while reading node " + id + " " + e);
        }
      }

      
      function removeFromMap(id) {
        
        
        if (itemsMap.has(id)) {
          let excludedItem = itemsMap.get(id);
          if (excludedItem.children) {
            for (let child of excludedItem.children) {
              removeFromMap(child.id);
            }
          }
          
          let parentItem = itemsMap.get(excludedItem.parent);
          parentItem.children = parentItem.children.filter(aChild => aChild.id != id);
          
          itemsMap.delete(id);
        }
      }

      for (let id of PlacesUtils.annotations.getItemsWithAnnotation(
                       PlacesUtils.EXCLUDE_FROM_BACKUP_ANNO)) {
        removeFromMap(id);
      }

      
      
      try {
        Services.telemetry
                .getHistogramById("PLACES_BACKUPS_BOOKMARKSTREE_MS")
                .add(Date.now() - startTime);
      } catch (ex) {
        Components.utils.reportError("Unable to report telemetry.");
      }

      return [itemsMap.get(PlacesUtils.placesRootId), itemsMap.size];
    });
  }
}







function sqliteRowToBookmarkObject(aRow) {
  let bookmark = {};
  for (let p of [ "id" ,"guid", "title", "index", "dateAdded", "lastModified" ]) {
    bookmark[p] = aRow.getResultByName(p);
  }
  Object.defineProperty(bookmark, "parent",
                        { value: aRow.getResultByName("parent") });

  let type = aRow.getResultByName("type");

  
  if (aRow.getResultByName("has_annos")) {
    try {
      bookmark.annos = PlacesUtils.getAnnotationsForItem(bookmark.id);
    } catch (e) {
      Cu.reportError("Unexpected error while reading annotations " + e);
    }
  }

  switch (type) {
    case Ci.nsINavBookmarksService.TYPE_BOOKMARK:
      
      bookmark.type = PlacesUtils.TYPE_X_MOZ_PLACE;
      
      
      bookmark.uri = NetUtil.newURI(aRow.getResultByName("url")).spec;
      
      let keyword = PlacesUtils.bookmarks.getKeywordForBookmark(bookmark.id);
      if (keyword)
        bookmark.keyword = keyword;
      let charset = aRow.getResultByName("charset");
      if (charset)
        bookmark.charset = charset;
      let tags = aRow.getResultByName("tags");
      if (tags)
        bookmark.tags = tags;
      let iconuri = aRow.getResultByName("iconuri");
      if (iconuri)
        bookmark.iconuri = iconuri;
      break;
    case Ci.nsINavBookmarksService.TYPE_FOLDER:
      bookmark.type = PlacesUtils.TYPE_X_MOZ_PLACE_CONTAINER;

      
      if (bookmark.id == PlacesUtils.placesRootId)
        bookmark.root = "placesRoot";
      else if (bookmark.id == PlacesUtils.bookmarksMenuFolderId)
        bookmark.root = "bookmarksMenuFolder";
      else if (bookmark.id == PlacesUtils.unfiledBookmarksFolderId)
        bookmark.root = "unfiledBookmarksFolder";
      else if (bookmark.id == PlacesUtils.toolbarFolderId)
        bookmark.root = "toolbarFolder";
      break;
    case Ci.nsINavBookmarksService.TYPE_SEPARATOR:
      bookmark.type = PlacesUtils.TYPE_X_MOZ_PLACE_SEPARATOR;
      break;
    default:
      Cu.reportError("Unexpected bookmark type");
      break;
  }
  return bookmark;
}
