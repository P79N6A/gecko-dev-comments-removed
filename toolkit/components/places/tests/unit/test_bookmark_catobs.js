



































Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");


let bs = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
         getService(Ci.nsINavBookmarksService);
let os = Cc["@mozilla.org/observer-service;1"].
         getService(Ci.nsIObserverService);

let gDummyCreated = false;
let gDummyAdded = false;

let observer = {
  observe: function(subject, topic, data) {
    if (topic == "dummy-observer-created")
      gDummyCreated = true;
    else if (topic == "dummy-observer-item-added")
      gDummyAdded = true;
  },

  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsIObserver,
    Ci.nsISupportsWeakReference,
  ])
};

function verify() {
  do_check_true(gDummyCreated);
  do_check_true(gDummyAdded);
  do_test_finished();
}


function run_test() {
  do_load_module("nsDummyObserver.js");

  os.addObserver(observer, "dummy-observer-created", true);
  os.addObserver(observer, "dummy-observer-item-added", true);

  
  bs.insertBookmark(bs.unfiledBookmarksFolder, uri("http://typed.mozilla.org"),
                    bs.DEFAULT_INDEX, "bookmark");

  do_test_pending();
  do_timeout(1000, verify);
}
