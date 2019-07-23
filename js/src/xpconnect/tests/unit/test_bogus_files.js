




































 
function test_BrokenFile(path, shouldThrow, expectedName) {
  var didThrow = false;
  try {
    Components.utils.import(path);
  } catch (ex) {
    var exceptionName = ex.name;
    print("ex: " + ex + "; name = " + ex.name);
    didThrow = true;
  }

  do_check_eq(didThrow, shouldThrow);
  if (didThrow)
    do_check_eq(exceptionName, expectedName);
}

function run_test() {
  const C_i = Components.interfaces;
  const ioService = Components.classes["@mozilla.org/network/io-service;1"]
                              .getService(C_i.nsIIOService);
  const resProt = ioService.getProtocolHandler("resource")
                           .QueryInterface(C_i.nsIResProtocolHandler);

  var curdir = do_get_file("js/src/xpconnect/tests/unit");
  var curURI = ioService.newFileURI(curdir);
  resProt.setSubstitution("test", curURI);

  test_BrokenFile("resource://test/bogus_exports_type.jsm", true, "Error");

  test_BrokenFile("resource://test/bogus_element_type.jsm", true, "Error");

  test_BrokenFile("resource://test/non_existing.jsm",
                  true,
                  "NS_ERROR_FILE_NOT_FOUND");

  test_BrokenFile("chrome://test/content/test.jsm",
                  true,
                  "NS_ERROR_ILLEGAL_VALUE");

  
  
  do_check_eq(typeof(Components.utils.import("resource://test/bogus_exports_type.jsm",
                                             null)),
              "object");

  do_check_eq(typeof(Components.utils.import("resource://test/bogus_element_type.jsm",
                                             null)),
              "object");

  resProt.setSubstitution("test", null);
}
