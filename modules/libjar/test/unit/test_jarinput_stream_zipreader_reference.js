



































function wrapInputStream(input)
{
  var nsIScriptableInputStream = Components.interfaces.nsIScriptableInputStream;
  var factory = Components.classes["@mozilla.org/scriptableinputstream;1"];
  var wrapper = factory.createInstance(nsIScriptableInputStream);
  wrapper.init(input);
  return wrapper;
}


function run_test() {
  const Cc = Components.classes;
  const Ci = Components.interfaces;

  
  var file = do_get_file("data/test_bug333423.zip");

  var zipreader = Cc["@mozilla.org/libjar/zip-reader;1"].
                  createInstance(Ci.nsIZipReader);
  zipreader.open(file);
  var entries = zipreader.findEntries(null);
  var stream = wrapInputStream(zipreader.getInputStream("modules/libjar/test/Makefile.in"))
  var dirstream= wrapInputStream(zipreader.getInputStream("modules/libjar/test/"))
  zipreader.close();
  zipreader = null;
  Components.utils.forceGC();
  do_check_true(stream.read(1024).length > 0);
  do_check_true(dirstream.read(100).length > 0);
}

