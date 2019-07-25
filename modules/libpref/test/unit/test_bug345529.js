
 



function run_test() {
  const Cc = Components.classes;
  const Ci = Components.interfaces;
  const PREF_NAME = "testPref";

  var prefs = Cc["@mozilla.org/preferences-service;1"]
              .getService(Ci.nsIPrefBranch);
  var observer = {
    QueryInterface: function QueryInterface(aIID) {
      if (aIID.equals(Ci.nsIObserver) ||
          aIID.equals(Ci.nsISupports))
         return this;
      throw Components.results.NS_NOINTERFACE;
    },

    observe: function observe(aSubject, aTopic, aState) {
      prefs.removeObserver(PREF_NAME, observer);
    }
  }
  prefs.addObserver(PREF_NAME, observer, false);

  prefs.setCharPref(PREF_NAME, "test0")
  
  
  
  prefs.setCharPref(PREF_NAME, "test1")

  do_check_true(true);
}
