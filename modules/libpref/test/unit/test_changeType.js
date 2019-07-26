





const PREF_INVALID = 0;
const PREF_BOOL    = 128;
const PREF_INT     = 64;
const PREF_STRING  = 32;

function run_test() {

  var ps = Cc["@mozilla.org/preferences-service;1"].
            getService(Ci.nsIPrefService);

  let defaultBranch = ps.getDefaultBranch("");
  let userBranch = ps.getBranch("");

  
  

  defaultBranch.setBoolPref("TypeTest.existing.bool", true);
  defaultBranch.setIntPref("TypeTest.existing.int", 23);
  defaultBranch.setCharPref("TypeTest.existing.char", "hey");

  
  do_check_eq(userBranch.getBoolPref("TypeTest.existing.bool"), true);
  do_check_eq(userBranch.getIntPref("TypeTest.existing.int"), 23);
  do_check_eq(userBranch.getCharPref("TypeTest.existing.char"), "hey");

  
  do_check_throws(function() {
    userBranch.setCharPref("TypeTest.existing.bool", "boo"); }, Cr.NS_ERROR_UNEXPECTED);
  do_check_throws(function() {
    userBranch.setIntPref("TypeTest.existing.bool", 5); }, Cr.NS_ERROR_UNEXPECTED);
  do_check_throws(function() {
    userBranch.setCharPref("TypeTest.existing.int", "boo"); }, Cr.NS_ERROR_UNEXPECTED);
  do_check_throws(function() {
    userBranch.setBoolPref("TypeTest.existing.int", true); }, Cr.NS_ERROR_UNEXPECTED);
  do_check_throws(function() {
    userBranch.setBoolPref("TypeTest.existing.char", true); }, Cr.NS_ERROR_UNEXPECTED);
  do_check_throws(function() {
    userBranch.setIntPref("TypeTest.existing.char", 6); }, Cr.NS_ERROR_UNEXPECTED);


  
  
  let pref = "TypeTest.user";
  userBranch.setBoolPref(pref, true);
  userBranch.setCharPref(pref, "yay");
  do_check_eq(userBranch.getCharPref(pref), "yay");
  userBranch.setIntPref(pref, 7);
  do_check_eq(userBranch.getIntPref(pref), 7);
  userBranch.setBoolPref(pref, false);
  do_check_eq(userBranch.getBoolPref(pref), false);
  userBranch.setIntPref(pref, 8);
  do_check_eq(userBranch.getIntPref(pref), 8);
  userBranch.setCharPref(pref, "whee");
  do_check_eq(userBranch.getCharPref(pref), "whee");
  userBranch.setBoolPref(pref, true);
  do_check_eq(userBranch.getBoolPref(pref), true);
}
