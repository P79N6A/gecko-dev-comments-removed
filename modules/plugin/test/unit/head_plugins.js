






































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
