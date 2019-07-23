




































function run_test() {
  const Cc = Components.classes;
  const Ci = Components.interfaces;
  
  var file = do_get_file("/");

  var zipreader = Cc["@mozilla.org/libjar/zip-reader;1"].
                  createInstance(Ci.nsIZipReader);
  var failed = false;
  try {
    zipreader.open(file);
  } catch (e) {
    failed = true;
  }
  do_check_true(failed);
  zipreader = null;
}

