





































const Cr = Components.results;
const Ci = Components.interfaces;

const CC = Components.Constructor;
var LocalFile = CC("@mozilla.org/file/local;1", "nsILocalFile", "initWithPath");

function run_test()
{
  test_normalized_vs_non_normalized();
}

function test_normalized_vs_non_normalized()
{
  
  var dirProvider = Components.classes["@mozilla.org/file/directory_service;1"].getService(Ci.nsIProperties);
  var tmp1 = dirProvider.get("TmpD", Ci.nsILocalFile);
  var exists = tmp1.exists();
  do_check_true(exists);
  if (!exists)
    return;

  
  var tmp2 = new LocalFile(tmp1.path);
  do_check_true(tmp1.equals(tmp2));

  
  tmp2.appendRelativePath(".");
  do_check_false(tmp1.equals(tmp2));

  
  tmp2.normalize();
  do_check_true(tmp1.equals(tmp2));
}
