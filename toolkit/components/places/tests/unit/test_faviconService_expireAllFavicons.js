










































var hs = Cc["@mozilla.org/browser/nav-history-service;1"].
         getService(Ci.nsINavHistoryService);
var bh = hs.QueryInterface(Ci.nsIBrowserHistory);
var bs = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
         getService(Ci.nsINavBookmarksService);
var os = Cc["@mozilla.org/observer-service;1"].
         getService(Ci.nsIObserverService);
var icons = Cc["@mozilla.org/browser/favicon-service;1"].
            getService(Ci.nsIFaviconService);
var cs = Cc["@mozilla.org/network/cache-service;1"].
         getService(Ci.nsICacheService);

const TEST_URI = "http://test.com/";
const TEST_ICON_URI = "http://test.com/favicon.ico";

const TEST_BOOKMARK_URI = "http://bookmarked.test.com/";
const TEST_BOOKMARK_ICON_URI = "http://bookmarked.test.com/favicon.ico";

const kExpirationFinished = "places-favicons-expired";


var tests = [
  function() {
    dump("\n\nTest that expireAllFavicons works as expected.\n");
    setup();
    icons.expireAllFavicons();
  },
];

function setup() {

  
  remove_all_bookmarks();
  bh.removeAllPages();

  
  bs.insertBookmark(bs.toolbarFolder, uri(TEST_BOOKMARK_URI),
                    bs.DEFAULT_INDEX, "visited");
  
  icons.setFaviconUrlForPage(uri(TEST_BOOKMARK_URI),
                             uri(TEST_BOOKMARK_ICON_URI));
  
  try {
    do_check_eq(icons.getFaviconForPage(uri(TEST_BOOKMARK_URI)).spec,
                TEST_BOOKMARK_ICON_URI);
  } catch(ex) {
    do_throw(ex.message);
  }

  
  let visitId = hs.addVisit(uri(TEST_URI), Date.now() * 1000, null,
                            hs.TRANSITION_TYPED, false, 0);
  
  icons.setFaviconUrlForPage(uri(TEST_URI), uri(TEST_ICON_URI));
  
  try {
    do_check_eq(icons.getFaviconForPage(uri(TEST_URI)).spec, TEST_ICON_URI);
  } catch(ex) {
    do_throw(ex.message);
  }
}

var observer = {
  observe: function(aSubject, aTopic, aData) {
    if (aTopic == kExpirationFinished) {
      
      try {
        icons.getFaviconForPage(uri(TEST_URI));
        do_throw("Visited page has still a favicon!");
      } catch (ex) {  }

      
      try {
        icons.getFaviconForPage(uri(TEST_BOOKMARK_URI));
        do_throw("Bookmarked page has still a favicon!");
      } catch (ex) {  }
    
      
      if (tests.length)
        (tests.shift())();
      else {
        os.removeObserver(this, kExpirationFinished);
        do_test_finished();
      }
    }
  }
}
os.addObserver(observer, kExpirationFinished, false);

function run_test()
{
  do_test_pending();
  (tests.shift())();
}
