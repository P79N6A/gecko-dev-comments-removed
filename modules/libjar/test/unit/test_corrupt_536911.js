




































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

  
  var file = do_get_file("data/test_corrupt.zip");

  var zipreader = Cc["@mozilla.org/libjar/zip-reader;1"].
                  createInstance(Ci.nsIZipReader);
  zipreader.open(file);
  
  
  var failed = false;
  try {
    var stream = wrapInputStream(zipreader.getInputStream("file"));
    stream.read(1024);
  } catch (ex) {
    failed = true;
  }
  do_check_true(failed);
}

