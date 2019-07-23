





































function run_test()
{
  var test;
  
  try {
    test = zipW.file;
    do_throw("Should have thrown unitialised error.");
  }
  catch (e) {
    do_check_eq(e.result, Components.results.NS_ERROR_NOT_INITIALIZED);
  }

  try {
    test = zipW.comment;
    do_throw("Should have thrown unitialised error.");
  }
  catch (e) {
    do_check_eq(e.result, Components.results.NS_ERROR_NOT_INITIALIZED);
  }

  try {
    zipW.comment = "test";
    do_throw("Should have thrown unitialised error.");
  }
  catch (e) {
    do_check_eq(e.result, Components.results.NS_ERROR_NOT_INITIALIZED);
  }

  try {
    zipW.addEntryDirectory("test", 0, false);
    do_throw("Should have thrown unitialised error.");
  }
  catch (e) {
    do_check_eq(e.result, Components.results.NS_ERROR_NOT_INITIALIZED);
  }

  try {
    zipW.addEntryFile("test", Ci.nsIZipWriter.COMPRESSION_DEFAULT, tmpDir, false);
    do_throw("Should have thrown unitialised error.");
  }
  catch (e) {
    do_check_eq(e.result, Components.results.NS_ERROR_NOT_INITIALIZED);
  }

  try {
    zipW.removeEntry("test", false);
    do_throw("Should have thrown unitialised error.");
  }
  catch (e) {
    do_check_eq(e.result, Components.results.NS_ERROR_NOT_INITIALIZED);
  }

  try {
    zipW.processQueue(null, null);
    do_throw("Should have thrown unitialised error.");
  }
  catch (e) {
    do_check_eq(e.result, Components.results.NS_ERROR_NOT_INITIALIZED);
  }

  try {
    zipW.close();
    do_throw("Should have thrown unitialised error.");
  }
  catch (e) {
    do_check_eq(e.result, Components.results.NS_ERROR_NOT_INITIALIZED);
  }
}
