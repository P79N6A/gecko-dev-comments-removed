





Cu.import("resource://gre/modules/BookmarkHTMLUtils.jsm");







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
      Task.spawn(function () {
        this.file = yield promiseFile("bookmarks-test_restoreNotification.json");
        addBookmarks();

        yield BookmarkJSONUtils.exportToFile(this.file);
        remove_all_bookmarks();
        try {
          yield BookmarkJSONUtils.importFromFile(this.file, true);
        }
        catch (e) {
          do_throw("  Restore should not have failed");
        }
      }.bind(this));
    }
  },

  {
    desc:       "JSON restore: empty file should succeed",
    currTopic:  NSIOBSERVER_TOPIC_BEGIN,
    finalTopic: NSIOBSERVER_TOPIC_SUCCESS,
    data:       NSIOBSERVER_DATA_JSON,
    folderId:   null,
    run:        function () {
      Task.spawn(function() {
        this.file = yield promiseFile("bookmarks-test_restoreNotification.json");
        try {
          yield BookmarkJSONUtils.importFromFile(this.file, true);
        }
        catch (e) {
          do_throw("  Restore should not have failed" + e);
        }
      }.bind(this));
    }
  },

  {
    desc:       "JSON restore: nonexistent file should fail",
    currTopic:  NSIOBSERVER_TOPIC_BEGIN,
    finalTopic: NSIOBSERVER_TOPIC_FAILED,
    data:       NSIOBSERVER_DATA_JSON,
    folderId:   null,
    run:        function () {
      this.file = Services.dirsvc.get("ProfD", Ci.nsILocalFile);
      this.file.append("this file doesn't exist because nobody created it 1");
      Task.spawn(function() {
        try {
          yield BookmarkJSONUtils.importFromFile(this.file, true);
          do_throw("  Restore should have failed");
        }
        catch (e) {
        }
      }.bind(this));
    }
  },

  {
    desc:       "HTML restore: normal restore should succeed",
    currTopic:  NSIOBSERVER_TOPIC_BEGIN,
    finalTopic: NSIOBSERVER_TOPIC_SUCCESS,
    data:       NSIOBSERVER_DATA_HTML,
    folderId:   null,
    run:        function () {
      Task.spawn(function() {
        this.file = yield promiseFile("bookmarks-test_restoreNotification.html");
        addBookmarks();
        yield BookmarkHTMLUtils.exportToFile(this.file);
        remove_all_bookmarks();
        try {
          BookmarkHTMLUtils.importFromFile(this.file, false)
                           .then(null, do_report_unexpected_exception);
        }
        catch (e) {
          do_throw("  Restore should not have failed");
        }
      }.bind(this));
    }
  },

  {
    desc:       "HTML restore: empty file should succeed",
    currTopic:  NSIOBSERVER_TOPIC_BEGIN,
    finalTopic: NSIOBSERVER_TOPIC_SUCCESS,
    data:       NSIOBSERVER_DATA_HTML,
    folderId:   null,
    run:        function () {
      Task.spawn(function (){
        this.file = yield promiseFile("bookmarks-test_restoreNotification.init.html");
        try {
          BookmarkHTMLUtils.importFromFile(this.file, false)
                           .then(null, do_report_unexpected_exception);
        }
        catch (e) {
          do_throw("  Restore should not have failed");
        }
      }.bind(this));
    }
  },

  {
    desc:       "HTML restore: nonexistent file should fail",
    currTopic:  NSIOBSERVER_TOPIC_BEGIN,
    finalTopic: NSIOBSERVER_TOPIC_FAILED,
    data:       NSIOBSERVER_DATA_HTML,
    folderId:   null,
    run:        Task.async(function* () {
      this.file = Services.dirsvc.get("ProfD", Ci.nsILocalFile);
      this.file.append("this file doesn't exist because nobody created it 2");
      try {
        yield BookmarkHTMLUtils.importFromFile(this.file, false);
        do_throw("Should fail!");
      }
      catch (e) {}
    }.bind(this))
  },

  {
    desc:       "HTML initial restore: normal restore should succeed",
    currTopic:  NSIOBSERVER_TOPIC_BEGIN,
    finalTopic: NSIOBSERVER_TOPIC_SUCCESS,
    data:       NSIOBSERVER_DATA_HTML_INIT,
    folderId:   null,
    run:        function () {
      Task.spawn(function () {
        this.file = yield promiseFile("bookmarks-test_restoreNotification.init.html");
        addBookmarks();
        yield BookmarkHTMLUtils.exportToFile(this.file);
        remove_all_bookmarks();
        try {
          BookmarkHTMLUtils.importFromFile(this.file, true)
                           .then(null, do_report_unexpected_exception);
        }
        catch (e) {
          do_throw("  Restore should not have failed");
        }
      }.bind(this));
    }
  },

  {
    desc:       "HTML initial restore: empty file should succeed",
    currTopic:  NSIOBSERVER_TOPIC_BEGIN,
    finalTopic: NSIOBSERVER_TOPIC_SUCCESS,
    data:       NSIOBSERVER_DATA_HTML_INIT,
    folderId:   null,
    run:        function () {
      Task.spawn(function () {
        this.file = yield promiseFile("bookmarks-test_restoreNotification.init.html");
        try {
          BookmarkHTMLUtils.importFromFile(this.file, true)
                           .then(null, do_report_unexpected_exception);
        }
        catch (e) {
          do_throw("  Restore should not have failed");
        }
      }.bind(this));
    }
  },

  {
    desc:       "HTML initial restore: nonexistent file should fail",
    currTopic:  NSIOBSERVER_TOPIC_BEGIN,
    finalTopic: NSIOBSERVER_TOPIC_FAILED,
    data:       NSIOBSERVER_DATA_HTML_INIT,
    folderId:   null,
    run:        Task.async(function* () {
      this.file = Services.dirsvc.get("ProfD", Ci.nsILocalFile);
      this.file.append("this file doesn't exist because nobody created it 3");
      try {
        yield BookmarkHTMLUtils.importFromFile(this.file, true);
        do_throw("Should fail!");
      }
      catch (e) {}
    }.bind(this))
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
    do_execute_soon(doNextTest);
  }
};


var currTestIndex = -1;

var bmsvc = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
            getService(Ci.nsINavBookmarksService);

var obssvc = Cc["@mozilla.org/observer-service;1"].
             getService(Ci.nsIObserverService);






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









function promiseFile(aBasename) {
  let path = OS.Path.join(OS.Constants.Path.profileDir, aBasename);
  dump("\n\nopening " + path + "\n\n");
  return OS.File.open(path, { truncate: true }).then(aFile => { aFile.close(); return path; });
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
