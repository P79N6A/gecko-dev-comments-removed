





































function run_test() {
  const Cc = Components.classes;
  const Ci = Components.interfaces;

  
  var file = do_get_file("data/test_corrupt2.zip");

  var zipreader = Cc["@mozilla.org/libjar/zip-reader;1"].
                  createInstance(Ci.nsIZipReader);
  var failed = false;
  try {
    zipreader.open(file);
    
  } catch (ex) {
    failed = true;
  }
  do_check_true(failed);
}

