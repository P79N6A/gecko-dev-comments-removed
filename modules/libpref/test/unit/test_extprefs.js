


Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");



function arrayenumerator(a)
{
  return {
    i_: 0,
    QueryInterface: XPCOMUtils.generateQI([Ci.nsISimpleEnumerator]),
    hasMoreElements: function ae_hasMoreElements() {
      return this.i_ < a.length;
    },
    getNext: function ae_getNext() {
      return a[this.i_++];
    }
  };
}

function run_test() {
  var ps = Cc["@mozilla.org/preferences-service;1"].
    getService(Ci.nsIPrefService).QueryInterface(Ci.nsIPrefBranch);

  var extprefs = [do_get_file("extdata")];
  
  var extProvider = {
    QueryInterface: XPCOMUtils.generateQI([Ci.nsIDirectoryServiceProvider,
                                           Ci.nsIDirectoryServiceProvider2]),
    getFile: function ep_getFile() {
      throw Cr.NS_ERROR_FAILURE;
    },
    
    getFiles: function ep_getFiles(key) {
      if (key != "ExtPrefDL")
        throw Cr.NS_ERROR_FAILURE;

      return arrayenumerator(extprefs);
    }
  };
  
  let prefFile = do_get_file("data/testPref.js");

  do_check_throws(function() {
    ps.getBoolPref("testExtPref.bool");
  }, Cr.NS_ERROR_UNEXPECTED);
  do_check_throws(function() {
    ps.getBoolPref("testPref.bool1");
  }, Cr.NS_ERROR_UNEXPECTED);
  
  ps.readUserPrefs(prefFile);

  do_check_true(ps.getBoolPref("testPref.bool1"));
  ps.setBoolPref("testPref.bool1", false);
  do_check_false(ps.getBoolPref("testPref.bool1"));
  
  dirSvc.registerProvider(extProvider);
  Services.obs.notifyObservers(null, "load-extension-defaults", null);

  
  do_check_true(ps.getBoolPref("testExtPref.bool"));

  
  do_check_false(ps.getBoolPref("testPref.bool2"));

  
  do_check_false(ps.getBoolPref("testPref.bool1"));
}
