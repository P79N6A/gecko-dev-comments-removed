









































function run_test() {
  

  const mimeService = Cc["@mozilla.org/mime;1"].
                      getService(Ci.nsIMIMEService);

  const categoryManager = Cc["@mozilla.org/categorymanager;1"].
                          getService(Ci.nsICategoryManager);

  

  const kTestExtension          = "testextension";
  const kTestExtensionMixedCase = "testExtensIon";
  const kTestMimeType           = "application/x-testextension";

  
  
  try {
    
    mimeService.getTypeFromExtension(kTestExtension);
    
    do_throw("nsIMIMEService.getTypeFromExtension succeeded unexpectedly");
  } catch (e if (e instanceof Ci.nsIException &&
                 e.result == Cr.NS_ERROR_NOT_AVAILABLE)) {
    
    
  }

  
  categoryManager.addCategoryEntry("ext-to-type-mapping", kTestExtension,
                                   kTestMimeType, false, true);

  
  var type = mimeService.getTypeFromExtension(kTestExtension);
  do_check_eq(type, kTestMimeType);

  
  type = mimeService.getTypeFromExtension(kTestExtensionMixedCase);
  do_check_eq(type, kTestMimeType);

  
  categoryManager.deleteCategoryEntry("ext-to-type-mapping", kTestExtension, false);
}
