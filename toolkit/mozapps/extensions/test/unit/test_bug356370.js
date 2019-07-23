




































const URI_EXTENSION_UPDATE_DIALOG = "chrome://mozapps/content/extensions/update.xul";



var WindowWatcher = {
  openWindow: function(parent, url, name, features, arguments) {
    
    do_check_eq(url, URI_EXTENSION_UPDATE_DIALOG);
  },

  QueryInterface: function(iid) {
    if (iid.equals(Components.interfaces.nsIWindowWatcher)
     || iid.equals(Components.interfaces.nsISupports))
      return this;

    throw Components.results.NS_ERROR_NO_INTERFACE;
  }
}

var WindowWatcherFactory = {
  createInstance: function createInstance(outer, iid) {
    if (outer != null)
      throw Components.results.NS_ERROR_NO_AGGREGATION;
    return WindowWatcher.QueryInterface(iid);
  }
};
var registrar = Components.manager.QueryInterface(Components.interfaces.nsIComponentRegistrar);
registrar.registerFactory(Components.ID("{1dfeb90a-2193-45d5-9cb8-864928b2af55}"),
                          "Fake Window Watcher",
                          "@mozilla.org/embedcomp/window-watcher;1",
                          WindowWatcherFactory);

function write_cache_line(stream, location, id, mtime) {
  var line = location + "\t" + id + "\trel%" + id + "\t" + Math.floor(mtime / 1000) + "\t\r\n";
  stream.write(line, line.length);
}














function setup_profile() {
  
  
  
  var source = do_get_file("data/test_bug356370.rdf");
  source.copyTo(gProfD, "extensions.rdf");

  
  
  var cache = gProfD.clone();
  cache.append("extensions.cache");
  var foStream = Components.classes["@mozilla.org/network/file-output-stream;1"]
                           .createInstance(Components.interfaces.nsIFileOutputStream);
  foStream.init(cache, 0x02 | 0x08 | 0x20, 0666, 0);  

  var addon = gProfD.clone();
  addon.append("extensions");
  addon.append("bug356370_1@tests.mozilla.org");
  source = do_get_file("data/test_bug356370_1.rdf");
  addon.create(Components.interfaces.nsIFile.DIRECTORY_TYPE, 0755);
  source.copyTo(addon, "install.rdf");
  write_cache_line(foStream, "app-profile", "bug356370_1@tests.mozilla.org",
                   addon.lastModifiedTime);

  addon = gProfD.clone();
  addon.append("extensions");
  addon.append("bug356370_2@tests.mozilla.org");
  source = do_get_file("data/test_bug356370_2.rdf");
  addon.create(Components.interfaces.nsIFile.DIRECTORY_TYPE, 0755);
  source.copyTo(addon, "install.rdf");
  write_cache_line(foStream, "app-profile", "bug356370_2@tests.mozilla.org",
                   addon.lastModifiedTime);

  addon = gProfD.clone();
  addon.append("extensions");
  addon.append("bug356370_4@tests.mozilla.org");
  source = do_get_file("data/test_bug356370_4.rdf");
  addon.create(Components.interfaces.nsIFile.DIRECTORY_TYPE, 0755);
  source.copyTo(addon, "install.rdf");

  
  write_cache_line(foStream, "invalid-lo", "bug356370_1@tests.mozilla.org", 0);
  write_cache_line(foStream, "invalid-hi", "bug356370_2@tests.mozilla.org", 0);
  write_cache_line(foStream, "invalid", "bug356370_3@tests.mozilla.org", 0);
  write_cache_line(foStream, "invalid", "bug356370_4@tests.mozilla.org", 0);
  foStream.close();
}

function run_test() {
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "1.9");
  gPrefs.setCharPref("extensions.lastAppVersion", "4");
  setup_profile();

  startupEM();
  do_check_neq(gEM.getItemForID("bug356370_1@tests.mozilla.org"), null);
  do_check_eq(getManifestProperty("bug356370_1@tests.mozilla.org", "installLocation"), "app-profile");
  do_check_neq(gEM.getItemForID("bug356370_2@tests.mozilla.org"), null);
  do_check_eq(getManifestProperty("bug356370_2@tests.mozilla.org", "installLocation"), "app-profile");
  
  do_check_eq(getManifestProperty("bug356370_2@tests.mozilla.org", "isDisabled"), "true");
  do_check_eq(gEM.getItemForID("bug356370_3@tests.mozilla.org"), null);
  do_check_neq(gEM.getItemForID("bug356370_4@tests.mozilla.org"), null);
  do_check_eq(getManifestProperty("bug356370_4@tests.mozilla.org", "installLocation"), "app-profile");

  gEM.installItemFromFile(do_get_addon("test_bug257155"), NS_INSTALL_LOCATION_APPPROFILE);
  do_check_neq(gEM.getItemForID("bug257155@tests.mozilla.org"), null);

  restartEM();
  do_check_neq(gEM.getItemForID("bug257155@tests.mozilla.org"), null);
  do_check_neq(gEM.getItemForID("bug356370_1@tests.mozilla.org"), null);
  do_check_neq(gEM.getItemForID("bug356370_2@tests.mozilla.org"), null);
  do_check_eq(gEM.getItemForID("bug356370_3@tests.mozilla.org"), null);
  do_check_neq(gEM.getItemForID("bug356370_4@tests.mozilla.org"), null);
}
