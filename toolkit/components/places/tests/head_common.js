




const CURRENT_SCHEMA_VERSION = 24;

const NS_APP_USER_PROFILE_50_DIR = "ProfD";
const NS_APP_PROFILE_DIR_STARTUP = "ProfDS";


const TRANSITION_LINK = Ci.nsINavHistoryService.TRANSITION_LINK;
const TRANSITION_TYPED = Ci.nsINavHistoryService.TRANSITION_TYPED;
const TRANSITION_BOOKMARK = Ci.nsINavHistoryService.TRANSITION_BOOKMARK;
const TRANSITION_EMBED = Ci.nsINavHistoryService.TRANSITION_EMBED;
const TRANSITION_FRAMED_LINK = Ci.nsINavHistoryService.TRANSITION_FRAMED_LINK;
const TRANSITION_REDIRECT_PERMANENT = Ci.nsINavHistoryService.TRANSITION_REDIRECT_PERMANENT;
const TRANSITION_REDIRECT_TEMPORARY = Ci.nsINavHistoryService.TRANSITION_REDIRECT_TEMPORARY;
const TRANSITION_DOWNLOAD = Ci.nsINavHistoryService.TRANSITION_DOWNLOAD;

const TITLE_LENGTH_MAX = 4096;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "FileUtils",
                                  "resource://gre/modules/FileUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "NetUtil",
                                  "resource://gre/modules/NetUtil.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Promise",
                                  "resource://gre/modules/Promise.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Services",
                                  "resource://gre/modules/Services.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
                                  "resource://gre/modules/Task.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "BookmarkJSONUtils",
                                  "resource://gre/modules/BookmarkJSONUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "BookmarkHTMLUtils",
                                  "resource://gre/modules/BookmarkHTMLUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PlacesBackups",
                                  "resource://gre/modules/PlacesBackups.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PlacesTransactions",
                                  "resource://gre/modules/PlacesTransactions.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "OS",
                                  "resource://gre/modules/osfile.jsm");


Cu.import("resource://gre/modules/PlacesUtils.jsm");

XPCOMUtils.defineLazyGetter(this, "SMALLPNG_DATA_URI", function() {
  return NetUtil.newURI(
         "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAA" +
         "AAAA6fptVAAAACklEQVQI12NgAAAAAgAB4iG8MwAAAABJRU5ErkJggg==");
});

function LOG(aMsg) {
  aMsg = ("*** PLACES TESTS: " + aMsg);
  Services.console.logStringMessage(aMsg);
  print(aMsg);
}

let gTestDir = do_get_cwd();


let gProfD = do_get_profile();


clearDB();







function uri(aSpec) NetUtil.newURI(aSpec);













let gDBConn;
function DBConn(aForceNewConnection) {
  if (!aForceNewConnection) {
    let db = PlacesUtils.history.QueryInterface(Ci.nsPIPlacesDatabase)
                                .DBConnection;
    if (db.connectionReady)
      return db;
  }

  
  if (!gDBConn || aForceNewConnection) {
    let file = Services.dirsvc.get('ProfD', Ci.nsIFile);
    file.append("places.sqlite");
    let dbConn = gDBConn = Services.storage.openDatabase(file);

    
    Services.obs.addObserver(function DBCloseCallback(aSubject, aTopic, aData) {
      Services.obs.removeObserver(DBCloseCallback, aTopic);
      dbConn.asyncClose();
    }, "profile-before-change", false);
  }

  return gDBConn.connectionReady ? gDBConn : null;
};





 
function readInputStreamData(aStream) {
  let bistream = Cc["@mozilla.org/binaryinputstream;1"].
                 createInstance(Ci.nsIBinaryInputStream);
  try {
    bistream.setInputStream(aStream);
    let expectedData = [];
    let avail;
    while ((avail = bistream.available())) {
      expectedData = expectedData.concat(bistream.readByteArray(avail));
    }
    return expectedData;
  } finally {
    bistream.close();
  }
}








function readFileData(aFile) {
  let inputStream = Cc["@mozilla.org/network/file-input-stream;1"].
                    createInstance(Ci.nsIFileInputStream);
  
  inputStream.init(aFile, 0x01, -1, null);

  
  let size  = inputStream.available();
  let bytes = readInputStreamData(inputStream);
  if (size != bytes.length) {
    throw "Didn't read expected number of bytes";
  }
  return bytes;
}











function readFileOfLength(aFileName, aExpectedLength) {
  let data = readFileData(do_get_file(aFileName));
  do_check_eq(data.length, aExpectedLength);
  return data;
}












function base64EncodeString(aString) {
  var stream = Cc["@mozilla.org/io/string-input-stream;1"]
               .createInstance(Ci.nsIStringInputStream);
  stream.setData(aString, aString.length);
  var encoder = Cc["@mozilla.org/scriptablebase64encoder;1"]
                .createInstance(Ci.nsIScriptableBase64Encoder);
  return encoder.encodeToString(stream, aString.length);
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








function page_in_database(aURI)
{
  let url = aURI instanceof Ci.nsIURI ? aURI.spec : aURI;
  let stmt = DBConn().createStatement(
    "SELECT id FROM moz_places WHERE url = :url"
  );
  stmt.params.url = url;
  try {
    if (!stmt.executeStep())
      return 0;
    return stmt.getInt64(0);
  }
  finally {
    stmt.finalize();
  }
}







function visits_in_database(aURI)
{
  let url = aURI instanceof Ci.nsIURI ? aURI.spec : aURI;
  let stmt = DBConn().createStatement(
    `SELECT count(*) FROM moz_historyvisits v
     JOIN moz_places h ON h.id = v.place_id
     WHERE url = :url`
  );
  stmt.params.url = url;
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











function promiseTopicObserved(aTopic)
{
  let deferred = Promise.defer();

  Services.obs.addObserver(
    function PTO_observe(aSubject, aTopic, aData) {
      Services.obs.removeObserver(PTO_observe, aTopic);
      deferred.resolve([aSubject, aData]);
    }, aTopic, false);

  return deferred.promise;
}








function promiseClearHistory() {
  let promise = promiseTopicObserved(PlacesUtils.TOPIC_EXPIRATION_FINISHED);
  do_execute_soon(function() PlacesUtils.bhistory.removeAllPages());
  return promise;
}





function shutdownPlaces(aKeepAliveConnection)
{
  let hs = PlacesUtils.history.QueryInterface(Ci.nsIObserver);
  hs.observe(null, "profile-change-teardown", null);
  hs.observe(null, "profile-before-change", null);
}

const FILENAME_BOOKMARKS_HTML = "bookmarks.html";
let (backup_date = new Date().toLocaleFormat("%Y-%m-%d")) {
  const FILENAME_BOOKMARKS_JSON = "bookmarks-" + backup_date + ".json";
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
  let bookmarksBackupDir = gProfD.clone();
  bookmarksBackupDir.append("bookmarkbackups");
  if (!bookmarksBackupDir.exists()) {
    bookmarksBackupDir.create(Ci.nsIFile.DIRECTORY_TYPE, parseInt("0755", 8));
    do_check_true(bookmarksBackupDir.exists());
  }
  let profileBookmarksJSONFile = bookmarksBackupDir.clone();
  profileBookmarksJSONFile.append(FILENAME_BOOKMARKS_JSON);
  if (profileBookmarksJSONFile.exists()) {
    profileBookmarksJSONFile.remove();
  }
  let bookmarksJSONFile = gTestDir.clone();
  bookmarksJSONFile.append(aFilename);
  do_check_true(bookmarksJSONFile.exists());
  bookmarksJSONFile.copyTo(bookmarksBackupDir, FILENAME_BOOKMARKS_JSON);
  profileBookmarksJSONFile = bookmarksBackupDir.clone();
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









function check_JSON_backup(aIsAutomaticBackup) {
  let profileBookmarksJSONFile;
  if (aIsAutomaticBackup) {
    let bookmarksBackupDir = gProfD.clone();
    bookmarksBackupDir.append("bookmarkbackups");
    let files = bookmarksBackupDir.directoryEntries;
    let backup_date = new Date().toLocaleFormat("%Y-%m-%d");
    while (files.hasMoreElements()) {
      let entry = files.getNext().QueryInterface(Ci.nsIFile);
      if (PlacesBackups.filenamesRegex.test(entry.leafName)) {
        profileBookmarksJSONFile = entry;
        break;
      }
    }
  } else {
    profileBookmarksJSONFile = gProfD.clone();
    profileBookmarksJSONFile.append("bookmarkbackups");
    profileBookmarksJSONFile.append(FILENAME_BOOKMARKS_JSON);
  }
  do_check_true(profileBookmarksJSONFile.exists());
  return profileBookmarksJSONFile;
}








function frecencyForUrl(aURI)
{
  let url = aURI instanceof Ci.nsIURI ? aURI.spec : aURI;
  let stmt = DBConn().createStatement(
    "SELECT frecency FROM moz_places WHERE url = ?1"
  );
  stmt.bindByIndex(0, url);
  try {
    if (!stmt.executeStep()) {
      throw new Error("No result for frecency.");
    }
    return stmt.getInt32(0);
  } finally {
    stmt.finalize();
  }
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














function promiseAsyncUpdates()
{
  let deferred = Promise.defer();

  let db = DBConn();
  let begin = db.createAsyncStatement("BEGIN EXCLUSIVE");
  begin.executeAsync();
  begin.finalize();

  let commit = db.createAsyncStatement("COMMIT");
  commit.executeAsync({
    handleResult: function () {},
    handleError: function () {},
    handleCompletion: function(aReason)
    {
      deferred.resolve();
    }
  });
  commit.finalize();

  return deferred.promise;
}







function waitForConnectionClosed(aCallback)
{
  Services.obs.addObserver(function WFCCCallback() {
    Services.obs.removeObserver(WFCCCallback, "places-connection-closed");
    aCallback();
  }, "places-connection-closed", false);
  shutdownPlaces();
}









function do_check_valid_places_guid(aGuid,
                                    aStack)
{
  if (!aStack) {
    aStack = Components.stack.caller;
  }
  do_check_true(/^[a-zA-Z0-9\-_]{12}$/.test(aGuid), aStack);
}










function do_get_guid_for_uri(aURI,
                             aStack)
{
  if (!aStack) {
    aStack = Components.stack.caller;
  }
  let stmt = DBConn().createStatement(
    `SELECT guid
     FROM moz_places
     WHERE url = :url`
  );
  stmt.params.url = aURI.spec;
  do_check_true(stmt.executeStep(), aStack);
  let guid = stmt.row.guid;
  stmt.finalize();
  do_check_valid_places_guid(guid, aStack);
  return guid;
}









function do_check_guid_for_uri(aURI,
                               aGUID)
{
  let caller = Components.stack.caller;
  let guid = do_get_guid_for_uri(aURI, caller);
  if (aGUID) {
    do_check_valid_places_guid(aGUID, caller);
    do_check_eq(guid, aGUID, caller);
  }
}










function do_get_guid_for_bookmark(aId,
                                  aStack)
{
  if (!aStack) {
    aStack = Components.stack.caller;
  }
  let stmt = DBConn().createStatement(
    `SELECT guid
     FROM moz_bookmarks
     WHERE id = :item_id`
  );
  stmt.params.item_id = aId;
  do_check_true(stmt.executeStep(), aStack);
  let guid = stmt.row.guid;
  stmt.finalize();
  do_check_valid_places_guid(guid, aStack);
  return guid;
}









function do_check_guid_for_bookmark(aId,
                                    aGUID)
{
  let caller = Components.stack.caller;
  let guid = do_get_guid_for_bookmark(aId, caller);
  if (aGUID) {
    do_check_valid_places_guid(aGUID, caller);
    do_check_eq(guid, aGUID, caller);
  }
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





function NavBookmarkObserver() {}

NavBookmarkObserver.prototype = {
  onBeginUpdateBatch: function () {},
  onEndUpdateBatch: function () {},
  onItemAdded: function () {},
  onItemRemoved: function () {},
  onItemChanged: function () {},
  onItemVisited: function () {},
  onItemMoved: function () {},
  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsINavBookmarkObserver,
  ])
};





function NavHistoryObserver() {}

NavHistoryObserver.prototype = {
  onBeginUpdateBatch: function () {},
  onEndUpdateBatch: function () {},
  onVisit: function () {},
  onTitleChanged: function () {},
  onDeleteURI: function () {},
  onClearHistory: function () {},
  onPageChanged: function () {},
  onDeleteVisits: function () {},
  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsINavHistoryObserver,
  ])
};






function NavHistoryResultObserver() {}

NavHistoryResultObserver.prototype = {
  batching: function () {},
  containerStateChanged: function () {},
  invalidateContainer: function () {},
  nodeAnnotationChanged: function () {},
  nodeDateAddedChanged: function () {},
  nodeHistoryDetailsChanged: function () {},
  nodeIconChanged: function () {},
  nodeInserted: function () {},
  nodeKeywordChanged: function () {},
  nodeLastModifiedChanged: function () {},
  nodeMoved: function () {},
  nodeRemoved: function () {},
  nodeTagsChanged: function () {},
  nodeTitleChanged: function () {},
  nodeURIChanged: function () {},
  sortingChanged: function () {},
  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsINavHistoryResultObserver,
  ])
};



















function promiseAddVisits(aPlaceInfo)
{
  let deferred = Promise.defer();
  let places = [];
  if (aPlaceInfo instanceof Ci.nsIURI) {
    places.push({ uri: aPlaceInfo });
  }
  else if (Array.isArray(aPlaceInfo)) {
    places = places.concat(aPlaceInfo);
  } else {
    places.push(aPlaceInfo)
  }

  
  let now = Date.now();
  for (let i = 0; i < places.length; i++) {
    if (!places[i].title) {
      places[i].title = "test visit for " + places[i].uri.spec;
    }
    places[i].visits = [{
      transitionType: places[i].transition === undefined ? TRANSITION_LINK
                                                         : places[i].transition,
      visitDate: places[i].visitDate || (now++) * 1000,
      referrerURI: places[i].referrer
    }];
  }

  PlacesUtils.asyncHistory.updatePlaces(
    places,
    {
      handleError: function AAV_handleError(aResultCode, aPlaceInfo) {
        let ex = new Components.Exception("Unexpected error in adding visits.",
                                          aResultCode);
        deferred.reject(ex);
      },
      handleResult: function () {},
      handleCompletion: function UP_handleCompletion() {
        deferred.resolve();
      }
    }
  );

  return deferred.promise;
}









function promiseIsURIVisited(aURI) {
  let deferred = Promise.defer();

  PlacesUtils.asyncHistory.isURIVisited(aURI, function(aURI, aIsVisited) {
    deferred.resolve(aIsVisited);
  });

  return deferred.promise;
}








function promiseSetIconForPage(aPageURI, aIconURI) {
  let deferred = Promise.defer();
  PlacesUtils.favicons.setAndFetchFaviconForPage(
    aPageURI, aIconURI, true,
    PlacesUtils.favicons.FAVICON_LOAD_NON_PRIVATE,
    () => { deferred.resolve(); });
  return deferred.promise;
}
