




































var _PBSvc = null;
function get_PBSvc() {
  if (_PBSvc)
    return _PBSvc;

  try {
    _PBSvc = Components.classes["@mozilla.org/privatebrowsing;1"].
             getService(Components.interfaces.nsIPrivateBrowsingService);
    if (_PBSvc) {
      var observer = {
        QueryInterface: function (iid) {
          const interfaces = [Components.interfaces.nsIObserver,
                              Components.interfaces.nsISupports];
          if (!interfaces.some(function(v) iid.equals(v)))
            throw Components.results.NS_ERROR_NO_INTERFACE;
          return this;
        },
        observe: function (subject, topic, data) {
          subject.QueryInterface(Components.interfaces.nsISupportsPRUint32);
          subject.data = 0;
        }
      };
      var os = Components.classes["@mozilla.org/observer-service;1"].
               getService(Components.interfaces.nsIObserverService);
      os.addObserver(observer, "private-browsing-enter", false);
    }
    return _PBSvc;
  } catch (e) {}
  return null;
}

var _FHSvc = null;
function get_FormHistory() {
  if (_FHSvc)
    return _FHSvc;

  return _FHSvc = Components.classes["@mozilla.org/satchel/form-history;1"].
                  getService(Components.interfaces.nsIFormHistory2);
}

function run_test() {
  var pb = get_PBSvc();
  if (pb) { 
    var fh = get_FormHistory();
    do_check_neq(fh, null);

    
    fh.removeEntriesForName("pair-A");
    fh.removeEntriesForName("pair-B");

    
    fh.addEntry("pair-A", "value-A");
    
    do_check_true(fh.entryExists("pair-A", "value-A"));
    
    pb.privateBrowsingEnabled = true;
    
    do_check_true(fh.entryExists("pair-A", "value-A"));
    
    fh.addEntry("pair-B", "value-B");
    
    do_check_false(fh.entryExists("pair-B", "value-B"));
    
    pb.privateBrowsingEnabled = false;
    
    do_check_true(fh.entryExists("pair-A", "value-A"));
    
    do_check_false(fh.entryExists("pair-B", "value-B"));
  }
}
