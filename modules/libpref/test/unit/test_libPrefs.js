



const PREF_INVALID = 0;
const PREF_BOOL    = 128;
const PREF_INT     = 64;
const PREF_STRING  = 32;

const MAX_PREF_LENGTH = 1 * 1024 * 1024;

function makeList(a)
{
  var o = {};
  for(var i=0; i<a.length; i++)
  {
    o[a[i]] = '';
  }
  return o;
}

function run_test() {

  var ps = Cc["@mozilla.org/preferences-service;1"].
            getService(Ci.nsIPrefService);

  var pb2= Cc["@mozilla.org/preferences-service;1"].
            getService(Ci.nsIPrefBranch);

  var pb = Cc["@mozilla.org/preferences-service;1"].
            getService(Ci.nsIPrefBranch);

  
  

  do_check_throws(function() {
    pb.getPrefType(null); },  Cr.NS_ERROR_INVALID_ARG);
  do_check_throws(function() {
    pb.getBoolPref(null); },  Cr.NS_ERROR_INVALID_ARG);
  do_check_throws(function() {
    pb.setBoolPref(null, false); },  Cr.NS_ERROR_INVALID_ARG);
  do_check_throws(function() {
    pb.getIntPref(null); },  Cr.NS_ERROR_INVALID_ARG);
  do_check_throws(function() {
    pb.setIntPref(null, 0); },  Cr.NS_ERROR_INVALID_ARG);
  do_check_throws(function() {
    pb.getCharPref(null); },  Cr.NS_ERROR_INVALID_ARG);
  do_check_throws(function() {
    pb.setCharPref(null, null); },  Cr.NS_ERROR_INVALID_ARG);
  do_check_throws(function() {
    pb.getComplexValue(null, Components.interfaces.nsISupportsString); },  Cr.NS_ERROR_INVALID_ARG);
  do_check_throws(function() {
    pb.setComplexValue(null, Components.interfaces.nsISupportsString, pb); },  Cr.NS_ERROR_INVALID_ARG);
  do_check_throws(function() {
    pb.clearUserPref(null); },  Cr.NS_ERROR_INVALID_ARG);
  do_check_throws(function() {
    pb.prefHasUserValue(null); },  Cr.NS_ERROR_INVALID_ARG);
  do_check_throws(function() {
    pb.lockPref(null); },  Cr.NS_ERROR_INVALID_ARG);
  do_check_throws(function() {
    pb.prefIsLocked(null); },  Cr.NS_ERROR_INVALID_ARG);
  do_check_throws(function() {
    pb.unlockPref(null); },  Cr.NS_ERROR_INVALID_ARG);
  do_check_throws(function() {
    pb.deleteBranch(null); },  Cr.NS_ERROR_INVALID_ARG);
  do_check_throws(function() {
    pb.getChildList(null); },  Cr.NS_ERROR_INVALID_ARG);

  
  

  do_check_eq(pb.prefHasUserValue("UserPref.nonexistent.hasUserValue"), false);
  pb.clearUserPref("UserPref.nonexistent.clearUserPref"); 
  do_check_eq(pb.getPrefType("UserPref.nonexistent.getPrefType"), PREF_INVALID);
  do_check_eq(pb.root, "");

  
  do_check_throws(function() {
    pb.getBoolPref("UserPref.nonexistent.getBoolPref");}, Cr.NS_ERROR_UNEXPECTED);
  pb.setBoolPref("UserPref.nonexistent.setBoolPref", false);
  do_check_eq(pb.getBoolPref("UserPref.nonexistent.setBoolPref"), false);

  
  do_check_throws(function() {
    pb.getIntPref("UserPref.nonexistent.getIntPref");}, Cr.NS_ERROR_UNEXPECTED);
  pb.setIntPref("UserPref.nonexistent.setIntPref", 5);
  do_check_eq(pb.getIntPref("UserPref.nonexistent.setIntPref"), 5);

  
  do_check_throws(function() {
    pb.getCharPref("UserPref.nonexistent.getCharPref");}, Cr.NS_ERROR_UNEXPECTED);
  pb.setCharPref("UserPref.nonexistent.setCharPref", "_test");
  do_check_eq(pb.getCharPref("UserPref.nonexistent.setCharPref"), "_test");

  
  

  pb.setBoolPref("UserPref.existing.bool", true);
  pb.setIntPref("UserPref.existing.int", 23);
  pb.setCharPref("UserPref.existing.char", "hey");

  
  do_check_eq(pb.getBoolPref("UserPref.existing.bool"), true);
  do_check_eq(pb.getIntPref("UserPref.existing.int"), 23);
  do_check_eq(pb.getCharPref("UserPref.existing.char"), "hey");

  
  pb.setBoolPref("UserPref.existing.bool", false);
  do_check_eq(pb.getBoolPref("UserPref.existing.bool"), false);
  pb.setIntPref("UserPref.existing.int", 24);
  do_check_eq(pb.getIntPref("UserPref.existing.int"), 24);
  pb.setCharPref("UserPref.existing.char", "hej då!");
  do_check_eq(pb.getCharPref("UserPref.existing.char"), "hej då!");

  
  do_check_true(pb.prefHasUserValue("UserPref.existing.bool"));
  do_check_true(pb.prefHasUserValue("UserPref.existing.int"));
  do_check_true(pb.prefHasUserValue("UserPref.existing.char"));

  
  pb.clearUserPref("UserPref.existing.bool");
  do_check_false(pb.prefHasUserValue("UserPref.existing.bool"));
  pb.clearUserPref("UserPref.existing.int");
  do_check_false(pb.prefHasUserValue("UserPref.existing.int"));
  pb.clearUserPref("UserPref.existing.char");
  do_check_false(pb.prefHasUserValue("UserPref.existing.char"));

  
  

  let largeStr = new Array(MAX_PREF_LENGTH + 1).join('x');
  pb.setCharPref("UserPref.large.char", largeStr);
  largeStr += 'x';
  do_check_throws(function() {
    pb.setCharPref("UserPref.large.char", largeStr); }, Cr.NS_ERROR_ILLEGAL_VALUE);

  
  

  
  pb.setBoolPref("UserPref.getPrefType.bool", true);
  do_check_eq(pb.getPrefType("UserPref.getPrefType.bool"), PREF_BOOL);

  
  pb.setIntPref("UserPref.getPrefType.int", -234);
  do_check_eq(pb.getPrefType("UserPref.getPrefType.int"), PREF_INT);

  
  pb.setCharPref("UserPref.getPrefType.char", "testing1..2");
  do_check_eq(pb.getPrefType("UserPref.getPrefType.char"), PREF_STRING);

  
  
  
  do_check_eq(ps.root, "");

  
  pb.setBoolPref("UserPref.root.boolPref", true);
  let pb_1 = ps.getBranch("UserPref.root.");
  do_check_eq(pb_1.getBoolPref("boolPref"), true);
  let pb_2 = ps.getBranch("UserPref.root.boolPref");
  do_check_eq(pb_2.getBoolPref(""), true);
  pb_2.setBoolPref(".anotherPref", false);
  let pb_3 = ps.getBranch("UserPref.root.boolPre");
  do_check_eq(pb_3.getBoolPref("f.anotherPref"), false);

  
  pb.setIntPref("UserPref.root.intPref", 23);
  pb_1 = ps.getBranch("UserPref.root.");
  do_check_eq(pb_1.getIntPref("intPref"), 23);
  pb_2 = ps.getBranch("UserPref.root.intPref");
  do_check_eq(pb_2.getIntPref(""), 23);
  pb_2.setIntPref(".anotherPref", 69);
  pb_3 = ps.getBranch("UserPref.root.intPre");
  do_check_eq(pb_3.getIntPref("f.anotherPref"), 69);

  
  pb.setCharPref("UserPref.root.charPref", "_char");
  pb_1 = ps.getBranch("UserPref.root.");
  do_check_eq(pb_1.getCharPref("charPref"), "_char");
  pb_2 = ps.getBranch("UserPref.root.charPref");
  do_check_eq(pb_2.getCharPref(""), "_char");
  pb_2.setCharPref(".anotherPref", "_another");
  pb_3 = ps.getBranch("UserPref.root.charPre");
  do_check_eq(pb_3.getCharPref("f.anotherPref"), "_another");

  
  

  
  pb1 = ps.getBranch("UserPref.root.");
  let prefList = pb1.getChildList("");
  do_check_eq(prefList.length, 6);

  
  do_check_true("boolPref" in makeList(prefList));
  do_check_true("intPref" in makeList(prefList));
  do_check_true("charPref" in makeList(prefList));
  do_check_true("boolPref.anotherPref" in makeList(prefList));
  do_check_true("intPref.anotherPref" in makeList(prefList));
  do_check_true("charPref.anotherPref" in makeList(prefList));

  
  

  
  pb1 = ps.getDefaultBranch("");
  pb1.setBoolPref("DefaultPref.bool", true);
  do_check_eq(pb1.getBoolPref("DefaultPref.bool"), true);  
  do_check_false(pb1.prefHasUserValue("DefaultPref.bool"));
  ps.setBoolPref("DefaultPref.bool", false);
  do_check_true(pb1.prefHasUserValue("DefaultPref.bool"));
  do_check_eq(ps.getBoolPref("DefaultPref.bool"), false); 

  
  pb1 = ps.getDefaultBranch("");
  pb1.setIntPref("DefaultPref.int", 100);
  do_check_eq(pb1.getIntPref("DefaultPref.int"), 100);  
  do_check_false(pb1.prefHasUserValue("DefaultPref.int"));
  ps.setIntPref("DefaultPref.int", 50);
  do_check_true(pb1.prefHasUserValue("DefaultPref.int"));
  do_check_eq(ps.getIntPref("DefaultPref.int"), 50); 

  
  pb1 = ps.getDefaultBranch("");
  pb1.setCharPref("DefaultPref.char", "_default");
  do_check_eq(pb1.getCharPref("DefaultPref.char"), "_default");  
  do_check_false(pb1.prefHasUserValue("DefaultPref.char"));
  ps.setCharPref("DefaultPref.char", "_user");
  do_check_true(pb1.prefHasUserValue("DefaultPref.char"));
  do_check_eq(ps.getCharPref("DefaultPref.char"), "_user"); 

  
  

  
  do_check_throws(function() {
    ps.lockPref("DefaultPref.nonexistent");}, Cr.NS_ERROR_UNEXPECTED);
  do_check_throws(function() {
    ps.unlockPref("DefaultPref.nonexistent");}, Cr.NS_ERROR_UNEXPECTED);

  
  do_check_false(ps.prefIsLocked("DefaultPref.char"));
  ps.lockPref("DefaultPref.char");
  do_check_eq(ps.getCharPref("DefaultPref.char"), "_default"); 
  do_check_true(ps.prefIsLocked("DefaultPref.char"));

  
  ps.unlockPref("DefaultPref.char");
  do_check_eq(ps.getCharPref("DefaultPref.char"), "_user"); 
  do_check_false(ps.prefIsLocked("DefaultPref.char"));

  
  
  ps.setCharPref("DefaultPref.char", "_default");
  do_check_false(pb1.prefHasUserValue("DefaultPref.char"));

  
  ps.unlockPref("DefaultPref.char");
  ps.lockPref("DefaultPref.char");
  ps.lockPref("DefaultPref.char");

  
  

  
  
  




  
  

  
  
  

  ps.deleteBranch("DefaultPref");
  pb = ps.getBranch("DefaultPref");
  pb1 = ps.getDefaultBranch("DefaultPref");

  
  do_check_throws(function() {
    pb.getBoolPref("DefaultPref.bool");}, Cr.NS_ERROR_UNEXPECTED);
  do_check_throws(function() {
    pb.getIntPref("DefaultPref.int");}, Cr.NS_ERROR_UNEXPECTED);
  do_check_throws(function() {
    pb.getCharPref("DefaultPref.char");}, Cr.NS_ERROR_UNEXPECTED);

  
  do_check_throws(function() {
    pb1.getBoolPref("DefaultPref.bool");}, Cr.NS_ERROR_UNEXPECTED);
  do_check_throws(function() {
    pb1.getIntPref("DefaultPref.int");}, Cr.NS_ERROR_UNEXPECTED);
  do_check_throws(function() {
    pb1.getCharPref("DefaultPref.char");}, Cr.NS_ERROR_UNEXPECTED);

  
  

  
  ps.setBoolPref("ReadPref.bool", true);
  ps.setIntPref("ReadPref.int", 230);
  ps.setCharPref("ReadPref.char", "hello");

  
  let savePrefFile = do_get_cwd();
  savePrefFile.append("data");
  savePrefFile.append("savePref.js");
  if (savePrefFile.exists())
    savePrefFile.remove(false);
  savePrefFile.create(Ci.nsIFile.NORMAL_FILE_TYPE, 0666);
  ps.savePrefFile(savePrefFile);
  ps.resetPrefs();

  
  let prefFile = do_get_file("data/testPref.js");
  ps.readUserPrefs(prefFile);

  
  do_check_throws(function() {
    do_check_eq(pb.getBoolPref("ReadPref.bool"));}, Cr.NS_ERROR_UNEXPECTED);
  do_check_throws(function() {
    do_check_eq(pb.getIntPref("ReadPref.int"));}, Cr.NS_ERROR_UNEXPECTED);
  do_check_throws(function() {
    do_check_eq(pb.getCharPref("ReadPref.char"));}, Cr.NS_ERROR_UNEXPECTED);

  
  pb = ps.getBranch("testPref.");
  do_check_eq(pb.getBoolPref("bool1"), true);
  do_check_eq(pb.getBoolPref("bool2"), false);
  do_check_eq(pb.getIntPref("int1"), 23);
  do_check_eq(pb.getIntPref("int2"), -1236);
  do_check_eq(pb.getCharPref("char1"), "_testPref");
  do_check_eq(pb.getCharPref("char2"), "älskar");

  
  ps.readUserPrefs(savePrefFile);
  
  savePrefFile.remove(false);
  do_check_eq(ps.getBoolPref("ReadPref.bool"), true);
  do_check_eq(ps.getIntPref("ReadPref.int"), 230);
  do_check_eq(ps.getCharPref("ReadPref.char"), "hello");

  
  do_check_eq(pb.getBoolPref("bool1"), true);
  do_check_eq(pb.getBoolPref("bool2"), false);
  do_check_eq(pb.getIntPref("int1"), 23);

  
  

  
  var observer = {
    QueryInterface: function QueryInterface(aIID) {
      if (aIID.equals(Ci.nsIObserver) ||
          aIID.equals(Ci.nsISupports))
         return this;
      throw Components.results.NS_NOINTERFACE;
    },

    observe: function observe(aSubject, aTopic, aState) {
      do_check_eq(aTopic, "nsPref:changed");
      do_check_eq(aState, "ReadPref.int");
      do_check_eq(ps.getIntPref(aState), 76);
      ps.removeObserver("ReadPref.int", this);

      
      do_test_finished();
    }
  }

  pb2.addObserver("ReadPref.int", observer, false);
  ps.setIntPref("ReadPref.int", 76);

  
  do_test_pending();

  
  pb2.removeObserver("ReadPref.int", observer);
  ps.setIntPref("ReadPref.int", 32);

  
  pb2.getBranch("ReadPref.");
  pb2.addObserver("int", observer, false);
  ps.setIntPref("ReadPref.int", 76);

  
  do_test_pending();
}
