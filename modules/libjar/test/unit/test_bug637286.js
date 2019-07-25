




































const Cc = Components.classes;
const Ci = Components.interfaces;


var ios = Cc["@mozilla.org/network/io-service;1"].
          getService(Ci.nsIIOService);

function open_inner_zip(base, idx) {
    var spec = "jar:" + base + "inner" + idx + ".zip!/foo";
    var channel = ios.newChannel(spec, null, null);
    var stream = channel.open();
}

function run_test() {
  var file = do_get_file("data/test_bug637286.zip");
  var outerJarBase = "jar:" + ios.newFileURI(file).spec + "!/";

  for (var i = 0; i < 40; i++) {
    open_inner_zip(outerJarBase, i);
    gc();
  }
}

