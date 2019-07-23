function run_test() { 
  
  
  if (!("nsILocalFileMac" in Components.interfaces))
    return;
  
  
  
  var keynoteBundle = do_get_file("xpcom/tests/unit/data/presentation.key");
  var appBundle = do_get_file("xpcom/tests/unit/data/SmallApp.app");
  
  do_check_true(keynoteBundle instanceof Components.interfaces.nsILocalFileMac);
  do_check_true(appBundle instanceof Components.interfaces.nsILocalFileMac);
  
  do_check_true(keynoteBundle.isPackage());
  do_check_true(appBundle.isPackage());
}
