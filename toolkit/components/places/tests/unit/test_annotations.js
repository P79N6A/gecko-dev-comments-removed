







































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
  do_check_eq(storageType.value, Ci.nsIAnnotationService.TYPE_STRING);

  
  var annoNames = annosvc.getPageAnnotationNames(testURI, {});
  do_check_eq(annoNames.length, 1);
  do_check_eq(annoNames[0], "moz-test-places/annotations");

  







  
  var int32Key = testAnnoName + "/types/Int32";
  var int32Val = 23;
  annosvc.setAnnotationInt32(testURI, int32Key, int32Val, 0, 0);
  var flags = {}, exp = {}, mimeType = {}, storageType = {};
  annosvc.getAnnotationInfo(testURI, int32Key, flags, exp, mimeType, storageType);
  do_check_eq(flags.value, 0);
  do_check_eq(exp.value, 0);
  do_check_eq(mimeType.value, null);
  do_check_eq(storageType.value, Ci.nsIAnnotationService.TYPE_INT32);
  var storedVal = annosvc.getAnnotationInt32(testURI, int32Key);
  do_check_eq(int32Val, storedVal);
  do_check_eq(typeof storedVal, "number");

  
  var int64Key = testAnnoName + "/types/Int64";
  var int64Val = 4294967296;
  annosvc.setAnnotationInt64(testURI, int64Key, int64Val, 0, 0);
  var flags = {}, exp = {}, mimeType = {}, storageType = {};
  annosvc.getAnnotationInfo(testURI, int64Key, flags, exp, mimeType, storageType);
  do_check_eq(flags.value, 0);
  do_check_eq(exp.value, 0);
  do_check_eq(mimeType.value, null);
  do_check_eq(storageType.value, Ci.nsIAnnotationService.TYPE_INT64);
  var storedVal = annosvc.getAnnotationInt64(testURI, int64Key);
  do_check_eq(int64Val, storedVal);
  do_check_eq(typeof storedVal, "number");

  
  var doubleKey = testAnnoName + "/types/Double";
  var doubleVal = 0.000002342;
  annosvc.setAnnotationDouble(testURI, doubleKey, doubleVal, 0, 0);
  var flags = {}, exp = {}, mimeType = {}, storageType = {};
  annosvc.getAnnotationInfo(testURI, doubleKey, flags, exp, mimeType, storageType);
  do_check_eq(flags.value, 0);
  do_check_eq(exp.value, 0);
  do_check_eq(mimeType.value, null);
  do_check_eq(storageType.value, Ci.nsIAnnotationService.TYPE_DOUBLE);
  var storedVal = annosvc.getAnnotationDouble(testURI, doubleKey);
  do_check_eq(doubleVal, storedVal);
  do_check_true(Math.round(storedVal) != storedVal);

  
  var binaryKey = testAnnoName + "/types/Binary";
  var binaryVal = Array.prototype.map.call("splarg", function(x) { return x.charCodeAt(0); });
  annosvc.setAnnotationBinary(testURI, binaryKey, binaryVal, binaryVal.length, "text/plain", 0, 0);
  var flags = {}, exp = {}, mimeType = {}, storageType = {};
  annosvc.getAnnotationInfo(testURI, binaryKey, flags, exp, mimeType, storageType);
  do_check_eq(flags.value, 0);
  do_check_eq(exp.value, 0);
  do_check_eq(mimeType.value, "text/plain");
  do_check_eq(storageType.value, Ci.nsIAnnotationService.TYPE_BINARY);
  var data = {}, length = {}, mimeType = {};
  annosvc.getAnnotationBinary(testURI, binaryKey, data, length, mimeType);
  do_check_eq(binaryVal.toString(), data.value.toString());
  do_check_eq(typeof data.value, "object");

  
  try {
    annosvc.getAnnotationString(testURI, int32Key);
    do_throw("annotation string accessor didn't throw for a wrong type!");
  } catch(ex) {}

  try {
    annosvc.getAnnotationInt32(testURI, int64Key);
    do_throw("annotation int32 accessor didn't throw for a wrong type!");
  } catch(ex) {}

  try {
    annosvc.getAnnotationInt64(testURI, int32Key);
    do_throw("annotation int64 accessor didn't throw for a wrong type!");
  } catch(ex) {}

  try {
    annosvc.getAnnotationDouble(testURI, int32Key);
    do_throw("annotation double accessor didn't throw for a wrong type!");
  } catch(ex) {}

  try {
    var data = {}, length = {}, mimeType = {};
    annosvc.getAnnotationBinary(testURI, int32Key, data, length, mimeType);
    do_throw("annotation binary accessor didn't throw for a wrong type!");
  } catch(ex) {}
}
