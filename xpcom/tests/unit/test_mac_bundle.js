function run_test() { 
  
  
  var thisFile = do_get_file("xpcom/tests/unit/test_mac_bundle.js");
  if (!thisFile instanceof Components.interfaces.nsILocalFileMac)
    return;
  
  
  
  var keynoteBundle = do_get_file("xpcom/tests/unit/data/presentation.key");
  var appBundle = do_get_file("xpcom/tests/unit/data/SmallApp.app");
  
  do_check_true(keynoteBundle instanceof Components.interfaces.nsILocalFileMac);
  do_check_true(appBundle instanceof Components.interfaces.nsILocalFileMac);
  
  do_check_true(keynoteBundle.isPackage());
  do_check_true(appBundle.isPackage());
}
