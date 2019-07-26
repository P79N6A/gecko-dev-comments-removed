function getTestPlugin(aName) {
  var pluginName = aName || "Test Plug-in";
  var ph = Components.classes["@mozilla.org/plugin/host;1"]
             .getService(Components.interfaces.nsIPluginHost);
  var tags = ph.getPluginTags();

  
  for (var i = 0; i < tags.length; i++) {
    if (tags[i].name == pluginName)
      return tags[i];
  }
  ok(false, "Unable to find plugin");
  return null;
}




function setTestPluginEnabledState(newEnabledState, pluginName) {
  var plugin = getTestPlugin(pluginName);
  var oldEnabledState = plugin.enabledState;
  plugin.enabledState = newEnabledState;
  SimpleTest.registerCleanupFunction(function() {
    getTestPlugin(pluginName).enabledState = oldEnabledState;
  });
}
