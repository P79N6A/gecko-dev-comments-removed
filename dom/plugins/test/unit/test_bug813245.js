




Components.utils.import("resource://gre/modules/Services.jsm");


var DELIM = ":";
if ("@mozilla.org/windows-registry-key;1" in Components.classes)
  DELIM = "|";

var gProfD = do_get_profile_startup();
var gDirSvc = Cc["@mozilla.org/file/directory_service;1"].
             getService(Ci.nsIProperties);


function write_registry(version, info) {
  let runtime = Cc["@mozilla.org/xre/runtime;1"].getService(Ci.nsIXULRuntime);

  var header = "Generated File. Do not edit.\n\n";
  header += "[HEADER]\n";
  header += "Version" + DELIM + version + DELIM + "$\n";
  header += "Arch" + DELIM + runtime.XPCOMABI + DELIM + "$\n";
  header += "\n";
  header += "[PLUGINS]\n";

  var registry = gProfD.clone();
  registry.append("pluginreg.dat");
  var foStream = Components.classes["@mozilla.org/network/file-output-stream;1"]
                           .createInstance(Components.interfaces.nsIFileOutputStream);
  
  foStream.init(registry, 0x02 | 0x08 | 0x20, 0666, 0);

  var charset = "UTF-8"; 
  var os = Cc["@mozilla.org/intl/converter-output-stream;1"].
           createInstance(Ci.nsIConverterOutputStream);
  os.init(foStream, charset, 0, 0x0000);

  os.writeString(header);
  os.writeString(info);
  os.close();
}

function run_test() {
  var plugin = get_test_plugintag();
  do_check_true(plugin == null);

  var file = get_test_plugin();
  if (!file) {
    do_throw("Plugin library not found");
  }

  
  let registry = "";

  if (gIsOSX) {
    registry += file.leafName + DELIM + "$\n";
    registry += file.path + DELIM + "$\n";
  } else {
    registry += file.path + DELIM + "$\n";
    registry += DELIM + "$\n";
  }
  registry += "0.0.0.0" + DELIM + "$\n";
  registry += "16725225600" + DELIM + "0" + DELIM + "5" + DELIM + "$\n";
  registry += "Plug-in for testing purposes." + DELIM + "$\n";
  registry += "Test Plug-in" + DELIM + "$\n";
  registry += "999999999999999999999999999999999999999999999999|0|5|$\n";
  registry += "0" + DELIM + "application/x-test" + DELIM + "Test mimetype" +
              DELIM + "tst" + DELIM + "$\n";

  registry += "\n";
  registry += "[INVALID]\n";
  registry += "\n";
  write_registry("0.15", registry);

  
  do_get_profile();

  var plugin = get_test_plugintag();
  if (!plugin)
    do_throw("Plugin tag not found");

  
  
  do_check_eq(plugin.version, "1.0.0.0");

  
  Services.prefs.clearUserPref("plugin.importedState");
}
