




































const NS_APP_USER_PROFILE_50_DIR = "ProfD";
const NS_APP_PROFILE_DIR_STARTUP = "ProfDS";
const NS_APP_HISTORY_50_FILE = "UHist";
const NS_APP_BOOKMARKS_50_FILE = "BMarks";


const TRANSITION_LINK = Ci.nsINavHistoryService.TRANSITION_LINK;
const TRANSITION_TYPED = Ci.nsINavHistoryService.TRANSITION_TYPED;
const TRANSITION_BOOKMARK = Ci.nsINavHistoryService.TRANSITION_BOOKMARK;
const TRANSITION_EMBED = Ci.nsINavHistoryService.TRANSITION_EMBED;
const TRANSITION_FRAMED_LINK = Ci.nsINavHistoryService.TRANSITION_FRAMED_LINK;
const TRANSITION_REDIRECT_PERMANENT = Ci.nsINavHistoryService.TRANSITION_REDIRECT_PERMANENT;
const TRANSITION_REDIRECT_TEMPORARY = Ci.nsINavHistoryService.TRANSITION_REDIRECT_TEMPORARY;
const TRANSITION_DOWNLOAD = Ci.nsINavHistoryService.TRANSITION_DOWNLOAD;



const FAVICON_ERRORPAGE_URL = "chrome://global/skin/icons/warning-16.png";

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


Services.prefs.setBoolPref("places.history.enabled", true);


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








function page_in_database(aUrl)
{
  let stmt = DBConn().createStatement(
    "SELECT id FROM moz_places WHERE url = :url"
  );
  stmt.params.url = aUrl;
  try {
    if (!stmt.executeStep())
      return 0;
    return stmt.getInt64(0);
  }
  finally {
    stmt.finalize();
  }
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
  if (root.childCount != 0)
    do_throw("Unable to remove all bookmarks");
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





function shutdownPlaces(aKeepAliveConnection)
{
  let hs = PlacesUtils.history.QueryInterface(Ci.nsIObserver);
  hs.observe(null, "profile-change-teardown", null);
  hs.observe(null, "profile-before-change", null);
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




















function waitForFrecency(aURI, aValidator, aCallback, aCbScope, aCbArguments) {
  Services.obs.addObserver(function (aSubject, aTopic, aData) {
    let frecency = frecencyForUrl(aURI);
    if (!aValidator(frecency)) {
      print("Has to wait for frecency...");
      return;
    }
    Services.obs.removeObserver(arguments.callee, aTopic);
    aCallback.apply(aCbScope, aCbArguments);
  }, "places-frecency-updated", false);
}








function frecencyForUrl(aURI)
{
  let url = aURI instanceof Ci.nsIURI ? aURI.spec : aURI;
  let stmt = DBConn().createStatement(
    "SELECT frecency FROM moz_places WHERE url = ?1"
  );
  stmt.bindByIndex(0, url);
  if (!stmt.executeStep())
    throw new Error("No result for frecency.");
  let frecency = stmt.getInt32(0);
  stmt.finalize();

  return frecency;
}








function isUrlHidden(aURI)
{
  let url = aURI instanceof Ci.nsIURI ? aURI.spec : aURI;
  let stmt = DBConn().createStatement(
    "SELECT hidden FROM moz_places WHERE url = ?1"
  );
  stmt.bindByIndex(0, url);
  if (!stmt.executeStep())
    throw new Error("No result for hidden.");
  let hidden = stmt.getInt32(0);
  stmt.finalize();

  return !!hidden;
}










function is_time_ordered(before, after) {
  
  
  
  let isWindows = ("@mozilla.org/windows-registry-key;1" in Cc);
  
  let skew = isWindows ? 20000000 : 0;
  return after - before > -skew;
}


















function waitForAsyncUpdates(aCallback, aScope, aArguments)
{
  let scope = aScope || this;
  let args = aArguments || [];
  let db = DBConn();
  db.createAsyncStatement("BEGIN EXCLUSIVE").executeAsync();
  db.createAsyncStatement("COMMIT").executeAsync({
    handleResult: function() {},
    handleError: function() {},
    handleCompletion: function(aReason)
    {
      aCallback.apply(scope, args);
    }
  });
}









function do_check_valid_places_guid(aGuid,
                                    aStack)
{
  if (!aStack) {
    aStack = Components.stack.caller;
  }
  do_check_true(/^[a-zA-Z0-9\-_]{12}$/.test(aGuid), aStack);
}









function do_check_guid_for_uri(aURI,
                               aGUID)
{
  let caller = Components.stack.caller;
  let stmt = DBConn().createStatement(
    "SELECT guid "
  + "FROM moz_places "
  + "WHERE url = :url "
  );
  stmt.params.url = aURI.spec;
  do_check_true(stmt.executeStep(), caller);
  do_check_valid_places_guid(stmt.row.guid, caller);
  if (aGUID) {
    do_check_valid_places_guid(aGUID, caller);
    do_check_eq(stmt.row.guid, aGUID, caller);
  }
  stmt.finalize();
}







function do_log_info(aMessage)
{
  print("TEST-INFO | " + _TEST_FILE + " | " + aMessage);
}












function do_compare_arrays(a1, a2, sorted)
{
  if (a1.length != a2.length)
    return false;

  if (sorted) {
    return a1.every(function (e, i) e == a2[i]);
  }
  else {
    return a1.filter(function (e) a2.indexOf(e) == -1).length == 0 &&
           a2.filter(function (e) a1.indexOf(e) == -1).length == 0;
  }
}
