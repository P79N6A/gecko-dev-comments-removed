



Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");


let os = Cc["@mozilla.org/observer-service;1"].
         getService(Ci.nsIObserverService);

let gDummyCreated = false;
let gDummyVisited = false;

let observer = {
  observe: function(subject, topic, data) {
    if (topic == "dummy-observer-created")
      gDummyCreated = true;
    else if (topic == "dummy-observer-visited")
      gDummyVisited = true;
  },

  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsIObserver,
    Ci.nsISupportsWeakReference,
  ])
};

function verify() {
  do_check_true(gDummyCreated);
  do_check_true(gDummyVisited);
  do_test_finished();
}


function run_test() {
  do_load_manifest("nsDummyObserver.manifest");

  os.addObserver(observer, "dummy-observer-created", true);
  os.addObserver(observer, "dummy-observer-visited", true);

  do_test_pending();

  
  promiseAddVisits(uri("http://typed.mozilla.org")).then(
            function () do_timeout(1000, verify));
}
