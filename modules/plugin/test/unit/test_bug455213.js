





































const Cc = Components.classes;
const Ci = Components.interfaces;


const CWD = do_get_cwd();
function checkOS(os) {
  const nsILocalFile_ = "nsILocalFile" + os;
  return nsILocalFile_ in Components.interfaces &&
         CWD instanceof Components.interfaces[nsILocalFile_];
}
const isMac = checkOS("Mac");


var DELIM = ":";
if ("@mozilla.org/windows-registry-key;1" in Components.classes)
  DELIM = "|";

var gProfD = do_get_profile();
var gDirSvc = Cc["@mozilla.org/file/directory_service;1"].
             getService(Ci.nsIProperties);


function write_registry(version, info) {
  var header = "Generated File. Do not edit.\n\n";
  header += "[HEADER]\n";
  header += "Version" + DELIM + version + DELIM + "$\n\n";
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


function get_test_plugintag() {
  var host = Cc["@mozilla.org/plugin/host;1"].
             getService(Ci.nsIPluginHost);
  var tags = host.getPluginTags({});
  for (var i = 0; i < tags.length; i++) {
    if (tags[i].name == "Test Plug-in")
      return tags[i];
  }
  return null;
}

function run_test() {
  var file = get_test_plugin();
  if (!file)
    do_throw("Plugin library not found");

  
  var registry = "";
  if (isMac) {
    registry += file.leafName + DELIM + "$\n";
    registry += file.path + DELIM + "$\n";
  } else {
    registry += file.path + DELIM + "$\n";
    registry += DELIM + "$\n";
  }
  registry += file.lastModifiedTime + DELIM + "0" + DELIM + "0" + DELIM + "$\n";
  registry += "Plug-in for testing purposes." + DELIM + "$\n";
  registry += "Test Plug-in" + DELIM + "$\n";
  registry += "1\n";
  registry += "0" + DELIM + "application/x-test" + DELIM + "Test mimetype" +
              DELIM + "tst" + DELIM + "$\n";
  write_registry("0.9", registry);

  var plugin = get_test_plugintag();
  if (!plugin)
    do_throw("Plugin tag not found");

  
  do_check_eq(plugin.version, "1.0.0.0");
  do_check_eq(plugin.description, "Plug-in for testing purposes.");
  
  do_check_true(plugin.disabled);
  do_check_false(plugin.blocklisted);
}
