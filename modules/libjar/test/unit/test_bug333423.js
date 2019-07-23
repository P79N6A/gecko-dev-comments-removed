








































function run_test() {
  const Cc = Components.classes;
  const Ci = Components.interfaces;

  
  var file = do_get_file("modules/libjar/test/unit/data/test_bug333423.zip");

  var zipreader = Cc["@mozilla.org/libjar/zip-reader;1"].
                  createInstance(Ci.nsIZipReader);
  zipreader.open(file);
  zipreader.close();
  var entries = zipreader.findEntries('*.*');
  do_check_true(!entries.hasMore()); 
}
