











































const NSIOBSERVER_TOPIC_BEGIN    = "bookmarks-restore-begin";
const NSIOBSERVER_TOPIC_SUCCESS  = "bookmarks-restore-success";
const NSIOBSERVER_TOPIC_FAILED   = "bookmarks-restore-failed";
const NSIOBSERVER_DATA_JSON      = "json";
const NSIOBSERVER_DATA_HTML      = "html";
const NSIOBSERVER_DATA_HTML_INIT = "html-initial";


var uris = [
  "http://example.com/1",
  "http://example.com/2",
  "http://example.com/3",
  "http://example.com/4",
  "http://example.com/5",
];













var tests = [
  {
    desc:       "JSON restore: normal restore should succeed",
    currTopic:  NSIOBSERVER_TOPIC_BEGIN,
    finalTopic: NSIOBSERVER_TOPIC_SUCCESS,
    data:       NSIOBSERVER_DATA_JSON,
    folderId:   null,
    run:        function () {
      this.file = createFile("bookmarks-test_restoreNotification.json");
      addBookmarks();
      PlacesUtils.backups.saveBookmarksToJSONFile(this.file);
      remove_all_bookmarks();
      try {
        PlacesUtils.restoreBookmarksFromJSONFile(this.file);
      }
      catch (e) {
        do_throw("  Restore should not have failed");
      }
    }
  },

  {
    desc:       "JSON restore: empty file should succeed",
    currTopic:  NSIOBSERVER_TOPIC_BEGIN,
    finalTopic: NSIOBSERVER_TOPIC_SUCCESS,
    data:       NSIOBSERVER_DATA_JSON,
    folderId:   null,
    run:        function () {
      this.file = createFile("bookmarks-test_restoreNotification.json");
      try {
        PlacesUtils.restoreBookmarksFromJSONFile(this.file);
      }
      catch (e) {
        do_throw("  Restore should not have failed");
      }
    }
  },

  {
    desc:       "JSON restore: nonexisting file should fail",
    currTopic:  NSIOBSERVER_TOPIC_BEGIN,
    finalTopic: NSIOBSERVER_TOPIC_FAILED,
    data:       NSIOBSERVER_DATA_JSON,
    folderId:   null,
    run:        function () {
      this.file = dirSvc.get("ProfD", Ci.nsILocalFile);
      this.file.append("this file doesn't exist because nobody created it");
      try {
        PlacesUtils.restoreBookmarksFromJSONFile(this.file);
        do_throw("  Restore should have failed");
      }
      catch (e) {}
    }
  },

  {
    desc:       "HTML restore: normal restore should succeed",
    currTopic:  NSIOBSERVER_TOPIC_BEGIN,
    finalTopic: NSIOBSERVER_TOPIC_SUCCESS,
    data:       NSIOBSERVER_DATA_HTML,
    folderId:   null,
    run:        function () {
      this.file = createFile("bookmarks-test_restoreNotification.html");
      addBookmarks();
      importer.exportHTMLToFile(this.file);
      remove_all_bookmarks();
      try {
        importer.importHTMLFromFile(this.file, false);
      }
      catch (e) {
        do_throw("  Restore should not have failed");
      }
    }
  },

  {
    desc:       "HTML restore: empty file should succeed",
    currTopic:  NSIOBSERVER_TOPIC_BEGIN,
    finalTopic: NSIOBSERVER_TOPIC_SUCCESS,
    data:       NSIOBSERVER_DATA_HTML,
    folderId:   null,
    run:        function () {
      this.file = createFile("bookmarks-test_restoreNotification.init.html");
      try {
        importer.importHTMLFromFile(this.file, false);
      }
      catch (e) {
        do_throw("  Restore should not have failed");
      }
    }
  },

  {
    desc:       "HTML restore: nonexisting file should fail",
    currTopic:  NSIOBSERVER_TOPIC_BEGIN,
    finalTopic: NSIOBSERVER_TOPIC_FAILED,
    data:       NSIOBSERVER_DATA_HTML,
    folderId:   null,
    run:        function () {
      this.file = dirSvc.get("ProfD", Ci.nsILocalFile);
      this.file.append("this file doesn't exist because nobody created it");
      try {
        importer.importHTMLFromFile(this.file, false);
        do_throw("  Restore should have failed");
      }
      catch (e) {}
    }
  },

  {
    desc:       "HTML initial restore: normal restore should succeed",
    currTopic:  NSIOBSERVER_TOPIC_BEGIN,
    finalTopic: NSIOBSERVER_TOPIC_SUCCESS,
    data:       NSIOBSERVER_DATA_HTML_INIT,
    folderId:   null,
    run:        function () {
      this.file = createFile("bookmarks-test_restoreNotification.init.html");
      addBookmarks();
      importer.exportHTMLToFile(this.file);
      remove_all_bookmarks();
      try {
        importer.importHTMLFromFile(this.file, true);
      }
      catch (e) {
        do_throw("  Restore should not have failed");
      }
    }
  },

  {
    desc:       "HTML initial restore: empty file should succeed",
    currTopic:  NSIOBSERVER_TOPIC_BEGIN,
    finalTopic: NSIOBSERVER_TOPIC_SUCCESS,
    data:       NSIOBSERVER_DATA_HTML_INIT,
    folderId:   null,
    run:        function () {
      this.file = createFile("bookmarks-test_restoreNotification.init.html");
      try {
        importer.importHTMLFromFile(this.file, true);
      }
      catch (e) {
        do_throw("  Restore should not have failed");
      }
    }
  },

  {
    desc:       "HTML initial restore: nonexisting file should fail",
    currTopic:  NSIOBSERVER_TOPIC_BEGIN,
    finalTopic: NSIOBSERVER_TOPIC_FAILED,
    data:       NSIOBSERVER_DATA_HTML_INIT,
    folderId:   null,
    run:        function () {
      this.file = dirSvc.get("ProfD", Ci.nsILocalFile);
      this.file.append("this file doesn't exist because nobody created it");
      try {
        importer.importHTMLFromFile(this.file, true);
        do_throw("  Restore should have failed");
      }
      catch (e) {}
    }
  },

  {
    desc:       "HTML restore into folder: normal restore should succeed",
    currTopic:  NSIOBSERVER_TOPIC_BEGIN,
    finalTopic: NSIOBSERVER_TOPIC_SUCCESS,
    data:       NSIOBSERVER_DATA_HTML,
    run:        function () {
      this.file = createFile("bookmarks-test_restoreNotification.html");
      addBookmarks();
      importer.exportHTMLToFile(this.file);
      remove_all_bookmarks();
      this.folderId = bmsvc.createFolder(bmsvc.unfiledBookmarksFolder,
                                         "test folder",
                                         bmsvc.DEFAULT_INDEX);
      print("  Sanity check: createFolder() should have succeeded");
      do_check_true(this.folderId > 0);
      try {
        importer.importHTMLFromFileToFolder(this.file, this.folderId, false);
      }
      catch (e) {
        do_throw("  Restore should not have failed");
      }
    }
  },

  {
    desc:       "HTML restore into folder: empty file should succeed",
    currTopic:  NSIOBSERVER_TOPIC_BEGIN,
    finalTopic: NSIOBSERVER_TOPIC_SUCCESS,
    data:       NSIOBSERVER_DATA_HTML,
    run:        function () {
      this.file = createFile("bookmarks-test_restoreNotification.init.html");
      this.folderId = bmsvc.createFolder(bmsvc.unfiledBookmarksFolder,
                                         "test folder",
                                         bmsvc.DEFAULT_INDEX);
      print("  Sanity check: createFolder() should have succeeded");
      do_check_true(this.folderId > 0);
      try {
        importer.importHTMLFromFileToFolder(this.file, this.folderId, false);
      }
      catch (e) {
        do_throw("  Restore should not have failed");
      }
    }
  },

  {
    desc:       "HTML restore into folder: nonexisting file should fail",
    currTopic:  NSIOBSERVER_TOPIC_BEGIN,
    finalTopic: NSIOBSERVER_TOPIC_FAILED,
    data:       NSIOBSERVER_DATA_HTML,
    run:        function () {
      this.file = dirSvc.get("ProfD", Ci.nsILocalFile);
      this.file.append("this file doesn't exist because nobody created it");
      this.folderId = bmsvc.createFolder(bmsvc.unfiledBookmarksFolder,
                                         "test folder",
                                         bmsvc.DEFAULT_INDEX);
      print("  Sanity check: createFolder() should have succeeded");
      do_check_true(this.folderId > 0);
      try {
        importer.importHTMLFromFileToFolder(this.file, this.folderId, false);
        do_throw("  Restore should have failed");
      }
      catch (e) {}
    }
  }
];


var beginObserver = {
  observe: function _beginObserver(aSubject, aTopic, aData) {
    var test = tests[currTestIndex];

    print("  Observed " + aTopic);
    print("  Topic for current test should be what is expected");
    do_check_eq(aTopic, test.currTopic);

    print("  Data for current test should be what is expected");
    do_check_eq(aData, test.data);

    
    test.currTopic = test.finalTopic;
  }
};



var successAndFailedObserver = {
  observe: function _successAndFailedObserver(aSubject, aTopic, aData) {
    var test = tests[currTestIndex];

    print("  Observed " + aTopic);
    print("  Topic for current test should be what is expected");
    do_check_eq(aTopic, test.currTopic);

    print("  Data for current test should be what is expected");
    do_check_eq(aData, test.data);

    
    try {
      test.file.remove(false);
    }
    catch (exc) {}

    
    
    if (aSubject) {
      do_check_eq(aSubject.QueryInterface(Ci.nsISupportsPRInt64).data,
                  test.folderId);
    }
    else
      do_check_eq(test.folderId, null);

    remove_all_bookmarks();
    doNextTest();
  }
};


var currTestIndex = -1;


Components.utils.import("resource://gre/modules/utils.js");

var bmsvc = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
            getService(Ci.nsINavBookmarksService);

var obssvc = Cc["@mozilla.org/observer-service;1"].
             getService(Ci.nsIObserverService);

var importer = Cc["@mozilla.org/browser/places/import-export-service;1"].
               getService(Ci.nsIPlacesImportExportService);






function addBookmarks() {
  uris.forEach(function (u) bmsvc.insertBookmark(bmsvc.bookmarksMenuFolder,
                                                 uri(u),
                                                 bmsvc.DEFAULT_INDEX,
                                                 u));
  checkBookmarksExist();
}






function checkBookmarksExist() {
  var hs = Cc["@mozilla.org/browser/nav-history-service;1"].
           getService(Ci.nsINavHistoryService);
  var queries = uris.map(function (u) {
    var q = hs.getNewQuery();
    q.uri = uri(u);
    return q;
  });
  var options = hs.getNewQueryOptions();
  options.queryType = options.QUERY_TYPE_BOOKMARKS;
  var root = hs.executeQueries(queries, uris.length, options).root;
  root.containerOpen = true;
  do_check_eq(root.childCount, uris.length);
  root.containerOpen = false;
}








function createFile(aBasename) {
  var file = dirSvc.get("ProfD", Ci.nsILocalFile);
  file.append(aBasename);
  if (file.exists())
    file.remove(false);
  file.create(file.NORMAL_FILE_TYPE, 0666);
  if (!file.exists())
    do_throw("Couldn't create file: " + aBasename);
  return file;
}




function doNextTest() {
  currTestIndex++;
  if (currTestIndex >= tests.length) {
    obssvc.removeObserver(beginObserver, NSIOBSERVER_TOPIC_BEGIN);
    obssvc.removeObserver(successAndFailedObserver, NSIOBSERVER_TOPIC_SUCCESS);
    obssvc.removeObserver(successAndFailedObserver, NSIOBSERVER_TOPIC_FAILED);
    do_test_finished();
  }
  else {
    var test = tests[currTestIndex];
    print("Running test: " + test.desc);
    test.run();
  }
}



function run_test() {
  do_test_pending();
  obssvc.addObserver(beginObserver, NSIOBSERVER_TOPIC_BEGIN, false);
  obssvc.addObserver(successAndFailedObserver, NSIOBSERVER_TOPIC_SUCCESS, false);
  obssvc.addObserver(successAndFailedObserver, NSIOBSERVER_TOPIC_FAILED, false);
  doNextTest();
}
