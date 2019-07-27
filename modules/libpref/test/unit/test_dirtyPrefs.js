





const PREF_INVALID = 0;
const PREF_BOOL    = 128;
const PREF_INT     = 64;
const PREF_STRING  = 32;

function run_test() {

  var ps = Cc["@mozilla.org/preferences-service;1"].
            getService(Ci.nsIPrefService);

  let defaultBranch = ps.getDefaultBranch("");
  let userBranch = ps.getBranch("");

  let prefFile = do_get_profile();
  prefFile.append("prefs.js");

  
  
  ps.savePrefFile(prefFile);
  do_check_false(ps.dirty);

  
  userBranch.setBoolPref("DirtyTest.new.bool", true);
  do_check_true(ps.dirty);
  ps.savePrefFile(prefFile);
  
  userBranch.setBoolPref("DirtyTest.new.bool", true);
  do_check_false(ps.dirty);

  
  userBranch.setIntPref("DirtyTest.new.int", 1);
  do_check_true(ps.dirty);
  ps.savePrefFile(prefFile);
  
  userBranch.setIntPref("DirtyTest.new.int", 1);
  do_check_false(ps.dirty);

  userBranch.setCharPref("DirtyTest.new.char", "oop");
  do_check_true(ps.dirty);
  ps.savePrefFile(prefFile);
  
  userBranch.setCharPref("DirtyTest.new.char", "oop");
  do_check_false(ps.dirty);

  
  userBranch.setBoolPref("DirtyTest.new.char", false);
  do_check_true(ps.dirty);
  ps.savePrefFile(prefFile);

  
  defaultBranch.setBoolPref("DirtyTest.existing.bool", true);
  do_check_false(ps.dirty);
  
  do_check_throws(function() {
    userBranch.setCharPref("DirtyTest.existing.bool", "boo"); }, Cr.NS_ERROR_UNEXPECTED);
  do_check_false(ps.dirty);

  
  userBranch.setBoolPref("DirtyTest.existing.bool", true);
  do_check_false(ps.dirty);
  
  userBranch.setBoolPref("DirtyTest.existing.bool", false);
  do_check_true(ps.dirty);
  ps.savePrefFile(prefFile);
  
  userBranch.setBoolPref("DirtyTest.existing.bool", true);
  do_check_true(ps.dirty);
  ps.savePrefFile(prefFile);
}
