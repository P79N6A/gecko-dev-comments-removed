




































const NS_APP_USER_PROFILE_50_DIR = "ProfD";
const NS_APP_PROFILE_DIR_STARTUP = "ProfDS";
const NS_APP_HISTORY_50_FILE = "UHist";
const NS_APP_BOOKMARKS_50_FILE = "BMarks";


const TOPIC_GLOBAL_SHUTDOWN = "profile-before-change";


const TRANSITION_LINK = Ci.nsINavHistoryService.TRANSITION_LINK;
const TRANSITION_TYPED = Ci.nsINavHistoryService.TRANSITION_TYPED;
const TRANSITION_BOOKMARK = Ci.nsINavHistoryService.TRANSITION_BOOKMARK;
const TRANSITION_EMBED = Ci.nsINavHistoryService.TRANSITION_EMBED;
const TRANSITION_FRAMED_LINK = Ci.nsINavHistoryService.TRANSITION_FRAMED_LINK;
const TRANSITION_REDIRECT_PERMANENT = Ci.nsINavHistoryService.TRANSITION_REDIRECT_PERMANENT;
const TRANSITION_REDIRECT_TEMPORARY = Ci.nsINavHistoryService.TRANSITION_REDIRECT_TEMPORARY;
const TRANSITION_DOWNLOAD = Ci.nsINavHistoryService.TRANSITION_DOWNLOAD;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyGetter(this, "Services", function() {
  Cu.import("resource://gre/modules/Services.jsm");
  return Services;
});

XPCOMUtils.defineLazyGetter(this, "NetUtil", function() {
  Cu.import("resource://gre/modules/NetUtil.jsm");
  return NetUtil;
});

XPCOMUtils.defineLazyGetter(this, "PlacesUtils", function() {
  Cu.import("resource://gre/modules/PlacesUtils.jsm");
  return PlacesUtils;
});


function LOG(aMsg) {
  aMsg = ("*** PLACES TESTS: " + aMsg);
  Services.console.logStringMessage(aMsg);
  print(aMsg);
}


let gTestDir = do_get_cwd();



let gProfD = do_get_profile();


let (provider = {
      getFile: function(prop, persistent) {
        persistent.value = true;
        if (prop == NS_APP_HISTORY_50_FILE) {
          let histFile = Services.dirsvc.get("ProfD", Ci.nsIFile);
          histFile.append("history.dat");
          return histFile;
        }
        throw Cr.NS_ERROR_FAILURE;
      },
      QueryInterface: XPCOMUtils.generateQI([Ci.nsIDirectoryServiceProvider])
    })
{
  Cc["@mozilla.org/file/directory_service;1"].
  getService(Ci.nsIDirectoryService).
  QueryInterface(Ci.nsIDirectoryService).registerProvider(provider);
}



clearDB();








function uri(aSpec) NetUtil.newURI(aSpec);








function DBConn() {
  let db = PlacesUtils.history.QueryInterface(Ci.nsPIPlacesDatabase)
                              .DBConnection;
  if (db.connectionReady)
    return db;

  
  let file = Services.dirsvc.get('ProfD', Ci.nsIFile);
  file.append("places.sqlite");
  return Services.storage.openDatabase(file);
};








function readFileData(aFile) {
  let inputStream = Cc["@mozilla.org/network/file-input-stream;1"].
                    createInstance(Ci.nsIFileInputStream);
  
  inputStream.init(aFile, 0x01, -1, null);
  let size = inputStream.available();

  
  let bis = Cc["@mozilla.org/binaryinputstream;1"].
            createInstance(Ci.nsIBinaryInputStream);
  bis.setInputStream(inputStream);

  let bytes = bis.readByteArray(size);

  if (size != bytes.length)
      throw "Didn't read expected number of bytes";

  return bytes;
}










function compareArrays(aArray1, aArray2) {
  if (aArray1.length != aArray2.length) {
    print("compareArrays: array lengths differ\n");
    return false;
  }

  for (let i = 0; i < aArray1.length; i++) {
    if (aArray1[i] != aArray2[i]) {
      print("compareArrays: arrays differ at index " + i + ": " +
            "(" + aArray1[i] + ") != (" + aArray2[i] +")\n");
      return false;
    }
  }

  return true;
}





function clearDB() {
  try {
    let file = Services.dirsvc.get('ProfD', Ci.nsIFile);
    file.append("places.sqlite");
    if (file.exists())
      file.remove(false);
  } catch(ex) { dump("Exception: " + ex); }
}








function dump_table(aName)
{
  let stmt = DBConn().createStatement("SELECT * FROM " + aName);

  print("\n*** Printing data from " + aName);
  let count = 0;
  while (stmt.executeStep()) {
    let columns = stmt.numEntries;

    if (count == 0) {
      
      for (let i = 0; i < columns; i++)
        dump(stmt.getColumnName(i) + "\t");
      dump("\n");
    }

    
    for (let i = 0; i < columns; i++) {
      switch (stmt.getTypeOfIndex(i)) {
        case Ci.mozIStorageValueArray.VALUE_TYPE_NULL:
          dump("NULL\t");
          break;
        case Ci.mozIStorageValueArray.VALUE_TYPE_INTEGER:
          dump(stmt.getInt64(i) + "\t");
          break;
        case Ci.mozIStorageValueArray.VALUE_TYPE_FLOAT:
          dump(stmt.getDouble(i) + "\t");
          break;
        case Ci.mozIStorageValueArray.VALUE_TYPE_TEXT:
          dump(stmt.getString(i) + "\t");
          break;
      }
    }
    dump("\n");

    count++;
  }
  print("*** There were a total of " + count + " rows of data.\n");

  stmt.finalize();
}





function remove_all_bookmarks() {
  let PU = PlacesUtils;
  
  PU.bookmarks.removeFolderChildren(PU.bookmarks.bookmarksMenuFolder);
  PU.bookmarks.removeFolderChildren(PU.bookmarks.toolbarFolder);
  PU.bookmarks.removeFolderChildren(PU.bookmarks.unfiledBookmarksFolder);
  
  check_no_bookmarks();
}





function check_no_bookmarks() {
  let query = PlacesUtils.history.getNewQuery();
  let folders = [
    PlacesUtils.bookmarks.toolbarFolder,
    PlacesUtils.bookmarks.bookmarksMenuFolder,
    PlacesUtils.bookmarks.unfiledBookmarksFolder,
  ];
  query.setFolders(folders, 3);
  let options = PlacesUtils.history.getNewQueryOptions();
  options.queryType = Ci.nsINavHistoryQueryOptions.QUERY_TYPE_BOOKMARKS;
  let root = PlacesUtils.history.executeQuery(query, options).root;
  root.containerOpen = true;
  do_check_eq(root.childCount, 0);
  root.containerOpen = false;
}














function setPageTitle(aURI, aTitle) {
  PlacesUtils.history.setPageTitle(aURI, aTitle);
}








function waitForClearHistory(aCallback) {
  let observer = {
    observe: function(aSubject, aTopic, aData) {
      Services.obs.removeObserver(this, PlacesUtils.TOPIC_EXPIRATION_FINISHED);
      aCallback();
    }
  };
  Services.obs.addObserver(observer, PlacesUtils.TOPIC_EXPIRATION_FINISHED, false);

  PlacesUtils.bhistory.removeAllPages();
}





function shutdownPlaces()
{
  let hs = Cc["@mozilla.org/browser/nav-history-service;1"].
           getService(Ci.nsIObserver);
  hs.observe(null, TOPIC_GLOBAL_SHUTDOWN, null);
}











function create_bookmarks_html(aFilename) {
  if (!aFilename)
    do_throw("you must pass a filename to create_bookmarks_html function");
  remove_bookmarks_html();
  let bookmarksHTMLFile = gTestDir.clone();
  bookmarksHTMLFile.append(aFilename);
  do_check_true(bookmarksHTMLFile.exists());
  bookmarksHTMLFile.copyTo(gProfD, FILENAME_BOOKMARKS_HTML);
  let profileBookmarksHTMLFile = gProfD.clone();
  profileBookmarksHTMLFile.append(FILENAME_BOOKMARKS_HTML);
  do_check_true(profileBookmarksHTMLFile.exists());
  return profileBookmarksHTMLFile;
}





function remove_bookmarks_html() {
  let profileBookmarksHTMLFile = gProfD.clone();
  profileBookmarksHTMLFile.append(FILENAME_BOOKMARKS_HTML);
  if (profileBookmarksHTMLFile.exists()) {
    profileBookmarksHTMLFile.remove(false);
    do_check_false(profileBookmarksHTMLFile.exists());
  }
}







function check_bookmarks_html() {
  let profileBookmarksHTMLFile = gProfD.clone();
  profileBookmarksHTMLFile.append(FILENAME_BOOKMARKS_HTML);
  do_check_true(profileBookmarksHTMLFile.exists());
  return profileBookmarksHTMLFile;
}











function create_JSON_backup(aFilename) {
  if (!aFilename)
    do_throw("you must pass a filename to create_JSON_backup function");
  remove_all_JSON_backups();
  let bookmarksBackupDir = gProfD.clone();
  bookmarksBackupDir.append("bookmarkbackups");
  if (!bookmarksBackupDir.exists()) {
    bookmarksBackupDir.create(Ci.nsIFile.DIRECTORY_TYPE, 0777);
    do_check_true(bookmarksBackupDir.exists());
  }
  let bookmarksJSONFile = gTestDir.clone();
  bookmarksJSONFile.append(aFilename);
  do_check_true(bookmarksJSONFile.exists());
  bookmarksJSONFile.copyTo(bookmarksBackupDir, FILENAME_BOOKMARKS_JSON);
  let profileBookmarksJSONFile = bookmarksBackupDir.clone();
  profileBookmarksJSONFile.append(FILENAME_BOOKMARKS_JSON);
  do_check_true(profileBookmarksJSONFile.exists());
  return profileBookmarksJSONFile;
}





function remove_all_JSON_backups() {
  let bookmarksBackupDir = gProfD.clone();
  bookmarksBackupDir.append("bookmarkbackups");
  if (bookmarksBackupDir.exists()) {
    bookmarksBackupDir.remove(true);
    do_check_false(bookmarksBackupDir.exists());
  }
}







function check_JSON_backup() {
  let profileBookmarksJSONFile = gProfD.clone();
  profileBookmarksJSONFile.append("bookmarkbackups");
  profileBookmarksJSONFile.append(FILENAME_BOOKMARKS_JSON);
  do_check_true(profileBookmarksJSONFile.exists());
  return profileBookmarksJSONFile;
}











function is_time_ordered(before, after) {
  
  
  
  let isWindows = ("@mozilla.org/windows-registry-key;1" in Cc);
  
  let skew = isWindows ? 20000000 : 0;
  return after - before > -skew;
}




let (randomFailingSyncTests = [
  "test_multi_word_tags.js",
  "test_removeVisitsByTimeframe.js",
  "test_utils_getURLsForContainerNode.js",
  "test_exclude_livemarks.js",
  "test_402799.js",
  "test_results-as-visit.js",
  "test_sorting.js",
  "test_redirectsMode.js",
  "test_384228.js",
  "test_395593.js",
  "test_containersQueries_sorting.js",
  "test_browserGlue_smartBookmarks.js",
  "test_browserGlue_distribution.js",
  "test_331487.js",
  "test_tags.js",
  "test_385829.js",
  "test_405938_restore_queries.js",
]) {
  let currentTestFilename = do_get_file(_TEST_FILE[0], true).leafName;
  if (randomFailingSyncTests.indexOf(currentTestFilename) != -1) {
    print("Test " + currentTestFilename +
          " is known random due to bug 507790, disabling PlacesDBFlush.");
    let sync = Cc["@mozilla.org/places/sync;1"].getService(Ci.nsIObserver);
    sync.observe(null, "places-debug-stop-sync", null);
  }
}
