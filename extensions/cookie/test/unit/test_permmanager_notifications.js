const Cc = Components.classes;
const Ci = Components.interfaces;


var dir = do_get_profile();


var pm = Cc["@mozilla.org/permissionmanager;1"]
          .getService(Ci.nsIPermissionManager);

var ios = Cc["@mozilla.org/network/io-service;1"]
          .getService(Ci.nsIIOService);
var permURI = ios.newURI("http://example.com", null, null);

var theTime = (new Date()).getTime();

var numadds = 0;
var numchanges = 0;
var numdeletes = 0;
var needsToClear = true;


var observer = {
  QueryInterface: 
  function(iid) {
    if (iid.equals(Ci.nsISupports) || 
        iid.equals(Ci.nsIObserver))
      return this;
    throw Components.results.NS_ERROR_NO_INTERFACE; 
  },

  observe:
  function(subject, topic, data) {
    if (topic !== "perm-changed")
      return;

    
    
    
    
    if (data == "added") {
      var perm = subject.QueryInterface(Ci.nsIPermission);
      numadds++;
      switch (numadds) {
        case 1: 
          do_check_eq(pm.EXPIRE_TIME, perm.expireType);
          do_check_eq(theTime + 10000, perm.expireTime);
          break;
        case 2: 
          do_check_eq(pm.EXPIRE_NEVER, perm.expireType);
          do_check_eq(pm.DENY_ACTION, perm.capability);
          break;
        default:
          do_throw("too many add notifications posted.");
      }
      do_test_finished();

    } else if (data == "changed") {
      var perm = subject.QueryInterface(Ci.nsIPermission);
      numchanges++;
      switch (numchanges) {
        case 1:
          do_check_eq(pm.EXPIRE_TIME, perm.expireType);
          do_check_eq(theTime + 20000, perm.expireTime);
          break;
        default:
          do_throw("too many change notifications posted.");
      }
      do_test_finished();

    } else if (data == "deleted") {
      var perm = subject.QueryInterface(Ci.nsIPermission);
      numdeletes++;
      switch (numdeletes) {
        case 1:
          do_check_eq("test/permission-notify", perm.type);
          break;
        default:
          do_throw("too many delete notifications posted.");
      }
      do_test_finished();

    } else if (data == "cleared") {
      
      do_check_true(needsToClear);
      needsToClear = false;
      do_test_finished();
    } else {
      dump("subject: " + subject + "  data: " + data + "\n");
    }
  },
};

function run_test() {

  var obs = Cc["@mozilla.org/observer-service;1"].getService()
            .QueryInterface(Ci.nsIObserverService);

  obs.addObserver(observer, "perm-changed", false);

  
  do_test_pending(); 
  pm.add(permURI, "test/expiration-perm", pm.ALLOW_ACTION, pm.EXPIRE_TIME, theTime + 10000);

  do_test_pending(); 
  pm.add(permURI, "test/expiration-perm", pm.ALLOW_ACTION, pm.EXPIRE_TIME, theTime + 20000);

  do_test_pending(); 
  pm.add(permURI, "test/permission-notify", pm.DENY_ACTION);

  do_test_pending(); 
  pm.remove(permURI.asciiHost, "test/permission-notify");

  do_test_pending(); 
  pm.removeAll();

  do_timeout(100, cleanup);
}

function cleanup() {
  obs.removeObserver(observer, "perm-changed");
}
