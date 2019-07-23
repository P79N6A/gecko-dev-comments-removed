





































const Cc = Components.classes;
const Ci = Components.interfaces;

const NS_APP_USER_PROFILE_50_DIR      = "ProfD";
const NS_APP_PROFILE_DIR_STARTUP      = "ProfDS";

var gProfD;
var gDirSvc = Cc["@mozilla.org/file/directory_service;1"].
             getService(Ci.nsIProperties);



function createProfileFolder() {
  gProfD = gDirSvc.get("CurProcD", Ci.nsILocalFile);
  gProfD = gProfD.parent.parent;
  gProfD.append("_tests");
  gProfD.append("xpcshell-simple");
  gProfD.append("test_plugin");
  gProfD.append("profile");
  
  if (gProfD.exists())
    gProfD.remove(true);
  gProfD.create(Components.interfaces.nsIFile.DIRECTORY_TYPE, 0755);
  
  var dirProvider = {
    getFile: function(prop, persistent) {
      persistent.value = true;
      if (prop == NS_APP_USER_PROFILE_50_DIR ||
          prop == NS_APP_PROFILE_DIR_STARTUP)
        return gProfD.clone();
      return null;
    },
    QueryInterface: function(iid) {
      if (iid.equals(Ci.nsIDirectoryServiceProvider) ||
          iid.equals(Ci.nsISupports)) {
        return this;
      }
      throw Components.results.NS_ERROR_NO_INTERFACE;
    }
  };
  gDirSvc.QueryInterface(Ci.nsIDirectoryService)
         .registerProvider(dirProvider);
}


function write_registry(version, info) {
  var header = "Generated File. Do not edit.\n\n";
  header += "[HEADER]\n";
  header += "Version:" + version + ":$\n\n";
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


function get_test_plugin() {
  var plugins = gDirSvc.get("CurProcD", Ci.nsILocalFile);
  plugins.append("plugins");
  do_check_true(plugins.exists());
  var plugin = plugins.clone();
  
  plugin.append("Test.plugin");
  if (plugin.exists()) {
    plugin.normalize();
    return plugin;
  }
  plugin = plugins.clone();
  
  plugin.append("libnptest.so");
  if (plugin.exists()) {
    plugin.normalize();
    return plugin;
  }
  
  plugin = plugins.clone();
  plugin.append("nptest.dll");
  if (plugin.exists()) {
    plugin.normalize();
    return plugin;
  }
  return null;
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
  createProfileFolder();
  var file = get_test_plugin();
  if (!file)
    do_throw("Plugin library not found");

  
  var registry = "";
  registry += file.leafName + ":$\n";
  registry += file.path + ":$\n";
  registry += file.lastModifiedTime + ":0:0" + ":$\n";
  registry += "Plug-in for testing purposes." + ":$\n";
  registry += "Test Plug-in" + ":$\n";
  registry += "1\n";
  registry += "0:application/x-test:Test mimetype:tst" + ":$\n";
  write_registry("0.9", registry);

  var plugin = get_test_plugintag();
  if (!plugin)
    do_throw("Plugin tag not found");

  
  do_check_eq(plugin.version, "1.0.0.0");
  do_check_eq(plugin.description, "Plug-in for testing purposes.");
  
  do_check_true(plugin.disabled);
  do_check_false(plugin.blocklisted);

  try {
    gProfD.remove(true);
  }
  catch (e) {
    
  }
}
