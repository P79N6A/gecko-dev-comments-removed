









































var iconsvc = PlacesUtils.favicons;

function addBookmark(aURI) {
  var bs = PlacesUtils.bookmarks;
  return bs.insertBookmark(bs.unfiledBookmarksFolder, aURI,
                           bs.DEFAULT_INDEX, aURI.spec);
}

function checkAddSucceeded(pageURI, mimetype, data) {
  var savedFaviconURI = iconsvc.getFaviconForPage(pageURI);
  var outMimeType = {};
  var outData = iconsvc.getFaviconData(savedFaviconURI, outMimeType, {});

  
  do_check_eq(mimetype, outMimeType.value);
  do_check_true(compareArrays(data, outData));
  do_check_guid_for_uri(pageURI);
}

var favicons = [
  {
    uri: uri(do_get_file("favicon-normal32.png")),
    data: readFileData(do_get_file("favicon-normal32.png")),
    mimetype: "image/png"
  }
];

var tests = [
  {
    desc: "test setAndLoadFaviconForPage for valid history uri",
    pageURI: uri("http://test1.bar/"),
    go: function go1() {
      this.favicon = favicons[0];

      iconsvc.setAndLoadFaviconForPage(this.pageURI, this.favicon.uri, true);
    },
    check: function check1() {
      checkAddSucceeded(this.pageURI, this.favicon.mimetype, this.favicon.data);
      do_log_info("Check that the added page is marked as hidden.");
      do_check_true(isUrlHidden(this.pageURI));
      do_log_info("Check that the added page has 0 frecency.");
      do_check_eq(frecencyForUrl(this.pageURI), 0);
    }
  },

  {
    desc: "test setAndLoadFaviconForPage for bookmarked about: URIs",
    pageURI: uri("about:test2"),
    favicon: favicons[0],
    go: function go3() {
      
      addBookmark(this.pageURI);

      iconsvc.setAndLoadFaviconForPage(this.pageURI, this.favicon.uri, true);
    },
    check: function check3() {
      checkAddSucceeded(this.pageURI, this.favicon.mimetype, this.favicon.data);
    }
  },

  {
    desc: "test setAndLoadFaviconForPage with history disabled for bookmarked URI",
    pageURI: uri("http://test3.bar/"),
    favicon: favicons[0],
    go: function go5() {
      
      var prefs = Cc["@mozilla.org/preferences-service;1"].getService(Ci.nsIPrefBranch);
      prefs.setBoolPref("places.history.enabled", false);

      
      addBookmark(this.pageURI);

      iconsvc.setAndLoadFaviconForPage(this.pageURI, this.favicon.uri, true);

      prefs.setBoolPref("places.history.enabled", true);
    },
    check: function check5() {
      checkAddSucceeded(this.pageURI, this.favicon.mimetype, this.favicon.data);
    }
  },

];

if ("@mozilla.org/privatebrowsing;1" in Cc) {
  tests.push({
    desc: "test setAndLoadFaviconForPage in PB mode for bookmarked URI",
    pageURI: uri("http://test4.bar/"),
    favicon: favicons[0],
    go: function go6() {
      try {
      
      var pb = Cc["@mozilla.org/privatebrowsing;1"].
               getService(Ci.nsIPrivateBrowsingService);
      var prefs = Cc["@mozilla.org/preferences-service;1"].getService(Ci.nsIPrefBranch);
      prefs.setBoolPref("browser.privatebrowsing.keep_current_session", true);
      pb.privateBrowsingEnabled = true;

      
      addBookmark(this.pageURI);

      iconsvc.setAndLoadFaviconForPage(this.pageURI, this.favicon.uri, true);

      pb.privateBrowsingEnabled = false;
      prefs.clearUserPref("browser.privatebrowsing.keep_current_session");
      } catch (ex) {do_throw("ex: " + ex); }
    },
    check: function check6() {
      checkAddSucceeded(this.pageURI, this.favicon.mimetype, this.favicon.data);
    }
  });
}
var historyObserver = {
  onBeginUpdateBatch: function() {},
  onEndUpdateBatch: function() {},
  onVisit: function() {},
  onTitleChanged: function() {},
  onBeforeDeleteURI: function() {},
  onDeleteURI: function() {},
  onClearHistory: function() {},
  onDeleteVisits: function() {},

  onPageChanged: function historyObserver_onPageChanged(pageURI, what, value) {
    if (what != Ci.nsINavHistoryObserver.ATTRIBUTE_FAVICON)
      return;

    if (pageURI.equals(tests[currentTestIndex].pageURI)) {
      tests[currentTestIndex].check();
      currentTestIndex++;
      if (currentTestIndex == tests.length) {
        do_test_finished();
      }
      else {
        do_log_info(tests[currentTestIndex].desc);
        tests[currentTestIndex].go();
      }
    }
    else
      do_throw("Received PageChanged for a non-current test!");
  },

  QueryInterface: function(iid) {
    if (iid.equals(Ci.nsINavHistoryObserver) ||
        iid.equals(Ci.nsISupports)) {
      return this;
    }
    throw Cr.NS_ERROR_NO_INTERFACE;
  }
};

var currentTestIndex = 0;
function run_test() {
  do_test_pending();

  
  do_check_eq(favicons[0].data.length, 344);

  PlacesUtils.history.addObserver(historyObserver, false);

  
  do_log_info(tests[currentTestIndex].desc);
  tests[currentTestIndex].go();
};
