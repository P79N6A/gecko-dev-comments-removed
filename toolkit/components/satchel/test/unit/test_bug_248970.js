




































var _PBSvc = null;
function get_PBSvc() {
  if (_PBSvc)
    return _PBSvc;

  try {
    _PBSvc = Components.classes["@mozilla.org/privatebrowsing;1"].
             getService(Components.interfaces.nsIPrivateBrowsingService);
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
    var prefBranch = Cc["@mozilla.org/preferences-service;1"].
                     getService(Ci.nsIPrefBranch);
    prefBranch.setBoolPref("browser.privatebrowsing.keep_current_session", true);

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

    prefBranch.clearUserPref("browser.privatebrowsing.keep_current_session");
  }
}
