









































const PB_KEEP_SESSION_PREF = "browser.privatebrowsing.keep_current_session";

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "pb",
                                   "@mozilla.org/privatebrowsing;1",
                                   "nsIPrivateBrowsingService");

let favicons = [
  {
    uri: uri(do_get_file("favicon-normal16.png")),
    data: readFileData(do_get_file("favicon-normal16.png")),
    mimetype: "image/png",
    size: 286
  },
  {
    uri: uri(do_get_file("favicon-normal32.png")),
    data: readFileData(do_get_file("favicon-normal32.png")),
    mimetype: "image/png",
    size: 344
  },
];

let tests = [

  {
    desc: "test setAndLoadFaviconForPage for about: URIs",
    pageURI: uri("about:test1"),
    go: function go1() {

      PlacesUtils.favicons.setAndLoadFaviconForPage(this.pageURI, favicons[0].uri, true);
    },
    clean: function clean1() {}
  },

  {
    desc: "test setAndLoadFaviconForPage with history disabled",
    pageURI: uri("http://test2.bar/"),
    go: function go2() {
      
      Services.prefs.setBoolPref("places.history.enabled", false);

      PlacesUtils.favicons.setAndLoadFaviconForPage(this.pageURI, favicons[0].uri, true);
    },
    clean: function clean2() {
      try {
        Services.prefs.clearUserPref("places.history.enabled");
      } catch (ex) {}
    }
  },

  {
    desc: "test setAndLoadFaviconForPage in PB mode for non-bookmarked URI",
    pageURI: uri("http://test3.bar/"),
    go: function go3() {
      if (!("@mozilla.org/privatebrowsing;1" in Cc))
        return;

      
      Services.prefs.setBoolPref(PB_KEEP_SESSION_PREF, true);
      pb.privateBrowsingEnabled = true;

      PlacesUtils.favicons.setAndLoadFaviconForPage(this.pageURI, favicons[0].uri, true);
    },
    clean: function clean3() {
      if (!("@mozilla.org/privatebrowsing;1" in Cc))
        return;

      pb.privateBrowsingEnabled = false;
    }
  },

  { 
    desc: "test setAndLoadFaviconForPage for valid history uri",
    pageURI: uri("http://test4.bar/"),
    go: function go4() {
      PlacesUtils.favicons.setAndLoadFaviconForPage(this.pageURI, favicons[1].uri, true);
    },
    clean: function clean4() {}
  },

];

let historyObserver = {
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

    
    dump_table("moz_places_temp");
    dump_table("moz_favicons");

    
    do_check_true(pageURI.equals(uri("http://test4.bar/")));

    
    let stmt = DBConn().createStatement(
      "SELECT url FROM moz_favicons"
    );
    let c = 0;
    try {
      while (stmt.executeStep()) {
        do_check_eq(stmt.row.url, favicons[1].uri.spec);
        c++;
      }
    }
    finally {
      stmt.finalize();
    }
    do_check_eq(c, 1);

    do_test_finished();
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsINavHistoryObserver])
};

let currentTest = null;

function run_test() {
  do_test_pending();

  
  do_check_eq(favicons[0].data.length, favicons[0].size);
  do_check_eq(favicons[1].data.length, favicons[1].size);

  
  PlacesUtils.history.addObserver(historyObserver, false);

  
  
  
  runNextTest();
};

function runNextTest() {
  if (currentTest) {
    currentTest.clean();
  }

  if (tests.length) {
    currentTest = tests.shift();
    print(currentTest.desc);
    currentTest.go();
    
    
    do_timeout(500, runNextTest);
  }
}
