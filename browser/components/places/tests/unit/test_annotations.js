







































try {
  var histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].getService(Ci.nsINavHistoryService);
} catch(ex) {
  do_throw("Could not get history service\n");
} 


try {
  var annosvc= Cc["@mozilla.org/browser/annotation-service;1"].getService(Ci.nsIAnnotationService);
} catch(ex) {
  do_throw("Could not get annotation service\n");
} 


function run_test() {
  
  var testURI = uri("http://mozilla.com/");
  var testAnnoName = "moz-test-places/annotations";
  var testAnnoVal = "test";

  
  try {
    annosvc.setAnnotationString(testURI, testAnnoName, testAnnoVal, 0, 0);
  } catch(ex) {
    do_throw("unable to add annotation");
  }

  
  var storedAnnoVal = annosvc.getAnnotationString(testURI, testAnnoName);
  do_check_eq(testAnnoVal, storedAnnoVal);

  
  try {
    annosvc.getAnnotationString(testURI, "blah");
    do_throw("fetching annotation that doesn't exist, should've thrown");
  } catch(ex) {}

  
  var flags = {}, exp = {}, mimeType = {}, storageType = {};
  annosvc.getAnnotationInfo(testURI, testAnnoName, flags, exp, mimeType, storageType);
  do_check_eq(flags.value, 0);
  do_check_eq(exp.value, 0);
  do_check_eq(mimeType.value, null);
  do_check_eq(storageType.value, Ci.mozIStorageValueArray.VALUE_TYPE_TEXT);

  
  var annoNames = annosvc.getPageAnnotationNames(testURI, {});
  do_check_eq(annoNames.length, 1);
  do_check_eq(annoNames[0], "moz-test-places/annotations");

  






}
