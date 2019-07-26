




const Cc = Components.classes;
const Ci = Components.interfaces;

const gIsWindows = ("@mozilla.org/windows-registry-key;1" in Cc);
const gIsOSX = ("nsILocalFileMac" in Ci);
const gIsLinux = ("@mozilla.org/gnome-gconf-service;1" in Cc);


function get_test_plugin() {
  var pluginEnum = gDirSvc.get("APluginsDL", Ci.nsISimpleEnumerator);
  while (pluginEnum.hasMoreElements()) {
    let dir = pluginEnum.getNext().QueryInterface(Ci.nsILocalFile);
    let plugin = dir.clone();
    
    plugin.append("Test.plugin");
    if (plugin.exists()) {
      plugin.normalize();
      return plugin;
    }
    plugin = dir.clone();
    
    plugin.append("libnptest.so");
    if (plugin.exists()) {
      plugin.normalize();
      return plugin;
    }
    
    plugin = dir.clone();
    plugin.append("nptest.dll");
    if (plugin.exists()) {
      plugin.normalize();
      return plugin;
    }
  }
  return null;
}


function get_test_plugintag() {
  const Cc = Components.classes;
  const Ci = Components.interfaces;

  var host = Cc["@mozilla.org/plugin/host;1"].
             getService(Ci.nsIPluginHost);
  var tags = host.getPluginTags();

  for (var i = 0; i < tags.length; i++) {
    if (tags[i].name == "Test Plug-in")
      return tags[i];
  }
  return null;
}


function do_get_profile_startup() {
  let env = Components.classes["@mozilla.org/process/environment;1"]
                      .getService(Components.interfaces.nsIEnvironment);
  
  let profd = env.get("XPCSHELL_TEST_PROFILE_DIR");
  let file = Components.classes["@mozilla.org/file/local;1"]
                       .createInstance(Components.interfaces.nsILocalFile);
  file.initWithPath(profd);

  let dirSvc = Components.classes["@mozilla.org/file/directory_service;1"]
                         .getService(Components.interfaces.nsIProperties);
  let provider = {
    getFile: function(prop, persistent) {
      persistent.value = true;
      if (prop == "ProfDS") {
        return file.clone();
      }
      throw Components.results.NS_ERROR_FAILURE;
    },
    QueryInterface: function(iid) {
      if (iid.equals(Components.interfaces.nsIDirectoryServiceProvider) ||
          iid.equals(Components.interfaces.nsISupports)) {
        return this;
      }
      throw Components.results.NS_ERROR_NO_INTERFACE;
    }
  };
  dirSvc.QueryInterface(Components.interfaces.nsIDirectoryService)
        .registerProvider(provider);
  return file.clone();
}

function get_platform_specific_plugin_name() {
  if (gIsWindows) return "nptest.dll";
  else if (gIsOSX) return "Test.plugin";
  else if (gIsLinux) return "libnptest.so";
  else return null;
}

function get_platform_specific_plugin_suffix() {
  if (gIsWindows) return ".dll";
  else if (gIsOSX) return ".plugin";
  else if (gIsLinux) return ".so";
  else return null;
}

function get_test_plugin_no_symlink() {
  let dirSvc = Cc["@mozilla.org/file/directory_service;1"]
                .getService(Ci.nsIProperties);
  let pluginEnum = dirSvc.get("APluginsDL", Ci.nsISimpleEnumerator);
  while (pluginEnum.hasMoreElements()) {
    let dir = pluginEnum.getNext().QueryInterface(Ci.nsILocalFile);
    let plugin = dir.clone();
    plugin.append(get_platform_specific_plugin_name());
    if (plugin.exists()) {
      return plugin;
    }
  }
  return null;
}
