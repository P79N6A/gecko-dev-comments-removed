




































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

function run_test()
{
  
  var longLeafName = new Array(256).join("T");

  
  var tempFile = Cc["@mozilla.org/file/directory_service;1"].
                 getService(Ci.nsIProperties).get("TmpD", Ci.nsIFile);
  tempFile.append(longLeafName);
  tempFile.append("test.txt");

  try {
    tempFile.createUnique(Ci.nsIFile.NORMAL_FILE_TYPE, 0600);
    do_throw("Creating an item in a folder with a very long name should throw");
  }
  catch (e if (e instanceof Ci.nsIException &&
               e.result == Cr.NS_ERROR_FILE_UNRECOGNIZED_PATH)) {
    
  }
}
